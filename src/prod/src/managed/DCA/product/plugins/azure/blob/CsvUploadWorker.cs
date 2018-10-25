// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Fabric.Dca.LttConsumerHelper;
    using System.Globalization;
    using System.IO;
    using System.Fabric.Common;
    using System.Linq;
    using System.Security;

    
    // This class implements the logic to upload files containing LTT
    // traces to Azure blob storage.
    internal class CsvUploadWorker : IDisposable
    {
        // Constants
        private const string CsvDirName = "Log";
        private const string V2SubFolderSuffixMapFileName = "SubFolderSuffixMap.dat";
        private const int V2SubFolderSuffixMapFormatVersion = 1;
        private const string V1CsvDirName = "ForUpload";
        private const string V2CsvDirNamePrefix = "Csv_";

        // Parameters passed to ETW upload worker object during initialization
        internal struct CsvUploadWorkerParameters
        {
            internal FabricEvents.ExtensionsEvents TraceSource;
            internal string FabricNodeId;
            internal string FabricNodeInstanceName;
            internal bool IsReadingFromApplicationManifest;
            internal bool HandleUpgradeFromV1;
            internal string LogDirectory;
            internal string WorkDirectory;
            internal string UploaderInstanceId;
            internal string ParentWorkFolderName;
            internal AzureBlobCsvUploader.BlobUploadSettings Settings;
        }

        // Initialization parameters
        private CsvUploadWorkerParameters initParam;

        // Settings for uploading data to blob storage
        private AzureBlobCsvUploader.BlobUploadSettings blobUploadSettings;
        
        // Tag used to represent the source of the log message
        private string logSourceId;
        
        // Object used for tracing
        private FabricEvents.ExtensionsEvents traceSource;

        // String that represents the destination account and container
        private string destinationKey;

        // Object that implements miscellaneous utility functions
        private AzureUtility azureUtility;

        // ETL to CSV converter interface reference
        private CsvToUploadFolderWriter csvToUploadFolderWriter;

        // Folder to hold CSV files containing ETW events
        private string csvFolder;

        // Helper object for uploading the CSV files to Azure blob storage
        private AzureFileUploader uploader;

        // Work folder used by the Azure blob uploader
        private string workFolder;

        // Whether or not the object has been disposed
        bool disposed;

        // Whether or not we should finish processing all pending data 
        // during dispose
        internal bool FlushDataOnDispose { set; private get; }

        internal CsvUploadWorker(CsvUploadWorkerParameters initParam, DiskSpaceManager diskSpaceManager)
        {
            // Initialization
            this.FlushDataOnDispose = false;
            this.initParam = initParam;
            this.logSourceId = this.initParam.UploaderInstanceId;
            this.traceSource = this.initParam.TraceSource;
            this.blobUploadSettings = this.initParam.Settings;
            this.azureUtility = new AzureUtility(this.traceSource, this.logSourceId);
            this.destinationKey = String.Join(
                                        "_",
                                        new string[]
                                        {
                                            StandardPluginTypes.AzureBlobCsvUploader,
                                            this.blobUploadSettings.StorageAccountFactory.Connection.UseDevelopmentStorage ?
                                                AzureConstants.DevelopmentStorageConnectionString :
                                                this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                                            this.blobUploadSettings.LttTraceContainerName
                                        });
            this.disposed = false;

            // Create a sub-directory for ourselves under the log directory
            bool success = GetCsvSubDirectory();

            string sourceDirectory = Path.Combine(initParam.LogDirectory, "Traces");
            
            // Create the helper object that writes events delivered from the LTT
            // files into CSV files.
            if (success)
            {
                this.csvToUploadFolderWriter = new CsvToUploadFolderWriter(
                                          this.logSourceId,
                                          this.initParam.FabricNodeId,
                                          this.csvFolder,
                                          sourceDirectory,
                                          diskSpaceManager,
                                          false);
                if (null == this.csvToUploadFolderWriter)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Failed to create CSV to Upload Foler writer helper object.");
                    success = false;
                }
            }

            if (success)
            {
                // Create a sub-directory for the uploader under the log directory
                success = GetUploaderWorkSubDirectory();
            }

            if (this.blobUploadSettings.Enabled)
            {
                // Create and initialize the uploader
                //
                // NOTE: By specifying 'true' for the 'filterDeletionByNodeId' parameter,
                // we only delete those blobs that were uploaded by the current node. We
                // identify this via the Fabric node ID that the ETL-to-CSV writer prefixed
                // to the file name before uploading. This is done so that all nodes don't
                // wastefully try to delete all blobs.
                try
                {
                    var destinationPath = string.Concat(
                          this.blobUploadSettings.StorageAccountFactory.Connection.UseDevelopmentStorage
                              ? AzureConstants.DevelopmentStorageConnectionString
                              : this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                          ";", // This separator cannot occur in account name or container name
                          this.blobUploadSettings.LttTraceContainerName);

                    this.uploader = new AzureFileUploader(
                        this.traceSource,
                        this.logSourceId,
                        this.csvFolder,
                        destinationPath,
                        this.workFolder,
                        this.blobUploadSettings.StorageAccountFactory,
                        this.blobUploadSettings.LttTraceContainerName,
                        this.blobUploadSettings.UploadInterval,
                        this.blobUploadSettings.FileSyncInterval,
                        this.blobUploadSettings.BlobDeletionAge,
                        this.initParam.FabricNodeInstanceName,
                        this.blobUploadSettings.DeploymentId);

                    this.uploader.Start();
                }
                catch (Exception ex)
                {
                    throw new InvalidOperationException("AzureFileUploader could not be constructed.", ex);
                }

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Upload to blob storage is configured. Storage account: {0}, Trace container: {1}, Local trace Path: {2}," +
                    "Upload interval (minutes): {3}",
                    this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                    this.blobUploadSettings.LttTraceContainerName,
                    this.csvFolder,
                    this.blobUploadSettings.UploadInterval);
            }
            else
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Upload to blob storage is disabled (Storage key not available). Only log age management is enabled."
                    + "Local trace Path: {0}",
                    this.csvFolder
                    );
            }
        }

        internal object GetDataSink()
        {
            return null;
        }

        private bool ShouldUpdateFilter(AzureBlobCsvUploader.BlobUploadSettings newSettings)
        {
            bool updateNeeded = false;
            if (false == newSettings.Filter.Equals(this.blobUploadSettings.Filter, StringComparison.Ordinal))
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Settings change detected. Current LTT event filter: {0}, new ETW event filter: {1}.",
                    this.blobUploadSettings.Filter,
                    newSettings.Filter);
                updateNeeded = true;
            }
            return updateNeeded;
        }

        private bool AreAccountKeysEqual(SecureString key1, SecureString key2)
        {
            bool equal = false;
            byte[] key1Bytes = null;
            byte[] key2Bytes = null;
            try
            {
                key1Bytes = Utility.SecureStringToByteArray(key1);
                key2Bytes = Utility.SecureStringToByteArray(key2);
                equal = Enumerable.SequenceEqual(key1Bytes, key2Bytes);
            }
            finally
            {
                if (null != key1Bytes)
                {
                    Array.Clear(key1Bytes, 0, key1Bytes.Length);
                }
                if (null != key2Bytes)
                {
                    Array.Clear(key2Bytes, 0, key2Bytes.Length);
                }
            }
            return equal;
        }

        private bool ShouldUpdateUploader(AzureBlobCsvUploader.BlobUploadSettings newSettings)
        {
            bool updateNeeded = false;

            if (false == this.blobUploadSettings.StorageAccountFactory.Connection.UseDevelopmentStorage)
            {
                if (false == AreAccountKeysEqual(
                                newSettings.StorageAccountFactory.Connection.AccountKey, 
                                this.blobUploadSettings.StorageAccountFactory.Connection.AccountKey))
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Settings change detected. Account key has changed.");
                    updateNeeded = true;
                }
            }

            if (newSettings.UploadInterval != this.blobUploadSettings.UploadInterval)
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Settings change detected. Current upload interval (minutes): {0}, new upload interval (minutes): {1}.",
                    this.blobUploadSettings.UploadInterval,
                    newSettings.UploadInterval);
                updateNeeded = true;
            }

            if (newSettings.BlobDeletionAge != this.blobUploadSettings.BlobDeletionAge)
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Settings change detected. Current file deletion age (minutes): {0}, new file deletion age (minutes): {1}.",
                    this.blobUploadSettings.BlobDeletionAge,
                    newSettings.BlobDeletionAge);
                updateNeeded = true;
            }

            return updateNeeded;
        }

        internal void UpdateSettings(AzureBlobCsvUploader.BlobUploadSettings newSettings)
        {
            bool updateFilter = ShouldUpdateFilter(newSettings);
            bool updateUploader = ShouldUpdateUploader(newSettings);

            if (updateFilter || updateUploader)
            {
                this.blobUploadSettings = newSettings;
            }

            if (updateUploader)
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Due to settings change, the uploader will be stopped and restarted.");

                // Stop the uploader
                if (null != this.uploader)
                {
                    this.uploader.Dispose();
                }

                // Restart the upload with the new settings
                try
                {
                    var destinationPath = string.Concat(
                          this.blobUploadSettings.StorageAccountFactory.Connection.UseDevelopmentStorage
                              ? AzureConstants.DevelopmentStorageConnectionString
                              : this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                          ";", // This separator cannot occur in account name or container name
                          this.blobUploadSettings.LttTraceContainerName);

                    this.uploader = new AzureFileUploader(
                        this.traceSource,
                        this.logSourceId,
                        this.csvFolder,
                        destinationPath,
                        this.workFolder,
                        this.blobUploadSettings.StorageAccountFactory,
                        this.blobUploadSettings.LttTraceContainerName,
                        this.blobUploadSettings.UploadInterval,
                        this.blobUploadSettings.FileSyncInterval,
                        this.blobUploadSettings.BlobDeletionAge,
                        this.initParam.FabricNodeInstanceName,
                        this.blobUploadSettings.DeploymentId);

                    this.uploader.Start();
                }
                catch (Exception ex)
                {
                    throw new InvalidOperationException("AzureFileUploader could not be constructed.", ex);
                }
            }
        }

        public void FlushData()
        {
            if (null != this.uploader)
            {
                this.uploader.FlushData();
            }
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }
            this.disposed = true;
            
            if (null != this.uploader)
            {
                // Tell the uploader to stop
                this.uploader.Dispose();
            }

            if (this.csvToUploadFolderWriter != null)
            {
                this.csvToUploadFolderWriter.Dispose();
            }

            GC.SuppressFinalize(this);
            return;
        }

        internal void SetLogFolders(string folderName)
        {
            this.csvToUploadFolderWriter.SetSourceLogFolders(folderName);
        }

        private bool GetCsvSubDirectory()
        {
            string CsvParentFolder = Path.Combine(
                                            this.initParam.WorkDirectory,
                                            this.initParam.ParentWorkFolderName);
            bool success = Utility.CreateWorkSubDirectory(
                                this.traceSource,
                                this.logSourceId,
                                this.destinationKey,
                                String.Concat(Utility.CsvConsumerWorkSubFolderIdPrefix, this.logSourceId),
                                CsvParentFolder,
                                out this.csvFolder);
            if (success)
            {
                this.csvFolder = Path.Combine(this.csvFolder, CsvDirName);
            }
            return success;
        }

        private bool GetCsvSubDirectoryOld()
        {
            bool success = true;
            this.csvFolder = null;

            if (this.initParam.IsReadingFromApplicationManifest)
            {
                string CsvParentFolder = Path.Combine(
                                                this.initParam.LogDirectory,
                                                this.initParam.UploaderInstanceId);
                string subFolderIndex;
                success = Utility.GetSubDirectoryNameFromMap(
                            this.traceSource,
                            this.logSourceId,
                            String.Concat(
                                this.blobUploadSettings.StorageAccountFactory.Connection.UseDevelopmentStorage ?
                                    AzureConstants.DevelopmentStorageConnectionString :
                                    this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                                this.blobUploadSettings.LttTraceContainerName),
                            CsvParentFolder, 
                            V2SubFolderSuffixMapFileName,
                            V2SubFolderSuffixMapFormatVersion,
                            out subFolderIndex);
                if (success)
                {
                    string csvSubDirName = String.Concat(
                                               V2CsvDirNamePrefix,
                                               subFolderIndex);
                    this.csvFolder = Path.Combine(CsvParentFolder, csvSubDirName);
                }
            }
            else
            {
                if (this.initParam.HandleUpgradeFromV1)
                {
                    // Compute the folder name that the v1 binary would have used
                    // for storing ETW CSV files
                    string existingSubdirectoryPath = Path.Combine(
                                                          this.initParam.LogDirectory,
                                                          V1CsvDirName);
                    // Check if that folder exists
                    if (FabricDirectory.Exists(existingSubdirectoryPath))
                    {
                        // Folder exists, so let's use that
                        this.csvFolder = existingSubdirectoryPath;
                    }
                }

                if (null == this.csvFolder)
                {
                    // Either we don't need to handle upgrade from v1 or the folder
                    // used by the v1 binary to store ETW CSV files does not exist.
                    // So let's create a new folder.
                    string csvSubDirName = String.Concat(
                                               V2CsvDirNamePrefix,
                                               this.initParam.UploaderInstanceId);
                    this.csvFolder = Path.Combine(
                                           this.initParam.LogDirectory,
                                           csvSubDirName);
                }
            }

            return success;
        }

        private bool GetUploaderWorkSubDirectory()
        {
            string workParentFolder = Path.Combine(
                                          this.initParam.WorkDirectory,
                                          this.initParam.ParentWorkFolderName);
            bool success = Utility.CreateWorkSubDirectory(
                               this.traceSource,
                               this.logSourceId,
                               this.destinationKey,
                               this.csvFolder,
                               workParentFolder,
                               out this.workFolder);
            if (success)
            {
                this.workFolder = Path.Combine(this.workFolder, AzureConstants.AzureFileUploaderFolder);
            }
            return success;
        }
    }
}
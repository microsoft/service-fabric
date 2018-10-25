// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Diagnostics;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Fabric.Dca.EtlConsumerHelper;
    using System.IO;
    using System.Linq;
    using System.Security;

    // This class implements the logic to upload files containing filtered ETW
    // traces to Azure blob storage.
    internal class EtwCsvUploadWorker : IDisposable
    {
        // Constants
        private const string CsvDirName = "Log";
        
        // Tag used to represent the source of the log message
        private readonly string logSourceId;
        
        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // String that represents the destination account and container
        private readonly string destinationKey;

        // ETL to CSV converter interface reference
        private readonly EtlToCsvFileWriter etlToCsvWriter;

        // Initialization parameters
        private EtwCsvUploadWorkerParameters initParam;

        // Settings for uploading data to blob storage
        private AzureBlobEtwCsvUploader.BlobUploadSettings blobUploadSettings;

        // Folder to hold CSV files containing ETW events
        private string etwCsvFolder;

        // Helper object for uploading the CSV files to Azure blob storage
        private AzureFileUploader uploader;

        // Work folder used by the Azure blob uploader
        private string workFolder;

        // Whether or not the object has been disposed
        private bool disposed;

        public EtwCsvUploadWorker(EtwCsvUploadWorkerParameters initParam)
        {
            // Initialization
            this.initParam = initParam;
            this.logSourceId = this.initParam.UploaderInstanceId;
            this.traceSource = this.initParam.TraceSource;
            this.blobUploadSettings = this.initParam.Settings;
            var accountName = this.blobUploadSettings.StorageAccountFactory.Connection.UseDevelopmentStorage
                ? AzureConstants.DevelopmentStorageConnectionString
                : this.blobUploadSettings.StorageAccountFactory.Connection.AccountName;
            this.destinationKey = string.Join(
                                        "_",
                                        StandardPluginTypes.AzureBlobEtwCsvUploader,
                                        accountName,
                                        this.blobUploadSettings.EtwTraceContainerName);

            // Create a sub-directory for ourselves under the log directory
            bool success = this.GetEtwCsvSubDirectory();

            // Create the helper object that writes events delivered from the ETL
            // files into CSV files.
            if (success)
            {
                this.etlToCsvWriter = new EtlToCsvFileWriter(
                    new TraceEventSourceFactory(),
                    this.logSourceId,
                    this.initParam.FabricNodeId,
                    this.etwCsvFolder,
                    false,
                    initParam.DiskSpaceManager);

                // Set the event filter
                this.etlToCsvWriter.SetEtwEventFilter(
                    this.blobUploadSettings.Filter,
                    this.initParam.IsReadingFromApplicationManifest ? string.Empty : WinFabDefaultFilter.StringRepresentation,
                    this.initParam.IsReadingFromApplicationManifest ? string.Empty : WinFabSummaryFilter.StringRepresentation,
                    !this.initParam.IsReadingFromApplicationManifest);

                // Create a sub-directory for the uploader under the log directory
                success = this.GetUploaderWorkSubDirectory();
            }

            if (success)
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
                        this.blobUploadSettings.EtwTraceContainerName);
                    this.uploader = new AzureFileUploader(
                        this.traceSource,
                        this.logSourceId,
                        this.etwCsvFolder,
                        destinationPath,
                        this.workFolder,
                        this.blobUploadSettings.StorageAccountFactory,
                        this.blobUploadSettings.EtwTraceContainerName,
                        this.blobUploadSettings.UploadInterval,
                        this.blobUploadSettings.FileSyncInterval,
                        this.blobUploadSettings.BlobDeletionAge,
                        this.initParam.FabricNodeInstanceName,
                        this.blobUploadSettings.DeploymentId);
                    this.uploader.Start();
                }
                catch (Exception e)
                {
                    throw new InvalidOperationException("AzureFileUploader could not be constructed.", e);
                }
            }

            if (success)
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Upload to blob storage is configured. Storage account: {0}, Trace container: {1}, Local trace Path: {2}, Upload interval (minutes): {3}",
                    this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                    this.blobUploadSettings.EtwTraceContainerName,
                    this.etwCsvFolder,
                    this.blobUploadSettings.UploadInterval.TotalMinutes);
            }
            else
            {
                if (null != this.etlToCsvWriter)
                {
                    this.etlToCsvWriter.Dispose();
                    this.etlToCsvWriter = null;
                }
            }

            if (!success)
            {
                throw new InvalidOperationException();
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

            if (null != this.etlToCsvWriter)
            {
                // Tell the ETL-to-CSV writer to stop
                this.etlToCsvWriter.Dispose();
            }

            if (null != this.uploader)
            {
                // Tell the uploader to stop
                this.uploader.Dispose();
            }

            GC.SuppressFinalize(this);
        }

        internal object GetDataSink()
        {
            // Return a sink if we are enabled, otherwise return null.
            return this.blobUploadSettings.Enabled ? (IEtlFileSink)this.etlToCsvWriter : null;
        }

        internal void UpdateSettings(AzureBlobEtwCsvUploader.BlobUploadSettings newSettings)
        {
            bool updateFilter = this.ShouldUpdateFilter(newSettings);
            bool updateUploader = this.ShouldUpdateUploader(newSettings);

            if (updateFilter || updateUploader)
            {
                this.blobUploadSettings = newSettings;
            }

            if (updateFilter)
            {
                if (null != this.etlToCsvWriter)
                {
                    // Update the filter
                    Debug.Assert(this.initParam.IsReadingFromApplicationManifest, "Updating the trace filter is only allowed for applications.");
                    this.etlToCsvWriter.SetEtwEventFilter(
                        this.blobUploadSettings.Filter,
                        string.Empty,
                        string.Empty,
                        false);
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "ETW event filter has been updated due to settings change. New ETW event filter: {0}.",
                        this.blobUploadSettings.Filter);
                }
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
                        this.blobUploadSettings.EtwTraceContainerName);
                    this.uploader = new AzureFileUploader(
                        this.traceSource,
                        this.logSourceId,
                        this.etwCsvFolder,
                        destinationPath,
                        this.workFolder,
                        this.blobUploadSettings.StorageAccountFactory,
                        this.blobUploadSettings.EtwTraceContainerName,
                        this.blobUploadSettings.UploadInterval,
                        this.blobUploadSettings.FileSyncInterval,
                        this.blobUploadSettings.BlobDeletionAge,
                        this.initParam.FabricNodeInstanceName,
                        this.blobUploadSettings.DeploymentId);
                    this.uploader.Start();
                }
                catch (Exception)
                {
                    this.uploader = null;
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Failed to restart uploader.");
                }
            }
        }

        private static bool AreAccountKeysEqual(SecureString key1, SecureString key2)
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

        private bool ShouldUpdateFilter(AzureBlobEtwCsvUploader.BlobUploadSettings newSettings)
        {
            bool updateNeeded = false;
            if (false == newSettings.Filter.Equals(this.blobUploadSettings.Filter, StringComparison.Ordinal))
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Settings change detected. Current ETW event filter: {0}, new ETW event filter: {1}.",
                    this.blobUploadSettings.Filter,
                    newSettings.Filter);
                updateNeeded = true;
            }

            return updateNeeded;
        }

        private bool ShouldUpdateUploader(AzureBlobEtwCsvUploader.BlobUploadSettings newSettings)
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

        private bool GetEtwCsvSubDirectory()
        {
            string etwCsvParentFolder = Path.Combine(
                                            this.initParam.WorkDirectory,
                                            this.initParam.ParentWorkFolderName);
            bool success = Utility.CreateWorkSubDirectory(
                                this.traceSource,
                                this.logSourceId,
                                this.destinationKey,
                                string.Concat(Utility.EtwConsumerWorkSubFolderIdPrefix, this.logSourceId),
                                etwCsvParentFolder,
                                out this.etwCsvFolder);
            if (success)
            {
                this.etwCsvFolder = Path.Combine(this.etwCsvFolder, CsvDirName);
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
                               this.etwCsvFolder,
                               workParentFolder,
                               out this.workFolder);
            if (success)
            {
                this.workFolder = Path.Combine(this.workFolder, AzureConstants.AzureFileUploaderFolder);
            }

            return success;
        }

        // Parameters passed to ETW upload worker object during initialization
        internal struct EtwCsvUploadWorkerParameters
        {
            internal FabricEvents.ExtensionsEvents TraceSource;
            internal string FabricNodeId;
            internal string FabricNodeInstanceName;
            internal bool IsReadingFromApplicationManifest;
            internal string LogDirectory;
            internal string WorkDirectory;
            internal string UploaderInstanceId;
            internal string ParentWorkFolderName;
            internal AzureBlobEtwCsvUploader.BlobUploadSettings Settings;
            internal DiskSpaceManager DiskSpaceManager;
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.IO;
    using System.Linq;
    using System.Globalization;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Diagnostics;
    using System.Security;
    using System.Fabric.Dca;

    // This class implements a wrapper around the worker object that uploads files
    // containing filtered ETW traces to Azure blob storage
    internal class AzureBlobCsvUploader : IDcaConsumer, IFolderSink
    {
        // List of worker objects currently available
        private static List<UploadWorkerInfo> UploadWorkers = new List<UploadWorkerInfo>();

        // Tag used to represent the source of the log message
        private string logSourceId;

        // Object used for tracing
        private FabricEvents.ExtensionsEvents traceSource;

        // Configuration reader
        private readonly AzureBlobConfigReader configReader;

        // Worker object that performs the upload of ETW events to Azure blob storage
        private readonly CsvUploadWorker uploadWorker;

        // Settings for uploading data to blob storage
        private readonly BlobUploadSettings blobUploadSettings;
        
        // Whether or not the object has been disposed
        private bool disposed;
        
        // Disk space manager instance.
        private readonly DiskSpaceManager diskSpaceManager;
        
        public AzureBlobCsvUploader(ConsumerInitializationParameters initParam)
        {
            // Initialization
            this.logSourceId = string.Concat(initParam.ApplicationInstanceId, "_", initParam.SectionName);
            this.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            this.configReader = new AzureBlobConfigReader(
                new ConfigReader(initParam.ApplicationInstanceId),
                initParam.SectionName,
                this.traceSource,
                this.logSourceId);
            this.diskSpaceManager = initParam.DiskSpaceManager;
            this.disposed = false;

            // Read blob-specific settings from settings.xml
            this.blobUploadSettings = this.GetSettings();
            
            if (this.configReader.IsReadingFromApplicationManifest)
            {
                // Check if we can use an existing upload worker object
                UploadWorkerKey key = new UploadWorkerKey()
                {
                    // Destination path is an concatenation of storage account name and container name
                    DestinationPath = string.Concat(
                                        this.blobUploadSettings.StorageAccountFactory.Connection.UseDevelopmentStorage ?
                                          AzureConstants.DevelopmentStorageConnectionString :
                                          this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                                          ";", // This separator cannot occur in account name or container name
                                        this.blobUploadSettings.LttTraceContainerName),
                    ApplicationType = this.configReader.GetApplicationType(),
                };
                lock (UploadWorkers)
                {
                    UploadWorkerInfo workerInfo = UploadWorkers.FirstOrDefault(w => w.Matches(key));
                    if (null != workerInfo)
                    {
                        // Existing upload worker object is available. Increment its
                        // reference count
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Existing upload worker object for application type {0}, Azure storage account {1} and container {2} is available and will be used.",
                            key.ApplicationType,
                            this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                            this.blobUploadSettings.LttTraceContainerName);

                        (workerInfo.RefCount)++;
                        workerInfo.UploadWorker.UpdateSettings(this.blobUploadSettings);
                        this.uploadWorker = workerInfo.UploadWorker;
                    }
                    else
                    {
                        // Create a new upload worker object
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Creating upload worker object for application type {0}, Azure storage account {1} and container {2} ...",
                            key.ApplicationType,
                            this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                            this.blobUploadSettings.LttTraceContainerName);

                        CsvUploadWorker.CsvUploadWorkerParameters param = new CsvUploadWorker.CsvUploadWorkerParameters()
                        {
                            TraceSource = this.traceSource,
                            FabricNodeId = initParam.FabricNodeId,
                            FabricNodeInstanceName = initParam.FabricNodeInstanceName,
                            IsReadingFromApplicationManifest = this.configReader.IsReadingFromApplicationManifest,
                            LogDirectory = initParam.LogDirectory,
                            WorkDirectory = initParam.WorkDirectory,
                            UploaderInstanceId = key.ApplicationType,
                            ParentWorkFolderName = key.ApplicationType,
                            Settings = this.blobUploadSettings
                        };

                        CsvUploadWorker newWorker;
                        try
                        {
                            newWorker = new CsvUploadWorker(param, this.diskSpaceManager);
                        }
                        catch (Exception)
                        {
                            this.traceSource.WriteError(
                                this.logSourceId,
                                "Failed to create upload worker object for application type {0}, Azure storage account {1} and container {2}.",
                                key.ApplicationType,
                                this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                                this.blobUploadSettings.LttTraceContainerName);
                            throw;
                        }

                        workerInfo = new UploadWorkerInfo()
                        {
                            Key = key,
                            RefCount = 1,
                            UploadWorker = newWorker
                        };
                        UploadWorkers.Add(workerInfo);
                        this.uploadWorker = workerInfo.UploadWorker;
                    }
                }
            }
            else
            {
                // Create a new upload worker object
                CsvUploadWorker.CsvUploadWorkerParameters param = new CsvUploadWorker.CsvUploadWorkerParameters()
                {
                    TraceSource = this.traceSource,
                    FabricNodeId = initParam.FabricNodeId,
                    FabricNodeInstanceName = initParam.FabricNodeInstanceName,
                    IsReadingFromApplicationManifest = this.configReader.IsReadingFromApplicationManifest,
                    LogDirectory = initParam.LogDirectory,
                    WorkDirectory = initParam.WorkDirectory,
                    UploaderInstanceId = this.logSourceId,
                    ParentWorkFolderName = Utility.ShortWindowsFabricIdForPaths,
                    Settings = this.blobUploadSettings
                };
                this.uploadWorker = new CsvUploadWorker(param, this.diskSpaceManager);
            }
        }
        
        public void FlushData()
        {
            if (this.configReader.IsReadingFromApplicationManifest)
            {
                UploadWorkerKey key = new UploadWorkerKey()
                {
                    // Destination path is an concatenation of storage account name and container name
                    DestinationPath = string.Concat(
                        this.blobUploadSettings.StorageAccountFactory.Connection.UseDevelopmentStorage
                            ? AzureConstants.DevelopmentStorageConnectionString
                            : this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                        ";", // This separator cannot occur in account name or container name
                        this.blobUploadSettings.LttTraceContainerName),
                    ApplicationType = this.configReader.GetApplicationType()
                };
                lock (UploadWorkers)
                {
                    // Drop the reference count on the upload worker object
                    UploadWorkerInfo workerInfo = UploadWorkers.FirstOrDefault(w => w.Matches(key));
                    workerInfo.UploadWorker.FlushData();
                }
            }
            else
            {
                if (null != this.uploadWorker)
                {
                    this.uploadWorker.FlushData();
                }
            }
        }

        public object GetDataSink()
        {
            return (this.blobUploadSettings.Enabled) ? ((IFolderSink)this) : null;
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }
            this.disposed = true;

            if (this.configReader.IsReadingFromApplicationManifest)
            {
                UploadWorkerKey key = new UploadWorkerKey()
                {
                    // Destination path is an concatenation of storage account name and container name
                    DestinationPath = string.Concat(
                                        this.blobUploadSettings.StorageAccountFactory.Connection.UseDevelopmentStorage ?
                                          AzureConstants.DevelopmentStorageConnectionString :
                                          this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                                          ";", // This separator cannot occur in account name or container name
                                        this.blobUploadSettings.LttTraceContainerName),
                    ApplicationType = this.configReader.GetApplicationType()
                };
                lock (UploadWorkers)
                {
                    // Drop the reference count on the upload worker object
                    UploadWorkerInfo workerInfo = UploadWorkers.FirstOrDefault(w => w.Matches(key));
                    (workerInfo.RefCount)--;
                    if (0 == workerInfo.RefCount)
                    {
                        // Tell the worker object to stop
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Stopping upload worker object for application type {0}, Azure storage account {1} and container {2} ...",
                            key.ApplicationType,
                            this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                            this.blobUploadSettings.LttTraceContainerName);

                        workerInfo.UploadWorker.Dispose();
                        UploadWorkers.Remove(workerInfo);
                    }
                    else
                    {
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Upload worker object for application type {0}, Azure storage account {1} and container {2} is still in use by other uploaders, so let it continue running",
                            key.ApplicationType,
                            this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                            this.blobUploadSettings.LttTraceContainerName);
                    }
                }
            }
            else
            {
                if (null != this.uploadWorker)
                {
                    this.uploadWorker.Dispose();
                }
            }

            GC.SuppressFinalize(this);
            return;
        }

        #region Private Fields

        private BlobUploadSettings GetSettings()
        {
            // Check for values in settings.xml
            var blobUploadSettings = new BlobUploadSettings();
            blobUploadSettings.Enabled = this.configReader.GetEnabled();
            if (!blobUploadSettings.Enabled)
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Blob storage upload not enabled");
                return blobUploadSettings;
            }

            blobUploadSettings.StorageAccountFactory = this.configReader.GetStorageAccountFactory();
            if (blobUploadSettings.StorageAccountFactory == null)
            {
                blobUploadSettings.Enabled = false;
                return blobUploadSettings;
            }

            blobUploadSettings.LttTraceContainerName = this.configReader.GetEtwTraceContainerName();
            blobUploadSettings.UploadInterval = this.configReader.GetUploadInterval();
            blobUploadSettings.FileSyncInterval = this.configReader.GetFileSyncInterval();
            blobUploadSettings.BlobDeletionAge = this.configReader.GetDataDeletionAge();
            blobUploadSettings.Filter = this.configReader.GetFilters();
            blobUploadSettings.DeploymentId = this.configReader.GetDeploymentId();

            // Write settings to log
            this.traceSource.WriteInfo(
                this.logSourceId,
                "Blob storage upload enabled: ETW trace container: {0}, Upload interval (minutes): {1}, Blob deletion age: {2}",
                blobUploadSettings.LttTraceContainerName,
                blobUploadSettings.UploadInterval,
                blobUploadSettings.BlobDeletionAge);

            return blobUploadSettings;
        }

        void IFolderSink.RegisterFolders(IEnumerable<string> folderNames)
        {
            if (folderNames.Count() != 1)
            {
                throw new ArgumentOutOfRangeException("Only a single folder path is allowed");
            }

            lock (UploadWorkers)
            {
                foreach (var uploader in UploadWorkers)
                {
                    uploader.UploadWorker.SetLogFolders(folderNames.First());
                }
            }
        }

        // Settings related to upload of data to blob storage
        internal struct BlobUploadSettings
        {
            internal bool Enabled;
            internal StorageAccountFactory StorageAccountFactory;
            internal string LttTraceContainerName;
            internal TimeSpan UploadInterval;
            internal TimeSpan FileSyncInterval;
            internal TimeSpan BlobDeletionAge;
            internal string Filter;
            internal string DeploymentId;
        }

        private struct UploadWorkerKey
        {
            internal string DestinationPath;
            internal string ApplicationType;

            internal bool Matches(UploadWorkerKey other)
            {
                return this.DestinationPath.Equals(other.DestinationPath, StringComparison.OrdinalIgnoreCase) &&
                        this.ApplicationType.Equals(other.ApplicationType, StringComparison.Ordinal);
            }
        }

        private class UploadWorkerInfo
        {
            internal UploadWorkerKey Key { get; set; }

            internal int RefCount { get; set; }

            internal CsvUploadWorker UploadWorker { get; set; }

            public bool Matches(UploadWorkerKey key)
            {
                return this.Key.Matches(key);
            }
        }
        #endregion
    }
}
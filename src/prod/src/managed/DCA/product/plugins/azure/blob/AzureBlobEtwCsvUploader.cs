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
    using System.Linq;

    // This class implements a wrapper around the worker object that uploads files
    // containing filtered ETW traces to Azure blob storage
    internal class AzureBlobEtwCsvUploader : IDcaConsumer
    {
        // List of worker objects currently available
        private static readonly List<UploadWorkerInfo> UploadWorkers = new List<UploadWorkerInfo>();
        
        // Tag used to represent the source of the log message
        private readonly string logSourceId;
        
        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Configuration reader
        private readonly AzureBlobConfigReader configReader;

        // Worker object that performs the upload of ETW events to Azure blob storage
        private readonly EtwCsvUploadWorker uploadWorker;

        // Settings for uploading data to blob storage
        private readonly BlobUploadSettings blobUploadSettings;

        // Whether or not the object has been disposed
        private bool disposed;

        public AzureBlobEtwCsvUploader(ConsumerInitializationParameters initParam)
        {
            // Initialization
            this.logSourceId = string.Concat(initParam.ApplicationInstanceId, "_", initParam.SectionName);
            this.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            this.configReader = new AzureBlobConfigReader(
                new ConfigReader(initParam.ApplicationInstanceId),
                initParam.SectionName,
                this.traceSource,
                this.logSourceId);

            // Read blob-specific settings from settings.xml
            this.blobUploadSettings = this.GetSettings();
            if (false == this.blobUploadSettings.Enabled)
            {
                // Upload to Azure blob storage is not enabled, so return immediately
                return;
            }

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
                                        this.blobUploadSettings.EtwTraceContainerName),
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
                            this.blobUploadSettings.EtwTraceContainerName);

                        workerInfo.RefCount++;
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
                            this.blobUploadSettings.EtwTraceContainerName);

                        var param = new EtwCsvUploadWorker.EtwCsvUploadWorkerParameters
                        {
                            TraceSource = this.traceSource,
                            FabricNodeId = initParam.FabricNodeId,
                            FabricNodeInstanceName = initParam.FabricNodeInstanceName,
                            IsReadingFromApplicationManifest = this.configReader.IsReadingFromApplicationManifest,
                            LogDirectory = initParam.LogDirectory,
                            WorkDirectory = initParam.WorkDirectory,
                            UploaderInstanceId = key.ApplicationType,
                            ParentWorkFolderName = key.ApplicationType,
                            Settings = this.blobUploadSettings,
                            DiskSpaceManager = initParam.DiskSpaceManager
                        };

                        EtwCsvUploadWorker newWorker;
                        try
                        {
                            newWorker = new EtwCsvUploadWorker(param);
                        }
                        catch (InvalidOperationException)
                        {
                            this.traceSource.WriteError(
                                this.logSourceId,
                                "Failed to create upload worker object for application type {0}, Azure storage account {1} and container {2}.",
                                key.ApplicationType,
                                this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                                this.blobUploadSettings.EtwTraceContainerName);
                            throw;
                        }

                        workerInfo = new UploadWorkerInfo
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
                EtwCsvUploadWorker.EtwCsvUploadWorkerParameters param = new EtwCsvUploadWorker.EtwCsvUploadWorkerParameters()
                {
                    TraceSource = this.traceSource,
                    FabricNodeId = initParam.FabricNodeId,
                    FabricNodeInstanceName = initParam.FabricNodeInstanceName,
                    IsReadingFromApplicationManifest = this.configReader.IsReadingFromApplicationManifest,
                    LogDirectory = initParam.LogDirectory,
                    WorkDirectory = initParam.WorkDirectory,
                    UploaderInstanceId = this.logSourceId,
                    ParentWorkFolderName = Utility.ShortWindowsFabricIdForPaths,
                    Settings = this.blobUploadSettings,
                    DiskSpaceManager = initParam.DiskSpaceManager
                };
                this.uploadWorker = new EtwCsvUploadWorker(param);
            }
        }

        public object GetDataSink()
        {
            // Return a sink if we are enabled, otherwise return null.
            return this.blobUploadSettings.Enabled ? ((IEtlFileSink)this.uploadWorker.GetDataSink()) : null;
        }

        public void FlushData()
        {
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
                                        this.blobUploadSettings.EtwTraceContainerName),
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
                                        this.blobUploadSettings.EtwTraceContainerName),
                    ApplicationType = this.configReader.GetApplicationType()
                };
                lock (UploadWorkers)
                {
                    // Drop the reference count on the upload worker object
                    UploadWorkerInfo workerInfo = UploadWorkers.FirstOrDefault(w => w.Matches(key));
                    workerInfo.RefCount--;
                    if (0 == workerInfo.RefCount)
                    {
                        // Tell the worker object to stop
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Stopping upload worker object for application type {0}, Azure storage account {1} and container {2} ...",
                            key.ApplicationType,
                            this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                            this.blobUploadSettings.EtwTraceContainerName);

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
                            this.blobUploadSettings.EtwTraceContainerName);
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
        }

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

            blobUploadSettings.EtwTraceContainerName = this.configReader.GetEtwTraceContainerName();
            blobUploadSettings.UploadInterval = this.configReader.GetUploadInterval();
            blobUploadSettings.FileSyncInterval = this.configReader.GetFileSyncInterval();
            blobUploadSettings.BlobDeletionAge = this.configReader.GetDataDeletionAge();
            blobUploadSettings.Filter = this.configReader.GetFilters();
            blobUploadSettings.DeploymentId = this.configReader.GetDeploymentId();

            // Write settings to log
            this.traceSource.WriteInfo(
                this.logSourceId,
                "Blob storage upload enabled: ETW trace container: {0}, Upload interval (minutes): {1}, Blob deletion age: {2}",
                blobUploadSettings.EtwTraceContainerName,
                blobUploadSettings.UploadInterval,
                blobUploadSettings.BlobDeletionAge);

            return blobUploadSettings;
        }

        // Settings related to upload of data to blob storage
        internal struct BlobUploadSettings
        {
            internal bool Enabled;
            internal StorageAccountFactory StorageAccountFactory;
            internal string EtwTraceContainerName;
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

            internal EtwCsvUploadWorker UploadWorker { get; set; }

            public bool Matches(UploadWorkerKey key)
            {
                return this.Key.Matches(key);
            }
        }
    }
}
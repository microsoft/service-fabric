// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Linq;

    // This class implements a wrapper around the worker object that uploads files
    // containing filtered ETW traces to a file share
    internal class FileShareEtwCsvUploader : IDcaConsumer
    {
        // Constants
        private const string AppInstanceEtwFilterPrefix = "*.*:";

        // List of worker objects currently available
        private static readonly List<UploadWorkerInfo> UploadWorkers = new List<UploadWorkerInfo>();
        
        // Tag used to represent the source of the log message
        private readonly string logSourceId;
        
        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Configuration reader
        private readonly ConfigReader configReader;

        // Worker object that performs the upload of ETW events to file share
        private readonly EtwCsvUploadWorker uploadWorker;

        // Settings for uploading data to file share
        private FileUploadSettings fileUploadSettings;

        // Consumer initialization parameters
        private ConsumerInitializationParameters initParam;

        // Whether or not the object has been disposed
        private bool disposed;

        public FileShareEtwCsvUploader(ConsumerInitializationParameters initParam)
        {
            // Initialization
            this.initParam = initParam;
            this.logSourceId = string.Concat(this.initParam.ApplicationInstanceId, "_", this.initParam.SectionName);
            this.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            this.configReader = new ConfigReader(initParam.ApplicationInstanceId);

            // Read file-share-specific settings from settings.xml
            this.GetSettings();
            if (false == this.fileUploadSettings.Enabled)
            {
                // Upload to file share is not enabled, so return immediately
                return;
            }

            if (this.configReader.IsReadingFromApplicationManifest)
            {
                // Check if we can use an existing upload worker object
                UploadWorkerKey key = new UploadWorkerKey()
                {
                    DestinationPath = this.fileUploadSettings.DestinationPath,
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
                            "Existing upload worker object for application type {0} and destination {1} is available and will be used.",
                            key.ApplicationType,
                            this.fileUploadSettings.DestinationPath);

                        workerInfo.RefCount++;
                        workerInfo.UploadWorker.UpdateSettings(this.fileUploadSettings);
                        this.uploadWorker = workerInfo.UploadWorker;
                    }
                    else
                    {
                        // Create a new upload worker object
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Creating upload worker object for application type {0} and destination {1} ...",
                            key.ApplicationType,
                            this.fileUploadSettings.DestinationPath);

                        EtwCsvUploadWorker.EtwCsvUploadWorkerParameters param = new EtwCsvUploadWorker.EtwCsvUploadWorkerParameters()
                        {
                            FabricNodeId = this.initParam.FabricNodeId,
                            FabricNodeInstanceName = this.initParam.FabricNodeInstanceName,
                            IsReadingFromApplicationManifest = this.configReader.IsReadingFromApplicationManifest,
                            LogDirectory = this.initParam.LogDirectory,
                            WorkDirectory = this.initParam.WorkDirectory,
                            UploaderInstanceId = key.ApplicationType,
                            ParentWorkFolderName = key.ApplicationType,
                            Settings = this.fileUploadSettings,
                            DiskSpaceManager = this.initParam.DiskSpaceManager
                        };
                        try
                        {
                            EtwCsvUploadWorker newWorker = new EtwCsvUploadWorker(param, this.traceSource);
                            workerInfo = new UploadWorkerInfo
                            {
                                Key = key,
                                RefCount = 1,
                                UploadWorker = newWorker
                            };
                            UploadWorkers.Add(workerInfo);
                            this.uploadWorker = workerInfo.UploadWorker;
                        }
                        catch (InvalidOperationException e)
                        {
                            var message = string.Format(
                                "Failed to create upload worker object for application type {0} and destination {1}.",
                                key.ApplicationType,
                                this.fileUploadSettings.DestinationPath);
                            this.traceSource.WriteError(
                                this.logSourceId,
                                message);
                            throw new InvalidOperationException(message, e);
                        }
                    }
                }
            }
            else
            {
                // Create a new upload worker object
                var param = new EtwCsvUploadWorker.EtwCsvUploadWorkerParameters
                {
                    FabricNodeId = this.initParam.FabricNodeId,
                    FabricNodeInstanceName = this.initParam.FabricNodeInstanceName,
                    IsReadingFromApplicationManifest = this.configReader.IsReadingFromApplicationManifest,
                    LogDirectory = this.initParam.LogDirectory,
                    WorkDirectory = this.initParam.WorkDirectory,
                    UploaderInstanceId = this.logSourceId,
                    ParentWorkFolderName = Utility.ShortWindowsFabricIdForPaths,
                    Settings = this.fileUploadSettings,
                    DiskSpaceManager = initParam.DiskSpaceManager
                };
                this.uploadWorker = new EtwCsvUploadWorker(param, this.traceSource);
            }
        }

        public object GetDataSink()
        {
            // Return a sink if we are enabled, otherwise return null.
            return this.fileUploadSettings.Enabled ? ((IEtlFileSink)this.uploadWorker.GetDataSink()) : null;
        }

        public void FlushData()
        {
            if (this.configReader.IsReadingFromApplicationManifest)
            {
                UploadWorkerKey key = new UploadWorkerKey()
                {
                    DestinationPath = this.fileUploadSettings.DestinationPath,
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
                    DestinationPath = this.fileUploadSettings.DestinationPath,
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
                            "Stopping upload worker object for application type {0} and destination {1} ...",
                            key.ApplicationType,
                            this.fileUploadSettings.DestinationPath);

                        workerInfo.UploadWorker.Dispose();
                        UploadWorkers.Remove(workerInfo);
                    }
                    else
                    {
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Upload worker object for application type {0} and destination {1} is still in use by other uploaders, so let it continue running",
                            key.ApplicationType,
                            this.fileUploadSettings.DestinationPath);
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

        #region Private Fields
        private void GetSettings()
        {
            // Check for values in settings.xml
            this.fileUploadSettings.Enabled = this.configReader.GetUnencryptedConfigValue(
                                                  this.initParam.SectionName,
                                                  FileShareUploaderConstants.EnabledParamName,
                                                  FileShareUploaderConstants.FileShareUploadEnabledByDefault);
            if (this.fileUploadSettings.Enabled)
            {
                if (false == FileShareCommon.GetDestinationPath(
                                this.traceSource,
                                this.logSourceId,
                                this.configReader,
                                this.initParam.SectionName,
                                out this.fileUploadSettings.DestinationIsLocalAppFolder,
                                out this.fileUploadSettings.DestinationPath))
                {
                    this.fileUploadSettings.Enabled = false;
                    return;
                }

                if (false == FileShareCommon.GetAccessInfo(
                                this.traceSource,
                                this.logSourceId,
                                this.configReader,
                                this.initParam.SectionName,
                                out this.fileUploadSettings.AccessInfo))
                {
                    this.fileUploadSettings.Enabled = false;
                    return;
                }

                var defaultUploadInterval = this.fileUploadSettings.DestinationIsLocalAppFolder ?
                    (int)FileShareUploaderConstants.DefaultLocalFolderFlushInterval.TotalMinutes :
                    (int)FileShareUploaderConstants.DefaultFileUploadInterval.TotalMinutes;
                this.fileUploadSettings.UploadInterval = TimeSpan.FromMinutes(this.configReader.GetUnencryptedConfigValue(
                                                                    this.initParam.SectionName,
                                                                    FileShareUploaderConstants.UploadIntervalParamName,
                                                                    defaultUploadInterval));
                this.fileUploadSettings.FileSyncInterval = TimeSpan.FromMinutes(this.configReader.GetUnencryptedConfigValue(
                                                                      this.initParam.SectionName,
                                                                      FileShareUploaderConstants.FileSyncIntervalParamName,
                                                                      (int)FileShareUploaderConstants.DefaultFileSyncInterval.TotalMinutes));
                var fileDeletionAge = TimeSpan.FromDays(this.configReader.GetUnencryptedConfigValue(
                                               this.initParam.SectionName,
                                               FileShareUploaderConstants.DataDeletionAgeParamName,
                                               (int)FileShareUploaderConstants.DefaultDataDeletionAgeDays.TotalDays));
                if (fileDeletionAge > FileShareUploaderConstants.MaxDataDeletionAge)
                {
                    this.traceSource.WriteWarning(
                        this.logSourceId,
                        "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                        fileDeletionAge,
                        this.initParam.SectionName,
                        FileShareUploaderConstants.DataDeletionAgeParamName,
                        FileShareUploaderConstants.MaxDataDeletionAge);
                    fileDeletionAge = FileShareUploaderConstants.MaxDataDeletionAge;
                }

                this.fileUploadSettings.FileDeletionAgeMinutes = fileDeletionAge;

                this.fileUploadSettings.Filter = this.GetFilters();

                // Check for test settings
                var logDeletionAgeTestValue = TimeSpan.FromMinutes(this.configReader.GetUnencryptedConfigValue(
                                                     this.initParam.SectionName,
                                                     FileShareUploaderConstants.TestDataDeletionAgeParamName,
                                                     0));
                if (logDeletionAgeTestValue != TimeSpan.Zero)
                {
                    if (logDeletionAgeTestValue > FileShareUploaderConstants.MaxDataDeletionAge)
                    {
                        this.traceSource.WriteWarning(
                            this.logSourceId,
                            "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                            logDeletionAgeTestValue,
                            this.initParam.SectionName,
                            FileShareUploaderConstants.TestDataDeletionAgeParamName,
                            FileShareUploaderConstants.MaxDataDeletionAge);
                        logDeletionAgeTestValue = FileShareUploaderConstants.MaxDataDeletionAge;
                    }

                    this.fileUploadSettings.FileDeletionAgeMinutes = logDeletionAgeTestValue;
                }

                // Write settings to log
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Folder upload enabled. Destination path: {0}, Account type: {1}, Domain name: {2}, User name: {3}, File deletion age ({4}): {5}",
                    this.fileUploadSettings.DestinationPath,
                    this.fileUploadSettings.AccessInfo.AccountType,
                    this.fileUploadSettings.AccessInfo.DomainName,
                    this.fileUploadSettings.AccessInfo.UserName,
                    (logDeletionAgeTestValue == TimeSpan.Zero) ? "days" : "minutes",
                    (logDeletionAgeTestValue == TimeSpan.Zero) ? fileDeletionAge : logDeletionAgeTestValue);
            }
            else
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Folder upload not enabled");
            }
        }

        private string GetFilters()
        {
            if (this.configReader.IsReadingFromApplicationManifest)
            {
                // Get filters from application manifest
                string levelName = this.configReader.GetUnencryptedConfigValue(
                                        this.initParam.SectionName,
                                        FileShareUploaderConstants.AppEtwLevelFilterParamName,
                                        FileShareUploaderConstants.DefaultAppEtwLevelFilter);
                if (false == Utility.EtwLevelNameToNumber.ContainsKey(levelName))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "The value {0} specified for section {1}, parameter {2} was not recognized. The default value {3} will be used instead.",
                        levelName,
                        this.initParam.SectionName,
                        FileShareUploaderConstants.AppEtwLevelFilterParamName,
                        FileShareUploaderConstants.DefaultAppEtwLevelFilter);
                    return string.Concat(
                                AppInstanceEtwFilterPrefix,
                                Utility.EtwLevelNameToNumber[FileShareUploaderConstants.DefaultAppEtwLevelFilter]);
                }
                else
                {
                    return string.Concat(
                                AppInstanceEtwFilterPrefix,
                                Utility.EtwLevelNameToNumber[levelName]);
                }
            }
            else
            {
                // Get filters from cluster manifest
                return this.configReader.GetUnencryptedConfigValue(
                            this.initParam.SectionName,
                            FileShareUploaderConstants.LogFilterParamName,
                            WinFabDefaultFilter.StringRepresentation);
            }
        }
        #endregion

        // Settings related to upload of data to file share
        internal struct FileUploadSettings
        {
            internal bool Enabled;
            internal string DestinationPath;
            internal bool DestinationIsLocalAppFolder;
            internal FileShareUploader.AccessInformation AccessInfo;
            internal TimeSpan UploadInterval;
            internal TimeSpan FileSyncInterval;
            internal TimeSpan FileDeletionAgeMinutes;
            internal string Filter;
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
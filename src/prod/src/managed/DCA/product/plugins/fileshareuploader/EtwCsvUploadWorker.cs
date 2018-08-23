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

    // This class implements the logic to upload files containing filtered ETW
    // traces to a file share
    internal class EtwCsvUploadWorker : IDisposable
    {
        // Constants
        private const string CsvDirName = "Log";

        // Tag used to represent the source of the log message
        private readonly string logSourceId;
        
        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // ETL to CSV converter interface reference
        private readonly EtlToCsvFileWriter etlToCsvWriter;

        // Full path to the subfolder for the current node under the destination path
        private readonly string destinationPathForNode;

        // Folder to hold CSV files containing ETW events
        private readonly string etwCsvFolder;

        // Initialization parameters
        private EtwCsvUploadWorkerParameters initParam;

        // Settings
        private FileShareEtwCsvUploader.FileUploadSettings fileUploadSettings;

        // Helper object for uploading the CSV files to a file share
        private FileShareUploader uploader;

        // Work folder used by the file share uploader
        private string workFolder;

        // Whether or not the object has been disposed
        private bool disposed;

        internal EtwCsvUploadWorker(EtwCsvUploadWorkerParameters param, FabricEvents.ExtensionsEvents traceSource)
        {
            // Initialization
            Action onError = () =>
            {
                if (null != this.etlToCsvWriter)
                {
                    this.etlToCsvWriter.Dispose();
                }
            };

            this.logSourceId = param.UploaderInstanceId;
            this.traceSource = traceSource;
            this.fileUploadSettings = param.Settings;
            this.etwCsvFolder = string.Empty;
            this.initParam = param;

            if (false == this.fileUploadSettings.DestinationIsLocalAppFolder)
            {
                // Append fabric node instance name to destination path, so that 
                // each node uploads to a different subfolder.
                this.destinationPathForNode = Path.Combine(
                                                this.fileUploadSettings.DestinationPath,
                                                param.FabricNodeInstanceName);
            }
            else
            {
                this.destinationPathForNode = this.fileUploadSettings.DestinationPath;
            }

            // Create a sub-directory for ourselves under the log directory
            this.etwCsvFolder = this.GetEtwCsvSubDirectory();
            if (!string.IsNullOrWhiteSpace(this.etwCsvFolder))
            {
                // Create the helper object that writes events delivered from the ETL
                // files into CSV files.
                this.etlToCsvWriter = new EtlToCsvFileWriter(
                    new TraceEventSourceFactory(),
                    this.logSourceId,
                    param.FabricNodeId,
                    this.etwCsvFolder,
                    false,
                    param.DiskSpaceManager,
                    new EtlToCsvFileWriterConfigReader());
            }

            // Set the event filter
            this.etlToCsvWriter.SetEtwEventFilter(
                this.fileUploadSettings.Filter,
                param.IsReadingFromApplicationManifest ? string.Empty : WinFabDefaultFilter.StringRepresentation,
                param.IsReadingFromApplicationManifest ? string.Empty : WinFabSummaryFilter.StringRepresentation,
                !param.IsReadingFromApplicationManifest);

            // Create a sub-directory for the uploader under the log directory
            if (!this.GetUploaderWorkSubDirectory())
            {
                onError();
                throw new InvalidOperationException();
            }

            try
            {
                this.uploader = new FileShareUploader(
                    this.traceSource,
                    this.logSourceId,
                    param.IsReadingFromApplicationManifest,
                    this.etwCsvFolder,
                    this.destinationPathForNode,
                    this.fileUploadSettings.AccessInfo,
                    this.workFolder,
                    this.fileUploadSettings.UploadInterval,
                    this.fileUploadSettings.FileSyncInterval,
                    this.fileUploadSettings.FileDeletionAgeMinutes,
                    this.fileUploadSettings.DestinationIsLocalAppFolder,
                    param.FabricNodeId);
                this.uploader.Start();
            }
            catch (Exception)
            {
                onError();
                throw;
            }

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Upload to file share is configured. Destination: {0}, Local trace Path: {1}.",
                this.fileUploadSettings.DestinationPath,
                this.etwCsvFolder);
        }

        public void FlushData()
        {
            // Do this in thread-safe manner.
            var uploaderLocal = this.uploader;
            if (null != uploaderLocal)
            {
                uploaderLocal.FlushData();
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
            return this.fileUploadSettings.Enabled ? ((IEtlFileSink)this.etlToCsvWriter) : null;
        }

        internal void UpdateSettings(FileShareEtwCsvUploader.FileUploadSettings newSettings)
        {
            bool updateFilter = this.ShouldUpdateFilter(newSettings);
            bool updateUploader = this.ShouldUpdateUploader(newSettings);

            if (updateFilter || updateUploader)
            {
                this.fileUploadSettings = newSettings;
            }

            if (updateFilter)
            {
                if (null != this.etlToCsvWriter)
                {
                    // Update the filter
                    Debug.Assert(this.initParam.IsReadingFromApplicationManifest, "Event filter updates are not allowed for cluster level diagnostics.");
                    this.etlToCsvWriter.SetEtwEventFilter(
                        this.fileUploadSettings.Filter,
                        string.Empty,
                        string.Empty,
                        false);
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "ETW event filter has been updated due to settings change. New ETW event filter: {0}.",
                        this.fileUploadSettings.Filter);
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
                    this.uploader = new FileShareUploader(
                        this.traceSource,
                        this.logSourceId,
                        this.initParam.IsReadingFromApplicationManifest,
                        this.etwCsvFolder,
                        this.destinationPathForNode,
                        this.fileUploadSettings.AccessInfo,
                        this.workFolder,
                        this.fileUploadSettings.UploadInterval,
                        this.fileUploadSettings.FileSyncInterval,
                        this.fileUploadSettings.FileDeletionAgeMinutes,
                        this.fileUploadSettings.DestinationIsLocalAppFolder,
                        this.initParam.FabricNodeId);
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

        private bool ShouldUpdateFilter(FileShareEtwCsvUploader.FileUploadSettings newSettings)
        {
            bool updateNeeded = false;
            if (false == newSettings.Filter.Equals(this.fileUploadSettings.Filter, StringComparison.Ordinal))
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Settings change detected. Current ETW event filter: {0}, new ETW event filter: {1}.",
                    this.fileUploadSettings.Filter,
                    newSettings.Filter);
                updateNeeded = true;
            }

            return updateNeeded;
        }

        private bool ShouldUpdateUploader(FileShareEtwCsvUploader.FileUploadSettings newSettings)
        {
            bool updateNeeded = false;
            if (newSettings.AccessInfo.AccountType != this.fileUploadSettings.AccessInfo.AccountType)
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Settings change detected. Current account type: {0}, new account type: {1}.",
                    this.fileUploadSettings.AccessInfo.AccountType,
                    newSettings.AccessInfo.AccountType);
                updateNeeded = true;
            }
            else
            {
                if (this.fileUploadSettings.AccessInfo.AccountType != FileShareUploader.FileShareAccessAccountType.None)
                {
                    if (false == newSettings.AccessInfo.UserName.Equals(
                                    this.fileUploadSettings.AccessInfo.UserName,
                                    StringComparison.OrdinalIgnoreCase))
                    {
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Settings change detected. Current user name: {0}, new user name: {1}.",
                            this.fileUploadSettings.AccessInfo.AccountType,
                            newSettings.AccessInfo.AccountType);
                        updateNeeded = true;
                    }

                    if ((null == newSettings.AccessInfo.AccountPassword) != (null == this.fileUploadSettings.AccessInfo.AccountPassword))
                    {
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Settings change detected. Account password has changed.");
                        updateNeeded = true;
                    }
                    else if (null != newSettings.AccessInfo.AccountPassword)
                    {
                        if (false == newSettings.AccessInfo.AccountPassword.Equals(
                                        this.fileUploadSettings.AccessInfo.AccountPassword,
                                        StringComparison.OrdinalIgnoreCase))
                        {
                            this.traceSource.WriteInfo(
                                this.logSourceId,
                                "Settings change detected. Account password has changed.");
                            updateNeeded = true;
                        }

                        if (newSettings.AccessInfo.AccountPasswordIsEncrypted != this.fileUploadSettings.AccessInfo.AccountPasswordIsEncrypted)
                        {
                            this.traceSource.WriteInfo(
                                this.logSourceId,
                                "Settings change detected. Account password has changed.");
                            updateNeeded = true;
                        }
                    }
                }
            }

            if (newSettings.UploadInterval != this.fileUploadSettings.UploadInterval)
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Settings change detected. Current upload interval (minutes): {0}, new upload interval (minutes): {1}.",
                    this.fileUploadSettings.UploadInterval,
                    newSettings.UploadInterval);
                updateNeeded = true;
            }

            if (newSettings.FileSyncInterval != this.fileUploadSettings.FileSyncInterval)
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Settings change detected. Current file sync interval (minutes): {0}, new file sync interval (minutes): {1}.",
                    this.fileUploadSettings.FileSyncInterval,
                    newSettings.FileSyncInterval);
                updateNeeded = true;
            }

            if (newSettings.FileDeletionAgeMinutes != this.fileUploadSettings.FileDeletionAgeMinutes)
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Settings change detected. Current file deletion age (minutes): {0}, new file deletion age (minutes): {1}.",
                    this.fileUploadSettings.FileDeletionAgeMinutes,
                    newSettings.FileDeletionAgeMinutes);
                updateNeeded = true;
            }

            return updateNeeded;
        }

        private string GetEtwCsvSubDirectory()
        {
            string etwCsvParentFolder = Path.Combine(
                                            this.initParam.WorkDirectory,
                                            this.initParam.ParentWorkFolderName);
            string csvFolder;
            bool success = Utility.CreateWorkSubDirectory(
                                this.traceSource,
                                this.logSourceId,
                                this.fileUploadSettings.DestinationPath,
                                string.Concat(Utility.EtwConsumerWorkSubFolderIdPrefix, this.logSourceId),
                                etwCsvParentFolder,
                                out csvFolder);
            if (success)
            {
                return Path.Combine(csvFolder, CsvDirName);
            }

            return null;
        }

        private bool GetUploaderWorkSubDirectory()
        {
            string workParentFolder = Path.Combine(
                                          this.initParam.WorkDirectory,
                                          this.initParam.ParentWorkFolderName);
            bool success = Utility.CreateWorkSubDirectory(
                               this.traceSource,
                               this.logSourceId,
                               this.fileUploadSettings.DestinationPath,
                               this.etwCsvFolder,
                               workParentFolder,
                               out this.workFolder);
            if (success)
            {
                this.workFolder = Path.Combine(this.workFolder, FileShareUploaderConstants.FileShareUploaderFolder);
            }

            return success;
        }

        // Parameters passed to ETW upload worker object during initialization
        internal struct EtwCsvUploadWorkerParameters
        {
            internal string FabricNodeId;
            internal string FabricNodeInstanceName;
            internal bool IsReadingFromApplicationManifest;
            internal string LogDirectory;
            internal string WorkDirectory;
            internal string UploaderInstanceId;
            internal string ParentWorkFolderName;
            internal FileShareEtwCsvUploader.FileUploadSettings Settings;
            internal DiskSpaceManager DiskSpaceManager;
        }
    }
}
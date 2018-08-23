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
    using System.IO;
    using System.Linq;

    // This class implements the logic to upload files from a given folder to 
    // a file share.
    internal class FileShareFolderUploader : IDcaConsumer, IFolderSink
    {
        // List of worker objects currently available
        private static readonly List<UploaderInfo> AllUploaders = new List<UploaderInfo>();
        
        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Configuration reader
        private readonly ConfigReader configReader;

        // Settings for uploading data to a file share
        private FileUploadSettings fileUploadSettings;

        // Consumer initialization parameters
        private ConsumerInitializationParameters initParam;

        // Helper object for uploading the CSV files to a file share
        private FileShareUploader[] uploaders;

        // Whether or not the object has been disposed
        private bool disposed;

        public FileShareFolderUploader(ConsumerInitializationParameters initParam)
        {
            // Initialization
            this.initParam = initParam;
            this.logSourceId = string.Concat(this.initParam.ApplicationInstanceId, "_", this.initParam.SectionName);
            this.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            this.configReader = new ConfigReader(initParam.ApplicationInstanceId);

            // Read file-share-specific settings from settings.xml
            this.GetSettings();
        }

        public object GetDataSink()
        {
            // Return a sink if we are enabled, otherwise return null.
            return this.fileUploadSettings.Enabled ? this : null;
        }

        public void FlushData()
        {
            var uploaders = this.uploaders;
            if (null == uploaders)
            {
                return;
            }

            foreach (FileShareUploader uploader in uploaders)
            {
                if (this.configReader.IsReadingFromApplicationManifest)
                {
                    lock (AllUploaders)
                    {
                        // Drop the reference count on the uploader object
                        UploaderInfo uploaderInfo = AllUploaders.FirstOrDefault(w => w.Uploader.Equals(uploader));
                        uploaderInfo.Uploader.FlushData();
                    }
                }
                else
                {
                    uploader.FlushData();
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

            if (null != this.uploaders)
            {
                foreach (FileShareUploader uploader in this.uploaders)
                {
                    if (this.configReader.IsReadingFromApplicationManifest)
                    {
                        // Stop the uploader for the application
                        this.StopUploaderForApplicationFolder(uploader);
                    }
                    else
                    {
                        // Stop the uploader for Windows Fabric
                        this.StopUploaderForWinFabFolder(uploader);
                    }
                }
            }

            GC.SuppressFinalize(this);
        }

        public void RegisterFolders(IEnumerable<string> folderNames)
        {
            if (null == folderNames)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Invalid parameter. folderNames should not be null");
                return;
            }

            string destination;
            List<string> uploaderWorkFolderPaths;
            bool appInstanceSpecificFolders = true;
            if (this.configReader.IsReadingFromApplicationManifest)
            {
                // We're uploading on behalf of an application. 

                // Figure out whether the source folders are specific to the application instance, i.e.
                // are they located under the application log folder?
                if (false == Utility.AreFoldersAppInstanceSpecific(
                                folderNames, 
                                this.configReader.GetApplicationLogFolder(), 
                                out appInstanceSpecificFolders))
                {
                    return;
                }

                // Compute the destination path and work folder path
                if (false == this.InitializeApplicationFolders(
                                folderNames,
                                appInstanceSpecificFolders,
                                out destination, 
                                out uploaderWorkFolderPaths))
                {
                    return;
                }
            }
            else
            {
                // We're uploading on behalf of Windows Fabric. Compute the destination path
                // and work folder path.
                if (false == this.InitializeWinFabFolders(
                                folderNames,
                                out destination, 
                                out uploaderWorkFolderPaths))
                {
                    return;
                }
            }

            // Create the uploaders
            this.uploaders = new FileShareUploader[folderNames.Count()];

            int i = 0;
            foreach (string folderName in folderNames)
            {
                bool success;
                if (this.configReader.IsReadingFromApplicationManifest)
                {
                    // Create the uploader for the application
                    success = this.CreateUploaderForApplicationFolder(
                                    appInstanceSpecificFolders ? this.logSourceId : this.configReader.GetApplicationType(),
                                    folderName,
                                    destination,
                                    uploaderWorkFolderPaths[i],
                                    out this.uploaders[i]);
                }
                else
                {
                    // Create the uploader for Windows Fabric
                    success = this.CreateUploaderForWinFabFolder(
                                    folderName, 
                                    destination,
                                    uploaderWorkFolderPaths[i],
                                    out this.uploaders[i]);
                }

                if (false == success)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Failed to configure upload to file share.  Destination: {0}, Local folder path: {1}.",
                        destination,
                        folderName);
                }

                i++;
            }
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

        private bool InitializeWinFabFolders(
                        IEnumerable<string> folderNames,
                        out string destination,
                        out List<string> uploaderWorkFolderPaths)
        {
            // Create a directory at the destination with the Fabric node ID as the name. 
            // This will make it easy to identify which node the data came from, if multiple
            // nodes are uploading to the same destination.
            destination = Path.Combine(
                            this.fileUploadSettings.DestinationPath,
                            this.initParam.FabricNodeInstanceName);

            string workParentFolder = Path.Combine(
                                          this.initParam.WorkDirectory,
                                          Utility.ShortWindowsFabricIdForPaths);
            return this.InitializeWorkFolders(
                        workParentFolder,
                        folderNames,
                        out uploaderWorkFolderPaths);
        }

        private bool CreateUploaderForWinFabFolder(
                        string source, 
                        string destination,
                        string workFolder,
                        out FileShareUploader uploader)
        {
            uploader = null;

            // Create and initialize the uploader
            try
            {
                FileShareUploader newUploader = new FileShareUploader(
                    this.traceSource,
                    this.logSourceId,
                    false, // runningOnBehalfOfApplication
                    source,
                    destination,
                    this.fileUploadSettings.AccessInfo,
                    workFolder,
                    this.fileUploadSettings.UploadInterval,
                    this.fileUploadSettings.FileSyncInterval,
                    this.fileUploadSettings.FileDeletionAgeMinutes,
                    this.fileUploadSettings.DestinationIsLocalAppFolder,
                    this.initParam.FabricNodeId);
                newUploader.Start();

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Upload to file share is configured.  Destination: {0}, Local folder path: {1}, Upload interval (minutes): {2}.",
                    destination,
                    source,
                    this.fileUploadSettings.UploadInterval);
                uploader = newUploader;
                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }

        private bool InitializeApplicationFolders(
                        IEnumerable<string> folderNames, 
                        bool appInstanceSpecificFolders,
                        out string destination,
                        out List<string> uploaderWorkFolderPaths)
        {
            // If the destination folder is not under the application's log folder on the
            // local node, then create a directory at the destination with the Fabric
            // node ID as the name. This will make it easy to identify which node the
            // data came from, if multiple nodes are uploading to the same destination.
            if (false == this.fileUploadSettings.DestinationIsLocalAppFolder)
            {
                destination = Path.Combine(
                                  this.fileUploadSettings.DestinationPath,
                                  this.initParam.FabricNodeInstanceName);
            }
            else
            {
                destination = this.fileUploadSettings.DestinationPath;
            }

            if (appInstanceSpecificFolders)
            {
                // Append the application instance ID to the destination path. This will be 
                // be useful on scale-min because the DCA node name will be the same for all uploads
                // because there is only one DCA on scale-min. Therefore, appending the application 
                // instance ID will make data from different nodes go to different subfolders.
                if (false == this.fileUploadSettings.DestinationIsLocalAppFolder)
                {
                    destination = Path.Combine(
                                      destination,
                                      this.initParam.ApplicationInstanceId);
                }
            }

            var applicationIdentifier = appInstanceSpecificFolders ?
                this.initParam.ApplicationInstanceId :
                this.configReader.GetApplicationType();
            string workParentFolder = Path.Combine(
                                        this.initParam.WorkDirectory,
                                        applicationIdentifier);
            return this.InitializeWorkFolders(
                        workParentFolder,
                        folderNames,
                        out uploaderWorkFolderPaths);
        }

        private bool InitializeWorkFolders(
                        string workParentFolder,
                        IEnumerable<string> folderNames,
                        out List<string> uploaderWorkFolderPaths)
        {
            uploaderWorkFolderPaths = new List<string>();
            bool success = true;
            foreach (string folderName in folderNames)
            {
                string level2WorkFolderPath;
                success = Utility.CreateWorkSubDirectory(
                            this.traceSource,
                            this.logSourceId,
                            this.fileUploadSettings.DestinationPath,
                            folderName,
                            workParentFolder,
                            out level2WorkFolderPath);
                if (false == success)
                {
                    break;
                }

                level2WorkFolderPath = Path.Combine(level2WorkFolderPath, FileShareUploaderConstants.FileShareUploaderFolder);
                uploaderWorkFolderPaths.Add(level2WorkFolderPath);
            }

            return success;
        }

        private bool CreateUploaderForApplicationFolder(
                        string uploaderId,
                        string source,
                        string destination,
                        string workFolder,
                        out FileShareUploader uploader)
        {
            uploader = null;
            bool success;
            FileShareUploader newUploader = null;

            // Check if we can use an existing uploader object
            UploaderKey key = new UploaderKey()
            {
                SourcePath = source,
                DestinationPath = destination,
                ApplicationType = this.configReader.GetApplicationType(),
            };
            lock (AllUploaders)
            {
                UploaderInfo uploaderInfo = AllUploaders.FirstOrDefault(w => w.Matches(key));
                if (null != uploaderInfo)
                {
                    // Existing uploader object is available. Increment its reference count
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Existing uploader object for application type {0}, source {1} and destination {2} is available and will be used.",
                        key.ApplicationType,
                        source,
                        destination);

                    uploaderInfo.RefCount++;
                    newUploader = uploaderInfo.Uploader;
                    success = true;
                }
                else
                {
                    // Create a new uploader object
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Creating uploader object for application type {0}, source {1} and destination {2} ...",
                        key.ApplicationType,
                        source,
                        destination);

                    // Create and initialize the uploader
                    try
                    {
                        newUploader = new FileShareUploader(
                            this.traceSource,
                            uploaderId,
                            true, // runningOnBehalfOfApplication
                            source,
                            destination,
                            this.fileUploadSettings.AccessInfo,
                            workFolder,
                            this.fileUploadSettings.UploadInterval,
                            this.fileUploadSettings.FileSyncInterval,
                            this.fileUploadSettings.FileDeletionAgeMinutes,
                            this.fileUploadSettings.DestinationIsLocalAppFolder,
                            this.initParam.FabricNodeId);
                        newUploader.Start();
                        success = true;
                    }
                    catch (Exception)
                    {
                        success = false;
                    }

                    if (success)
                    {
                        uploaderInfo = new UploaderInfo()
                        {
                            Key = key,
                            RefCount = 1,
                            Uploader = newUploader
                        };
                        AllUploaders.Add(uploaderInfo);
                    }
                    else
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Failed to create uploader object for application type {0}, source {1} and destination {2}.",
                            key.ApplicationType,
                            source,
                            destination);
                    }
                }
            }

            if (success)
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Upload to file share is configured.  Destination: {0}, Local folder path: {1}, Upload interval (minutes): {2}.",
                    destination,
                    source,
                    this.fileUploadSettings.UploadInterval);
                uploader = newUploader;
            }

            return success;
        }

        private void StopUploaderForWinFabFolder(FileShareUploader uploader)
        {
            if (null == uploader)
            {
                return;
            }

            uploader.Dispose();
        }

        private void StopUploaderForApplicationFolder(FileShareUploader uploader)
        {
            if (null == uploader)
            {
                return;
            }

            lock (AllUploaders)
            {
                // Drop the reference count on the uploader object
                UploaderInfo uploaderInfo = AllUploaders.FirstOrDefault(w => w.Uploader.Equals(uploader));
                uploaderInfo.RefCount--;
                if (0 == uploaderInfo.RefCount)
                {
                    // Tell the uploader object to stop
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Stopping uploader object for application type {0}, source {1} and destination {2} ...",
                        uploaderInfo.Key.ApplicationType,
                        uploaderInfo.Key.SourcePath,
                        uploaderInfo.Key.DestinationPath);

                    uploaderInfo.Uploader.Dispose();
                    AllUploaders.Remove(uploaderInfo);
                }
                else
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Uploader object for application type {0}, source {1} and destination {2} is still in use by other uploaders, so let it continue running",
                        uploaderInfo.Key.ApplicationType,
                        uploaderInfo.Key.SourcePath,
                        this.fileUploadSettings.DestinationPath);
                }
            }
        }

        #endregion
        // Settings related to upload of data to a file share
        private struct FileUploadSettings
        {
            internal bool Enabled;
            internal string DestinationPath;
            internal FileShareUploader.AccessInformation AccessInfo;
            internal bool DestinationIsLocalAppFolder;
            internal TimeSpan UploadInterval;
            internal TimeSpan FileSyncInterval;
            internal TimeSpan FileDeletionAgeMinutes;
        }

        private struct UploaderKey
        {
            internal string SourcePath;
            internal string DestinationPath;
            internal string ApplicationType;

            internal bool Matches(UploaderKey other)
            {
                return this.SourcePath.Equals(other.SourcePath, StringComparison.OrdinalIgnoreCase) &&
                        this.DestinationPath.Equals(other.DestinationPath, StringComparison.OrdinalIgnoreCase) &&
                        this.ApplicationType.Equals(other.ApplicationType, StringComparison.Ordinal);
            }
        }

        private class UploaderInfo
        {
            internal UploaderKey Key { get; set; }

            internal int RefCount { get; set; }

            internal FileShareUploader Uploader { get; set; }

            public bool Matches(UploaderKey key)
            {
                return this.Key.Matches(key);
            }
        }
    }
}
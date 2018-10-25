// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.IO;
    using System.Linq;

    // This class implements the logic to upload files from a given folder to 
    // Azure blob storage.
    internal class AzureBlobFolderUploader : IDcaConsumer, IFolderSink
    {
        // List of worker objects currently available
        private static readonly List<UploaderInfo> AllUploaders = new List<UploaderInfo>();
        
        // Settings for uploading data to blob storage
        private readonly BlobUploadSettings blobUploadSettings;
        
        // Tag used to represent the source of the log message
        private readonly string logSourceId;
        
        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // String that represents the destination account and container
        private readonly string destinationKey;

        // Consumer initialization parameters
        private readonly ConsumerInitializationParameters initParam;

        // Configuration reader
        private readonly AzureBlobConfigReader configReader;

        // Helper object for uploading the CSV files to Azure blob storage
        private AzureFileUploader[] uploaders;

        // Whether or not the object has been disposed
        private bool disposed;

        public AzureBlobFolderUploader(ConsumerInitializationParameters initParam)
        {
            // Initialization
            this.initParam = initParam;
            this.logSourceId = string.Concat(this.initParam.ApplicationInstanceId, "_", this.initParam.SectionName);
            this.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            this.configReader = new AzureBlobConfigReader(
                new ConfigReader(initParam.ApplicationInstanceId),
                this.initParam.SectionName,
                this.traceSource,
                this.logSourceId);

            // Read blob-specific settings from settings.xml
            this.blobUploadSettings = this.GetSettings();
            if (false == this.blobUploadSettings.Enabled)
            {
                // Upload to Azure blob storage is not enabled, so return immediately
                return;
            }

            var accountName = this.blobUploadSettings.StorageAccountFactory.Connection.UseDevelopmentStorage
                ? AzureConstants.DevelopmentStorageConnectionString
                : this.blobUploadSettings.StorageAccountFactory.Connection.AccountName;
            this.destinationKey = string.Join(
                                        "_",
                                        StandardPluginTypes.AzureBlobFolderUploader,
                                        accountName,
                                        this.blobUploadSettings.ContainerName);
        }

        public object GetDataSink()
        {
            // Return a sink if we are enabled, otherwise return null.
            return this.blobUploadSettings.Enabled ? this : null;
        }

        public void FlushData()
        {
            var uploaders = this.uploaders;
            if (null == uploaders)
            {
                return;
            }

            foreach (AzureFileUploader uploader in uploaders.Where(u => u != null))
            {
                if (this.configReader.IsReadingFromApplicationManifest)
                {
                    lock (AllUploaders)
                    {
                        UploaderInfo uploaderInfo = AllUploaders.FirstOrDefault(w => w.Uploader.Equals(uploader));
                        if (uploaderInfo != null)
                        {
                            uploaderInfo.Uploader.FlushData();
                        }
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
                foreach (AzureFileUploader uploader in this.uploaders)
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

                // Compute the work folder path
                if (false == this.InitializeApplicationFolders(
                                folderNames,
                                appInstanceSpecificFolders,
                                out uploaderWorkFolderPaths))
                {
                    return;
                }
            }
            else
            {
                // We're uploading on behalf of Windows Fabric. Compute the work folder path.
                if (false == this.InitializeWinFabFolders(
                                folderNames,
                                out uploaderWorkFolderPaths))
                {
                    return;
                }
            }

            // Create the uploaders
            this.uploaders = new AzureFileUploader[folderNames.Count()];

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
                                    uploaderWorkFolderPaths[i],
                                    out this.uploaders[i]);
                }
                else
                {
                    // Create the uploader for Windows Fabric
                    success = this.CreateUploaderForWinFabFolder(
                                    folderName,
                                    uploaderWorkFolderPaths[i],
                                    out this.uploaders[i]);
                }

                if (false == success)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Failed to configure upload to Azure blob.  Storage account: {0}, container: {1}, Local folder path: {2}.",
                        this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                        this.blobUploadSettings.ContainerName,
                        folderName);
                }

                i++;
            }
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

            blobUploadSettings.ContainerName = this.configReader.GetContainerName();
            blobUploadSettings.UploadInterval = this.configReader.GetUploadInterval();
            blobUploadSettings.FileSyncInterval = this.configReader.GetFileSyncInterval();
            blobUploadSettings.BlobDeletionAge = this.configReader.GetDataDeletionAge();
            blobUploadSettings.DeploymentId = this.configReader.GetDeploymentId();

            // Write settings to log
            this.traceSource.WriteInfo(
                this.logSourceId,
                "Blob storage upload enabled: Container: {0}, Upload interval (minutes): {1}, Blob deletion age: {2}",
                blobUploadSettings.ContainerName,
                blobUploadSettings.UploadInterval,
                blobUploadSettings.BlobDeletionAge);

            return blobUploadSettings;
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
                            this.destinationKey,
                            folderName,
                            workParentFolder,
                            out level2WorkFolderPath);
                if (false == success)
                {
                    break;
                }

                level2WorkFolderPath = Path.Combine(level2WorkFolderPath, AzureConstants.AzureFileUploaderFolder);
                uploaderWorkFolderPaths.Add(level2WorkFolderPath);
            }

            return success;
        }

        private bool InitializeWinFabFolders(
                        IEnumerable<string> folderNames,
                        out List<string> uploaderWorkFolderPaths)
        {
            string workParentFolder = Path.Combine(
                                          this.initParam.WorkDirectory,
                                          Utility.ShortWindowsFabricIdForPaths);
            return this.InitializeWorkFolders(
                        workParentFolder,
                        folderNames,
                        out uploaderWorkFolderPaths);
        }

        private bool InitializeApplicationFolders(
                        IEnumerable<string> folderNames,
                        bool appInstanceSpecificFolders,
                        out List<string> uploaderWorkFolderPaths)
        {
            var appInstanceId = appInstanceSpecificFolders
                ? this.initParam.ApplicationInstanceId
                : this.configReader.GetApplicationType();
            string workParentFolder = Path.Combine(
                                        this.initParam.WorkDirectory,
                                        appInstanceId);
            return this.InitializeWorkFolders(
                        workParentFolder,
                        folderNames,
                        out uploaderWorkFolderPaths);
        }

        private bool CreateUploaderForWinFabFolder(
                        string source,
                        string workFolder,
                        out AzureFileUploader uploader)
        {
            uploader = null;

            // Create and initialize the uploader
            //
            // NOTE: We do not make any assumptions about the contents of general
            // data folders. Hence, as part of old data deletion, we attempt to 
            // delete all blobs in the corresponding container, without trying to
            // filter blobs by Fabric node ID. We do this by specifying 'false'
            // for the 'filterDeletionByNodeId' parameter.
            try
            {
                var destinationPath = string.Concat(
                    this.blobUploadSettings.StorageAccountFactory.Connection.UseDevelopmentStorage
                        ? AzureConstants.DevelopmentStorageConnectionString
                        : this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                    ";", // This separator cannot occur in account name or container name
                    this.blobUploadSettings.ContainerName);
                AzureFileUploader newUploader = new AzureFileUploader(
                    this.traceSource,
                    this.logSourceId,
                    source,
                    destinationPath,
                    workFolder,
                    this.blobUploadSettings.StorageAccountFactory,
                    this.blobUploadSettings.ContainerName,
                    this.blobUploadSettings.UploadInterval,
                    this.blobUploadSettings.FileSyncInterval,
                    this.blobUploadSettings.BlobDeletionAge,
                    this.initParam.FabricNodeInstanceName,
                    this.blobUploadSettings.DeploymentId);
                newUploader.Start();

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Upload to blob storage is configured. Storage account: {0}, Container: {1}, Local data Path: {2}, Upload interval (minutes): {3}",
                    this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                    this.blobUploadSettings.ContainerName,
                    source,
                    this.blobUploadSettings.UploadInterval);
                uploader = newUploader;
                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }

        private bool CreateUploaderForApplicationFolder(
                        string uploaderId,
                        string source,
                        string workFolder,
                        out AzureFileUploader uploader)
        {
            uploader = null;
            bool success = true;
            AzureFileUploader newUploader;

            // Check if we can use an existing uploader object
            UploaderKey key = new UploaderKey()
            {
                SourcePath = source,

                // Destination path is an concatenation of storage account name and container name
                DestinationPath = string.Concat(
                                    this.blobUploadSettings.StorageAccountFactory.Connection.UseDevelopmentStorage ?
                                      AzureConstants.DevelopmentStorageConnectionString :
                                      this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                                      ";", // This separator cannot occur in account name or container name
                                    this.blobUploadSettings.ContainerName),
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
                        "Existing uploader object for application type {0}, source {1}, storage account {2}, container {3} is available and will be used.",
                        key.ApplicationType,
                        source,
                        this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                        this.blobUploadSettings.ContainerName);

                    uploaderInfo.RefCount++;
                    newUploader = uploaderInfo.Uploader;
                    success = true;
                }
                else
                {
                    // Create a new uploader object
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Creating uploader object for application type {0}, source {1}, storage account {2}, container {3} ...",
                        key.ApplicationType,
                        source,
                        this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                        this.blobUploadSettings.ContainerName);

                    // Create and initialize the uploader
                    //
                    // NOTE: We do not make any assumptions about the contents of general
                    // data folders. Hence, as part of old data deletion, we attempt to 
                    // delete all files in the corresponding container, without trying to
                    // filter files by Fabric node ID. We do this by specifying 'false'
                    // for the 'filterDeletionByNodeId' parameter.
                    try
                    {
                        var destinationPath = string.Concat(
                            this.blobUploadSettings.StorageAccountFactory.Connection.UseDevelopmentStorage
                                ? AzureConstants.DevelopmentStorageConnectionString
                                : this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                            ";", // This separator cannot occur in account name or container name
                            this.blobUploadSettings.ContainerName);
                        newUploader = new AzureFileUploader(
                            this.traceSource,
                            this.logSourceId,
                            source,
                            destinationPath,
                            workFolder,
                            this.blobUploadSettings.StorageAccountFactory,
                            this.blobUploadSettings.ContainerName,
                            this.blobUploadSettings.UploadInterval,
                            this.blobUploadSettings.FileSyncInterval,
                            this.blobUploadSettings.BlobDeletionAge,
                            this.initParam.FabricNodeInstanceName,
                            this.blobUploadSettings.DeploymentId);
                        newUploader.Start();
                        uploaderInfo = new UploaderInfo()
                        {
                            Key = key,
                            RefCount = 1,
                            Uploader = newUploader
                        };
                        AllUploaders.Add(uploaderInfo);

                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Upload to file share is configured. Storage account: {0}, Container: {1}, Local folder path: {2}, Upload interval (minutes): {3}.",
                            this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                            this.blobUploadSettings.ContainerName,
                            source,
                            this.blobUploadSettings.UploadInterval);
                        uploader = newUploader;
                    }
                    catch (Exception)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Failed to create uploader object for application type {0}, source {1}, storage account {2}, container {3}.",
                            key.ApplicationType,
                            source,
                            this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                            this.blobUploadSettings.ContainerName);
                    }
                }
            }

            return success;
        }

        private void StopUploaderForWinFabFolder(AzureFileUploader uploader)
        {
            if (null == uploader)
            {
                return;
            }

            uploader.Dispose();
        }

        private void StopUploaderForApplicationFolder(AzureFileUploader uploader)
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
                        "Uploader object for application type {0}, source {1}, storage account {2}, container {3} is still in use by other uploaders, so let it continue running",
                        uploaderInfo.Key.ApplicationType,
                        uploaderInfo.Key.SourcePath,
                        this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                        this.blobUploadSettings.ContainerName);
                }
            }
        }

        // Settings related to upload of data to blob storage
        private struct BlobUploadSettings
        {
            internal bool Enabled;
            internal StorageAccountFactory StorageAccountFactory;
            internal string ContainerName;
            internal TimeSpan UploadInterval;
            internal TimeSpan FileSyncInterval;
            internal TimeSpan BlobDeletionAge;
            internal string DeploymentId;
        }

        private struct UploaderKey
        {
            internal string SourcePath;
            internal string DestinationPath;
            internal string ApplicationType;

            internal bool Matches(UploaderKey other)
            {
                return this.SourcePath.Equals(other.SourcePath, StringComparison.OrdinalIgnoreCase) &&
                        this.DestinationPath.Equals(other.DestinationPath, StringComparison.Ordinal) &&
                        this.ApplicationType.Equals(other.ApplicationType, StringComparison.Ordinal);
            }
        }

        private class UploaderInfo
        {
            internal UploaderKey Key { get; set; }

            internal int RefCount { get; set; }

            internal AzureFileUploader Uploader { get; set; }

            public bool Matches(UploaderKey key)
            {
                return this.Key.Matches(key);
            }
        }
    }
}
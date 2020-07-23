// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.IO;

    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Blob;
    using Microsoft.WindowsAzure.Storage.RetryPolicies;

    /// <summary>
    /// Azure blob BackupStoreManager Base class
    /// </summary>
    internal abstract class AzureBlobBackupStoreManagerBase : IBackupStoreManager
    {
        private readonly string TraceType;

        private readonly int retryCount = 3;

        private readonly FabricEvents.ExtensionsEvents TraceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.BackupRestoreService);

        public AzureBlobBackupStoreManagerBase(string traceType)
        {
            this.TraceType = traceType;
        }

        #region IBackupStoreManager Methods

        public void Upload(string sourceFileOrFolderPath, string destinationFolderName, bool sourceIsFile = false)
        {
            var containerName = this.GetContainerName();
            var destinationFolder = Path.Combine(this.GetFolderPath(), destinationFolderName == null ? string.Empty : destinationFolderName);

            this.TraceSource.WriteInfo(TraceType, "Moving backup from {0} to {1}/{2}", sourceFileOrFolderPath, containerName, destinationFolder);

            CloudBlobContainer container = null;

            // Create container
            try
            {
                container = BackupRestoreUtility.PerformWithRetries<string, CloudBlobContainer>(
                    this.GetOrCreateContainer,
                    containerName,
                    new RetriableOperationExceptionHandler(this.AzureStorageExceptionHandler),
                    this.retryCount);
            }
            catch (Exception e)
            {
                this.TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "Failed to create container {0} on Azure storage.",
                    containerName);

                throw;
            }

            string sourceFolder;
            if (!sourceIsFile)
            {
                sourceFolder = sourceFileOrFolderPath;
            }
            else
            {
                sourceFolder = Path.GetDirectoryName(sourceFileOrFolderPath);
            }

            if (sourceIsFile)
            {
                CopyFileToDestination(container, sourceFileOrFolderPath, destinationFolder, this.retryCount);
            }
            else
            {
                var sourceDirInfo = new DirectoryInfo(sourceFolder);

                // TODO: Add following files in concurrent bag and run loop with scheduled number of threads to upload these in parallel.
                foreach (var file in sourceDirInfo.GetFiles("*", SearchOption.AllDirectories))
                {
                    string sourceRelative = file.DirectoryName.Substring(sourceFolder.Trim('\\').Length).Trim('\\');
                    string effectiveDestinationPath = Path.Combine(destinationFolder, sourceRelative);

                    this.TraceSource.WriteInfo(TraceType, "Full file name: {0} \t destinationFolder: {1} ", file.FullName, effectiveDestinationPath);
                    CopyFileToDestination(container, file.FullName, effectiveDestinationPath, this.retryCount);
                }
            }
        }

        public void Download(string sourceFileOrFolderName, string destinationFolder, bool sourceIsFile = false)
        {
            var containerName = this.GetContainerName();
            TraceSource.WriteInfo(TraceType, "Moving backup from Azure storage container {0} with relative path {1} to {2}", containerName, sourceFileOrFolderName, destinationFolder);

            CloudBlobContainer container = null;

            // Create container
            container = this.GetContainer(containerName);

            string sourceFolderPath = sourceFileOrFolderName;

            // Create a list to hold the hierarchy of the source
            List<string> sourceRelativeParts = new List<string>();

            if (sourceIsFile)
            {
                sourceFolderPath = Path.GetDirectoryName(sourceFileOrFolderName);
                sourceRelativeParts.Add(Path.GetFileName(sourceFileOrFolderName));
            }

            string effectiveSourceName = string.Empty;

            // Calculate effective folder
            string effectiveSourceFolder = Path.Combine(this.GetFolderPath(), sourceFolderPath);

            if (!string.IsNullOrWhiteSpace(effectiveSourceFolder))
            {
                // Add each directory in the directory hierarchy to the list
                string sourceRelativePartialPath = effectiveSourceFolder;
                while (false == string.IsNullOrEmpty(sourceRelativePartialPath))
                {
                    string sourceRelativePart = Path.GetFileName(sourceRelativePartialPath);
                    sourceRelativeParts.Add(sourceRelativePart);

                    sourceRelativePartialPath = Path.GetDirectoryName(sourceRelativePartialPath);
                }

                // Reverse the list, so that top-level directories appear first
                sourceRelativeParts.Reverse();

                // Compute the directory name under the container
                effectiveSourceName = string.Join(
                                                    "/",
                                                    sourceRelativeParts.ToArray());
            }

            if (sourceIsFile)
            {
                CloudBlockBlob blob = this.GetBlockBlobReference(container, effectiveSourceName);
                string fullDestinationPath = Path.Combine(destinationFolder, Path.GetFileName(sourceFileOrFolderName));
                TraceSource.WriteInfo(TraceType, "BlobName: {0} /t FullDestinationPath: {1}", blob.Name, fullDestinationPath);

                this.CopyFileToDestination(blob, fullDestinationPath, this.retryCount);
            }
            else
            {
                // Get AzureStorageDirectory
                CloudBlobDirectory sourceRootDirectory = this.GetCloudBlobDirectoryRef(container, effectiveSourceName);

                // Get list of all blobs under this directory.
                IEnumerable<IListBlobItem> blobList = this.GetBlobList(sourceRootDirectory, true);

                foreach (var blobItem in blobList)
                {
                    CloudBlockBlob blob = blobItem as CloudBlockBlob;

                    if (blob != null)
                    {
                        string relativePath = Uri.UnescapeDataString(blobItem.Uri.AbsolutePath.Substring(sourceRootDirectory.Uri.AbsolutePath.Length));
                        string fullDestinationPath = Path.Combine(destinationFolder, relativePath);

                        TraceSource.WriteInfo(TraceType, "BlobName: {0} /t RelativePath:  {1} /t FullDestinationPath: {2}", blob.Name, relativePath, fullDestinationPath);

                        this.CopyFileToDestination(blob, fullDestinationPath, this.retryCount);
                    }
                    else
                    {
                        TraceSource.WriteWarning(TraceType, "Skipping non BlockBlob, Uri: {0}", blobItem.Uri);
                    }
                }
            }
        }
        #endregion

        #region Base class abstract methods
        protected abstract string GetContainerName();

        protected abstract string GetFolderPath();

        protected abstract CloudStorageAccount GetCloudStorageAccount();
        #endregion

        #region Private Methods


        #region File Copy Helper Methods
        #region File Copy Helper Methods with Retry Logic        
        private bool CopyFileToDestination(CloudBlockBlob blob, string destinationFullFileName, int retryCount)
        {
            CloudFileCopyInfo cloudFileCopyInfo = new CloudFileCopyInfo()
            {
                SourceBlob = blob,
                FullFileName = destinationFullFileName,
                RetryCount = retryCount
            };

            try
            {
                BackupRestoreUtility.PerformWithRetries(
                    this.CopyBlobToDestinationFile,
                    cloudFileCopyInfo,
                    new RetriableOperationExceptionHandler(this.AzureStorageExceptionHandler),
                    this.retryCount);
            }
            catch (Exception e)
            {
                this.TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "Failed to copy blob: {0} to file: {1}.",
                    blob.Name,
                    destinationFullFileName);

                throw;
            }

            return true;
        }

        private bool CopyFileToDestination(CloudBlobContainer container, string source, string destinationPath, int retryCount)
        {
            // Get the name of the source file
            string sourceFileName = Path.GetFileName(source);

            // Create a list to hold the directory hierarchy of the source
            List<string> sourceRelativeParts = new List<string>();

            string relativeDirectoryName = string.Empty;

            if (!string.IsNullOrWhiteSpace(destinationPath))
            { 
                // Add each directory in the directory hierarchy to the list
                string sourceRelativePartialPath = destinationPath;
                while (false == string.IsNullOrEmpty(sourceRelativePartialPath))
                {
                    string sourceRelativePart = Path.GetFileName(sourceRelativePartialPath);
                    sourceRelativeParts.Add(sourceRelativePart);

                    sourceRelativePartialPath = Path.GetDirectoryName(sourceRelativePartialPath);
                }

                // Reverse the list, so that top-level directories appear first
                sourceRelativeParts.Reverse();

                // Compute the directory name under the container
                relativeDirectoryName = string.Join(
                                                   "/",
                                                   sourceRelativeParts.ToArray());
            }

            // Copy the file to the Azure blob
            FileCopyInfo fileCopyInfo = new FileCopyInfo()
            {
                SourceFullPath = source,
                SourceFileName = sourceFileName,
                RelativeDirectoryName = relativeDirectoryName,
                RetryCount = retryCount,
                Container = container
            };
            try
            {
                BackupRestoreUtility.PerformWithRetries(
                    this.CopyFileToDestinationBlob,
                    fileCopyInfo,
                    new RetriableOperationExceptionHandler(this.AzureStorageExceptionHandler),
                    retryCount);
            }
            catch (Exception e)
            {
                this.TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "Failed to copy file {0} to Azure blob storage container {1}.",
                    source,
                    container.Name);

                throw;
            }

            return true;
        }

        private void CopyFileToDestinationBlobWorker(FileCopyInfo fileCopyInfo, CloudBlockBlob destinationBlob)
        {
            // First let's try to copy directly from the source location. This should work fine if the
            // source file is not in use.
            bool exceptionWithDirectCopy = false;
            try
            {
                CreateStreamAndUploadToBlob(fileCopyInfo.SourceFullPath, destinationBlob);
                this.TraceSource.WriteInfo(
                    TraceType,
                    "Uploaded file {0} to destination blob {1}",
                    fileCopyInfo.SourceFullPath,
                    destinationBlob.Name);
            }
            catch (Exception e)
            {
                if ((false == (e is IOException)) && (false == (e is FabricException)))
                {
                    throw;
                }

                exceptionWithDirectCopy = true;
            }

            if (exceptionWithDirectCopy)
            {
                // We couldn't copy the file directly to blob. This can happen when the file is in use,
                // because we would be unable to open a handle to the file in that case. Therefore,
                // we make a copy of the file to a temp location and then open a handle to that temp 
                // copy for uploading to blob.
                string tempDir = Path.GetTempPath();
                if (!String.IsNullOrWhiteSpace(fileCopyInfo.RelativeDirectoryName))
                {
                    tempDir = Path.Combine(tempDir, fileCopyInfo.RelativeDirectoryName);
                }

                string tempDest = Path.Combine(tempDir, fileCopyInfo.SourceFileName);
                FabricDirectory.CreateDirectory(tempDir);
                try
                {
                    FabricFile.Copy(fileCopyInfo.SourceFullPath, tempDest, true);
                    CreateStreamAndUploadToBlob(tempDest, destinationBlob);
                    this.TraceSource.WriteInfo(
                    TraceType,
                    "Uploaded file {0} to destination blob {1}",
                    fileCopyInfo.SourceFullPath,
                    destinationBlob.Name);
                }
                finally
                {
                    try
                    {
                        BackupRestoreUtility.PerformIOWithRetries(
                            () =>
                            {
                                FabricDirectory.Delete(tempDir, true);
                            });
                    }
                    catch (Exception e)
                    {
                        this.TraceSource.WriteExceptionAsError(
                            TraceType,
                            e,
                            "Failed to delete temporary directory {0}.",
                            tempDir);
                    }
                }
            }
        }
        #endregion

        private void CopyBlobToDestinationFile(object context)
        {
            CloudFileCopyInfo fileCopyInfo = (CloudFileCopyInfo)context;
            string directory = Path.GetDirectoryName(fileCopyInfo.FullFileName);

            if (!Directory.Exists(directory))
            {
                Directory.CreateDirectory(directory);
            }

            fileCopyInfo.SourceBlob.DownloadToFile(fileCopyInfo.FullFileName, FileMode.Create);
        }

        private void CopyFileToDestinationBlob(object context)
        {
            FileCopyInfo fileCopyInfo = (FileCopyInfo)context;

            // Get the directory object under the container
            CloudBlobDirectory destinationDirectory = fileCopyInfo.Container.GetDirectoryReference(fileCopyInfo.RelativeDirectoryName);

            // Get the blob object
            CloudBlockBlob destinationBlob = destinationDirectory.GetBlockBlobReference(fileCopyInfo.SourceFileName);

            // Copy the file to the blob
            this.CopyFileToDestinationBlobWorker(fileCopyInfo, destinationBlob);
        }

        private static void CreateStreamAndUploadToBlob(string sourceFile, CloudBlockBlob destinationBlob)
        {
            using (FileStream fs = new FileStream(sourceFile, FileMode.Open, FileAccess.Read))
            {
                destinationBlob.UploadFromStream(fs, null, null, null);
            }
        }
        #endregion

        #region Azure Storage Helper Methods with Retry Logic
        private CloudBlobContainer GetContainerRef(string containerName)
        {
            CloudStorageAccount storageAccount = this.GetCloudStorageAccount();

            // Create the blob client object
            CloudBlobClient blobClient = storageAccount.CreateCloudBlobClient();

            // Set the retry policy
            blobClient.DefaultRequestOptions.RetryPolicy = new ExponentialRetry();

            // Get the container reference
            return blobClient.GetContainerReference(containerName);
        }

        private CloudBlobContainer GetOrCreateContainer(string containerName)
        {
            try
            {
                return BackupRestoreUtility.PerformWithRetries<string, CloudBlobContainer>(
                    (string name) =>
                    {
                        CloudBlobContainer blobContainer = this.GetContainerRef(containerName);

                        // Create the container
                        blobContainer.CreateIfNotExists(null, null);

                        return blobContainer;
                    },
                    containerName,
                    new RetriableOperationExceptionHandler(this.AzureStorageExceptionHandler),
                    this.retryCount);
            }
            catch (Exception e)
            {
                this.TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "Failed to GetOrCreate container {0} with Azure storage.",
                    containerName);

                throw;
            }
        }

        private CloudBlobContainer GetContainer(string containerName)
        {
            try
            {
                return BackupRestoreUtility.PerformWithRetries<string, CloudBlobContainer>(
                    (string name) =>
                    {
                        CloudBlobContainer container = this.GetContainerRef(name);

                        // Create the container
                        if (!container.Exists())
                        {
                            throw new ArgumentException(string.Format("Given container {0} doesn't exist.", name), "containerName");
                        }

                        return container;
                    },
                    containerName,
                    new RetriableOperationExceptionHandler(this.AzureStorageExceptionHandler),
                    this.retryCount);
            }
            catch (Exception e)
            {
                this.TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "Failed to get container {0} from Azure storage.",
                    containerName);

                throw;
            }
        }

        private CloudBlockBlob GetBlockBlobReference(CloudBlobContainer container, string blobName)
        {
            try
            {
                return BackupRestoreUtility.PerformWithRetries<string, CloudBlockBlob>(
                    (string name) =>
                    {
                        return container.GetBlockBlobReference(name);
                    },
                    blobName,
                    new RetriableOperationExceptionHandler(this.AzureStorageExceptionHandler),
                    this.retryCount);
            }
            catch (Exception e)
            {
                this.TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "Failed to get cloud block blob {0} from Azure storage blob container {1}.",
                    blobName,
                    container.Name);

                throw;
            }
        }

        private CloudBlobDirectory GetCloudBlobDirectoryRef(CloudBlobContainer container, string directoryName)
        {
            try
            {
                return BackupRestoreUtility.PerformWithRetries<string, CloudBlobDirectory>(
                    (string relDirName) =>
                    {
                        return container.GetDirectoryReference(relDirName);
                    },
                    directoryName,
                    new RetriableOperationExceptionHandler(this.AzureStorageExceptionHandler),
                    this.retryCount);
            }
            catch (Exception e)
            {
                this.TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "Failed to get directory reference {0} from Azure storage.",
                    directoryName);

                throw;
            }
        }

        private IEnumerable<IListBlobItem> GetBlobList(CloudBlobDirectory directory, bool useFlatBlobListing)
        {
            try
            {
                return BackupRestoreUtility.PerformWithRetries<bool, IEnumerable<IListBlobItem>>(
                    (bool flatBlobListing) =>
                    {
                        return directory.ListBlobs(flatBlobListing);
                    },
                    useFlatBlobListing,
                    new RetriableOperationExceptionHandler(this.AzureStorageExceptionHandler),
                    this.retryCount);
            }
            catch (Exception e)
            {
                this.TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "Failed to list blobs under directory {0} from Azure storage.",
                    directory.Prefix);

                throw;
            }

        }

        private RetriableOperationExceptionHandler.Response AzureStorageExceptionHandler(Exception e)
        {
            if (e is StorageException)
            {
                // If this was a network error, we'll retry later. Else, we'll
                // just give up.
                StorageException storageException = (StorageException)e;
                if (this.IsNetworkError(storageException.RequestInformation.HttpStatusCode))
                {
                    return RetriableOperationExceptionHandler.Response.Retry;
                }

                this.TraceSource.WriteExceptionAsWarning(
                    TraceType,
                    e,
                    "Encountered an unhandled storage exception with http code {0}",
                    storageException.RequestInformation.HttpStatusCode);
            }
            else if ((e is IOException) || (e is FabricException))
            {
                // If there was an error accessing the file, but it was not because
                // the file was not found, we'll retry later. Else, we'll just give up.
                if (false == ((e is FileNotFoundException) || (e is DirectoryNotFoundException)))
                {
                    return RetriableOperationExceptionHandler.Response.Retry;
                }
            }

            return RetriableOperationExceptionHandler.Response.Abort;
        }

        private bool IsNetworkError(int httpStatusCode)
        {
            const int HttpCodeInternalServerError = 500;
            const int HttpCodeBadGateway = 502;
            const int HttpCodeServiceUnavailable = 503;
            const int HttpCodeGatewayTimeout = 504;

            return (httpStatusCode == HttpCodeInternalServerError) ||
                    (httpStatusCode == HttpCodeBadGateway) ||
                    (httpStatusCode == HttpCodeServiceUnavailable) ||
                    (httpStatusCode == HttpCodeGatewayTimeout);
        }
        #endregion

        private class FileCopyInfo
        {
            internal string SourceFullPath { get; set; }

            internal string SourceFileName { get; set; }

            internal CloudBlobContainer Container { get; set; }

            internal string RelativeDirectoryName { get; set; }

            internal int RetryCount { get; set; }
        }

        private class CloudFileCopyInfo
        {
            internal CloudBlockBlob SourceBlob { get; set; }

            internal string FullFileName { get; set; }

            internal int RetryCount { get; set; }
        }
        #endregion
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Fabric.Dca.FileUploader;
    using System.IO;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography;
    using System.Threading.Tasks;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Blob;
    using Microsoft.WindowsAzure.Storage.RetryPolicies;

    // This class implements the logic to upload files in a folder to Azure blob storage.
    internal class AzureFileUploader : FileUploaderBase, IDisposable
    {
        // Trimmer objects responsible for deleting old files from various file shares.
        // Key is the path to the file share.
        private static readonly Dictionary<string, TrimmerInfo> Trimmers = new Dictionary<string, TrimmerInfo>();

        // Container in the blob storage account where data needs to be uploaded
        private readonly string containerName;

        // Object that measures performance
        private readonly AzureBlobPerformance perfHelper;

        private string directoryName;

        private string fabricNodeInstanceName;

        private StorageAccountFactory storageAccountFactory;

        internal AzureFileUploader(
            FabricEvents.ExtensionsEvents traceSource,
            string logSourceId,
            string folderName,
            string destinationPath,
            string workFolder,
            StorageAccountFactory storageAccountFactory,
            string containerName,
            TimeSpan uploadIntervalMinutes,
            TimeSpan fileSyncIntervalInMinutes,
            TimeSpan blobDeletionAgeMinutes,
            string fabricNodeInstanceName,
            string deploymentId)
            : base(
                traceSource,
                logSourceId,
                folderName,
                destinationPath,
                workFolder,
                uploadIntervalMinutes,
                fileSyncIntervalInMinutes)
        {
            // Initialization
            this.storageAccountFactory = storageAccountFactory;
            this.containerName = containerName;
            this.directoryName = AzureUtility.IsAzureInterfaceAvailable()
                ? string.Join(
                    "/",
                    string.IsNullOrEmpty(deploymentId) ? AzureUtility.DeploymentId : deploymentId,
                    AzureUtility.RoleName,
                    AzureUtility.RoleInstanceId)
                : fabricNodeInstanceName;
            this.fabricNodeInstanceName = fabricNodeInstanceName;

            this.perfHelper = new AzureBlobPerformance(this.TraceSource, this.LogSourceId);

            // Blob copy is done one at a time, so the concurrency count is 1.
            this.perfHelper.ExternalOperationInitialize(
                ExternalOperationTime.ExternalOperationType.BlobCopy,
                1);

            // Create the container at the destination
            try
            {
                Utility.PerformWithRetries(
                    this.CreateContainer,
                    (object)null,
                    new RetriableOperationExceptionHandler(this.AzureStorageExceptionHandler));
            }
            catch (Exception e)
            {
                var message = string.Format(
                    "Error creating container {0}, account {1}.",
                    this.containerName,
                    this.storageAccountFactory.Connection.AccountName);
                this.TraceSource.WriteExceptionAsError(
                    this.LogSourceId,
                    e,
                    message);
                throw new InvalidOperationException(message, e);
            }

            // Check if a trimmer already exists to delete old files from this destination.
            lock (Trimmers)
            {
                if (Trimmers.ContainsKey(destinationPath))
                {
                    // Trimmer already exists. Increment its reference count.
                    TrimmerInfo trimmerInfo = Trimmers[destinationPath];
                    trimmerInfo.RefCount++;
                }
                else
                {
                    // Trimmer does not exist. Create it.
                    AzureFileTrimmer trimmer = new AzureFileTrimmer(
                                                        folderName,
                                                        LocalMapFolderPath,
                                                        storageAccountFactory,
                                                        containerName,
                                                        blobDeletionAgeMinutes,
                                                        fabricNodeInstanceName,
                                                        deploymentId,
                                                        AzureUtility.IsAzureInterfaceAvailable());
                    TrimmerInfo trimmerInfo = new TrimmerInfo
                    {
                        RefCount = 1,
                        Trimmer = trimmer
                    };

                    Trimmers[destinationPath] = trimmerInfo;
                }
            }
        }

        public void Dispose()
        {
            this.DisposeBase();

            // Destination path is an concatenation of storage account name and container name.
            string destinationPath = string.Concat(
                                        this.storageAccountFactory.Connection.UseDevelopmentStorage ?
                                          AzureConstants.DevelopmentStorageConnectionString :
                                          this.storageAccountFactory.Connection.AccountName,
                                        ";", // This separator cannot occur in account name or container name
                                        this.containerName);
            lock (Trimmers)
            {
                if (Trimmers.ContainsKey(destinationPath))
                {
                    // Decrement reference count on the trimmer object
                    TrimmerInfo trimmerInfo = Trimmers[destinationPath];
                    trimmerInfo.RefCount--;

                    if (0 == trimmerInfo.RefCount)
                    {
                        // No one else is using the trimmer. Dispose it.
                        trimmerInfo.Trimmer.Dispose();
                        Trimmers.Remove(destinationPath);
                    }
                }
            }

            GC.SuppressFinalize(this);
        }

        protected override bool FileUploadPassBegin()
        {
            this.perfHelper.AzureBlobUploadPassBegin();
            return true;
        }

        protected override bool CopyFileToDestination(string source, string sourceRelative, int retryCount, out bool fileSkipped, out bool fileCompressed)
        {
            fileSkipped = true;
            fileCompressed = false;

            // Get the name of the source file
            string sourceFileName = Path.GetFileName(sourceRelative);

            // Create a list to hold the directory hierarchy of the source
            List<string> sourceRelativeParts = new List<string>();

            // Add each directory in the directory hierarchy to the list
            string sourceRelativePath = FabricPath.GetDirectoryName(sourceRelative);

            while (false == string.IsNullOrEmpty(sourceRelativePath))
            {
                string sourceRelativePart = Path.GetFileName(sourceRelativePath);
                sourceRelativeParts.Add(sourceRelativePart);

                sourceRelativePath = FabricPath.GetDirectoryName(sourceRelativePath);
            }

            // Reverse the list, so that top-level directories appear first
            sourceRelativeParts.Reverse();

            // Compute the directory name under the container
            string relativeDirectoryName = string.Join(
                                               "/",
                                               sourceRelativeParts.ToArray());

            // Copy the file to the Azure blob
            FileCopyInfo fileCopyInfo = new FileCopyInfo()
            {
                SourceFullPath = source,
                SourceFileName = sourceFileName,
                RelativeDirectoryName = relativeDirectoryName,
                RetryCount = retryCount
            };
            try
            {
                Utility.PerformWithRetries(
                    this.CopyFileToDestinationBlob,
                    fileCopyInfo,
                    new RetriableOperationExceptionHandler(this.AzureStorageExceptionHandler),
                    retryCount);
            }
            catch (Exception e)
            {
                this.TraceSource.WriteExceptionAsError(
                    this.LogSourceId,
                    e,
                    "Failed to copy file {0} to Azure blob account {1}, container {2}.",
                    source,
                    this.storageAccountFactory.Connection.AccountName,
                    this.containerName);
                return false;
            }

            fileSkipped = false;
            return true;
        }

        protected override void FileUploadPassEnd()
        {
            this.perfHelper.AzureBlobUploadPassEnd();
        }

#if DotNetCoreClr
        private static string GetMD5HashFromStream(byte[] data)
        {
            MD5 md5 = MD5.Create();
            byte[] blockHash = md5.ComputeHash(data);
            return Convert.ToBase64String(blockHash, 0, 16);
        }
#endif

        private static void CreateStreamAndUploadToBlob(string sourceFile, CloudBlockBlob destinationBlob)
        {
#if DotNetCoreClr

            if (destinationBlob.ExistsAsync().Result && destinationBlob.DownloadBlockListAsync().Result.Any())
            {
                // Delete all blocks in case we crashed and failed the upload in previous attempt.
                destinationBlob.PutBlockListAsync(new List<string>()).Wait();
            }

            int blockSize = 4 * 1024 * 1024; //4 MB

            using (FileStream fileStream =
              new FileStream(sourceFile, FileMode.Open, FileAccess.Read))
            {
                var tasks = new List<Task>();

                long fileSize = fileStream.Length;

                //block count is the number of blocks + 1 for the last one
                int blockCount = (int)((float)fileSize / (float)blockSize) + 1;

                //List of block ids; the blocks will be committed in the order of this list
                List<string> blockIDs = new List<string>();

                //starting block number
                int blockNumber = 0;

                int bytesRead = 0; //number of bytes read so far
                long bytesLeft = fileSize; //number of bytes left to read and upload

                //do until all of the bytes are uploaded
                while (bytesLeft > 0)
                {
                    blockNumber++;
                    int bytesToRead;
                    if (bytesLeft >= blockSize)
                    {
                        //more than one block left, so put up another whole block
                        bytesToRead = blockSize;
                    }
                    else
                    {
                        //less than one block left, read the rest of it
                        bytesToRead = (int)bytesLeft;
                    }

                    //create a blockID from the block number, add it to the block ID list
                    //the block ID is a base64 string
                    string blockId =
                        Convert.ToBase64String(System.Text.ASCIIEncoding.ASCII.GetBytes(string.Format("BlockId{0}",
                        blockNumber.ToString("d7"))));

                    blockIDs.Add(blockId);
                    //set up new buffer with the right size, and read that many bytes into it
                    byte[] bytes = new byte[bytesToRead];
                    fileStream.Read(bytes, 0, bytesToRead);

                    //calculate the MD5 hash of the byte array
                    string blockHash = GetMD5HashFromStream(bytes);

                    //upload the block, provide the hash so Azure can verify it
                    tasks.Add(destinationBlob.PutBlockAsync(blockId, new MemoryStream(bytes), blockHash));

                    //increment/decrement counters
                    bytesRead += bytesToRead;
                    bytesLeft -= bytesToRead;

                    // Do not start more instances to limit memory usage
                    if (tasks.Count == 5)
                    {
                        Task.WaitAll(tasks.ToArray());
                        tasks.Clear();
                    }
                }

                if (tasks.Any())
                {
                    Task.WaitAll(tasks.ToArray());
                }

                //commit the blocks
                destinationBlob.PutBlockListAsync(blockIDs).Wait();
            }
#else
            using (FileStream fs = new FileStream(sourceFile, FileMode.Open, FileAccess.Read))
            {
                destinationBlob.UploadFromStream(fs, null, null, null);
            }
#endif
        }

        private void CreateContainer(object context)
        {
            var container = this.storageAccountFactory.GetContainer(this.containerName);

#if DotNetCoreClr
                // Do not remove below line.
                Console.WriteLine("DateTime.Now.Ticks: {0}", DateTime.Now.Ticks.ToString());
                bool retVal = container.CreateIfNotExistsAsync().Result;
                Console.WriteLine("Container creation complete with return value: {0}", retVal);
#else
            container.CreateIfNotExists(null, null);
#endif
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
                    this.LogSourceId,
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
                string tempDir = Utility.GetTempDirName();
                string tempDest = Path.Combine(tempDir, fileCopyInfo.SourceFileName);
                FabricDirectory.CreateDirectory(tempDir);
                try
                {
                    FabricFile.Copy(fileCopyInfo.SourceFullPath, tempDest, true);
                    CreateStreamAndUploadToBlob(tempDest, destinationBlob);
                    this.TraceSource.WriteInfo(
                    this.LogSourceId,
                    "Uploaded file {0} to destination blob {1}",
                    fileCopyInfo.SourceFullPath,
                    destinationBlob.Name);
                }
                finally
                {
                    try
                    {
                        Utility.PerformIOWithRetries(
                            () =>
                            {
                                FabricDirectory.Delete(tempDir, true);
                            });
                    }
                    catch (Exception e)
                    {
                        this.TraceSource.WriteExceptionAsError(
                            this.LogSourceId,
                            e,
                            "Failed to delete temporary directory {0}.",
                            tempDir);
                    }
                }
            }
        }

        private void CopyFileToDestinationBlob(object context)
        {
            this.perfHelper.ExternalOperationBegin(
                ExternalOperationTime.ExternalOperationType.BlobCopy,
                0);

            FileCopyInfo fileCopyInfo = (FileCopyInfo)context;
            var destinationDirectory = this.GetDirectoryForUpload(this.containerName, this.directoryName, fileCopyInfo.RelativeDirectoryName);

            // Get the blob object
            CloudBlockBlob destinationBlob = destinationDirectory.GetBlockBlobReference(fileCopyInfo.SourceFileName);

            // Copy the file to the blob
            this.CopyFileToDestinationBlobWorker(fileCopyInfo, destinationBlob);

            this.perfHelper.ExternalOperationEnd(
                ExternalOperationTime.ExternalOperationType.BlobCopy,
                0);
            this.perfHelper.AzureBlobUploaded();
        }

        private void TryFileCompression(ref FileCopyInfo fileCopyInfo, out bool fileCompressed)
        {
            fileCompressed = false;

            if (fileCopyInfo.SourceFileName.EndsWith(".zip"))
            {
                // already compressed skip it.
                return;
            }

            string tempArchivePath = string.Empty;
            string actualFilePath = fileCopyInfo.SourceFullPath;
            string archiveEntryName = Path.GetFileNameWithoutExtension(fileCopyInfo.SourceFileName);
            try
            {
                FileCompressor.Compress(fileCopyInfo.SourceFullPath, archiveEntryName, out tempArchivePath);
                Utility.PerformIOWithRetries(
                        () => { FabricFile.Delete(actualFilePath); });
            }
            catch (Exception e)
            {
                // Consume the exception and upload the uncompressed file.
                this.TraceSource.WriteExceptionAsWarning(
                        this.LogSourceId,
                        e,
                        "Failed to compress crash dump for upload");
                return;
            }

            fileCopyInfo.SourceFileName = archiveEntryName + ".zip";
            fileCopyInfo.SourceFullPath = tempArchivePath;
            fileCompressed = true;
        }

        private CloudBlobDirectory GetDirectoryForUpload(string containerName, string directoryName, string relativeDirectoryName)
        {
            CloudBlobDirectory directory = null;

            try
            {
                Utility.PerformWithRetries(
                    (object context) => { directory = this.GetDirectoryReferenceForUpload(containerName, directoryName, relativeDirectoryName); },
                    (object)null,
                    new RetriableOperationExceptionHandler(this.AzureStorageExceptionHandler));
            }
            catch (Exception e)
            {
                this.TraceSource.WriteExceptionAsError(
                    this.LogSourceId,
                    e,
                    "Error obtaining directory reference for directory {0}, container {1}, account {2}.",
                    this.directoryName,
                    this.containerName,
                    this.storageAccountFactory.Connection.AccountName);
            }

            return directory;
        }

        private CloudBlobDirectory GetDirectoryReferenceForUpload(string containerName, string directoryName, string relativeDirectoryName)
        {
            var directory = this.storageAccountFactory.GetDirectory(containerName, directoryName, relativeDirectoryName);

#if DotNetCoreClrLinux
            // This special handling is needed to decide path to upload, long or short
            // <blob>/<deployment id>/<rolename>/<roleinstance id>
            // or
            // <blob>/<node instance name>
            //
            // The existing clusters will continue to upload to long paths
            // New clusters from this DCA version onwards will upload to shorter paths
            // This code is not needed on windows as windows fixes this by using registry
            // and fabricsetup.exe
            try
            {
                BlobContinuationToken blobContinuationToken = new BlobContinuationToken();
                if (!directory.ListBlobsSegmentedAsync(blobContinuationToken).Result.Results.Any())
                {
                    this.TraceSource.WriteInfo(
                        this.LogSourceId,
                        "Traces will be uploaded in shorter format.");

                    // Update these variables to use shorter paths on target blob.
                    this.directoryName = this.fabricNodeInstanceName;
                    directory = this.storageAccountFactory.GetDirectory(containerName, this.directoryName, relativeDirectoryName);
                }
            }
            catch (AggregateException ae)
            {
                throw ae.Flatten().InnerException;
            }
#endif
            return directory;
        }

        private RetriableOperationExceptionHandler.Response AzureStorageExceptionHandler(Exception e)
        {
            if (e is StorageException)
            {
                // If this was a network error, we'll retry later. Else, we'll
                // just give up.
                StorageException storageException = (StorageException)e;
                if (Utility.IsNetworkError(storageException.RequestInformation.HttpStatusCode))
                {
                    return RetriableOperationExceptionHandler.Response.Retry;
                }

                this.TraceSource.WriteExceptionAsWarning(
                    this.LogSourceId,
                    e,
                    "DCA encountered an unhandled storage exception with http code {0}",
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

        private class TrimmerInfo
        {
            internal int RefCount { get; set; }

            internal AzureFileTrimmer Trimmer { get; set; }
        }

        private class FileCopyInfo
        {
            internal string SourceFullPath { get; set; }

            internal string SourceFileName { get; set; }

            internal string RelativeDirectoryName { get; set; }

            internal int RetryCount { get; set; }
        }
    }
}

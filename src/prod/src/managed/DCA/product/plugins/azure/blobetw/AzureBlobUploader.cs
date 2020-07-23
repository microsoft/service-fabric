// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Fabric.Dca.EtlConsumerHelper;
    using System.Globalization;
    using System.IO;
    using System.IO.Compression;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Blob;
    using Microsoft.WindowsAzure.Storage.RetryPolicies;

    // This class implements the logic to upload compressed memory stream to Azure blob storage
    // and if that fails, persists the compressed memory content to disk
    internal class AzureBlobUploader : IDisposable
    {
        internal const string LocalMapFolder = "LMap";

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Container in the blob storage account where data needs to be uploaded
        private readonly string containerName;

        // Azure directory name used to get a reference to the directory object
        private readonly string directoryName;

        // Fabric node ID
        private readonly string fabricNodeId;

        // Object that measures performance
        private readonly AzureBlobPerformance perfHelper;

        // Local folder containing a map of files that we uploaded
        private readonly string localMap;

        // Block FlushData calls while async file uploader timer callback is executing.
        private readonly object fileUploaderLock = new object();

        private readonly StorageAccountFactory storageAccountFactory;

        // Current ETL file being processed
        private readonly string etlFileName;

        // Whether current ETL file is the active file
        private readonly bool isActiveEtl;

        // Stream writer to which trace events will be written to
        private readonly StreamWriter streamWriter;

        // Folder to hold compressed CSV files in case of 
        // failure to upload compressed memory stream to blob storage
        private readonly string etwLogDirName;

        // Azure in-memory stream to blob uploader will check this access condition before uploading.
        // Used by tests to make the upload in-memory zipped stream code path fail 
        // allowing flow though the upload from persisted file code path.
        private readonly AccessCondition uploadStreamAccessCondition;

        // Azure file stream to blob uploader will check this access condition before uploading.
        private readonly AccessCondition uploadFileAccessCondition;

        // Whether or not the object has been disposed
        private bool disposed;

        // Whether or not we are in the process of stopping
        private bool stopping;

        // Last event index processed
        private EventIndex lastEventIndexProcessed;

        internal AzureBlobUploader(
           FabricEvents.ExtensionsEvents traceSource,
           string logSourceId,
           string etwLogDirName,
           string workFolder,
           StorageAccountFactory storageAccountFactory,
           string containerName,
           string fabricNodeInstanceName,
           string deploymentId,
           AzureBlobPerformance perfHelper,
           AccessCondition uploadStreamAccessCondition,
           AccessCondition uploadFileAccessCondition)
            : this(
            traceSource,
            logSourceId,
            etwLogDirName,
            workFolder,
            storageAccountFactory,
            string.Empty,
            false,
            containerName,
            string.Empty,
            fabricNodeInstanceName,
            deploymentId,
            perfHelper,
            uploadStreamAccessCondition,
            uploadFileAccessCondition)
        {
        }

        internal AzureBlobUploader(
            FabricEvents.ExtensionsEvents traceSource, 
            string logSourceId, 
            string etwLogDirName,
            string workFolder,
            StorageAccountFactory storageAccountFactory,
            string etlFileName,
            bool isActiveEtl,
            string containerName, 
            string fabricNodeId,
            string fabricNodeInstanceName, 
            string deploymentId,
            AzureBlobPerformance perfHelper,
            AccessCondition uploadStreamAccessCondition,
            AccessCondition uploadFileAccessCondition)
        {
            this.stopping = false;
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;
            this.etwLogDirName = etwLogDirName;
            this.storageAccountFactory = storageAccountFactory;
            this.containerName = containerName;
            this.etlFileName = etlFileName;
            this.isActiveEtl = isActiveEtl;
            this.fabricNodeId = fabricNodeId;
            this.directoryName = AzureUtility.IsAzureInterfaceAvailable()
                ? string.Join(
                    "/",
                    string.IsNullOrEmpty(deploymentId) ? AzureUtility.DeploymentId : deploymentId,
                    AzureUtility.RoleName,
                    AzureUtility.RoleInstanceId)
                : fabricNodeInstanceName;

            this.localMap = Path.Combine(workFolder, LocalMapFolder);

            this.uploadStreamAccessCondition = uploadStreamAccessCondition;
            this.uploadFileAccessCondition = uploadFileAccessCondition;

            this.perfHelper = perfHelper;

            // Blob copy is done one at a time, so the concurrency count is 1.
            this.perfHelper.ExternalOperationInitialize(
                ExternalOperationTime.ExternalOperationType.BlobCopy,
                1);

            this.streamWriter = new StreamWriter(new MemoryStream());
                
            this.lastEventIndexProcessed.Set(DateTime.MinValue, -1);

            // Create the container at the destination
            try
            {
                Utility.PerformWithRetries(
                    this.CreateContainer,
                    (object)null,
                    new RetriableOperationExceptionHandler(this.AzureStorageExceptionHandler),
                    AzureBlobEtwConstants.MethodExecutionInitialRetryIntervalMs,
                    AzureBlobEtwConstants.MethodExecutionMaxRetryCount,
                    AzureBlobEtwConstants.MethodExecutionMaxRetryIntervalMs);
            }
            catch (Exception e)
            {
                var message = string.Format(
                    "Error creating container {0}, account {1}.",
                    this.containerName,
                    this.storageAccountFactory.Connection.AccountName);
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    message);
                throw new InvalidOperationException(message, e);
            }
        }

        // Last event index processed
        internal EventIndex LastEventIndexProcessed
        {
            get
            {
                return this.lastEventIndexProcessed;
            }
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            this.stopping = true;

            if (this.streamWriter != null)
            {
                this.streamWriter.Dispose();
            }

            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Action passed to EtlToInMemoryBufferWriter that
        /// writes the decoded event to a memory stream.
        /// </summary>
        /// <param name="decodedEventWrapper"></param>
        internal void ProcessTraceEvent(DecodedEventWrapper decodedEventWrapper)
        {
            if (this.stopping)
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "The consumer is being stopped. Therefore, no more in-memory trace events will be processed in this pass.");
                return;
            }

            try
            {
                this.WriteEvent(decodedEventWrapper.StringRepresentation.Replace("\r\n", "\r\t").Replace("\n", "\t"));
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Failed to write filtered event {0} to memory stream.",
                    decodedEventWrapper.StringRepresentation);
            }

            // update last event index processed
            this.lastEventIndexProcessed.Set(decodedEventWrapper.Timestamp, decodedEventWrapper.TimestampDifferentiator);
        }

        /// <summary>
        /// Performs actual uploading of compressed stream. If that fails then the
        /// compressed stream is persisted to disk.
        /// </summary>
        /// <param name="traceFileSubFolder"></param>
        internal void UploadStreamToDestinationBlob(string traceFileSubFolder)
        {
            if (false == this.UploadStream(traceFileSubFolder))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Failed to upload memory stream for file {0} to destination blob.",
                    this.etlFileName);

                // Write memory stream to file
                if (false == this.SaveStream(traceFileSubFolder))
                {
                    this.traceSource.WriteError(
                    this.logSourceId,
                    "Failed to save memory stream for file {0} to disk.",
                    this.etlFileName);
                }
                else
                {
                    this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Successfully saved memory stream for file {0}.",
                    this.etlFileName);
                }
            }

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Successfully uploaded memory stream for file {0}.",
                this.etlFileName);
        }

        /// <summary>
        /// Uploads compressed files persisted to disk to Azure Blob storage.
        /// </summary>
        internal void UploadFilesToDestinationBlob()
        {
            lock (this.fileUploaderLock)
            {
                // Initialization
                if (false == this.CreateLocalMapFolder())
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Failed to create directory for local map.");
                    return;
                }

                try
                {
                    DirectoryInfo parentDirectory = new DirectoryInfo(this.etwLogDirName);

                    var subDirectories = parentDirectory.EnumerateDirectories();

                    foreach (var subDirectory in subDirectories)
                    {
                        var files = subDirectory.EnumerateFiles();

                        foreach (var file in files)
                        {
                            if (false == this.UploadFile(file.Directory.Name, file.FullName, file.Name))
                            {
                                this.traceSource.WriteError(
                                    this.logSourceId,
                                    "Failed to upload file {0} to blob storage.",
                                    file.Name);
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    this.traceSource.WriteError(
                       this.logSourceId,
                       "Failed to upload files to destination blob: {0}",
                       ex);
                }
            }
        }

        private bool UploadFile(string traceFileSubFolder, string filePath, string fileName)
        {
            FileCopyInfo fileCopyInfo = new FileCopyInfo()
            {
                SourceFullPath = filePath,
                SourceFileName = fileName,
                RelativeDirectoryName = traceFileSubFolder
            };

            // Figure out the name of the directory in the local map.
            string localMapDestination = Path.Combine(this.localMap, fileCopyInfo.RelativeDirectoryName, fileCopyInfo.SourceFileName);
            string localMapDestinationDir;
            try
            {
                localMapDestinationDir = FabricPath.GetDirectoryName(localMapDestination);
            }
            catch (PathTooLongException e)
            {
                // The path to the local map directory is too long. Skip it.
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Failed to get local map directory for uploading file {0}. Exception information: {1}",
                    fileCopyInfo.SourceFileName,
                    e);
                return false;
            }

            // File has already been uploaded before
            if (true == FabricFile.Exists(localMapDestination))
            {
                return true;
            }

            try
            {
                Utility.PerformWithRetries(
                    this.UploadFileWorker,
                    fileCopyInfo,
                    new RetriableOperationExceptionHandler(this.AzureStorageExceptionHandler),
                    AzureBlobEtwConstants.MethodExecutionInitialRetryIntervalMs,
                    AzureBlobEtwConstants.MethodExecutionMaxRetryCount,
                    AzureBlobEtwConstants.MethodExecutionMaxRetryIntervalMs);
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Failed to copy file {0} to Azure blob account {1}, container {2}.",
                    fileName,
                    this.storageAccountFactory.Connection.AccountName,
                    this.containerName);
                return false;
            }

            if (false == this.UpdateLocalMap(localMapDestinationDir, localMapDestination))
            {
                return false;
            }

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Successfully uploaded file {0} to blob storage.",
                fileCopyInfo.SourceFileName);

            return true;
        }

        private void UploadFileWorker(object context)
        {
            FileCopyInfo fileCopyInfo = (FileCopyInfo)context;

            var destinationDirectory = this.GetDirectoryForUpload(this.containerName, this.directoryName, fileCopyInfo.RelativeDirectoryName);
            
            // Get the blob object
            CloudBlockBlob destinationBlob = destinationDirectory.GetBlockBlobReference(fileCopyInfo.SourceFileName);

            // Copy the file to the blob
            this.UploadFileWorker(fileCopyInfo.SourceFullPath, destinationBlob);
        }

        private void UploadFileWorker(string sourceFilePath, CloudBlockBlob destinationBlob)
        {
            this.perfHelper.ExternalOperationBegin(
                ExternalOperationTime.ExternalOperationType.BlobCopy,
                0);

            using (var fs = new FileStream(sourceFilePath, FileMode.Open, FileAccess.Read))
            {
                destinationBlob.UploadFromStream(fs, this.uploadFileAccessCondition, null, null);
            }

            // log bytes read and written
            var fileInfo = new FileInfo(sourceFilePath);
            this.perfHelper.BytesRead(fileInfo.Length);
            this.perfHelper.BytesWritten(fileInfo.Length);

            this.perfHelper.ExternalOperationEnd(
                ExternalOperationTime.ExternalOperationType.BlobCopy,
                0);

            this.perfHelper.AzureBlobUploaded();
        }

        private void CreateContainer(object context)
        {
            var container = this.storageAccountFactory.GetContainer(this.containerName);
            container.CreateIfNotExists(null, null);
        }

        private bool CreateLocalMapFolder()
        {
            bool success = false;

            if (FabricDirectory.Exists(this.localMap))
            {
                return true;
            }

            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    {
                        FabricDirectory.CreateDirectory(this.localMap);
                    },
                    AzureBlobEtwConstants.MethodExecutionInitialRetryIntervalMs,
                    AzureBlobEtwConstants.MethodExecutionMaxRetryCount,
                    AzureBlobEtwConstants.MethodExecutionMaxRetryIntervalMs);
                success = true;
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    string.Format("Unable to create directory {0}.", this.localMap));
            }

            return success;
        }

        private CloudBlobDirectory GetDirectoryForUpload(string containerName, string directoryName, string relativeDirectoryName)
        {
            CloudBlobDirectory directory = null;

            try
            {
                Utility.PerformWithRetries(
                    (object context) => { directory = this.storageAccountFactory.GetDirectory(containerName, directoryName, relativeDirectoryName); },
                    (object)null,
                    new RetriableOperationExceptionHandler(this.AzureStorageExceptionHandler),
                    AzureBlobEtwConstants.MethodExecutionInitialRetryIntervalMs,
                    AzureBlobEtwConstants.MethodExecutionMaxRetryCount,
                    AzureBlobEtwConstants.MethodExecutionMaxRetryIntervalMs);
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Error obtaining directory reference for directory {0}, container {1}, account {2}.",
                    this.directoryName,
                    this.containerName,
                    this.storageAccountFactory.Connection.AccountName);
            }

            return directory;
        }

        private RetriableOperationExceptionHandler.Response AzureStorageExceptionHandler(Exception e)
        {
            this.traceSource.WriteError(
                this.logSourceId,
                "Exception encountered while accessing Azure storage account. Exception information: {0}",
                e);

            if (e is StorageException)
            {
                // If this was a network error, we'll retry later. Else, we'll
                // just give up.
                StorageException storageException = (StorageException)e;
                if (Utility.IsNetworkError(storageException.RequestInformation.HttpStatusCode))
                {
                    return RetriableOperationExceptionHandler.Response.Retry;
                }
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

        /// <summary>
        /// Create destination file name for filtered trace.
        /// </summary>
        /// <param name="etlFileName"></param>
        /// <param name="isActiveEtl"></param>
        /// <returns></returns>
        private string CreateDestinationFileName(string etlFileName, bool isActiveEtl)
        {
            string differentiator = string.Format(
                                        CultureInfo.InvariantCulture,
                                        "{0:D10}",
                                        isActiveEtl ? this.lastEventIndexProcessed.TimestampDifferentiator : int.MaxValue);

            string traceFileNamePrefix = string.Format(
                                                  CultureInfo.InvariantCulture,
                                                  "{0}_{1}_{2:D20}_{3}.",
                                                  this.fabricNodeId,
                                                  Path.GetFileNameWithoutExtension(etlFileName),
                                                  this.lastEventIndexProcessed.Timestamp.Ticks,
                                                  differentiator);

            var applicationInstanceId = ContainerEnvironment.GetContainerApplicationInstanceId(this.logSourceId);
            if (ContainerEnvironment.IsContainerApplication(applicationInstanceId))
            {
                // Note that the a hash of the applicationInstanceId is being used to reduce file name length in around 70 characters
                // This is done to workaround PathTooLong exception in FileUploaderBase.cs since we don't have an interop for FileSystemWatcher
                // and .NET 4.5 used does not support long paths yet.
                traceFileNamePrefix = string.Format(
                                                CultureInfo.InvariantCulture,
                                                "{0}_{1:X8}_{2}_{3:D20}_{4}.",
                                                this.fabricNodeId,
                                                Path.GetFileName(applicationInstanceId).GetHashCode(),
                                                Path.GetFileNameWithoutExtension(etlFileName),
                                                this.lastEventIndexProcessed.Timestamp.Ticks,
                                                differentiator);
            }

            return string.Concat(
                traceFileNamePrefix,
                EtlConsumerConstants.FilteredEtwTraceFileExtension);
        }

        private bool UploadStream(string traceFileSubFolder)
        {
            CloudBlobDirectory destinationDirectory = this.GetDirectoryForUpload(this.containerName, this.directoryName, traceFileSubFolder);
            if (destinationDirectory == null)
            {
                return false;
            }

            // Get the compressed file name for blob storage upload
            var destinationFileName = this.CreateDestinationFileName(this.etlFileName, this.isActiveEtl);
            var compressedDestinationFileName = string.Concat(destinationFileName, EtlConsumerConstants.CompressedFilteredEtwTraceFileExtension);

            // Get the blob object
            CloudBlockBlob destinationBlob = destinationDirectory.GetBlockBlobReference(compressedDestinationFileName);

            try
            {
                Utility.PerformWithRetries(
                    this.UploadStreamWorker,
                    destinationBlob,
                    new RetriableOperationExceptionHandler(this.AzureStorageExceptionHandler),
                    AzureBlobEtwConstants.MethodExecutionInitialRetryIntervalMs,
                    AzureBlobEtwConstants.MethodExecutionMaxRetryCount,
                    AzureBlobEtwConstants.MethodExecutionMaxRetryIntervalMs);
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Failed to copy file {0} to Azure blob account {1}, container {2}.",
                    compressedDestinationFileName,
                    this.storageAccountFactory.Connection.AccountName,
                    this.containerName);

                return false;
            }

            return true;
        }

        private void UploadStreamWorker(object context)
        {
            this.perfHelper.ExternalOperationBegin(
                ExternalOperationTime.ExternalOperationType.BlobCopy,
                0);

            CloudBlockBlob destinationBlob = (CloudBlockBlob)context;

            using (var zipStream = new MemoryStream())
            {
                using (var zipArchive = new ZipArchive(zipStream, ZipArchiveMode.Create, true))
                {
                    var archiveName = this.CreateDestinationFileName(this.etlFileName, this.isActiveEtl);
                    var zipArchiveEntry = zipArchive.CreateEntry(archiveName, CompressionLevel.Optimal);
                    using (var sw = zipArchiveEntry.Open())
                    {
                        this.streamWriter.BaseStream.Position = 0;
                        this.streamWriter.BaseStream.CopyTo(sw);
                    }
                }

                zipStream.Position = 0;
                destinationBlob.UploadFromStream(zipStream, this.uploadStreamAccessCondition, null, null);

                // log bytes written
                this.perfHelper.BytesWritten(zipStream.Length);
            }

            this.perfHelper.ExternalOperationEnd(
                ExternalOperationTime.ExternalOperationType.BlobCopy,
                0);

            this.perfHelper.AzureBlobUploaded();
        }

        private bool SaveStream(string traceFileSubFolder)
        {
            var traceFileDestinationPath = Path.Combine(
                this.etwLogDirName,
                traceFileSubFolder);

            var destinationFileName = this.CreateDestinationFileName(this.etlFileName, this.isActiveEtl);
            var compressedDestinationFileName = string.Concat(destinationFileName, EtlConsumerConstants.CompressedFilteredEtwTraceFileExtension);
            var compressedTraceFileDestinationPath = Path.Combine(traceFileDestinationPath, compressedDestinationFileName);

            try
            {
                Utility.PerformIOWithRetries(
                    this.SaveStreamWorker,
                    compressedTraceFileDestinationPath,
                    AzureBlobEtwConstants.MethodExecutionInitialRetryIntervalMs,
                    AzureBlobEtwConstants.MethodExecutionMaxRetryCount,
                    AzureBlobEtwConstants.MethodExecutionMaxRetryIntervalMs);
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Failed to save memory stream to disk for file {0}.",
                    this.etlFileName);
                return false;
            }

            return true;
        }

        private void SaveStreamWorker(object context)
        {
            var compressedTraceFileDestinationPath = (string)context;

            using (var fs = new FileStream(compressedTraceFileDestinationPath, FileMode.Create))
            {
                using (var zipArchive = new ZipArchive(fs, ZipArchiveMode.Create))
                {
                    var archiveName = this.CreateDestinationFileName(this.etlFileName, this.isActiveEtl);
                    var zipArchiveEntry = zipArchive.CreateEntry(archiveName, CompressionLevel.Optimal);
                    using (var sw = zipArchiveEntry.Open())
                    {
                        this.streamWriter.BaseStream.Position = 0;
                        this.streamWriter.BaseStream.CopyTo(sw);
                    }
                }
            }

            // log bytes written
            var fileInfo = new FileInfo(compressedTraceFileDestinationPath);
            this.perfHelper.BytesWritten(fileInfo.Length);
        }

        private void WriteEvent(string eventString)
        {
            Utility.PerformIOWithRetries(
                this.WriteEventWorker,
                eventString,
                AzureBlobEtwConstants.MethodExecutionInitialRetryIntervalMs,
                AzureBlobEtwConstants.MethodExecutionMaxRetryCount,
                AzureBlobEtwConstants.MethodExecutionMaxRetryIntervalMs);
        }

        private void WriteEventWorker(object context)
        {
            string eventString = (string)context;
            try
            {
                this.streamWriter.WriteLine(eventString);
                this.streamWriter.Flush();
            }
            catch (System.Text.EncoderFallbackException e)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Exception occurred while writing filtered ETW event to memory stream. Exception information: {0}",
                    e);
            }
        }

        private bool UpdateLocalMap(string localMapDestinationDir, string localMapDestination)
        {
            // Figure out in which directory in the local map we will
            // record information about this file. Create that directory
            // if it doesn't exist.
            if ((false == string.IsNullOrEmpty(localMapDestinationDir)) &&
                (false == FabricDirectory.Exists(localMapDestinationDir)))
            {
                try
                {
                    Utility.PerformIOWithRetries(
                        () => { FabricDirectory.CreateDirectory(localMapDestinationDir); },
                        AzureBlobEtwConstants.MethodExecutionInitialRetryIntervalMs,
                        AzureBlobEtwConstants.MethodExecutionMaxRetryCount,
                        AzureBlobEtwConstants.MethodExecutionMaxRetryIntervalMs);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Failed to create directory {0} in the local map of uploaded files.",
                        localMapDestinationDir);
                    return false;
                }
            }

            // If the file doesn't exist in the local map, create it.
            if (false == FabricFile.Exists(localMapDestination))
            {
                if (false == this.CreateLocalMapFile(localMapDestination))
                {
                    return false;
                }
            }

            return true;
        }

        private bool CreateLocalMapFile(string localMapDestination)
        {
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    {
                        using (FileStream fs = FabricFile.Create(localMapDestination))
                        {
                        }
                    },
                    AzureBlobEtwConstants.MethodExecutionInitialRetryIntervalMs,
                    AzureBlobEtwConstants.MethodExecutionMaxRetryCount,
                    AzureBlobEtwConstants.MethodExecutionMaxRetryIntervalMs);
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Failed to create local map file {0}.",
                    localMapDestination);
                return false;
            }

            return true;
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
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageStore
{
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Blob;
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.ImageStore;
    using System.Globalization;
    using System.IO;
    using System.Threading;

    /// <summary>
    /// The class performs file operations by starting
    /// a worker threads; the thread get tasks from a task queue
    /// </summary>
    [SuppressMessage("Microsoft.Design", "CA1001:TypesThatOwnDisposableFieldsShouldBeDisposable",
        Justification = "The logger is a wrapper arround handle and there is nothing to dispose, reviewed ok.")]
    internal class XStoreFileOperationsWorker
    {
        //// Private FabricValidatorConstants:

        /// <summary>
        /// Meta name of blob to indicate the last write time utc (to compare if files are different)
        /// </summary>
        private const string LastWriteTimeUTC = "LastWriteTimeUtc";

        /// <summary>Maximum retry time for a file operation </summary>
        private const int MaxRetryAttempts = 10;

        /// <summary>The next retry should not be prior to MIN_RETRY_IN_SECONDS</summary>
        private const int MinRetryInSeconds = 1;

        /// <summary>The next retry should not be later than MAX_RETRY_IN_SECONDS </summary>
        private const int MaxRetryInSeconds = 60;

        /// <summary>The threshold for starting parralel downloading a single file (azure blob handles upload but not download) </summary>
        private const int DownloadSizeThreshold = 16 * 1024 * 1024;

        /// <summary>Download size of each parallel download thread (for multi thread downloading a single file) </summary>
        private const int ParrallelDownloadSize = 8 * 1024 * 1024;

        /// <summary>Maximum thread number to parralel download a single file (similar to PARALLEL_OPERATIONTHREAD_COUNT for upload) </summary>
        private const int ParrallelDownloadThreadCount = 50;

        /// <summary>Extesion to mark new folder in atomic copies </summary>
        private const string NewExtension = ".new";

        /// <summary>Extesion to mark old folder in atomic copies </summary>
        private const string OldExtension = ".old";

        /// <summary>Class name </summary>
        private const string ClassName = "XStoreFileOperationsWorker";

        /// <summary>Default timeout (seconds) for an operation to be completed </summary>
        private const int DefaultTimeoutInSeconds = 120;

        /// <summary>
        /// Source for writing traces.
        /// </summary>
        private static FabricEvents.ExtensionsEvents traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.ImageStoreClient);

        /// <summary>Sync events to sync the multi threads file operation</summary>
        private readonly XStoreFileOperationSyncEvents syncEvents = null;

        /// <summary>ID of the file operation worker (to be removed)</summary>
        private readonly int workerID;

        /// <summary>XstoreTaskPool object </summary>
        private readonly XStoreTaskPool xstoreTaskPool = null;

        /// <summary>File extensions that are to be excluded from file operations</summary>
        private readonly Dictionary<string, bool> filterExtensions = null;

        /// <summary>File names that are to be excluded from file operations</summary>
        private readonly HashSet<string> filterFileNames = null;

        /// <summary>XStore params </summary>
        private readonly XStoreParameters storeParams = null;

        /// <summary>Worker thread's handle</summary>
        private Thread threadInst = null;

        /// <summary>Default blob request option </summary>
        private BlobRequestOptions defaultRequestOption = null;

        /// <summary>Bool flag to indicate if the worker has encountered fatal error </summary>
        private bool hasFatalError = false;

        /// <summary>Bool flag to indicate if the worker has encountered timeout error </summary>
        private bool hasTimeoutError = false;

        /// <summary>
        /// Initializes a new instance of the XStoreFileOperationsWorker class.
        /// </summary>
        /// <param name="xstoreTaskPool">The task pool.</param>
        /// <param name="syncEvent">The synchronizatino event.</param>
        /// <param name="storeParams">The store parameters.</param>
        /// <param name="filterExtensions">The filter extensions.</param>
        /// <param name="filterFileNames"></param>
        /// <param name="workerID">The worker Id.</param>
        public XStoreFileOperationsWorker(
            XStoreTaskPool xstoreTaskPool,
            XStoreFileOperationSyncEvents syncEvent,
            XStoreParameters storeParams,
            Dictionary<string, bool> filterExtensions,
            HashSet<string> filterFileNames,
            int workerID)
        {
            this.xstoreTaskPool = xstoreTaskPool;
            this.syncEvents = syncEvent;
            this.workerID = workerID;
            this.filterExtensions = filterExtensions;
            this.filterFileNames = filterFileNames;
            this.storeParams = storeParams;
            this.hasFatalError = false;
        }

        /// <summary>
        /// Gets the thread handle
        /// </summary>
        internal Thread ThreadInst
        {
            get { return this.threadInst; }
        }

        /// <summary>
        /// Gets a value indicating whether a fatal error has occured.
        /// </summary>
        internal bool HasFatalError
        {
            get { return this.hasFatalError; }
        }

        /// <summary>
        /// Gets a value indicating whether a timeout error has occured.
        /// </summary>
        internal bool HasTimeoutError
        {
            get { return this.hasTimeoutError; }
        }


        /// <summary>
        /// Start thread to execute file operations in the queue
        /// </summary>
        internal void Start()
        {
            // set up default blob request option
            this.defaultRequestOption = new BlobRequestOptions
            {
                RetryPolicy = new Microsoft.WindowsAzure.Storage.RetryPolicies.NoRetry(),
                MaximumExecutionTime = new TimeSpan(0, 0, DefaultTimeoutInSeconds)
            };

            // start uploader thread
            this.threadInst = new Thread(this.ThreadRun);
            this.threadInst.Start();
        }

        /// <summary>
        /// Delete a file (blob) from XStore
        /// </summary>
        /// <param name="blobContainer">The container from which to delete.</param>
        /// <param name="task">The task to perform.</param>
        private static void DeleteFileFromXStore(CloudBlobContainer blobContainer, XStoreFileOperationTask task)
        {
            var blob = blobContainer.GetBlockBlobReference(task.SrcUri);
#if !DotNetCoreClr
            blob.DeleteIfExists();
#else
            blob.DeleteIfExistsAsync().Wait();
#endif
        }

        /// <summary>
        /// Rename a folder; retry if it doesn't succeed
        /// </summary>
        /// <param name="srcFolderName">
        /// Source folder name
        /// </param>
        /// <param name="dstFolderName">
        /// Destination folder name
        /// </param>
        [SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes",
            Justification = "Rename folder shouldn't throw exception as it tries its best to copy rest")]
        private static void RenameFolder(string srcFolderName, string dstFolderName)
        {
            if (string.IsNullOrEmpty(srcFolderName) || !FabricDirectory.Exists(srcFolderName))
            {
                throw new ArgumentException("Invalid source folder name specified");
            }

            if (string.IsNullOrEmpty(dstFolderName) || FabricDirectory.Exists(dstFolderName))
            {
                throw new ArgumentException("Invalid destination folder name specified");
            }

            for (int i = 1; i <= MaxRetryAttempts; i++)
            {
                try
                {
                    FabricFile.Move(srcFolderName, dstFolderName);
                    if (i > 1)
                    {
                        // Try sleeping second time around
                        Thread.Sleep(500);
                    }

                    //This is for the rare case of renaming failed without any error from the common library; referring to incident #45017448
                    if (!FabricDirectory.Exists(dstFolderName))
                    {
                        traceSource.WriteWarning(
                            ClassName,
                            "Failed to rename folder {0} to {1} without any error, and retrying",
                            srcFolderName,
                            dstFolderName);
                    }
                    else
                    {
                        break;
                    }
                }
                catch (Exception ex)
                {
                    traceSource.WriteError(
                        ClassName,
                        "Failed to rename folder {0} to {1} with error {2}, and detail exception {3}",
                        srcFolderName,
                        dstFolderName,
                        ex.Message,
                        ex);
                }
            }
        }

        private BlobRequestOptions GetRequestOptions(TimeoutHelper helper)
        {
            return helper.GetRemainingTime().TotalSeconds > DefaultTimeoutInSeconds
            ? new BlobRequestOptions()
            {
                RetryPolicy = new Microsoft.WindowsAzure.Storage.RetryPolicies.NoRetry(),
                MaximumExecutionTime = helper.GetRemainingTime()
            }
            : this.defaultRequestOption;
        }

        /// <summary>
        /// Post-processing of copying (from XStore to SMB)
        /// </summary>
        /// <param name="task">The task to end.</param>
        private static void EndCopyFromXStoreToSMB(XStoreFileOperationTask task)
        {
            if (task.IsSucceeded)
            {
                // Step 3, 4 and 5 for atomic copy
                // 3. delete dstFolder.old
                // 4. rename dstFolder -> dstFolder.old
                // 5. rename dstFolder.new -> dstFolder

                // get the original destination path
                string newDstRootUri = task.DstUri + NewExtension;
                string oldDstRootUri = task.DstUri + OldExtension;

                // Step 3, 4 & 5
                if (FabricDirectory.Exists(oldDstRootUri))
                {
                    // delete dstFolder.old
                    FabricDirectory.Delete(oldDstRootUri, recursive: true, deleteReadOnlyFiles: true);
                }

                if (FabricDirectory.Exists(task.DstUri))
                {
                    // rename dstFolder -> dstFolder.old
                    RenameFolder(task.DstUri, oldDstRootUri);
                }

                // rename dstFolder.new -> dstFolder; and clean up
                RenameFolder(newDstRootUri, task.DstUri);
                if (FabricDirectory.Exists(oldDstRootUri))
                {
                    FabricDirectory.Delete(oldDstRootUri, recursive: true, deleteReadOnlyFiles: true);
                }
            }
        }

        /// <summary>
        /// Post-processing of copying (from SMB to XStore)
        /// </summary>
        /// <param name="blobContainer">The blob container from which copy is happening.</param>
        /// <param name="task">The task to end.</param>
        private static void EndCopyFromSMBToXStore(CloudBlobContainer blobContainer, XStoreFileOperationTask task)
        {
            if (task.IsSucceeded)
            {
                // Generate a manifest on the xstore for the copied folder - only need for atomic copy
                // When doing atomic copy we have following assumptions:
                // 1) only one writer
                // 2) we don't overwrite to the exisiting folder
                // 3) later this folder is copied to smb with the exact path, no sub folder or parent folder
                // therefore it is enough to only generate a manifest file on the folder only
                var blob = blobContainer.GetBlockBlobReference(XStoreCommon.GetXStoreFolderManifest(task.DstUri));
#if !DotNetCoreClr
                blob.UploadText(DateTime.Now.ToString());
#else
                blob.UploadTextAsync(DateTime.Now.ToString()).Wait();
#endif
            }
        }

        /// <summary>
        /// Post-processing of copying (from XStore to XStore)
        /// </summary>
        /// <param name="blobContainer">The blob container from which copy is happening.</param>
        /// <param name="task">The task to end.</param>
        private static void EndCopyFromXStoreToXStore(CloudBlobContainer blobContainer, XStoreFileOperationTask task)
        {
            if (task.IsSucceeded)
            {
                // Generate a manifest on the xstore for the copied folder - only need for atomic copy
                // When doing atomic copy we have following assumptions:
                // 1) only one writer
                // 2) we don't overwrite to the exisiting folder
                // 3) later this folder is copied to smb with the exact path, no sub folder or parent folder
                // therefore it is enough to only generate a manifest file on the folder only
                var blob = blobContainer.GetBlockBlobReference(XStoreCommon.GetXStoreFolderManifest(task.DstUri));
#if !DotNetCoreClr
                blob.UploadText(DateTime.Now.ToString());
#else
                blob.UploadTextAsync(DateTime.Now.ToString()).Wait();
#endif
            }
        }

        /// <summary>
        /// Retrieve files and folders under the current folder from SMB
        /// </summary>
        /// <param name="srcPath"> The SMB path </param>
        /// <param name="files">String array of file names in the folder</param>
        /// <param name="folders">String array of subfolder names in the folder</param>
        private static void GetFilesAndSubfoldersFromSMB(
            string srcPath,
            out string[] files,
            out string[] folders)
        {
            files = FabricDirectory.GetFiles(srcPath);
            folders = FabricDirectory.GetDirectories(srcPath);
        }

        /// <summary>
        /// Find the corresponding blob uri based on the smb path
        /// SMB path is \\XXX\YYY; XStore path is XXX/YYY
        /// </summary>
        /// <param name="smbPath">The smb path to be converted.</param>
        /// <param name="srcRootUri">The source root uri.</param>
        /// <param name="dstRootUri">The destination root uri.</param>
        /// <returns>The blob uri</returns>
        private static string ConvertSMBPathToBlobReference(string smbPath, string srcRootUri, string dstRootUri)
        {
            smbPath = XStoreCommon.FormatSMBUri(smbPath);
            if (smbPath.StartsWith(srcRootUri, StringComparison.OrdinalIgnoreCase))
            {
                smbPath = smbPath.Substring(srcRootUri.Length);
            }

            return dstRootUri + smbPath.Replace('\\', '/');
        }

        /// <summary>
        /// Find the corresponding smb path based on the blob uri
        /// SMB path is \\XXX\YYY; XStore path is XXX/YYY
        /// </summary>
        /// <returns>The SMB path corresponding to the XStore Path.</returns>
        /// <param name="blobReference">The reference to blob.</param>
        /// <param name="srcRootUri">The root uri of the source.</param>
        /// <param name="dstRootUri">The root uri of the destination.</param>
        private static string ConvertBlobReferenceToSMBPath(string blobReference, string srcRootUri, string dstRootUri)
        {
            char[] trims = { '/', ' ' };
            blobReference = XStoreCommon.FormatXStoreUri(blobReference);
            if (blobReference.StartsWith(srcRootUri, StringComparison.OrdinalIgnoreCase))
            {
                blobReference = blobReference.Substring(srcRootUri.Length).Trim(trims);
            }

            return Path.Combine(dstRootUri, blobReference.Replace('/', '\\'));
        }

        private static string ConvertBlobReferenceToBlobReference(string blobReference, string srcRootUri, string dstRootUri)
        {
            char[] trims = { '/', ' ' };
            blobReference = XStoreCommon.FormatXStoreUri(blobReference);
            if (blobReference.StartsWith(srcRootUri, StringComparison.OrdinalIgnoreCase))
            {
                blobReference = blobReference.Substring(srcRootUri.Length).Trim(trims);
            }

            return string.Format("{0}/{1}", dstRootUri, blobReference);
        }

        /// <summary>
        /// Returns whether the file should be filtered out or not
        /// </summary>
        /// <param name="source">
        /// Source file name
        /// </param>
        /// <returns>
        /// Returns true if should not be filtered
        /// </returns>
        private bool ShouldCopy(string source)
        {
            // FileInfo.Extension method is fine for both xstore uri and smb path
            string extension = Path.GetExtension(source);
            string fileName = Path.GetFileName(source);

            // Should copy if this._filterExtensions is null or the filterextensions does not contain the key or it is not the manifest file
            bool shouldCopy = (this.filterExtensions == null ||
                    (!this.filterExtensions.ContainsKey(extension))) &&
                   (0 != string.Compare(fileName, XStoreCommon.ManifestFilename, StringComparison.OrdinalIgnoreCase));

            if(shouldCopy)
            {
                shouldCopy = (this.filterFileNames == null ||
                    (!this.filterFileNames.Contains(fileName))) &&
                    (0 != string.Compare(fileName, XStoreCommon.ManifestFilename, StringComparison.OrdinalIgnoreCase));
            }

            return shouldCopy;
        }

        /// <summary>
        /// Upload a file from SMB to xstore
        /// </summary>
        /// <param name="blobContainer">
        /// Blob container
        /// </param>
        /// <param name="task">
        /// Task object
        /// </param>
        private void TransferFileFromSMBToXStore(CloudBlobContainer blobContainer, XStoreFileOperationTask task)
        {
            if (!this.ShouldCopy(task.SrcUri))
            {
                return;
            }

            Int64 size = FabricFile.GetSize(task.SrcUri);
            // Create the Blob and upload the file
            var blob = blobContainer.GetBlockBlobReference(task.DstUri);
            var blobWrapper = new XStoreBlobWrapper(blob);

            if (task.TimeoutHelper != null)
            {
                //task.TimeoutHelper.ThrowIfExpired();
                blobWrapper.UploadFromFile(task.SrcUri, (int)size, task.TimeoutHelper.GetRemainingTime());
            }
            else
            {
                blobWrapper.UploadFromFile(task.SrcUri, (int)size, TimeSpan.MaxValue);
            }
        }

        /// <summary>
        /// Copy a file from xstore to xstore
        /// </summary>
        /// <param name="blobContainer">
        /// Blob container
        /// </param>
        /// <param name="task">
        /// Task object
        /// </param>
        private void TransferFileFromXStoreToXStore(CloudBlobContainer blobContainer, XStoreFileOperationTask task)
        {
            if (!this.ShouldCopy(task.SrcUri))
            {
                return;
            }

            var destinationBlob = blobContainer.GetBlockBlobReference(task.DstUri);
            var sourceBlob = blobContainer.GetBlockBlobReference(task.SrcUri);
#if !DotNetCoreClr
            sourceBlob.FetchAttributes();
#else
            sourceBlob.FetchAttributesAsync().Wait();
#endif
            int blobLength = (int)sourceBlob.Properties.Length;
            var blobWrapper = new XStoreBlobWrapper(destinationBlob);
            blobWrapper.CopyFromXStore(sourceBlob, blobLength, task.TimeoutHelper != null ? task.TimeoutHelper.GetRemainingTime() : TimeSpan.MaxValue);
        }

        /// <summary>
        /// Download file from XStore to SMB
        /// </summary>
        /// <param name="blobContainer">The blob container having the file.</param>
        /// <param name="task">The task to be performed.</param>
        private void TransferFileFromXStoreToSMB(CloudBlobContainer blobContainer, XStoreFileOperationTask task)
        {
            if (!this.ShouldCopy(task.SrcUri))
            {
                return;
            }

            // download the file
            if (task.PartialID >= 0)
            {
                // This is already a divided work; let's go ahead and download
                string dstSMBPath = XStoreCommon.GetPartialFileName(task.DstUri, task.PartialID);

                // Delete the file if it exists
                string directoryName = FabricPath.GetDirectoryName(dstSMBPath);
                if (!FabricDirectory.Exists(directoryName))
                {
                    FabricDirectory.CreateDirectory(directoryName);
                }

                if (FabricFile.Exists(dstSMBPath))
                {
                    FabricFile.Delete(dstSMBPath, deleteReadonly: true);
                }

                var blob = blobContainer.GetBlockBlobReference(task.SrcUri);
                var blobWrapper = new XStoreBlobWrapper(blob);
                if (task.TimeoutHelper != null)
                {
                   //task.TimeoutHelper.ThrowIfExpired();
                    blobWrapper.DownloadPartToFile(dstSMBPath, task.Offset, task.Length, task.TimeoutHelper.GetRemainingTime());
                }
                else
                {
                    blobWrapper.DownloadPartToFile(dstSMBPath, task.Offset, task.Length, TimeSpan.MaxValue);
                }
            }
            else
            {
                // we are going to download a file, which we don't know the size yet
                var blob = blobContainer.GetBlockBlobReference(task.SrcUri);
#if !DotNetCoreClr
                blob.FetchAttributes(null, this.defaultRequestOption);
#else
                blob.FetchAttributesAsync(null, this.defaultRequestOption, null).Wait();
#endif
                int blobLength = (int)blob.Properties.Length;

                // Delete the file if it exists
                string directoryName = FabricPath.GetDirectoryName(task.DstUri);
                if (!FabricDirectory.Exists(directoryName))
                {
                    FabricDirectory.CreateDirectory(directoryName);
                }

                if (FabricFile.Exists(task.DstUri))
                {
                    FabricFile.Delete(task.DstUri, deleteReadonly: true);
                }

                if (blobLength < DownloadSizeThreshold)
                {
                    var blobWrapper = new XStoreBlobWrapper(blob);
                    if (task.TimeoutHelper != null)
                    {
                        blobWrapper.DownloadToFile(task.DstUri, blobLength, task.TimeoutHelper.GetRemainingTime(), task.OperationContext);
                    }
                    else
                    {
                        blobWrapper.DownloadToFile(task.DstUri, blobLength, TimeSpan.MaxValue, task.OperationContext);
                    }

                    return;
                }
                else
                {
                    // For large files we divided the work to couple threads.
                    int numThreads = Math.Min(
                        blobLength / ParrallelDownloadSize,
                        ParrallelDownloadThreadCount);

                    // Create new tasks to parallel download
                    Queue<XStoreFileOperationTask> tasks = new Queue<XStoreFileOperationTask>();
                    int offset = 0;
                    for (int i = 0; i < numThreads; i++)
                    {
                        int length;
                        if (i < numThreads - 1)
                        {
                            length = blobLength / numThreads;
                        }
                        else
                        {
                            length = blobLength - offset;
                        }

                        XStoreFileOperationTask newTask = new XStoreFileOperationTask(
                            XStoreFileOperationTask.XStoreTaskType.CopyFromXStoreToSMB,
                            task.SrcUri,
                            task.DstUri,
                            false,
                            0,
                            task.TimeoutHelper);

                        newTask.FileCopyFlag = task.FileCopyFlag;
                        newTask.Offset = offset;
                        newTask.Length = length;
                        newTask.PartialID = i;

                        tasks.Enqueue(newTask);
                        offset += length;
                    }

                    // enqueue all divided tasks
                    this.xstoreTaskPool.AddTaskToTaskQueue(tasks);

                    // Add an EndTask to the endTaskQueue as well; to combine all downloads
                    XStoreFileOperationTask endTask = new XStoreFileOperationTask(
                        XStoreFileOperationTask.XStoreTaskType.CombinePartialFiles,
                        task.SrcUri,
                        task.DstUri,
                        false,
                        0,
                        task.TimeoutHelper);

                    endTask.IsSucceeded = true;
                    endTask.FileCopyFlag = task.FileCopyFlag;
                    endTask.PartialID = numThreads;
                    endTask.Content = task.DstUri;
                    this.xstoreTaskPool.AddTaskToEndTaskQueue(endTask);
                }
            }
        }

        /// <summary>
        /// Retry the task
        /// </summary>
        /// <param name="task">The task to be performed.</param>
        private void RetryTask(XStoreFileOperationTask task)
        {
            // retrycount + 1 and enqueue again
            if (task.RetryCount + 1 > MaxRetryAttempts)
            {
                // Enqueue to endqueue
                task.IsSucceeded = false;
                this.xstoreTaskPool.AddTaskToEndTaskQueue(task);
            }
            else
            {
                task.RetryCount = task.RetryCount + 1;
                this.xstoreTaskPool.AddTaskToTaskQueue(task);
            }
        }

        /// <summary>
        /// Processing folder
        /// </summary>
        /// <param name="blobContainer">The blob container.</param>
        /// <param name="task">The task to be performed.</param>
        private void HandleFolder(CloudBlobContainer blobContainer, XStoreFileOperationTask task)
        {
            // expand the task and enqueue new tasks
            string[] files;
            string[] subFolders;

            // See if we need to handle the folder
            if (task.OpType == XStoreFileOperationTask.XStoreTaskType.CopyFromXStoreToSMB)
            {
                if (task.FileCopyFlag == CopyFlag.AtomicCopySkipIfExists && FabricDirectory.Exists(task.DstUri))
                {
                    // We will skip the copy because the final destination exists
                    return;
                }
                else if ((task.FileCopyFlag == CopyFlag.AtomicCopy ||
                          task.FileCopyFlag == CopyFlag.AtomicCopySkipIfExists) &&
                         task.IsRoot)
                {
                    XStoreFileOperationTask endTask = new XStoreFileOperationTask(
                        XStoreFileOperationTask.XStoreTaskType.EndCopyFromXStoreToSMB,
                        task.SrcUri,
                        task.DstUri,
                        true,
                        0,
                        task.TimeoutHelper);

                    endTask.IsSucceeded = true;
                    endTask.FileCopyFlag = task.FileCopyFlag;
                    this.xstoreTaskPool.AddTaskToEndTaskQueue(endTask);
                    task.DstUri = task.DstUri + NewExtension;

                    // step1: delete dstFolder.new
                    if (FabricDirectory.Exists(task.DstUri))
                    {
                        FabricDirectory.Delete(task.DstUri, recursive: true, deleteReadOnlyFiles: true);
                    }
                }
            }
            else if (task.OpType == XStoreFileOperationTask.XStoreTaskType.CopyFromSMBToXStore)
            {
                if (task.FileCopyFlag == CopyFlag.AtomicCopySkipIfExists &&
                    XStoreCommon.XStoreFolderExists(blobContainer, task.DstUri, false, task.TimeoutHelper != null ? GetRequestOptions(task.TimeoutHelper) : null))
                {
                    // We will skip the copy because the final destination exists
                    return;
                }
                else if (task.IsRoot)
                {
                    // if this is the root uri of this operation
                    XStoreFileOperationTask endTask = new XStoreFileOperationTask(
                        XStoreFileOperationTask.XStoreTaskType.EndCopyFromSMBToXStore,
                        task.SrcUri,
                        task.DstUri,
                        true,
                        0,
                        task.TimeoutHelper);

                    endTask.IsSucceeded = true;
                    endTask.FileCopyFlag = task.FileCopyFlag;
                    this.xstoreTaskPool.AddTaskToEndTaskQueue(endTask);

                    // remove the manifest of this folder
                    var blob = blobContainer.GetBlockBlobReference(XStoreCommon.GetXStoreFolderManifest(task.DstUri));
#if !DotNetCoreClr
                    blob.DeleteIfExists(DeleteSnapshotsOption.None, null, task.TimeoutHelper != null ? GetRequestOptions(task.TimeoutHelper) : null);
#else
                    blob.DeleteIfExistsAsync(DeleteSnapshotsOption.None, null, task.TimeoutHelper != null ? GetRequestOptions(task.TimeoutHelper) : null, null).Wait();
#endif
                }
            }
            else if (task.OpType == XStoreFileOperationTask.XStoreTaskType.CopyFromXStoreToXStore)
            {
                if (task.FileCopyFlag == CopyFlag.AtomicCopySkipIfExists &&
                    XStoreCommon.XStoreFolderExists(blobContainer, task.DstUri, false, task.TimeoutHelper != null ? GetRequestOptions(task.TimeoutHelper) : null))
                {
                    // We will skip the copy because the final destination exists
                    return;
                }
                else if (task.IsRoot)
                {
                    // if this is the root uri of this operation
                    XStoreFileOperationTask endTask = new XStoreFileOperationTask(
                        XStoreFileOperationTask.XStoreTaskType.EndCopyFromXStoreToXStore,
                        task.SrcUri,
                        task.DstUri,
                        true,
                        0,
                        task.TimeoutHelper);

                    endTask.IsSucceeded = true;
                    endTask.FileCopyFlag = task.FileCopyFlag;

                    this.xstoreTaskPool.AddTaskToEndTaskQueue(endTask);

                    // remove the manifest of this folder
                    var blob = blobContainer.GetBlockBlobReference(XStoreCommon.GetXStoreFolderManifest(task.DstUri));

#if !DotNetCoreClr
                    blob.DeleteIfExists(DeleteSnapshotsOption.None, null, task.TimeoutHelper == null ? null : GetRequestOptions(task.TimeoutHelper));
#else
                    blob.DeleteIfExistsAsync(DeleteSnapshotsOption.None, null, task.TimeoutHelper == null ? null : GetRequestOptions(task.TimeoutHelper), null).Wait();
#endif
                }
            }

            string dstUri = null;

            switch (task.OpType)
            {
                case XStoreFileOperationTask.XStoreTaskType.CopyFromXStoreToSMB:
                case XStoreFileOperationTask.XStoreTaskType.CopyFromXStoreToXStore:
                case XStoreFileOperationTask.XStoreTaskType.RemoveFromXStore:
                    this.GetFilesAndSubfoldersFromXStore(blobContainer, task.SrcUri, out files, out subFolders, task.TimeoutHelper);
                    break;
                case XStoreFileOperationTask.XStoreTaskType.CopyFromSMBToXStore:
                default:
                    GetFilesAndSubfoldersFromSMB(task.SrcUri, out files, out subFolders);
                    break;
            }

            Queue<XStoreFileOperationTask> tasks = new Queue<XStoreFileOperationTask>();
            foreach (var file in files)
            {
                switch (task.OpType)
                {
                    case XStoreFileOperationTask.XStoreTaskType.CopyFromXStoreToSMB:
                        dstUri = ConvertBlobReferenceToSMBPath(file, task.SrcUri, task.DstUri);
                        if (task.OperationId != null)
                        {
                            XStoreCommon.AddDownloadContentEntry(task.OperationId.GetValueOrDefault(), file, dstUri);
                        }
                        break;
                    case XStoreFileOperationTask.XStoreTaskType.CopyFromSMBToXStore:
                        dstUri = ConvertSMBPathToBlobReference(file, task.SrcUri, task.DstUri);
                        break;
                    case XStoreFileOperationTask.XStoreTaskType.CopyFromXStoreToXStore:
                        dstUri = ConvertBlobReferenceToBlobReference(file, task.SrcUri, task.DstUri);
                        break;
                }

                XStoreFileOperationTask newTask = new XStoreFileOperationTask(task.OpType, file, dstUri, false, 0, task.TimeoutHelper, task.OperationId);
                newTask.FileCopyFlag = task.FileCopyFlag;
                tasks.Enqueue(newTask);
            }

            foreach (var folder in subFolders)
            {
                switch (task.OpType)
                {
                    case XStoreFileOperationTask.XStoreTaskType.CopyFromXStoreToSMB:
                        dstUri = ConvertBlobReferenceToSMBPath(folder, task.SrcUri, task.DstUri);
                        break;
                    case XStoreFileOperationTask.XStoreTaskType.CopyFromSMBToXStore:
                        dstUri = ConvertSMBPathToBlobReference(folder, task.SrcUri, task.DstUri);
                        break;
                    case XStoreFileOperationTask.XStoreTaskType.CopyFromXStoreToXStore:
                        dstUri = ConvertBlobReferenceToBlobReference(folder, task.SrcUri, task.DstUri);
                        break;
                }

                XStoreFileOperationTask newTask = new XStoreFileOperationTask(task.OpType, folder, dstUri, true, 0, task.TimeoutHelper);
                newTask.FileCopyFlag = task.FileCopyFlag;
                tasks.Enqueue(newTask);
            }

            this.xstoreTaskPool.AddTaskToTaskQueue(tasks);
        }

        /// <summary>
        /// Wrapper for thread function; capture exceptions that
        /// are not likely to be recovered; and reprot failure
        /// </summary>
        private void ThreadRun()
        {
            try
            {
                this.ThreadRunImpl();
            }
            catch (Exception e)
            {
                traceSource.WriteError(
                    ClassName,
                    string.Format(CultureInfo.InvariantCulture, "[THREAD Exception] non recoverable exception happened in worker thread. e = {0}", e.Message));

                if (e is TimeoutException)
                {
                    this.hasTimeoutError = true;
                }
                else
                {
                    this.hasFatalError = true;
                }
            }
        }

        /// <summary>
        /// Worker thread to perform the file operations
        /// </summary>
        private void ThreadRunImpl()
        {
            // Initialize xstore blobcontainer and connection in the thread
            // This is to prevent we block the dispatcher

            // Connect to azure blob storage
            string connectionString = this.storeParams.ConnectionString;

            var storageAccount = CloudStorageAccount.Parse(connectionString);
            CloudBlobClient blobClient = storageAccount.CreateCloudBlobClient();

            blobClient.DefaultRequestOptions = new BlobRequestOptions();
            blobClient.DefaultRequestOptions.MaximumExecutionTime = new TimeSpan(0, 5, 0);

            // For large file copies set up a custom timeout period; and using parallel settings
            blobClient.DefaultRequestOptions.ParallelOperationThreadCount = XStoreCommon.ParallelOperationThreadCount;

            // Get and create the container
            CloudBlobContainer blobContainer = blobClient.GetContainerReference(this.storeParams.Container);

            while (WaitHandle.WaitAny(this.syncEvents.EventArray) != 1)
            {
                XStoreFileOperationTask task = this.xstoreTaskPool.GetTaskFromTaskQueue();

                if (null != task)
                {
                    // We have got valid task
                    if (task.RetryCount > 0)
                    {
                        // Sleep random time to prevent all worker threads do the next try at the same time
                        Random rand = new Random(DateTime.Now.Millisecond + this.workerID);
                        int timeoutInSeconds = rand.Next(MinRetryInSeconds, MaxRetryInSeconds);
                        traceSource.WriteError(
                            ClassName,
                            string.Format(CultureInfo.InvariantCulture, "[RETRY] sleep {0} seconds to work on {1}", timeoutInSeconds, task.SrcUri));

                        Thread.Sleep(timeoutInSeconds * 1000);
                    }

                    if (task.IsFolder &&
                        task.OpType != XStoreFileOperationTask.XStoreTaskType.EndCopyFromXStoreToSMB &&
                        task.OpType != XStoreFileOperationTask.XStoreTaskType.EndCopyFromSMBToXStore &&
                        task.OpType != XStoreFileOperationTask.XStoreTaskType.EndCopyFromXStoreToXStore)
                    {
                        try
                        {
                            this.HandleFolder(blobContainer, task);
                        }
                        catch (Exception e)
                        {
                            traceSource.WriteError(
                                ClassName,
                                string.Format(CultureInfo.InvariantCulture, "[{0}] [{1}] EXCEPTION = {2}", Thread.CurrentThread.ManagedThreadId, DateTime.Now, e.Message));

                            if (e is TimeoutException)
                            {
                                throw;
                            }

                            if (e is StorageException && e.InnerException is TimeoutException)
                            {
                                throw new TimeoutException(e.InnerException.Message);
                            }

                            if (ExceptionHandler.IsFatalException(e))
                            {
                                throw;
                            }

                            this.RetryTask(task);
                        }
                        finally
                        {
                            this.xstoreTaskPool.DecrementTaskCount();
                        }
                    }
                    else
                    {
                        try
                        {
                            switch (task.OpType)
                            {
                                case XStoreFileOperationTask.XStoreTaskType.CopyFromXStoreToSMB:
                                    this.TransferFileFromXStoreToSMB(blobContainer, task);
                                    break;
                                case XStoreFileOperationTask.XStoreTaskType.CopyFromSMBToXStore:
                                    this.TransferFileFromSMBToXStore(blobContainer, task);
                                    break;
                                case XStoreFileOperationTask.XStoreTaskType.CopyFromXStoreToXStore:
                                    this.TransferFileFromXStoreToXStore(blobContainer, task);
                                    break;
                                case XStoreFileOperationTask.XStoreTaskType.RemoveFromXStore:
                                    DeleteFileFromXStore(blobContainer, task);
                                    break;
                                case XStoreFileOperationTask.XStoreTaskType.EndCopyFromSMBToXStore:
                                    EndCopyFromSMBToXStore(blobContainer, task);
                                    break;
                                case XStoreFileOperationTask.XStoreTaskType.EndCopyFromXStoreToSMB:
                                    EndCopyFromXStoreToSMB(task);
                                    break;
                                case XStoreFileOperationTask.XStoreTaskType.EndCopyFromXStoreToXStore:
                                    EndCopyFromXStoreToXStore(blobContainer, task);
                                    break;
                                default:
                                    break;
                            }
                        }
                        catch (Exception e)
                        {
                            traceSource.WriteError(
                                ClassName,
                                string.Format(CultureInfo.InvariantCulture, "[{0}] [{1}] EXCEPTION = {2}", Thread.CurrentThread.ManagedThreadId, DateTime.Now, e.Message));

                            string extraTracing = task.GetExtraTracing();
                            if (!string.IsNullOrEmpty(extraTracing))
                            {
                                traceSource.WriteInfo(ClassName, extraTracing);
                            }

                            if (e is TimeoutException)
                            {
                                throw;
                            }

                            if (e is StorageException && e.InnerException is TimeoutException)
                            {
                                throw new TimeoutException(e.InnerException.Message);
                            }

                            if (ExceptionHandler.IsFatalException(e))
                            {
                                throw;
                            }

                            this.RetryTask(task);
                        }
                        finally
                        {
                            this.xstoreTaskPool.DecrementTaskCount();
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Format the uri path to make it represent a blob reference -
        /// it shouldn't have the container name in it
        /// </summary>
        /// <returns>The blob reference.</returns>
        /// <param name="uriPath">The uri of the blob.</param>
        private string ConvertUriToBlobReference(string uriPath)
        {
            // Need to remove "/ContainerName/" from the uri
            char[] trims = { '/', ' ' };
            string blobReference = uriPath.Trim(trims);
            if (blobReference.StartsWith("devstoreaccount1", StringComparison.OrdinalIgnoreCase))
            {
                blobReference = blobReference.Substring("devstoreaccount1".Length).Trim(trims);
            }

            if (blobReference.StartsWith(this.storeParams.Container, StringComparison.OrdinalIgnoreCase))
            {
                return blobReference.Substring(this.storeParams.Container.Length).Trim(trims);
            }

            return blobReference;
        }

        /// <summary>
        /// Retrieve files and folders under the current folder from XStore
        /// </summary>
        /// <param name="blobContainer">Blob container</param>
        /// <param name="srcPath">XStore path</param>
        /// <param name="files">String array of file names in the folder</param>
        /// <param name="folders">String array of subfolder names in the folder</param>
        /// <param name="helper">The timeout helper object.</param>
        private void GetFilesAndSubfoldersFromXStore(
            CloudBlobContainer blobContainer,
            string srcPath,
            out string[] files,
            out string[] folders,
            TimeoutHelper helper)
        {
            List<string> fileList = new List<string>();
            List<string> folderList = new List<string>();

            CloudBlobDirectory cloudBlobDirectory = blobContainer.GetDirectoryReference(this.ConvertUriToBlobReference(srcPath));

            // Azure storage team has a bug that the same BlobDirectory can be duplicated in return
            // to work around this issue we always check if the same directory has been added
            // [TO DO] this is to be removed once azure storage team fixes this issue
            HashSet<string> blobDirectories = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

#if !DotNetCoreClr
            IEnumerable<IListBlobItem> blobItems = helper != null
                ? cloudBlobDirectory.ListBlobs(false, BlobListingDetails.None, GetRequestOptions(helper))
                : cloudBlobDirectory.ListBlobs();
#else
            BlobContinuationToken continuationToken = null;
            do
            {
                BlobResultSegment resultSegment = helper != null
                    ? cloudBlobDirectory.ListBlobsSegmentedAsync(false, BlobListingDetails.None, null, continuationToken, GetRequestOptions(helper), null).Result
                    : cloudBlobDirectory.ListBlobsSegmentedAsync(continuationToken).Result;
                IEnumerable<IListBlobItem> blobItems = resultSegment.Results;
                continuationToken = resultSegment.ContinuationToken;
#endif
            foreach (IListBlobItem blobItem in blobItems)
                {
                    Uri uri = blobItem.Uri;
                    if (blobItem is CloudBlobDirectory)
                    {
                        if (!blobDirectories.Contains(uri.LocalPath.Trim()))
                        {
                            blobDirectories.Add(uri.LocalPath.Trim());
                            folderList.Add(this.ConvertUriToBlobReference(uri.LocalPath));
                        }
                    }
                    else
                    {
                        fileList.Add(this.ConvertUriToBlobReference(uri.LocalPath));
                    }
                }
#if DotNetCoreClr
            } while (continuationToken != null);
#endif
            files = fileList.ToArray();
            folders = folderList.ToArray();
        }
    }
}
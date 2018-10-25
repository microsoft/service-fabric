// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageStore
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Security.Cryptography;
    using System.Threading;

    /// <summary>
    /// Class that implements a multi-threaded folder copy using threadpool
    /// TODO: implement copy flags:
    ///             AbortOnFirstError
    ///             OverwriteExisting
    ///             OverwriteOlderFiles
    ///             MirrorSource
    /// </summary>
    internal class FolderCopy : IDisposable
    {
        /// <summary>
        /// Extension used while making mirror(atomic) copy.  
        /// Files are copied with extension .new, Old ones are renamed to .old
        /// and .new is then renamed back to the appropriate file name
        /// </summary>
        private const string NewExtension = ".new";

        /// <summary>
        /// Extension used while making mirror(atomic) copy.  
        /// Files are copied with extension .new, Old ones are renamed to .old
        /// and .new is then renamed back to the appropriate file name
        /// </summary>
        private const string OldExtension = ".old";

        /// <summary>
        /// Component name used for logging trace records
        /// </summary>
        private const string ClassName = "FolderCopy";

        /// <summary>
        /// Number of attempts to try if there is a failure.
        /// </summary>
        private const int MaxRetryAttempts = 10;

        /// <summary>
        /// Generates random sleep times between retries (e.g. if writing to a file failed)
        /// </summary>
        private static readonly Random Randomizer = new Random();

        /// <summary>
        /// Copy flag options
        /// </summary>
        private readonly CopyFlag copyFlag;

        /// <summary>
        /// File extensions to filter
        /// </summary>
        private readonly Dictionary<string, bool> filterExtensions;

        /// <summary>
        /// Keeps track of the number of items enqueued.  copy is done when this reverts to 0.
        /// </summary>
        private int workItemsInProgress = 0;

        /// <summary>
        /// Event that is fired to let external users know when copy is complete.
        /// </summary>
        private ManualResetEvent asyncEvent = new ManualResetEvent(false);

        /// <summary>
        /// This is used to trigger internal async copy is complete, we need this event to do post clean up
        /// task after last work item is done.
        /// </summary>
        private ManualResetEvent internalCopyAsyncEvent = new ManualResetEvent(false);

        /// <summary>
        /// Keeps track of the number of items that weren't copied
        /// </summary>
        private int failedItems = 0;

        /// <summary>
        /// Whether we skipped due to destination already existing
        /// </summary>
        private bool skipCopy;

        /// <summary>
        /// Logger for logging folder copy statements.
        /// </summary>
        private FabricEvents.ExtensionsEvents traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.ImageStoreClient);

        /// <summary>
        /// To handled object disposal correctly
        /// </summary>
        private bool disposed;

        /// <summary>
        /// Initializes a new instance of the FolderCopy class. 
        /// </summary>
        public FolderCopy()
            : this(CopyFlag.CopyIfDifferent, null)
        {
        }

        /// <summary>
        /// Initializes a new instance of the FolderCopy class. Constructor that takes Copy flags and file extensions to be filterd out
        ///  while copying
        /// </summary>
        /// <param name="flag">Flag specifying what type of copying needs to be done.</param>
        /// <param name="filterExtensions">The file extensions that need to be filtered.</param>
        public FolderCopy(CopyFlag flag, IEnumerable<string> filterExtensions)
        {
            if (filterExtensions != null)
            {
                foreach (string e in filterExtensions)
                {
                    string extension = e;
                    if (this.filterExtensions == null)
                    {
                        this.filterExtensions = new Dictionary<string, bool>(StringComparer.OrdinalIgnoreCase);
                    }

                    // normalize; Path.GetExtension returns . + extension like ".pdb"
                    if (extension.Length > 0 && extension[0] != '.')
                    {
                        extension = '.' + extension;
                    }

                    if (!this.filterExtensions.ContainsKey(extension))
                    {
                        this.filterExtensions[extension] = true;
                    }
                }
            }

            this.copyFlag = flag;
        }

        /// <summary>
        /// Gets the number of items that were not copied.
        /// </summary>
        public int FailedToCopy
        {
            get 
            { 
                this.ThrowIfDisposed();  
                return this.failedItems; 
            }
        }

        /// <summary>
        /// Gets the async wait handle.
        /// </summary>
        public WaitHandle AsyncWaitHandle
        {
            get 
            { 
                this.ThrowIfDisposed(); 
                return this.asyncEvent; 
            }
        }

        /// <summary>
        /// Renames/Moves tempSourceFilePath to destinationFilePath.
        /// Function retries if there is another process/thread currently
        /// reading from destinationFilePath.
        /// <para></para>
        /// Throws if after several retries the move wasn't successful.
        /// For effectivity purposes, the tempSourceFilePath and destinationFilePath should
        /// be located in the same directory / at least in the same volume.
        /// <para></para>
        /// The use case for using this function is to quickly move the file with retry 
        /// so other readers don't have to wait/retry for a long time.
        /// e.g. File.Replace() or File.Copy() would take a long time if the file is large.
        /// <para></para>
        /// If the process/thread is stopped externally during execution of this function,
        /// one of the results can be that a temporary file (original destinationFilePath renamed) can stay
        /// orphaned on the disk and the file in destinationFilePath does not exist.
        /// Next time the process is started programmer should expect 
        /// the destinationFilePath may exist but needn't.
        /// </summary>
        /// <param name="sourceFilePath">
        /// File has to exist.
        /// It is supposed that the file is temporary, so nobody is using it.
        /// (so no IO conflicts/race condition is applied here)
        /// Path cannot be null or empty.
        /// </param>
        /// <param name="destinationFilePath">
        /// File can exist but doesn't have to.
        /// File can be access by other threads/processes while moving - retry policy is applied.
        /// Path cannot be null or empty
        /// </param>
        public static void MoveFileWithRetry(string sourceFilePath, string destinationFilePath)
        {
            // Steps of atomic file switch
            // 1. Remove old temp file (if exists)
            // 2. Rename destination file to temp file (if destination file exist)
            // 3. Rename/Move sourceFile to destinationFile
            // 4. Delete old temp file
            string tempFileName = destinationFilePath + ".oldTemp" + DateTime.UtcNow.Ticks.ToString(CultureInfo.InvariantCulture);

            int maxRetryCount = 4;
            for (int i = 0; i <= maxRetryCount; i++)
            {
                try
                {
                    // Step 1. Remove old temp file (if exists)
                    // There shouldn't be any other process/thread using this file
                    // Remark: If 1st attempt failed after step 2 the tempFileName
                    // now contains the original destinationFilePath (if existed).
                    // We are deleting the tempFileName now.
                    if (FabricFile.Exists(tempFileName))
                    {
                        FabricFile.Delete(tempFileName, deleteReadonly: true);
                    }

                    // Step 2. Rename destination file to temp file
                    // Retry because there can be other threads/processes
                    // reading/writing/moving to that file - race condition.
                    if (FabricFile.Exists(destinationFilePath))
                    {
                        FabricFile.Move(destinationFilePath, tempFileName);
                    }

                    // REMARK: If the process crashes/stops here
                    // before completing the step 3
                    // the tempFileName will stay orphaned 
                    // and destinationFilePath will not exist.

                    // Step 3. Rename sourceFile to destinationFile
                    // This can fail if another process/thread meanwhile created
                    // the destinationFilePath after it was deleted in this thread in step 2.
                    FabricFile.Move(sourceFilePath, destinationFilePath);
                    break;
                }
                catch (Exception e)
                {
                    if ((i >= maxRetryCount) || !ExceptionHandler.IsIOException(e))
                    {
                        // Step 4. Delete old temp file
                        TryDeleteFile(tempFileName);
                        throw;
                    }

                    int timeToSleep = 40;
                    lock (FolderCopy.Randomizer)
                    {
                        timeToSleep = FolderCopy.Randomizer.Next(40, 70);
                    }

                    Thread.Sleep(timeToSleep);
                }
            }

            // REMARK: If the process crashes/stops here
            // before completing the step 4
            // the tempFileName will stay orphaned.

            // Step 4. Delete old temp file
            TryDeleteFile(tempFileName);
        }

        /// <summary>
        /// Copy method that blocks till the operartion is complete
        /// </summary>
        /// <param name="source">
        /// Source folder that needs to be copied
        /// </param>
        /// <param name="destination">
        /// Destination folder
        /// </param>
        public void Copy(string source, string destination)
        {
            this.ThrowIfDisposed();
            
            this.BeginCopy(source, destination);
            this.AsyncWaitHandle.WaitOne();
            if (this.FailedToCopy > 0)
            {
                string errorMessage = string.Format(
                    CultureInfo.InvariantCulture, StringResources.ImageStoreError_FailedToCopy, this.FailedToCopy, source, destination);
                this.traceSource.WriteError(ClassName, errorMessage);
                if (this.copyFlag == CopyFlag.AtomicCopy || this.copyFlag == CopyFlag.AtomicCopySkipIfExists)
                {
                    throw new InvalidDataException(errorMessage);
                }
            }
        }

        /// <summary>
        /// Async copy method that begins the copy operation.  Wait on AsyncWaitHandle to be notified of completion.
        /// </summary>
        /// <param name="source">
        /// Source folder that needs to be copied
        /// </param>
        /// <param name="destination">
        /// Destination folder
        /// </param>
        public void BeginCopy(string source, string destination)
        {
            this.ThrowIfDisposed();
            
            // make sure that the source folder exists
            if (!FabricDirectory.Exists(source))
            {
                throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, StringResources.ImageStoreError_DoesNotExistError, source));
            }

            // make sure that the event indicating completion is non-signalled
            this.asyncEvent.Reset();
            this.internalCopyAsyncEvent.Reset();

            // To make folder copy atomic, the sequence of actions are as below.
            // 1. delete dstFolder.new
            // 2. copy srcFolder -> dstFolder.new
            // 3. delete dstFolder.old
            // 4. rename dstFolder -> dstFolder.old
            // 5. rename dstFolder.new -> dstFolder
            string newDstFolder = destination;
            if (this.copyFlag == CopyFlag.AtomicCopySkipIfExists && FabricDirectory.Exists(newDstFolder))
            {
                // We will skip the copy because the final destination exists
                this.skipCopy = true;
            }
            else if (this.copyFlag == CopyFlag.AtomicCopy || this.copyFlag == CopyFlag.AtomicCopySkipIfExists)
            {
                newDstFolder = destination + FolderCopy.NewExtension;

                // Step 1.
                if (FabricDirectory.Exists(newDstFolder))
                {
                    FabricDirectory.Delete(newDstFolder, recursive: true, deleteReadOnlyFiles: true);
                }
            }

            // Queue up post copy completion task that needs to perform steps 3,4 & 5 mentioned above.
            ThreadPool.QueueUserWorkItem(this.EndCopy, new CopyArgs(source, destination, true));

            // set the work items to 1 and enqueue the first item to be copied
            this.workItemsInProgress = 1;

            if (!this.skipCopy)
            {
                ThreadPool.QueueUserWorkItem(this.CopyItem, new CopyArgs(source, newDstFolder, true));
            }
            else
            {
                // If we are skipping then just decrement and signal immediately. 
                // We still Queue up the EndCopy because it translates the inner event into the outer event.
                this.DecrementWorkItem();
            }
        }

        /// <summary>
        /// Compare two files and return true if they are different.
        /// </summary>
        /// <param name="firstFilename">The name of the first file to compare.</param>
        /// <param name="secondFilename">The name of the second file to compare.</param>
        /// <returns>True if the files are different, false otherwise </returns>
        public bool AreFilesDifferent(string firstFilename, string secondFilename)
        {
            this.ThrowIfDisposed();
            
            if (this.copyFlag == CopyFlag.AtomicCopy || this.copyFlag == CopyFlag.AtomicCopySkipIfExists)
            {
                return true;
            }
            else
            {
                return (FabricFile.Exists(firstFilename) != FabricFile.Exists(secondFilename) ||
                    FabricFile.GetSize(firstFilename) != FabricFile.GetSize(secondFilename) ||
                    FabricFile.GetLastWriteTime(firstFilename) != FabricFile.GetLastWriteTime(firstFilename));
            }
        }

        /// <summary>
        /// Dispose method
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Private Dispose method called by the public Dispose method
        /// </summary>
        /// <param name="isDisposing">Whether the object is disposing.</param>
        protected virtual void Dispose(bool isDisposing)
        {
            if (!this.disposed && isDisposing)
            {
                if (this.internalCopyAsyncEvent != null)
                {
                    this.internalCopyAsyncEvent.Dispose();
                    this.internalCopyAsyncEvent = null;
                }

                if (this.asyncEvent != null)
                {
                    this.asyncEvent.Dispose();
                    this.asyncEvent = null;
                }

                this.disposed = true;
            }

            // Clean native resources.
        }

        /// <summary>
        /// Try to delete the file.
        /// </summary>
        /// <param name="filePath">The complete path of the file to be deleted.</param>
        private static void TryDeleteFile(string filePath)
        {
            try
            {
                if (FabricFile.Exists(filePath))
                {
                    FabricFile.Delete(filePath, deleteReadonly: true);
                }
            }
            catch (Exception e)
            {
                if (!ExceptionHandler.IsIOException(e))
                {
                    throw;
                }
            }
        }

        private void ThrowIfDisposed()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException("FolderCopy");
            }
        }

        /// <summary>
        /// End copy delegate that is used to perform post copy completion task.
        /// </summary>
        /// <param name="state">
        /// Instance of CopyArgs that contains the source and destination folders
        /// </param>
        private void EndCopy(object state)
        {
            this.internalCopyAsyncEvent.WaitOne();

            CopyArgs args = state as CopyArgs;
            string oldDstFolderOriginal = args.Destination + FolderCopy.OldExtension;
            string oldDstFolderNew = args.Destination + FolderCopy.OldExtension;

            string newDstFolderNew = args.Destination + FolderCopy.NewExtension;

            string dstFolderOld = args.Destination;
            string dstFolderNew = args.Destination;

            try
            {
                if (!this.skipCopy && (this.copyFlag == CopyFlag.AtomicCopy || this.copyFlag == CopyFlag.AtomicCopySkipIfExists) &&
                    this.failedItems == 0 /* rename only if copying all items is successfull */)
                {
                    for (int i = 1; i <= MaxRetryAttempts; i++)
                    {
                        try
                        {
                            // Step 3, 4 & 5
                            if (oldDstFolderOriginal != null && FabricDirectory.Exists(oldDstFolderOriginal))
                            {
                                FabricDirectory.Delete(oldDstFolderOriginal, recursive: true, deleteReadOnlyFiles: true);
                                oldDstFolderOriginal = null;
                            }

                            if (dstFolderOld != null && FabricDirectory.Exists(dstFolderOld))
                            {
                                this.RenameFolder(dstFolderOld, oldDstFolderNew);
                                dstFolderOld = null;
                            }

                            if (dstFolderNew != null)
                            {
                                this.RenameFolder(newDstFolderNew, dstFolderNew);
                                dstFolderNew = null;
                            }

                            if (oldDstFolderNew != null && FabricDirectory.Exists(oldDstFolderNew))
                            {
                                FabricDirectory.Delete(oldDstFolderNew, recursive: true, deleteReadOnlyFiles: true);
                                oldDstFolderNew = null;
                            }

                            break;
                        }
                        catch (Exception ex)
                        {
                            this.traceSource.WriteError(
                                ClassName,
                                "EndCopy exception: destination {0}, current object {1} exception {2}\r\n",
                                args.Destination,
                                oldDstFolderOriginal ?? dstFolderOld ?? dstFolderNew ?? oldDstFolderNew ?? "(null)",
                                ex);

                            if (ExceptionHandler.IsFatalException(ex))
                            {
                                throw;
                            }

                            if (i >= MaxRetryAttempts)
                            {
                                Interlocked.Increment(ref this.failedItems);
                            }
                            else
                            {
                                Thread.Sleep(500);
                            }
                        }
                    }
                }
            }
            finally
            {
                if (newDstFolderNew != null && FabricDirectory.Exists(newDstFolderNew))
                {
                    FabricDirectory.Delete(newDstFolderNew, recursive: true, deleteReadOnlyFiles: true);
                }
            }        

            // Signal completion of copy task.
            this.asyncEvent.Set();
        }

        /// <summary>
        /// Copy Item delegate that is used to dequeue a work item that is enqueued to be copied (file/folder)
        /// </summary>
        /// <param name="state">
        /// Instance of CopyArgs that contains the source and destination folders
        /// </param>
        [SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes",
            Justification = "Rename folder shouldn't throw exception as it tries its best to copy rest")]
        private void CopyItem(object state)
        {
            // get the CopyArgs instance from state param
            CopyArgs args = state as CopyArgs;
            try
            {
                // Copy the item calling the appropriate method
                if (args.IsFolder)
                {
                    this.CopyFolder(args.Source, args.Destination);
                }
                else
                {
                    this.CopyFile(args.Source, args.Destination);
                }
            }
            catch (Exception ex)
            {
                // swallow any exception since there is no point throwing from the thread pool thread
                // track the number of items that failed to copy
                Interlocked.Increment(ref this.failedItems);
                this.traceSource.WriteError(ClassName, "CopyItem failed: source {0}, destination {1}, exception {2}\r\n", args.Source, args.Destination, ex);
            }
            finally
            {
                // Decrement work item and dequeue
                this.DecrementWorkItem();
            }
        }

        /// <summary>
        /// Copy Folder method called by the CopyItem delegate to dequeue a folder copy item
        /// This method enumerates the folder enqueues all its children and dequeues the current item
        /// </summary>
        /// <param name="sourceDir">
        /// Source folder to be copied
        /// </param>
        /// <param name="destinationDir">
        /// Destination folder
        /// </param>
        private void CopyFolder(string sourceDir, string destinationDir)
        {
            // create the destination folder if it doesn't exist
            if (!FabricDirectory.Exists(destinationDir))
            {
                FabricDirectory.CreateDirectory(destinationDir);
            }

            // Enumerate source and queue up work items for each child folder
            var directoryNames = FabricDirectory.GetDirectories(
                sourceDir, 
                "*", 
                false, // full path
                SearchOption.TopDirectoryOnly);

            var fileNames = FabricDirectory.GetFiles(
                sourceDir, 
                "*", 
                false, // full path
                SearchOption.TopDirectoryOnly);

            foreach (var directoryName in directoryNames)
            {
                this.QueueCopyItem(sourceDir, destinationDir, directoryName, true);
            }
            
            foreach (var fileName in fileNames)
            {
                this.QueueCopyItem(sourceDir, destinationDir, fileName, false);
            }
        }

        private void QueueCopyItem(string srcDir, string destDir, string itemName, bool isDirectory)
        {
            try
            {
                // increment the workitems count before enqueuing the child folder
                Interlocked.Increment(ref this.workItemsInProgress);

                var src = Path.Combine(srcDir, itemName);
                var dest = Path.Combine(destDir, itemName);

                ThreadPool.QueueUserWorkItem(
                    this.CopyItem,
                    new CopyArgs(src, dest, isDirectory));
            }
            catch (Exception)
            {
                // QueueUserWorkItem may throw if there isn't enough memory
                // make sure to decrement work item count before the exception is caught later in this method
                // no need to check if workitem count is reduced to zero here since it cannot be
                // at this point there is atleast one work item which needs to be dequeued - the current source folder
                Interlocked.Decrement(ref this.workItemsInProgress);
                throw;
            }
        }

        /// <summary>
        /// Copy File method called by the CopyItem delegate to dequeue a file copy item
        /// </summary>
        /// <param name="source">
        /// Source file to be copied
        /// </param>
        /// <param name="destination">
        /// Destination file
        /// </param>
        private void CopyFile(string source, string destination)
        {
            // Copy the file and decrement work items
            if (this.ShouldCopy(source) && this.AreFilesDifferent(source, destination))
            {
                bool isCopied = false;
                Exception ex = null;
                int timeoutInSeconds = 1;
                for (int i = 1; i <= MaxRetryAttempts; i++)
                {
                    try
                    {
                        FabricFile.Copy(source, destination, true);
                        isCopied = true;
                        break;
                    }   
                    catch (Exception e)
                    {
                        if (ExceptionHandler.IsFatalException(e))
                        {
                            throw;
                        }

                        ex = e;
                    }

                    timeoutInSeconds = 2 * timeoutInSeconds;
                    timeoutInSeconds = (timeoutInSeconds >= 180) ? 180 : timeoutInSeconds;
                    Thread.Sleep(timeoutInSeconds * 1000);
                }

                if (!isCopied)
                {
                    this.traceSource.WriteError(
                        ClassName,
                        "File copy failed to copy {0}, to {1}, with message {2}, exception {3}\r\n",
                        source,
                        destination,
                        ex.Message,
                        ex);
                    Interlocked.Increment(ref this.failedItems);
                }
            }
        }

        /// <summary>
        /// Method called by the CopyItem delegate to dequeue the current item and check if the operation is complete
        /// </summary>
        private void DecrementWorkItem()
        {
            // decrement work item and get the current count after this decrement
            long currentItems = Interlocked.Decrement(ref this.workItemsInProgress);

            // if the worktiems count revert to 0, there is no more items queued for copy, 
            // signal the event to indicate that the operation has been completed
            if (0 == currentItems)
            {
                this.internalCopyAsyncEvent.Set();
            }
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
            string extension = Path.GetExtension(source);

            // Should copy if this._filterExtensions is null or the filterextensions does not contain the key
            return this.filterExtensions == null || !this.filterExtensions.ContainsKey(extension);
        }

        /// <summary>
        /// Renames a folder from <paramref name="srcFolderName"/> to <paramref name="dstFolderName"/>
        /// </summary>
        /// <param name="srcFolderName">The source folder.</param>
        /// <param name="dstFolderName">The destination folder.</param>
        [SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes",
            Justification = "Rename folder shouldn't throw exception as it tries its best to copy rest")]
        private void RenameFolder(string srcFolderName, string dstFolderName)
        {
            if (string.IsNullOrEmpty(srcFolderName) || !FabricDirectory.Exists(srcFolderName))
            {
                this.traceSource.WriteError(ClassName, "SourceFolder {0} doesn't exist", srcFolderName);
                throw new ArgumentException(StringResources.ImageStoreError_InvalidSourceFolderSpecified);
            }

            if (string.IsNullOrEmpty(dstFolderName) || FabricDirectory.Exists(dstFolderName))
            {
                this.traceSource.WriteError(ClassName, "Destination folder {0} does exist", dstFolderName);
                throw new ArgumentException(StringResources.ImageStoreError_InvalidDestinationFolderSpecified);
            }

            try
            {
                FabricFile.Move(srcFolderName, dstFolderName);
            }
            catch (Exception ex)
            {
                this.traceSource.WriteError(
                    ClassName,
                    "Failed to rename folder {0} to {1} with error {2}, and detail exception {3}",
                    srcFolderName,
                    dstFolderName,
                    ex.Message,
                    ex);
                throw;
            }
        }

        /// <summary>
        /// Internal class used to pass around source and destination folder pairs
        /// to WaitCallback delegates as the state param
        /// </summary>
        internal class CopyArgs
        {
            /// <summary>
            /// Field to store the source uri to copy from
            /// </summary>
            private readonly string source;

            /// <summary>
            /// Field to store the destination uri to copy to
            /// </summary>
            private readonly string destination;

            /// <summary>
            /// Field that identifies whether the current item is a folder or a file
            /// </summary>
            private readonly bool isFolder;

            /// <summary>
            /// Initializes a new instance of the CopyArgs class. Internal constructor that takes copy arguments for the current copy item
            /// </summary>
            /// <param name="source">Source uri to copy from</param>
            /// <param name="destination">Destination uri to copy to</param>
            /// <param name="isFolder">Identifies if the current copy item is a folder</param>
            internal CopyArgs(string source, string destination, bool isFolder)
            {
                this.source = source;
                this.destination = destination;
                this.isFolder = isFolder;
            }

            /// <summary>
            /// Gets the value of the source.
            /// </summary>
            internal string Source
            {
                get { return this.source; }
            }

            /// <summary>
            /// Gets the value of the destination.
            /// </summary>
            internal string Destination
            {
                get { return this.destination; }
            }

            /// <summary>
            /// Gets a value indicating whether the source is a folder.
            /// </summary>
            internal bool IsFolder
            {
                get { return this.isFolder; }
            }
        }
    }
}
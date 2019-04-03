// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.FileUploader 
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.IO;
    using System.Linq;
    using System.Threading;

    // Base class for uploaders that upload a set of files to a diagnostic store
    internal abstract class FileUploaderBase
    {
        private const string FileUploadTimerIdSuffix = "_FileUploadTimer";
        private const string FileSyncTimerIdSuffix = "_FileSyncTimer";
        private const string LocalMapFolder = "LMap";
        private const int RetryCountForIo = 3;

        // User-friendly ID representing the destination to which files are to be copied.
        // Used only for logging.
        private readonly string destinationId;

        // Set of files to copy on the next file copy pass
        private readonly HashSet<string> filesToCopy;

        // Timer to upload files to file share
        private readonly DcaTimer fileUploadTimer;

        // Local folder containing a map of files that we uploaded
        private readonly string localMap;

        private readonly string folderName;

        // Timer to sync files between the local folder and the file share
        private readonly DcaTimer fileSyncTimer;

        // Object that measures performance
        private readonly FileUploaderBasePerformance perfHelper;

        // Whether or not we are uploading crash dumps of Windows Fabric binaries.
        // If true, we write an ETW event for every file that we upload. This enables
        // users to raise alerts when crash dumps are uploaded.
        private readonly bool uploadingWinFabCrashDumps;

        private readonly string localMapFolderPath;
        private readonly FabricEvents.ExtensionsEvents traceSource;
        private readonly string logSourceId;

        // File system watcher
        private FileSystemWatcher watcher;

        // Whether or not we need to sync files between the local folder and
        // the file share
        private bool fileSyncNeeded;

        // Whether or not the we are in the process of stopping
        private bool stopping;

        // Whether or not the object has been disposed
        private bool disposed;

        protected FileUploaderBase(
            FabricEvents.ExtensionsEvents traceSource, 
            string logSourceId,
            string folderName,
            string destinationId,
            string workFolder,
            TimeSpan uploadInterval,
            TimeSpan fileSyncInterval)
        {
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;
            this.folderName = folderName;
            this.destinationId = destinationId;
            this.localMap = Path.Combine(workFolder, LocalMapFolder);
            this.localMapFolderPath = this.localMap;
            this.uploadingWinFabCrashDumps = folderName.StartsWith(
                                                Utility.CrashDumpsDirectory,
                                                StringComparison.OrdinalIgnoreCase);

            // Create the local map folder
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    {
                        FabricDirectory.CreateDirectory(this.localMap);
                    });
            }
            catch (Exception e)
            {
                throw new IOException(
                    string.Format("Unable to create directory {0}.", this.localMap),
                    e);
            }

            this.perfHelper = new FileUploaderBasePerformance(this.TraceSource, this.LogSourceId);
            this.filesToCopy = new HashSet<string>();

            // When we are first initialized, we always need a file sync because
            // we don't know what local files may have been modified during the
            // time that we were not running.
            this.fileSyncNeeded = true;

            // Configure the file watcher to watch the folder of interest
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    {
                        FabricDirectory.CreateDirectory(this.folderName);
                        this.StartFileWatcher();
                    });
            }
            catch (Exception e)
            {
                throw new IOException(
                    string.Format("Unable to create directory {0}.", this.folderName),
                    e);
            }

            // Create a timer to schedule the upload of files to the destination
            string timerId = string.Concat(
                                 this.LogSourceId,
                                 FileUploadTimerIdSuffix);
            this.fileUploadTimer = new DcaTimer(
                                           timerId,
                                           this.UploadToFileShare,
                                           uploadInterval,
                                           true);

            // Create a timer to check whether files on the local node need to 
            // be sync'ed with the files on the destination.
            timerId = string.Concat(
                          this.LogSourceId,
                          FileSyncTimerIdSuffix);
            this.fileSyncTimer = new DcaTimer(
                                         timerId,
                                         this.CheckForFilesToBeSynced,
                                         fileSyncInterval);
        }

        // Object used for tracing
        protected FabricEvents.ExtensionsEvents TraceSource
        {
            get { return this.traceSource; }
        }

        // Tag used to represent the source of the log message
        protected string LogSourceId
        {
            get { return this.logSourceId; }
        }

        protected string LocalMapFolderPath
        {
            get { return this.localMapFolderPath; }
        }

        public virtual void Start()
        {
            this.fileUploadTimer.Start();

            // Schedule a work item to check for files to be sync'ed immediately.
            // Since it is the first check, we do it immediately. Subsequent checks
            // will be driven by the timer.
            ThreadPool.QueueUserWorkItem(this.CheckForFilesToBeSynced);
        }

        public void FlushData()
        {
            // Compute the maximum time available for flush
            DateTime dueTime = DateTime.UtcNow.Add(Utility.GetPluginFlushTimeout());

            // Figure out which files need to be synced with the destination.
            // This is best effort, so we ignore return value.
            this.TryCheckForFilesToBeSynced(dueTime);

            // Upload the files that need to be synced
            this.UploadToFileShare(dueTime);
        }

        protected abstract bool CopyFileToDestination(string source, string sourceRelative, int retryCount, out bool fileSkipped, out bool fileCompressed);

        protected virtual bool FileUploadPassBegin()
        {
            return true;
        }

        protected void DisposeBase()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            // Keep track of the fact that the consumer is stopping
            this.stopping = true;

            if (null != this.fileUploadTimer)
            {
                // Stop and dispose timer
                this.fileUploadTimer.StopAndDispose();

                // Wait for timer callback to finish executing
                this.fileUploadTimer.DisposedEvent.WaitOne();
            }

            if (null != this.fileSyncTimer)
            {
                // Stop and dispose timer
                this.fileSyncTimer.StopAndDispose();

                // Wait for timer callback to finish executing
                this.fileSyncTimer.DisposedEvent.WaitOne();
            }

            // Configure the file watcher to stop watching the folder of interest
            this.StopFileWatcher();
        }

        protected virtual void FileUploadPassEnd()
        {
        }

        private static bool ShouldCopy(FileSystemEventArgs e)
        {
            // If the entity being changed is a directory, we only care about
            // creation or rename. We should ignore changes to last write time
            // of directories because we get a separate notifications about the
            // last write time change of the files inside it that were modified.
            if (e.ChangeType == WatcherChangeTypes.Changed)
            {
                bool shouldCopy = false;
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            FileInfo fileInfo = new FileInfo(e.FullPath);
                            shouldCopy = !fileInfo.Attributes.HasFlag(FileAttributes.Directory);
                        });
                }
                catch (Exception)
                {
                    return false;
                }

                return shouldCopy;
            }

            return true;
        }

        private void StartFileWatcher()
        {
            this.watcher = new FileSystemWatcher(this.folderName)
            {
                IncludeSubdirectories = true,
                NotifyFilter = NotifyFilters.LastWrite | NotifyFilters.DirectoryName | NotifyFilters.FileName
            };
            this.watcher.Changed += this.FileChangeHandler;
            this.watcher.Renamed += this.FileRenameHandler;
            this.watcher.Created += this.FileChangeHandler;
            this.watcher.Error += (sender, eventArgs) =>
            {
                // Can't guarantee files didn't change so we should sync.
                this.fileSyncNeeded = true;
            };
            this.watcher.EnableRaisingEvents = true;
        }

        private void StopFileWatcher()
        {
            this.watcher.EnableRaisingEvents = false;
            this.watcher.Changed -= this.FileChangeHandler;
            this.watcher.Renamed -= this.FileRenameHandler;
            this.watcher.Created -= this.FileChangeHandler;
            this.watcher.Dispose();
        }

        private void FileChangeHandler(object sender, FileSystemEventArgs e)
        {
            if (ShouldCopy(e))
            {
                this.AddFileToCopy(e.Name);
            }
        }

        private void FileRenameHandler(object sender, RenamedEventArgs e)
        {
            this.AddFileToCopy(e.Name);
        }

        private void AddFileToCopy(string filename)
        {
#if DotNetCoreClrLinux
            if (this.filesToCopy.Contains(filename))
            {
                // On Linux, the filewatcher keeps raising events for every write
                // Return here stops trace spamming
                return;
            }

            if(Utility.IgnoreUploadFileList.Exists(x => x.Equals(filename)))
            {
                this.TraceSource.WriteInfo(this.logSourceId, "Skipping file:{0} for upload as it is in ignore list.", filename);
                return;
            }
#endif

            this.TraceSource.WriteInfo(this.logSourceId, "Found file to copy {0}.", filename);

            lock (this.filesToCopy)
            {
                this.filesToCopy.Add(filename);
            } 
        }

        private void UploadToFileShare(object state)
        {
            this.UploadToFileShare(DateTime.MaxValue);
        }

        private void UploadToFileShare(DateTime dueTime)
        {
            this.TraceSource.WriteInfo(
                this.LogSourceId,
                "Starting upload pass. Source: {0}, destination: {1}.",
                this.folderName,
                this.destinationId);

            // In case of IO failures, we retry only if the caller did not pass a timeout.
            // If the caller passed in a timeout, we want to get things done as quickly as
            // possible, so no retries.
            int retryCount = dueTime.Equals(DateTime.MaxValue) ? RetryCountForIo : 0;

            if (false == this.FileUploadPassBegin())
            {
                return;
            }

            // Get the current list of files that we need to upload
            string[] filesToCopySnapshot;
            lock (this.filesToCopy)
            {
                filesToCopySnapshot = this.filesToCopy.ToArray();
                this.filesToCopy.Clear();
            }

            // Copy the files to the file share
            List<FolderInfo> foldersToCopy = new List<FolderInfo>();
            foreach (string fileToCopy in filesToCopySnapshot)
            {
                if (!this.TryUploadFileToFileShare(dueTime, fileToCopy, retryCount, foldersToCopy))
                {
                    this.fileSyncNeeded = true;
                }
            }

            this.FileUploadPassEnd();

            this.TraceSource.WriteInfo(
                this.LogSourceId,
                "Finished upload pass. Source: {0}, destination: {1}.{2}",
                this.folderName,
                this.destinationId,
                this.fileSyncNeeded ? " File sync was scheduled." : string.Empty);

            // For each folder that we encountered, add its contents to the set
            // of files to be copied. Those files will be copied over in the next
            // pass.
            foreach (FolderInfo folderToCopy in foldersToCopy)
            {
                bool interruptedDueToStop = false;
                bool interruptedDueToTimeout = false;
                if (this.CheckForInterruptions(dueTime, ref interruptedDueToStop, ref interruptedDueToTimeout))
                {
                    var message = interruptedDueToStop
                        ? "The consumer is being stopped. Therefore, enumeration of folders that need to be uploaded is being interrupted."
                        : "Enumeration of folders that need to be uploaded is being interrupted due to a timeout.";
                    this.TraceSource.WriteInfo(
                        this.LogSourceId,
                        message);
                    break;
                }

                this.AddFilesToCopySet(folderToCopy, dueTime);
            }

            // Schedule the next pass
            this.fileUploadTimer.Start();
        }

        private bool TryUploadFileToFileShare(DateTime dueTime, string fileToCopy, int retryCount, List<FolderInfo> foldersToCopy)
        {
            bool interruptedDueToStop = false;
            bool interruptedDueToTimeout = false;
            if (this.CheckForInterruptions(dueTime, ref interruptedDueToStop, ref interruptedDueToTimeout))
            {
                this.TraceSource.WriteInfo(
                    this.LogSourceId,
                    "No more files from {0} will be uploaded to {1} because {2}.",
                    this.folderName,
                    this.destinationId,
                    interruptedDueToStop ? "the consumer is being stopped" : "of a timeout");
                return false;
            }

            string source = Path.Combine(this.folderName, fileToCopy);

            FileAttributes attr;
            if (!this.TryGetFileAttributes(retryCount, source, out attr))
            {
                return false;
            }

            if ((attr & FileAttributes.Directory) == FileAttributes.Directory)
            {
                // The path is a directory. We'll need to copy all the files
                // and folders inside it.
                FolderInfo folderToCopy = new FolderInfo
                {
                    FullPath = source,
                    RelativePath = fileToCopy
                };
                foldersToCopy.Add(folderToCopy);
                return false;
            }

            // Figure out the name of the directory in the local map.
            string localMapDestination = Path.Combine(this.localMap, fileToCopy);
            string localMapDestinationDir;
            try
            {
                localMapDestinationDir = FabricPath.GetDirectoryName(localMapDestination);
            }
            catch (PathTooLongException e)
            {
                // The path to the local map directory is too long. Skip it.
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Failed to create local map directory for copying file {0}. Exception information: {1}",
                    source,
                    e);
                return false;
            }

            bool sourceFileNotFound;
            DateTime sourceLastWriteTime;
            if (!this.TryGetLastWriteTime(retryCount, source, out sourceFileNotFound, out sourceLastWriteTime))
            {
                return false;
            }

            bool fileSkipped, fileCompressed = false;
            if (sourceFileNotFound)
            {
                // The source file was not found. Maybe it
                // got deleted before we had a chance to copy
                // it. Skip it and move on.
                fileSkipped = true;
            }
            else
            {
                // Copy the file over to its destination
                if (false == this.CopyFileToDestination(source, fileToCopy, retryCount, out fileSkipped, out fileCompressed))
                {
                    this.fileSyncNeeded = true;

                    // Add the file back for retry next interval.
                    this.AddFileToCopy(fileToCopy);

                    return false;
                }
            }

            if (fileSkipped)
            {
                this.TraceSource.WriteWarning(
                    this.LogSourceId,
                    "Errors encountered while uploading file {0} to {1}. Skipping this file.",
                    source,
                    this.destinationId);
                return false;
            }

            if (fileCompressed)
            {
                // Note that we only change this for local Map
                // but not fileToCopy, as FileToCopy is used to
                // conditionally write an event for crash dump
                // based on extension below.
                localMapDestination = Path.ChangeExtension(localMapDestination, "zip");
            }

            if (!this.TryUpdateLocalMap(retryCount, localMapDestinationDir, localMapDestination, sourceLastWriteTime))
            {
                return false;
            }

            if (this.uploadingWinFabCrashDumps)
            {
                // Before logging an event that a crash dump was found, double-check
                // to make sure that the file name extension is ".dmp".  
                string extension = Path.GetExtension(fileToCopy);
                if (extension.Equals(".dmp", StringComparison.OrdinalIgnoreCase))
                {
                    string fileNameWithoutPath = Path.GetFileName(fileToCopy);
                    FabricEvents.Events.WindowsFabricCrashDumpFound(fileNameWithoutPath);
                }
            }

            return true;
        }

        private bool TryUpdateLocalMap(int retryCount, string localMapDestinationDir, string localMapDestination, DateTime sourceLastWriteTime)
        {
            // Record the source file's last write time based on the snapshot
            // we took before copying the source file. We know that we copied
            // over a version that it at least as new as the timestamp in the
            // snapshot.

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
                        retryCount);
                }
                catch (Exception e)
                {
                    this.TraceSource.WriteExceptionAsError(
                        this.LogSourceId,
                        e,
                        "Failed to create directory {0} in the local map of uploaded files.",
                        localMapDestinationDir);
                    return false;
                }
            }

            // If the file doesn't exist in the local map, create it.
            if (false == FabricFile.Exists(localMapDestination))
            {
                if (!this.TryCreateLocalMapFile(retryCount, localMapDestination, sourceLastWriteTime))
                {
                    return false;
                }
            }
            else
            {
                // File already exists in the local map. Set the last
                // write time for the file in the local map to the last
                // write time from the source file snapshot
                // It's a Best Effort operation.
                this.TryUpdateLastWriteTime(localMapDestination, sourceLastWriteTime);
            }

            return true;
        }

        private bool TryGetLastWriteTime(int retryCount, string source, out bool sourceFileNotFound, out DateTime sourceLastWriteTime)
        {
            // Take a snapshot of the source file's last write time
            sourceFileNotFound = false;
            sourceLastWriteTime = DateTime.MinValue;

            var localSourceFileNotFound = sourceFileNotFound;
            var localSourceLastWriteTime = sourceLastWriteTime;
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    {
                        try
                        {
                            // Take a snapshot of the source file's last write time
                            FileInfo sourceInfo = new FileInfo(source);
                            localSourceLastWriteTime = sourceInfo.LastWriteTime;
                        }
                        catch (FileNotFoundException)
                        {
                            localSourceFileNotFound = true;
                        }
                    },
                    retryCount);
            }
            catch (Exception e)
            {
                this.TraceSource.WriteExceptionAsError(
                    this.LogSourceId,
                    e,
                    "Failed to retrieve last write time of file {0}",
                    source);
                this.fileSyncNeeded = true;
                return false;
            }

            sourceLastWriteTime = localSourceLastWriteTime;
            sourceFileNotFound = localSourceFileNotFound;
            return true;
        }

        private bool TryGetFileAttributes(int retryCount, string source, out FileAttributes attr)
        {
            var localAttr = FileAttributes.Normal;
            attr = localAttr;
            bool attributesKnown = false;
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    {
                        try
                        {
                            localAttr = File.GetAttributes(source);
                            attributesKnown = true;
                        }
                        catch (Exception e)
                        {
                            // If an exception was thrown because the file or directory
                            // was not found, then we handle the exception and move on
                            if (false == ((e is FileNotFoundException) ||
                                          (e is DirectoryNotFoundException)))
                            {
                                throw;
                            }
                        }
                    },
                    retryCount);
            }
            catch (Exception e)
            {
                this.TraceSource.WriteExceptionAsError(
                    this.LogSourceId,
                    e,
                    "Unable to retrieve attributes for {0}.",
                    source);
                this.fileSyncNeeded = true;
                return false;
            }

            if (false == attributesKnown)
            {
                // We couldn't determine if the source path is a file or a directory. 
                // Skip it and move on.
                return false;
            }

            attr = localAttr;
            return true;
        }

        private bool TryCreateLocalMapFile(int retryCount, string localMapDestination, DateTime sourceLastWriteTime)
        {
            // First create a temp file
            string tempFile = Utility.GetTempFileName();
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    {
                        using (FileStream fs = FabricFile.Create(tempFile))
                        {
                        }
                    },
                    retryCount);
            }
            catch (Exception e)
            {
                this.TraceSource.WriteExceptionAsError(
                    this.LogSourceId,
                    e,
                    "Failed to create temporary file to move to {0}.",
                    localMapDestination);
                return false;
            }

            // Move the temp file to the local map
            try
            {
                // Set the last write time for the temp file to the
                // last write time from the source file snapshot
                // Today, it appear that last write time isn't really being used (for LMap files.. so we ignore the rare case where it may fail)
                this.TryUpdateLastWriteTime(tempFile, sourceLastWriteTime);

                Utility.PerformIOWithRetries(
                    () => { FabricFile.Move(tempFile, localMapDestination); },
                    retryCount);
            }
            catch (Exception e)
            {
                this.TraceSource.WriteExceptionAsError(
                    this.LogSourceId,
                    e,
                    "Failed to move file {0} to {1}.",
                    tempFile,
                    localMapDestination);
                return false;
            }

            return true;
        }

        private void AddFilesToCopySet(FolderInfo folderToCopy, DateTime dueTime)
        {
            this.TraceSource.WriteInfo(
                this.LogSourceId,
                "Starting to examine folder {0} for files to upload to the file share ...",
                folderToCopy.FullPath);

            // Do a breadth-first traversal of the folder 
            Queue<FolderInfo> folderQueue = new Queue<FolderInfo>();
            folderQueue.Enqueue(folderToCopy);
            bool interruptedDueToStop = false;
            bool interruptedDueToTimeout = false;
            while (folderQueue.Count != 0)
            {
                if (this.CheckForInterruptions(dueTime, ref interruptedDueToStop, ref interruptedDueToTimeout))
                {
                    break;
                }

                FolderInfo folderInfo = folderQueue.Dequeue();

                try
                {
                    // Create a DirectoryInfo object representing the current folder
                    DirectoryInfo dirInfo = new DirectoryInfo(folderInfo.FullPath);

                    // Add the current folder's sub-folders to the queue
                    IEnumerable<DirectoryInfo> subFolders = dirInfo.EnumerateDirectories();
                    foreach (DirectoryInfo subFolder in subFolders)
                    {
                        if (this.CheckForInterruptions(dueTime, ref interruptedDueToStop, ref interruptedDueToTimeout))
                        {
                            break;
                        }

                        FolderInfo subFolderInfo = new FolderInfo
                        {
                            FullPath = subFolder.FullName,
                            RelativePath = Path.Combine(
                                folderInfo.RelativePath,
                                subFolder.Name)
                        };
                        folderQueue.Enqueue(subFolderInfo);
                    }

                    // Iterate over the files in the current folder and add them
                    // to the set of files to be uploaded
                    IEnumerable<FileInfo> files = dirInfo.EnumerateFiles();
                    foreach (FileInfo file in files)
                    {
                        try
                        {
                            if (this.CheckForInterruptions(dueTime, ref interruptedDueToStop, ref interruptedDueToTimeout))
                            {
                                break;
                            }

                            this.AddFileToCopy(Path.Combine(
                                folderInfo.RelativePath,
                                file.Name));
                        }
                        catch (IOException)
                        {
                            // Unable to read information about the file. Maybe
                            // it got deleted. Handle the exception and move on.
                        }
                    }
                }
                catch (IOException e)
                {
                    // Exception occurred during enumeration of directories or files.
                    // Handle the exception and move on.
                    this.TraceSource.WriteError(
                        this.LogSourceId,
                        "Failed to enumerate folder {0} to find files to upload. Exception information: {1}",
                        folderInfo.FullPath,
                        e);
                }
            }

            if (interruptedDueToStop)
            {
                this.TraceSource.WriteInfo(
                    this.LogSourceId,
                    "The consumer is being stopped. Therefore, examination of folder {0} for files to upload to the file share was interrupted.",
                    folderToCopy.FullPath);
            }
            else if (interruptedDueToTimeout)
            {
                this.TraceSource.WriteInfo(
                    this.LogSourceId,
                    "Examination of folder {0} for files to upload to the file share was interrupted due to timeout.",
                    folderToCopy.FullPath);
            }
            else
            {
                this.TraceSource.WriteInfo(
                    this.LogSourceId,
                    "Finished examining folder {0} for files to upload to the file share.",
                    folderToCopy.FullPath);
            }
        }

        private void CheckForFilesToBeSynced(object state)
        {
            if (false == this.fileSyncNeeded)
            {
                // No need to synchronize files. Nothing more to be done here.
                this.TraceSource.WriteInfo(
                    this.LogSourceId,
                    "Skipping examination of folder {0} for files to synchronize with the file share.",
                    this.folderName);

                // Schedule the next pass
                this.fileSyncTimer.Start();
                return;
            }

            // On errors, try again after interval.
            this.fileSyncNeeded = !this.TryCheckForFilesToBeSynced(DateTime.MaxValue);

            // Schedule the next pass
            this.fileSyncTimer.Start();
        }

        private bool TryCheckForFilesToBeSynced(DateTime dueTime)
        {
            var syncCompletedSuccessfully = true;
            this.perfHelper.FileSyncPassBegin();
            this.TraceSource.WriteInfo(
                this.LogSourceId,
                "Starting to examine folder {0} for files to synchronize with the file share ...",
                this.folderName);

            // Do a breadth-first traversal of the folder from which files need
            // to be uploaded
            FolderInfo folderInfo = new FolderInfo
            {
                FullPath = this.folderName,
                RelativePath = string.Empty,
                LocalMapPath = this.localMap
            };

            Queue<FolderInfo> folderQueue = new Queue<FolderInfo>();
            folderQueue.Enqueue(folderInfo);
            bool syncInterruptedDueToStop = false;
            bool syncInterruptedDueToTimeout = false;
            while (folderQueue.Count != 0)
            {
                if (this.CheckForInterruptions(dueTime, ref syncInterruptedDueToStop, ref syncInterruptedDueToTimeout))
                {
                    break;
                }

                folderInfo = folderQueue.Dequeue();

                try
                {
                    // Create a DirectoryInfo object representing the current folder
                    DirectoryInfo dirInfo = new DirectoryInfo(folderInfo.FullPath);

                    // Add the current folder's sub-folders to the queue
                    IEnumerable<DirectoryInfo> subFolders = dirInfo.EnumerateDirectories();
                    foreach (DirectoryInfo subFolder in subFolders)
                    {
                        if (this.CheckForInterruptions(dueTime, ref syncInterruptedDueToStop, ref syncInterruptedDueToTimeout))
                        {
                            break;
                        }

                        FolderInfo subFolderInfo = new FolderInfo();
                        subFolderInfo.FullPath = subFolder.FullName;
                        subFolderInfo.RelativePath = string.IsNullOrEmpty(folderInfo.RelativePath) ?
                                                         subFolder.Name :
                                                         Path.Combine(
                                                             folderInfo.RelativePath,
                                                             subFolder.Name);
                        subFolderInfo.LocalMapPath = Path.Combine(
                                                         folderInfo.LocalMapPath,
                                                         subFolder.Name);
                        folderQueue.Enqueue(subFolderInfo);
                    }

                    // Iterate over the files in the current folder and figure out which
                    // ones need to be synchronized
                    IEnumerable<FileInfo> files = dirInfo.EnumerateFiles();
                    foreach (FileInfo file in files)
                    {
                        try
                        {
                            if (this.CheckForInterruptions(dueTime, ref syncInterruptedDueToStop, ref syncInterruptedDueToTimeout))
                            {
                                break;
                            }

                            bool needsSync = false;
                            string localMapFile = Path.Combine(
                                                      folderInfo.LocalMapPath,
                                                      file.Name);

                            if (false == FabricFile.Exists(localMapFile))
                            {
                                // We don't have any record of this file in our local map.
                                // So we definitely need to synchronize this file.
                                needsSync = true;
                            }
                            else
                            {
                                // We have a record of this file in our local map. Compare 
                                // the timestamp of the file with the timestamp of its 
                                // counterpart in the local map to determine if the file
                                // has changed since we last copied it to the destination.
                                FileInfo localMapFileInfo = new FileInfo(localMapFile);
                                if (localMapFileInfo.LastWriteTime.CompareTo(file.LastWriteTime) < 0)
                                {
                                    // The file has changed since we last copied it to the
                                    // destination. We need to synchronize this file.
                                    needsSync = true;
                                }
                            }

                            if (needsSync)
                            {
                                // This file needs to be synchronized. Add it to the list
                                // of files to be copied to the file share.
                                this.AddFileToCopy(Path.Combine(
                                    folderInfo.RelativePath,
                                    file.Name));
                            }

                            this.perfHelper.FileCheckedForSync();
                        }
                        catch (IOException)
                        {
                            // Unable to read information about the file. Maybe
                            // it got deleted. Handle the exception and move on.
                            syncCompletedSuccessfully = false;
                        }
                    }
                }
                catch (IOException e)
                {
                    // Exception occurred during enumeration of directories or files.
                    // Handle the exception and move on.
                    this.TraceSource.WriteError(
                        this.LogSourceId,
                        "Failed to enumerate folder {0} while looking for files to sync. Exception information: {1}",
                        folderInfo.FullPath,
                        e);
                    syncCompletedSuccessfully = false;
                }
            }

            if (syncInterruptedDueToStop)
            {
                this.TraceSource.WriteInfo(
                    this.LogSourceId,
                    "The consumer is being stopped. Therefore, examination of folder {0} for files to synchronize with the file share was interrupted.",
                    this.folderName);
                syncCompletedSuccessfully = false;
            }
            else if (syncInterruptedDueToTimeout)
            {
                this.TraceSource.WriteInfo(
                    this.LogSourceId,
                    "Examination of folder {0} for files to synchronize with the file share was interrupted due to timeout.",
                    this.folderName);
                syncCompletedSuccessfully = false;
            }
            else
            {
                this.TraceSource.WriteInfo(
                    this.LogSourceId,
                    "Finished examining folder {0} for files to synchronize with the file share.",
                    this.folderName);
            }

            this.perfHelper.FileSyncPassEnd();
            return syncCompletedSuccessfully;
        }

        private bool CheckForInterruptions(DateTime dueTime, ref bool syncInterruptedDueToStop, ref bool syncInterruptedDueToTimeout)
        {
            if (this.stopping)
            {
                syncInterruptedDueToStop = true;
                return true;
            }

            if (dueTime.CompareTo(DateTime.UtcNow) < 0)
            {
                syncInterruptedDueToTimeout = true;
                return true;
            }

            return false;
        }

        /// <summary>
        /// Bit of explanation required here. So, we have a problem at hand where some of our files are beyond the max that .NET can handle for us.
        /// One such operation that .net can't handle is setting last write time. This is a Windows specific problem and not something that happens on windows.
        /// </summary>
        /// <remarks>
        /// The below code is written to minimize the changes in the current behavior. First we use the .net API and only if we get a Path too long exception, we fallback to Native Version. Technically,
        /// we can always use the Native one for Windows, but this change is going very late in release, as part of a P0 bug, and I am aiming for
        /// as few deviation from original behavior as possible. In 6.5, this should be cleaned up.
        /// </remarks>
        /// <param name="fullPath"></param>
        /// <param name="dateTime"></param>
        /// <returns></returns>
        private bool TryUpdateLastWriteTime(string fullPath, DateTime dateTime)
        {
            try
            {
                File.SetLastWriteTime(fullPath, dateTime);
                return true;
            }
            catch (PathTooLongException)
            {
                this.TraceSource.WriteInfo(
                    this.LogSourceId,
                    "Encountered Path too long exception while updating LastWritten time of file : {0}. Retrying with Native API",
                    fullPath);

                try
                {
                    FabricFile.SetLastWriteTime(fullPath, dateTime);
                    return true;
                }
                catch (Win32Exception exp)
                {
                    this.TraceSource.WriteWarning(
                        this.LogSourceId,
                        "Encountered Exception: {0} while Trying to update file : {1} last Write time to : {2}",
                        exp.ToString(),
                        fullPath,
                        dateTime);

                    return false;
                }
            }
        }

        private struct FolderInfo
        {
            internal string FullPath;
            internal string RelativePath;
            internal string LocalMapPath;
        }
    }
}
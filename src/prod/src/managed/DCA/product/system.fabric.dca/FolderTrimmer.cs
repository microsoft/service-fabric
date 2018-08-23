// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.FileUploader 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.IO;
    using System.Linq;

    // This class implements methods for deleting old files from a 
    // folder, with additional support for customizing the deletion
    // logic
    internal class FolderTrimmer : IDisposable
    {
        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Time after which the files become a candidate for deletion
        private readonly TimeSpan fileDeletionAge;

        // Whether or not the we are in the process of stopping
        private bool stopping;

        // Whether or not the object has been disposed
        private bool disposed;

        internal FolderTrimmer(
                        FabricEvents.ExtensionsEvents traceSource,
                        string logSourceId,
                        TimeSpan fileDeletionAge)
        {
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;
            this.fileDeletionAge = fileDeletionAge;
        }

        internal delegate bool DestinationOperation(object context);

        internal delegate bool DestinationOperationPerformer(DestinationOperation operation, object context);

        internal delegate void SetFolderDeletionInfoContext(FolderDeletionInfo folderInfo, FolderDeletionInfo parentFolderInfo, string folderNameWithoutPath);

        internal delegate bool IsOkToDeleteFile(FolderDeletionInfo folderInfo, string fileName);

        // Events raised when we go through the process of deleting old files
        internal event EventHandler OnFolderEnumerated;

        internal event EventHandler OnFolderDeleted;

        internal event EventHandler OnFileEnumerated;

        internal event EventHandler OnPreFileDeletion;

        internal event EventHandler OnPostFileDeletion;

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            // Keep track of the fact that the trimmer is stopping
            this.stopping = true;

            GC.SuppressFinalize(this);
        }

        internal void DeleteOldFilesFromFolder(
                            string folder,
                            SetFolderDeletionInfoContext setContext,
                            IsOkToDeleteFile isOkToDelete,
                            DestinationOperationPerformer destinationOperationPerformer)
        {
            this.traceSource.WriteInfo(
                this.logSourceId,
                "Starting deletion of old files in directory {0} ...",
                folder);

            List<FolderDeletionInfo> folderList;
            this.GetSubFoldersToDelete(folder, destinationOperationPerformer, setContext, out folderList);
            this.DeleteFilesFromSubFolders(folder, destinationOperationPerformer, folderList, isOkToDelete);

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Finished deletion of old logs in directory {0}.",
                folder);
        }

        private void GetSubFoldersToDelete(
                            string folder,
                            DestinationOperationPerformer destinationOperationPerformer,
                            SetFolderDeletionInfoContext setContext,
                            out List<FolderDeletionInfo> folderList)
        {
            // Do a breadth-first traversal of the folder and make a list of all
            // subfolders (including itself)
            folderList = new List<FolderDeletionInfo>();

            FolderDeletionInfo folderInfo = new FolderDeletionInfo();
            folderInfo.FolderName = folder;
            folderInfo.Context = null;
            if (null != setContext)
            {
                setContext(folderInfo, null, null);
            }

            Queue<FolderDeletionInfo> folderQueue = new Queue<FolderDeletionInfo>();
            folderQueue.Enqueue(folderInfo);

            bool enumerationInterruptedDueToStop = false;
            while (folderQueue.Count != 0)
            {
                if (this.stopping)
                {
                    enumerationInterruptedDueToStop = true;
                    break;
                }

                folderInfo = folderQueue.Dequeue();
                folderList.Add(folderInfo);

                SubFolderEnumerationData enumData = new SubFolderEnumerationData();
                enumData.DestinationOperationPerformer = destinationOperationPerformer;
                enumData.FolderInfo = folderInfo;
                enumData.FolderQueue = folderQueue;
                enumData.SetContext = setContext;
                try
                {
                    Utility.PerformIOWithRetries(
                        this.AddSubFoldersToQueue,
                        enumData);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Unable to enumerate subfolders of folder {0} while attempting to delete old files.",
                        folderInfo.FolderName);
                }
            }

            if (enumerationInterruptedDueToStop)
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "The consumer is being stopped. Therefore, enumeration of files to delete from folder {0} was interrupted.",
                    folder);
                folderList.Clear();
                return;
            }

            // Now we have a list of folders, top-down. However, we want to 
            // process the folders bottom-up, so that the folders themselves 
            // can be deleted if they become empty. So let's reverse the list.
            folderList.Reverse();
        }

        private void AddSubFoldersToQueue(object context)
        {
            SubFolderEnumerationData enumData = (SubFolderEnumerationData)context;
            string folder = enumData.FolderInfo.FolderName;
            DestinationOperationPerformer destinationOperationPerformer = enumData.DestinationOperationPerformer;

            if (null != destinationOperationPerformer)
            {
                // Folder is at the destination. So we might need to impersonate.
                destinationOperationPerformer(
                    this.AddSubFoldersToQueueWorker,
                    context);
            }
            else
            {
                // Folder is not at the destination. No need to impersonate.
                this.AddSubFoldersToQueueWorker(context);
            }
        }

        private bool AddSubFoldersToQueueWorker(object context)
        {
            SubFolderEnumerationData enumData = (SubFolderEnumerationData)context;
            string folder = enumData.FolderInfo.FolderName;
            Queue<FolderDeletionInfo> folderQueue = enumData.FolderQueue;
            SetFolderDeletionInfoContext setContext = enumData.SetContext;

            if (false == FabricDirectory.Exists(folder))
            {
                // Directory does not exist. Nothing more to be done here.
                return true;
            }

            // Create a DirectoryInfo object representing the current folder
            DirectoryInfo dirInfo = new DirectoryInfo(folder);

            // Add the current folder's sub-folders to the queue
            IEnumerable<DirectoryInfo> subFolders = dirInfo.EnumerateDirectories();
            foreach (DirectoryInfo subFolder in subFolders)
            {
                if (this.stopping)
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "The consumer is being stopped. Therefore, enumeration of subfolders of folder {0} during old file deletion was interrupted.",
                        folder);
                    break;
                }

                FolderDeletionInfo subFolderInfo = new FolderDeletionInfo();
                subFolderInfo.FolderName = subFolder.FullName;
                subFolderInfo.Context = null;
                if (null != setContext)
                {
                    setContext(subFolderInfo, enumData.FolderInfo, subFolder.Name);
                }

                folderQueue.Enqueue(subFolderInfo);
            }

            return true;
        }

        private void DeleteDirectoryIfOld(DirectoryDeletionInfo delInfo)
        {
            DirectoryInfo dirInfo = new DirectoryInfo(delInfo.DirName);
            if (dirInfo.LastWriteTimeUtc.CompareTo(delInfo.CutoffTime) < 0)
            {
                FabricDirectory.Delete(delInfo.DirName, true);
            }
        }

        private bool DeleteDirectoryAtDestination(object context)
        {
            DirectoryDeletionInfo delInfo = (DirectoryDeletionInfo)context;
            this.DeleteDirectoryIfOld(delInfo);
            return true;
        }

        private void DeleteFilesFromSubFolders(
                            string folder,
                            DestinationOperationPerformer destinationOperationPerformer,
                            List<FolderDeletionInfo> folderList,
                            IsOkToDeleteFile isOkToDelete)
        {
            // Figure out the timestamp before which all files will be deleted
            DateTime cutoffTime = DateTime.UtcNow.Add(-this.fileDeletionAge);

            foreach (FolderDeletionInfo folderInfo in folderList)
            {
                if (this.stopping)
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "The consumer is being stopped. Therefore, no more files from folder {0} will be deleted.",
                        folder);
                    break;
                }

                DeleteFilesData deletionData = new DeleteFilesData();
                deletionData.DestinationOperationPerformer = destinationOperationPerformer;
                deletionData.FolderInfo = folderInfo;
                deletionData.CutoffTime = cutoffTime;
                deletionData.IsOkToDelete = isOkToDelete;
                try
                {
                    Utility.PerformIOWithRetries(
                        this.DeleteFilesFromFolder,
                        deletionData);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Unable to enumerate files in folder {0} while attempting to delete old files.",
                        folderInfo.FolderName);
                    continue;
                }

                if (null != this.OnFolderEnumerated)
                {
                    this.OnFolderEnumerated(this, EventArgs.Empty);
                }

                // We want to delete the directory if it is empty and if it is
                // not the root directory. 
                if (false == folder.Equals(
                                    folderInfo.FolderName,
                                    StringComparison.OrdinalIgnoreCase))
                {
                    // This is not the root directory. Try to delete it. If the 
                    // directory is not empty, an IOException is thrown. We handle
                    // the exception and move on.
                    try
                    {
                        DirectoryDeletionInfo delInfo = new DirectoryDeletionInfo() { DirName = folderInfo.FolderName, CutoffTime = cutoffTime };
                        if (null != destinationOperationPerformer)
                        {
                            // Folder is at the destination. So we might need to impersonate.
                            destinationOperationPerformer(
                                this.DeleteDirectoryAtDestination,
                                delInfo);
                        }
                        else
                        {
                            // Folder is not at the destination. No need to impersonate.
                            this.DeleteDirectoryIfOld(delInfo);
                        }

                        if (null != this.OnFolderDeleted)
                        {
                            this.OnFolderDeleted(this, EventArgs.Empty);
                        }
                    }
                    catch (Exception e)
                    {
                        if (false == ((e is FabricException) || (e is IOException)))
                        {
                            throw;
                        }
                    }
                }
            }
        }

        private void DeleteFilesFromFolder(object context)
        {
            DeleteFilesData data = (DeleteFilesData)context;
            string folder = data.FolderInfo.FolderName;
            DestinationOperationPerformer destinationOperationPerformer = data.DestinationOperationPerformer;

            if (null != destinationOperationPerformer)
            {
                // Folder is at the destination. So we might need to impersonate.
                destinationOperationPerformer(
                    this.DeleteFilesFromFolderWorker,
                    context);
            }
            else
            {
                // Folder is not at the destination. No need to impersonate.
                this.DeleteFilesFromFolderWorker(context);
            }
        }

        private bool DeleteFilesFromFolderWorker(object context)
        {
            DeleteFilesData data = (DeleteFilesData)context;
            string folder = data.FolderInfo.FolderName;
            DateTime cutoffTime = data.CutoffTime;
            IsOkToDeleteFile isOkToDelete = data.IsOkToDelete;

            if (false == FabricDirectory.Exists(folder))
            {
                // Directory does not exist. Nothing more to be done here.
                return true;
            }

            // Iterate over the files in the current folder and figure out which
            // ones need to be deleted
            DirectoryInfo dirInfo = new DirectoryInfo(folder);
            IEnumerable<FileInfo> files = dirInfo.EnumerateFiles().Where(file => file.LastWriteTimeUtc.CompareTo(cutoffTime) < 0);
            foreach (FileInfo file in files)
            {
                if (this.stopping)
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "The consumer is being stopped. Therefore, no more files from folder {0} will be deleted.",
                        folder);
                    break;
                }

                if (null != this.OnFileEnumerated)
                {
                    this.OnFileEnumerated(this, EventArgs.Empty);
                }

                if ((null != isOkToDelete) &&
                    (false == isOkToDelete(data.FolderInfo, file.Name)))
                {
                    continue;
                }

                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            if (null != OnPreFileDeletion)
                            {
                                OnPreFileDeletion(this, EventArgs.Empty);
                            }

                            FabricFile.Delete(file.FullName);

                            if (null != OnPostFileDeletion)
                            {
                                OnPostFileDeletion(this, EventArgs.Empty);
                            }
                        });
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Failed to delete file {0}.",
                        file.FullName);
                }
            }

            return true;
        }

        internal class FolderDeletionInfo
        {
            internal string FolderName { get; set; }

            internal object Context { get; set; }
        }

        private class SubFolderEnumerationData
        {
            internal DestinationOperationPerformer DestinationOperationPerformer { get; set; }

            internal FolderDeletionInfo FolderInfo { get; set; }

            internal Queue<FolderDeletionInfo> FolderQueue { get; set; }

            internal SetFolderDeletionInfoContext SetContext { get; set; }
        }

        private class DeleteFilesData
        {
            internal DestinationOperationPerformer DestinationOperationPerformer { get; set; }

            internal FolderDeletionInfo FolderInfo { get; set; }

            internal DateTime CutoffTime { get; set; }

            internal IsOkToDeleteFile IsOkToDelete { get; set; }
        }

        private class DirectoryDeletionInfo
        {
            internal string DirName { get; set; }

            internal DateTime CutoffTime { get; set; }
        }
    }
}
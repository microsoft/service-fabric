// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Fabric.Dca.FileUploader;

    // This class implements the logic to delete old files from a file share
    internal class FileShareTrimmer : IDisposable
    {
        // Constants
        private const string TraceType = "FileShareTrimmer";
        private const string OldLogDeletionTimerIdSuffix = "_OldFileDeletionTimer";

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;
        
        // Folder on the file share to which data needs to be uploaded
        private readonly string destinationPath;

        // Method invoked to perform operation on destination
        private readonly FolderTrimmer.DestinationOperationPerformer destinationOperationPerformer;

        // Local folder containing a map of files that we uploaded
        private readonly string localMap;

        // Object that measures performance
        private readonly FileSharePerformance perfHelper;

        // Helper objects used in deleting old files
        private readonly FolderTrimmer folderTrimmer;
        private readonly LocalMapTrimmingHelper localMapTrimmingHelper;

        // Timer to delete old logs
        private readonly DcaTimer oldLogDeletionTimer;

        // Whether or not the object has been disposed
        private bool disposed;

        internal FileShareTrimmer(
                    string folderName,
                    string destinationPath,
                    FolderTrimmer.DestinationOperationPerformer destinationOperationPerformer,
                    string localMap,
                    TimeSpan fileDeletionAge)
        {
            // Initialization
            this.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            this.logSourceId = string.Concat(TraceType, "-", Guid.NewGuid().ToString("P"));
            this.destinationPath = destinationPath;
            this.destinationOperationPerformer = destinationOperationPerformer;
            this.localMap = localMap;
            this.localMapTrimmingHelper = new LocalMapTrimmingHelper(folderName);
            this.folderTrimmer = new FolderTrimmer(
                                        this.traceSource,
                                        this.logSourceId,
                                        fileDeletionAge);

            this.perfHelper = new FileSharePerformance(this.traceSource, this.logSourceId);

            // File deletion is done one at a time, so the
            // concurrency count is 1.
            this.perfHelper.ExternalOperationInitialize(
                ExternalOperationTime.ExternalOperationType.FileShareDeletion,
                1);

            // Create a timer to delete old logs
            string timerId = string.Concat(
                          this.logSourceId,
                          OldLogDeletionTimerIdSuffix);
            var oldLogDeletionInterval = (fileDeletionAge < TimeSpan.FromDays(1)) ?
                FileShareUploaderConstants.OldLogDeletionIntervalForTest :
                FileShareUploaderConstants.OldLogDeletionInterval;
            this.oldLogDeletionTimer = new DcaTimer(
                                               timerId,
                                               this.DeleteOldFilesHandler,
                                               oldLogDeletionInterval);
            this.oldLogDeletionTimer.Start();
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            if (null != this.folderTrimmer)
            {
                this.folderTrimmer.Dispose();
            }

            if (null != this.oldLogDeletionTimer)
            {
                // Stop and dispose timer
                this.oldLogDeletionTimer.StopAndDispose();

                // Wait for timer callback to finish executing
                this.oldLogDeletionTimer.DisposedEvent.WaitOne();
            }

            GC.SuppressFinalize(this);
        }

        private void DeleteOldFilesHandler(object state)
        {
            this.folderTrimmer.OnFileEnumerated += this.OnFileEnumerated;
            this.folderTrimmer.OnFolderDeleted += this.OnFolderDeleted;
            this.folderTrimmer.OnFolderEnumerated += this.OnFolderEnumerated;
            this.folderTrimmer.OnPreFileDeletion += this.OnPreFileDeletion;
            this.folderTrimmer.OnPostFileDeletion += this.OnPostFileDeletion;

            // Delete old files from destination
            this.perfHelper.FileDeletionPassBegin();
            this.folderTrimmer.DeleteOldFilesFromFolder(
                this.destinationPath,
                null, 
                null,
                this.destinationOperationPerformer);
            this.perfHelper.FileDeletionPassEnd();

            this.folderTrimmer.OnFileEnumerated -= this.OnFileEnumerated;
            this.folderTrimmer.OnFolderDeleted -= this.OnFolderDeleted;
            this.folderTrimmer.OnFolderEnumerated -= this.OnFolderEnumerated;
            this.folderTrimmer.OnPreFileDeletion -= this.OnPreFileDeletion;
            this.folderTrimmer.OnPostFileDeletion -= this.OnPostFileDeletion;

            // Delete old files from local map
            this.folderTrimmer.DeleteOldFilesFromFolder(
                this.localMap,
                this.localMapTrimmingHelper.SetFolderDeletionContext,
                this.localMapTrimmingHelper.OkToDeleteFile,
                null);

            // Schedule the next pass
            this.oldLogDeletionTimer.Start();
        }

        private void OnFolderEnumerated(object sender, EventArgs e)
        {
            this.perfHelper.FolderEnumerated();
        }

        private void OnFolderDeleted(object sender, EventArgs e)
        {
            this.perfHelper.FolderDeleted();
        }

        private void OnFileEnumerated(object sender, EventArgs e)
        {
            this.perfHelper.FileEnumerated();
        }

        private void OnPreFileDeletion(object sender, EventArgs e)
        {
            this.perfHelper.ExternalOperationBegin(
                ExternalOperationTime.ExternalOperationType.FileShareDeletion,
                0);
        }

        private void OnPostFileDeletion(object sender, EventArgs e)
        {
            this.perfHelper.ExternalOperationEnd(
                ExternalOperationTime.ExternalOperationType.FileShareDeletion,
                0);
            this.perfHelper.FileDeleted();
        }
    }
}
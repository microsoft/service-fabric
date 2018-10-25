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

    internal class FileSharePerformance
    {
        // Constants
        private const string TraceType = FileShareUploaderConstants.PerformanceTraceType;

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Objects to keep track of how much time it takes to perform file share operations
        private readonly Dictionary<ExternalOperationTime.ExternalOperationType, ExternalOperationTime> externalOperationTime =
                           new Dictionary<ExternalOperationTime.ExternalOperationType, ExternalOperationTime>();

        // The beginning tick count for file upload pass
        private long fileUploadPassBeginTime;

        // Counter to count the number of files uploaded to file share
        private ulong filesUploaded;

        // The beginning tick count for file deletion pass
        private long fileDeletionPassBeginTime;

        // Counter to count the number of files enumerated
        private ulong filesEnumerated;

        // Counter to count the number of files deleted
        private ulong filesDeleted;

        // Counter to count the number of folders enumerated
        private ulong foldersEnumerated;

        // Counter to count the number of folders deleted
        private ulong foldersDeleted;

        internal FileSharePerformance(FabricEvents.ExtensionsEvents traceSource, string logSourceId)
        {
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;
        }

        internal void ExternalOperationInitialize(ExternalOperationTime.ExternalOperationType type, int concurrencyCount)
        {
            this.externalOperationTime[type] = new ExternalOperationTime(concurrencyCount);
        }

        internal void ExternalOperationBegin(ExternalOperationTime.ExternalOperationType type, int index)
        {
            this.externalOperationTime[type].OperationBegin(index);
        }

        internal void ExternalOperationEnd(ExternalOperationTime.ExternalOperationType type, int index)
        {
            this.externalOperationTime[type].OperationEnd(index);
        }

        internal void FileUploaded()
        {
            this.filesUploaded++;
        }

        internal void FileUploadPassBegin()
        {
            this.filesUploaded = 0;
            this.fileUploadPassBeginTime = DateTime.Now.Ticks;

            this.externalOperationTime[ExternalOperationTime.ExternalOperationType.FileShareCopy].Reset();
        }

        internal void FileUploadPassEnd()
        {
            long fileUploadPassEndTime = DateTime.Now.Ticks;
            double fileUploadPassTimeSeconds = ((double)(fileUploadPassEndTime - this.fileUploadPassBeginTime)) / (10 * 1000 * 1000);

            this.traceSource.WriteInfo(
                TraceType,
                "FileShareUploadPassInfo - {0}: {1},{2}", // WARNING: The performance test parses this message,
                                                          // so changing the message can impact the test.
                this.logSourceId,
                fileUploadPassTimeSeconds,
                this.filesUploaded);

            this.traceSource.WriteInfo(
                TraceType,
                "ExternalOperationFileShareCopy - {0}: {1},{2}", // WARNING: The performance test parses this message,
                                                                 // so changing the message can impact the test.
                this.logSourceId,
                this.externalOperationTime[ExternalOperationTime.ExternalOperationType.FileShareCopy].OperationCount,
                this.externalOperationTime[ExternalOperationTime.ExternalOperationType.FileShareCopy].CumulativeTimeSeconds);
        }

        internal void FileEnumerated()
        {
            this.filesEnumerated++;
        }

        internal void FileDeleted()
        {
            this.filesDeleted++;
        }

        internal void FolderEnumerated()
        {
            this.foldersEnumerated++;
        }

        internal void FolderDeleted()
        {
            this.foldersDeleted++;
        }

        internal void FileDeletionPassBegin()
        {
            this.filesEnumerated = 0;
            this.filesDeleted = 0;
            this.foldersEnumerated = 0;
            this.foldersDeleted = 0;
            this.fileDeletionPassBeginTime = DateTime.Now.Ticks;

            this.externalOperationTime[ExternalOperationTime.ExternalOperationType.FileShareDeletion].Reset();
        }

        internal void FileDeletionPassEnd()
        {
            long fileDeletionPassEndTime = DateTime.Now.Ticks;
            double fileDeletionPassTimeSeconds = ((double)(fileDeletionPassEndTime - this.fileDeletionPassBeginTime)) / (10 * 1000 * 1000);

            this.traceSource.WriteInfo(
                TraceType,
                "FileShareDeletionPassInfo - {0}: {1},{2},{3},{4},{5}", // WARNING: The performance test parses this message,
                                                                        // so changing the message can impact the test.
                this.logSourceId,
                fileDeletionPassTimeSeconds,
                this.filesEnumerated,
                this.filesDeleted,
                this.foldersEnumerated,
                this.foldersDeleted);

            this.traceSource.WriteInfo(
                TraceType,
                "ExternalOperationFileShareDeletion - {0}: {1},{2}", // WARNING: The performance test parses this message,
                                                                     // so changing the message can impact the test.
                this.logSourceId,
                this.externalOperationTime[ExternalOperationTime.ExternalOperationType.FileShareDeletion].OperationCount,
                this.externalOperationTime[ExternalOperationTime.ExternalOperationType.FileShareDeletion].CumulativeTimeSeconds);
        }
    }
}
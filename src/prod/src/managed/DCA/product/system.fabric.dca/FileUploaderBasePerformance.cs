// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.FileUploader 
{
    using System;
    using System.Fabric.Common.Tracing;

    internal class FileUploaderBasePerformance
    {
        // Constants
        private const string TraceType = "Performance";

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // The beginning tick count for file sync pass
        private long fileSyncPassBeginTime;

        // Counter to count the number of files checked for synchronization
        private ulong filesCheckedForSync;

        internal FileUploaderBasePerformance(FabricEvents.ExtensionsEvents traceSource, string logSourceId)
        {
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;
        }

        internal void FileCheckedForSync()
        {
            this.filesCheckedForSync++;
        }

        internal void FileSyncPassBegin()
        {
            this.filesCheckedForSync = 0;
            this.fileSyncPassBeginTime = DateTime.Now.Ticks;
        }

        internal void FileSyncPassEnd()
        {
            long fileSyncPassEndTime = DateTime.Now.Ticks;
            double fileSyncPassTimeSeconds = ((double)(fileSyncPassEndTime - this.fileSyncPassBeginTime)) / (10 * 1000 * 1000);

            this.traceSource.WriteInfo(
                TraceType,
                "FileShareSyncPassInfo - {0}: {1},{2}", // WARNING: The performance test parses this message,
                                                        // so changing the message can impact the test.
                this.logSourceId,
                fileSyncPassTimeSeconds,
                this.filesCheckedForSync);
        }
    }
}
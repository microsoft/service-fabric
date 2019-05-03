// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Backup log record.
    /// Contains information on the last completed backup.
    /// </summary>
    internal sealed class IncrementalBackupLogRecordAsyncEnumerator : IAsyncEnumerator<LogRecord>
    {
        private readonly string traceType;
        private readonly Guid backupId;

        private readonly BackupLogRecord lastBackupLogRecord;

        private readonly IBackupRestoreProvider loggingReplicator;

        private readonly IAsyncEnumerator<LogRecord> source;

        private uint count;

        private Epoch lastEpoch;

        private LogicalSequenceNumber lastLsn;

        private Epoch startingEpoch;

        private LogicalSequenceNumber startingLsn;

        public IncrementalBackupLogRecordAsyncEnumerator(
            string traceType,
            Guid backupId, 
            IAsyncEnumerator<LogRecord> source,
            BackupLogRecord lastBackupLogRecord,
            IBackupRestoreProvider loggingReplicator)
        {
            this.traceType = traceType;
            this.backupId = backupId;
            this.source = source;
            this.lastBackupLogRecord = lastBackupLogRecord;
            this.loggingReplicator = loggingReplicator;
            this.startingEpoch = LoggingReplicator.InvalidEpoch;
            this.startingLsn = LogicalSequenceNumber.InvalidLsn;
            this.lastEpoch = LoggingReplicator.InvalidEpoch;
            this.lastLsn = LogicalSequenceNumber.InvalidLsn;

            this.count = 0;
        }

        internal IncrementalBackupLogRecordAsyncEnumerator(
            string traceType,
            Guid backupId,
            IAsyncEnumerator<LogRecord> source,
            BackupLogRecord lastBackupLogRecord,
            Epoch startingEpoch,
            LogicalSequenceNumber startingLsn)
        {
            this.traceType = traceType;
            this.backupId = backupId;
            this.source = source;
            this.lastBackupLogRecord = lastBackupLogRecord;
            this.loggingReplicator = null;

            // TODO: verify startingEpoch & startingLsn is not invalid
            this.startingEpoch = startingEpoch;
            this.startingLsn = startingLsn;
            this.lastEpoch = LoggingReplicator.InvalidEpoch;
            this.lastLsn = LogicalSequenceNumber.InvalidLsn;

            this.count = 0;
        }

        public uint Count
        {
            get { return this.count; }
        }

        public LogRecord Current
        {
            get { return this.source.Current; }
        }

        public Epoch HighestBackedupEpoch
        {
            get { return this.lastEpoch; }
        }

        public LogicalSequenceNumber HighestBackedupLsn
        {
            get { return this.lastLsn; }
        }

        public Epoch StartingEpoch
        {
            get { return this.startingEpoch; }
        }

        public LogicalSequenceNumber StartingLsn
        {
            get { return this.startingLsn; }
        }

        public void Dispose()
        {
            if (this.source != null)
            {
                this.source.Dispose();
            }
        }

        public async Task<bool> MoveNextAsync(CancellationToken cancellationToken)
        {
            while (true)
            {
                if (await this.source.MoveNextAsync(cancellationToken).ConfigureAwait(false) == false)
                {
                    return false;
                }

                var logicalRecord = this.source.Current as LogicalLogRecord;
                if (logicalRecord == null)
                {
                    // In incremental backups, we do not have to copy physical log records.
                    continue;
                }

                // If the logical log record is already backed up skip it.
                if (this.source.Current.Lsn < this.lastBackupLogRecord.HighestBackedUpLsn)
                {
                    continue;
                }

                // UpdateEpoch is a logical log record that does not have a unique LSN.
                var updateEpochLogRecord = logicalRecord as UpdateEpochLogRecord;
                if (updateEpochLogRecord != null)
                {
                    // If the previous backup included this epoch, we can ignore it.
                    // If the previous backup included a higher epoch, it must have included this one or this one is stagger.
                    if (updateEpochLogRecord.Epoch.CompareTo(this.lastBackupLogRecord.HighestBackedUpEpoch) <= 0)
                    {
                        continue;
                    }

                    this.lastEpoch = updateEpochLogRecord.Epoch;
                }

                this.count++;
                this.lastLsn = logicalRecord.Lsn;

                if (this.startingLsn == LogicalSequenceNumber.InvalidLsn)
                {
                    this.startingLsn = logicalRecord.Lsn;
                    this.startingEpoch = this.loggingReplicator.GetEpoch(logicalRecord.Lsn);
                }

                return true;
            }
        }

        public void Reset()
        {
            this.source.Reset();
        }

        public async Task VerifyDrainedAsync()
        {
            bool hasMoreItems = await this.source.MoveNextAsync(CancellationToken.None).ConfigureAwait(false);

            // Checks multiple conditions inside since Assert to reduce boxing with high number of arguments.
            // 1. Stream must have been drained.
            // 2. The startingLSN must be >= HighestBackedUpLsn (Equal case is for backup stitching).
            // 3. Number of logical records backed up must be at least one (Backup log record & barrier).
            Utility.Assert(
                hasMoreItems == false && this.startingLsn >= this.lastBackupLogRecord.HighestBackedUpLsn && this.count > 0,
                "{0}: BackupId: {1} LastBackupRecord LSN: {2} PSN: {3} StartingLSN: HighestBackedupLSN: {5} Count: {6}. One of the conditions is invalid: isNotDrained == false && this.startingLsn >= this.lastBackupLogRecord.HighestBackedUpLsn && this.count > 0.", 
                this.traceType,
                this.backupId,
                this.lastBackupLogRecord.Lsn.LSN,
                this.lastBackupLogRecord.Psn.PSN,
                this.startingLsn, 
                this.lastBackupLogRecord.HighestBackedUpLsn,
                this.count);
        }
    }
}
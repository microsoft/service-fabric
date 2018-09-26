// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// An interface between BackupManager and LoggingReplicator (Implemented by LoggingReplicator)
    /// used by the Backup Manager to provide backup and restore functionality.
    /// </summary>
    internal interface IBackupRestoreProvider
    {
        // Properties used for Backup and Restore
        Epoch CurrentLogTailEpoch { get; }

        // Properties used for Restore
        LogicalSequenceNumber CurrentLogTailLsn { get; }

        // Properties used for Backup
        LogicalSequenceNumber LastStableLsn { get; }

        TransactionalReplicatorSettings ReplicatorSettings { get; }

        // Required for Backup
        void ThrowIfNoWriteStatus();

        Task<bool> AcquireBackupAndCopyConsistencyLockAsync(
            string lockTaker,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        void ReleaseBackupAndCopyConsistencyLock(string lockReleaser);

        Task<bool> AcquireStateManagerApiLockAsync(
            string lockTaker,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        void ReleaseStateManagerApiLock(string lockReleaser);

        void StartDrainableBackupPhase();

        void CompleteDrainableBackupPhase();

        Task ReplicateBackupLogRecordAsync(Guid backupId, ReplicatorBackup replicatorBackup);

        Task<IAsyncEnumerator<LogRecord>> GetLogRecordsAsync(
            LogicalSequenceNumber previousLogicalSequenceNumber,
            string readerName,
            LogManager.LogReaderType logReaderType);

        IAsyncEnumerator<LogRecord> GetLogRecordsCallerHoldsLock(
            IndexingLogRecord previousIndexingRecord,
            string readerName,
            LogManager.LogReaderType logReaderType);

        // Required for Restore
        Epoch GetEpoch(LogicalSequenceNumber lsn);

        Task PrepareForRestore(Guid restoreId);

        Task PrepareToCloseAsync();

        Task RecoverRestoredDataAsync(Guid restoreId, IList<string> backupLogPathList);

        Task BecomePrimaryAsync(bool isForRestore);

        Task CancelTheAbortingOfTransactionsIfNecessaryAsync();

        Task FinishRestoreAsync(Epoch dataLossEpoch);
    }
}
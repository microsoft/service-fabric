// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        interface IBackupManager
        {
            K_SHARED_INTERFACE(IBackupManager)

        public:
            /// <summary>
            /// Sets the last completed log record.
            /// </summary>
            virtual void SetLastCompletedBackupLogRecord(
                __in LogRecordLib::BackupLogRecord const & backupLogRecord) = 0;

            /// <summary>
            /// Gets the last completed log record.
            /// </summary>
            virtual LogRecordLib::BackupLogRecord::CSPtr GetLastCompletedBackupLogRecord() const = 0;

            /// <summary>
            /// Notification that the last completed backup log record has been false progressed.
            /// </summary>
            virtual void UndoLastCompletedBackupLogRecord() = 0;

            /// <summary>
            /// Return boolean indicating whether LoggingReplicator should delete its persisted state.
            /// </summary>
            /// <returns>Boolean indicating whether LoggingReplicator should delete its persisted state.</returns>
            virtual bool ShouldCleanState() const = 0;

            /// <summary>
            /// Informs backup that cleaning has been completed.
            /// </summary>
            virtual void StateCleaningComplete() = 0;

            virtual bool IsLogUnavailableDueToRestore() const noexcept = 0;

            virtual ktl::Awaitable<TxnReplicator::BackupInfo> BackupAsync(
                __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler) = 0;

            virtual ktl::Awaitable<TxnReplicator::BackupInfo> BackupAsync(
                __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler,
                __in FABRIC_BACKUP_OPTION backupOption,
                __in Common::TimeSpan const & timeout,
                __in ktl::CancellationToken const & cancellationToken) = 0;

            virtual ktl::Awaitable<void> RestoreAsync(
                __in KString const & backupFolder) = 0;

            virtual ktl::Awaitable<void> RestoreAsync(
                __in KString const & backupFolder,
                __in FABRIC_RESTORE_POLICY restorePolicy,
                __in Common::TimeSpan const & timeout,
                __in ktl::CancellationToken const & cancellationToken) = 0;

            virtual void EnableBackup() noexcept  = 0;
            virtual ktl::Awaitable<void> DisableBackupAndDrainAsync() = 0;

            virtual void EnableRestore() noexcept = 0;
            virtual void DisableRestore() noexcept = 0;

            virtual NTSTATUS DeletePersistedState() noexcept = 0;
        };
    }
}

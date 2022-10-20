// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    interface ILoggingReplicator 
        : public Data::Utilities::IDisposable
        , public ITransactionManager
        , public IVersionManager
    {
        K_SHARED_INTERFACE(ILoggingReplicator)
        K_WEAKREF_INTERFACE(ILoggingReplicator)

    public:
        typedef KDelegate<ktl::Awaitable<NTSTATUS>(FABRIC_REPLICA_ROLE, ktl::CancellationToken)> StateManagerBecomeSecondaryDelegate;

        __declspec(property(get = get_HasPersistedState)) bool HasPersistedState;
        virtual bool get_HasPersistedState() const = 0;

        virtual ktl::Awaitable<NTSTATUS> OpenAsync(__out RecoveryInformation & recoveryInformation) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> PerformRecoveryAsync(__in RecoveryInformation const & recoveryInformation) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> CloseAsync() noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> BecomeActiveSecondaryAsync(__in StateManagerBecomeSecondaryDelegate stateManagerCRDelegate) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> BecomeIdleSecondaryAsync() noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> BecomeNoneAsync() noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> BecomePrimaryAsync(bool isForRestore) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> DeleteLogAsync() noexcept = 0;

        virtual NTSTATUS GetSafeLsnToRemoveStateProvider(__out LONG64 & lsn) noexcept = 0;

        virtual bool IsReadable() noexcept = 0;

        virtual NTSTATUS GetLastStableSequenceNumber(__out LONG64 & lsn) noexcept = 0;

        virtual NTSTATUS GetLastCommittedSequenceNumber(__out LONG64 & lsn) noexcept = 0;

        virtual NTSTATUS GetCurrentEpoch(__out FABRIC_EPOCH & epoch) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> BackupAsync(
            __in IBackupCallbackHandler & backupCallbackHandler,
            __out BackupInfo & backupInfo) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> BackupAsync(
            __in IBackupCallbackHandler & backupCallbackHandler,
            __in FABRIC_BACKUP_OPTION backupOption,
            __in Common::TimeSpan const & timeout,
            __in ktl::CancellationToken const & cancellationToken,
            __out BackupInfo & backupInfo) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> RestoreAsync(
            __in KString const & backupFolder) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> RestoreAsync(
            __in KString const & backupFolder,
            __in FABRIC_RESTORE_POLICY restorePolicy,
            __in Common::TimeSpan const & timeout,
            __in ktl::CancellationToken const & cancellationToken) noexcept = 0;
 
    public: // Notification APIs
        virtual NTSTATUS RegisterTransactionChangeHandler(
            __in TxnReplicator::ITransactionChangeHandler & transactionChangeHandler) noexcept = 0;

        virtual NTSTATUS UnRegisterTransactionChangeHandler() noexcept = 0;

    public: // Test support
        virtual NTSTATUS Test_RequestCheckpointAfterNextTransaction() noexcept
        {
            CODING_ASSERT("Test_SetTestHookContext not implemented");
        }

        virtual NTSTATUS Test_GetPeriodicCheckpointAndTruncationTimestampTicks(
            __out LONG64 & lastPeriodicCheckpointTimeTicks,
            __out LONG64 & lastPeriodicTruncationTimeTicks) noexcept
        {
            UNREFERENCED_PARAMETER(lastPeriodicCheckpointTimeTicks);
            UNREFERENCED_PARAMETER(lastPeriodicTruncationTimeTicks);
            CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
        }
    };
}

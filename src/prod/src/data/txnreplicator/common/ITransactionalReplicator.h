// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "../../../../test/TestHooks/TestHooks.h"

namespace TxnReplicator
{
    interface ITransactionalReplicator : 
        public IStateProviderMap,
        public ITransactionManager,
        public IVersionManager
    {
        K_SHARED_INTERFACE(ITransactionalReplicator)
        K_WEAKREF_INTERFACE(ITransactionalReplicator)

    public:
        __declspec(property(get = get_IsReadable)) bool IsReadable;
        virtual bool get_IsReadable() const = 0;

        __declspec(property(get = get_StatefulPartition)) Data::Utilities::IStatefulPartition::SPtr StatefulPartition;
        virtual Data::Utilities::IStatefulPartition::SPtr get_StatefulPartition() const = 0;

        __declspec(property(get = get_HasPersistedState)) bool HasPeristedState;
        virtual bool get_HasPersistedState() const = 0;

        virtual NTSTATUS GetLastStableSequenceNumber(__out LONG64 & lsn) noexcept = 0;

        virtual NTSTATUS GetLastCommittedSequenceNumber(__out LONG64 & lsn) noexcept = 0;

        virtual NTSTATUS GetCurrentEpoch(__out FABRIC_EPOCH & epoch) noexcept = 0;

        virtual NTSTATUS RegisterTransactionChangeHandler(
            __in ITransactionChangeHandler & transactionChangeHandler) noexcept = 0;

        virtual NTSTATUS UnRegisterTransactionChangeHandler() noexcept = 0;

        virtual NTSTATUS RegisterStateManagerChangeHandler(
            __in IStateManagerChangeHandler & stateManagerChangeHandler) noexcept = 0;

        virtual NTSTATUS UnRegisterStateManagerChangeHandler() noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> BackupAsync(
            __in IBackupCallbackHandler & backupCallbackHandler,
            __out BackupInfo & result) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> BackupAsync(
            __in IBackupCallbackHandler & backupCallbackHandler,
            __in FABRIC_BACKUP_OPTION backupOption,
            __in Common::TimeSpan const & timeout,
            __in ktl::CancellationToken const & cancellationToken,
            __out BackupInfo & result) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> RestoreAsync(
            __in KString const & backupFolder) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> RestoreAsync(
            __in KString const & backupFolder,
            __in FABRIC_RESTORE_POLICY restorePolicy,
            __in Common::TimeSpan const & timeout,
            __in ktl::CancellationToken const & cancellationToken) noexcept = 0;

    public: // Test Support
        virtual NTSTATUS Test_RequestCheckpointAfterNextTransaction() noexcept
        {
            CODING_ASSERT("Test_RequestCheckpointAfterNextTransaction not implemented");
        }

        virtual void Test_SetTestHookContext(TestHooks::TestHookContext const &)
        {
            CODING_ASSERT("Test_SetTestHookContext not implemented");
        }

        virtual TestHooks::TestHookContext const & Test_GetTestHookContext()
        {
            CODING_ASSERT("Test_GetTestHookContext not implemented");
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

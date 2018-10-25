// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    // Interface: LoggingReplicator -> StateManager.
    // 
    // The interface LoggingReplicator uses to talk to the only State Provider it knowns: StateManager.
    interface IStateProviderManager
    {
        K_SHARED_INTERFACE(IStateProviderManager)
        K_WEAKREF_INTERFACE(IStateProviderManager)

    public:
        virtual  ktl::Awaitable<NTSTATUS> ApplyAsync(
            __in LONG64 logicalSequenceNumber,
            __in TransactionBase const & transactionBase,
            __in ApplyContext::Enum applyContext,
            __in_opt Data::Utilities::OperationData const * const metadataPtr,
            __in_opt Data::Utilities::OperationData const * const dataPtr,
            __out OperationContext::CSPtr & result) noexcept = 0;

        virtual NTSTATUS Unlock(__in OperationContext const & operationContext) noexcept = 0;

        virtual NTSTATUS PrepareCheckpoint(__in LONG64 checkpointLSN) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> PerformCheckpointAsync(
            __in ktl::CancellationToken const & cancellationToken) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> CompleteCheckpointAsync(
            __in ktl::CancellationToken const & cancellationToken) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> BackupCheckpointAsync(
            __in KString const & backupDirectory,
            __in ktl::CancellationToken const & cancellationToken) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> RestoreCheckpointAsync(
            __in KString const & backupDirectory,
            __in ktl::CancellationToken const & cancellationToken) noexcept = 0;

        virtual NTSTATUS GetCurrentState(
            __in FABRIC_REPLICA_ID replicaId,
            __out OperationDataStream::SPtr & result) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> BeginSettingCurrentStateAsync() noexcept = 0;
        
        /// This API will not get called if copy process gets aborted in the middle.
        virtual ktl::Awaitable<NTSTATUS> EndSettingCurrentState() noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> SetCurrentStateAsync(
            __in LONG64 stateRecordNumber,
            __in Data::Utilities::OperationData const & data) noexcept = 0;

        // This API is required for Restore.
        virtual ktl::Awaitable<void> ChangeRoleAsync(
            __in FABRIC_REPLICA_ROLE newRole,
            __in ktl::CancellationToken & cancellationToken) = 0;
    };
}

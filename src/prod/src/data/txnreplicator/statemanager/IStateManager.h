// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        interface IStateManager
            : public TxnReplicator::IStateProviderMap
            , public ITransactionStateManager
            , public TxnReplicator::IStateProviderManager
        {
            K_SHARED_INTERFACE(IStateManager)
            K_WEAKREF_INTERFACE(IStateManager)

        public: // Life cycle
            virtual void Initialize(
                __in TxnReplicator::ITransactionalReplicator & transactionalReplicator,
                __in TxnReplicator::ILoggingReplicator & loggingReplicator) = 0;

            virtual ktl::Awaitable<NTSTATUS> OpenAsync(
                __in bool completeCheckpoint,
                __in bool cleanupRestore,
                __in ktl::CancellationToken & cancellationToken) = 0;

            virtual ktl::Awaitable<void> ChangeRoleAsync(
                __in FABRIC_REPLICA_ROLE newRole,
                __in ktl::CancellationToken & cancellationToken) = 0;

            virtual ktl::Awaitable<NTSTATUS> CloseAsync(__in ktl::CancellationToken & cancellationToken) = 0;

        public: // Notification APIs
            virtual NTSTATUS RegisterStateManagerChangeHandler(
                __in TxnReplicator::IStateManagerChangeHandler & stateManagerChangeHandler) noexcept  = 0;

            virtual NTSTATUS UnRegisterStateManagerChangeHandler() noexcept = 0;
        };
    }
}

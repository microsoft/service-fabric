// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        interface ITransactionStateManager
        {
            K_SHARED_INTERFACE(ITransactionStateManager)
            K_WEAKREF_INTERFACE(ITransactionStateManager)

        public:
            virtual NTSTATUS SingleOperationTransactionAbortUnlock(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in TxnReplicator::OperationContext const & operationContext) noexcept = 0;

            virtual NTSTATUS ErrorIfStateProviderIsNotRegistered(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in LONG64 transactionId) noexcept = 0;

            // Api for single operation transaction optimization where begin, first operation and end is encoded in one API.
            virtual ktl::Awaitable<NTSTATUS> BeginTransactionAsync(
                __in TxnReplicator::Transaction & transaction,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const  undo,
                __in_opt Utilities::OperationData const * const  redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext,
                __out LONG64 & commitLsn) noexcept = 0;

            virtual NTSTATUS BeginTransaction(
                __in TxnReplicator::Transaction & transaction,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const  undo,
                __in_opt Utilities::OperationData const * const  redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext) = 0;

            virtual NTSTATUS AddOperation(
                __in TxnReplicator::Transaction & transaction,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const  undo,
                __in_opt Utilities::OperationData const * const  redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext) = 0;

            virtual ktl::Awaitable<NTSTATUS> AddOperationAsync(
                __in TxnReplicator::AtomicOperation & atomicOperation,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const  undo,
                __in_opt Utilities::OperationData const * const  redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext,
                __out LONG64 & commitLsn) noexcept = 0;

            virtual ktl::Awaitable<NTSTATUS> AddOperationAsync(
                __in TxnReplicator::AtomicRedoOperation & atomicRedoOperation,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const  redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext,
                __out LONG64 & commitLsn) noexcept = 0;
        };
    }
}

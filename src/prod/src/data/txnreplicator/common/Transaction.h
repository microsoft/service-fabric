// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    class Transaction
        : public TransactionBase
        , public ITransaction
    {
        K_FORCE_SHARED(Transaction)
        K_SHARED_INTERFACE_IMP(ITransaction)

        friend class LoggingReplicatorTests::TestTransaction;
        friend class TransactionBase;

    public:

        // Invoked when users create transactions
        static Transaction::SPtr CreateTransaction(
            __in ITransactionManager & replicator,
            __in KAllocator & allocator);

        //
        // Adds an operation in the context of this transaction
        //
        NTSTATUS AddOperation(
            __in_opt Data::Utilities::OperationData const * const metaData,
            __in_opt Data::Utilities::OperationData const * const undo,
            __in_opt Data::Utilities::OperationData const * const redo,
            __in LONG64 stateProviderId,
            __in_opt OperationContext const * const operationContext) noexcept;

        ktl::Awaitable<NTSTATUS> CommitAsync() noexcept override;
 
        ktl::Awaitable<NTSTATUS> CommitAsync(
            __in Common::TimeSpan const & timeout,
            __in ktl::CancellationToken const & cancellationToken) noexcept override;

        ktl::Awaitable<NTSTATUS> GetVisibilitySequenceNumberAsync(__out FABRIC_SEQUENCE_NUMBER & vsn) noexcept;

        NTSTATUS Abort() noexcept override;

        void AbortBadTransaction() noexcept;

        NTSTATUS AbortDuringDispose() noexcept;

        void Dispose() override;

        __declspec (property(get = get_CommitLatencyWatch)) Common::Stopwatch CommitLatencyWatch;
        Common::Stopwatch get_CommitLatencyWatch() const { return commitLatencyWatch_; }

    private:

        class FirstOperation : public KObject<FirstOperation>, public KShared<FirstOperation>
        {
            K_FORCE_SHARED(FirstOperation)

        public:

            static FirstOperation::CSPtr Create(
                __in_opt Data::Utilities::OperationData const * const metaData,
                __in_opt Data::Utilities::OperationData const * const undo,
                __in_opt Data::Utilities::OperationData const * const redo,
                __in LONG64 stateProviderId,
                __in_opt OperationContext const * const operationContext,
                __in KAllocator & allocator);

            __declspec(property(get = get_Metadata)) Data::Utilities::OperationData::CSPtr Metadata;
            Data::Utilities::OperationData::CSPtr get_Metadata() const
            {
                return metaData_;
            }

            __declspec(property(get = get_Undo)) Data::Utilities::OperationData::CSPtr Undo;
            Data::Utilities::OperationData::CSPtr get_Undo() const
            {
                return undo_;
            }

            __declspec(property(get = get_Redo)) Data::Utilities::OperationData::CSPtr Redo;
            Data::Utilities::OperationData::CSPtr get_Redo() const
            {
                return redo_;
            }

            __declspec(property(get = get_StateProdiverId)) LONG64 StateProdiverId;
            LONG64 get_StateProdiverId() const
            {
                return stateProviderId_;
            }

            __declspec(property(get = get_Context)) OperationContext::CSPtr Context;
            OperationContext::CSPtr get_Context() const
            {
                return context_;
            }

        private:

            FirstOperation(
                __in_opt Data::Utilities::OperationData const * const metaData,
                __in_opt Data::Utilities::OperationData const * const undo,
                __in_opt Data::Utilities::OperationData const * const redo,
                __in LONG64 stateProviderId,
                __in_opt OperationContext const * const operationContext);

            Data::Utilities::OperationData::CSPtr metaData_;
            Data::Utilities::OperationData::CSPtr redo_;
            Data::Utilities::OperationData::CSPtr undo_;
            OperationContext::CSPtr context_;
            LONG64 stateProviderId_;
        };

        Transaction(
            __in LONG64 transactionId,
            __in bool isPrimaryTransaction);

        Transaction(
            __in ITransactionManager & transactionalReplicator,
            __in LONG64 transactionId,
            __in bool isPrimaryTransaction);

        NTSTATUS PrivateAddOperation(
            __in_opt Data::Utilities::OperationData const * const metaData,
            __in_opt Data::Utilities::OperationData const * const undo,
            __in_opt Data::Utilities::OperationData const * const redo,
            __in LONG64 stateProviderId,
            __in_opt OperationContext const * const operationContext) noexcept;

        ktl::Awaitable<NTSTATUS> PrivateCommitAsync(
            __in Common::TimeSpan const & timeout,
            __in ktl::CancellationToken const & cancellationToken) noexcept;

        ktl::Awaitable<void> PrivateAbortTask() noexcept;

        ktl::Awaitable<NTSTATUS> AbortTransactionAsync() noexcept;

        ktl::Awaitable<NTSTATUS> PrivateGetVisibilitySequenceNumberAsync(__out FABRIC_SEQUENCE_NUMBER & lsn) noexcept;

        Common::atomic_bool isReplicatorAbortingBadTransaction_;

        LONG64 visibilityLsn_;

        FirstOperation::CSPtr firstOperation_;

        // Needed to unregister from version manager after commit/abort is done and the transaction is destructing
        KWeakRef<ITransactionManager>::SPtr transactionManagerForUnregistering_;

        Common::Stopwatch commitLatencyWatch_;
    };
}

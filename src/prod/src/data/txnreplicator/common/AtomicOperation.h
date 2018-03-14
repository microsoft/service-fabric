// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    //
    // Represents an atomic operation which is equivalent to a V1 replicator operation as there is no undo/rollback
    //
    class AtomicOperation 
        : public TransactionBase
    {
        K_FORCE_SHARED(AtomicOperation)

            friend class TransactionBase;

    public:

        //
        // Invoked when users create atomic operations
        //
        static AtomicOperation::SPtr CreateAtomicOperation(
            __in ITransactionManager & replicator,
            __in KAllocator & allocator);

        //
        // Test only method.
        //
        static AtomicOperation::SPtr CreateAtomicOperation(
            __in LONG64 transactionId,
            __in bool isPrimaryTransaction,
            __in KAllocator & allocator);

        //
        // The only operation that associated with an atomic operation.
        // If the API is invoked a second time on the same object, an invalid exception is thrown
        //
        ktl::Awaitable<NTSTATUS> AddOperationAsync(
            __in_opt Data::Utilities::OperationData const * const metaData,
            __in_opt Data::Utilities::OperationData const * const undo,
            __in_opt Data::Utilities::OperationData const * const redo,
            __in LONG64 stateProviderId,
            __in_opt OperationContext const * const operationContext) noexcept;

    private:

        AtomicOperation(
            __in LONG64 transactionId,
            __in bool isPrimaryTransaction);

        AtomicOperation(
            __in ITransactionManager & transactionalReplicator,
            __in LONG64 transactionId,
            __in bool isPrimaryTransaction);
    };
}

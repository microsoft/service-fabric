// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    //
    // Represents an atomic redo only operation which is equivalent to a V1 replicator operation as there is no undo/rollback
    // In addition, this operation provides a semantic that it can never be un-done. If ever this operation ends up being false progressed,
    // the primary will ensure that the state provider is never invoked to "undo" the effects of this operation. Instead, there is a full
    // copy.
    //
    class AtomicRedoOperation 
        : public TransactionBase
    {
        K_FORCE_SHARED(AtomicRedoOperation)

            friend class TransactionBase;

    public:

        //
        // Invoked when users create atomic redo operations
        //
        static AtomicRedoOperation::SPtr CreateAtomicRedoOperation(
            __in ITransactionManager & replicator,
            __in KAllocator & allocator);

        //
        // Invoked during recovery where replicator is re-creating atomic redo operations from disk
        //
        static AtomicRedoOperation::SPtr CreateAtomicRedoOperation(
            __in LONG64 transactionId,
            __in bool isPrimaryTransaction,
            __in KAllocator & allocator);
            
        //
        // The only operation that associated with an atomic operation.
        // If the API is invoked a second time on the same object, an invalid exception is thrown
        //
        ktl::Awaitable<NTSTATUS> AddOperationAsync(
            __in_opt Data::Utilities::OperationData const * const metaData,
            __in_opt Data::Utilities::OperationData const * const redo,
            __in LONG64 stateProviderId,
            __in_opt OperationContext const * const operationContext) noexcept;

    private:

        AtomicRedoOperation(
            __in LONG64 transactionId,
            __in bool isPrimaryTransaction);

        AtomicRedoOperation(
            __in ITransactionManager & replicator,
            __in LONG64 transactionId,
            __in bool isPrimaryTransaction);
    };
}

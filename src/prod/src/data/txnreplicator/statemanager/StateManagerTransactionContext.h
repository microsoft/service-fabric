// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        //
        // Txn::LockContext used by the StateManager
        // Contains information about the transaction, the lock that was taken and the type of the operation the lock was take for.
        // 
        // Use:
        // - Primary: LockContext is explicitly added through AddLockContext.
        // - Recovery && Secondary: Return from ApplyAsync.
        //
        // Note: Rename to TransactionLockContext. In managed they shared the namespace causing SM prefix. 
        class StateManagerTransactionContext final : public TxnReplicator::LockContext
        {
            K_FORCE_SHARED(StateManagerTransactionContext)

        public:
            //
            // Creates the StateManagerTransactionContext::SPtr.
            // 
            // Params:
            // transactionId            Id of the parent transaction.
            // stateManagerLockContext  Information on the lock that was taken for the operation.
            // operationType            Type of the operation for which the lock was taken.
            //
            static NTSTATUS Create(
                __in LONG64 transactionId,
                __in StateManagerLockContext & stateManagerLockContext,
                __in OperationType::Enum operationType,
                __in KAllocator & allocator,
                __out StateManagerTransactionContext::SPtr & result) noexcept;

            __declspec(property(get = get_TransactionId)) LONG64 TransactionId;
            LONG64 StateManagerTransactionContext::get_TransactionId() const
            {
                return transactionId_;
            }

            __declspec(property(get = get_LockContext)) StateManagerLockContext::SPtr LockContext;
            StateManagerLockContext::SPtr StateManagerTransactionContext::get_LockContext() const
            {
                return lockContextSPtr_;
            }

            __declspec(property(get = get_OperationType)) OperationType::Enum OperationType;
            OperationType::Enum StateManagerTransactionContext::get_OperationType() const
            {
                return operationType_;
            }

            //
            // Releases the lock and cleans up transient state.
            // 
            // Use: Called by the Logging Replicator when the lock needs to be released.
            // - Abort: Inflight transactions during P->!P, Undo False Progress, Abort.
            // - Commit.
            //
            void Unlock() override;
        
        private:
            // Transaction id.
            const LONG64 transactionId_;

            // Shared pointer to the information about the lock.
            const StateManagerLockContext::SPtr lockContextSPtr_;

            // Type of the operation that requested the lock.
            const OperationType::Enum operationType_;
            
            //
            // NOFAIL Constructor for StateManagerTransactionContext class.
            //
            // Parameters:
            // transactionId            Id of the parent transaction.
            // stateManagerLockContext  Information on the lock that was taken for the operation.
            // operationType            Type of the operation for which the lock was taken.
            //
            NOFAIL StateManagerTransactionContext(
                __in LONG64 transactionId,
                __in StateManagerLockContext & stateManagerLockContext,
                __in OperationType::Enum operationType);
        };
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    //
    // Base class for all types of transactions -
    //  1. Transaction
    //  2. AtomicOperation
    //  3. AtomicRedoOperation
    //
    class TransactionBase
        : public KObject<TransactionBase>
        , public KShared<TransactionBase>
        , public Data::Utilities::IDisposable
    {
        friend class LoggingReplicatorTests::TestTransaction;

        K_FORCE_SHARED_WITH_INHERITANCE(TransactionBase)
        K_SHARED_INTERFACE_IMP(IDisposable)

    public:

        // Invoked when replicator creates transaction/atomic operation from disk during recovery
        static TransactionBase::SPtr CreateTransaction(
            __in LONG64 transactionId,
            __in bool isPrimaryTransaction,
            __in KAllocator & allocator);

        static bool IsRetriableException(ktl::Exception const & e);
        static bool TransactionBase::IsRetriableNTSTATUS(
            __in NTSTATUS status) noexcept;

        bool operator==(TransactionBase const & other) const;
        bool operator>(TransactionBase const & other) const;
        bool operator<(TransactionBase const & other) const;
        bool operator>=(TransactionBase const & other) const;
        bool operator<=(TransactionBase const & other) const;
        bool operator!=(TransactionBase const & other) const;

        //
        // The transaction was created on a primary replica
        //
        __declspec(property(get = get_IsPrimaryTransaction)) bool IsPrimaryTransaction;
        bool get_IsPrimaryTransaction() const
        {
            return isPrimaryTransaction_;
        }

        __declspec(property(get = get_TransactionId)) LONG64 TransactionId;
        LONG64 get_TransactionId() const
        {
            return transactionId_;
        }

        __declspec(property(get = get_IsAtomicOperation)) bool IsAtomicOperation;
        bool get_IsAtomicOperation() const
        {
            return (transactionId_ < 0);
        }

        __declspec(property(get = get_IsValidTransaction)) bool IsValidTransaction;
        bool get_IsValidTransaction() const
        {
            return (transactionId_ != 0);
        }

        __declspec(property(get = get_State, put = set_State)) TransactionState::Enum StateValue;
        TransactionState::Enum get_State() const
        {
            return state_;
        }

        __declspec(property(get = get_CommitSequenceNumber, put = set_CommitSequenceNumber)) LONG64 CommitSequenceNumber;
        FABRIC_SEQUENCE_NUMBER get_CommitSequenceNumber() const
        {
            return commitLsn_;
        }
        void set_CommitSequenceNumber(__in FABRIC_SEQUENCE_NUMBER value);

        virtual void Dispose() override;

        __checkReturn NTSTATUS AddLockContext(__in LockContext & lockContext) noexcept;

    protected:

        static const ULONG MaxRetryDelayMilliseconds;
        static LONG64 CreateTransactionId();
        static NTSTATUS ErrorIfNotPrimaryTransaction(bool isPrimaryTransaction);

        TransactionBase(
            __in LONG64 transactionId,
            __in bool isPrimaryTransaction);

        TransactionBase(
            __in ITransactionManager & replicator,
            __in LONG64 transactionId,
            __in bool isPrimaryTransaction);

        __declspec(property(get = get_IsWriteTransaction, put = set_IsWriteTransaction)) bool IsWriteTransaction;
        bool get_IsWriteTransaction() const
        {
            return isWriteTransaction_;
        }
        void set_IsWriteTransaction(bool value)
        {
            isWriteTransaction_ = value;
        }

        __declspec(property(get = get_RetryDelay, put = set_RetryDelay)) ULONG RetryDelayMilliseconds;
        ULONG get_RetryDelay() const
        {
            return retryDelay_;
        }
        void set_RetryDelay(ULONG value)
        {
            if (value > MaxRetryDelayMilliseconds)
            {
                retryDelay_ = MaxRetryDelayMilliseconds;
            }
        }

        __declspec(property(get = get_IsDisposed)) bool IsDisposed;
        bool get_IsDisposed()
        {
            return isDisposed_;
        }

        __declspec(property(get = get_StateMachine)) KSharedPtr<TransactionStateMachine> StateMachine;
        KSharedPtr<TransactionStateMachine> get_StateMachine();

        __declspec(property(get = get_State)) TransactionState::Enum State;
        TransactionState::Enum get_State();

        NTSTATUS GetTransactionManager(__out ITransactionManager::SPtr & result);

        void set_State(TransactionState::Enum value)
        {
            state_ = value;
        }

    private:
        static LONG64 lastTransactionId_;
        static KSpinLock transactionIdLock_;

        LONG64 const transactionId_;
        bool const isPrimaryTransaction_;

        KArray<LockContext::SPtr> lockContexts_;
        KWeakRef<ITransactionManager>::SPtr transactionManager_;
        KSpinLock transactionManagerLock_;

        Data::Utilities::ThreadSafeSPtrCache<TransactionStateMachine> stateMachine_;

        TransactionState::Enum state_;
        LONG64 commitLsn_;
        bool isWriteTransaction_;
        ULONG retryDelay_;

        // Flag set to true upon Dispose.  Used to make Dispose idempotent.
        bool isDisposed_;
    };
}

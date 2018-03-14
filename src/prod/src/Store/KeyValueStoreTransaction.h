// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class KeyValueStoreTransaction 
        : public Api::ITransaction
        , public Common::ComponentRoot
        , public Common::TextTraceComponent<Common::TraceTaskCodes::KeyValueStore>
    {
        DENY_COPY(KeyValueStoreTransaction);

    public:
        KeyValueStoreTransaction(
            Common::Guid const & transactionId,
            FABRIC_TRANSACTION_ISOLATION_LEVEL const isolationLevel,
            IStoreBase::TransactionSPtr const & txreplicatedStoreTx,
            DeadlockDetectorSPtr const &);
        virtual ~KeyValueStoreTransaction();

        inline IStoreBase::TransactionSPtr const & get_ReplicatedStoreTransaction() const
        {
            return this->replicatedStoreTx_;
        }

        //
        // ITransactionBase methods
        // 
        Common::Guid get_Id();

        FABRIC_TRANSACTION_ISOLATION_LEVEL get_IsolationLevel();

        //
        // ITransaction methods
        // 
        Common::AsyncOperationSPtr BeginCommit(
            Common::TimeSpan const & timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndCommit(
            Common::AsyncOperationSPtr const & asyncOperation, 
            __out ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber);

        void Rollback();

    private:
        Common::Guid const id_;
        FABRIC_TRANSACTION_ISOLATION_LEVEL isolationLevel_;
        IStoreBase::TransactionSPtr replicatedStoreTx_;
        DeadlockDetectorSPtr deadlockDetector_;
    };
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    typedef Data::TStore::IStoreTransaction<KString::SPtr, KBuffer::SPtr> IKvsTransaction; 

    class TSTransaction 
        : public IStoreBase::TransactionBase
        , public Common::ComponentRoot
        , public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
        , protected TSComponent
    {
        DENY_COPY(TSTransaction)

    private:
        TSTransaction(
            Store::PartitionedReplicaId const &,
            TxnReplicator::Transaction::SPtr &&,
            IKvsTransaction::SPtr &&);

    public:
        static std::shared_ptr<TSTransaction> Create(
            Store::PartitionedReplicaId const &,
            TxnReplicator::Transaction::SPtr &&,
            IKvsTransaction::SPtr &&);
        virtual ~TSTransaction();

        __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
        std::wstring const & get_TraceId() const { return traceId_; }

        TxnReplicator::Transaction::SPtr const & GetInnerTransaction() { return innerTx_; }
        IKvsTransaction::SPtr const & GetTStoreTransaction() { return storeTx_; }

    protected:

        //
        // IStoreBase::TransactionBase
        //
        
        virtual Common::AsyncOperationSPtr BeginCommit(
            __in Common::TimeSpan const timeout,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & state) override;

        virtual Common::ErrorCode EndCommit(
            __in Common::AsyncOperationSPtr const & operation,
            __out ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber) override;

        virtual Common::ErrorCode Commit(
            __out ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber,
            __in Common::TimeSpan const timeout = Common::TimeSpan::MaxValue) override;

        virtual void Rollback() override;

    protected:

        //
        // TSComponent
        //

        Common::StringLiteral const & GetTraceComponent() const override;
        std::wstring const & GetTraceId() const override;

    private:
        
        class CommitAsyncOperation;

        ktl::Awaitable<Common::ErrorCode> CommitAsync(Common::TimeSpan const);
        Common::ErrorCode GetCommitLsn(__out FABRIC_SEQUENCE_NUMBER & lsn);

        TxnReplicator::Transaction::SPtr innerTx_;
        IKvsTransaction::SPtr storeTx_;
        std::wstring traceId_;
        Common::atomic_bool isActive_;
    };
}

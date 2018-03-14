// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace Data;
using namespace ktl;
using namespace ServiceModel;
using namespace std;
using namespace Store;

StringLiteral const TraceComponent("TSTransaction");

class TSTransaction::CommitAsyncOperation : public TimedAsyncOperation
{
public:

    CommitAsyncOperation(
        TSTransaction & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent) 
        //
        // TStore stack doesn't always honor CommitAsync timeout
        // (e.g. when replication is blocked)
        //
        : TimedAsyncOperation(timeout, callback, parent) 
        , owner_(owner)
        , lsnResult_(0)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out FABRIC_SEQUENCE_NUMBER & result)
    {
        auto casted = AsyncOperation::End<CommitAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            result = casted->lsnResult_;
        }

        return casted->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        if (owner_.isActive_.exchange(false))
        {
            this->CommitTask(thisSPtr);
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::StoreTransactionNotActive);
        }
    }

private:

    ktl::Task CommitTask(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);

        auto txRoot = owner_.CreateComponentRoot();;
        auto selfRoot = thisSPtr;

        auto error = co_await owner_.CommitAsync(this->RemainingTime);

        if (error.IsSuccess())
        {
            error = owner_.GetCommitLsn(lsnResult_);
        }

        // Do not TryComplete in a coroutine. This will call back into a thread
        // that may use SyncAwait(), which can cause deadlocks when used on a 
        // KTL thread.
        //
        Threadpool::Post([this, txRoot, selfRoot, error] { this->TryComplete(selfRoot, error); });

        co_return;
    }

    TSTransaction & owner_;
    FABRIC_SEQUENCE_NUMBER lsnResult_;
};

//
// TSTransaction
//

TSTransaction::TSTransaction(
    ::Store::PartitionedReplicaId const & partitionedReplicaId,
    TxnReplicator::Transaction::SPtr && innerTx,
    IKvsTransaction::SPtr && storeTx)
    : PartitionedReplicaTraceComponent(partitionedReplicaId)
    , TSComponent()
    , innerTx_(move(innerTx))
    , storeTx_(move(storeTx))
    , isActive_(true)
    , traceId_()
{
    traceId_ = wformatString("[{0}+{1}]", 
        PartitionedReplicaTraceComponent::TraceId,
        innerTx_->TransactionId);

    WriteNoise(
        TraceComponent,
        "{0} ctor",
        this->TraceId);
}

TSTransaction::~TSTransaction() 
{ 
    WriteNoise(
        TraceComponent,
        "{0} ~dtor",
        this->TraceId);

    this->Rollback();

    innerTx_->Dispose();
}

shared_ptr<TSTransaction> TSTransaction::Create(
    ::Store::PartitionedReplicaId const & partitionedReplicaId,
    TxnReplicator::Transaction::SPtr && innerTx,
    IKvsTransaction::SPtr && storeTx)
{
    return shared_ptr<TSTransaction>(new TSTransaction(partitionedReplicaId, move(innerTx), move(storeTx)));
}

AsyncOperationSPtr TSTransaction::BeginCommit(
    __in TimeSpan const timeout,
    __in AsyncCallback const & callback,
    __in AsyncOperationSPtr const & state)
{
    return AsyncOperation::CreateAndStart<CommitAsyncOperation>(*this, timeout, callback, state);
}

ErrorCode TSTransaction::EndCommit(
    __in AsyncOperationSPtr const & operation,
    __out ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber)
{
    auto error = CommitAsyncOperation::End(operation, commitSequenceNumber);

    WriteNoise(
        TraceComponent,
        "{0} committed: lsn={1} error={2}",
        this->TraceId,
        commitSequenceNumber,
        error);

    return error;
}

ErrorCode TSTransaction::Commit(
    __out ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber,
    __in TimeSpan const timeout)
{
    ManualResetEvent event(false);

    auto operation = this->BeginCommit(
        timeout,
        [&event](AsyncOperationSPtr const &) { event.Set(); },
        this->CreateAsyncOperationRoot());

    event.WaitOne();

    return this->EndCommit(operation, commitSequenceNumber);
}

void TSTransaction::Rollback()
{
    if (isActive_.exchange(false))
    {
        WriteNoise(
            TraceComponent,
            "{0} rolled back",
            this->TraceId);

        innerTx_->Abort();
    }
}

ktl::Awaitable<ErrorCode> TSTransaction::CommitAsync(TimeSpan const timeout)
{
    NTSTATUS status = co_await innerTx_->CommitAsync(timeout, CancellationToken::None);
    co_return this->FromNtStatus("Commitasync", status);
}

ErrorCode TSTransaction::GetCommitLsn(__out FABRIC_SEQUENCE_NUMBER & lsn)
{
    lsn = innerTx_->CommitSequenceNumber;
    return ErrorCodeValue::Success;
}

StringLiteral const & TSTransaction::GetTraceComponent() const
{
    return TraceComponent;
}

wstring const & TSTransaction::GetTraceId() const
{
    return this->TraceId;
}

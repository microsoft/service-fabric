// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Store;
using namespace Management::HealthManager;

StringLiteral const TraceComponent("HMReplicatedStore");

// ************************************
// Helper class for asynchronous commit
// ************************************

class ReplicatedStoreWrapper::CommitTxAsyncOperation : public AsyncOperation
{
public:
    CommitTxAsyncOperation(
        Store::ReplicaActivityId const & replicaActivityId,
        PrepareTransactionCallback const & prepareTxCallback,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root);
    ~CommitTxAsyncOperation();

    static ErrorCode End(
        AsyncOperationSPtr const & operation);

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr);

private:
    void OnCommitComplete(
        AsyncOperationSPtr const & operation, 
        bool expectedCompletedSynchronously);

    void ProcessResult(
        AsyncOperationSPtr const & thisSPtr, 
        ErrorCode && error);

    PrepareTransactionCallback prepareTxCallback_;
    IStoreBase::TransactionSPtr tx_;
    // The activityId of the parent operation that triggered this commit
    Store::ReplicaActivityId const & replicaActivityId_;
};

ReplicatedStoreWrapper::ReplicatedStoreWrapper(
    __in Store::IReplicatedStore * store,
    Store::PartitionedReplicaId const & partitionedReplicaId)
    : store_(store)
    , partitionedReplicaId_(partitionedReplicaId)
{
}

ReplicatedStoreWrapper::~ReplicatedStoreWrapper()
{
}

void ReplicatedStoreWrapper::SetThrottleCallback(Store::ThrottleCallback const & throttleCallback)
{
    auto error = store_->SetThrottleCallback(throttleCallback);
    if (!error.IsSuccess())
    {
        if (!error.IsError(ErrorCodeValue::InvalidState))
        {
            TRACE_LEVEL_AND_TESTASSERT(
                HealthManagerReplica::WriteWarning,
                TraceComponent,
                "{0}: SetThrottleCallback returned error {1}",
                partitionedReplicaId_,
                error);
        }
        else
        {
            HealthManagerReplica::WriteInfo(TraceComponent, "{0}: SetThrottleCallback returned InvalidState.", partitionedReplicaId_);
        }
    }
}

StoreTransaction ReplicatedStoreWrapper::CreateTransaction(
    ActivityId const & activityId)
{
    return StoreTransaction::Create(store_, partitionedReplicaId_, activityId);
}

Common::ErrorCode ReplicatedStoreWrapper::CreateTransaction(
    ActivityId const & activityId,
    __out IStoreBase::TransactionSPtr & tx)
{
    return store_->CreateTransaction(activityId, tx);
}

Common::ErrorCode ReplicatedStoreWrapper::CreateSimpleTransaction(
    ActivityId const & activityId,
    __out IStoreBase::TransactionSPtr & tx)
{
    return store_->CreateSimpleTransaction(activityId, tx);
}

Common::ErrorCode ReplicatedStoreWrapper::ReadExact(
    Common::ActivityId const & activityId,
    __in Store::StoreData & storeData)
{
    auto tx = CreateTransaction(activityId);
    auto error = tx.ReadExact(storeData);
    tx.CommitReadOnly();
    return error;
}

Common::ErrorCode ReplicatedStoreWrapper::Insert(
    __in IStoreBase::TransactionSPtr & tx,
    Store::StoreData const & storeData) const
{
    ASSERT_IFNOT(tx, "{0}: Insert: tx should exist", storeData.TraceId);
    
    // TODO: consider using the following to avoid copying when replicated store supports it
  	// FabricSerializer::Serialize(IFabricSerializable const * serializable, IFabricSerializableStreamUPtr & streamUPtr ...)
    KBuffer::SPtr bytes;
    NTSTATUS status = KBuffer::Create(0, bytes, Common::GetSFDefaultPagedKAllocator());
    if (!NT_SUCCESS(status))
    {
        return ErrorCode::FromNtStatus(status);
    }

    status = FabricSerializer::Serialize(&storeData, *bytes);
    if (!NT_SUCCESS(status))
    {
        return ErrorCode::FromNtStatus(status);
    }

    auto error = store_->Insert(
        tx, 
        storeData.Type, 
        storeData.Key, 
        bytes->GetBuffer(),
        bytes->QuerySize());
    return error;
}

Common::ErrorCode ReplicatedStoreWrapper::Update(
    __in IStoreBase::TransactionSPtr & tx,
    Store::StoreData const & storeData) const
{
    ASSERT_IFNOT(tx, "{0}: Update: tx should exist", storeData.TraceId);
    KBuffer::SPtr bytes;
    NTSTATUS status = KBuffer::Create(0, bytes, Common::GetSFDefaultPagedKAllocator());
    if (!NT_SUCCESS(status))
    {
        return ErrorCode::FromNtStatus(status);
    }

    status = FabricSerializer::Serialize(&storeData, *bytes);
    if (!NT_SUCCESS(status))
    {
        return ErrorCode::FromNtStatus(status);
    }

    auto error = store_->Update(
        tx, 
        storeData.Type, 
        storeData.Key, 
        0, /*ignoreSequenceNumberCheck*/
        storeData.Key, 
        bytes->GetBuffer(),
        bytes->QuerySize());
    return error;
}

Common::ErrorCode ReplicatedStoreWrapper::Delete(
    __in IStoreBase::TransactionSPtr & tx,
    Store::StoreData const & storeData) const
{
    ASSERT_IFNOT(tx, "{0}: Delete: tx should exist", storeData.TraceId);
    
    return store_->Delete(
        tx, 
        storeData.Type, 
        storeData.Key, 
        0 /*ignoreSequenceNumberCheck*/);
}

Common::AsyncOperationSPtr ReplicatedStoreWrapper::BeginCommit(
    Store::ReplicaActivityId const & replicaActivityId,
    PrepareTransactionCallback const & prepareTxCallback,
    Common::AsyncCallback const & callback, 
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CommitTxAsyncOperation>(
        replicaActivityId,
        prepareTxCallback,
        callback,
        parent);
}

Common::ErrorCode ReplicatedStoreWrapper::EndCommit(
    Common::AsyncOperationSPtr const & operation)
{
    return CommitTxAsyncOperation::End(operation);
}

bool ReplicatedStoreWrapper::ShowsInconsistencyBetweenMemoryAndStore(Common::ErrorCode const & error)
{
    switch (error.ReadValue())
    {
    // HM keeps in memory data that should always be in sync with the store data
    // If store conflict happens, it means there is a bug in HM memory state
    case ErrorCodeValue::StoreWriteConflict:
        return true;

    default:
        return false;
    }
}

// ************************************
// Implementation for helper class 
// ************************************

ReplicatedStoreWrapper::CommitTxAsyncOperation::CommitTxAsyncOperation(
    Store::ReplicaActivityId const & replicaActivityId,
    PrepareTransactionCallback const & prepareTxCallback,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : AsyncOperation(callback, root)
    , prepareTxCallback_(prepareTxCallback)
    , tx_()
    , replicaActivityId_(replicaActivityId)
{
}

ReplicatedStoreWrapper::CommitTxAsyncOperation::~CommitTxAsyncOperation()
{
}

ErrorCode ReplicatedStoreWrapper::CommitTxAsyncOperation::End(
    AsyncOperationSPtr const & operation)
{
    auto casted = AsyncOperation::End<CommitTxAsyncOperation>(operation);
    return casted->Error;
}

void ReplicatedStoreWrapper::CommitTxAsyncOperation::OnStart(
    AsyncOperationSPtr const & thisSPtr)
{
    auto error = prepareTxCallback_(replicaActivityId_, tx_);
    if (error.IsSuccess())
    {
        auto operation = tx_->BeginCommit(
            TimeSpan::MaxValue,
            [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
            thisSPtr);
        this->OnCommitComplete(operation, true);
    }
    else
    {
        if (tx_)
        {
            tx_->Rollback();
            tx_.reset();
        }

        ProcessResult(thisSPtr, move(error));
    }
}

void ReplicatedStoreWrapper::CommitTxAsyncOperation::OnCommitComplete(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (expectedCompletedSynchronously != operation->CompletedSynchronously)
    {
        return;
    }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    auto error = tx_->EndCommit(operation);
    tx_.reset();
    ProcessResult(thisSPtr, move(error));
}

void ReplicatedStoreWrapper::CommitTxAsyncOperation::ProcessResult(
    AsyncOperationSPtr const & thisSPtr, 
    ErrorCode && error)
{
    if (!error.IsSuccess())
    {
        HMEvents::Trace->CommitFailed(replicaActivityId_.TraceId, error, error.Message);
    }

    this->TryComplete(thisSPtr, move(error));
}


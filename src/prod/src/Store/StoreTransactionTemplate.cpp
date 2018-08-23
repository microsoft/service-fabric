// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Store;

StringLiteral const TraceComponent("StoreTransaction");

// ************************************
// Helper class for asynchronous commit
// ************************************

template <class TReplicatedStore>
class StoreTransactionTemplate<TReplicatedStore>::CommitAsyncOperation : public AsyncOperation
{
public:

    CommitAsyncOperation(
        StoreTransactionTemplate<TReplicatedStore> && storeTx,
        __in StoreData & storeData,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : AsyncOperation(callback, root)
        , storeTx_(move(storeTx))
        , storeDataHolder_(make_unique<StoreDataHolder>(storeData))
        , timeoutHelper_(timeout)
    {
    }

    CommitAsyncOperation(
        StoreTransactionTemplate<TReplicatedStore> && storeTx,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : AsyncOperation(callback, root)
        , storeTx_(move(storeTx))
        , storeDataHolder_(nullptr)
        , timeoutHelper_(timeout)
    {
    }

    ~CommitAsyncOperation()
    {
        if (!this->Error.IsSuccess())
        {
            storeTx_.Rollback();
        }
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CommitAsyncOperation>(operation)->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (!storeTx_.StoreError.IsSuccess()) 
        { 
            this->TryComplete(thisSPtr, storeTx_.StoreError);
        }
        else
        {
            auto operation = storeTx_.Transaction->BeginCommit(
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
                thisSPtr);
            this->OnCommitComplete(operation, true);
        }
    }


private:

    // Wrapper against the reference of the store data object.
    // Some commit operations may want to pass this object to update some information on it.
    // Others do not pass in any store data, so we need the wrapper to show that the reference
    // is not set and not used.
    //
    struct StoreDataHolder
    {
    private:
        DENY_COPY(StoreDataHolder)
    public:
        explicit StoreDataHolder(__in StoreData & storeData) : StoreDataObj(storeData) {}
        StoreData & StoreDataObj;
    };

    void OnCommitComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        int64 operationLSN = -1;
        ErrorCode error = storeTx_.Transaction->EndCommit(operation, operationLSN);

        if (error.IsSuccess() && storeDataHolder_ && operationLSN > 0)
        {
            storeDataHolder_->StoreDataObj.SetSequenceNumber(operationLSN);
        }

        this->TryComplete(operation->Parent, error);
    }

    StoreTransactionTemplate<TReplicatedStore> storeTx_;
    std::unique_ptr<StoreDataHolder> storeDataHolder_;
    TimeoutHelper timeoutHelper_;
};

// ************************
// StoreTransactionTemplate
// ************************

template <class TReplicatedStore>
StoreTransactionTemplate<TReplicatedStore>::StoreTransactionTemplate(Store::ReplicaActivityId const & rid)
    : ReplicaActivityTraceComponent(rid)
    , store_(nullptr)
    , txSPtr_()
    , storeError_(ErrorCodeValue::InvalidState)
    , isReadOnly_()
{
}

template <class TReplicatedStore>
StoreTransactionTemplate<TReplicatedStore>::StoreTransactionTemplate(StoreTransactionTemplate && other)
    : ReplicaActivityTraceComponent(move(other))
    , store_(move(other.store_))
    , txSPtr_(move(other.txSPtr_))
    , storeError_(other.storeError_)
    , isReadOnly_(other.isReadOnly_)
{
}

template <class TReplicatedStore>
StoreTransactionTemplate<TReplicatedStore> & StoreTransactionTemplate<TReplicatedStore>::operator=(
    StoreTransactionTemplate && other)
{
    if (this != &other)
    {
        ReplicaActivityTraceComponent::operator=(move(other));

        store_ = move(other.store_);
        txSPtr_ = move(other.txSPtr_);
        storeError_ = move(other.storeError_);
        isReadOnly_ = move(other.isReadOnly_);
    }

    return *this;
}

template <class TReplicatedStore>
StoreTransactionTemplate<TReplicatedStore>::StoreTransactionTemplate(
    __in TReplicatedStore * store,
    bool isValid,
    bool isSimpleTransaction,
    Store::PartitionedReplicaId const & partitionedReplicaId,
    Common::ActivityId const & activityId)
    : ReplicaActivityTraceComponent(partitionedReplicaId, activityId)
    , store_(store)
    , txSPtr_(nullptr)
    , storeError_(ErrorCodeValue::InvalidState)
    , isReadOnly_(true)
{
    if (isValid)
    {
        if(isSimpleTransaction)
        {
            storeError_ = store_->CreateSimpleTransaction(this->ActivityId, txSPtr_);            
        }
        else
        {
            storeError_ = store_->CreateTransaction(this->ActivityId, txSPtr_);
        }
    }
}

template <class TReplicatedStore>
StoreTransactionTemplate<TReplicatedStore> StoreTransactionTemplate<TReplicatedStore>::CreateInvalid(
    __in TReplicatedStore * store,
    __in Store::PartitionedReplicaId const & partitionedReplicaId)
{
    return StoreTransactionTemplate(store, false, false, partitionedReplicaId, Common::ActivityId());
}

template <class TReplicatedStore>
StoreTransactionTemplate<TReplicatedStore> StoreTransactionTemplate<TReplicatedStore>::Create(
    __in TReplicatedStore * store,
    __in Store::PartitionedReplicaId const & partitionedReplicaId,
    Common::ActivityId const & activityId)
{
    return StoreTransactionTemplate(store, true, false, partitionedReplicaId, activityId);
}

template <class TReplicatedStore>
StoreTransactionTemplate<TReplicatedStore> StoreTransactionTemplate<TReplicatedStore>::CreateSimpleTransaction(
    __in TReplicatedStore * store,
    __in Store::PartitionedReplicaId const & partitionedReplicaId,
    Common::ActivityId const & activityId)
{
    return StoreTransactionTemplate(store, true, true, partitionedReplicaId, activityId);
}

template <class TReplicatedStore>
ErrorCode StoreTransactionTemplate<TReplicatedStore>::Insert(StoreData const & storeData) const
{
    if (!storeError_.IsSuccess()) { return storeError_; }

    isReadOnly_ = false;

    KBuffer::SPtr bufferSPtr;
    ErrorCode error = FabricSerializer::Serialize(&storeData, bufferSPtr);

    if (error.IsSuccess())
    {
        WriteNoise(
            TraceComponent, 
            "{0} Insert({1}, {2})", 
            this->TraceId, 
            storeData.Type, 
            storeData.Key);

        error = this->OnInsert(storeData, *bufferSPtr);
    }

    return error;
}

template <class TReplicatedStore>
ErrorCode StoreTransactionTemplate<TReplicatedStore>::Update(StoreData const & storeData, bool checkSequenceNumber) const
{
    if (!storeError_.IsSuccess()) { return storeError_; }

    isReadOnly_ = false;

    KBuffer::SPtr bufferSPtr;
    ErrorCode error = FabricSerializer::Serialize(&storeData, bufferSPtr);

    if (error.IsSuccess())
    {
        WriteNoise(
            TraceComponent, 
            "{0} Update({1}, {2}, {3}, {4})", 
            this->TraceId, 
            storeData.Type, 
            storeData.Key, 
            storeData.SequenceNumber, 
            checkSequenceNumber);

        error = this->OnUpdate(
            storeData,
            (checkSequenceNumber ? storeData.SequenceNumber : ILocalStore::SequenceNumberIgnore),
            *bufferSPtr);
    }

    return error;
}

template <class TReplicatedStore>
ErrorCode StoreTransactionTemplate<TReplicatedStore>::Delete(StoreData const & storeData, bool checkSequenceNumber) const
{
    if (!storeError_.IsSuccess()) { return storeError_; }

    isReadOnly_ = false;

    WriteNoise(
        TraceComponent, 
        "{0} Delete({1}, {2}, {3}, {4})", 
        this->TraceId, 
        storeData.Type, 
        storeData.Key, 
        storeData.SequenceNumber,
        checkSequenceNumber);

    return this->OnDelete(
        storeData,
        (checkSequenceNumber ? storeData.SequenceNumber : ILocalStore::SequenceNumberIgnore));
}

template <class TReplicatedStore>
ErrorCode StoreTransactionTemplate<TReplicatedStore>::Exists(
    StoreData const & data) const
{
    return InternalReadExact(data.Type, data.Key, nullptr);
}

template <class TReplicatedStore>
ErrorCode StoreTransactionTemplate<TReplicatedStore>::ReadExact(
    __inout StoreData & result) const
{
    return InternalReadExact(result.Type, result.Key, &result);
}

template <class TReplicatedStore>
ErrorCode StoreTransactionTemplate<TReplicatedStore>::InternalReadExact(
    std::wstring const & type,
    std::wstring const & key,
    __inout StoreData * result) const
{
    if (!storeError_.IsSuccess()) { return storeError_; }

    vector<byte> buffer;
    __int64 operationLsn = -1;

    auto error = this->OnReadExact(type, key, buffer, operationLsn);
    if (!error.IsSuccess()) { return error; } 

    if (result != nullptr)
    {
        error = FabricSerializer::Deserialize(*result, buffer);

        if (error.IsSuccess())
        {
            if (operationLsn < 0)
            {
                TRACE_ERROR_AND_TESTASSERT(
                    TraceComponent, 
                    "{0} Invalid LSN during InternalReadExact", 
                    this->TraceId);
            }

            result->SetSequenceNumber(operationLsn);
            result->ReInitializeTracing(this->ReplicaActivityId);
        }
    }

    return error;
}

template <class TReplicatedStore>
ErrorCode StoreTransactionTemplate<TReplicatedStore>::TryReadOrInsertIfNotFound(__inout StoreData & result, __out bool & readExisting) const
{
    if (!storeError_.IsSuccess()) { return storeError_; }

    ErrorCode error = this->ReadExact(result);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        error = this->Insert(result);
        readExisting = false;
    }
    else if (error.IsSuccess())
    {
        readExisting = true;
    }

    return error;
}

template <class TReplicatedStore>
ErrorCode StoreTransactionTemplate<TReplicatedStore>::InsertIfNotFound(__in StoreData & data, __out bool & inserted) const
{
    if (!storeError_.IsSuccess()) { return storeError_; }

    ErrorCode error = this->Exists(data);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        error = this->Insert(data);
        inserted = true;
    }
    else
    {
        inserted = false;
    }

    return error;
}

template <class TReplicatedStore>
ErrorCode StoreTransactionTemplate<TReplicatedStore>::InsertIfNotFound(__in StoreData & data) const
{
    bool unused;
    return this->InsertIfNotFound(data, unused);
}

template <class TReplicatedStore>
ErrorCode StoreTransactionTemplate<TReplicatedStore>::DeleteIfFound(__in StoreData & data) const
{
    if (!storeError_.IsSuccess()) { return storeError_; }

    ErrorCode error = this->Exists(data);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        error = ErrorCodeValue::Success;
    }
    else
    {
        error = this->Delete(data, false);
    }

    return error;
}

template <class TReplicatedStore>
ErrorCode StoreTransactionTemplate<TReplicatedStore>::UpdateOrInsertIfNotFound(__in StoreData & data) const
{
    bool inserted;
    ErrorCode error = this->InsertIfNotFound(data, inserted);

    if (error.IsSuccess() && !inserted)
    {
        error = this->Update(data, false);
    }

    return error;
}

template <class TReplicatedStore>
void StoreTransactionTemplate<TReplicatedStore>::CommitReadOnly() const
{
    if (txSPtr_) 
    { 
        if (!isReadOnly_) 
        { 
            TRACE_INFO_AND_TESTASSERT(TraceComponent, "{0} transaction is not read only", this->TraceId); 
        }
        else
        {
            // TSTransaction commit calls into the KTL stack, which can schedule coroutine
            // continuations on the work queue of the calling thread before releasing the thread
            // back to the caller. i.e. A "synchronous" completion can just be queued work on
            // the calling thread. This means that blocking a thread that has called into 
            // a coroutine on completion of the coroutine itself can deadlock.
            //
            // Rollback when committing a readonly transaction instead of Commit() to avoid
            // the WaitOne() in Commit().
            //
            txSPtr_->Rollback();
        }
    }
}

template <class TReplicatedStore>
void StoreTransactionTemplate<TReplicatedStore>::Rollback() const
{
    if (txSPtr_) 
    { 
        txSPtr_->Rollback();
    }
}

template <class TReplicatedStore>
AsyncOperationSPtr StoreTransactionTemplate<TReplicatedStore>::BeginCommit(
    StoreTransactionTemplate<TReplicatedStore> && storeTx,
    TimeSpan const timeout, 
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & root)
{
    return AsyncOperation::CreateAndStart<CommitAsyncOperation>(
        move(storeTx),
        timeout,
        callback,
        root);
}

template <class TReplicatedStore>
AsyncOperationSPtr StoreTransactionTemplate<TReplicatedStore>::BeginCommit(
    StoreTransactionTemplate<TReplicatedStore> && storeTx,
    __in StoreData & storeData,
    TimeSpan const timeout, 
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & root)
{
    return AsyncOperation::CreateAndStart<CommitAsyncOperation>(
        move(storeTx),
        storeData,
        timeout,
        callback,
        root);
}

template <class TReplicatedStore>
AsyncOperationSPtr StoreTransactionTemplate<TReplicatedStore>::BeginCommit(
    StoreTransactionTemplate<TReplicatedStore> && storeTx,
    __in StoreData & storeData,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & root)
{
    return BeginCommit(
        move(storeTx),
        storeData,
        TimeSpan::MaxValue,
        callback,
        root);
}

template <class TReplicatedStore>
ErrorCode StoreTransactionTemplate<TReplicatedStore>::EndCommit(AsyncOperationSPtr const & operation)
{
    return CommitAsyncOperation::End(operation);
}

template <class TReplicatedStore>
TimeSpan StoreTransactionTemplate<TReplicatedStore>::GetRandomizedOperationRetryDelay(
    ErrorCode const & error,
    TimeSpan const maxDelay)
{
    switch (error.ReadValue())
    {
        case ErrorCodeValue::StoreWriteConflict:
        case ErrorCodeValue::StoreSequenceNumberCheckFailed:
        {
            double r = static_cast<double>(rand());
            double retryDelayMillis = (r / RAND_MAX) * maxDelay.TotalMilliseconds();

            return TimeSpan::FromMilliseconds(retryDelayMillis);
        }

        default:
            return maxDelay;
    }
}

// *** Specializations

template <>
ErrorCode StoreTransactionTemplate<ReplicatedStore>::OnInsert(
    StoreData const & storeData,
    __in KBuffer & buffer) const
{
    return store_->InternalInsert(
        txSPtr_, 
        storeData.Type,
        storeData.Key,
        buffer.GetBuffer(),
        buffer.QuerySize());
}

template <>
ErrorCode StoreTransactionTemplate<IReplicatedStore>::OnInsert(
    StoreData const & storeData,
    __in KBuffer & buffer) const
{
    return store_->Insert(
        txSPtr_, 
        storeData.Type,
        storeData.Key,
        buffer.GetBuffer(),
        buffer.QuerySize());
}

template <>
ErrorCode StoreTransactionTemplate<ReplicatedStore>::OnUpdate(
    StoreData const & storeData,
    ::FABRIC_SEQUENCE_NUMBER checkSequenceNumber,
    __in KBuffer & buffer) const
{
    return store_->InternalUpdate(
        txSPtr_, 
        storeData.Type,
        storeData.Key,
        checkSequenceNumber,
        storeData.Key,
        buffer.GetBuffer(),
        buffer.QuerySize());
}

template <>
ErrorCode StoreTransactionTemplate<IReplicatedStore>::OnUpdate(
    StoreData const & storeData,
    ::FABRIC_SEQUENCE_NUMBER checkSequenceNumber,
    __in KBuffer & buffer) const
{
    return store_->Update(
        txSPtr_, 
        storeData.Type,
        storeData.Key,
        checkSequenceNumber,
        storeData.Key,
        buffer.GetBuffer(),
        buffer.QuerySize());
}

template <>
ErrorCode StoreTransactionTemplate<ReplicatedStore>::OnDelete(
    StoreData const & storeData,
    ::FABRIC_SEQUENCE_NUMBER checkSequenceNumber) const
{
    return store_->InternalDelete(
        txSPtr_, 
        storeData.Type,
        storeData.Key,
        checkSequenceNumber);
}

template <>
ErrorCode StoreTransactionTemplate<IReplicatedStore>::OnDelete(
    StoreData const & storeData,
    ::FABRIC_SEQUENCE_NUMBER checkSequenceNumber) const
{
    return store_->Delete(
        txSPtr_, 
        storeData.Type,
        storeData.Key,
        checkSequenceNumber);
}

template<>
ErrorCode StoreTransactionTemplate<ReplicatedStore>::OnCreateEnumeration(
    std::wstring const & type,
    std::wstring const & key,
    __out IStoreBase::EnumerationSPtr & enumSPtr) const
{
    auto casted = dynamic_pointer_cast<ReplicatedStore::Transaction>(txSPtr_);
    if (!casted)
    {
        TRACE_ERROR_AND_TESTASSERT(
            TraceComponent,
            "{0} failed to cast ReplicatedStore::Transaction",
            this->TraceId);

        return ErrorCodeValue::InvalidState;
    }

    IStoreBase::TransactionSPtr innerTxSPtr;
    auto error = casted->TryGetInnerTransaction(innerTxSPtr);

    if (error.IsSuccess())
    {
        error = store_->LocalStore->CreateEnumerationByTypeAndKey(
            innerTxSPtr,
            type, 
            key, 
            enumSPtr);
    }

    return error;
}

template<>
ErrorCode StoreTransactionTemplate<IReplicatedStore>::OnCreateEnumeration(
    std::wstring const & type,
    std::wstring const & key,
    __out IStoreBase::EnumerationSPtr & enumSPtr) const
{
    return store_->CreateEnumerationByTypeAndKey(
        txSPtr_, 
        type, 
        key, 
        enumSPtr);
}

template<>
ErrorCode StoreTransactionTemplate<ReplicatedStore>::OnReadExact(
    std::wstring const & type,
    std::wstring const & key,
    __out vector<byte> & buffer,
    __out __int64 & operationLsn) const
{

    return store_->InternalReadExact(txSPtr_, type, key, buffer, operationLsn);
}

template<>
ErrorCode StoreTransactionTemplate<IReplicatedStore>::OnReadExact(
    std::wstring const & type,
    std::wstring const & key,
    __out vector<byte> & buffer,
    __out __int64 & operationLsn) const
{

    return store_->ReadExact(txSPtr_, type, key, buffer, operationLsn);
}

template class Store::StoreTransactionTemplate<ReplicatedStore>;
template class Store::StoreTransactionTemplate<IReplicatedStore>;

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Transport;
    using namespace Federation;
    using namespace Store;

    StringLiteral const TraceComponent("NamingStore");

    // *************
    // Inner classes
    // *************

    class NamingStore::CommitAsyncOperation : public AsyncOperation
    {
    public:
        CommitAsyncOperation(
            TransactionSPtr && txSPtr,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & root)
            : AsyncOperation(callback, root)
            , txSPtr_(move(txSPtr))
            , timeoutHelper_(timeout)
            , lsn_(0)
        {
        }

        void OnStart(AsyncOperationSPtr const &);

        static ErrorCode End(AsyncOperationSPtr const &);

        static ErrorCode End(AsyncOperationSPtr const &, __out ::FABRIC_SEQUENCE_NUMBER &);

    private:
        void OnCommitComplete(AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        TransactionSPtr txSPtr_;
        TimeoutHelper timeoutHelper_;
        FABRIC_SEQUENCE_NUMBER lsn_;
    };

    // ***********
    // NamingStore
    // ***********

    NamingStore::NamingStore(
        __in Store::IReplicatedStore & replicatedStore, 
        __in StoreServiceProperties & properties,
        Common::Guid const & partitionId,
        FABRIC_REPLICA_ID replicaId)
      : iReplicatedStore_(replicatedStore)
      , traceId_()
      , properties_(properties)
      , partitionId_(partitionId)
      , replicaId_(replicaId)
      , operationQueues_()
      , operationQueueLock_()
      , operationMsgNotifier_()
      , operationMsgNotifierLock_()
    {
        StringWriter writer(traceId_);
        writer.Write("[{0}:{1}]", partitionId_, replicaId_);
    }

    bool NamingStore::IsRepairable(ErrorCode const & error)
    {
        return !IsCancellable(error);
    }

    bool NamingStore::IsCancellable(ErrorCode const & error)
    {
        switch (error.ReadValue())
        {
        case ErrorCodeValue::NotPrimary:
            // During change role, the Reliability layer may continue to 
            // return NotPrimary until the new role has been committed.
            // Keep retrying operations in this case.
            //
            return !iReplicatedStore_.GetIsActivePrimary();

        case ErrorCodeValue::ObjectClosed:
        case ErrorCodeValue::InvalidState:  // returned from the replicator
            return true;

        default:
            return false;
        }
    }

    bool NamingStore::NeedsReversion(Common::ErrorCode const & error)
    {
        return NamingErrorCategories::ShouldRevertCreateService(error);
    }

    ErrorCode NamingStore::ToRemoteError(Common::ErrorCode && error)
    {
        switch (error.ReadValue())
        {
            // Convert these local error codes to an error that can be handled 
            // appropriately by the remote caller 
            // (i.e. Naming Gateway or Authority Owner)
            // 
            case ErrorCodeValue::ObjectClosed:
            case ErrorCodeValue::InvalidState:
                return ErrorCode(ErrorCodeValue::ReconfigurationPending, error.TakeMessage());

            default:
                return move(error);
        }
    }

    ErrorCode NamingStore::DeleteFromStore(
        TransactionSPtr const & txSPtr,
        std::wstring const & storeType,
        std::wstring storeKey,
        _int64 currentSequenceNumber)
    {
        ErrorCode error = iReplicatedStore_.Delete(txSPtr, storeType, storeKey, currentSequenceNumber);

        WriteNoise(
            TraceComponent,
            "{0} deleted data item {1}:{2} ({3}): error = {4}", 
            this->TraceId,
            storeType, 
            storeKey, 
            currentSequenceNumber,
            error);

        return error;
    }        

    ErrorCode NamingStore::TryGetCurrentSequenceNumber(
        TransactionSPtr const & txSPtr, 
        wstring const & type, 
        wstring const & key, 
        __out _int64 & result)
    {
        // TODO: Optimize with GetSequenceNumber API from v2 stack
        //
        vector<byte> unusedBuffer;
        return iReplicatedStore_.ReadExact(txSPtr, type, key, unusedBuffer, result);
    }

    ErrorCode NamingStore::TryGetCurrentSequenceNumber(
        TransactionSPtr const & txSPtr, 
        wstring const & type, 
        wstring const & key, 
        __out _int64 & result,
        __out EnumerationSPtr & enumSPtr)
    {
        ErrorCode error = iReplicatedStore_.CreateEnumerationByTypeAndKey(txSPtr, type, key, enumSPtr);

        if (error.IsSuccess())
        {
            error = TrySeekToDataItem(type, key, enumSPtr);
        }

        if (error.IsSuccess())
        {
            error = enumSPtr->CurrentOperationLSN(result);
        }

        if (error.IsSuccess())
        {
            WriteNoise(
                TraceComponent,
                "{0} returning {1} as the current sequence number for data item {2}:{3}",
                TraceId,
                result,
                type,
                key);
        }
        // NotFound is already traced by TrySeekToDataItem()
        else if (!error.IsError(ErrorCodeValue::NotFound))
        {
            WriteInfo(
                TraceComponent,
                "{0} reading current sequence number failed for data item {1}:{2}.  Error={3}",
                TraceId,
                type,
                key,
                error);
        }

        return error;
    }

    ErrorCode NamingStore::GetCurrentProgress(__out _int64 & lsn)
    {
        return iReplicatedStore_.GetLastCommittedSequenceNumber(lsn);
    }

    ErrorCode NamingStore::TrySeekToDataItem(
        wstring const & type, 
        wstring const & key, 
        EnumerationSPtr const & enumSPtr)
    {
        ErrorCode error = enumSPtr->MoveNext();
        if (!error.IsSuccess())
        {
            WriteNoise(
                TraceComponent,
                "{0} enumerating data item {1}:{2} failed with {3}", 
                this->TraceId,
                type, 
                key, 
                error);

            return (error.IsError(ErrorCodeValue::EnumerationCompleted) ? ErrorCodeValue::NotFound : error);
        }

        wstring currentKey;
        if (!(error = enumSPtr->CurrentKey(currentKey)).IsSuccess())
        {
            return error;
        }

        if (currentKey != key)
        {
            WriteNoise(
                TraceComponent,
                "{0} data item {1}:{2} not found",
                this->TraceId,
                type, 
                key);

            return ErrorCodeValue::NotFound;
        }

        return error;
    }

    ErrorCode NamingStore::CreateTransaction(FabricActivityHeader const & activityHeader, TransactionSPtr & tx)
    {
        return iReplicatedStore_.CreateTransaction(activityHeader.ActivityId, tx);
    }

    ErrorCode NamingStore::CreateEnumeration(
        TransactionSPtr const & txSPtr,
        wstring const & storeType,
        wstring const & storeName,
        EnumerationSPtr & enumSPtr)
    {
        return iReplicatedStore_.CreateEnumerationByTypeAndKey(txSPtr, storeType, storeName, enumSPtr);
    }

    AsyncOperationSPtr NamingStore::BeginAcquireNamedLockHelper(
        NamingUri const & name,
        bool const isForceDelete,
        wstring const & traceId,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & operation)
    {
        properties_.Trace.AcquireNamedLockStart(traceId, (int64)static_cast<void*>(operation.get()), name);

        AsyncExclusiveLockSPtr lockSPtr;
        {
            AcquireReadLock rLock(operationQueueLock_);
            auto searchIter = operationQueues_.find(name);
            if (searchIter != operationQueues_.end())
            {
                lockSPtr = searchIter->second;
                lockSPtr->IncrementPendingOperations();
                // Forceful delete has the highest priority
                if (isForceDelete)
                {
                    AcquireWriteLock wOpNotifierLock(operationMsgNotifierLock_);
                    operationMsgNotifier_[name] = NotificationEnum::ConvertToForceDelete;
                }
            }
        }

        if (lockSPtr == nullptr)
        {
            AcquireWriteLock wOpQueueLock(operationQueueLock_);
            auto searchIter = operationQueues_.find(name);
            if (searchIter == operationQueues_.end())
            {
                // no lock, create it
                lockSPtr = make_shared<AsyncExclusiveLock>();
                operationQueues_[name] = lockSPtr;
            }
            else
            {
                lockSPtr = searchIter->second;
                // Forceful delete has highest priority
                if (isForceDelete)
                {
                    AcquireWriteLock wOpNotifierLock(operationMsgNotifierLock_);
                    operationMsgNotifier_[name] = NotificationEnum::ConvertToForceDelete;
                }
            }
            lockSPtr->IncrementPendingOperations();
        }

        return lockSPtr->BeginAcquire(timeout, callback, operation);
    }

    AsyncOperationSPtr NamingStore::BeginAcquireNamedLock(
        NamingUri const & name,
        wstring const & traceId,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & operation)
    {
       return this->BeginAcquireNamedLockHelper(
            name,
            false,
            traceId,
            timeout,
            callback,
            operation);
    }

    ErrorCode NamingStore::EndAcquireNamedLock(
        NamingUri const & name, 
        wstring const & traceId,
        AsyncOperationSPtr const & operation)
    {   
        AsyncExclusiveLockSPtr lockSPtr;
        {
            AcquireReadLock lock(operationQueueLock_);
            auto searchIter = operationQueues_.find(name);
            ASSERT_IF(
                searchIter == operationQueues_.end(),
                "{0}: Operation {1} - Lock not found for name {2}",
                traceId, 
                static_cast<void*>(operation->Parent.get()),
                name);
            lockSPtr = searchIter->second;
        }

        ErrorCode error = lockSPtr->EndAcquire(operation);
        if (error.IsSuccess())
        {
            properties_.Trace.AcquireNamedLockSuccess(traceId, (int64)static_cast<void*>(operation->Parent.get()), name);
        }
        else
        {            
            properties_.Trace.AcquireNamedLockFailed(traceId, (int64)static_cast<void*>(operation->Parent.get()), name, error);
            AcquireWriteLock lock(operationQueueLock_);
            lockSPtr->DecrementPendingOperations();
        }

        return error;
    }

    AsyncOperationSPtr NamingStore::BeginAcquireDeleteServiceNamedLock(
        NamingUri const & name,
        bool const isForceDelete,
        wstring const & traceId,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & operation)
    {
        return this->BeginAcquireNamedLockHelper(
            name,
            isForceDelete,
            traceId,
            timeout,
            callback,
            operation);
    }

    ErrorCode NamingStore::EndAcquireDeleteServiceNamedLock(
        NamingUri const & name,
        wstring const & traceId,
        AsyncOperationSPtr const & operation)
    {
        return EndAcquireNamedLock(
            name,
            traceId,
            operation);
    }

    void NamingStore::ReleaseNamedLock(
        NamingUri const & name, 
        wstring const & traceId,
        AsyncOperationSPtr const & operation)
    {        
        properties_.Trace.ReleasedNamedLock(traceId, (int64)static_cast<void*>(operation.get()), name);

        AsyncExclusiveLockSPtr lockSPtr;
        {
            AcquireWriteLock lock(operationQueueLock_);
            auto lockMapIter = operationQueues_.find(name);
            if (lockMapIter == operationQueues_.end())
            {
                Assert::CodingError(
                    "{0}: Operation {1} - No lock asssociated with name {2}",
                    traceId,
                    static_cast<void*>(operation.get()),
                    name);
            }

            lockSPtr = lockMapIter->second;
            if (lockSPtr->DecrementPendingOperations() == 0)
            {
                // no other async operation is trying to get the lock
                properties_.Trace.DeleteNamedLock(traceId, (int64)static_cast<void*>(operation.get()));

                operationQueues_.erase(name);
                AcquireWriteLock wLock(operationMsgNotifierLock_);
                if (operationMsgNotifier_.find(name) != operationMsgNotifier_.end())
                {
                    operationMsgNotifier_.erase(name);
                }
            }
        }
        
        lockSPtr->Release(operation);
    }

    void NamingStore::ReleaseDeleteServiceNamedLock(
        NamingUri const & name,
        wstring const & traceId,
        AsyncOperationSPtr const & operation)
    {
        this->ReleaseNamedLock(
            name,
            traceId,
            operation);
    }

    bool NamingStore::ShouldForceDelete(
        NamingUri const & name)
    {
        AcquireReadLock rLock(operationMsgNotifierLock_);
        auto iter = operationMsgNotifier_.find(name);
        return iter != operationMsgNotifier_.end() && iter->second == NotificationEnum::ConvertToForceDelete;
    }

    ErrorCode NamingStore::ReadProperty(
        NamingUri const & namingUri,
        TransactionSPtr const & txSPtr,
        NamePropertyOperationType::Enum operationType,
        wstring const & storeType,
        wstring const & storeKey,
        size_t operationIndex,
        __out NamePropertyResult & result)
    {
        vector<byte> buffer;
        _int64 sequenceNumber;
        FILETIME lastModifiedUtc;
        auto error = iReplicatedStore_.ReadExact(txSPtr, storeType, storeKey, buffer, sequenceNumber, lastModifiedUtc);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent, 
                "{0} {1}: ReadExact failed: type={2} key={3} error={4}",  
                this->TraceId, 
                operationType,
                storeType, 
                storeKey, 
                error);
            return error;
        }

        NamePropertyStoreData nameProperty;
        error = FabricSerializer::Deserialize(nameProperty, buffer);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent, 
                "{0} {1}: Deserialize failed: type={2} key={3} error={4}",  
                this->TraceId, 
                operationType,
                storeType, 
                storeKey, 
                error);
            return error;
        }

        result = NamePropertyResult(
            NamePropertyMetadataResult(namingUri, nameProperty.TakeMetadata(), DateTime(lastModifiedUtc), sequenceNumber, operationIndex),
            nameProperty.TakeBytes());

        return error;
    }

    ErrorCode NamingStore::ReadProperty(
        NamingUri const & namingUri,
        EnumerationSPtr const & enumSPtr,
        NamePropertyOperationType::Enum operationType,
        size_t operationIndex,
        __out NamePropertyResult & result)
    {
        NamePropertyStoreData nameProperty;
        ErrorCode error = this->ReadCurrentData<NamePropertyStoreData>(enumSPtr, nameProperty);
        if (!error.IsSuccess())
        {
            WriteInfo(TraceComponent, "{0} {1}: ReadCurrentData returned error {2}",  this->TraceId, operationType, error);
            return error;
        }

        FILETIME lastModifiedUtc;
        error = enumSPtr->CurrentLastModifiedFILETIME(lastModifiedUtc);
        if (!error.IsSuccess())
        {
            WriteInfo(TraceComponent, "{0} {1}: CurrentLastModifiedFILETIME returns error {2}", this->TraceId, operationType, error);
            return error;
        }

        ::FABRIC_SEQUENCE_NUMBER sequenceNumber;
        error = enumSPtr->CurrentOperationLSN(sequenceNumber);
        if (!error.IsSuccess())
        {
            WriteInfo(TraceComponent, "{0} {1}: CurrentSequenceNumber returns error {2}",  this->TraceId, operationType, error);
            return error;
        }

        result = NamePropertyResult(
            NamePropertyMetadataResult(namingUri, nameProperty.TakeMetadata(), DateTime(lastModifiedUtc), sequenceNumber, operationIndex),
            nameProperty.TakeBytes());
        return error;
    }

    AsyncOperationSPtr NamingStore::BeginCommit(
        TransactionSPtr && txSPtr, 
        TimeSpan const & timeout, 
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & root)
    {
        return AsyncOperation::CreateAndStart<CommitAsyncOperation>(move(txSPtr), timeout, callback, root);
    }

    ErrorCode NamingStore::EndCommit(AsyncOperationSPtr const & operation)
    {
        return CommitAsyncOperation::End(operation);
    }

    ErrorCode NamingStore::EndCommit(AsyncOperationSPtr const & operation, __out FABRIC_SEQUENCE_NUMBER & lsn)
    {
        return CommitAsyncOperation::End(operation, lsn);
    }

    void NamingStore::CommitReadOnly(TransactionSPtr && txSPtr)
    {
        if (txSPtr)
        {
            txSPtr->Commit();
        }
    }

    // ******************
    // Template functions
    // ******************

    template ErrorCode NamingStore::TryWriteData(TransactionSPtr const &, HierarchyNameData const &, wstring const &, wstring const &, _int64);
    template ErrorCode NamingStore::TryWriteData(TransactionSPtr const &, NameData const &, wstring const &, wstring const &, _int64);
    template ErrorCode NamingStore::TryWriteData(TransactionSPtr const &, NamePropertyStoreData const &, wstring const &, wstring const &, _int64);
    template ErrorCode NamingStore::TryWriteData(TransactionSPtr const &, PartitionedServiceDescriptor const &, wstring const &, wstring const &, _int64);
    template ErrorCode NamingStore::TryWriteData(TransactionSPtr const &, ServiceUpdateDescription const &, wstring const &, wstring const &, _int64);

    template ErrorCode NamingStore::TryWriteDataAndGetCurrentSequenceNumber(TransactionSPtr const &, HierarchyNameData const &, wstring const &, wstring const &, __inout _int64 &);
    template ErrorCode NamingStore::TryWriteDataAndGetCurrentSequenceNumber(TransactionSPtr const &, NameData const &, wstring const &, wstring const &, __inout _int64 &);
    template ErrorCode NamingStore::TryWriteDataAndGetCurrentSequenceNumber(TransactionSPtr const &, NamePropertyStoreData const &, wstring const &, wstring const &, __inout _int64 &);
    template ErrorCode NamingStore::TryWriteDataAndGetCurrentSequenceNumber(TransactionSPtr const &, PartitionedServiceDescriptor const &, wstring const &, wstring const &, __inout _int64 &);
    template ErrorCode NamingStore::TryWriteDataAndGetCurrentSequenceNumber(TransactionSPtr const &, ServiceUpdateDescription const &, wstring const &, wstring const &, __inout _int64 &);

    template <class T>
    ErrorCode NamingStore::TryWriteData(
        TransactionSPtr const & txSPtr,
        T const & data,
        std::wstring const & type,
        std::wstring const & key,
        _int64 currentSequenceNumber)
    {
        return this->TryWriteDataAndGetCurrentSequenceNumber(
            txSPtr,
            data,
            type,
            key,
            currentSequenceNumber);
    }

    template <class T>
    ErrorCode NamingStore::TryWriteDataAndGetCurrentSequenceNumber(
        TransactionSPtr const & txSPtr,
        T const & data,
        std::wstring const & type,
        std::wstring const & key,
        __inout _int64 & currentSequenceNumber)
    {
        KBuffer::SPtr bufferSPtr;
        auto error = FabricSerializer::Serialize(&data, bufferSPtr);
        if (!error.IsSuccess()) { return error; }

        if (currentSequenceNumber == ILocalStore::OperationNumberUnspecified || currentSequenceNumber == ILocalStore::SequenceNumberIgnore)
        {
            error = this->TryGetCurrentSequenceNumber(txSPtr, type, key, currentSequenceNumber);

            if (error.IsError(ErrorCodeValue::StoreRecordNotFound) || error.IsError(ErrorCodeValue::NotFound))
            {
                error = iReplicatedStore_.Insert(
                    txSPtr, 
                    type,
                    key,
                    bufferSPtr->GetBuffer(),
                    bufferSPtr->QuerySize());

                WriteNoise(
                    TraceComponent,
                    "{0} inserted data item {1}:{2} with {3}", 
                    this->TraceId,
                    type,
                    key,
                    error);

                return error;
            }
            else if (!error.IsSuccess())
            {
                return error;
            }
        }

        error = iReplicatedStore_.Update(
            txSPtr, 
            type,
            key,
            currentSequenceNumber,
            key,
            bufferSPtr->GetBuffer(),
            bufferSPtr->QuerySize());

        WriteNoise(
            TraceComponent,
            "{0} updated data item {1}:{2} ({3}) with {4}", 
            this->TraceId,
            type,
            key,
            currentSequenceNumber,
            error);

        return error;
    }        

    template ErrorCode NamingStore::TryReadData(TransactionSPtr const &, wstring const &, wstring const &, __out HierarchyNameData &, __out _int64 &);
    template ErrorCode NamingStore::TryReadData(TransactionSPtr const &, wstring const &, wstring const &, __out NameData &, __out _int64 &);
    template ErrorCode NamingStore::TryReadData(TransactionSPtr const &, wstring const &, wstring const &, __out NamePropertyStoreData &, __out _int64 &);
    template ErrorCode NamingStore::TryReadData(TransactionSPtr const &, wstring const &, wstring const &, __out PartitionedServiceDescriptor &, __out _int64 &);
    template ErrorCode NamingStore::TryReadData(TransactionSPtr const &, wstring const &, wstring const &, __out ServiceUpdateDescription &, __out _int64 &);

    template <class T>
    ErrorCode NamingStore::TryReadData(
        TransactionSPtr const & txSPtr,
        std::wstring const & type,
        std::wstring const & key,
        __out T & result,
        __out _int64 & currentSequenceNumber)
    {
        std::vector<byte> buffer;
        auto error = iReplicatedStore_.ReadExact(txSPtr, type, key, buffer, currentSequenceNumber);
        if (!error.IsSuccess()) { return error; }

        return FabricSerializer::Deserialize(result, buffer);
    }

    template ErrorCode NamingStore::ReadCurrentData(EnumerationSPtr const & enumSPtr, __out HierarchyNameData  & result);
    template ErrorCode NamingStore::ReadCurrentData(EnumerationSPtr const & enumSPtr, __out NameData  & result);
    template ErrorCode NamingStore::ReadCurrentData(EnumerationSPtr const & enumSPtr, __out NamePropertyStoreData  & result);
    template ErrorCode NamingStore::ReadCurrentData(EnumerationSPtr const & enumSPtr, __out PartitionedServiceDescriptor  & result);
    template ErrorCode NamingStore::ReadCurrentData(EnumerationSPtr const & enumSPtr, __out ServiceUpdateDescription  & result);

    template <class T>
    ErrorCode NamingStore::ReadCurrentData(EnumerationSPtr const & enumSPtr, __out T & result)
    {
        std::vector<byte> buffer;
        ErrorCode error = enumSPtr->CurrentValue(buffer);
        if (error.IsSuccess())
        {
            error = FabricSerializer::Deserialize(result, buffer);
        }
        return error;
    }

    template ErrorCode NamingStore::TryInsertRootName(TransactionSPtr const &, HierarchyNameData, std::wstring const &);
    template ErrorCode NamingStore::TryInsertRootName(TransactionSPtr const &, NameData, std::wstring const &);
    
    template <class T>
    ErrorCode NamingStore::TryInsertRootName(TransactionSPtr const & txSPtr, T nameData, std::wstring const & dataType)
    {
        vector<byte> unusedBuffer;
        _int64 nameSequenceNumber = -1;
        auto error = iReplicatedStore_.ReadExact(
            txSPtr, 
            dataType, 
            NamingUri::RootNamingUriString, 
            unusedBuffer, 
            nameSequenceNumber);

        if (error.IsSuccess() || !error.IsError(ErrorCodeValue::NotFound)) 
        { 
            return error; 
        }

        KBuffer::SPtr bufferSPtr;
        error = FabricSerializer::Serialize(&nameData, bufferSPtr);
        if (!error.IsSuccess()) { return error; }

        return iReplicatedStore_.Insert(
            txSPtr, 
            dataType,
            NamingUri::RootNamingUriString,
            bufferSPtr->GetBuffer(),
            bufferSPtr->QuerySize());
    }

    // *************
    // Inner classes
    // *************

    void NamingStore::CommitAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = txSPtr_->BeginCommit(
            timeoutHelper_.GetRemainingTime(), 
            [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
            thisSPtr);
        this->OnCommitComplete(operation, true);
    }

    ErrorCode NamingStore::CommitAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CommitAsyncOperation>(operation)->Error;
    }

    ErrorCode NamingStore::CommitAsyncOperation::End(AsyncOperationSPtr const & operation, __out FABRIC_SEQUENCE_NUMBER & result)
    {
        auto casted = AsyncOperation::End<CommitAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            result = casted->lsn_;
        }

        return casted->Error;
    }

    void NamingStore::CommitAsyncOperation::OnCommitComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ErrorCode error = txSPtr_->EndCommit(operation, lsn_);

        this->TryComplete(operation->Parent, error);
    }
}

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
    using namespace Reliability;
    using namespace Store;
    using namespace ServiceModel;

    StringLiteral const TraceComponent("ProcessPropertyBatch");

    StoreService::ProcessPropertyBatchRequestAsyncOperation::ProcessPropertyBatchRequestAsyncOperation(
        MessageUPtr && request,
        __in NamingStore & namingStore,
        __in StoreServiceProperties & properties,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ProcessRequestAsyncOperation(
            std::move(request),
            namingStore,
            properties,
            timeout,
            callback,
            root)
        , batchResult_()
        , nameWithoutFragment_()
        , lockAcquired_(false)
        , containsCheckOperation_(false)
        , sequenceNumberCache_()
    {
        // For TStore, the sequence number of keys in the write set is always 0. This means that
        // a batch in the order { Put(A), CheckSeqNo(A) } will never work - the client would have
        // to put all sequence number checks before all writes. Work around this for now by
        // caching sequence numbers on first write.
        //
        if (NamingConfig::GetConfig().EnableTStore || StoreConfig::GetConfig().EnableTStore)
        {
            sequenceNumberCache_ = make_unique<unordered_map<wstring, _int64>>();
        }
    }

    ErrorCode StoreService::ProcessPropertyBatchRequestAsyncOperation::HarvestRequestMessage(Transport::MessageUPtr && request)
    {
        if (request->GetBody(batch_))
        {
            this->SetName(batch_.NamingUri);
            return ErrorCodeValue::Success;
        }
        else
        {
            return ErrorCodeValue::InvalidMessage;
        }
    }

    bool StoreService::ProcessPropertyBatchRequestAsyncOperation::AllowNameFragment()
    {
        this->NameWithoutFragment = NamingUri(this->Name.Path);

        // Allow property operations on service group members
        return false;
    }

    void StoreService::ProcessPropertyBatchRequestAsyncOperation::PerformRequest(AsyncOperationSPtr const & thisSPtr)
    {
        this->StartAcquireNamedLockForWrites(thisSPtr);
    }

    void StoreService::ProcessPropertyBatchRequestAsyncOperation::StartAcquireNamedLockForWrites(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            "{0} processing batch for {1}. Operation count = {2}.",
            this->TraceId,
            this->NameString,
            batch_.Operations.size());

        bool acquireLock = false;

        // Only acquire named lock if the batch contains a write operation to avoid extra message
        // hops from write conflicts when updating the properties version on the parent name.
        // This must be done before creating the local store transaction.
        //
        for (auto iter = batch_.Operations.begin(); iter != batch_.Operations.end(); ++iter)
        {
            if (iter->IsWrite)
            {
                acquireLock = true;
            }

            if (iter->IsCheck)
            {
                containsCheckOperation_ = true;
            }

            if (acquireLock && containsCheckOperation_)
            {
                // determined both flags already
                break;
            }
        }

        if (acquireLock)
        {
            auto inner = this->Store.BeginAcquireNamedLock(
                this->NameWithoutFragment,
                this->TraceId,
                this->GetRemainingTime(),
                [this] (AsyncOperationSPtr const & operation) { this->OnNamedLockAcquireComplete(operation, false); },
                thisSPtr);
            this->OnNamedLockAcquireComplete(inner, true);
        }
        else
        {
            this->StartProcessingPropertyBatch(thisSPtr);
        }
    }

    void StoreService::ProcessPropertyBatchRequestAsyncOperation::OnNamedLockAcquireComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = this->Store.EndAcquireNamedLock(this->NameWithoutFragment, this->TraceId, operation);

        if (error.IsSuccess())
        {
            lockAcquired_ = true;

            // Stale property write detection for batches that do not contain
            // any check operations from the user already. This is primarily 
            // for the single-operation convenience APIs.
            //
            bool acceptBatch = containsCheckOperation_;

            if (!acceptBatch)
            {
                acceptBatch = true;
                set<wstring> acceptedProperties;

                for (auto iter = batch_.Operations.begin(); iter != batch_.Operations.end(); ++iter)
                {
                    if (iter->IsWrite)
                    {
                        auto findIter = acceptedProperties.find(iter->PropertyName);
                        if (findIter == acceptedProperties.end())
                        {
                            if (this->TryAcceptRequest(*iter, thisSPtr))
                            {
                                acceptedProperties.insert(iter->PropertyName);
                            }
                            else
                            {
                                acceptBatch = false;
                                break;
                            }
                        }
                    }
                }
            }

            if (acceptBatch)
            {
                this->StartProcessingPropertyBatch(thisSPtr);
            }
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    bool StoreService::ProcessPropertyBatchRequestAsyncOperation::TryAcceptRequest(
        NamePropertyOperation const & operation, 
        AsyncOperationSPtr const & thisSPtr)
    {
        MessageUPtr failureReply;

        bool accepted = this->Properties.PropertyRequestTracker.TryAcceptRequest(
            this->ActivityHeader.ActivityId,
            NamePropertyKey(this->Name, operation.PropertyName),
            this->RequestInstance,
            failureReply);

        if (!accepted)
        {
            this->Reply = move(failureReply);

            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }

        return accepted;
    }

    void StoreService::ProcessPropertyBatchRequestAsyncOperation::StartProcessingPropertyBatch(AsyncOperationSPtr const & thisSPtr)
    {
        TransactionSPtr txSPtr;
        ErrorCode error = Store.CreateTransaction(this->ActivityHeader, txSPtr);

        wstring nameOwnerName = this->NameWithoutFragment.ToString();

        // Make sure the name exists and also update it conditional on the sequence number 
        // to make sure that the name (and its property contents) didn't change while 
        // processing this batch.
        //
        // This has the effect of serializing write property batches on a per name basis
        // but is necessary to be consistent with name deletion.
        //
        _int64 currentSequenceNumber = -1;
        EnumerationSPtr nameEnumSPtr;

        if (error.IsSuccess())
        {
            error = Store.TryGetCurrentSequenceNumber(
                        txSPtr,
                        Constants::NonHierarchyNameEntryType,
                        nameOwnerName,
                        currentSequenceNumber,
                        nameEnumSPtr);
        }

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} could not read name {1} ({2}) for batch operation: error = {3}", 
                this->TraceId,
                nameOwnerName,
                this->NameString,
                error);

            if (error.IsError(ErrorCodeValue::NotFound))
            {
                error = ErrorCodeValue::NameNotFound;
            }

            NamingStore::CommitReadOnly(move(txSPtr));

            this->TryComplete(thisSPtr, error);
            return;
        }

        bool isPropertiesModified = false;

        // Batched operations are processed in the order in which they
        // appear in the request. Processing of the batch is terminated
        // upon encountering any error.
        //
        for (size_t i = 0; 
            batchResult_.Error.IsSuccess() && i < batch_.Operations.size(); 
            ++i)
        {
            NamePropertyOperation & operation = batch_.Operations[i];

            WriteNoise(
                TraceComponent,
                "{0} processing batch operation {1} on '{2}' ({3})",
                this->TraceId,
                operation.OperationType,
                operation.PropertyName,
                i);

            switch (batch_.Operations[i].OperationType)
            {
            case NamePropertyOperationType::PutProperty:
            case NamePropertyOperationType::PutCustomProperty:
                DoWriteProperty(txSPtr, operation, i);
                isPropertiesModified = true;
                break;

            case NamePropertyOperationType::DeleteProperty:
                DoDeleteProperty(txSPtr, operation, i);
                isPropertiesModified = true;
                break;

            case NamePropertyOperationType::CheckExistence:
                DoCheckPropertyExistence(txSPtr, operation, i);
                break;

            case NamePropertyOperationType::CheckSequence:
                DoCheckPropertySequenceNumber(txSPtr, operation, i);
                break;

            case NamePropertyOperationType::GetProperty:
                DoGetProperty(txSPtr, operation, i);
                break;

            case NamePropertyOperationType::CheckValue:
                DoCheckPropertyValue(txSPtr, operation, i);
                break;

            default:
                WriteInfo(
                    TraceComponent,
                    "{0} dropping unsupported batch operation type {1}", 
                    this->TraceId,
                    operation.OperationType);
                batchResult_.SetFailedOperationIndex(ErrorCodeValue::UnsupportedNamingOperation, i);
                break;
            }
        } // for each operation in batch

        if (batchResult_.Error.IsSuccess())
        {
            if (isPropertiesModified)
            {
                NameData nameOwnerEntry;
                error = Store.ReadCurrentData<NameData>(nameEnumSPtr, nameOwnerEntry);

                if (error.IsSuccess())
                {
                    nameOwnerEntry.IncrementPropertiesVersion();

                    // Update name sequence number for this transaction only if writes occurred 
                    // to allow read-only batches to proceed without affecting one another.
                    //
                    error = Store.TryWriteData<NameData>(
                        txSPtr,
                        nameOwnerEntry,
                        Constants::NonHierarchyNameEntryType,
                        nameOwnerName,
                        currentSequenceNumber);
                }

                if (error.IsSuccess())
                {
                    nameEnumSPtr.reset();
                    auto operation = NamingStore::BeginCommit(
                        move(txSPtr),
                        this->GetRemainingTime(),
                        [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
                        thisSPtr);
                    this->OnCommitComplete(operation, true);
                    
                    return;
                }
            }
            else
            {
                nameEnumSPtr.reset();
                NamingStore::CommitReadOnly(move(txSPtr));
            }
        }
        else if (NamingErrorCategories::IsRetryableAtGateway(batchResult_.Error))
        {
            // Let the Naming Gateway retry if the batch fails due to 
            // a retryable error
            //
            error = batchResult_.Error;
            nameEnumSPtr.reset();
            NamingStore::CommitReadOnly(move(txSPtr));
        }

        this->FinishProcessingPropertyBatch(thisSPtr, error);
    }

    void StoreService::ProcessPropertyBatchRequestAsyncOperation::OnCommitComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ErrorCode error = NamingStore::EndCommit(operation);

        this->FinishProcessingPropertyBatch(operation->Parent, error);
    }

    void StoreService::ProcessPropertyBatchRequestAsyncOperation::FinishProcessingPropertyBatch(
        AsyncOperationSPtr const & thisSPtr, 
        ErrorCode const & error)
    {
        if (error.IsSuccess())
        {
            Reply = NamingMessage::GetPeerPropertyBatchReply(batchResult_);
        }

        this->TryComplete(thisSPtr, error);
    }

    void StoreService::ProcessPropertyBatchRequestAsyncOperation::DoWriteProperty(
        TransactionSPtr const & txSPtr,
        NamePropertyOperation & operation,
        size_t operationIndex)
    {
        auto propertyTypeId = operation.PropertyTypeId;
        wstring const & propertyName = operation.PropertyName;
        vector<byte> propertyValue(operation.TakeBytesParam());
        auto valueSize = propertyValue.size();
        
        if (valueSize > ServiceModel::Constants::NamedPropertyMaxValueSize)
        {
            WriteWarning(
                TraceComponent,
                "{0} property value exceeds maximum allowed size: property = {1}:{2} size = {3} max = {4}",
                this->TraceId,
                this->NameString,
                propertyName,
                valueSize,
                ServiceModel::Constants::NamedPropertyMaxValueSize);

            batchResult_.SetFailedOperationIndex(ErrorCodeValue::PropertyTooLarge, operationIndex);
            return;
        }
        else if (propertyName.size() > static_cast<size_t>(ServiceModelConfig::GetConfig().MaxPropertyNameLength))
        {
            WriteWarning(
                TraceComponent, 
                "{0} property name '{1}' exceeds maximum allowed length: length = {2} max = {3}",
                this->TraceId,
                propertyName,
                propertyName.size(),
                ServiceModelConfig::GetConfig().MaxPropertyNameLength);

            batchResult_.SetFailedOperationIndex(ErrorCodeValue::PropertyTooLarge, operationIndex);
            return;
        }

        wstring const & storeType = Constants::NamedPropertyType;
        wstring storeKey = NamePropertyKey::CreateKey(this->Name, propertyName);

        NamePropertyMetadata metadata(
            propertyName, propertyTypeId, static_cast<ULONG>(valueSize), operation.CustomTypeId);
        NamePropertyStoreData property(std::move(metadata), std::move(propertyValue));

        _int64 sequenceNumber = Store::ILocalStore::OperationNumberUnspecified;
        ErrorCode error = Store.TryWriteDataAndGetCurrentSequenceNumber<NamePropertyStoreData>(
            txSPtr, 
            property, 
            storeType, 
            storeKey, 
            sequenceNumber);

        if (sequenceNumberCache_.get() != nullptr)
        {
            auto & cache = *sequenceNumberCache_;
            if (cache.find(propertyName) == cache.end())
            {
                cache[propertyName] = sequenceNumber;

                WriteNoise(
                    TraceComponent,
                    "{0} cached sequence number {1}:{2} sequence={3}",
                    this->TraceId,
                    this->NameString,
                    propertyName,
                    sequenceNumber);
            }
        }

        WriteNoise(
            TraceComponent,
            "{0} wrote property {1}:{2}. {3} bytes. customType='{4}'. Error = {5}",
            this->TraceId,
            this->NameString,
            propertyName,
            valueSize,
            operation.CustomTypeId,
            error);

        if (!error.IsSuccess())
        {
            batchResult_.SetFailedOperationIndex(move(error), operationIndex);
        }
    }

    void StoreService::ProcessPropertyBatchRequestAsyncOperation::DoDeleteProperty(
        TransactionSPtr const & txSPtr,
        NamePropertyOperation & operation,
        size_t operationIndex)
    {
        wstring const & propertyName = operation.PropertyName;
        wstring const & storeType = Constants::NamedPropertyType;
        wstring storeKey = NamePropertyKey::CreateKey(this->Name, propertyName);

        _int64 currentSequenceNumber = -1;
        ErrorCode error = Store.TryGetCurrentSequenceNumber(txSPtr, storeType, storeKey, currentSequenceNumber);

        if (!error.IsSuccess())
        {
            batchResult_.SetFailedOperationIndex(
                    (error.IsError(ErrorCodeValue::NotFound) ? ErrorCodeValue::PropertyNotFound : error),
                    operationIndex);
            return;
        }

        error = Store.DeleteFromStore(
            txSPtr, 
            storeType,
            storeKey,
            currentSequenceNumber);

        WriteNoise(
            TraceComponent,
            "{0} deleted property {1}:{2}: error = {3}", 
            this->TraceId,
            this->NameString,
            propertyName,
            error);

        if (!error.IsSuccess())
        {
            batchResult_.SetFailedOperationIndex(move(error), operationIndex);
        }
    }

    void StoreService::ProcessPropertyBatchRequestAsyncOperation::DoCheckPropertyExistence(
        TransactionSPtr const & txSPtr,
        NamePropertyOperation & operation,
        size_t operationIndex)
    {
        wstring const & propertyName = operation.PropertyName;
        bool expected = operation.BoolParam;
        
        wstring const & storeType = Constants::NamedPropertyType;
        wstring storeKey = NamePropertyKey::CreateKey(this->Name, propertyName);

        _int64 unusedLsn;
        auto error = Store.TryGetCurrentSequenceNumber(txSPtr, storeType, storeKey, unusedLsn);

        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotFound))
        {
            WriteInfo(
                TraceComponent,
                "{0} DoCheckPropertyExistence: TryGetCurrentSequenceNumber returned error {1}",
                this->TraceId, error);

            batchResult_.SetFailedOperationIndex(move(error), operationIndex);
            return;
        }

        WriteNoise(
            TraceComponent,
            "{0} checked property {1}:{2}: expected = {3} actual = {4}", 
            this->TraceId,
            this->NameString,
            propertyName,
            expected,
            error.IsSuccess());

        if (expected != error.IsSuccess())
        {
            batchResult_.SetFailedOperationIndex(ErrorCodeValue::PropertyCheckFailed, operationIndex);
        }
    }

    void StoreService::ProcessPropertyBatchRequestAsyncOperation::DoCheckPropertySequenceNumber(
        TransactionSPtr const & txSPtr,
        NamePropertyOperation & operation,
        size_t operationIndex)
    {
        wstring const & propertyName = operation.PropertyName;
        _int64 expected = operation.Int64Param;

        wstring const & storeType = Constants::NamedPropertyType;
        wstring storeKey = NamePropertyKey::CreateKey(this->Name, propertyName);

        ErrorCode error = ErrorCodeValue::Success;
        _int64 currentSequenceNumber = Store::ILocalStore::OperationNumberUnspecified;

        if (sequenceNumberCache_.get() != nullptr)
        {
            auto & cache = *sequenceNumberCache_;
            if (cache.find(propertyName) != cache.end())
            {
                currentSequenceNumber = cache[propertyName];

                WriteNoise(
                    TraceComponent,
                    "{0} check from cache {1}:{2} sequenceNumber={3}",
                    this->TraceId,
                    this->NameString,
                    propertyName,
                    currentSequenceNumber);
            }
        }

        if (currentSequenceNumber == Store::ILocalStore::OperationNumberUnspecified)
        {
            error = Store.TryGetCurrentSequenceNumber(
                txSPtr,
                storeType,
                storeKey,
                currentSequenceNumber);
        }

        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotFound))
        {
            WriteInfo(
                TraceComponent,
                "{0} DoCheckPropertyExistence: TryGetCurrentSequenceNumber returned error {1}",
                this->TraceId, error);

            batchResult_.SetFailedOperationIndex(move(error), operationIndex);
            return;
        }

        WriteNoise(
            TraceComponent,
            "{0} checked property {1}:{2}: sequenceNumber = {3} expected = {4} error = {5}", 
            this->TraceId,
            this->NameString,
            propertyName,
            currentSequenceNumber,
            expected,
            error);

        if (error.IsError(ErrorCodeValue::NotFound) || (expected != currentSequenceNumber))
        {
            batchResult_.SetFailedOperationIndex(ErrorCodeValue::PropertyCheckFailed, operationIndex);
        }
    }

    void StoreService::ProcessPropertyBatchRequestAsyncOperation::DoGetProperty(
        TransactionSPtr const & txSPtr,
        NamePropertyOperation & operation,
        size_t operationIndex)
    {
        wstring const & propertyName = operation.PropertyName;
        wstring const & storeType = Constants::NamedPropertyType;
        wstring storeKey = NamePropertyKey::CreateKey(this->Name, propertyName);

        NamePropertyResult propertyResult;
        auto error = Store.ReadProperty(this->Name, txSPtr, operation.OperationType, storeType, storeKey, operationIndex, propertyResult);

        if (error.IsError(ErrorCodeValue::NotFound))
        {
            batchResult_.AddPropertyNotFound(operationIndex);
            return;
        }
        else if (!error.IsSuccess())
        {
            batchResult_.SetFailedOperationIndex(move(error), operationIndex);
            return;
        }

        WriteNoise(
            TraceComponent,
            "{0} DoGetProperty({1}) {2}:{3} (sequence = {4})",
            this->TraceId,
            operation.BoolParam,
            this->NameString, 
            propertyName,
            propertyResult.Metadata.SequenceNumber);
            
        if (operation.BoolParam)
        {
            batchResult_.AddProperty(move(propertyResult));
        }
        else
        {
            batchResult_.AddMetadata(propertyResult.TakeMetadata());
        }
    }
        
    void StoreService::ProcessPropertyBatchRequestAsyncOperation::DoCheckPropertyValue(
        TransactionSPtr const & txSPtr,
        NamePropertyOperation & operation,
        size_t operationIndex)
    {
        wstring const & propertyName = operation.PropertyName;
        wstring const & storeType = Constants::NamedPropertyType;
        wstring storeKey = NamePropertyKey::CreateKey(this->Name, propertyName);

        NamePropertyResult propertyResult;
        auto error = Store.ReadProperty(this->Name, txSPtr, operation.OperationType, storeType, storeKey, operationIndex, propertyResult);
        if (error.IsError(ErrorCodeValue::NotFound))
        {
            batchResult_.AddPropertyNotFound(operationIndex);
            return;
        }
        else if (!error.IsSuccess())
        {
            batchResult_.SetFailedOperationIndex(move(error), operationIndex);
            return;
        }

        if (propertyResult.Metadata.TypeId != operation.PropertyTypeId)
        {
            // If the type doesn't match, do not check the value
             WriteInfo(
                TraceComponent,
                "{0} DoCheckPropertyValue: type mismatch: expected {1}, actual {2}", 
                this->TraceId,
                operation.PropertyTypeId,
                propertyResult.Metadata.TypeId);
            batchResult_.SetFailedOperationIndex(ErrorCodeValue::PropertyCheckFailed, operationIndex);
            return;
        }

        vector<byte> propertyValue(operation.TakeBytesParam());
        if (propertyResult.Bytes.size() != propertyValue.size())
        {
            WriteInfo(
                TraceComponent,
                "{0} DoCheckPropertyValue: value length mismatch: expected {1}, actual {2}", 
                this->TraceId,
                propertyValue.size(),
                propertyResult.Bytes.size());
            batchResult_.SetFailedOperationIndex(ErrorCodeValue::PropertyCheckFailed, operationIndex);
            return;
        }

        for (size_t i = 0; i < propertyResult.Bytes.size(); ++i)
        {
            if (propertyValue[i] != propertyResult.Bytes[i])
            {
                WriteInfo(
                    TraceComponent,
                    "{0} DoCheckPropertyValue: value mismatch at byte {1}: expected {2}, actual {3}", 
                    this->TraceId,
                    i,
                    propertyValue[i],
                    propertyResult.Bytes[i]);
                batchResult_.SetFailedOperationIndex(ErrorCodeValue::PropertyCheckFailed, operationIndex);
                return;
            }
        }

        WriteNoise(
            TraceComponent,
            "{0} DoCheckPropertyValue {1}:{2} (sequence = {3}): value matches",
            this->TraceId,
            this->NameString, 
            propertyName,
            propertyResult.Metadata.SequenceNumber);
    }
    
    void StoreService::ProcessPropertyBatchRequestAsyncOperation::OnCompleted()
    {
        StoreService::ProcessRequestAsyncOperation::OnCompleted();

        if (lockAcquired_)
        {
            this->Store.ReleaseNamedLock(this->NameWithoutFragment, this->TraceId, shared_from_this());
        }
    }
}

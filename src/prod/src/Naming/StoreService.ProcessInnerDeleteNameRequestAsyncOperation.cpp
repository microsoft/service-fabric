// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace Common;
    using namespace Transport;
    using namespace Federation;
    using namespace Reliability;
    using namespace Store;
    using namespace std;

    StringLiteral const TraceComponent("ProcessInnerDeleteName");

    StoreService::ProcessInnerDeleteNameRequestAsyncOperation::ProcessInnerDeleteNameRequestAsyncOperation(
        MessageUPtr && request,
        __in NamingStore & namingStore,
        __in StoreServiceProperties & properties,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ProcessRequestAsyncOperation(
            std::move(request),
            namingStore,
            properties,
            timeout,
            callback,
            root)
    {
    }
        
    ErrorCode StoreService::ProcessInnerDeleteNameRequestAsyncOperation::HarvestRequestMessage(Transport::MessageUPtr && request)
    {
        if (this->TryGetNameFromRequest(request))
        {
            this->ProcessRecoveryHeader(move(request));
            return ErrorCodeValue::Success;
        }
        else
        {
            return ErrorCodeValue::InvalidMessage;
        }
    }

    void StoreService::ProcessInnerDeleteNameRequestAsyncOperation::PerformRequest(AsyncOperationSPtr const & thisSPtr)
    {
        this->Properties.Trace.NOInnerDeleteNameRequestReceiveComplete(
            this->TraceId, 
            this->Node.Id.ToString(), 
            this->Node.InstanceId, 
            this->NameString);
        
        if (this->TryAcceptRequestAtNameOwner(thisSPtr))
        {
            // Just send an empty reply
            Reply = NamingMessage::GetInnerDeleteNameOperationReply();

            this->StartDeleteName(thisSPtr);
        }
    }

    void StoreService::ProcessInnerDeleteNameRequestAsyncOperation::StartDeleteName(AsyncOperationSPtr const & thisSPtr)
    {
        wstring const & dataType = Constants::NonHierarchyNameEntryType;

        TransactionSPtr txSPtr;
        ErrorCode error = this->Store.CreateTransaction(this->ActivityHeader, txSPtr);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        // Can't delete a name that doesn't exist
        _int64 currentSequenceNumber = -1;
        error = this->Store.TryGetCurrentSequenceNumber(
            txSPtr,
            dataType,
            this->NameString,
            currentSequenceNumber);

        if (error.IsError(ErrorCodeValue::NotFound))
        {
            error = ErrorCodeValue::NameNotFound;
        }
        else if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} cannot read non-hierarchy name {1} for delete: error = {2}",
                this->TraceId,
                this->NameString,
                error);
        }

        if (error.IsSuccess())
        {
            // Can't delete a name that still has existing properties
            wstring storePropertyType = Constants::NamedPropertyType;
            wstring storePropertyKeyPrefix = NamePropertyKey::CreateKey(this->Name, L"");

            EnumerationSPtr propertyEnumSPtr;
			
            error = this->Store.CreateEnumeration(txSPtr, storePropertyType, storePropertyKeyPrefix, propertyEnumSPtr);
            if (error.IsSuccess() && propertyEnumSPtr->MoveNext().IsSuccess())
            {
                wstring currentKey;
                error = propertyEnumSPtr->CurrentKey(currentKey);

                if (error.IsSuccess() && StringUtility::StartsWith(currentKey, storePropertyKeyPrefix))
                {
                    WriteInfo(
                        TraceComponent,
                        "{0} cannot delete non-hierarchy name {1} since it still has property {2}", 
                        this->TraceId,
                        this->NameString,
                        currentKey);
                    error = ErrorCodeValue::NameNotEmpty;
                }
            }
        }

        if (error.IsSuccess())
        {
            // Can't delete a name with an existing service description
            wstring serviceDataType = Constants::UserServiceDataType;
            
            EnumerationSPtr serviceEnumSPtr;
            error = this->Store.CreateEnumeration(txSPtr, serviceDataType, this->NameString, serviceEnumSPtr);
            if (error.IsSuccess() && serviceEnumSPtr->MoveNext().IsSuccess())
            {
                wstring currentKey;
                error = serviceEnumSPtr->CurrentKey(currentKey);

                if (error.IsSuccess() && currentKey == this->NameString)
                {
                    WriteInfo(
                        TraceComponent,
                        "{0} cannot delete non-hierarchy name {1} since it still has a service", 
                        this->TraceId,
                        this->NameString);
                    error = ErrorCodeValue::NameNotEmpty;
                }
            }
        }

        // No hierarchy-related operations (e.g. sub/parent name checks)
        // are performed at the name owner

        if (error.IsSuccess())
        {
            error = this->Store.DeleteFromStore(
                txSPtr, 
                dataType,
                this->NameString,
                currentSequenceNumber);
        }

        if (error.IsSuccess())
        {
            auto operation = NamingStore::BeginCommit(
                move(txSPtr),
                this->GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
                thisSPtr);
            this->OnCommitComplete(operation, true);
        }
        else
        {
            txSPtr.reset();
            this->TryComplete(thisSPtr, error);
        }
    }

    void StoreService::ProcessInnerDeleteNameRequestAsyncOperation::OnCommitComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ErrorCode error = NamingStore::EndCommit(operation);

        this->TryComplete(operation->Parent, error);
    }

    void StoreService::ProcessInnerDeleteNameRequestAsyncOperation::OnCompleted()
    {
        StoreService::ProcessRequestAsyncOperation::OnCompleted();

        this->Properties.Trace.NOInnerDeleteNameRequestProcessComplete(
            this->TraceId, 
            this->Node.Id.ToString(), 
            this->Node.InstanceId, 
            this->NameString);
    }
}

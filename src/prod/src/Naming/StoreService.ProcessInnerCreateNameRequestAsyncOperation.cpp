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

    StringLiteral const TraceComponent("ProcessInnerCreateName");

    StoreService::ProcessInnerCreateNameRequestAsyncOperation::ProcessInnerCreateNameRequestAsyncOperation(
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
    {
    }

    ErrorCode StoreService::ProcessInnerCreateNameRequestAsyncOperation::HarvestRequestMessage(Transport::MessageUPtr && request)
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
        
    void StoreService::ProcessInnerCreateNameRequestAsyncOperation::PerformRequest(AsyncOperationSPtr const & thisSPtr)
    {            
        this->Properties.Trace.NOInnerCreateNameRequestReceiveComplete(
            this->TraceId, 
            this->Node.Id.ToString(), 
            this->Node.InstanceId, 
            this->NameString);

        if (this->TryAcceptRequestAtNameOwner(thisSPtr))
        {
            // Just send an empty reply
            Reply = NamingMessage::GetInnerCreateNameOperationReply();
            
            this->StartCreateName(thisSPtr);
        }
    }

    void StoreService::ProcessInnerCreateNameRequestAsyncOperation::StartCreateName(AsyncOperationSPtr const & thisSPtr)
    {
        wstring const & dataType = Constants::NonHierarchyNameEntryType;

        TransactionSPtr txSPtr;
        ErrorCode error = this->Store.CreateTransaction(this->ActivityHeader, txSPtr);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        // Do not check for a parent at the name owner since the parent name
        // could belong to another partition

        // Check for pre-existing name
        NameData nameData(UserServiceState::None, DateTime::Now().Ticks);
        _int64 unusedSequenceNumber = -1;
        error = this->Store.TryReadData(txSPtr, dataType, this->NameString, nameData, unusedSequenceNumber);

        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} cannot create duplicate non-hierarchy name {1}.", 
                this->TraceId,
                this->NameString);
            error = ErrorCodeValue::NameAlreadyExists;
        }
        else if (error.IsError(ErrorCodeValue::NotFound))
        {
            error = this->Store.TryWriteData<NameData>(
                txSPtr, 
                nameData,
                dataType, 
                this->NameString,
                ILocalStore::SequenceNumberIgnore);
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

    void StoreService::ProcessInnerCreateNameRequestAsyncOperation::OnCommitComplete(
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

    void StoreService::ProcessInnerCreateNameRequestAsyncOperation::OnCompleted()
    {
        StoreService::ProcessRequestAsyncOperation::OnCompleted();

        this->Properties.Trace.NOInnerCreateNameRequestProcessComplete(
            this->TraceId, 
            this->Node.Id.ToString(), 
            this->Node.InstanceId, 
            this->NameString);
    }
}

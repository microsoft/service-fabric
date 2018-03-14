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

    StringLiteral const TraceComponent("ProcessInnerDeleteService");

    StoreService::ProcessInnerDeleteServiceRequestAsyncOperation::ProcessInnerDeleteServiceRequestAsyncOperation(
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
        , body_()
    {
    }

    ErrorCode StoreService::ProcessInnerDeleteServiceRequestAsyncOperation::HarvestRequestMessage(Transport::MessageUPtr && request)
    {
        if (request->GetBody(body_))
        {
            this->SetName(this->RequestBody.NamingUri);
            this->IsForce = this->RequestBody.IsForce;
            return ErrorCodeValue::Success;
        }
        else
        {
            return ErrorCodeValue::InvalidMessage;
        }
    }

    void StoreService::ProcessInnerDeleteServiceRequestAsyncOperation::PerformRequest(AsyncOperationSPtr const & thisSPtr)
    {
        this->Properties.Trace.NOInnerDeleteServiceRequestReceiveComplete(
            this->TraceId,
            this->Node.Id.ToString(),
            this->Node.InstanceId, 
            this->NameString);
        
        if (this->TryAcceptRequestAtNameOwner(thisSPtr))
        {
            // Just send an empty reply
            Reply = NamingMessage::GetInnerDeleteServiceOperationReply();

            this->StartRemoveService(thisSPtr);
        }
    }

    void StoreService::ProcessInnerDeleteServiceRequestAsyncOperation::StartRemoveService(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = AsyncOperation::CreateAndStart<RemoveServiceAtNameOwnerAsyncOperation>(
            this->Name,
            this->Store,
            false /*isDeletionComplete*/,
            this->ActivityHeader,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnTentativeRemoveComplete(operation, false); },
            thisSPtr);
        this->OnTentativeRemoveComplete(operation, true);
    }

    void StoreService::ProcessInnerDeleteServiceRequestAsyncOperation::OnTentativeRemoveComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = RemoveServiceAtNameOwnerAsyncOperation::End(operation);

        // By-pass to FM if force delete
        if ((error.IsError(ErrorCodeValue::UserServiceNotFound) || error.IsError(ErrorCodeValue::NameNotFound)) && this->IsForce)
        {
            this->StartRequestToFM(thisSPtr);
        }
        else if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
        }
        else
        {
            this->StartRequestToFM(thisSPtr);
        }
    }

    void StoreService::ProcessInnerDeleteServiceRequestAsyncOperation::StartRequestToFM(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = this->Properties.AdminClient.BeginDeleteService(
            this->NameString,
            this->IsForce,
            DateTime::Now().Ticks,
            this->ActivityHeader,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnRequestToFMComplete(operation, false); },
            thisSPtr);
        
        this->Properties.Trace.NOInnerDeleteServiceToFMSendComplete(
            this->TraceId, 
            this->Node.Id.ToString(), 
            this->Node.InstanceId, 
            this->NameString);

        this->OnRequestToFMComplete(operation, true);
    }

    void StoreService::ProcessInnerDeleteServiceRequestAsyncOperation::OnRequestToFMComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = this->Properties.AdminClient.EndDeleteService(operation);

        // FM will continue returning FMServiceDeleteInProgress until all replicas are dropped.
        // Block delete until FM finishes dropping all replicas since returning early just
        // causes confusion for users when the application/service is gone from CM/Naming,
        // but remains running in the system.
        //
        if (error.IsError(ErrorCodeValue::FMServiceDoesNotExist))
        {
            error = ErrorCodeValue::Success;
        }

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
        }
        else
        {
            this->FinishRemoveService(thisSPtr);
        }
    }

    void StoreService::ProcessInnerDeleteServiceRequestAsyncOperation::FinishRemoveService(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = AsyncOperation::CreateAndStart<RemoveServiceAtNameOwnerAsyncOperation>(
            this->Name,
            this->Store,
            true /*isDeletionComplete*/,
            this->ActivityHeader,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnRemoveServiceComplete(operation, false); },
            thisSPtr);
        this->OnRemoveServiceComplete(operation, true);
    }

    void StoreService::ProcessInnerDeleteServiceRequestAsyncOperation::OnRemoveServiceComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;
        
        FABRIC_SEQUENCE_NUMBER lsn = 0;
        ErrorCode error = RemoveServiceAtNameOwnerAsyncOperation::End(operation, lsn);

        if (error.IsSuccess())
        {
            this->PsdCache.RemoveCacheEntry(this->NameString, lsn);
        }

        this->TryComplete(thisSPtr, error);
    }

    void StoreService::ProcessInnerDeleteServiceRequestAsyncOperation::OnCompleted()
    {
        StoreService::ProcessRequestAsyncOperation::OnCompleted();

        this->Properties.Trace.NOInnerDeleteServiceRequestProcessComplete(
            this->TraceId,
            this->Node.Id.ToString(),
            this->Node.InstanceId,
            this->NameString);
    }
}

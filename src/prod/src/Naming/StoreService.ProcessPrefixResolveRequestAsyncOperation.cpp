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

    StringLiteral const TraceComponent("ProcessPrefixResolve");

    StoreService::ProcessPrefixResolveRequestAsyncOperation::ProcessPrefixResolveRequestAsyncOperation(
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

    ErrorCode StoreService::ProcessPrefixResolveRequestAsyncOperation::HarvestRequestMessage(Transport::MessageUPtr && request)
    {
        ServiceCheckRequestBody body;
        if (request->GetBody(body))
        {
            if (!body.IsPrefixMatch)
            {
                TRACE_ERROR_AND_TESTASSERT(
                    TraceComponent,
                    "{0}: exact match not supported at Authority Owner: {1}",
                    this->TraceId,
                    body.Name);
                return ErrorCodeValue::InvalidMessage;
            }
            else
            {
                this->SetName(body.Name);

                return ErrorCodeValue::Success;
            }
        }
        else
        {
            return ErrorCodeValue::InvalidMessage;
        }
    }

    void StoreService::ProcessPrefixResolveRequestAsyncOperation::PerformRequest(AsyncOperationSPtr const & thisSPtr)
    {
        this->StartPrefixResolution(thisSPtr);
    }

    void StoreService::ProcessPrefixResolveRequestAsyncOperation::StartPrefixResolution(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = this->SetCurrentPrefix();
        
        if (error.IsSuccess())
        {
            this->StartResolveNameOwner(thisSPtr);
        }
        else if (error.IsError(ErrorCodeValue::UserServiceNotFound))
        {
            this->TryComplete(thisSPtr, error);
        }
        else
        {
            this->TimeoutOrScheduleRetry(thisSPtr,
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartPrefixResolution(thisSPtr); });
        }
    }

    void StoreService::ProcessPrefixResolveRequestAsyncOperation::StartResolveNameOwner(AsyncOperationSPtr const & thisSPtr)
    {
        auto inner = this->BeginResolveNameLocation(
            currentPrefix_,
            [this](AsyncOperationSPtr const & operation) { this->OnResolveNameOwnerComplete(operation, false); },
            thisSPtr);
        this->OnResolveNameOwnerComplete(inner, true);
    }

    void StoreService::ProcessPrefixResolveRequestAsyncOperation::OnResolveNameOwnerComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (expectedCompletedSynchronously != operation->CompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        auto error = this->EndResolveNameLocation(operation, nameOwnerLocation_);
        
        if (!error.IsSuccess())
        {
            this->TimeoutOrScheduleRetry(thisSPtr,
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartResolveNameOwner(thisSPtr); });
        }
        else
        {
            this->StartRequestToNameOwner(thisSPtr);
        }
    }

    void StoreService::ProcessPrefixResolveRequestAsyncOperation::StartRequestToNameOwner(AsyncOperationSPtr const & thisSPtr)
    {
        auto message = NamingMessage::GetPeerGetServiceDescription(ServiceCheckRequestBody(false, currentPrefix_));
        message->Headers.Replace(nameOwnerLocation_.CreateFilterHeader());

        auto inner = this->BeginRequestReplyToPeer(
            std::move(message),
            nameOwnerLocation_.NodeInstance,
            [this](AsyncOperationSPtr const & asyncOperation) { this->OnRequestReplyToPeerComplete(asyncOperation, false); },
            thisSPtr);
        this->OnRequestReplyToPeerComplete(inner, true);
    }

    void StoreService::ProcessPrefixResolveRequestAsyncOperation::OnRequestReplyToPeerComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (expectedCompletedSynchronously != operation->CompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        MessageUPtr reply;
        auto error = this->EndRequestReplyToPeer(operation, reply);

        ServiceCheckReplyBody body;
        if (error.IsSuccess())
        {
            if (reply->GetBody(body))
            {
                if (!body.DoesExist)
                {
                    error = ErrorCodeValue::UserServiceNotFound;
                }
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        if (error.IsSuccess())
        {
            this->Reply = NamingMessage::GetPrefixResolveReply(body);

            this->TryComplete(thisSPtr, error);
        }
        else if (error.IsError(ErrorCodeValue::UserServiceNotFound))
        {
            // Service must have been deleted in the meantime,
            // try again from the beginning.
            //
            this->StartPrefixResolution(thisSPtr);
        }
        else
        {
            this->TimeoutOrScheduleRetry(thisSPtr,
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartResolveNameOwner(thisSPtr); });
        }
    }

    ErrorCode StoreService::ProcessPrefixResolveRequestAsyncOperation::SetCurrentPrefix()
    {
        wstring const & dataType = Constants::HierarchyNameEntryType;

        TransactionSPtr txSPtr;
        auto error = this->Store.CreateTransaction(this->ActivityHeader, txSPtr);

        if (!error.IsSuccess()) { return error; }
        
        currentPrefix_ = this->Name;

        for (auto nameString = currentPrefix_.ToString(); 
            !currentPrefix_.IsRootNamingUri; 
            currentPrefix_ = currentPrefix_.GetParentName(),
            nameString = currentPrefix_.ToString())
        {
            HierarchyNameData nameData;
            __int64 nameSequenceNumber = -1;

            error = this->Store.TryReadData(
                txSPtr,
                dataType,
                nameString,
                nameData,
                nameSequenceNumber);

            if (error.IsError(ErrorCodeValue::NotFound))
            {
                continue;
            }
            else if (!error.IsSuccess())
            {
                return error;
            }

            if (nameData.NameState == HierarchyNameState::Created && nameData.ServiceState == UserServiceState::Created)
            {
                return ErrorCodeValue::Success;
            }
        }

        return ErrorCodeValue::UserServiceNotFound;
    }
}

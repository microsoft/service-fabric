// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace Naming
{
    using namespace Common;

    using namespace Transport;
    using namespace Federation;
    using namespace Reliability;
    using namespace std;
    using namespace Store;

    StringLiteral const TraceComponent("ProcessUpdateService");

    StoreService::ProcessUpdateServiceRequestAsyncOperation::ProcessUpdateServiceRequestAsyncOperation(
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
        , hasNamedLock_(false)
        , validationError_(ErrorCodeValue::Success)
    {
    }

    ErrorCode StoreService::ProcessUpdateServiceRequestAsyncOperation::HarvestRequestMessage(Transport::MessageUPtr && request)
    {
        if (request->GetBody(body_))
        {
            this->SetName(this->RequestBody.ServiceName);

            this->ProcessRecoveryHeader(move(request));
            return ErrorCodeValue::Success;
        }
        else
        {
            return ErrorCodeValue::InvalidMessage;
        }
    }

    void StoreService::ProcessUpdateServiceRequestAsyncOperation::PerformRequest(AsyncOperationSPtr const & thisSPtr)
    {
        // Set reply now since it will always be an empty body
        this->Reply = NamingMessage::GetServiceOperationReply(); 

        auto operation = Store.BeginAcquireNamedLock(
            this->Name, 
            this->TraceId,
            this->GetRemainingTime(),
            [this] (AsyncOperationSPtr const & operation) { this->OnNamedLockAcquireComplete(operation, false); },
            thisSPtr);
        this->OnNamedLockAcquireComplete(operation, true);
    }

    void StoreService::ProcessUpdateServiceRequestAsyncOperation::OnNamedLockAcquireComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (expectedCompletedSynchronously != operation->CompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = this->Store.EndAcquireNamedLock(this->Name, this->TraceId, operation);
        
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
        }
        else
        {
            hasNamedLock_ = true;

            if (this->TryAcceptRequestAtAuthorityOwner(thisSPtr))
            {
                this->StartUpdateService(thisSPtr);
            }
        }
    }

    void StoreService::ProcessUpdateServiceRequestAsyncOperation::StartUpdateService(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = AsyncOperation::CreateAndStart<UpdateServiceAtAuthorityAsyncOperation>(
            this->Name,
            this->Store,
            false /*isUpdateComplete*/,
            this->RequestBody.UpdateDescription,
            this->ActivityHeader,
            this->GetRemainingTime(),
            [this] (AsyncOperationSPtr const & operation) { this->OnTentativeUpdateComplete(operation, false); },
            thisSPtr);
        this->OnTentativeUpdateComplete(operation, true);
    }

    void StoreService::ProcessUpdateServiceRequestAsyncOperation::OnTentativeUpdateComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = UpdateServiceAtAuthorityAsyncOperation::End(operation);

        if (this->IsLocalRetryable(error))
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error), 
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartUpdateService(thisSPtr); });
        }
        else if (!error.IsSuccess())
        {
            this->CompleteOrRecoverPrimary(thisSPtr, move(error),
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartUpdateService(thisSPtr); });
        }
        else
        {
            this->StartResolveNameOwner(thisSPtr);
        }
    }

    void StoreService::ProcessUpdateServiceRequestAsyncOperation::StartResolveNameOwner(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = this->BeginResolveNameLocation(
            this->Name,
            [this] (AsyncOperationSPtr const & operation) { this->OnResolveNameOwnerComplete(operation, false); },
            thisSPtr);   
        this->OnResolveNameOwnerComplete(operation, true);
    }

    void StoreService::ProcessUpdateServiceRequestAsyncOperation::OnResolveNameOwnerComplete(
        Common::AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (expectedCompletedSynchronously != operation->CompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = EndResolveNameLocation(operation, nameOwnerLocation_);

        if (!error.IsSuccess())
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error),
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartResolveNameOwner(thisSPtr); });
        }
        else
        {
            this->StartRequestToNameOwner(thisSPtr);
        }
    }

    void StoreService::ProcessUpdateServiceRequestAsyncOperation::StartRequestToNameOwner(AsyncOperationSPtr const & thisSPtr)
    {
        MessageUPtr message = NamingMessage::GetInnerUpdateService(this->RequestBody);
        message->Headers.Replace(nameOwnerLocation_.CreateFilterHeader());

        auto operation = this->BeginRequestReplyToPeer(
            std::move(message),
            nameOwnerLocation_.NodeInstance,
            [this](AsyncOperationSPtr const & operation) { this->OnRequestReplyToPeerComplete(operation, false); },
            thisSPtr);

        this->Properties.Trace.AOInnerUpdateServiceRequestToNOSendComplete(
            this->TraceId,
            this->Node.Id.ToString(),
            this->Node.InstanceId,
            this->NameString,
            nameOwnerLocation_.NodeInstance.Id.ToString(), 
            nameOwnerLocation_.NodeInstance.InstanceId); 

        this->OnRequestReplyToPeerComplete(operation, true);
    }

    void StoreService::ProcessUpdateServiceRequestAsyncOperation::OnRequestReplyToPeerComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        MessageUPtr reply;
        ErrorCode error = this->EndRequestReplyToPeer(operation, reply);

        if (error.IsSuccess())
        {
            UpdateServiceReplyBody body;
            if (reply->GetBody(body))
            {
                // Finish updating the service in order to revert tentative changes,
                // but return an error to the Naming Gateway
                //
                validationError_ = ErrorCode(body.ValidationError, body.TakeErrorMessage());
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }
        else if (this->Store.NeedsReversion(error))
        {
            // Revert tentative changes
            //
            validationError_ = move(error);

            error = ErrorCodeValue::Success;
        }

        if (!error.IsSuccess())
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error),
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartResolveNameOwner(thisSPtr); });
        }
        else
        {
            this->FinishUpdateService(thisSPtr);
        }
    }

    void StoreService::ProcessUpdateServiceRequestAsyncOperation::FinishUpdateService(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = AsyncOperation::CreateAndStart<UpdateServiceAtAuthorityAsyncOperation>(
            this->Name,
            this->Store,
            true /*isUpdateComplete*/,
            this->RequestBody.UpdateDescription,
            this->ActivityHeader,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnUpdateServiceComplete(operation, false); },
            thisSPtr);
        this->OnUpdateServiceComplete(operation, true);
    }

    void StoreService::ProcessUpdateServiceRequestAsyncOperation::OnUpdateServiceComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = UpdateServiceAtAuthorityAsyncOperation::End(operation);

        if (error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(this->IsPrimaryRecovery ? error : validationError_));
        }
        else
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error),
                [this](AsyncOperationSPtr const & thisSPtr) { this->FinishUpdateService(thisSPtr); });
        }
    }

    void StoreService::ProcessUpdateServiceRequestAsyncOperation::OnCompleted()
    {
        StoreService::ProcessRequestAsyncOperation::OnCompleted();

        if (hasNamedLock_)
        {
            this->Store.ReleaseNamedLock(this->Name, this->TraceId, shared_from_this());
        }

        this->Properties.Trace.AOUpdateServiceComplete(
            this->TraceId, 
            this->Node.Id.ToString(), 
            this->Node.InstanceId, 
            this->NameString);
    }
}

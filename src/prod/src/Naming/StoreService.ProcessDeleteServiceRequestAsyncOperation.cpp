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
    using namespace std;
    using namespace Store;

    StringLiteral const TraceComponent("ProcessDeleteService");

    StoreService::ProcessDeleteServiceRequestAsyncOperation::ProcessDeleteServiceRequestAsyncOperation(
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
        , nameOwnerLocation_()
        , body_()
        , isForceDelete_(false)
        , serviceNotFoundOnAuthorityOwner_(false)
    {
    }

    void StoreService::ProcessDeleteServiceRequestAsyncOperation::CompleteOrScheduleRetry(
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode && error,
        RetryCallback const & callback)
    {
        this->UpdateForceDeleteFlag(); 
        StoreService::ProcessRequestAsyncOperation::CompleteOrScheduleRetry(thisSPtr, move(error), callback);
    }

    void StoreService::ProcessDeleteServiceRequestAsyncOperation::UpdateForceDeleteFlag()
    {
        if (!isForceDelete_ && this->Store.ShouldForceDelete(this->Name))
        {
            isForceDelete_ = true;
            this->body_.IsForce = true;
        }
    }

    ErrorCode StoreService::ProcessDeleteServiceRequestAsyncOperation::HarvestRequestMessage(Transport::MessageUPtr && request)
    {
        if (request->GetBody(body_))
        {
            this->SetName(this->RequestBody.NamingUri);
            this->isForceDelete_ = this->RequestBody.IsForce;

            this->ProcessRecoveryHeader(move(request));
            return ErrorCodeValue::Success;
        }
        else
        {
            return ErrorCodeValue::InvalidMessage;
        }
    }

    void StoreService::ProcessDeleteServiceRequestAsyncOperation::PerformRequest(AsyncOperationSPtr const & thisSPtr)
    {
        this->Reply = NamingMessage::GetDeleteServiceReply(VersionedReplyBody());

        auto operation = this->Store.BeginAcquireDeleteServiceNamedLock(
            this->Name,
            this->isForceDelete_,
            this->TraceId,
            this->GetRemainingTime(),
            [this] (AsyncOperationSPtr const & operation) { this->OnNamedLockAcquireComplete(operation, false); },
            thisSPtr);
        this->OnNamedLockAcquireComplete(operation, true);
    }

    void StoreService::ProcessDeleteServiceRequestAsyncOperation::OnNamedLockAcquireComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = this->Store.EndAcquireDeleteServiceNamedLock(this->Name, this->TraceId, operation);
        this->UpdateForceDeleteFlag(); 

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
        }
        else
        {
            hasNamedLock_ = true;

            if (this->TryAcceptRequestAtAuthorityOwner(thisSPtr))
            {
                this->StartRemoveService(thisSPtr);
            }
        }
    }

    void StoreService::ProcessDeleteServiceRequestAsyncOperation::StartRemoveService(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = AsyncOperation::CreateAndStart<RemoveServiceAtAuthorityAsyncOperation>(
            this->Name,
            this->Store,
            false /*isDeletionComplete*/,
            this->isForceDelete_,
            this->ActivityHeader,
            this->GetRemainingTime(),
            [this] (AsyncOperationSPtr const & operation) { this->OnTentativeRemoveComplete(operation, false); },
            thisSPtr);
        this->OnTentativeRemoveComplete(operation, true);
    }

    void StoreService::ProcessDeleteServiceRequestAsyncOperation::OnTentativeRemoveComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = RemoveServiceAtAuthorityAsyncOperation::End(operation, this->isForceDelete_);

        if (error.IsError(ErrorCodeValue::UserServiceNotFound) || error.IsError(ErrorCodeValue::NameNotFound))
        {
            // By-pass to Name Owner if force delete
            if (this->IsPrimaryRecovery || isForceDelete_)
            {
                error = ErrorCodeValue::Success;
            }
            else
            {
                serviceNotFoundOnAuthorityOwner_ = true;

                // Recursive name deletion is consistent but not atomic
                this->StartRecursiveDeleteName(thisSPtr);
                return;
            }
        }

        if (this->IsLocalRetryable(error))
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error), 
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartRemoveService(thisSPtr); });
        }
        else if (!error.IsSuccess())
        {
            this->CompleteOrRecoverPrimary(thisSPtr, move(error),
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartRemoveService(thisSPtr); });
        }
        else
        {
            this->StartResolveNameOwner(thisSPtr);
        }
    }

    void StoreService::ProcessDeleteServiceRequestAsyncOperation::StartResolveNameOwner(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = this->BeginResolveNameLocation(
            this->Name,
            [this](AsyncOperationSPtr const & operation) { this->OnResolveNameOwnerComplete(operation, false); },
            thisSPtr);
        this->OnResolveNameOwnerComplete(operation, true);
    }

    void StoreService::ProcessDeleteServiceRequestAsyncOperation::OnResolveNameOwnerComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = this->EndResolveNameLocation(operation, nameOwnerLocation_);

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
    
    void StoreService::ProcessDeleteServiceRequestAsyncOperation::StartRequestToNameOwner(AsyncOperationSPtr const & thisSPtr)
    {
        MessageUPtr innerDeleteRequest = NamingMessage::GetInnerDeleteService(this->RequestBody); 
        innerDeleteRequest->Headers.Replace(nameOwnerLocation_.CreateFilterHeader());

        auto operation = this->BeginRequestReplyToPeer(
            std::move(innerDeleteRequest),
            nameOwnerLocation_.NodeInstance,
            [this](AsyncOperationSPtr const & operation) { this->OnRequestToNameOwnerComplete(operation, false); },
            thisSPtr);

        this->Properties.Trace.AOInnerDeleteServiceRequestToNOSendComplete(
            this->TraceId,
            this->Node.Id.ToString(),
            this->Node.InstanceId,
            this->NameString, 
            nameOwnerLocation_.NodeInstance.Id.ToString(), 
            nameOwnerLocation_.NodeInstance.InstanceId);

        this->OnRequestToNameOwnerComplete(operation, true);
    }

    void StoreService::ProcessDeleteServiceRequestAsyncOperation::OnRequestToNameOwnerComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        MessageUPtr unusedReply;
        ErrorCode error = this->EndRequestReplyToPeer(operation, unusedReply);

        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::UserServiceNotFound) && !error.IsError(ErrorCodeValue::NameNotFound))
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error),
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartResolveNameOwner(thisSPtr); });
        }
        else
        {
            this->Properties.Trace.AOInnerDeleteServiceReplyFromNOReceiveComplete(
                this->TraceId,
                this->Node.Id.ToString(), 
                this->Node.InstanceId, 
                this->NameString);

            this->FinishRemoveService(thisSPtr);
        }
    }

    void StoreService::ProcessDeleteServiceRequestAsyncOperation::FinishRemoveService(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = AsyncOperation::CreateAndStart<RemoveServiceAtAuthorityAsyncOperation>(
            this->Name,
            this->Store,
            true /*isDeletionComplete*/,
            this->isForceDelete_,
            this->ActivityHeader,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnRemoveServiceComplete(operation, false); },
            thisSPtr);
        this->OnRemoveServiceComplete(operation, true);
    }

    void StoreService::ProcessDeleteServiceRequestAsyncOperation::OnRemoveServiceComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = RemoveServiceAtAuthorityAsyncOperation::End(operation);

        if (error.IsError(ErrorCodeValue::UserServiceNotFound) || error.IsError(ErrorCodeValue::NameNotFound))
        {
            error = ErrorCodeValue::Success;
        }

        if (error.IsSuccess())
        {
            this->StartRecursiveDeleteName(thisSPtr);
        }
        else
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error),
                [this](AsyncOperationSPtr const & thisSPtr) { this->FinishRemoveService(thisSPtr); });
        }
    }

    void StoreService::ProcessDeleteServiceRequestAsyncOperation::StartRecursiveDeleteName(AsyncOperationSPtr const & thisSPtr)
    {
        auto innerRequest = NamingMessage::GetPeerNameDelete(this->Name);
        innerRequest->Headers.Replace(this->ActivityHeader);

        auto inner = AsyncOperation::CreateAndStart<ProcessDeleteNameRequestAsyncOperation>(
            move(innerRequest),
            this->Store,
            this->Properties,
            false,  // don't need to re-acquire named lock since we are already holding it to create the service
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & inner) { this->OnRecursiveDeleteNameComplete(inner, false); },
            thisSPtr);
        this->OnRecursiveDeleteNameComplete(inner, true);
    }

    void StoreService::ProcessDeleteServiceRequestAsyncOperation::OnRecursiveDeleteNameComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = ProcessRequestAsyncOperation::End(operation);

        // Always need to retry on errors since the recursive delete name operation 
        // is not holding the named lock
        //
        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NameNotFound))
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error),
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartRecursiveDeleteName(thisSPtr); });
        }
        else
        {
            this->Reply = NamingMessage::GetDeleteServiceReply(VersionedReplyBody());
            this->TryComplete(
                thisSPtr,
                (serviceNotFoundOnAuthorityOwner_ ? ErrorCodeValue::UserServiceNotFound : ErrorCodeValue::Success));
        }
    }

    void StoreService::ProcessDeleteServiceRequestAsyncOperation::OnCompleted()
    {
        StoreService::ProcessRequestAsyncOperation::OnCompleted();

        if (hasNamedLock_)
        {
            this->Store.ReleaseDeleteServiceNamedLock(this->Name, this->TraceId, shared_from_this());
        }

        this->Properties.Trace.AODeleteServiceComplete(
            this->TraceId, 
            this->Node.Id.ToString(), 
            this->Node.InstanceId, 
            this->NameString);
    }

    bool StoreService::ProcessDeleteServiceRequestAsyncOperation::ShouldTerminateProcessing() const
    {
        return this->OperationRetryStopwatch.IsRunning
            && NamingConfig::GetConfig().ServiceFailureTimeout.SubtractWithMaxAndMinValueCheck(this->OperationRetryStopwatch.Elapsed) <= TimeSpan::Zero;
    }
}

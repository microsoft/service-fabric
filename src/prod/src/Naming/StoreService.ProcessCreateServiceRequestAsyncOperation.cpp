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

    StringLiteral const TraceComponent("ProcessCreateService");

    StoreService::ProcessCreateServiceRequestAsyncOperation::ProcessCreateServiceRequestAsyncOperation(
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
        , isRebuildFromFM_(false)
        , revertError_()
    {
    }

    ErrorCode StoreService::ProcessCreateServiceRequestAsyncOperation::HarvestRequestMessage(Transport::MessageUPtr && request)
    {
        if (request->GetBody(body_))
        {
            this->SetName(this->RequestBody.NamingUri);

            this->ProcessRecoveryHeader(move(request));
            return ErrorCodeValue::Success;
        }
        else
        {
            return ErrorCodeValue::InvalidMessage;
        }
    }

    void StoreService::ProcessCreateServiceRequestAsyncOperation::PerformRequest(AsyncOperationSPtr const & thisSPtr)
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

    void StoreService::ProcessCreateServiceRequestAsyncOperation::OnNamedLockAcquireComplete(
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
                this->StartCreateService(thisSPtr);
            }
        }
    }

    void StoreService::ProcessCreateServiceRequestAsyncOperation::StartCreateService(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = AsyncOperation::CreateAndStart<WriteServiceAtAuthorityAsyncOperation>(
            this->Name,
            this->Store,
            false /*isCreationComplete*/,
            this->RequestBody.PartitionedServiceDescriptor,
            this->ActivityHeader,
            this->GetRemainingTime(),
            [this] (AsyncOperationSPtr const & operation) { this->OnTentativeWriteComplete(operation, false); },
            thisSPtr);
        this->OnTentativeWriteComplete(operation, true);
    }

    void StoreService::ProcessCreateServiceRequestAsyncOperation::OnTentativeWriteComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = WriteServiceAtAuthorityAsyncOperation::End(operation);

        if (this->IsPrimaryRecovery && error.IsError(ErrorCodeValue::UserServiceAlreadyExists))
        {
            error = ErrorCodeValue::Success;
        }

        if (this->IsLocalRetryable(error))
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error), 
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartCreateService(thisSPtr); });
        }
        else if (error.IsError(ErrorCodeValue::NameNotFound))
        {
            this->StartRecursiveCreateName(thisSPtr);
        }
        else if (!error.IsSuccess())
        {
            this->CompleteOrRecoverPrimary(thisSPtr, move(error),
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartCreateService(thisSPtr); });
        }
        else
        {
            this->StartResolveNameOwner(thisSPtr);
        }
    }

    void StoreService::ProcessCreateServiceRequestAsyncOperation::StartRecursiveCreateName(AsyncOperationSPtr const & thisSPtr)
    {
        auto innerRequest = NamingMessage::GetPeerNameCreate(this->Name);
        innerRequest->Headers.Replace(this->ActivityHeader);

        auto inner = AsyncOperation::CreateAndStart<ProcessCreateNameRequestAsyncOperation>(
            move(innerRequest),
            this->Store,
            this->Properties,
            false,  // don't need to re-acquire named lock since we are already holding it to create the service
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & inner) { this->OnRecursiveCreateNameComplete(inner, false); },
            thisSPtr);
        this->OnRecursiveCreateNameComplete(inner, true);
    }

    void StoreService::ProcessCreateServiceRequestAsyncOperation::OnRecursiveCreateNameComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = ProcessRequestAsyncOperation::End(operation);

        // Always need to retry on errors since the recursive create name operation 
        // is not holding the named lock
        //
        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NameAlreadyExists))
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error),
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartRecursiveCreateName(thisSPtr); });
        }
        else
        {
            this->StartCreateService(thisSPtr);
        }
    }

    void StoreService::ProcessCreateServiceRequestAsyncOperation::StartResolveNameOwner(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = this->BeginResolveNameLocation(
            this->Name,
            [this] (AsyncOperationSPtr const & operation) { this->OnResolveNameOwnerComplete(operation, false); },
            thisSPtr);   
        this->OnResolveNameOwnerComplete(operation, true);
    }

    void StoreService::ProcessCreateServiceRequestAsyncOperation::OnResolveNameOwnerComplete(
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

    void StoreService::ProcessCreateServiceRequestAsyncOperation::StartRequestToNameOwner(AsyncOperationSPtr const & thisSPtr)
    {
        MessageUPtr message = NamingMessage::GetInnerCreateService(this->RequestBody);
        message->Headers.Replace(nameOwnerLocation_.CreateFilterHeader());

        auto operation = this->BeginRequestReplyToPeer(
            std::move(message),
            nameOwnerLocation_.NodeInstance,
            [this](AsyncOperationSPtr const & operation) { this->OnRequestReplyToPeerComplete(operation, false); },
            thisSPtr);

        this->Properties.Trace.AOInnerCreateServiceRequestToNOSendComplete(
            this->TraceId,
            this->Node.Id.ToString(),
            this->Node.InstanceId,
            this->NameString,
            nameOwnerLocation_.NodeInstance.Id.ToString(), 
            nameOwnerLocation_.NodeInstance.InstanceId); 

        this->OnRequestReplyToPeerComplete(operation, true);
    }

    void StoreService::ProcessCreateServiceRequestAsyncOperation::OnRequestReplyToPeerComplete(
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

        // Tactical fix to ensure consistency between Naming and FM
        //
        if (error.IsError(ErrorCodeValue::UserServiceAlreadyExists))
        {
            isRebuildFromFM_ = true;
            error = ErrorCodeValue::Success;
        }

        // Do not expect the name to be missing, but avoid getting stuck
        // on create service step if this ever happens.
        //
        if (error.IsError(ErrorCodeValue::NameNotFound))
        {
            this->StartRecursiveCreateName(thisSPtr);
        }
        // Special-case:
        // Intervention from user is needed - the FM/PLB cannot process this request
        //
        else if (this->Store.NeedsReversion(error))
        {
            revertError_ = error;
            this->RevertTentativeCreate(thisSPtr);
        }
        else if (!error.IsSuccess())
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error),
                [this](AsyncOperationSPtr const & thisSPtr) { this->StartResolveNameOwner(thisSPtr); });
        }
        else
        {
            this->FinishCreateService(thisSPtr);
        }
    }

    void StoreService::ProcessCreateServiceRequestAsyncOperation::FinishCreateService(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = AsyncOperation::CreateAndStart<WriteServiceAtAuthorityAsyncOperation>(
            this->Name,
            this->Store,
            true /*isCreationComplete*/,
            this->RequestBody.PartitionedServiceDescriptor,
            this->ActivityHeader,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnWriteServiceComplete(operation, false); },
            thisSPtr);
        this->OnWriteServiceComplete(operation, true);
    }

    void StoreService::ProcessCreateServiceRequestAsyncOperation::OnWriteServiceComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = WriteServiceAtAuthorityAsyncOperation::End(operation);

        if (!error.IsSuccess())
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error),
                [this](AsyncOperationSPtr const & thisSPtr) { this->FinishCreateService(thisSPtr); });
        }
        else
        {
            // Tactical fix to ensure consistency between Naming and FM
            //
            if (isRebuildFromFM_ && !this->IsPrimaryRecovery)
            {
                error = ErrorCodeValue::UserServiceAlreadyExists;
            }

            if (error.IsSuccess())
            {
                this->Reply = NamingMessage::GetServiceOperationReply(); 
            }

            this->TryComplete(thisSPtr, move(error));
        }
    }

    void StoreService::ProcessCreateServiceRequestAsyncOperation::RevertTentativeCreate(AsyncOperationSPtr const & thisSPtr)
    {
        auto inner = AsyncOperation::CreateAndStart<RemoveServiceAtAuthorityAsyncOperation>(
            this->Name,
            this->Store,
            true /*isDeletionComplete*/,
            false /*isForceDelete*/,
            this->ActivityHeader,
            this->GetRemainingTime(),
            [this] (AsyncOperationSPtr const & operation) { this->OnRevertTentativeComplete(operation, false); },
            thisSPtr);
        this->OnRevertTentativeComplete(inner, true);
    }

    void StoreService::ProcessCreateServiceRequestAsyncOperation::OnRevertTentativeComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = RemoveServiceAtAuthorityAsyncOperation::End(operation);

        if (error.IsError(ErrorCodeValue::UserServiceNotFound))
        {
            error = ErrorCodeValue::Success;
        }

        if (!error.IsSuccess())
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error),
                [this](AsyncOperationSPtr const & thisSPtr) { this->RevertTentativeCreate(thisSPtr); });
        }
        else
        {
            this->TryComplete(thisSPtr, revertError_);
        }
    }

    void StoreService::ProcessCreateServiceRequestAsyncOperation::OnCompleted()
    {
        StoreService::ProcessRequestAsyncOperation::OnCompleted();

        if (hasNamedLock_)
        {
            this->Store.ReleaseNamedLock(this->Name, this->TraceId, shared_from_this());
        }

        this->Properties.Trace.AOCreateServiceComplete(
            this->TraceId, 
            this->Node.Id.ToString(), 
            this->Node.InstanceId, 
            this->NameString);
    }

    bool StoreService::ProcessCreateServiceRequestAsyncOperation::ShouldTerminateProcessing() const 
    {
        return this->OperationRetryStopwatch.IsRunning
            && NamingConfig::GetConfig().ServiceFailureTimeout.SubtractWithMaxAndMinValueCheck(this->OperationRetryStopwatch.Elapsed) <= TimeSpan::Zero;
    }
}

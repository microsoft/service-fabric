// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::Assert;
using Common::AsyncCallback;
using Common::AsyncOperation;
using Common::AsyncOperationSPtr;
using Common::ComPointer;
using Common::ErrorCode;

using std::move;

OperationStream::GetOperationAsyncOperation::GetOperationAsyncOperation(
    __in OperationStream & parent,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
    :   AsyncOperation(callback, state),
        parent_(parent),
        operation_(NULL)
{
}

void OperationStream::GetOperationAsyncOperation::OnStart(
    AsyncOperationSPtr const & )
{
}

void OperationStream::GetOperationAsyncOperation::ResumeOutsideLock(
    AsyncOperationSPtr const & thisSPtr)
{
    StartDequeue(thisSPtr);
}

void OperationStream::GetOperationAsyncOperation::StartDequeue(
    AsyncOperationSPtr const & thisSPtr)
{
    auto inner = parent_.dispatchQueue_->BeginDequeue(
        Common::TimeSpan::MaxValue, 
        [this](AsyncOperationSPtr const & asyncOperation) 
        {
            this->DequeueCallback(asyncOperation);
        },
        thisSPtr);
                    
    if(inner->CompletedSynchronously)
    {
        FinishDequeue(inner);
    }
}

void OperationStream::GetOperationAsyncOperation::DequeueCallback(
    AsyncOperationSPtr const & asyncOperation)
{
    if(!asyncOperation->CompletedSynchronously)
    {
        FinishDequeue(asyncOperation);
    }
}

void OperationStream::GetOperationAsyncOperation::FinishDequeue(
    AsyncOperationSPtr const & asyncOperation)
{
    std::unique_ptr<ComOperationCPtr> operation;
    ErrorCode error = parent_.dispatchQueue_->EndDequeue(asyncOperation, operation);
    AsyncOperationSPtr const & thisSPtr = asyncOperation->Parent;
    if (!error.IsSuccess())
    {
        thisSPtr->TryComplete(thisSPtr, error);
        return;
    }

    if (operation && 
        !operation->GetRawPointer()->IsEndOfStreamOperation)
    {
        ComOperationCPtr const & opPtr = *(operation.get());
            
        if (parent_.checkUpdateEpochOperations_)
        {
            auto specialOp = dynamic_cast<ComUpdateEpochOperation*>(opPtr.GetRawPointer());
            if (specialOp)
            {
                std::shared_ptr<ComOperationCPtr> operationSPtr(std::move(operation));
                auto root = parent_.CreateComponentRoot();
                Common::Threadpool::Post([this, root, operationSPtr, specialOp, thisSPtr]()
                {
                    this->UpdateSecondaryEpoch(
                        specialOp->Epoch, 
                        specialOp->PreviousEpochSequenceNumber,
                        thisSPtr);
                });
                return;
            }
        }

        operation_ = opPtr.GetRawPointer();
        ASSERT_IF(
            operation_ == nullptr, 
            "{0}:{1} The operation shouldn't be null", 
            parent_.endpointUniqueId_, parent_.purpose_);

        // Increase the ref count of the operation
        // to keep it alive.
        // When the user is done with it,
        // he should Release it.
        operation_->AddRef();

        thisSPtr->TryComplete(thisSPtr, error);
    }
    else
    {
        ReplicatorEventSource::Events->SecondaryGetLastOp(
            parent_.partitionId_,
            parent_.endpointUniqueId_,
            parent_.purpose_);
        
        if (operation)
        {
            // This is the end of stream operation
            ASSERT_IFNOT(
                operation->GetRawPointer()->IsEndOfStreamOperation,
                "{0}:{1} The operation type is invalid",
                parent_.endpointUniqueId_, parent_.purpose_);

            ComOperationCPtr const & opPtr = *(operation.get());
            operation_ = opPtr.GetRawPointer();
            operation_->AddRef();
        }

        thisSPtr->TryComplete(thisSPtr, error);
    }
}

void OperationStream::GetOperationAsyncOperation::UpdateSecondaryEpoch(
    FABRIC_EPOCH const & epoch,
    FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
    AsyncOperationSPtr const & thisSPtr)
{
    AsyncOperationSPtr inner = parent_.secondary_->BeginUpdateStateProviderEpochFromOperationStream(
        epoch, 
        previousEpochLastSequenceNumber,
        [this](AsyncOperationSPtr const & asyncOperation)
        {
            this->FinishUpdateSecondaryEpoch(asyncOperation, false);
        },
        thisSPtr);
    this->FinishUpdateSecondaryEpoch(inner, true);
}

void OperationStream::GetOperationAsyncOperation::FinishUpdateSecondaryEpoch(
    Common::AsyncOperationSPtr const & asyncOperation,
    bool completedSynchronously)
{
    if (asyncOperation->CompletedSynchronously == completedSynchronously)
    {
        ErrorCode error = parent_.secondary_->EndUpdateStateProviderEpochFromOperationStream(asyncOperation);

        if (!error.IsSuccess())
        {
            if (error.IsError(ErrorCodeValue::ReplicatorInternalError))
            {
                parent_.ReportFaultInternal(
                    FABRIC_FAULT_TYPE_TRANSIENT,
                    L"Secondary StateProvider EndUpdateEpoch Failed with ReplicatorInternalError");
            }
            else
            {
                parent_.ReportFaultInternal(
                    FABRIC_FAULT_TYPE_TRANSIENT,
                    L"Secondary StateProvider EndUpdateEpoch");
            }
        }

        StartDequeue(asyncOperation->Parent);
    }
}

ErrorCode OperationStream::GetOperationAsyncOperation::End(
    AsyncOperationSPtr const & asyncOperation,
    __out IFabricOperation * & operation)
{
    auto casted = AsyncOperation::End<GetOperationAsyncOperation>(asyncOperation);
    operation = casted->operation_;
    return casted->Error;
}

} // end namespace ReplicationComponent
} // end namespace Reliability

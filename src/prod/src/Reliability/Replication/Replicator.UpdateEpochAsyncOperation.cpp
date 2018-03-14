// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::AcquireWriteLock;
using Common::AsyncCallback;
using Common::AsyncOperation;
using Common::AsyncOperationSPtr;
using Common::ErrorCode;
using Common::Assert;
using Common::Threadpool;

using std::move;

Replicator::UpdateEpochAsyncOperation::UpdateEpochAsyncOperation(
    __in Replicator & parent,
    FABRIC_EPOCH const & epoch,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
    :   AsyncOperation(callback, state, true),
        parent_(parent),
        epoch_(epoch),
        primaryCopy_(),
        secondaryCopy_()
{
}

void Replicator::UpdateEpochAsyncOperation::OnStart(
    AsyncOperationSPtr const & thisSPtr)
{
    ErrorCode error;
    bool shouldComplete = false;
    
    {
        AcquireWriteLock lock(parent_.lock_);
        if(parent_.secondary_)
        {
            if (!parent_.VerifyIsSecondaryCallerHoldsLock(L"UpdateEpoch", error))
            {
                shouldComplete = true;
            }
            else
            {
                secondaryCopy_ = parent_.secondary_;
            }
        }
        else
        {
            ASSERT_IF(!parent_.primary_, "both secondary_ and primary_ are empty");
            primaryCopy_ = parent_.primary_;
        }
    }

    if(shouldComplete)
    {
        // Complete outside the lock
        TryComplete(thisSPtr, error);
    }
    else
    {
        if (primaryCopy_)
        {
            auto inner = primaryCopy_->BeginUpdateEpoch(
                epoch_,
                [this](AsyncOperationSPtr const & asyncOperation)                        
                {
                    this->UpdateEpochCallback(asyncOperation);
                }, 
                thisSPtr); 
            if(inner->CompletedSynchronously)
            {
                FinishUpdateEpoch(inner);
            }
        }
        else
        {
            auto inner = secondaryCopy_->BeginUpdateEpoch(
                epoch_,
                [this](AsyncOperationSPtr const & asyncOperation)                        
                {
                    this->UpdateEpochCallback(asyncOperation);
                }, 
                thisSPtr); 
            if(inner->CompletedSynchronously)
            {
                FinishUpdateEpoch(inner);
            }
        }

        error.ReadValue();
    }
}

void Replicator::UpdateEpochAsyncOperation::UpdateEpochCallback(
    AsyncOperationSPtr const & asyncOperation)
{
    if(!asyncOperation->CompletedSynchronously)
    {
        FinishUpdateEpoch(asyncOperation);
    }
}

void Replicator::UpdateEpochAsyncOperation::FinishUpdateEpoch(
    AsyncOperationSPtr const & asyncOperation)
{
    ErrorCode error;
    
    if (primaryCopy_)
    {
        error = primaryCopy_->EndUpdateEpoch(asyncOperation);
    }
    else
    {
        error = secondaryCopy_->EndUpdateEpoch(asyncOperation);
    }
    TryComplete(asyncOperation->Parent, error);
}

ErrorCode Replicator::UpdateEpochAsyncOperation::End(
    AsyncOperationSPtr const & asyncOperation)
{
    auto casted = AsyncOperation::End<UpdateEpochAsyncOperation>(asyncOperation);
    return casted->Error;
}

} // end namespace ReplicationComponent
} // end namespace Reliability

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::AcquireWriteLock;
using Common::Assert;
using Common::AsyncCallback;
using Common::AsyncOperation;
using Common::AsyncOperationSPtr;
using Common::ErrorCode;

using std::move;

Replicator::CloseAsyncOperation::CloseAsyncOperation(
    __in Replicator & parent,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
    :   AsyncOperation(callback, state, true),
        parent_(parent),
        primaryCopy_(nullptr),
        secondaryCopy_(nullptr)
{
}

void Replicator::CloseAsyncOperation::OnStart(
    AsyncOperationSPtr const & thisSPtr)
{
    ErrorCode error;
    
    {
        AcquireWriteLock lock(parent_.lock_);
        error = parent_.state_.TransitionToClosing();
        if (!error.IsSuccess())
        {
            ReplicatorEventSource::Events->ReplicatorInvalidState(
                parent_.partitionId_,
                parent_, // need to print the current state
                L"Close",
                (const int) ReplicatorState::Opened);
        }
        else 
        {
            if (parent_.primary_ && parent_.secondary_)
            {
                Assert::CodingError(
                    "{0}: Close: Can't have both primary and secondary",
                    parent_.ToString());
            }
            else if (!parent_.primary_ && !parent_.secondary_)
            {
                error = parent_.state_.TransitionToClosed();
            }
            else
            {
                // on the primary, close could wait (based on config) for pending repl operations to complete.
                // so make a copy 
                primaryCopy_ = parent_.primary_;

                // On the secondary, some operations are allowed when state is Closing 
                // (eg. GetOperation)
                // so make a copy of the secondary shared_ptr 
                secondaryCopy_ = parent_.secondary_;
            }
        }
    }

    if (primaryCopy_)
    {
        ClosePrimary(thisSPtr);
    }
    else if (secondaryCopy_)
    {
        CloseSecondary(thisSPtr);
    }
    else
    {
        // Complete outside the lock
        TryComplete(thisSPtr, error);
    }
}

ErrorCode Replicator::CloseAsyncOperation::End(
    AsyncOperationSPtr const & asyncOperation)
{
    auto casted = AsyncOperation::End<CloseAsyncOperation>(asyncOperation);

    casted->parent_.transport_->UnregisterMessageProcessor(casted->parent_);
    casted->parent_.perfCounters_.reset();
    casted->parent_.apiMonitor_->Close();
    casted->parent_.apiMonitor_.reset();
    // Do not reset the job queue because there could be pending items being processed and resetting the job queue results in asserting if that is the case
    casted->parent_.messageProcessorJobQueueUPtr_->Close();

    if (casted->Error.IsSuccess())
    {
        ComProxyStateProvider cleanup(move(casted->parent_.stateProvider_));
    }

    return casted->Error;
}

void Replicator::CloseAsyncOperation::ClosePrimary(AsyncOperationSPtr const & thisSPtr)
{
    auto inner = primaryCopy_->BeginClose(
        false,
        parent_.config_->PrimaryWaitForPendingQuorumsTimeout,
        [this](AsyncOperationSPtr const & asyncOperation) 
        {
            this->ClosePrimaryCallback(asyncOperation); 
        },
        thisSPtr);
    if (inner->CompletedSynchronously)
    {
        FinishClosePrimary(inner);
    }
}

void Replicator::CloseAsyncOperation::ClosePrimaryCallback(AsyncOperationSPtr const & asyncOperation)
{
    if (!asyncOperation->CompletedSynchronously)
    {
        FinishClosePrimary(asyncOperation);
    }
}

void Replicator::CloseAsyncOperation::FinishClosePrimary(AsyncOperationSPtr const & asyncOperation)
{
    ErrorCode error = primaryCopy_->EndClose(asyncOperation);
    {
        AcquireWriteLock lock(parent_.lock_);
        if (error.IsSuccess())
        {
            error = parent_.state_.TransitionToClosed();
            parent_.primary_.reset();
            primaryCopy_.reset();
        }
        else
        {
            parent_.state_.TransitionToFaulted();
            // A close could be due to a report fault. Reporting fault again here is not necessary
            // RAP aborts if close fails
        }
    }
       
    this->TryComplete(asyncOperation->Parent, error);
}

void Replicator::CloseAsyncOperation::CloseSecondary(AsyncOperationSPtr const & thisSPtr)
{
    auto inner = secondaryCopy_->BeginClose(
        false /*isPromiting*/,
        [this](AsyncOperationSPtr const & asyncOperation) 
        {
            this->CloseSecondaryCallback(asyncOperation); 
        },
        thisSPtr);
    if (inner->CompletedSynchronously)
    {
        FinishCloseSecondary(inner);
    }
}

void Replicator::CloseAsyncOperation::CloseSecondaryCallback(AsyncOperationSPtr const & asyncOperation)
{
    if (!asyncOperation->CompletedSynchronously)
    {
        FinishCloseSecondary(asyncOperation);
    }
}

void Replicator::CloseAsyncOperation::FinishCloseSecondary(AsyncOperationSPtr const & asyncOperation)
{
    ErrorCode error = secondaryCopy_->EndClose(asyncOperation);
    {
        AcquireWriteLock lock(parent_.lock_);
        if (error.IsSuccess())
        {
            error = parent_.state_.TransitionToClosed();
            parent_.secondary_.reset();
            secondaryCopy_.reset();
        }
        else
        {
            parent_.state_.TransitionToFaulted();
            // A close could be due to a report fault. Reporting fault again here is not necessary
            // RAP aborts if close fails
        }
    }
    
    this->TryComplete(asyncOperation->Parent, error);
}

} // end namespace ReplicationComponent
} // end namespace Reliability

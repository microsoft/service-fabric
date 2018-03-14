// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::AcquireReadLock;
using Common::AcquireWriteLock;
using Common::Assert;
using Common::AsyncCallback;
using Common::AsyncOperation;
using Common::AsyncOperationSPtr;
using Common::ComPointer;
using Common::ErrorCode;
using Common::Threadpool;

using std::move;

SecondaryReplicator::DrainQueueAsyncOperation::DrainQueueAsyncOperation(
    __in SecondaryReplicator & parent,
    bool requireServiceAck,
    DispatchQueueSPtr const & dispatchQueue,
    bool isCopyQueue,
    std::wstring const & queueType,
    std::wstring const & drainScenario,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
    :   AsyncOperation(callback, state, true),
        parent_(parent),
        requireServiceAck_(requireServiceAck),
        hasQueueDrainCompletedSuccessfully_(false),
        dispatchQueue_(dispatchQueue),
        queueType_(queueType),
        drainScenario_(drainScenario),
        isCopyQueue_(isCopyQueue),
        startedComplete_(false),
        isStreamFaulted_(false),
        isCancelRequested_(false)
{
}

// This runs under a lock, so do minimal work here
void SecondaryReplicator::DrainQueueAsyncOperation::OnStart(AsyncOperationSPtr const & )
{
}

void SecondaryReplicator::DrainQueueAsyncOperation::OnDrainStreamFaulted()
{
    isStreamFaulted_.store(true);
}

void SecondaryReplicator::DrainQueueAsyncOperation::OnCancel()
{
    bool hasQueueDrainedSuccessfully = false;
    {
        AcquireWriteLock grab(lock_);
        isCancelRequested_ = true;
        hasQueueDrainedSuccessfully = hasQueueDrainCompletedSuccessfully_;
    }

    if (hasQueueDrainedSuccessfully)
    {
        // If the service has finished draining the dispatch queue, but cannot ACK the operations and hence reports fault, we get a cancel call and we must complete ourself
        // Or else, this operation will never complete - BUG 1427038
        {
            AcquireWriteLock grab(lock_);
            startedComplete_ = true;
        }

        this->TryComplete(shared_from_this(), ErrorCode(Common::ErrorCodeValue::OperationCanceled));
    }
}

void SecondaryReplicator::DrainQueueAsyncOperation::ResumeOutsideLock(Common::AsyncOperationSPtr const & thisSPtr)
{
    if (this->IsCompleted)
    {
        // There could be a racing cancel that has already completed this operation 
        ReplicatorEventSource::Events->SecondaryDrainDispatchQueueDone(
            parent_.partitionId_,
            parent_.endpointUniqueId_,
            queueType_,
            static_cast<int>(thisSPtr->Error.ReadValue()),
            drainScenario_);
    }
    else
    {
        ReplicatorEventSource::Events->SecondaryDrainDispatchQueue(
            parent_.partitionId_,
            parent_.endpointUniqueId_,
            queueType_,
            drainScenario_,
            dispatchQueue_->ItemCount);
        
        // After the queue finishes draining, all the drained items could get ACK'd before the actual completion routine runs (FinishWaitForQueueToDrain)
        // That could result in completing this async operation out of band
        // 
        // So, explicitly keep the parent alive as this async chain can be broken here
        auto keepParentAlive = parent_.CreateComponentRoot();

        auto inner = dispatchQueue_->BeginWaitForQueueToDrain(
            [this, keepParentAlive](AsyncOperationSPtr const & asyncOperation)
        {
            this->WaitForQueueToDrainCallback(asyncOperation);
        },
            thisSPtr);

        if (inner->CompletedSynchronously)
        {
            FinishWaitForQueueToDrain(inner);
        }
    }
}

void SecondaryReplicator::DrainQueueAsyncOperation::WaitForQueueToDrainCallback(Common::AsyncOperationSPtr const & asyncOperation)
{
    if(!asyncOperation->CompletedSynchronously)
    {
        FinishWaitForQueueToDrain(asyncOperation);
    }
}

void SecondaryReplicator::DrainQueueAsyncOperation::FinishWaitForQueueToDrain(Common::AsyncOperationSPtr const & asyncOperation)
{
    auto error = dispatchQueue_->EndWaitForQueueToDrain(asyncOperation);
    ReplicatorEventSource::Events->SecondaryDrainDispatchQueueDone(
            parent_.partitionId_, 
            parent_.endpointUniqueId_,
            queueType_,
            static_cast<int>(error.ReadValue()),
            drainScenario_);
    AsyncOperationSPtr const & thisSPtr = asyncOperation->Parent;
    if (error.IsSuccess() && requireServiceAck_)
    {
        bool cancelRequested = false;
        {
            AcquireWriteLock grab(lock_);
            hasQueueDrainCompletedSuccessfully_ = true;
            cancelRequested = isCancelRequested_;
        }

        if (cancelRequested)
        {
            {
                AcquireWriteLock grab(lock_);
                startedComplete_ = true;
            }

            this->TryComplete(thisSPtr, ErrorCode(Common::ErrorCodeValue::OperationCanceled));
        }
        else if (isCopyQueue_)
        {
            CheckIfAllCopyOperationsAcked(thisSPtr);
        }
        else
        {
            CheckIfAllReplicationOperationsAcked(thisSPtr);
        }
    }
    else
    { 
        {
            AcquireWriteLock grab(lock_);
            startedComplete_ = true;
        }

        thisSPtr->TryComplete(thisSPtr, error);
    }
}

void SecondaryReplicator::DrainQueueAsyncOperation::CheckIfAllReplicationOperationsAcked(
    AsyncOperationSPtr const & asyncOperation)
{
    ASSERT_IF(isCopyQueue_, "{0}: Call CheckIfAllCopyOperationsAcked method for copy queue drain check", parent_.endpointUniqueId_);

    if (!requireServiceAck_) // This is a const, so it is OK to read without a lock
    {
        return;
    }

    bool cachedStartedComplete;
    bool cachedHasQueueDrainCompletedSuccessfully;
    {
        AcquireReadLock grab(lock_);
        cachedStartedComplete = startedComplete_;
        cachedHasQueueDrainCompletedSuccessfully = hasQueueDrainCompletedSuccessfully_;
    }

    if (!cachedHasQueueDrainCompletedSuccessfully || cachedStartedComplete)
    {
        // If the child drain async operation is not yet completed (!cachedCheckAllOperationAcked)
        // or if another thread has already started completing this async operation, we can NO-OP this call
        return;
    }

    // Wait until all operations are ACKed by the service
    {
        AcquireReadLock lock(parent_.queuesLock_);
        if (!parent_.replicationReceiver_.AllOperationsAcked)
        {
            // There are operations committed, but not completed 
            // (given to the service, not ACKed yet)
            return;
        }
    }
    
    {
        AcquireWriteLock grab(lock_);
        if (startedComplete_)
        {
            // If the service has implemented multi-threaded pumping, there can be 2 threads that could reach this point simultanesouly.
            // the write lock lets through only 1 of them to trace out the event and schedule the completion of the async operation
            return;
        } 
        else
        {
            startedComplete_ = true;
        }
    }

    bool streamFaulted = isStreamFaulted_.load();

    ReplicatorEventSource::Events->SecondaryWaitForAcksDone(
        parent_.partitionId_,
        parent_.endpointUniqueId_,
        queueType_,
        drainScenario_,
        streamFaulted);
 
    Threadpool::Post([this, asyncOperation, streamFaulted]()
        {
            if (streamFaulted)
            {
                asyncOperation->TryComplete(asyncOperation, Common::ErrorCodeValue::OperationStreamFaulted);
            }
            else
            {
                asyncOperation->TryComplete(asyncOperation, Common::ErrorCodeValue::Success);
            }
        });
}

void SecondaryReplicator::DrainQueueAsyncOperation::CheckIfAllCopyOperationsAcked(
    AsyncOperationSPtr const & asyncOperation)
{
    ASSERT_IFNOT(isCopyQueue_, "{0}: Call CheckIfAllReplicationOperationsAcked method for replication queue drain check", parent_.endpointUniqueId_);

    if (!requireServiceAck_) // This is a const, so it is OK to read without a lock
    {
        return;
    }

    bool cachedStartedComplete;
    bool cachedHasQueueDrainCompletedSuccessfully;
    {
        AcquireReadLock grab(lock_);
        cachedStartedComplete = startedComplete_;
        cachedHasQueueDrainCompletedSuccessfully = hasQueueDrainCompletedSuccessfully_;
    }

    if (!cachedHasQueueDrainCompletedSuccessfully || cachedStartedComplete)
    {
        // If the child drain async operation is not yet completed (!cachedCheckAllOperationAcked)
        // or if another thread has already started completing this async operation, we can NO-OP this call
        return;
    }

    // Wait until all operations are ACKed by the service
    {
        AcquireReadLock lock(parent_.queuesLock_);
        if (!parent_.copyReceiver_.AllOperationsAcked)
        {
            // There are operations committed, but not completed 
            // (given to the service, not ACKed yet)
            return;
        }
    }
    
    {
        AcquireWriteLock grab(lock_);
        if (startedComplete_)
        {
            // If the service has implemented multi-threaded pumping, there can be 2 threads that could reach this point simultanesouly.
            // the write lock lets through only 1 of them to trace out the event and schedule the completion of the async operation
            return;
        } 
        else
        {
            startedComplete_ = true;
        }
    }
    
    bool streamFaulted = isStreamFaulted_.load();

    ReplicatorEventSource::Events->SecondaryWaitForAcksDone(
        parent_.partitionId_,
        parent_.endpointUniqueId_,
        queueType_,
        drainScenario_,
        streamFaulted);

    Threadpool::Post([this, asyncOperation, streamFaulted]()
        {
            if (streamFaulted)
            {
                asyncOperation->TryComplete(asyncOperation, Common::ErrorCodeValue::OperationStreamFaulted);
            }
            else
            {
                asyncOperation->TryComplete(asyncOperation, Common::ErrorCodeValue::Success);
            }
        });
}

ErrorCode SecondaryReplicator::DrainQueueAsyncOperation::End(
    AsyncOperationSPtr const & asyncOperation)
{
    auto casted = AsyncOperation::End<DrainQueueAsyncOperation>(asyncOperation);
    return casted->Error;
}

} // end namespace ReplicationComponent
} // end namespace Reliability

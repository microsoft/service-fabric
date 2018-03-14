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
using Common::Threadpool;

SecondaryReplicator::CloseAsyncOperation::CloseAsyncOperation(
    __in SecondaryReplicator & parent,
    bool isPromoting,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
    :   AsyncOperation(callback, state, true),
        parent_(parent),
        isPromoting_(isPromoting)
{
}

// This runs under a lock, so do minimal work here
void SecondaryReplicator::CloseAsyncOperation::OnStart(AsyncOperationSPtr const &)
{
}

void SecondaryReplicator::CloseAsyncOperation::ResumeOutsideLock(Common::AsyncOperationSPtr const & thisSPtr)
{
    CopySenderSPtr cachedCopySender;
    bool shouldComplete = false;
    ErrorCode errorCode = Common::ErrorCodeValue::Success;

    if (isPromoting_)
    {
        // Close on ChangeRole, when the secondary is promoted to primary
        // Can only be called if the secondary is fully built,
        // so copy is already done.
        // For non-persisted services, this may mean that all operations
        // are completed, but the service didn't pick them up.
        // In that case, close waits for all copy operations to be 
        // taken by the service.

        std::wstring faultErrorMessage;
        bool waitForCopyQueueToDrain = false;
        bool abortReplicationQueue = false;

        {
            AcquireWriteLock queuesLock(parent_.queuesLock_);
            cachedCopySender = parent_.copySender_;

            if (parent_.requireServiceAck_)
            {
                // Copy is done when all operations are ACKed by the service,
                // so there can't be any pending items
                if (parent_.copyReceiver_.IsCopyInProgress())
                {
                    abortReplicationQueue = true;
                    parent_.replicationReceiver_.CleanQueueOnClose(parent_, true, true); // Since this replica will fault and abort, we can clear the items 
                    errorCode = Common::ErrorCodeValue::InvalidState;
                    faultErrorMessage = L"Can't Close secondary during promotion to primary if copy is in progress";
                }
            }
            else if (parent_.copyReceiver_.IsCopyInProgress())
            {
                if (!parent_.copyReceiver_.AllOperationsReceived)
                {
                    abortReplicationQueue = true;
                    parent_.replicationReceiver_.CleanQueueOnClose(parent_, true, true); // Since this replica will fault and abort, we can clear the items 
                    errorCode = Common::ErrorCodeValue::InvalidState;
                    faultErrorMessage = L"Can't Close secondary during promotion to primary if all copy operations are not received";
                }
                else 
                {
                    // Since all copy ops are received, we will wait for service to drain copy dispatch queue
                    waitForCopyQueueToDrain = true;
                }
            }
        }

        parent_.CloseDispatchQueues(false /*abortCopy*/, abortReplicationQueue /*abortRepl*/);
        
        if (errorCode.IsSuccess())
        {
            ReplicatorEventSource::Events->SecondaryClose(
                parent_.partitionId_,
                parent_.endpointUniqueId_,
                true,
                waitForCopyQueueToDrain);

            if (waitForCopyQueueToDrain)
            {
                WaitForCopyQueueToDrain(thisSPtr);
            }
            else 
            {
                WaitForReplicationQueueToDrain(thisSPtr);
            }
        }
        else 
        {
            shouldComplete = true;
            parent_.SetReportedFault(
                errorCode,
                faultErrorMessage);
        }
    }
    else
    {
        bool copyInProgress = parent_.copyReceiver_.IsCopyInProgress();
        ReplicatorEventSource::Events->SecondaryClose(
            parent_.partitionId_,
            parent_.endpointUniqueId_,
            false,
            copyInProgress);

        if (copyInProgress)
        {
            // Close when copy is in progress:
            // Repl dispatch queue:   abort dispatch queue.
            // Repl operation queue:  discard all non-completed operations.
            // Copy dispatch queue:   close and wait dispatch queue be drained (and ACK'ed for persisted services)
            // Copy operations queue: all out-of-order operations are discarded.
            // We're not waiting queues be drained before closing services, and service might stop pumping operations when close,
            // so we need to abort both copy and repl queues to avoid memory leak.
            parent_.CloseDispatchQueues(true /*abortCopy*/, true /*abortRepl*/);
            {
                AcquireWriteLock queuesLock(parent_.queuesLock_);
                cachedCopySender = parent_.copySender_;

                parent_.replicationReceiver_.CleanQueueOnClose(parent_, true /*clearItems*/, true/*IgnoreEOSAck*/);
                parent_.copyReceiver_.CleanQueueOnClose(parent_, true /*clearItems*/);
                parent_.copyReceiver_.CloseCopyOperationQueue();
            }
            /*TODO - 935703 - WaitForCopyQueueToDrain(thisSPtr);*/
            shouldComplete = true;
        }
        else
        {
            // Close when copy finished:
            // Repl dispatch queue:   close and wait dispatch queue be drained (and ACK'ed for persisted services)
            // Repl operations queue: all out-of-order operations are discarded.
            parent_.CloseDispatchQueues(true /*abortCopy*/, true /*abortRepl*/);
            {
                AcquireWriteLock queuesLock(parent_.queuesLock_);
                cachedCopySender = parent_.copySender_;

                parent_.replicationReceiver_.CleanQueueOnClose(parent_, true/*clearItems*/, true/*IgnoreEOSAck*/);
            }
            /*TODO - 935703 - WaitForReplicationQueueToDrain(thisSPtr);*/
            shouldComplete = true;
        }
    }

    parent_.ackSender_.Close();
    if (cachedCopySender)
    {
        cachedCopySender->FinishOperation(ErrorCode(Common::ErrorCodeValue::OperationCanceled));
    }

    if (shouldComplete)
    {
        if (errorCode.IsSuccess())
        {
            // We will wait for any pending Update Epoch Call that is running in the state provider before completing the close
            WaitForPendingUpdateEpochToFinish(thisSPtr);
        }
        else 
        {
            // Complete the close with an Error without waiting for any pending Update Epoch to complete.
            // The consequent abort(due to the error in close) should block on pending update epoch
            thisSPtr->TryComplete(thisSPtr, errorCode);
        }
    }
}

void SecondaryReplicator::CloseAsyncOperation::WaitForCopyQueueToDrain(
    AsyncOperationSPtr const & thisSPtr)
{
    AsyncOperationSPtr drainCopyOp;
    {
        AcquireWriteLock grab(parent_.queuesLock_);
        drainCopyOp = AsyncOperation::CreateAndStart<DrainQueueAsyncOperation>(
            parent_,
            parent_.requireServiceAck_,
            parent_.copyReceiver_.DispatchQueue,
            true, /* isCopyQueue */
            Constants::CopyOperationTrace,
            Constants::CloseDrainQueue,
            [this](AsyncOperationSPtr const & asyncOperation)
            {
                WaitForCopyQueueToDrainCallback(asyncOperation, false);
            },
            thisSPtr);
            
        ASSERT_IF(parent_.drainCopyOperationAsyncOperation_, "SecondaryReplicator drainCopyOperationAsyncOperation_ should be null");
        parent_.drainCopyOperationAsyncOperation_ = drainCopyOp;
    }
    AsyncOperation::Get<DrainQueueAsyncOperation>(drainCopyOp)->ResumeOutsideLock(drainCopyOp);
    WaitForCopyQueueToDrainCallback(drainCopyOp, true);
}

void SecondaryReplicator::CloseAsyncOperation::WaitForCopyQueueToDrainCallback(
    AsyncOperationSPtr const & asyncOperation,
    bool completedSynchronously)
{
    if (completedSynchronously == asyncOperation->CompletedSynchronously)
    {
        {
            AcquireWriteLock grab(parent_.queuesLock_);
            ASSERT_IFNOT(asyncOperation == parent_.drainCopyOperationAsyncOperation_, "SecondaryReplicator drainCopyOperationAsyncOperation_ mismatch");
            parent_.drainCopyOperationAsyncOperation_ = nullptr;
        }
        // Silently ignore error and continue to drain replication queue.
        auto casted = AsyncOperation::End<DrainQueueAsyncOperation>(asyncOperation);
        if (!casted->Error.IsError(Common::ErrorCodeValue::OperationCanceled))
        {
            WaitForReplicationQueueToDrain(asyncOperation->Parent);
        }
    }
}

void SecondaryReplicator::CloseAsyncOperation::WaitForReplicationQueueToDrain(
    AsyncOperationSPtr const & thisSPtr)
{
    AsyncOperationSPtr drainReplOp;
    {
        AcquireWriteLock grab(parent_.queuesLock_);
        drainReplOp = AsyncOperation::CreateAndStart<DrainQueueAsyncOperation>(
            parent_,
            parent_.requireServiceAck_,
            parent_.replicationReceiver_.DispatchQueue,
            false, /* isReplicationQueue */
            Constants::ReplOperationTrace,
            Constants::CloseDrainQueue,
            [this](AsyncOperationSPtr const & asyncOperation)
            {
                WaitForReplicationQueueToDrainCallback(asyncOperation, false);
            },
            thisSPtr);

        ASSERT_IF(parent_.drainReplOperationAsyncOperation_, "SecondaryReplicator drainReplOperationAsyncOperation_ should be null");
        parent_.drainReplOperationAsyncOperation_ = drainReplOp;
    }
    AsyncOperation::Get<DrainQueueAsyncOperation>(drainReplOp)->ResumeOutsideLock(drainReplOp);
    WaitForReplicationQueueToDrainCallback(drainReplOp, true);
}

void SecondaryReplicator::CloseAsyncOperation::WaitForReplicationQueueToDrainCallback(
    AsyncOperationSPtr const & asyncOperation,
    bool completedSynchronously)
{
    if (completedSynchronously == asyncOperation->CompletedSynchronously)
    {
        auto casted = AsyncOperation::End<DrainQueueAsyncOperation>(asyncOperation);
        ErrorCode error = casted->Error;
        {
            AcquireWriteLock grab(parent_.queuesLock_);
            ASSERT_IFNOT(asyncOperation == parent_.drainReplOperationAsyncOperation_, "SecondaryReplicator drainReplOperationAsyncOperation_ mismatch");
            parent_.drainReplOperationAsyncOperation_ = nullptr;
        }
        
        if (error.IsSuccess())
        {
            // We will wait for any pending Get copy Context Call that is running in the state provider before completing the close
            WaitForPendingGetCopyContextToFinish(asyncOperation->Parent);
        }
        else 
        {
            // Complete the close with an Error without waiting for any pending state provider call to complete.
            // The consequent abort(due to the error in close) should block 
            TryComplete(asyncOperation->Parent, error);
        }
    }
}

void SecondaryReplicator::CloseAsyncOperation::WaitForPendingGetCopyContextToFinish(
    AsyncOperationSPtr const & thisSPtr)
{
    AsyncOperationSPtr inner = parent_.finishedStateProviderGetCopyContext_->BeginWaitOne(
        Common::TimeSpan::MaxValue,
        [this](AsyncOperationSPtr const & asyncOperation)
        {
            WaitForPendingGetCopyContextToFinishCallback(asyncOperation, false);
        },
        thisSPtr);

        WaitForPendingGetCopyContextToFinishCallback(inner, true);    
}

void SecondaryReplicator::CloseAsyncOperation::WaitForPendingGetCopyContextToFinishCallback(
    AsyncOperationSPtr const & asyncOperation,
    bool completedSynchronously)
{
    if (completedSynchronously == asyncOperation->CompletedSynchronously)
    {
        ErrorCode error = parent_.finishedStateProviderGetCopyContext_->EndWaitOne(asyncOperation);

        if (error.IsSuccess())
        {
            // We will wait for any pending Update Epoch Call that is running in the state provider before completing the close
            WaitForPendingUpdateEpochToFinish(asyncOperation->Parent);
        }
        else
        {
            // Complete the close with an Error without waiting for any pending state provider call to complete.
            // The consequent abort(due to the error in close) should block 
            TryComplete(asyncOperation->Parent, error);
        }
    }
}

void SecondaryReplicator::CloseAsyncOperation::WaitForPendingUpdateEpochToFinish(
    AsyncOperationSPtr const & thisSPtr)
{
    std::shared_ptr<Common::AsyncManualResetEvent> event;

    {
        AcquireExclusiveLock grab(parent_.queuesLock_);
        event = parent_.finishedStateProviderUpdateEpoch_;
        parent_.doNotInvokeUEOnStateProvider_ = true;
    }

    AsyncOperationSPtr inner = event->BeginWaitOne(
        Common::TimeSpan::MaxValue,
        [this](AsyncOperationSPtr const & asyncOperation)
        {
            WaitForPendingUpdateEpochToFinishCallback(asyncOperation, false);
        },
        thisSPtr);

    WaitForPendingUpdateEpochToFinishCallback(inner, true);    
}

void SecondaryReplicator::CloseAsyncOperation::WaitForPendingUpdateEpochToFinishCallback(
    AsyncOperationSPtr const & asyncOperation,
    bool completedSynchronously)
{
    if (completedSynchronously == asyncOperation->CompletedSynchronously)
    {
        ErrorCode error = parent_.finishedStateProviderUpdateEpoch_->EndWaitOne(asyncOperation);

        TryComplete(asyncOperation->Parent, error);
    }
}

ErrorCode SecondaryReplicator::CloseAsyncOperation::End(
    AsyncOperationSPtr const & asyncOperation)
{
    auto casted = AsyncOperation::End<CloseAsyncOperation>(asyncOperation);
    return casted->Error;
}

} // end namespace ReplicationComponent
} // end namespace Reliability

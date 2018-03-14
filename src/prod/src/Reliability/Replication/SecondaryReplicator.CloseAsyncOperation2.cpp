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
using Common::make_unique;

SecondaryReplicator::CloseAsyncOperation2::CloseAsyncOperation2(
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
void SecondaryReplicator::CloseAsyncOperation2::OnStart(AsyncOperationSPtr const &)
{
}

void SecondaryReplicator::CloseAsyncOperation2::ResumeOutsideLock(Common::AsyncOperationSPtr const & thisSPtr)
{
    CopySenderSPtr cachedCopySender;
    bool shouldComplete = false;
    ErrorCode errorCode = Common::ErrorCodeValue::Success;
    
    if (isPromoting_)
    {
        std::wstring faultErrorMessage;
        bool waitForCopyQueueToDrain = false;

        {
            AcquireWriteLock queuesLock(parent_.queuesLock_);
            cachedCopySender = parent_.copySender_;

            if (parent_.copyReceiver_.IsCopyInProgress())
            {
                ReplicatorEventSource::Events->SecondaryClose(
                    parent_.partitionId_,
                    parent_.endpointUniqueId_,
                    true,
                    true);

                if (!parent_.copyReceiver_.AllOperationsReceived)
                {
                    // We report fault here. Do NOT enqueue EOS ops as the resulting close/CR->None will do that
                    errorCode = Common::ErrorCodeValue::InvalidState;
                    faultErrorMessage = L"Can't Close secondary during promotion to primary if all copy operations are not received";
                }
                else 
                {
                    // Since all copy ops are received, we will wait for service to drain copy dispatch queue
                    waitForCopyQueueToDrain = true;
                    parent_.EnqueueEndOfStreamOperations();
                    parent_.CloseDispatchQueues(false, false);
                }
            }
            else
            { 
                ReplicatorEventSource::Events->SecondaryClose(
                    parent_.partitionId_,
                    parent_.endpointUniqueId_,
                    true,
                    false);

                parent_.EnqueueEndOfStreamOperations();
                parent_.CloseDispatchQueues(false, false);
            }
        }
        
        if (errorCode.IsSuccess())
        {
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
    }/* end if promoting_*/
    else
    {
        bool copyInProgress = false;
        bool abortCopyDispatch = false;
        bool abortReplDispatch = false;

        {
            AcquireWriteLock queuesLock(parent_.queuesLock_);
            cachedCopySender = parent_.copySender_;

            if (parent_.copyReceiver_.IsCopyInProgress())
            {
                copyInProgress = true;

                ReplicatorEventSource::Events->SecondaryClose(
                    parent_.partitionId_,
                    parent_.endpointUniqueId_,
                    false,
                    true);

                if (!parent_.copyReceiver_.AllOperationsReceived)
                {
                    // This order is important:
                    // 1. Remove any pending replication operations that is not yet pumped by service -> This is needed so that service does NOT see REPL operations before completing copy
                    // 2. Enqueue EOS to both streams
                    // 3. Discard any replication operations that need ACKs
                    parent_.replicationReceiver_.DispatchQueue->ClearPendingItems();
                    parent_.EnqueueEndOfStreamOperations();
                    parent_.replicationReceiver_.CleanQueueOnClose(parent_, true/* clearItems */, false /*ignoreEOSOperationAck*/);
                }
            }
            else
            {
                ReplicatorEventSource::Events->SecondaryClose(
                    parent_.partitionId_,
                    parent_.endpointUniqueId_,
                    false,
                    false);
            }
            
            // This call is idempotent.
            parent_.EnqueueEndOfStreamOperations();

            // Check here if the streams have been initialized.If not, waiting for drain can get stuck forever
            // This is possible if the service never called "GetCopyStream" or "GetReplicationStream" and faulted its Change role call
            if (!parent_.copyReceiver_.IsStreamInitialized())
            {
                parent_.copyReceiver_.CleanQueueOnClose(parent_, true);
                abortCopyDispatch = true;
            }
            if (!parent_.replicationReceiver_.IsStreamInitialized())
            {
                parent_.replicationReceiver_.CleanQueueOnClose(parent_, true, true);
                abortReplDispatch = true;
            }
        }
        parent_.CloseDispatchQueues(abortCopyDispatch, abortReplDispatch);

        if (copyInProgress)
        {
            WaitForCopyQueueToDrain(thisSPtr);
        }
        else 
        {
            WaitForReplicationQueueToDrain(thisSPtr);
        }
    }

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

void SecondaryReplicator::CloseAsyncOperation2::WaitForCopyQueueToDrain(
    AsyncOperationSPtr const & thisSPtr)
{
    AsyncOperationSPtr drainCopyOp;
    {
        AcquireWriteLock grab(parent_.queuesLock_);
        drainCopyOp = AsyncOperation::CreateAndStart<DrainQueueAsyncOperation>(
            parent_,
            parent_.requireServiceAck_,
            parent_.copyReceiver_.DispatchQueue,
            true /*isCopyQueue*/,
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

void SecondaryReplicator::CloseAsyncOperation2::WaitForCopyQueueToDrainCallback(
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

        auto casted = AsyncOperation::End<DrainQueueAsyncOperation>(asyncOperation);
        if (casted->Error.IsSuccess())
        {
            WaitForReplicationQueueToDrain(asyncOperation->Parent);
        }
        else
        {
            // Complete the close with an Error without waiting for any pending Update Epoch to complete.
            // The consequent abort(due to the error in close) should block on pending update epoch
            TryComplete(asyncOperation->Parent, casted->Error);
        }
    }
}

void SecondaryReplicator::CloseAsyncOperation2::WaitForReplicationQueueToDrain(
    AsyncOperationSPtr const & thisSPtr)
{
    AsyncOperationSPtr drainReplOp;
    {
        AcquireWriteLock grab(parent_.queuesLock_);
        drainReplOp = AsyncOperation::CreateAndStart<DrainQueueAsyncOperation>(
            parent_,
            parent_.requireServiceAck_,
            parent_.replicationReceiver_.DispatchQueue,
            false /*isCopyQueue*/,
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

void SecondaryReplicator::CloseAsyncOperation2::WaitForReplicationQueueToDrainCallback(
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

void SecondaryReplicator::CloseAsyncOperation2::WaitForPendingGetCopyContextToFinish(
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

void SecondaryReplicator::CloseAsyncOperation2::WaitForPendingGetCopyContextToFinishCallback(
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

void SecondaryReplicator::CloseAsyncOperation2::WaitForPendingUpdateEpochToFinish(
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

void SecondaryReplicator::CloseAsyncOperation2::WaitForPendingUpdateEpochToFinishCallback(
    AsyncOperationSPtr const & asyncOperation,
    bool completedSynchronously)
{
    if (completedSynchronously == asyncOperation->CompletedSynchronously)
    {
        ErrorCode error = parent_.finishedStateProviderUpdateEpoch_->EndWaitOne(asyncOperation);

        TryComplete(asyncOperation->Parent, error);
    }
}

ErrorCode SecondaryReplicator::CloseAsyncOperation2::End(
    AsyncOperationSPtr const & asyncOperation)
{
    auto casted = AsyncOperation::End<CloseAsyncOperation2>(asyncOperation);
    return casted->Error;
}

} // end namespace ReplicationComponent
} // end namespace Reliability

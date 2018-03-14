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
using Common::ErrorCode;

SecondaryReplicator::UpdateEpochAsyncOperation::UpdateEpochAsyncOperation(
    __in SecondaryReplicator & parent,
    __in FABRIC_EPOCH const & epoch,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
    :   AsyncOperation(callback, state, true),
        parent_(parent),
        epoch_(epoch)
{
}

// This runs under a lock, so do minimal work here
void SecondaryReplicator::UpdateEpochAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (parent_.CheckReportedFault())
    {
        TryComplete(thisSPtr, Common::ErrorCodeValue::OperationFailed);
        return;
    }

    bool shouldComplete = true;
    ErrorCode error;

    {
        AcquireWriteLock lock(parent_.queuesLock_);
        if (epoch_ < parent_.minAllowedEpoch_)
        {
            ReplicatorEventSource::Events->SecondaryUpdateEpochError(
                parent_.partitionId_, 
                parent_.endpointUniqueId_, 
                epoch_.DataLossNumber,
                epoch_.ConfigurationNumber,
                parent_.minAllowedEpoch_.DataLossNumber,
                parent_.minAllowedEpoch_.ConfigurationNumber);
            error = Common::ErrorCodeValue::REInvalidEpoch;
        }
        else if (parent_.minAllowedEpoch_ < epoch_)
        {
            shouldComplete = false;
            parent_.replicationReceiver_.DiscardOutOfOrderOperations();
            ReplicatorEventSource::Events->SecondaryUpdateEpoch(
                    parent_.partitionId_, 
                    parent_.endpointUniqueId_, 
                    parent_.minAllowedEpoch_.DataLossNumber,
                    parent_.minAllowedEpoch_.ConfigurationNumber,
                    epoch_.DataLossNumber,
                    epoch_.ConfigurationNumber,
                    parent_.replicationReceiver_.GetNextToBeCompleted());
        
            parent_.minAllowedEpoch_ = epoch_;
            if (parent_.copyReceiver_.IsCopyInProgress())
            {
                // If copy is in progress, it indicates that the primary is down and failover's view of this replica is still "Idle" (Even though all copy operations
                // could have possibly been received).
                // So rather than waiting for a new primary to try building this replica and then reporting fault, pro-actively report fault here.
                parent_.SetReportedFault(
                    ErrorCode(Common::ErrorCodeValue::InvalidState),
                    L"Secondary received UpdateEpoch while copy is in progress");
                           
                shouldComplete = true;
                error = Common::ErrorCodeValue::InvalidState;
            }
        }
        // Else duplicate call, complete immediately.
    }

    if (shouldComplete)
    {
        TryComplete(thisSPtr, error);
    }
    else
    {
        WaitForReplicationQueueToDrain(thisSPtr);
    }
}

void SecondaryReplicator::UpdateEpochAsyncOperation::WaitForReplicationQueueToDrain(
    AsyncOperationSPtr const & thisSPtr)
{
    AsyncOperationSPtr drainReplOp;
    {
        AcquireWriteLock grab(parent_.queuesLock_);
        drainReplOp = AsyncOperation::CreateAndStart<DrainQueueAsyncOperation>(
            parent_,
            parent_.requireServiceAck_,
            parent_.replicationReceiver_.DispatchQueue,
            false /* isCopyQueue*/,
            Constants::ReplOperationTrace,
            Constants::UpdateEpochDrainQueue,
            [this](AsyncOperationSPtr const & asyncOperation)
            {
                this->WaitForReplicationQueueToDrainCallback(asyncOperation, false);
            },
            thisSPtr);

        ASSERT_IF(parent_.drainReplOperationAsyncOperation_, "SecondaryReplicator drainReplOperationAsyncOperation_ should be null");
        parent_.drainReplOperationAsyncOperation_ = drainReplOp;
    }
    AsyncOperation::Get<DrainQueueAsyncOperation>(drainReplOp)->ResumeOutsideLock(drainReplOp);
    WaitForReplicationQueueToDrainCallback(drainReplOp, true);
}

void SecondaryReplicator::UpdateEpochAsyncOperation::WaitForReplicationQueueToDrainCallback(
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

        if (error.IsError(Common::ErrorCodeValue::OperationCanceled))
        {
            // Report fault to indicate that the replicator is in a bad state (Queues not drained but epoch is updated in OnStart)
            parent_.SetReportedFault(
                ErrorCode(Common::ErrorCodeValue::InvalidState),
                L"Cancelling update epoch on secondary while waiting for dispatch queues to drain will result in an invalid state");
        }
    
        asyncOperation->Parent->TryComplete(asyncOperation->Parent, error);
    }
}

ErrorCode SecondaryReplicator::UpdateEpochAsyncOperation::End(
    AsyncOperationSPtr const & asyncOperation)
{
    auto casted = AsyncOperation::End<UpdateEpochAsyncOperation>(asyncOperation);
    return casted->Error;
}

} // end namespace ReplicationComponent
} // end namespace Reliability

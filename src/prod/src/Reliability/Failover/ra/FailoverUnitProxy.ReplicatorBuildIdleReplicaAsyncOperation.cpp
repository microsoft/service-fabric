// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Common::ApiMonitoring;

void FailoverUnitProxy::ReplicatorBuildIdleReplicaAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    AsyncOperationSPtr outstandingBuildOperationToCancel = nullptr;
    int64 replicaIdToRemove = Constants::InvalidReplicaId;

    {
        AcquireExclusiveLock grab(owner_.lock_);

        owner_.TraceBeforeOperation(
            grab, 
            ApiMonitoring::InterfaceName::IReplicator, 
            ApiMonitoring::ApiName::BuildReplica, 
            TraceCorrelatedEvent<Reliability::ReplicaDescription>(idleReplicaDescription_));

        ASSERT_IFNOT(owner_.IsStatefulServiceFailoverUnitProxy(), "Attempt to build idle replica using replicator on an FUP that is not hosting a stateful service");
        ASSERT_IFNOT(owner_.replicator_, "Attempt to build idle replica using replicator when replicator is invalid");
        ASSERT_IFNOT(owner_.replicatorState_ == ReplicatorStates::Opened, "Call to build idle replica for a replicator that isn't in the opened state.");
        ASSERT_IF(owner_.replicatorOperationManager_ == nullptr, "Replicator operation manager expected but not available.");

        // Validate that the replica and replicator roles are correct to be able to build this Idle replica
        if (owner_.currentServiceRole_ != ReplicaRole::Primary || owner_.currentReplicatorRole_ != ReplicaRole::Primary)
        {
            auto error = ErrorCodeValue::RAProxyOperationIncompatibleWithCurrentFupState;

            bool didComplete = thisSPtr->TryComplete(thisSPtr, error);
            ASSERT_IFNOT(didComplete, "Failed to complete BuildIdleReplicaAsyncOperation");
            return;
        }

        if (owner_.currentReplicaState_ != ReplicaStates::Ready)
        {
            bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::RAProxyOperationIncompatibleWithCurrentFupState);
            ASSERT_IFNOT(didComplete, "Failed to complete BuildIdleReplicaAsyncOperation");
            return;
        }

        // Search for the target in the list of remote replicas
        auto entry = owner_.remoteReplicas_.find(idleReplicaDescription_.FederationNodeId);
        
        // Found an entry in the remote replicas list for the replica?
        if (entry != owner_.remoteReplicas_.end())
        {
            bool shouldCompleteOp = false;
            bool isIncomptibleState = false;

            ErrorCodeValue::Enum returnCode = ErrorCodeValue::Success;

            ReplicaProxy & thisReplica = entry->second;

            switch (thisReplica.ReplicatorState)
            {
                case ReplicaProxyStates::InBuild:
                    if (thisReplica.ReplicaDescription.InstanceId >= idleReplicaDescription_.InstanceId)
                    {
                        // Stale or duplicate message, suppress reply
                        isIncomptibleState = true;
                    }
                    else
                    {
                        // This is the case where we got a request with a higher instance id than a pending request
                        // We will need to cancel the build and restart it
                        // So, we cancel it now, suppress reply, and wait for the retry to restart it

                        if (thisReplica.BuildIdleOperation.AsyncOperation)
                        {
                            outstandingBuildOperationToCancel = thisReplica.BuildIdleOperation.AsyncOperation;
                            shouldCompleteOp = false; // Just being clear...
                        }
                        else
                        {
                            // We need to cancel the build, but it hasn't started yet
                            // We can try again at retry (suppress reply)
                            isIncomptibleState = true;
                        }
                    }
                    break;
                case ReplicaProxyStates::Ready:
                    if (thisReplica.ReplicaDescription.InstanceId == idleReplicaDescription_.InstanceId)
                    {
                        // Duplicate message
                        // We have already done this build
                        shouldCompleteOp = true;
                        returnCode = ErrorCodeValue::Success;
                    }
                    else if (thisReplica.ReplicaDescription.InstanceId < idleReplicaDescription_.InstanceId)
                    {
                        // Target has a higher instance id than the built replica
                        // We need to remove so the build can proceed on retry
                        // Check that we can do the remove now
                        if (owner_.replicatorOperationManager_->TryStartOperation(ApiMonitoring::ApiName::RemoveReplica))

                        {
                            thisReplica.ReplicatorState = ReplicaProxyStates::InDrop;
                            replicaIdToRemove = thisReplica.ReplicaDescription.ReplicaId;
                            shouldCompleteOp = false;
                        }
                        else
                        {
                            // Drop operation not allowed right now (replicator busy)
                            // Fail and suppress reply
                            isIncomptibleState = true;                            
                        }
                    }
                    else
                    {
                        // Stale message, ignore
                        isIncomptibleState = true;
                    }
                    break;
                case ReplicaProxyStates::InDrop:
                    if (thisReplica.ReplicaDescription.InstanceId <= idleReplicaDescription_.InstanceId)
                    {
                        // It is already marked for drop (from previous attempt), so 
                        // retry the drop

                        // Check that we can do the remove now
                        if (owner_.replicatorOperationManager_->TryStartOperation(ApiMonitoring::ApiName::RemoveReplica))

                        {
                            replicaIdToRemove = thisReplica.ReplicaDescription.ReplicaId;
                            shouldCompleteOp = false;
                        }
                        else
                        {
                            // Drop operation not allowed right now (replicator busy)
                            // Fail and suppress reply
                            isIncomptibleState = true;                            
                        }
                    }
                    else
                    {
                        // Wait for the drop to happen before we can build
                        isIncomptibleState = true;                        
                    }
                    break;
                case ReplicaProxyStates::Dropped:
                    if (thisReplica.ReplicaDescription.InstanceId > idleReplicaDescription_.InstanceId)
                    {
                        // Stale message, fail and suppress reply
                        isIncomptibleState = true;                        
                    }
                    else
                    {
                        if(thisReplica.ReplicaDescription.InstanceId == idleReplicaDescription_.InstanceId &&
                           thisReplica.FailoverUnitProxyState == ReplicaProxyStates::Dropped)
                        {
                            // This replica has alread been dropped by the system, ignore stale message
                            isIncomptibleState = true;
                        }
                        else
                        {
                            if (owner_.replicatorOperationManager_->TryStartMultiInstanceOperation(ApiName::BuildReplica, wformatString(idleReplicaDescription_.ReplicaId), owner_.ReplicaDescription, thisReplica.BuildIdleOperation))
                            {
                                // Reusing an entry from a previously removed idle
                                // Mark the entry as InBuild
                                thisReplica.State = ReplicaProxyStates::InBuild; 
                                thisReplica.ReplicaDescription = idleReplicaDescription_;

                                shouldCompleteOp = false; // Just being clear...
                            }
                            else
                            {
                                // Operation not allowed right now (replicator busy)
                                // Fail and suppress reply
                                isIncomptibleState = true;                            
                            }
                        }
                    }
                    break;
            }

            if (isIncomptibleState)
            {
                shouldCompleteOp = true;
                returnCode = ErrorCodeValue::RAProxyOperationIncompatibleWithCurrentFupState;            
            }
            
            if (shouldCompleteOp)
            {
                bool didComplete = thisSPtr->TryComplete(thisSPtr, returnCode);
                ASSERT_IFNOT(didComplete, "Failed to complete ReplicatorBuildIdleReplicaAsyncOperation");
                return;
            }
        }
        else
        {
            ReplicaProxy replicaProxy(idleReplicaDescription_);
            replicaProxy.State = ReplicaProxyStates::InBuild;

            // Add it to the list
            if (!owner_.replicatorOperationManager_->TryStartMultiInstanceOperation(ApiName::BuildReplica, wformatString(idleReplicaDescription_.ReplicaId), owner_.ReplicaDescription, replicaProxy.BuildIdleOperation))
            {
                bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::RAProxyOperationIncompatibleWithCurrentFupState);
                ASSERT_IFNOT(didComplete, "Failed to complete BuildIdleReplicaAsyncOperation");
                return;
            }

            owner_.remoteReplicas_.insert(std::pair<Federation::NodeId, ReplicaProxy>(idleReplicaDescription_.FederationNodeId, std::move(replicaProxy)));
        }
    }

    // If doing a cancel instead of build (rebuild case), do that and return
    if (outstandingBuildOperationToCancel)
    {
        outstandingBuildOperationToCancel->Cancel();
        bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::RAProxyOperationIncompatibleWithCurrentFupState);
        ASSERT_IFNOT(didComplete, "Failed to complete ReplicatorBuildIdleReplicaAsyncOperation");
        return;
    }

    // If doing a remove instead of a build (another rebuild case), do that and return
    if (replicaIdToRemove != Constants::InvalidReplicaId)
    {
        ErrorCode removeResult = owner_.replicator_->RemoveIdleReplica(replicaIdToRemove);

        {
            AcquireExclusiveLock grab(owner_.lock_);

            auto entry = owner_.remoteReplicas_.find(idleReplicaDescription_.FederationNodeId);
            ASSERT_IF(entry == owner_.remoteReplicas_.end(), "Replica wasn't found in the list of remote replicas after remove completed.");

            ReplicaProxy & replicaToRemove = entry->second;

            ErrorCodeValue::Enum retValue = removeResult.ReadValue();
            if (retValue == ErrorCodeValue::Success || retValue == ErrorCodeValue::REReplicaDoesNotExist)
            {
                // Mark as dropped if we successfully removed it
                replicaToRemove.ReplicatorState = ReplicaProxyStates::Dropped;
            }

            // Finish the remove with the operation manager
            owner_.replicatorOperationManager_->FinishOperation(ApiMonitoring::ApiName::RemoveReplica, removeResult);

        }

        // Now suppress the reply
        bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::RAProxyOperationIncompatibleWithCurrentFupState);
        ASSERT_IFNOT(didComplete, "Failed to complete ReplicatorBuildIdleReplicaAsyncOperation");
        return;
    }

    // Make the call to build the idle replica
    AsyncOperationSPtr operation = owner_.replicator_->BeginBuildIdleReplica(
        idleReplicaDescription_,
        [this] (AsyncOperationSPtr const & operation) { this->ReplicatorBuildIdleReplicaCompletedCallback(operation); },
        thisSPtr);

    bool cancel = false;

    {
        AcquireExclusiveLock grab(owner_.lock_);

        // Update the replica with the operation
        auto entry = owner_.remoteReplicas_.find(idleReplicaDescription_.FederationNodeId);
        ASSERT_IF(entry == owner_.remoteReplicas_.end(), "Replica not found when attempting to update for pending build idle replica.");

        ReplicaProxy & idleReplica = entry->second;

        switch (idleReplica.ReplicatorState)
        {
            case ReplicaProxyStates::InBuild:
                cancel = !operation->IsCompleted && 
                    !owner_.replicatorOperationManager_->TryContinueMultiInstanceOperation(ApiName::BuildReplica, idleReplica.BuildIdleOperation, operation);
                break;
            case ReplicaProxyStates::Ready:
                // We lost the race with the callback, do nothing
                break;
            case ReplicaProxyStates::InDrop:
            case ReplicaProxyStates::Dropped:
                // A drop started since we start this call
                // We are responsible for cancel
                // Set the flag and cancel outside the lock
                cancel = !operation->IsCompleted;
                break;
        }
    }

    if (cancel)
    {
        operation->Cancel();
    }

    if (operation->CompletedSynchronously)
    {
        FinishReplicatorBuildIdleReplica(operation);
    }
}

ErrorCode FailoverUnitProxy::ReplicatorBuildIdleReplicaAsyncOperation::End(AsyncOperationSPtr const & asyncOperation)
{
    auto thisPtr = AsyncOperation::End<FailoverUnitProxy::ReplicatorBuildIdleReplicaAsyncOperation>(asyncOperation);
    return thisPtr->Error;
}

void FailoverUnitProxy::ReplicatorBuildIdleReplicaAsyncOperation::ReplicatorBuildIdleReplicaCompletedCallback(AsyncOperationSPtr const & buildIdleReplicaAsyncOperation)
{
    if (!buildIdleReplicaAsyncOperation->CompletedSynchronously)
    {
        FinishReplicatorBuildIdleReplica(buildIdleReplicaAsyncOperation);
    }
}

void FailoverUnitProxy::ReplicatorBuildIdleReplicaAsyncOperation::FinishReplicatorBuildIdleReplica(AsyncOperationSPtr const & buildIdleReplicaAsyncOperation)
{    
    ErrorCode errorCode = owner_.replicator_->EndBuildIdleReplica(buildIdleReplicaAsyncOperation);
     
    AsyncOperationSPtr const & thisSPtr = buildIdleReplicaAsyncOperation->Parent;

    {
        AcquireExclusiveLock grab(owner_.lock_);
            
        // Succeeded, so ensure it is added to / updated in the list of remote replicas
        auto entry = owner_.remoteReplicas_.find(idleReplicaDescription_.FederationNodeId);
        ASSERT_IF(entry == owner_.remoteReplicas_.end(), "Replica not found after build completed.");

        ReplicaProxy & thisReplica = entry->second;

        // Was the build successful?
        // The state will be set to drop if build is canceled just before callback is called
        if(thisReplica.ReplicatorState == ReplicaProxyStates::InBuild)
        {
            if (errorCode.IsSuccess())
            {
                // Finished, update state
                thisReplica.State = ReplicaProxyStates::Ready; 
            }
            else
            {
                // Failed, mark proxy's replicatorstate as in-drop
                thisReplica.ReplicatorState = ReplicaProxyStates::InDrop;
            }
        }
        else
        {
            errorCode.ReadValue();
            errorCode = ErrorCodeValue::RAProxyOperationIncompatibleWithCurrentFupState;
        }

        owner_.replicatorOperationManager_->FinishMultiInstanceOperation(ApiName::BuildReplica, thisReplica.BuildIdleOperation, errorCode);
    }

    bool didComplete = thisSPtr->TryComplete(thisSPtr, errorCode);
    ASSERT_IFNOT(didComplete, "Failed to complete ReplicatorBuildIdleReplicaAsyncOperation");
}

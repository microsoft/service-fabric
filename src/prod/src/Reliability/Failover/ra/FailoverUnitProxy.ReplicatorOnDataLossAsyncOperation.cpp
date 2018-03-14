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

void FailoverUnitProxy::ReplicatorOnDataLossAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    {
        AcquireExclusiveLock grab(owner_.lock_);

        TraceBeforeStart(grab);

        ASSERT_IF(owner_.IsStatelessServiceFailoverUnitProxy(), "Call to OnDataLoss on an FUP hosting a stateless service.");
        ASSERT_IF(owner_.replicaState_ != FailoverUnitProxyStates::Opened, "Call to OnDataLoss for a closed FUP");
        ASSERT_IF(owner_.currentReplicatorRole_ != ReplicaRole::Primary, "Call to OnDataLoss for a replicator that is not primary.");
        ASSERT_IF(owner_.currentServiceRole_!= ReplicaRole::Primary, "Call to OnDataLoss for a replica that is not primary.");
        ASSERT_IF(owner_.replicatorOperationManager_ == nullptr, "Replicator operation manager expected but not available.");

        bool isDataLoss = owner_.ReplicaDescription.CurrentConfigurationRole == ReplicaRole::Primary &&
            owner_.FailoverUnitDescription.PreviousConfigurationEpoch != Epoch::InvalidEpoch() &&
            owner_.FailoverUnitDescription.IsDataLossBetweenPCAndCC;
        if (!isDataLoss)
        {
            // The replica is now ready
            owner_.currentReplicaState_ = ReplicaStates::Ready;

            bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            ASSERT_IFNOT(didComplete, "Failed to complete ReplicatorOnDataLossAsyncOperation");
            return;
        }

        ASSERT_IF(owner_.currentReplicaState_ != ReplicaStates::InBuild, "Primary must be IB {0}", owner_);

        RAPEventSource::Events->ActionDataLoss(
            owner_.FailoverUnitId.Guid,
            owner_.RAPId,
            owner_.FailoverUnitDescription.PreviousConfigurationEpoch,
            owner_.FailoverUnitDescription.CurrentConfigurationEpoch);

        owner_.AssertCatchupNotStartedCallerHoldsLock();

        if (!TryStartUserApi(grab, thisSPtr))
        {
            return;
        }
    }

    AsyncOperationSPtr operation = owner_.replicator_->BeginOnDataLoss(
        [this] (AsyncOperationSPtr const & operation) { this->ReplicatorOnDataLossCompletedCallback(operation); },
        thisSPtr);

    TryContinueUserApi(operation);

    if (operation->CompletedSynchronously)
    {
        FinishReplicatorOnDataLoss(operation);
    }
}

ProxyErrorCode FailoverUnitProxy::ReplicatorOnDataLossAsyncOperation::End(AsyncOperationSPtr const & asyncOperation, int64 & lastLSN)
{
    auto thisPtr = AsyncOperation::End<FailoverUnitProxy::ReplicatorOnDataLossAsyncOperation>(asyncOperation);
    lastLSN = thisPtr->lastLSN_;
    return thisPtr->EndHelper();
}

void FailoverUnitProxy::ReplicatorOnDataLossAsyncOperation::ReplicatorOnDataLossCompletedCallback(AsyncOperationSPtr const & onDataLossAsyncOperation)
{
    if (!onDataLossAsyncOperation->CompletedSynchronously)
    {
        FinishReplicatorOnDataLoss(onDataLossAsyncOperation);
    }
}

void FailoverUnitProxy::ReplicatorOnDataLossAsyncOperation::FinishReplicatorOnDataLoss(AsyncOperationSPtr const & onDataLossAsyncOperation)
{    
    bool isStateChanged = false;
    ErrorCode errorCode = owner_.replicator_->EndOnDataLoss(onDataLossAsyncOperation, isStateChanged);
    
    {
        AcquireExclusiveLock grab(owner_.lock_);
        FinishUserApi(grab, errorCode);

        if (errorCode.IsSuccess())
        {
            // If this replica is the primary, transition it to RD now
            // It can service requests for build etc
            ASSERT_IF(owner_.currentServiceRole_ != ReplicaRole::Primary, "Cannot be non primary");
            owner_.currentReplicaState_ = ReplicaStates::Ready;
        }
    }

    // RF and complete outside the lock
    if (!errorCode.IsSuccess())
    {
        // OnDataLoss has failed. Not much we can do. Restart and have another replica try this
        owner_.ReportFault(FaultType::Transient);

        AsyncOperationSPtr const & thisSPtr = onDataLossAsyncOperation->Parent;
        bool didComplete = thisSPtr->TryComplete(thisSPtr, errorCode);
        ASSERT_IFNOT(didComplete, "Failed to complete ReplicatorOnDataLossAsyncOperation");
        return;
    }

    errorCode = HandleStateChangeOnDataLoss(isStateChanged);

    AsyncOperationSPtr const & thisSPtr = onDataLossAsyncOperation->Parent;
    bool didComplete = thisSPtr->TryComplete(thisSPtr, errorCode);
    ASSERT_IFNOT(didComplete, "Failed to complete ReplicatorOnDataLossAsyncOperation");
}

ErrorCode FailoverUnitProxy::ReplicatorOnDataLossAsyncOperation::HandleStateChangeOnDataLoss(bool isStateChanged)
{
    // If it was state changed on data loss then we need the last LSN from the replicator
    if (!isStateChanged)
    {
        return ErrorCode::Success();
    }

    /*
        A note about treating ReplicatorGetStatus as a user API after OnDataLoss returns that state has changed

        ReplicatorGetStatus is a user api and 
        the OnDataLossAsyncOp breaks the convention in the RAP where every async op
        represents a single user API call. In the state change case the OnDataLoss invokes
        two user API calls.

        The reason for this is that the action list itself will terminate because 
        when OnDataLoss fails the contract is for RAP to return an error code.
        There is no support for conditional behavior in action lists such that if
        OnDataLoss returns !stateChanged then proceed to catchup else perform GetLSN
        This could be done by adding GetStatus as an action to the list and looking at 
        the catchup result and executing that action and terminating that list there
    */
    int64 firstLSN = 0;
    auto errorCode = owner_.ReplicatorGetStatus(firstLSN, lastLSN_);
    if (!errorCode.IsSuccess())
    {
        owner_.ReportFault(FaultType::Transient); // Not much we can do in terms of handling this
        return errorCode.ErrorCode;
    }
    else
    {
        AcquireExclusiveLock grab(owner_.lock_);
        owner_.catchupResult_ = CatchupResult::DataLossReported;
        owner_.replicaDescription_.LastAcknowledgedLSN = lastLSN_; // For retries

        /*
            Update Read/Write status here as the action list will terminate as an error code is being returned
        */
        owner_.UpdateReadAndWriteStatus(grab);

        return ErrorCodeValue::RAProxyStateChangedOnDataLoss;
    }
}

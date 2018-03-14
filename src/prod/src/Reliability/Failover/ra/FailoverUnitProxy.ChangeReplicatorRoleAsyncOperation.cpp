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

void FailoverUnitProxy::ChangeReplicatorRoleAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    Epoch primaryEpoch;
    FABRIC_REPLICA_ROLE newRole;
    ProxyOutgoingMessageUPtr rwStatusNotificationMsg;

    {
        AcquireExclusiveLock grab(owner_.lock_);

        TraceBeforeStart(grab);

        ASSERT_IF(owner_.failoverUnitDescription_.CurrentConfigurationEpoch == Epoch::InvalidEpoch(), "Epoch passed to replicator must be valid.");

        primaryEpoch = owner_.failoverUnitDescription_.CurrentConfigurationEpoch.ToPrimaryEpoch();

        if(owner_.replicatorState_ == ReplicatorStates::Closed)
        {
            bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            ASSERT_IFNOT(didComplete, "Failed to complete ChangeReplicatorRoleAsyncOperation");

            return;
        }

        ASSERT_IFNOT(owner_.serviceDescription_.IsStateful, "Attempt to change role on an FUP that is not hosting a stateful service");
        
        if (owner_.currentReplicatorRole_ == owner_.replicaDescription_.CurrentConfigurationRole)
        {
            bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            ASSERT_IFNOT(didComplete, "Failed to complete ChangeReplicatorRoleAsyncOperation");

            return;
        }

        ASSERT_IFNOT(owner_.replicatorState_ == ReplicatorStates::Opened, "Attempt to change role on a replicator not in the Opened state.");
       
        ASSERT_IF(owner_.replicatorOperationManager_ == nullptr, "Replicator operation manager expected but not available.");

        if (!TryStartUserApi(grab, wformatString(owner_.replicaDescription_.CurrentConfigurationRole), thisSPtr))
        {
            return;
        }

        previousRole_ = owner_.currentReplicatorRole_;
        newRole = ReplicaRole::ConvertToPublicReplicaRole(owner_.replicaDescription_.CurrentConfigurationRole);

        if (owner_.replicaDescription_.CurrentConfigurationRole == ReplicaRole::None)
        {
            rwStatusNotificationMsg = owner_.ComposeReadWriteStatusRevokedNotification(grab);
        }
    }

    owner_.SendReadWriteStatusRevokedNotification(move(rwStatusNotificationMsg));

    // Start changing the replicator role

    AsyncOperationSPtr operation = owner_.replicator_->BeginChangeRole(
        primaryEpoch.ToPublic(),
        newRole,
        [this] (AsyncOperationSPtr const & operation) { this->ChangeRoleCompletedCallback(operation); },
        thisSPtr);

    TryContinueUserApi(operation);

    if (operation->CompletedSynchronously)
    {
        FinishChangeRole(operation);
    }
}

ProxyErrorCode FailoverUnitProxy::ChangeReplicatorRoleAsyncOperation::End(AsyncOperationSPtr const & asyncOperation)
{
    auto thisPtr = AsyncOperation::End<FailoverUnitProxy::ChangeReplicatorRoleAsyncOperation>(asyncOperation);
    return thisPtr->EndHelper();
}

void FailoverUnitProxy::ChangeReplicatorRoleAsyncOperation::ChangeRoleCompletedCallback(AsyncOperationSPtr const & changeRoleAsyncOperation)
{
    if (!changeRoleAsyncOperation->CompletedSynchronously)
    {
        FinishChangeRole(changeRoleAsyncOperation);
    }
}

void FailoverUnitProxy::ChangeReplicatorRoleAsyncOperation::FinishChangeRole(AsyncOperationSPtr const & changeRoleAsyncOperation)
{   
    ErrorCode errorCode = owner_.replicator_->EndChangeRole(changeRoleAsyncOperation);

    bool registerForCleanup = false;
    bool unregisterForCleanup = false;

    {
        AcquireExclusiveLock grab(owner_.lock_);

        if (errorCode.IsSuccess())
        {
            owner_.currentReplicatorRole_ = owner_.replicaDescription_.CurrentConfigurationRole;
            owner_.lastUpdateEpochPrimaryEpochValue_ = owner_.failoverUnitDescription_.CurrentConfigurationEpoch.ToPrimaryEpoch();

            if (owner_.currentReplicatorRole_ == ReplicaRole::Primary)
            {
                registerForCleanup = true;
            }
            else
            {
               // Clear the replica list for non-primary
                owner_.remoteReplicas_.clear();
                owner_.configurationReplicas_.clear();
                unregisterForCleanup = true;
            }
        }
        else
        {
            owner_.currentReplicatorRole_ = previousRole_;
        }
    
        FinishUserApi(grab, errorCode);
    }

    if (registerForCleanup)
    {
        owner_.RegisterFailoverUnitProxyForCleanupEvent();
    }
    else if(unregisterForCleanup)
    {
        owner_.UnregisterFailoverUnitProxyForCleanupEvent();
    }

    AsyncOperationSPtr const & thisSPtr = changeRoleAsyncOperation->Parent;
    bool didComplete = thisSPtr->TryComplete(thisSPtr, errorCode);
    ASSERT_IFNOT(didComplete, "Failed to complete ChangeReplicatorRoleAsyncOperation");
}

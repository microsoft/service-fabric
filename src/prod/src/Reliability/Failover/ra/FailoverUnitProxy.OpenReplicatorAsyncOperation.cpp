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

void FailoverUnitProxy::OpenReplicatorAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    {
        AcquireExclusiveLock grab(owner_.lock_);

        TraceBeforeStart(grab);

        if (owner_.isMarkedForAbort_ && owner_.replicaState_ == ReplicatorStates::Closed)
        {
            // If opening the replica to call CR(None) during RF permanent fails then let open of replicator go through to finish the aciton list
            bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            ASSERT_IFNOT(didComplete, "Failed to complete ChangeReplicaRoleAsyncOperation");

            return;
        }

        ASSERT_IFNOT(owner_.IsStatefulServiceFailoverUnitProxy(), "Attempt to open replicator on an FUP that is not hosting a stateful service");
        ASSERT_IFNOT(owner_.replicator_, "Attempt to open replicator when replicator is invalid");
    
        if(owner_.replicatorState_ == ReplicatorStates::Opened)
        {
            // Already open
            // Replication endpoint is unchanged
            replicationEndpoint_ = owner_.replicaDescription_.ReplicationEndpoint;

            bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            ASSERT_IFNOT(didComplete, "Failed to complete OpenReplicatorAsyncOperation");

            return;
        }

        if (!owner_.replicatorOperationManager_)
        {
            owner_.replicatorOperationManager_ = make_unique<ReplicatorOperationManager>(owner_);
        }

        ASSERT_IF(owner_.replicatorOperationManager_ == nullptr, "Replicator operation manager expected but not available.");

        if (!TryStartUserApi(grab, thisSPtr))
        {
            return;
        }

        TESTASSERT_IF(owner_.currentReplicatorRole_ != ReplicaRole::Unknown, "Current replicator role mismatch {0}", owner_);
        owner_.replicatorState_ = ReplicatorStates::Opening;
    }

    AsyncOperationSPtr operation = owner_.replicator_->BeginOpen(
        [this] (AsyncOperationSPtr const & operation) { this->OpenReplicatorCompletedCallback(operation); },
        thisSPtr);

    TryContinueUserApi(operation);

    if (operation->CompletedSynchronously)
    {
        FinishOpenReplicator(operation);
    }
}

ProxyErrorCode FailoverUnitProxy::OpenReplicatorAsyncOperation::End(AsyncOperationSPtr const & asyncOperation, __out std::wstring & replicationEndpoint)
{
    auto thisPtr = AsyncOperation::End<FailoverUnitProxy::OpenReplicatorAsyncOperation>(asyncOperation);
    replicationEndpoint = thisPtr->replicationEndpoint_;
    return thisPtr->EndHelper();
}

void FailoverUnitProxy::OpenReplicatorAsyncOperation::OpenReplicatorCompletedCallback(AsyncOperationSPtr const & openAsyncOperation)
{
    if (!openAsyncOperation->CompletedSynchronously)
    {
        FinishOpenReplicator(openAsyncOperation);
    }
}

void FailoverUnitProxy::OpenReplicatorAsyncOperation::FinishOpenReplicator(AsyncOperationSPtr const & openAsyncOperation)
{    
    ErrorCode errorCode = owner_.replicator_->EndOpen(openAsyncOperation, replicationEndpoint_);

    {
        AcquireExclusiveLock grab(owner_.lock_);

        if (errorCode.IsSuccess())
        {
            // Replicator is now opened
            owner_.replicatorState_ = ReplicatorStates::Opened;
            owner_.currentReplicatorRole_ = ReplicaRole::Unknown;
    
            owner_.replicaDescription_.ReplicationEndpoint = replicationEndpoint_;

            RAPEventSource::Events->FailoverUnitProxyEndpointUpdate(
                owner_.FailoverUnitId.Guid,
                owner_.RAPId,
                interface_,
                replicationEndpoint_);
        }
        else
        {
            owner_.replicator_ = nullptr;
            owner_.replicatorState_ = ReplicatorStates::Closed;
        }

        FinishUserApi(grab, errorCode);
    }

    AsyncOperationSPtr const & thisSPtr = openAsyncOperation->Parent;
    bool didComplete = thisSPtr->TryComplete(thisSPtr, errorCode);
    ASSERT_IFNOT(didComplete, "Failed to complete OpenReplicatorAsyncOperation");
}

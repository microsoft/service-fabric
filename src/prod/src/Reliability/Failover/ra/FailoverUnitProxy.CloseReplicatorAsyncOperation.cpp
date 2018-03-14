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

void FailoverUnitProxy::CloseReplicatorAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    ProxyOutgoingMessageUPtr rwStatusNotificationMsg;

    {
        AcquireExclusiveLock grab(owner_.lock_);

        TraceBeforeStart(grab);

        if (owner_.replicatorState_ == ReplicatorStates::Closed)
        {
            bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            ASSERT_IFNOT(didComplete, "Failed to complete CloseReplicatorAsyncOperation");

            return;
        }

        ASSERT_IF(owner_.replicatorState_ != ReplicatorStates::Opened, "Attempt to close a replicator service not in Opened or Closed state.");

        ASSERT_IFNOT(owner_.replicator_, "Replicator null when attempting to close replicator.");
        ASSERT_IF(owner_.replicatorOperationManager_ == nullptr, "Replicator operation manager expected but not available.");

        if (!TryStartUserApi(grab, thisSPtr))
        {
            return;
        }

        owner_.replicatorState_ = ReplicatorStates::Closing;
        
        if (owner_.replicaDescription_.CurrentConfigurationRole == ReplicaRole::None)
        {
            rwStatusNotificationMsg = owner_.ComposeReadWriteStatusRevokedNotification(grab);
        }
    }

    owner_.SendReadWriteStatusRevokedNotification(move(rwStatusNotificationMsg));

    AsyncOperationSPtr operation = owner_.replicator_->BeginClose(
        [this] (AsyncOperationSPtr const & operation) { this->CloseReplicatorCompletedCallback(operation); },
        thisSPtr);

    TryContinueUserApi(operation);

    if (operation->CompletedSynchronously)
    {
        FinishCloseReplicator(operation);
    }
}

ProxyErrorCode FailoverUnitProxy::CloseReplicatorAsyncOperation::End(AsyncOperationSPtr const & asyncOperation)
{
    auto thisPtr = AsyncOperation::End<FailoverUnitProxy::CloseReplicatorAsyncOperation>(asyncOperation);
    return thisPtr->EndHelper();
}

void FailoverUnitProxy::CloseReplicatorAsyncOperation::CloseReplicatorCompletedCallback(AsyncOperationSPtr const & closeAsyncOperation)
{
    if (!closeAsyncOperation->CompletedSynchronously)
    {
        FinishCloseReplicator(closeAsyncOperation);
    }
}

void FailoverUnitProxy::CloseReplicatorAsyncOperation::FinishCloseReplicator(AsyncOperationSPtr const & closeAsyncOperation)
{    
    ErrorCode errorCode = owner_.replicator_->EndClose(closeAsyncOperation);

    bool unregisterForCleanup = false;

    {
        AcquireExclusiveLock grab(owner_.lock_);

        if (errorCode.IsSuccess())
        {
            // Replicator is now closed
            owner_.TransitionReplicatorToClosed(grab);
            owner_.replicator_ = nullptr;
            owner_.remoteReplicas_.clear();
            owner_.configurationReplicas_.clear();
            unregisterForCleanup = true;
        }
        else
        {
            owner_.replicatorState_ = ReplicatorStates::Opened;
        }

        FinishUserApi(grab, errorCode);
    }

    if (unregisterForCleanup)
    {
        owner_.UnregisterFailoverUnitProxyForCleanupEvent();
    }

    AsyncOperationSPtr const & thisSPtr = closeAsyncOperation->Parent;
    bool didComplete = thisSPtr->TryComplete(thisSPtr, errorCode);
    ASSERT_IFNOT(didComplete, "Failed to complete CloseReplicatorAsyncOperation");
}

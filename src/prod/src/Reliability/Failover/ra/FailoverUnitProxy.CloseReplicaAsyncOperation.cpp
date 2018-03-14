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

void FailoverUnitProxy::CloseReplicaAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    {
        AcquireExclusiveLock grab(owner_.lock_);

        TraceBeforeStart(grab);

        ASSERT_IF(owner_.IsStatelessServiceFailoverUnitProxy(), "Call to close stateful service on an FUP hosting a stateless service.");

        if (owner_.replicaState_ == FailoverUnitProxyStates::Closed)
        {
            bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            ASSERT_IFNOT(didComplete, "Failed to complete CloseReplicaAsyncOperation");
            return;
        }

        ASSERT_IF(owner_.replicaState_ != FailoverUnitProxyStates::Opened, "Attempt to close a stateful service not in Ready or Closed state.");

        ASSERT_IFNOT(owner_.IsStatefulServiceFailoverUnitProxy(), "Stateful service is invalid while closing");

        ASSERT_IF(owner_.serviceOperationManager_ == nullptr, "Service operation manager expected but not available.");

        if (!TryStartUserApi(grab, thisSPtr))
        {
            return;
        }

        owner_.replicaState_ = FailoverUnitProxyStates::Closing;
    }

    AsyncOperationSPtr operation = owner_.statefulService_->BeginClose(
        [this] (AsyncOperationSPtr const & operation) { this->CloseReplicaCompletedCallback(operation); },
        thisSPtr);

    TryContinueUserApi(operation);

    if (operation->CompletedSynchronously)
    {
        FinishCloseReplica(operation);
    }
}

ProxyErrorCode FailoverUnitProxy::CloseReplicaAsyncOperation::End(AsyncOperationSPtr const & asyncOperation)
{
    auto thisPtr = AsyncOperation::End<FailoverUnitProxy::CloseReplicaAsyncOperation>(asyncOperation);
    return thisPtr->EndHelper();
}

void FailoverUnitProxy::CloseReplicaAsyncOperation::CloseReplicaCompletedCallback(AsyncOperationSPtr const & closeAsyncOperation)
{
    if (!closeAsyncOperation->CompletedSynchronously)
    {
        FinishCloseReplica(closeAsyncOperation);
    }
}

void FailoverUnitProxy::CloseReplicaAsyncOperation::FinishCloseReplica(AsyncOperationSPtr const & closeAsyncOperation)
{    
    ErrorCode errorCode = owner_.statefulService_->EndClose(closeAsyncOperation);

    Common::ComPointer<ComStatefulServicePartition> statefulServicePartition;

    {
        AcquireExclusiveLock grab(owner_.lock_);

        if (errorCode.IsSuccess())
        {
            owner_.statefulService_ = nullptr;
            owner_.replicator_ = nullptr;
            statefulServicePartition = std::move(owner_.statefulServicePartition_);

            owner_.TransitionServiceToClosed(grab);
        }
        else
        {
            owner_.replicaState_ = FailoverUnitProxyStates::Opened;
        }

        FinishUserApi(grab, errorCode);
    }

    if (statefulServicePartition)
    {
        statefulServicePartition->ClosePartition();
        statefulServicePartition.Release();
    }

    AsyncOperationSPtr const & thisSPtr = closeAsyncOperation->Parent;
    bool didComplete = thisSPtr->TryComplete(thisSPtr, errorCode);
    ASSERT_IFNOT(didComplete, "Failed to complete CloseReplicaAsyncOperation");
}

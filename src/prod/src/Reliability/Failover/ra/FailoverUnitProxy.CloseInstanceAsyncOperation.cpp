// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Common;
using namespace Hosting2;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

void FailoverUnitProxy::CloseInstanceAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    {
        AcquireExclusiveLock grab(owner_.lock_);

        TraceBeforeStart(grab);

        ASSERT_IF(owner_.IsStatefulServiceFailoverUnitProxy(), "Call to close stateless service on an FUP hosting a stateful service.");

        if (owner_.replicaState_ == FailoverUnitProxyStates::Closed)
        {
            bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            ASSERT_IFNOT(didComplete, "Failed to complete CloseInstanceAsyncOperation");
            return;
        }

        ASSERT_IF(owner_.replicaState_ != FailoverUnitProxyStates::Opened, "Attempt to close a stateless service not in Ready or Closed state.");

        ASSERT_IFNOT(owner_.IsStatelessServiceFailoverUnitProxy(), "Stateless service is invalid while closing");

        owner_.replicaState_ = FailoverUnitProxyStates::Closing;

        ASSERT_IF(owner_.serviceOperationManager_ == nullptr, "Service operation manager expected but not available.");

        if (!TryStartUserApi(grab, thisSPtr))
        {
            return;
        }
    }

    AsyncOperationSPtr operation = owner_.statelessService_->BeginClose(
        [this] (AsyncOperationSPtr const & operation) { this->CloseInstanceCompletedCallback(operation); },
        thisSPtr);

    TryContinueUserApi(operation);

    if (operation->CompletedSynchronously)
    {
        FinishCloseInstance(operation);
    }
}

ProxyErrorCode FailoverUnitProxy::CloseInstanceAsyncOperation::End(AsyncOperationSPtr const & asyncOperation)
{
    auto thisPtr = AsyncOperation::End<FailoverUnitProxy::CloseInstanceAsyncOperation>(asyncOperation);
    return thisPtr->EndHelper();
}

void FailoverUnitProxy::CloseInstanceAsyncOperation::CloseInstanceCompletedCallback(AsyncOperationSPtr const & closeAsyncOperation)
{
    if (!closeAsyncOperation->CompletedSynchronously)
    {
        FinishCloseInstance(closeAsyncOperation);
    }
}

void FailoverUnitProxy::CloseInstanceAsyncOperation::FinishCloseInstance(AsyncOperationSPtr const & closeAsyncOperation)
{    
    ErrorCode errorCode = owner_.statelessService_->EndClose(closeAsyncOperation);

    Common::ComPointer<ComStatelessServicePartition> statelessServicePartition;

    {
        AcquireExclusiveLock grab(owner_.lock_);

        if (errorCode.IsSuccess())
        {
            owner_.statelessService_ = nullptr;
            statelessServicePartition = std::move(owner_.statelessServicePartition_);
            owner_.replicaState_ = FailoverUnitProxyStates::Closed;
        }
        else
        {
            owner_.replicaState_ = FailoverUnitProxyStates::Opened;
        }

        FinishUserApi(grab, errorCode);
    }

    if (statelessServicePartition)
    {
        statelessServicePartition->ClosePartition();
        statelessServicePartition.Release();
    }

    AsyncOperationSPtr const & thisSPtr = closeAsyncOperation->Parent;
    bool didComplete = thisSPtr->TryComplete(thisSPtr, errorCode);
    ASSERT_IFNOT(didComplete, "Failed to complete CloseInstanceAsyncOperation");
}

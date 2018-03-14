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

void FailoverUnitProxy::ReplicatorUpdateEpochAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    Epoch primaryEpoch;

    {
        AcquireExclusiveLock grab(owner_.lock_);

        TraceBeforeStart(grab);

        ASSERT_IF(owner_.failoverUnitDescription_.CurrentConfigurationEpoch == Epoch::InvalidEpoch(), "Epoch passed to replicator must be valid");
        primaryEpoch = owner_.failoverUnitDescription_.CurrentConfigurationEpoch.ToPrimaryEpoch();

        ASSERT_IFNOT(owner_.IsStatefulServiceFailoverUnitProxy() && owner_.replicaState_ == FailoverUnitProxyStates::Opened, "Cannot update replicator epoch except on an open failover unit proxy hosting a stateful service.");
        ASSERT_IFNOT(owner_.replicator_ && owner_.replicatorState_ == ReplicatorStates::Opened, "Cannot update replicator epoch without an opened replicator.");
        
        if (primaryEpoch <= owner_.lastUpdateEpochPrimaryEpochValue_)
        {
            bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            ASSERT_IFNOT(didComplete, "Failed to complete ReplicatorUpdateEpochAsyncOperation");
            return;
        }

        if (!TryStartUserApi(grab, thisSPtr))
        {
            return;
        }        
    }

    AsyncOperationSPtr operation = owner_.replicator_->BeginUpdateEpoch(
        primaryEpoch.ToPublic(),
        [this](AsyncOperationSPtr const & operation) { this->ReplicatorUpdateEpochCompletedCallback(operation); },
        thisSPtr);

    TryContinueUserApi(operation);

    if (operation->CompletedSynchronously)
    {
        FinishReplicatorUpdateEpoch(operation);
    }
}

ProxyErrorCode FailoverUnitProxy::ReplicatorUpdateEpochAsyncOperation::End(AsyncOperationSPtr const & asyncOperation)
{
    auto thisPtr = AsyncOperation::End<FailoverUnitProxy::ReplicatorUpdateEpochAsyncOperation>(asyncOperation);
    return thisPtr->EndHelper();
}

void FailoverUnitProxy::ReplicatorUpdateEpochAsyncOperation::ReplicatorUpdateEpochCompletedCallback(AsyncOperationSPtr const & replicatorUpdateEpochAsyncOperation)
{
    if (!replicatorUpdateEpochAsyncOperation->CompletedSynchronously)
    {
        FinishReplicatorUpdateEpoch(replicatorUpdateEpochAsyncOperation);
    }
}

void FailoverUnitProxy::ReplicatorUpdateEpochAsyncOperation::FinishReplicatorUpdateEpoch(AsyncOperationSPtr const & replicatorUpdateEpochAsyncOperation)
{   
    ErrorCode errorCode = owner_.replicator_->EndUpdateEpoch(replicatorUpdateEpochAsyncOperation);

    {
        AcquireExclusiveLock grab(owner_.lock_);

        if(errorCode.IsSuccess())
        {
            owner_.lastUpdateEpochPrimaryEpochValue_ = owner_.failoverUnitDescription_.CurrentConfigurationEpoch.ToPrimaryEpoch();
        }

        FinishUserApi(grab, errorCode);
    }

    AsyncOperationSPtr const & thisSPtr = replicatorUpdateEpochAsyncOperation->Parent;
    bool didComplete = thisSPtr->TryComplete(thisSPtr, errorCode);
    ASSERT_IFNOT(didComplete, "Failed to complete ReplicatorUpdateEpochAsyncOperation");
}

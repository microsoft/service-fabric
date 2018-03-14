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

void FailoverUnitProxy::ReplicatorCatchupReplicaSetAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    FABRIC_REPLICA_SET_QUORUM_MODE catchupMode = FABRIC_REPLICA_SET_QUORUM_INVALID;

    {
        AcquireExclusiveLock grab(owner_.lock_);

        TraceBeforeStart(grab);

        ASSERT_IFNOT(owner_.IsStatefulServiceFailoverUnitProxy(), "Attempt to catchup replica set using replicator on an FUP that is not hosting a stateful service");
        ASSERT_IFNOT(owner_.replicator_, "Attempt to catchup replica set using replicator when replicator is invalid");
        ASSERT_IFNOT(owner_.replicatorState_ == ReplicatorStates::Opened, "Call to catchup replica set for a replicator that isn't in the opened state.");
               
        ASSERT_IF(owner_.replicatorOperationManager_ == nullptr, "Replicator operation manager expected but not available.");

        if (catchupType_ == CatchupType::PreWriteStatusRevokeCatchup && !owner_.IsPreWriteStatusCatchupEnabled)
        {
            TryComplete(thisSPtr, ErrorCode::Success());
            return;
        }

        catchupMode = GetCatchupModeCallerHoldsLock();

        owner_.AssertCatchupNotStartedCallerHoldsLock();

        if (!TryStartUserApi(grab, thisSPtr))
        {
            return;
        }
    }

    // Make the call to catchup replica set

    AsyncOperationSPtr operation = owner_.replicator_->BeginWaitForCatchUpQuorum(
        catchupMode,
        [this] (AsyncOperationSPtr const & operation) { this->ReplicatorCatchupReplicaSetCompletedCallback(operation); },
        thisSPtr);

    TryContinueUserApi(operation);

    if (operation->CompletedSynchronously)
    {
        FinishReplicatorCatchupReplicaSet(operation);
    }
}

ProxyErrorCode FailoverUnitProxy::ReplicatorCatchupReplicaSetAsyncOperation::End(AsyncOperationSPtr const & asyncOperation)
{
    auto thisPtr = AsyncOperation::End<FailoverUnitProxy::ReplicatorCatchupReplicaSetAsyncOperation>(asyncOperation);
    return thisPtr->EndHelper();
}

void FailoverUnitProxy::ReplicatorCatchupReplicaSetAsyncOperation::ReplicatorCatchupReplicaSetCompletedCallback(AsyncOperationSPtr const & catchupReplicaSetAsyncOperation)
{
    if (!catchupReplicaSetAsyncOperation->CompletedSynchronously)
    {
        FinishReplicatorCatchupReplicaSet(catchupReplicaSetAsyncOperation);
    }
}

void FailoverUnitProxy::ReplicatorCatchupReplicaSetAsyncOperation::FinishReplicatorCatchupReplicaSet(AsyncOperationSPtr const & catchupReplicaSetAsyncOperation)
{    
    ErrorCode errorCode = owner_.replicator_->EndWaitForCatchUpQuorum(catchupReplicaSetAsyncOperation);
  
    {
        AcquireExclusiveLock grab(owner_.lock_);

        /*
            Catchup phase for the reconfiguration is completed only if it is not pre write status revoke catchup
            
            If it is pre write status revoke catchup then set the configuration stage and revoke the write status
        */
        if (errorCode.IsSuccess())
        {        
            if (catchupType_ != CatchupType::PreWriteStatusRevokeCatchup)
            {
                owner_.catchupResult_ = CatchupResult::CatchupCompleted;
            }
            else 
            {
                owner_.ConfigurationStage = ProxyConfigurationStage::CatchupPending;
                owner_.UpdateReadAndWriteStatus(grab);
            }
        }
        
        FinishUserApi(grab, errorCode);
    }

    AsyncOperationSPtr const & thisSPtr = catchupReplicaSetAsyncOperation->Parent;
    bool didComplete = thisSPtr->TryComplete(thisSPtr, errorCode);
    ASSERT_IFNOT(didComplete, "Failed to complete ReplicatorCatchupReplicaSetAsyncOperation");
}

FABRIC_REPLICA_SET_QUORUM_MODE FailoverUnitProxy::ReplicatorCatchupReplicaSetAsyncOperation::GetCatchupModeCallerHoldsLock() const
{
    switch (catchupType_)
    {
    case CatchupType::CatchupQuorum:
        return ::FABRIC_REPLICA_SET_WRITE_QUORUM;
    case CatchupType::PreWriteStatusRevokeCatchup:
    case CatchupType::CatchupDuringSwap:
        return owner_.replicator_->DoesReplicatorSupportCatchupSpecificQuorum ? ::FABRIC_REPLICA_SET_WRITE_QUORUM : ::FABRIC_REPLICA_SET_QUORUM_ALL;
    default:
        Assert::CodingError("Unknown catchup type {0}", static_cast<int>(catchupType_));
    }
}

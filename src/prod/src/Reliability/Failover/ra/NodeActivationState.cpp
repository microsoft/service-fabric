// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;

namespace
{
    void ValidateArguments(int64 sequenceNumber)
    {
        ASSERT_IF(sequenceNumber <= Reliability::Constants::InvalidNodeActivationSequenceNumber, "seq num must be greater than min value");
    }
}

NodeActivationState::NodeActivationState(ReconfigurationAgent & ra)
: ra_(ra)
{
}

void NodeActivationState::Close()
{
    AsyncOperationSPtr fm, fmm;
    
    {
        AcquireWriteLock grab(lock_);
        swap(fmState_.ReplicaCloseMonitorAsyncOperation, fm);
        swap(fmmState_.ReplicaCloseMonitorAsyncOperation, fmm);
    }

    AsyncOperation::CancelIfNotNull(fm);
    AsyncOperation::CancelIfNotNull(fmm);
}

bool NodeActivationState::TryChangeActivationStatus(bool isFmm, NodeActivationState::Enum state, int64 sequenceNumber, NodeActivationState::Enum & currentActivationStateOut, int64 & currentSequenceNumberOut)
{
    ValidateArguments(sequenceNumber);

	AcquireWriteLock grab(lock_);

    InnerState * innerState = &GetState(isFmm);

	if (sequenceNumber <= innerState->SequenceNumber)
	{
		currentActivationStateOut = innerState->State;
		currentSequenceNumberOut = innerState->SequenceNumber;
		return false;
	}
	
	// assign the new sequence number
	innerState->SequenceNumber = sequenceNumber;
	innerState->State = state;
	
	// set out parameters
	currentSequenceNumberOut = innerState->SequenceNumber;
	currentActivationStateOut = innerState->State;
	

    AssertInvariants();

    return true;
}

void NodeActivationState::StartReplicaCloseOperation(
    std::wstring const & activityId, 
    bool isFmm, 
    int64 sequenceNumber, 
    EntityEntryBaseList && ftsToMonitor)
{
    MultipleReplicaCloseCompletionCheckAsyncOperation::Parameters parameters;
    parameters.ActivityId = activityId;
    parameters.Callback = [](std::wstring const & innerActivityId, EntityEntryBaseList const & fts, ReconfigurationAgent & ra)
    {
        RAEventSource::Events->LifeCycleNodeDeactivationProgress(ra.NodeInstanceIdStr, innerActivityId, fts);
    };
    parameters.CloseMode = ReplicaCloseMode::DeactivateNode;
    parameters.FTsToClose = move(ftsToMonitor);
    parameters.MaxWaitBeforeTerminationEntry = &ra_.Config.NodeDeactivationMaxReplicaCloseDurationEntry;
    parameters.MonitoringIntervalEntry = &ra_.Config.NodeDeactivationReplicaCloseCompletionCheckIntervalEntry;
    parameters.RA = &ra_;
    parameters.TerminateReason = Hosting::TerminateServiceHostReason::NodeDeactivation;

    AsyncOperationSPtr snap;

    {
        AcquireWriteLock grab(lock_);
        auto & innerState = GetState(isFmm);

        if (sequenceNumber == innerState.SequenceNumber)
        {
            snap = AsyncOperation::CreateAndStart<MultipleReplicaCloseCompletionCheckAsyncOperation>(
                move(parameters),
                [](AsyncOperationSPtr const & inner)
                {
                    MultipleReplicaCloseCompletionCheckAsyncOperation::End(inner);
                },
                ra_.Root.CreateAsyncOperationRoot());

            swap(innerState.ReplicaCloseMonitorAsyncOperation, snap);
        }
    }

    AsyncOperation::CancelIfNotNull(snap);
}

void NodeActivationState::CancelReplicaCloseOperation(std::wstring const &, bool isFmm, int64 sequenceNumber)
{
    AsyncOperationSPtr snap;

    {
        AcquireWriteLock grab(lock_);
        auto & innerState = GetState(isFmm);
        if (innerState.SequenceNumber == sequenceNumber)
        {
            swap(innerState.ReplicaCloseMonitorAsyncOperation, snap);
        }
    }
    
    AsyncOperation::CancelIfNotNull(snap);
}

bool NodeActivationState::IsCurrent(Reliability::FailoverUnitId const & ftId, int64 sequenceNumber) const
{
    ValidateArguments(sequenceNumber);

    AcquireReadLock grab(lock_);
    return sequenceNumber == GetStateForFTId(ftId).SequenceNumber;
}

bool NodeActivationState::IsActivated(Reliability::FailoverUnitId const & ftId) const
{
    AcquireReadLock grab(lock_);
    return GetStateForFTId(ftId).State == NodeActivationState::Activated;
}

bool NodeActivationState::IsActivated(bool isFmm) const
{
    AcquireReadLock grab(lock_);
    return isFmm ? fmmState_.State == NodeActivationState::Activated : fmState_.State == NodeActivationState::Activated;
}

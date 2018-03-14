// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Node;
using namespace Infrastructure;

NodeDeactivationState::NodeDeactivationState(
    Reliability::FailoverManagerId const & fm,
    ReconfigurationAgent & ra) :
    fm_(fm),
    ra_(ra)
{
}

bool NodeDeactivationState::TryStartChange(
    NodeDeactivationInfo const & newActivationInfo)
{
    AcquireWriteLock grab(lock_);

    if (newActivationInfo.SequenceNumber <= state_.SequenceNumber)
    {
        return false;
    }

    state_ = newActivationInfo;
    RAEventSource::Events->LifeCycleNodeActivationStateChange(ra_.NodeInstanceIdStr, state_, fm_);

    /*
        It is important that work is enqueued only after node up ack is processed
        as the activation state may be changed during node up ack processing.

        Consider the scenario below:
        1. Node comes up
        2. Node sends node up ack to FM and FMM
        3. FM replies with activated and some instance
        4. This updates activation state for both FM and FMM as the node is activated

        However, the FM replica cannot be opened until the node up ack from fmm is processed
        Returning false from here will ensure that no job items are enqueued for this
    */
    return ra_.StateObj.GetIsReady(fm_);
}

void NodeDeactivationState::FinishChange(
    std::wstring const & activityId,
    NodeDeactivationInfo const & activationInfo,
    Infrastructure::EntityEntryBaseList && ftsToMonitor)
{
    if (activationInfo.IsActivated)
    {
        CancelReplicaCloseOperation(activationInfo, move(ftsToMonitor));
    }
    else
    {
        StartReplicaCloseOperation(activityId, activationInfo, move(ftsToMonitor));
    }
}

void NodeDeactivationState::Close()
{
    AsyncOperationSPtr op;

    {
        AcquireWriteLock grab(lock_);
        swap(replicaCloseMonitorAsyncOperation_, op);
    }

    AsyncOperation::CancelIfNotNull(op);
}

void NodeDeactivationState::StartReplicaCloseOperation(
    std::wstring const & activityId,
    NodeDeactivationInfo const & activationInfo,
    Infrastructure::EntityEntryBaseList && ftsToMonitor)
{
    UNREFERENCED_PARAMETER(activationInfo);

    MultipleReplicaCloseCompletionCheckAsyncOperation::Parameters parameters;
    parameters.Clock = ra_.ClockSPtr;
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
        
        if (!CheckStalenessAtFinish(grab, activationInfo))
        {
            return;
        }

        snap = AsyncOperation::CreateAndStart<MultipleReplicaCloseCompletionCheckAsyncOperation>(
            move(parameters),
            [](AsyncOperationSPtr const & inner)
            {
                MultipleReplicaCloseCompletionCheckAsyncOperation::End(inner);
            },
            ra_.Root.CreateAsyncOperationRoot());

        swap(replicaCloseMonitorAsyncOperation_, snap);
    }

    AsyncOperation::CancelIfNotNull(snap);
}

void NodeDeactivationState::CancelReplicaCloseOperation(
    NodeDeactivationInfo const & activationInfo,
    Infrastructure::EntityEntryBaseList && ftsToMonitor)
{
    UNREFERENCED_PARAMETER(activationInfo);
    UNREFERENCED_PARAMETER(ftsToMonitor);

    AsyncOperationSPtr snap;

    {
        AcquireWriteLock grab(lock_);
        
        if (!CheckStalenessAtFinish(grab, activationInfo))
        {
            return;
        }

        swap(replicaCloseMonitorAsyncOperation_, snap);
    }

    AsyncOperation::CancelIfNotNull(snap);
}

bool NodeDeactivationState::CheckStalenessAtFinish(
    Common::AcquireWriteLock const &,
    NodeDeactivationInfo const & incoming) const
{
    ASSERT_IF(incoming.SequenceNumber > state_.SequenceNumber, "At finish incoming cannot be greater than current because this means start was not called Incoming {0}. Current {1}", incoming, state_);
    ASSERT_IF(incoming.SequenceNumber == state_.SequenceNumber && incoming.IsActivated != state_.IsActivated, "sequence number match but intent does not. Incoming {0}. Current {1}.", incoming, state_);

    return incoming.SequenceNumber == state_.SequenceNumber;
}

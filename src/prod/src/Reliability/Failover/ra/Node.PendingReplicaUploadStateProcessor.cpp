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

namespace
{
    wstring CreateId(ReconfigurationAgent & ra, FailoverManagerId const & fm)
    {
        return wformatString("{0}_{1}_{2}", ra.NodeInstanceIdStr, EntitySetName::ReplicaUploadPending, fm.ToDisplayString());
    }

    EntitySet::Parameters CreateSetParameters(ReconfigurationAgent & ra, FailoverManagerId const & fm)
    {
        wstring name = CreateId(ra, fm);
        return EntitySet::Parameters(name, EntitySetIdentifier(EntitySetName::ReplicaUploadPending, fm), nullptr);
    }
}

PendingReplicaUploadStateProcessor::PendingReplicaUploadStateProcessor(
    ReconfigurationAgent & ra,
    Infrastructure::EntitySetCollection & setCollection,
    Reliability::FailoverManagerId const & failoverManager) :
    ra_(ra),
    pendingUploadState_(CreateSetParameters(ra, failoverManager), setCollection, ra.Config.ReopenSuccessWaitIntervalEntry),
    retryInterval_(&ra.Config.LastReplicaUpTimerIntervalEntry),
    timer_(
        CreateId(ra, failoverManager), 
        ra, 
        ra.Config.LastReplicaUpTimerIntervalEntry, 
        L"lru",
        [this](std::wstring const & activityId, ReconfigurationAgent &) { OnTimer(activityId); }),
    owner_(failoverManager)
{
}

void PendingReplicaUploadStateProcessor::ProcessNodeUpAck(
    std::wstring const & activityId,
    bool isDeferredUploadRequired)
{    
    PendingReplicaUploadState::Action action(Owner);

    {
        AcquireWriteLock grab(lock_);

        action = pendingUploadState_.OnNodeUpAckProcessed(activityId, GetClock().Now(), isDeferredUploadRequired);

        timer_.Set();
    }

    Execute(activityId, action);
}

void PendingReplicaUploadStateProcessor::Close()
{
    AcquireWriteLock grab(lock_);

    timer_.Close();
}

void PendingReplicaUploadStateProcessor::OnTimer(std::wstring const & activityId)
{
    PendingReplicaUploadState::Action action(Owner);

    {
        AcquireWriteLock grab(lock_);

        action = pendingUploadState_.OnTimer(activityId, GetClock().Now());

        timer_.Set();
    }

    Execute(activityId, action);
}

void PendingReplicaUploadStateProcessor::ProcessLastReplicaUpReply(
    std::wstring const & activityId)
{
    PendingReplicaUploadState::Action action(Owner);

    {
        AcquireWriteLock grab(lock_);

        action = pendingUploadState_.OnLastReplicaUpReply(activityId, GetClock().Now());

        timer_.Close();
    }

    Execute(activityId, action);
}

bool PendingReplicaUploadStateProcessor::IsLastReplicaUpMessage(
    Infrastructure::EntityEntryBaseSet const & entitiesInMessage) const
{
    AcquireReadLock grab(lock_);
    return pendingUploadState_.IsLastReplicaUpMessage(entitiesInMessage);
}

Infrastructure::IClock const & PendingReplicaUploadStateProcessor::GetClock() const
{
    return ra_.Clock;
}

void PendingReplicaUploadStateProcessor::Execute(
    std::wstring const & activityId,
    PendingReplicaUploadState::Action const & action)
{
    TrySendLastReplicaUp(activityId, action);

    TryRequestUploadOnEntities(activityId, action);

    Trace(activityId, action.TraceDataObj);
}

void PendingReplicaUploadStateProcessor::TrySendLastReplicaUp(
    std::wstring const & activityId,
    PendingReplicaUploadState::Action const & action)
{
    if (!action.SendLastReplicaUp)
    {
        return;
    }

    ReplicaUpMessageBody body(vector<FailoverUnitInfo>(), vector<FailoverUnitInfo>(), true, Owner.IsFmm);
    ra_.FMTransportObj.SendMessageToFM(Owner, RSMessage::GetReplicaUp(), activityId, body);    
}

void PendingReplicaUploadStateProcessor::TryRequestUploadOnEntities(
    std::wstring const & activityId,
    PendingReplicaUploadState::Action const & action)
{
    if (!action.InvokeUploadOnEntities)
    {
        return;
    }

    ASSERT_IF(action.Entities.size() == 0, "Cannot invoke upload on 0 entities");

    auto factory = [] (EntityEntryBaseSPtr const & entry, std::shared_ptr<MultipleEntityWork> const & inner)
    {
        auto handler = [](HandlerParameters & handlerParameters, JobItemContextBase &)
        {
            handlerParameters.FailoverUnit->UploadReplicaForInitialDiscovery(handlerParameters.ExecutionContext);

            // always trace - these job items are executed only once in the lifecycle of the node
            return true;
        };

        UploadForReplicaDiscoveryJobItem::Parameters jobItemParameters(
            entry,
            inner,
            handler,
            JobItemCheck::DefaultAndOpen,
            *JobItemDescription::UploadForReplicaDiscovery,
            move(JobItemContextBase()));

        return make_shared<UploadForReplicaDiscoveryJobItem>(move(jobItemParameters));
    };

    auto work = make_shared<MultipleEntityWork>(activityId);

    ra_.JobQueueManager.CreateJobItemsAndStartMultipleFailoverUnitWork(work, action.Entities, factory);
}

void PendingReplicaUploadStateProcessor::Trace(
    std::wstring const & activityId, 
    PendingReplicaUploadState::TraceData const & traceData)
{
    RAEventSource::Events->LifeCycleLastReplicaUpState(ra_.NodeInstanceIdStr, activityId, traceData);
}

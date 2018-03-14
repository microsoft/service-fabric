// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace Node;

PerformanceCounterData f;

PendingReplicaUploadState::PendingReplicaUploadState(
    EntitySet::Parameters const & setParameters,
    EntitySetCollection & setCollection,
    TimeSpanConfigEntry const & reopenSuccessWaitDurationConfig) :
    pendingReplicas_(setParameters),
    reopenSuccessWaitDurationConfig_(&reopenSuccessWaitDurationConfig),
    isComplete_(false),
    hasUploadBeenRequested_(false),
    isDeferredUploadRequired_(false)
{
    setCollection.Add(pendingReplicas_.Id, pendingReplicas_, nullptr);
}

PendingReplicaUploadState::Action PendingReplicaUploadState::OnNodeUpAckProcessed(
    wstring const & activityId,
    StopwatchTime now,
    bool isDeferredUploadRequired)
{    
    AssertIfNodeUpAckProcessed(activityId, "Duplicate node up ack being processed");

    nodeUpAckTime_ = now;
    isDeferredUploadRequired_ = isDeferredUploadRequired;
    
    if (HasPendingReplicas)
    {
        return CreateNoOpAction(now);
    }

    return CreateSendLastReplicaUpAction(now);
}

PendingReplicaUploadState::Action PendingReplicaUploadState::OnTimer(
    std::wstring const & activityId,
    StopwatchTime now) 
{
    AssertIfNodeUpAckNotProcessed(activityId, "OnTimer");

    if (isComplete_)
    {
        return CreateNoOpAction(now);
    }
    else if (!HasPendingReplicas)
    {
        return CreateSendLastReplicaUpAction(now);
    }
    else if (isDeferredUploadRequired_ && !hasUploadBeenRequested_ && (now - nodeUpAckTime_) > reopenSuccessWaitDurationConfig_->GetValue())
    {
        auto entities = pendingReplicas_.GetEntities();
        if (entities.empty())
        {
            return CreateSendLastReplicaUpAction(now);
        }
        else
        {
            hasUploadBeenRequested_ = true;
            return CreateMarkForUploadAction(move(entities), now);
        }
    }
    else
    {
        return CreateNoOpAction(now);
    }
}

PendingReplicaUploadState::Action PendingReplicaUploadState::OnLastReplicaUpReply(
    std::wstring const & activityId,
    StopwatchTime now)
{
    AssertIfNodeUpAckNotProcessed(activityId, "LastReplicaUpReply");
    ASSERT_IF(HasPendingReplicas, "Pending replicas present on receiving last replica up reply {0}. Activity: {1}", FailoverManager, activityId);

    isComplete_ = true;

    return CreateNoOpAction(now);
}

bool PendingReplicaUploadState::IsLastReplicaUpMessage(
    Infrastructure::EntityEntryBaseSet const & entitiesInMessage) const
{
    if (!HasNodeUpAckBeenProcessed)
    {
        return false;
    }

    if (isComplete_)
    {
        return false;
    }

    return pendingReplicas_.IsSubsetOf(entitiesInMessage);
}

void PendingReplicaUploadState::AssertIfNodeUpAckNotProcessed(
    wstring const & activityId,
    Common::StringLiteral const & tag) const
{
    ASSERT_IF(!HasNodeUpAckBeenProcessed, "Activity: {2}. NodeUpAckNotProcessed {0} {1}", tag, FailoverManager, activityId);
}

void PendingReplicaUploadState::AssertIfNodeUpAckProcessed(
    wstring const & activityId,
    Common::StringLiteral const & tag) const
{
    ASSERT_IF(HasNodeUpAckBeenProcessed, "Activity: {2}. NodeUpAckProcessed {0} {1}", tag, FailoverManager, activityId);
}

PendingReplicaUploadState::Action PendingReplicaUploadState::CreateSendLastReplicaUpAction(StopwatchTime now) const
{
    auto rv = CreateNoOpAction(now);
    rv.SendLastReplicaUp = true;
    return rv;
}

PendingReplicaUploadState::Action PendingReplicaUploadState::CreateMarkForUploadAction(
    EntityEntryBaseList && entities,
    StopwatchTime now) const
{
    auto rv = CreateNoOpAction(now);
    rv.InvokeUploadOnEntities = true;
    rv.Entities = move(entities);
    return rv;
}

PendingReplicaUploadState::Action PendingReplicaUploadState::CreateNoOpAction(StopwatchTime now) const
{
    PendingReplicaUploadState::Action result(FailoverManager);
    result.TraceDataObj.PendingReplicaCount = pendingReplicas_.Size;
    result.TraceDataObj.IsComplete = isComplete_;
    result.TraceDataObj.Elapsed = now - nodeUpAckTime_;
    result.TraceDataObj.RandomPendingEntityTraceId = pendingReplicas_.GetRandomEntityTraceId();
    return result;
}

std::string PendingReplicaUploadState::TraceData::AddField(Common::TraceEvent& traceEvent, std::string const& name)
{
    string format = "LastReplicaUpState {0}. Pending Count = {1}. IsComplete = {2}. Elapsed = {3}. Random Pending Entity Id = {4}";
    size_t index = 0;

    traceEvent.AddEventField<FailoverManagerId>(format, name + ".owner", index);
    traceEvent.AddEventField<int64>(format, name + ".pendingCount", index);
    traceEvent.AddEventField<bool>(format, name + ".isComplete", index);
    traceEvent.AddEventField<TimeSpan>(format, name + ".elapsed", index);
    traceEvent.AddEventField<std::string>(format, name + ".pendingEntityId", index);

    return format;
}

void PendingReplicaUploadState::TraceData::FillEventData(Common::TraceEventContext& context) const
{
    context.Write(Owner);
    context.Write(PendingReplicaCount);
    context.Write(IsComplete);
    context.Write(Elapsed);
    context.Write(RandomPendingEntityTraceId);
}

void PendingReplicaUploadState::TraceData::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    w << wformatString("LastReplicaUpState {0}. Pending Count = {1}. IsComplete = {2}. Elapsed = {3}. Random Pending Entity Id = {4}", Owner, PendingReplicaCount, IsComplete, Elapsed, RandomPendingEntityTraceId);
}

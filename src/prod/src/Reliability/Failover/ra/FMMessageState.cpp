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
using ReconfigurationAgentComponent::Communication::RequestFMMessageRetryAction;

FMMessageState::FMMessageState(
    bool const * localReplicaDeleted, 
    FailoverManagerId const & fm,
    Infrastructure::IRetryPolicySPtr const & retryPolicy) :
localReplicaDeleted_(localReplicaDeleted),
messageStage_(FMMessageStage::None),
fmMessageRetryPendingFlag_(EntitySetIdentifier(EntitySetName::FailoverManagerMessageRetry, fm)),
preReopenLocalReplicaInstanceId_(InvalidInstance),
retryState_(retryPolicy)
{
}

FMMessageState::FMMessageState(bool const * localReplicaDeleted, FMMessageState const & other) :
localReplicaDeleted_(localReplicaDeleted),
messageStage_(other.messageStage_),
fmMessageRetryPendingFlag_(other.fmMessageRetryPendingFlag_),
preReopenLocalReplicaInstanceId_(other.preReopenLocalReplicaInstanceId_),
retryState_(other.retryState_)
{   
}

FMMessageState & FMMessageState::operator=(FMMessageState const & other)
{
    if (this != &other)
    {
        messageStage_ = other.messageStage_;
        fmMessageRetryPendingFlag_ = other.fmMessageRetryPendingFlag_;
        preReopenLocalReplicaInstanceId_ = other.preReopenLocalReplicaInstanceId_;
        retryState_ = other.retryState_;
    }

    return *this;
}

void FMMessageState::Test_SetMessageStage(FMMessageStage::Enum value)
{
    messageStage_ = value; 
    fmMessageRetryPendingFlag_.Test_SetValue(messageStage_ != FMMessageStage::None);
    retryState_.Test_SetIsPending(messageStage_ != FMMessageStage::None);
}

void FMMessageState::OnLastReplicaUpPending(
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    AssertIfDeleted();
    AssertIfMessageStageIs({ FMMessageStage::EndpointAvailable, FMMessageStage::ReplicaUpload }, "Invalid MessageStage OnLastReplicaUpPending");

    if (messageStage_ == FMMessageStage::None)
    {
        TransitionMessageStage(FMMessageStage::ReplicaUpload, stateUpdateContext);
    }
}

void FMMessageState::OnLastReplicaUpAcknowledged(
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    AssertIfMessageStageIs({ FMMessageStage::EndpointAvailable }, "Invalid MessageStage OnLastReplicaUpPending");

    if (messageStage_ == FMMessageStage::ReplicaUpload)
    {
        TransitionMessageStage(FMMessageStage::None, stateUpdateContext);
    }
}

void FMMessageState::MarkReplicaEndpointUpdatePending(
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    AssertIfDeleted();
    AssertIfMessageStageIsNot({ FMMessageStage::None, FMMessageStage::EndpointAvailable }, "Invalid MessageStage EndpointUpdate");
    AssertIfInstanceIsSet();

    if (messageStage_ == FMMessageStage::None && !IsInstanceSet)
    {
        TransitionMessageStage(FMMessageStage::EndpointAvailable, stateUpdateContext);
    }
}

void FMMessageState::Reset(
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    TransitionMessageStage(FMMessageStage::None, stateUpdateContext);
    ClearInstanceIfSet(stateUpdateContext);
}

void FMMessageState::OnDropped(
    Infrastructure::StateUpdateContext & stateUpdateContext)
{    
    AssertIfMessageStageIs({ FMMessageStage::EndpointAvailable, FMMessageStage::ReplicaDropped }, "Invalid MessageStage OnDropped");
    
    if (IsDeleted)
    {
        return;
    }

    TESTASSERT_IF(messageStage_ == FMMessageStage::ReplicaDown && !IsInstanceSet, "Cannot drop volatile replica");

    TransitionMessageStage(FMMessageStage::ReplicaDropped, stateUpdateContext);
    ClearInstanceIfSet(stateUpdateContext);
}

void FMMessageState::OnClosing(
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    AssertIfMessageStageIs({ FMMessageStage::ReplicaDropped }, "Cannot start closing if already DD");

    // Don't send endpoint update or replica up 
    // Continue sending LRU because otherwise LRU is blocked until close completes
    // Some message will be sent to fm after close completes
    if (messageStage_ == FMMessageStage::ReplicaUp || messageStage_ == FMMessageStage::EndpointAvailable)
    {
        TransitionMessageStage(FMMessageStage::None, stateUpdateContext);
    }
}

void FMMessageState::OnReplicaUp(
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    AssertIfDeleted();

    // Replica up should only happen if the replica is down or replica down reply has been received for persisted
    // or replica down (and opening) and lru is pending - now replica up will be pending
    bool replicaUpAllowed = IsInstanceSet && (messageStage_ == FMMessageStage::ReplicaDown || messageStage_ == FMMessageStage::None || messageStage_ == FMMessageStage::ReplicaUpload);
    TESTASSERT_IF(!replicaUpAllowed, "Invalid messagestate OnReplicaUp");

    if (IsDeleted)
    {
        return;
    }
    
    TransitionMessageStage(FMMessageStage::ReplicaUp, stateUpdateContext);
    ClearInstanceIfSet(stateUpdateContext);
}

void FMMessageState::OnReplicaDown(
    bool isPersisted,
    int64 replicaInstance,
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    AssertIfMessageStageIs({ FMMessageStage::EndpointAvailable, FMMessageStage::ReplicaDropped }, "Invalid messagestage OnReplicaDown");

    TESTASSERT_IF(replicaInstance < preReopenLocalReplicaInstanceId_, "Stale replica instance on replica down");
    TESTASSERT_IF(messageStage_ == FMMessageStage::ReplicaUpload && !IsInstanceSet && !isPersisted, "LRU without instance means it is a volatile replica so cannot go down ");
    TESTASSERT_IF(!isPersisted && messageStage_ == FMMessageStage::ReplicaUpload && IsInstanceSet, "LRU with instance means persisted and cant have volatile go down");

    if (IsDeleted)
    {
        return;
    }

    if (replicaInstance <= preReopenLocalReplicaInstanceId_)
    {
        return;
    }

    TransitionMessageStage(FMMessageStage::ReplicaDown, stateUpdateContext);

    // Store instance only for persisted services
    // Volatile services have down = dropped
    if (isPersisted)
    {
        SetInstance(replicaInstance, stateUpdateContext);
    }
}

void FMMessageState::OnLoadedFromLfum(
    int64 replicaInstance,
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    AssertIfMessageStageIsNot({ FMMessageStage::None }, "Invalid messagestage OnLoadedFromLfum");

    if (IsDeleted)
    {
        return;
    }

    if (messageStage_ == FMMessageStage::None)
    {
        SetInstance(replicaInstance, stateUpdateContext);
    }
}

void FMMessageState::OnReplicaUpAcknowledged(
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    AssertIfMessageStageIs({ FMMessageStage::ReplicaUpload }, "Invalid messagestage OnReplicaUpAcknowledeged");

    if (messageStage_ == FMMessageStage::ReplicaUp)
    {
        TransitionMessageStage(FMMessageStage::None, stateUpdateContext);
    }
}

void FMMessageState::OnReplicaDownReply(
    int64 replicaInstance,
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    AssertIfMessageStageIs({ FMMessageStage::ReplicaUpload }, "Invalid messagestage OnReplicaUpAcknowledeged");

    // SV closed replicas do not have an instance
    if (messageStage_ == FMMessageStage::ReplicaDown && 
        (replicaInstance == preReopenLocalReplicaInstanceId_ || !IsInstanceSet))
    {
        TransitionMessageStage(FMMessageStage::None, stateUpdateContext);
    }
}

void FMMessageState::OnReplicaDroppedReply(
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    AssertIfMessageStageIs({ FMMessageStage::ReplicaUpload }, "Invalid messagestage OnReplicaUpAcknowledeged");

    if (messageStage_ == FMMessageStage::ReplicaDropped)
    {
        TransitionMessageStage(FMMessageStage::None, stateUpdateContext);
    }
}

bool FMMessageState::ShouldRetry(
    Common::StopwatchTime now,
    __out int64 & sequenceNumber) const
{
    return retryState_.ShouldRetry(now, sequenceNumber);
}

void FMMessageState::OnRetry(
    Common::StopwatchTime now,
    int64 sequenceNumber,
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    if (retryState_.OnRetry(sequenceNumber, now))
    {
        stateUpdateContext.UpdateContextObj.EnableInMemoryUpdate();
    }
}

void FMMessageState::TransitionMessageStage(
    FMMessageStage::Enum target,
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    bool isDifferentMessageStage = messageStage_ != target;
    bool isRetryRequired = target != FMMessageStage::None;

    if (isDifferentMessageStage || isRetryRequired)
    {
        stateUpdateContext.UpdateContextObj.EnableInMemoryUpdate();
    }

    // Transition the retry state
    if (isRetryRequired)
    {
        retryState_.Start();
    }
    else
    {
        retryState_.Finish();
    }

    // It may be that a state update is not required but it is a retry
    // example: replica down with i1 and then before the open completes
    // down happens again with i2
    // in this case the message retry needs to be requested immediately
    // to tell the fm of the new instance
    fmMessageRetryPendingFlag_.SetValue(target != FMMessageStage::None, stateUpdateContext.ActionQueue);
    messageStage_ = target;
    EnqueueFMMessageRetryAction(stateUpdateContext.ActionQueue); // call after updating message stage_
}

void FMMessageState::EnqueueFMMessageRetryAction(
    Infrastructure::StateMachineActionQueue & queue) const
{
    if (messageStage_ != FMMessageStage::None)
    {
        queue.Enqueue(
            make_unique<RequestFMMessageRetryAction>(fmMessageRetryPendingFlag_.Id.FailoverManager));
    }
}

void FMMessageState::ClearInstanceIfSet(
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    if (IsInstanceSet)
    {
        SetInstance(InvalidInstance, stateUpdateContext);
    }
}

void FMMessageState::SetInstance(
    int64 replicaInstance,
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    stateUpdateContext.UpdateContextObj.EnableInMemoryUpdate();
    preReopenLocalReplicaInstanceId_ = replicaInstance;
}

void FMMessageState::AssertInvariants(FailoverUnit const & ft) const
{
    bool isEndpointUpdateAllowed = false;
    bool isDroppedAllowed = false;
    bool isDownAllowed = false;
    bool isUpAllowed = false;

    switch (ft.ReconfigurationStage)
    {
    case FailoverUnitReconfigurationStage::Phase1_GetLSN:
    case FailoverUnitReconfigurationStage::None:
    case FailoverUnitReconfigurationStage::Abort_Phase0_Demote:
        isEndpointUpdateAllowed = false;
        break;

    case FailoverUnitReconfigurationStage::Phase0_Demote:
    case FailoverUnitReconfigurationStage::Phase2_Catchup:
    case FailoverUnitReconfigurationStage::Phase3_Deactivate:
    case FailoverUnitReconfigurationStage::Phase4_Activate:
        isEndpointUpdateAllowed = true;
        break;

    default:
        Assert::TestAssert("Unknown reconfig stage {0}", static_cast<int>(ft.ReconfigurationStage));
        return;
    }

    isDroppedAllowed = ft.IsClosed && !IsDeleted;
    isDownAllowed = ft.IsClosed || !ft.LocalReplica.IsUp || ((ft.LocalReplica.IsStandBy || ft.LocalReplica.IsInDrop) && !ft.LocalReplicaOpen) || ft.LocalReplicaClosePending.IsSet;
    isUpAllowed = ft.IsOpen && ft.LocalReplica.IsUp && ft.LocalReplicaOpen && ft.LocalReplica.IsStandBy;

    bool isInstanceAllowed = (messageStage_ == FMMessageStage::None || messageStage_ == FMMessageStage::ReplicaDown || messageStage_ == FMMessageStage::ReplicaUpload) && ft.HasPersistedState;

    TESTASSERT_IF(IsDeleted && messageStage_ != FMMessageStage::None, "Deleted ft cannot send message to FM {0}", ft);

    // Validation
    if (messageStage_ == FMMessageStage::EndpointAvailable)
    {
        TESTASSERT_IF(!isEndpointUpdateAllowed, "Endpoint update set set when it should not be {0}", ft);
    }
    else if (messageStage_ == FMMessageStage::ReplicaDropped)
    {
        TESTASSERT_IF(!isDroppedAllowed, "Dropped set when it should not be {0}", ft);
    }
    else if (messageStage_ == FMMessageStage::ReplicaDown)
    {
        TESTASSERT_IF(!isDownAllowed, "Down set when it should not be {0}", ft);
    }
    else if (messageStage_ == FMMessageStage::ReplicaUp)
    {
        TESTASSERT_IF(!isUpAllowed, "Up set when it should not be {0}", ft);
    }

    TESTASSERT_IF(IsInstanceSet && !isInstanceAllowed, "instance set when it should not be {0}", ft);
    
    bool isRetryExpected = messageStage_ != FMMessageStage::None;
    TESTASSERT_IF(isRetryExpected != retryState_.IsPending, "FMmessage state {0}. Retry: {1}", messageStage_, retryState_.IsPending);
}

vector<SetMembershipFlag const*> FMMessageState::Test_GetAllSets() const
{
    vector<SetMembershipFlag const*> v;
    v.push_back(&fmMessageRetryPendingFlag_);
    return v;
}

std::string FMMessageState::AddField(Common::TraceEvent& traceEvent, std::string const& name)
{
    string format = "{0}";
    size_t index = 0;

    traceEvent.AddEventField<FMMessageStage::Trace>(format, name + ".fmMessageStage", index);

    return format;
}

void FMMessageState::FillEventData(Common::TraceEventContext& context) const
{
    context.Write<FMMessageStage::Trace>(messageStage_);
}

void FMMessageState::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    w << messageStage_;
}

void FMMessageState::AssertIfMessageStageIsNot(std::initializer_list<FMMessageStage::Enum> const & values, Common::StringLiteral const & tag) const
{
    TESTASSERT_IF(find(values.begin(), values.end(), messageStage_) == values.end(), "MessageStage {0} at {1} not allowed", messageStage_, tag);
}

void FMMessageState::AssertIfMessageStageIs(std::initializer_list<FMMessageStage::Enum> const & values, Common::StringLiteral const & tag) const
{
    TESTASSERT_IF(find(values.begin(), values.end(), messageStage_) != values.end(), "MessageStage {0} at {1} not allowed", messageStage_, tag);
}

void FMMessageState::AssertIfInstanceIsNotSet() const
{
    TESTASSERT_IF(!IsInstanceSet, "Instance should be set");
}

void FMMessageState::AssertIfInstanceIsSet() const
{
    TESTASSERT_IF(IsInstanceSet, "Instance should not be set");
}

void FMMessageState::AssertIfDeleted() const
{
    TESTASSERT_IF(IsDeleted, "IsDeleted not expected");
}

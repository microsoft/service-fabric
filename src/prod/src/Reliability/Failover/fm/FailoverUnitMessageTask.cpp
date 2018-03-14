// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

class MissingFailoverUnitJobItem : public Common::CommonTimedJobItem<FailoverManager>
{
public:
    MissingFailoverUnitJobItem(StateMachineTaskUPtr && task, TimeSpan timeout)
        : CommonTimedJobItem(timeout)
        , task_(move(task))
    {
    }

    virtual void Process(FailoverManager & fm)
    {
        vector<StateMachineActionUPtr> actions;
        LockedFailoverUnitPtr failoverUnit;
        task_->CheckFailoverUnit(failoverUnit, actions);

        for (StateMachineActionUPtr const & action : actions)
        {
            action->PerformAction(fm);
        }
    }

private:
    StateMachineTaskUPtr task_;
};

template <class T>
FailoverUnitMessageTask<T>::FailoverUnitMessageTask(
    wstring const & action,
    unique_ptr<T> && body,
    FailoverManager & failoverManager,
    NodeInstance const & from)
    :   action_(action),
        body_(move(body)),
        failoverManager_(failoverManager),
        from_(from)
{
}

template <class T>
FailoverUnitId const & FailoverUnitMessageTask<T>::GetFailoverUnitId() const
{
    return body_->FailoverUnitDescription.FailoverUnitId;
}

template <class T>
void FailoverUnitMessageTask<T>::ProcessRequest(
    Transport::Message & request,
    FailoverManager & failoverManager,
    NodeInstance const & from)
{
    unique_ptr<T> body = make_unique<T>();
    if (!request.GetBody<T>(*body))
    {
        failoverManager.MessageEvents.InvalidMessageDropped(failoverManager.Id, request.MessageId, request.Action, request.Status);
        return;
    }

    FailoverUnitMessageTask<T>::ProcessRequest(request.Action, move(body), failoverManager, from);
}

template <class T>
void FailoverUnitMessageTask<T>::ProcessRequest(
    wstring const& action,
    unique_ptr<T> && body,
    FailoverManager & failoverManager,
    NodeInstance const & from)
{
    FailoverUnitId failoverUnitId = body->FailoverUnitDescription.FailoverUnitId;

    DynamicStateMachineTaskUPtr task(new FailoverUnitMessageTask(action, move(body), failoverManager, from));

    bool result = failoverManager.FailoverUnitCacheObj.TryProcessTaskAsync(failoverUnitId, task, from);
    if (!result)
    {
        failoverManager.CommonQueue.Enqueue(make_unique<MissingFailoverUnitJobItem>(move(task), TimeSpan::MaxValue));
    }
}

template <class T>
void FailoverUnitMessageTask<T>::CheckFailoverUnit(
    LockedFailoverUnitPtr & lockedFailoverUnit,
    vector<StateMachineActionUPtr> & actions)
{
    UNREFERENCED_PARAMETER(actions);

    if (lockedFailoverUnit)
    {
        if (IsStaleRequest(lockedFailoverUnit))
        {
            failoverManager_.FTEvents.FTMessageIgnore(
                lockedFailoverUnit->Id.Guid,
                action_,
                from_.Id,
                from_.InstanceId,
                TraceCorrelatedEvent<T>(*body_));

            return;
        }
    }
    else if (action_ != RSMessage::GetReplicaDropped().Action && 
             action_ != RSMessage::GetDeleteReplicaReply().Action)
    {
        failoverManager_.FTEvents.FTMessageIgnore(
            GetFailoverUnitId().Guid,
            action_,
            from_.Id,
            from_.InstanceId,
            TraceCorrelatedEvent<T>(*body_));

        return;
    }

    failoverManager_.FTEvents.FTMessageProcessing(
        lockedFailoverUnit ? lockedFailoverUnit->Id.Guid : GetFailoverUnitId().Guid, 
        action_, 
        TraceCorrelatedEvent<T>(*body_));

    FailoverUnitMessageProcessor & processor = failoverManager_.MessageProcessor;

    __if_exists(T::ReplicaMessageBody)
    {
        if (action_ == RSMessage::GetReplicaDropped().Action)
        {
            processor.ReplicaDroppedProcessor(lockedFailoverUnit, action_, move(body_), from_, actions);
        }
        else if (action_ == RSMessage::GetReplicaEndpointUpdated().Action)
        {
            processor.ReplicaEndpointUpdatedProcessor(lockedFailoverUnit, move(body_), from_, actions);
        }
    }

    __if_exists(T::ReplicaReplyMessageBody)
    {
        if (action_ == RSMessage::GetAddInstanceReply().Action)
        {
            processor.AddInstanceReplyProcessor(lockedFailoverUnit, *body_);
        }
        else if (action_ == RSMessage::GetRemoveInstanceReply().Action)
        {
            processor.RemoveInstanceReplyProcessor(lockedFailoverUnit, action_, move(body_), from_);
        }
        else if (action_ == RSMessage::GetAddReplicaReply().Action)
        {
            processor.AddReplicaReplyProcessor(lockedFailoverUnit, *body_);
        }
        else if (action_ == RSMessage::GetAddPrimaryReply().Action)
        {
            processor.AddPrimaryReplyProcessor(lockedFailoverUnit, *body_);
        }
        else if (action_ == RSMessage::GetRemoveReplicaReply().Action)
        {
            processor.RemoveReplicaReplyProcessor(lockedFailoverUnit, *body_);
        }
        else if (action_ == RSMessage::GetDeleteReplicaReply().Action)
        {
            processor.DeleteReplicaReplyProcessor(lockedFailoverUnit, action_, move(body_), from_);
        }
    }

    __if_exists(T::ConfigurationMessageBody)
    {
        if (action_ == RSMessage::GetDatalossReport().Action)
        {
            processor.DatalossReportProcessor(lockedFailoverUnit, *body_, actions);
        }
        else if (action_ == RSMessage::GetChangeConfiguration().Action)
        {
            processor.ChangeConfigurationProcessor(lockedFailoverUnit, *body_, from_, actions);
        }
    }

    __if_exists(T::ConfigurationReplyMessageBody)
    {
        if (action_ == RSMessage::GetDoReconfigurationReply().Action)
        {
            processor.ReconfigurationReplyProcessor(lockedFailoverUnit, *body_);
        }
    }
}

template <class T>
bool FailoverUnitMessageTask<T>::IsStaleRequest(LockedFailoverUnitPtr const& failoverUnit)
{
    Epoch incomingEpoch = body_->FailoverUnitDescription.CurrentConfigurationEpoch;
    Epoch currentEpoch = failoverUnit->CurrentConfigurationEpoch;

    ASSERT_IF(
        currentEpoch < incomingEpoch,
        "FM received request {0} with incoming epoch newer than GFUM: {1}",
        *body_, *failoverUnit);

    // In the replica dropped case we check the staleness only based on replica instance id
    if (action_ != RSMessage::GetReplicaDropped().Action &&
        action_ != RSMessage::GetDeleteReplicaReply().Action &&
        (currentEpoch.ConfigurationVersion > incomingEpoch.ConfigurationVersion ||
         currentEpoch.DataLossVersion > incomingEpoch.DataLossVersion))
    {
        return true;
    }

    __if_exists(T::ReplicaDescription)
    {
        ReplicaDescription const & incomingReplica = body_->ReplicaDescription;
        Replica const* replica = failoverUnit->GetReplica(incomingReplica.FederationNodeId);
        if (replica == nullptr || replica->InstanceId > incomingReplica.InstanceId)
        {
            return true;
        }
    }

    return false;
}

template class Reliability::FailoverManagerComponent::FailoverUnitMessageTask<ReplicaMessageBody>;
template class Reliability::FailoverManagerComponent::FailoverUnitMessageTask<ReplicaReplyMessageBody>;
template class Reliability::FailoverManagerComponent::FailoverUnitMessageTask<ConfigurationMessageBody>;
template class Reliability::FailoverManagerComponent::FailoverUnitMessageTask<ConfigurationReplyMessageBody>;

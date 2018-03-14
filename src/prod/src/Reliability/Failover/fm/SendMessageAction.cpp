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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
SendMessageAction<T>::SendMessageAction(
    RSMessage const& message,
    NodeInstance const& target,
    T && body)
    : message_(message)
    , target_(target)
    , body_(std::move(body))
{
}

template <class T>
int SendMessageAction<T>::OnPerformAction(FailoverManager & failoverManager)
{
    failoverManager.FTEvents.FTMessageSend(
        body_.FailoverUnitDescription.FailoverUnitId.ToString(),
        message_.Action,
        target_.Id,
        target_.InstanceId,
        Common::TraceCorrelatedEvent<T>(body_));

    Transport::MessageUPtr message = message_.CreateMessage(body_, body_.FailoverUnitDescription.FailoverUnitId.Guid);

    GenerationHeader header(failoverManager.Generation, body_.FailoverUnitDescription.IsForFMReplica);

    message->Headers.Add(header);

    failoverManager.SendToNodeAsync(std::move(message), target_);

    return 1;
}

template <class T>
void SendMessageAction<T>::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    WriteBody(w);
}

template <class T>
void SendMessageAction<T>::WriteToEtw(uint16 contextSequenceId) const
{
    FailoverManager::FMEventSource->StateMachineAction(contextSequenceId, message_.Action);
}

template<>
void SendMessageAction<ReplicaMessageBody>::WriteBody(Common::TextWriter & w) const
{
    ReplicaDescription const& replica = body_.ReplicaDescription;

    w.Write("{0}->{1} [{2} {3}/{4}]",
        message_.Action,
        target_.Id,
        replica.FederationNodeId,
        replica.PreviousConfigurationRole,
        replica.CurrentConfigurationRole);
}

template<>
void SendMessageAction<DeleteReplicaMessageBody>::WriteBody(Common::TextWriter & w) const
{
    ReplicaDescription const& replica = body_.ReplicaDescription;

    w.Write("{0}->{1} [{2} {3}/{4}]",
        message_.Action,
        target_.Id,
        replica.FederationNodeId,
        replica.PreviousConfigurationRole,
        replica.CurrentConfigurationRole);
}

template<>
void SendMessageAction<ReplicaReplyMessageBody>::WriteBody(Common::TextWriter & w) const
{
    ReplicaDescription const& replica = body_.ReplicaDescription;

    w.Write(
        "{0}->{1} [{2} {3}/{4} {5}]",
        message_.Action,
        target_.Id,
        replica.FederationNodeId,
        replica.PreviousConfigurationRole,
        replica.CurrentConfigurationRole,
        body_.ErrorCode);
}

template<>
void SendMessageAction<FailoverUnitReplyMessageBody>::WriteBody(Common::TextWriter & w) const
{
    w.Write("{0}->{1} [{2}]", message_.Action, target_.Id, body_.ErrorCode);
}

template<>
void SendMessageAction<DoReconfigurationMessageBody>::WriteBody(Common::TextWriter & w) const
{
    w.Write("{0}->{1}", message_.Action, target_.Id);

    for (size_t i = 0; i < body_.ReplicaDescriptions.size(); i++)
    {
        ReplicaDescription const& replica = body_.ReplicaDescriptions[i];

        w.Write(
            " [{0} {1}/{2}]",
            replica.FederationNodeId,
            replica.PreviousConfigurationRole,
            replica.CurrentConfigurationRole);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AddInstanceAction::AddInstanceAction(Replica const& replica)
    : SendMessageAction(
        RSMessage::GetAddInstance(),
        replica.FederationNodeInstance,
        std::move(replica.CreateReplicaMessageBody(true)))
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RemoveInstanceAction::RemoveInstanceAction(Replica const& replica)
    : SendMessageAction(
        RSMessage::GetRemoveInstance(),
        replica.FederationNodeInstance,
        std::move(replica.CreateDeleteReplicaMessageBody()))
{
}

RemoveInstanceAction::RemoveInstanceAction(NodeInstance const& target, DeleteReplicaMessageBody && body)
    : SendMessageAction(
        RSMessage::GetRemoveInstance(),
        target,
        std::move(body))
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DoReconfigurationAction::DoReconfigurationAction(FailoverUnit const& failoverUnit)
    : SendMessageAction(
        RSMessage::GetDoReconfiguration(),
        failoverUnit.ReconfigurationPrimary->FederationNodeInstance,
        failoverUnit.CreateDoReconfigurationMessageBody())
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AddPrimaryAction::AddPrimaryAction(FailoverUnit const& failoverUnit)
    : SendMessageAction(
        RSMessage::GetAddPrimary(),
        failoverUnit.ReconfigurationPrimary->FederationNodeInstance,
        failoverUnit.Primary->CreateReplicaMessageBody(true))
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AddReplicaAction::AddReplicaAction(FailoverUnit const& failoverUnit, Replica const& replica)
    : SendMessageAction(
        RSMessage::GetAddReplica(),
        failoverUnit.ReconfigurationPrimary->FederationNodeInstance,
        replica.CreateReplicaMessageBody(true))
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RemoveReplicaAction::RemoveReplicaAction(Replica  & replica, NodeInstance const& target)
    : SendMessageAction(
        RSMessage::GetRemoveReplica(),
        target,
        replica.CreateReplicaMessageBody(false))
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DeleteReplicaAction::DeleteReplicaAction(Replica const& replica)
    : SendMessageAction(
        RSMessage::GetDeleteReplica(),
        replica.FederationNodeInstance,
        replica.CreateDeleteReplicaMessageBody())
{
}

DeleteReplicaAction::DeleteReplicaAction(NodeInstance const& target, Replica const& replica)
    : SendMessageAction(
        RSMessage::GetDeleteReplica(),
        target,
        replica.CreateDeleteReplicaMessageBody())
{
}

DeleteReplicaAction::DeleteReplicaAction(NodeInstance const& target, DeleteReplicaMessageBody && body)
    : SendMessageAction(
        RSMessage::GetDeleteReplica(),
        target,
        std::move(body))
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ReplicaDroppedReplyAction::ReplicaDroppedReplyAction(ReplicaReplyMessageBody && body, NodeInstance const& from)
    : SendMessageAction(
        RSMessage::GetReplicaDroppedReply(),
        from,
        std::move(body))
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ChangeConfigurationReplyAction::ChangeConfigurationReplyAction(FailoverUnit const& failoverUnit, NodeInstance const& from)
    : SendMessageAction(RSMessage::GetChangeConfigurationReply(),
        from,
        FailoverUnitReplyMessageBody(failoverUnit.FailoverUnitDescription, Common::ErrorCode()))
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ReplicaEndpointUpdatedReplyAction::ReplicaEndpointUpdatedReplyAction(ReplicaReplyMessageBody && body, NodeInstance const& from)
    : SendMessageAction(
        RSMessage::GetReplicaEndpointUpdatedReply(),
        from,
        std::move(body))
{
}

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

NodeDeactivationMessageProcessor::NodeDeactivationMessageProcessor(
    ReconfigurationAgent & ra) :
    processor_(make_unique<NodeDeactivationStateProcessor>(ra)),
    ra_(ra)
{
}

void NodeDeactivationMessageProcessor::ProcessActivateMessage(Transport::Message & request)
{
    NodeActivateRequestMessageBody body;
    if (!Infrastructure::TryGetMessageBody(ra_.NodeInstanceIdStr, request, body))
    {
        return;
    }

    Process(wformatString(request.MessageId), body.Sender, NodeDeactivationInfo(true, body.SequenceNumber));
}

void NodeDeactivationMessageProcessor::ProcessDeactivateMessage(Transport::Message & request)
{
    NodeDeactivateRequestMessageBody body;
    if (!Infrastructure::TryGetMessageBody(ra_.NodeInstanceIdStr, request, body))
    {
        return;
    }

    Process(wformatString(request.MessageId), body.Sender, NodeDeactivationInfo(false, body.SequenceNumber));
}

void NodeDeactivationMessageProcessor::Process(
    std::wstring const & activityId,
    Reliability::FailoverManagerId const & sender,
    NodeDeactivationInfo const & newInfo)
{
    if (!ra_.StateObj.GetIsReady(sender.IsFmm))
    {
        return;
    }

    ChangeActivationState(activityId, sender, newInfo);

    // For node deactivation, the reply will be sent to fm after all replicas are closed, not immediately
    if (newInfo.IsActivated)
    {
        SendReply(activityId, sender, newInfo);
    }
}

void NodeDeactivationMessageProcessor::ProcessNodeUpAck(
    std::wstring const & activityId,
    Reliability::FailoverManagerId const & sender,
    NodeDeactivationInfo const & newInfo)
{
    ChangeActivationState(activityId, sender, newInfo);
}

void NodeDeactivationMessageProcessor::ChangeActivationState(
    std::wstring const & activityId,
    Reliability::FailoverManagerId const & sender,
    NodeDeactivationInfo const & newInfo)
{
    Trace(activityId, sender, newInfo);

    /*
        For activate request update activate both FM and FMM as there are no safety checks to be performed
        if and only if the sender is FM because the FM state is reliable
        This also helps alleviate issues where FMM forgets about the deactivation state during rebuild
     
        For deactivate deactivate only the sender of the request
    */
    vector<FailoverManagerId> toBeChanged;

    if (newInfo.IsActivated && !sender.IsFmm)
    {
        toBeChanged.push_back(*FailoverManagerId::Fmm);
        toBeChanged.push_back(*FailoverManagerId::Fm);
    }
    else
    {
        toBeChanged.push_back(sender);        
    }

    for (auto const & it : toBeChanged)
    {
        processor_->ProcessActivationStateChange(activityId, ra_.NodeStateObj.GetNodeDeactivationState(it), newInfo);
    }
}

void NodeDeactivationMessageProcessor::Trace(
    std::wstring const & activityId,
    Reliability::FailoverManagerId const & sender,
    NodeDeactivationInfo const & newInfo) const
{
    auto fmState = ra_.NodeStateObj.GetNodeDeactivationState(*FailoverManagerId::Fm).DeactivationInfo;
    auto fmmState = ra_.NodeStateObj.GetNodeDeactivationState(*FailoverManagerId::Fmm).DeactivationInfo;

    Infrastructure::RAEventSource::Events->LifeCycleNodeActivationMessageProcess(
        ra_.NodeInstanceIdStr,
        newInfo,
        sender,
        fmState,
        fmmState,
        activityId);
}

void NodeDeactivationMessageProcessor::SendReply(
    std::wstring const & activityId,
    Reliability::FailoverManagerId const & sender,
    NodeDeactivationInfo const & newInfo) const
{
    ASSERT_IF(!newInfo.IsActivated, "NodeDeactivationReply should not be sent here");

    ra_.FMTransportObj.SendMessageToFM(sender, RSMessage::GetNodeActivateReply(), activityId, NodeActivateReplyMessageBody(newInfo.SequenceNumber));
}

void NodeDeactivationMessageProcessor::Close()
{
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace ReconfigurationAgentComponent::Communication;

FMMessageBuilder::FMMessageBuilder(
    FMTransport & transport,
    Reliability::FailoverManagerId const & target) :
    transport_(transport),
    target_(target)
{
}

void FMMessageBuilder::Send(
    std::wstring const & activityId,
    FMMessageDescription & desc)
{
    Infrastructure::EntityEntryBaseSPtr const & entry = desc.Source;
    FMMessageData & message = desc.Message;

    switch (message.FMMessageStage)
    {
    case FMMessageStage::ReplicaDown: 
    case FMMessageStage::ReplicaUpload: 
    case FMMessageStage::ReplicaUp:
    case FMMessageStage::ReplicaDropped: 
        AddReplicaUp(entry, message.TakeReplicaUp());
        break;

    case FMMessageStage::EndpointAvailable: 
        SendReplicaMessage(activityId, entry, RSMessage::GetReplicaEndpointUpdated(), message.TakeReplicaMessage());
        break;

    case FMMessageStage::None: 
        __fallthrough;
    default: 
        Assert::TestAssert("Unknown message stage at fm message builder {0}", static_cast<int>(message.FMMessageStage));
    }
}

void FMMessageBuilder::TakeEntriesInReplicaUpMessage(
    __out Infrastructure::EntityEntryBaseSet & entries)
{
    entries = move(replicaUpMessageEntities_);
}

void FMMessageBuilder::AddReplicaUp(
    Infrastructure::EntityEntryBaseSPtr const & entry,
    FMMessageData::ReplicaUpElement && item)
{
    replicaUpMessageEntities_.insert(entry);
    
    if (item.second)
    {
        replicaUpDroppedList_.push_back(move(item.first));
    }
    else
    {
        replicaUpNormalList_.push_back(move(item.first));
    }
}

void FMMessageBuilder::Finalize(std::wstring const & activityId, bool isLast)
{
    SendReplicaUp(activityId, isLast);
}

void FMMessageBuilder::SendReplicaUp(
    std::wstring const & activityId,
    bool isLast)
{
    if (replicaUpNormalList_.empty() && replicaUpDroppedList_.empty())
    {
        return;
    }

    ReplicaUpMessageBody body(move(replicaUpNormalList_), move(replicaUpDroppedList_), isLast, target_.IsFmm);
    transport_.SendMessageToFM(target_, RSMessage::GetReplicaUp(), activityId, body);
}

void FMMessageBuilder::SendReplicaMessage(
    std::wstring const & activityId,
    Infrastructure::EntityEntryBaseSPtr const & entry,
    RSMessage const & message,
    ReplicaMessageBody const & body)
{
    transport_.SendEntityMessageToFM<ReplicaMessageBody>(
        target_,
        message,
        entry,
        activityId,
        body);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Microsoft Corporation.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

using namespace Common;
using namespace Management::NetworkInventoryManager;
using namespace std;

// Actors for all messages sent to the Network Inventory Manager
Transport::Actor::Enum const NIMMessage::NIMActor(Transport::Actor::NetworkInventoryService);

//
// Messages used by external / internal APIs exposed by Network Inventory Manager
//

Global<NIMMessage> NIMMessage::CreateNetwork = CreateCommonToNIMMessage(L"CreateNetwork");
Global<NIMMessage> NIMMessage::DeleteNetwork = CreateCommonToNIMMessage(L"DeleteNetwork");
Global<NIMMessage> NIMMessage::ValidateNetwork = CreateCommonToNIMMessage(L"ValidateNetwork");

void NIMMessage::AddActivityHeader(Transport::Message & message, Transport::FabricActivityHeader const & activityHeader)
{
    bool exists = false;
    auto iter = message.Headers.Begin();
    while (iter != message.Headers.End())
    {
        if (iter->Id() == Transport::FabricActivityHeader::Id)
        {
            exists = true;
            break;
        }

        ++iter;
    }

    if (exists)
    {
        message.Headers.Remove(iter);
    }

    message.Headers.Add(activityHeader);
}

ActivityId NIMMessage::GetActivityId(Transport::Message & message)
{
    Transport::FabricActivityHeader header;
    if (!message.Headers.TryReadFirst(header))
    {		
        Assert::TestAssert("NIMMessage::GetActivityId Missing activity id header");
    }
    return header.ActivityId;
}

void NIMMessage::AddHeaders(Transport::Message & message) const
{
    message.Headers.Add(Transport::ActorHeader(actor_));
    message.Headers.Add(actionHeader_);
    message.Idempotent = idempotent_;
}

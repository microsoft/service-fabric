// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TestCommon;
using namespace std;
using namespace Transport;

const Actor::Enum DistributedSessionMessage::Actor(Transport::Actor::DistributedSession);

const wstring DistributedSessionMessage::ExecuteCommandAction(L"ExecuteCommand");
const wstring DistributedSessionMessage::ConnectAction(L"Connect");
const wstring DistributedSessionMessage::ConnectReplyAction(L"ConnectReply");

void DistributedSessionMessage::SetHeaders(Message & message, Transport::Actor::Enum actor, wstring const & action)
{
    message.Headers.Add(ActionHeader(action));
    message.Headers.Add(ActorHeader(actor));
}

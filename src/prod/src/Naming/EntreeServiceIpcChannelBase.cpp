// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace Transport;
using namespace ServiceModel;
using namespace Naming;
using namespace SystemServices;

static const Common::StringLiteral TraceType("EntreeServiceIpcChannelBase");

EntreeServiceIpcChannelBase::EntreeServiceIpcChannelBase(ComponentRoot const & root)
    : RootedObject(root)
{
}

bool EntreeServiceIpcChannelBase::RegisterHandler(
    Actor::Enum actor,
    BeginFunction const &beginFunction,
    EndFunction const &endFunction)
{
    AcquireWriteLock writeLock(handlerMapLock_);
    if (handlerMap_.find(actor) != handlerMap_.end())
    {
        WriteWarning(
            TraceType,
            "Actor {0} already registered",
            actor);
        return false;
    }

    auto handler = make_pair(beginFunction, endFunction);
    handlerMap_.insert(make_pair(actor, handler));
    return true;
}

bool EntreeServiceIpcChannelBase::UnregisterHandler(Actor::Enum actor)
{
    AcquireWriteLock writeLock(handlerMapLock_);
    if (handlerMap_.erase(actor) != 1)
    {
        WriteWarning(
            TraceType,
            "Unregistering an Actor: {0} that was not registered",
            actor);
        return false;
    }
    return true;
}

void EntreeServiceIpcChannelBase::SendIpcReply(MessageUPtr && reply, ReceiverContext const &receiverContext)
{
    receiverContext.Reply(move(reply));
}

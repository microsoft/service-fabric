// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Transport;

IpcDemuxer::IpcDemuxer(ComponentRoot const & root, IDatagramTransportSPtr const & datagramTransport) 
    : DemuxerT<Actor::Enum, IpcReceiverContext>(root, datagramTransport)
{
}

IpcReceiverContextUPtr IpcDemuxer::CreateReceiverContext(Message & message, ISendTarget::SPtr const & replyTargetSPtr)
{
    if (!replyTargetSPtr->IsAnonymous())
    {
        // remote side is IpcServer, no IpcHeader on the message
        return make_unique<IpcReceiverContext>(IpcHeader(), message.MessageId, replyTargetSPtr, datagramTransport_);
    }

    IpcHeader ipcHeader;
    if (message.Headers.TryReadFirst(ipcHeader))
    {
        return make_unique<IpcReceiverContext>(std::move(ipcHeader), message.MessageId, replyTargetSPtr, datagramTransport_);
    }

    IpcClient::WriteWarning(Constants::DemuxerTrace, "dropping message {0}: failed to read IpcHeader, Actor = {1}, Action = '{2}'", message.MessageId, message.Actor, message.Action);
    replyTargetSPtr->Reset();
    return std::unique_ptr<IpcReceiverContext>(nullptr);
}

Actor::Enum IpcDemuxer::GetActor(Message & message)
{
    return message.Actor;
}

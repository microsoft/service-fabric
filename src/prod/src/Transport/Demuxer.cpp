// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Transport;

Demuxer::Demuxer(ComponentRoot const & root, IDatagramTransportSPtr const & datagramTransport)
    : DemuxerT<Actor::Enum, ReceiverContext>(root, datagramTransport)
{
}

ReceiverContextUPtr Demuxer::CreateReceiverContext(Message & message, ISendTarget::SPtr const & replyTargetSPtr)
{
    return Common::make_unique<ReceiverContext>(message.MessageId, replyTargetSPtr, this->datagramTransport_);
}

Actor::Enum Demuxer::GetActor(Message & message)
{
    return message.Actor;
}

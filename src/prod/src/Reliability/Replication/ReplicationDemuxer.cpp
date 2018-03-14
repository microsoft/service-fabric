// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Transport;
using namespace Reliability::ReplicationComponent;

ReplicationDemuxer::ReplicationDemuxer(ComponentRoot const & root, IDatagramTransportSPtr const & datagramTransport)
    : DemuxerT<ReplicationEndpointId, ReceiverContext>(root, datagramTransport)
{
}

ReceiverContextUPtr ReplicationDemuxer::CreateReceiverContext(Message & message, ISendTarget::SPtr const & replyTargetSPtr)
{
    return Common::make_unique<ReceiverContext>(message.MessageId, replyTargetSPtr, this->datagramTransport_);
}

ReplicationEndpointId ReplicationDemuxer::GetActor(Message & message)
{
    ReplicationActorHeader actorHeader;

    message.Headers.TryReadFirst(actorHeader);

    return actorHeader.Actor;
}

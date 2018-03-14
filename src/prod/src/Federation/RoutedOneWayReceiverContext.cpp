// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace Federation;
using namespace std;

RoutedOneWayReceiverContext::RoutedOneWayReceiverContext(
    SiteNodeSPtr siteNode,
    PartnerNodeSPtr const & from, 
    NodeInstance const & fromInstance,
    Transport::MessageId const & relatesToId,
    RequestReceiverContextUPtr && requestContext,
    bool sendAckToPreviousHop,
    bool isIdempotent)
    :   OneWayReceiverContext(from, fromInstance, relatesToId),
        siteNode_(siteNode),
        requestContext_(move(requestContext)),
        sendAckToPreviousHop_(sendAckToPreviousHop),
        isIdempotent_(isIdempotent)
{
}

RoutedOneWayReceiverContext::~RoutedOneWayReceiverContext()
{
}

void RoutedOneWayReceiverContext::Accept()
{
    // 'from' is the original sender
    auto ack = FederationMessage::GetRoutingAck().CreateMessage();
    ack->Headers.Add(RelatesToHeader(this->RelatesToId));
    this->SendReply(move(ack));
}

void RoutedOneWayReceiverContext::Reject(ErrorCode const & error, ActivityId const & activityId)
{
    auto nack = FederationMessage::GetRejectFault().CreateMessage();
    nack->Headers.Add(FaultHeader(error.ReadValue()));
    nack->Headers.Add(RelatesToHeader(this->RelatesToId));
    if (!activityId.IsEmpty)
    {
        nack->Headers.Add(FabricActivityHeader(activityId));
    }

    this->SendReply(move(nack));
}

void RoutedOneWayReceiverContext::SendReply(Transport::MessageUPtr && reply) const
{
    this->siteNode_->GetRoutingManager().SendReply(move(reply), this->From, this->RelatesToId, requestContext_, sendAckToPreviousHop_, isIdempotent_);
}

void RoutedOneWayReceiverContext::Ignore()
{
    siteNode_->GetRoutingManager().TryRemoveMessageIdFromProcessingSet(this->RelatesToId);
}

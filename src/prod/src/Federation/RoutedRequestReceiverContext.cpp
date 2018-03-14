// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace Federation;
using namespace std;

RoutedRequestReceiverContext::RoutedRequestReceiverContext(
    SiteNodeSPtr siteNode,
    PartnerNodeSPtr const & from,
    NodeInstance const & fromInstance, 
    Transport::MessageId const & relatesToId,
    RequestReceiverContextUPtr && requestContext,
    bool sendAckToPreviousHop,
    bool isIdempotent)
    :   RequestReceiverContext(siteNode, from, fromInstance, relatesToId),
        requestContext_(move(requestContext)),
        sendAckToPreviousHop_(sendAckToPreviousHop),
        isIdempotent_(isIdempotent)
{
}

RoutedRequestReceiverContext::~RoutedRequestReceiverContext()
{
}

void RoutedRequestReceiverContext::Reply(MessageUPtr && reply)
{
    reply->Headers.Add(RelatesToHeader(this->RelatesToId));
    this->siteNode_->GetRoutingManager().SendReply(move(reply), this->From, this->RelatesToId, requestContext_, sendAckToPreviousHop_, isIdempotent_);
}

void RoutedRequestReceiverContext::Ignore()
{
    siteNode_->GetRoutingManager().TryRemoveMessageIdFromProcessingSet(this->RelatesToId);
}

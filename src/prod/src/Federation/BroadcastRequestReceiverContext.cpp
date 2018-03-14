// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace Federation;
using namespace std;

BroadcastRequestReceiverContext::BroadcastRequestReceiverContext(
    SiteNodeSPtr const & siteNode,
    RequestReceiverContextUPtr && routedRequestContext,
    PartnerNodeSPtr const & from,
    NodeInstance const & fromInstance,
    MessageId const & relatesToId)
    :   RequestReceiverContext(siteNode, from, fromInstance, relatesToId),
        routedRequestContext_(move(routedRequestContext))
{
}

BroadcastRequestReceiverContext::~BroadcastRequestReceiverContext()
{
}

void BroadcastRequestReceiverContext::Reply(MessageUPtr && reply)
{
    reply->Idempotent = true;

    reply->Headers.Add(BroadcastRelatesToHeader(this->RelatesToId));

    if (routedRequestContext_)
    {
        reply->Headers.Add(BroadcastRangeHeader(this->siteNode_->Token.Range));
        routedRequestContext_->Reply(std::move(reply));
    }
    else
    {
        this->siteNode_->GetPointToPointManager().PToPSend(std::move(reply), this->From, true, PToPActor::Broadcast);
    }
}

void BroadcastRequestReceiverContext::Ignore()
{
    if (routedRequestContext_)
    {
        routedRequestContext_->Ignore();
    }
}

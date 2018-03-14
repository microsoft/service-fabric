// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace Federation;
using namespace std;

RequestReceiverContext::RequestReceiverContext(
    SiteNodeSPtr const & siteNode,
    PartnerNodeSPtr const & from,
    NodeInstance const & fromInstance,
    MessageId const & relatesToId)
    :   ReceiverContext(from, fromInstance, relatesToId),
        siteNode_(siteNode)
{        
}

RequestReceiverContext::~RequestReceiverContext()
{
}

void RequestReceiverContext::Reply(MessageUPtr && reply)
{
    reply->Headers.Add(RelatesToHeader(this->RelatesToId));
    // 'from' is the original sender
    this->siteNode_->GetPointToPointManager().PToPSend(move(reply), this->From, true, PToPActor::Direct);
}

void RequestReceiverContext::Reject(ErrorCode const & error, ActivityId const & activityId)
{
    ErrorCode rejectError;
    if (error.IsError(ErrorCodeValue::RoutingNodeDoesNotMatchFault))
    {
        rejectError = ErrorCodeValue::RoutingError;
    }
    else if (error.IsError(ErrorCodeValue::P2PNodeDoesNotMatchFault))
    {
        rejectError = ErrorCodeValue::P2PError;
    }
    else
    {
        rejectError = error;
    }

    InternalReject(rejectError, activityId);
}

void RequestReceiverContext::InternalReject(ErrorCode const & error, ActivityId const & activityId)
{
    MessageUPtr nack;
    if (error.Message.empty())
    {
        nack = FederationMessage::GetRejectFault().CreateMessage();
    }
    else
    {
        nack = FederationMessage::GetRejectFault().CreateMessage(RejectFaultBody(error));
    }
    nack->Headers.Add(FaultHeader(error.ReadValue(), !error.Message.empty()));
    if (!activityId.IsEmpty)
    {
        nack->Headers.Add(FabricActivityHeader(activityId));
    }

    this->Reply(std::move(nack));
}

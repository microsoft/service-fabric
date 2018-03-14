// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Transport;

ReceiverContext::ReceiverContext(Transport::MessageId const & messageId, ISendTarget::SPtr const & replyTargetSPtr, IDatagramTransportSPtr const & datagramTransport)
    : messageId_(messageId), replyTargetSPtr_(replyTargetSPtr), datagramTransport_(datagramTransport)
{
}

ReceiverContext::~ReceiverContext() 
{
}

Transport::MessageId const & ReceiverContext::getMessageId() const
{
    return this->messageId_;
}

ErrorCode ReceiverContext::Reply(Transport::MessageUPtr && reply, TransportPriority::Enum priority) const
{
    reply->Headers.Add(Transport::RelatesToHeader(this->messageId_));
    auto replyExpiry = timeoutHeader_.GetRemainingTime();
    if (replyExpiry == TimeSpan::MaxValue)
    {
        replyExpiry = TransportConfig::GetConfig().DefaultOutgoingMessageExpiration;
    }

    return this->datagramTransport_->SendOneWay(
        this->replyTargetSPtr_, 
        std::move(reply),
        replyExpiry,
        priority);
}

ISendTarget::SPtr const & ReceiverContext::getReplyTarget() const
{
    return this->replyTargetSPtr_;
}

void ReceiverContext::SetTimeoutHeader(TimeoutHeader const & timeoutHeader)
{
    timeoutHeader_ = timeoutHeader;
}

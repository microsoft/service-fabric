// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace std;

const Common::StringLiteral MulticastLifetimeSource = "Multicast.Lifetime";

MulticastDatagramSender::MulticastDatagramSender(IDatagramTransportSPtr unicastTransport)
    : transport_(unicastTransport)
{
}

MulticastDatagramSender::~MulticastDatagramSender()
{
}

void MulticastDatagramSender::Send(
        MulticastSendTargetSPtr const & multicastTarget,
        MessageUPtr && message,
        MessageHeadersCollection && targetHeaders)
{
    MessageHeadersCollection headersCollection = std::move(targetHeaders);

    message->CheckPoint();

    if (headersCollection.size() == 0)
    {
        for(size_t i = 0; i < multicastTarget->size(); i++)
        {
            transport_->SendOneWay(
                (*multicastTarget)[i], 
                message->Clone());
        }
    }
    else
    {
        ASSERT_IF(headersCollection.size() != multicastTarget->size(),
            "The size of targetHeaders (MessageHeadersCollection) must match the number of targets identified by specified multicastTarget.");

        for(size_t i = 0; i < multicastTarget->size(); i++)
        {
            transport_->SendOneWay(
                (*multicastTarget)[i], 
                message->Clone(std::move(headersCollection[i])));
        }
    }
}

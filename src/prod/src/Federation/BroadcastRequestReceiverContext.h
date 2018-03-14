// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class BroadcastRequestReceiverContext : public RequestReceiverContext
    {
    public:
        BroadcastRequestReceiverContext(
            SiteNodeSPtr const & siteNode,
            RequestReceiverContextUPtr && routedRequestContext,
            PartnerNodeSPtr const & from,
            NodeInstance const & fromInstance,
            Transport::MessageId const & relatesToId);

        ~BroadcastRequestReceiverContext();

        virtual void Reply(Transport::MessageUPtr && reply);

        virtual void Ignore();

    private:
        RequestReceiverContextUPtr routedRequestContext_;
    };
}

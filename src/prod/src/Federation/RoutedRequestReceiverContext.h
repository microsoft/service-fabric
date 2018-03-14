// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class RoutedRequestReceiverContext : public RequestReceiverContext
    {
    public:
        RoutedRequestReceiverContext(
            SiteNodeSPtr siteNode,
            PartnerNodeSPtr const & from,
            NodeInstance const & fromInstance, 
            Transport::MessageId const & relatesToId,
            RequestReceiverContextUPtr && requestContext,
            bool sendAckToPreviousHop,
            bool isIdempotent);

        ~RoutedRequestReceiverContext();

        virtual void Reply(Transport::MessageUPtr && reply);

        virtual void Ignore();

    private:
        RequestReceiverContextUPtr requestContext_;
        bool sendAckToPreviousHop_;
        bool isIdempotent_;
    };

    typedef std::unique_ptr<RoutedRequestReceiverContext> RoutedRequestReceiverContextUPtr;
}

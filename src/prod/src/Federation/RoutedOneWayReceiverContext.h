// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class RoutedOneWayReceiverContext : public OneWayReceiverContext
    {
    public:
        RoutedOneWayReceiverContext(
            SiteNodeSPtr siteNode,
            PartnerNodeSPtr const & from, 
            NodeInstance const & fromInstance,
            Transport::MessageId const & relatesToId,
            RequestReceiverContextUPtr && requestContext,
            bool sendAckToPreviousHop,
            bool isIdempotent);

        ~RoutedOneWayReceiverContext();

        virtual void Accept();

        virtual void Reject(Common::ErrorCode const & error, Common::ActivityId const & activityId = Common::ActivityId::Empty);

        virtual void Ignore();

    private:
        void SendReply(Transport::MessageUPtr && reply) const;

        SiteNodeSPtr siteNode_;
        RequestReceiverContextUPtr requestContext_;
        bool sendAckToPreviousHop_;
        bool isIdempotent_;
    };
}

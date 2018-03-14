// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class RequestReceiverContext : public ReceiverContext
    {
    public:
        RequestReceiverContext(
            SiteNodeSPtr const & siteNode,
            PartnerNodeSPtr const & from,
            NodeInstance const & fromInstance,
            Transport::MessageId const & relatesToId);

        ~RequestReceiverContext();

        virtual void Reject(Common::ErrorCode const & error, Common::ActivityId const & activityId = Common::ActivityId::Empty);

        virtual void Reply(Transport::MessageUPtr && reply);

        void InternalReject(Common::ErrorCode const & error, Common::ActivityId const & activityId = Common::ActivityId::Empty);

    protected:
        SiteNodeSPtr siteNode_;
    };
}

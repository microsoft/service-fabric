// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    typedef std::function<void(Transport::MessageUPtr & message, PartnerNodeSPtr const & to)> SendCallback;

    class ReceiverContext
    {
        DENY_COPY(ReceiverContext)

    public:
        ReceiverContext(
            PartnerNodeSPtr const & from,
            NodeInstance const & fromInstance,
            Transport::MessageId const & relatesToId);

        virtual ~ReceiverContext();

        // Properties
        __declspec(property(get=get_RelatesToId)) Transport::MessageId const & RelatesToId;
        Transport::MessageId const & get_RelatesToId() const { return relatesToId_; }

        __declspec (property(get=get_From)) PartnerNodeSPtr const & From;
        PartnerNodeSPtr const & get_From() const { return this->from_; }

        __declspec (property(get = get_From)) PartnerNodeSPtr const & FromWithSameInstance;
        PartnerNodeSPtr const & get_FromWithSameInstance() const;

        __declspec (property(get=get_FromInstance)) NodeInstance const & FromInstance;
        NodeInstance const & get_FromInstance() const { return this->fromInstance_; }

        virtual void Reject(Common::ErrorCode const & error, Common::ActivityId const & activityId = Common::ActivityId::Empty) = 0;

        virtual void Ignore() {};

    private:
        PartnerNodeSPtr from_;
        NodeInstance fromInstance_;
        Transport::MessageId relatesToId_;
    };
}

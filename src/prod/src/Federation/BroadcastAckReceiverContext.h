// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class BroadcastAckReceiverContext : public OneWayReceiverContext
    {
    public:
        BroadcastAckReceiverContext(
            BroadcastManager & manager,
            Transport::MessageId const & broadcastId,
            PartnerNodeSPtr const & from,
            NodeInstance const & fromInstance);

        ~BroadcastAckReceiverContext();

        virtual void Accept();

        virtual void Reject(Common::ErrorCode const & error, Common::ActivityId const & activityId = Common::ActivityId::Empty);

        virtual void Ignore();

    private:
        BroadcastManager & manager_;
        Transport::MessageId broadcastId_;
    };
}

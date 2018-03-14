// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class MulticastAckReceiverContext : public OneWayReceiverContext
    {
    public:
        MulticastAckReceiverContext(
            MulticastManager & manager,
            Transport::MessageId const & multicastId,
            PartnerNodeSPtr const & from,
            NodeInstance const & fromInstance);

        ~MulticastAckReceiverContext();

        virtual void Accept();

        virtual void Reject(Common::ErrorCode const & error);

        virtual void Ignore();

    private:
        MulticastManager & manager_;
        Transport::MessageId multicastId_;
    };
}

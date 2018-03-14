// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class OneWayReceiverContext : public ReceiverContext
    {
    public:
        OneWayReceiverContext(
            PartnerNodeSPtr const & from,
            NodeInstance const & fromInstance,
            Transport::MessageId const & relatesToId);

        ~OneWayReceiverContext();

        virtual void Accept();

        virtual void Reject(Common::ErrorCode const & error, Common::ActivityId const & activityId = Common::ActivityId::Empty);
    };
}

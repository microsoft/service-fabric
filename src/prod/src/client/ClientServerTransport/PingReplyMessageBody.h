// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ClientServerTransport
{
    class PingReplyMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        PingReplyMessageBody()
            : gatewayDescription_()
        {
        }

        PingReplyMessageBody(
            Naming::GatewayDescription const & gatewayDescription)
            : gatewayDescription_(gatewayDescription)
        {
        }

        PingReplyMessageBody(
            Naming::GatewayDescription && gatewayDescription)
            : gatewayDescription_(std::move(gatewayDescription))
        {
        }

        Naming::GatewayDescription && TakeDescription() { return std::move(gatewayDescription_); }

        FABRIC_FIELDS_01(gatewayDescription_);

    private:
        Naming::GatewayDescription gatewayDescription_;
    };
}

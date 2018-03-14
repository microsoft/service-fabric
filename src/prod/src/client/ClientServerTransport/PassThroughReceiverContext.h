// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class PassThroughReceiverContext : public Transport::ReceiverContext
    {
        DENY_COPY_ASSIGNMENT(PassThroughReceiverContext)

    public:
        PassThroughReceiverContext(
            Common::AsyncOperationSPtr const &internalContext,
            Transport::MessageId const &messageId,
            Transport::ISendTarget::SPtr const &sendTargetSPtr,
            ClientServerPassThroughTransport &transport)
            : ReceiverContext(messageId, sendTargetSPtr, nullptr)
            , internalContext_(internalContext)
            , transport_(transport)
        {
        }

        virtual void Reply(Transport::MessageUPtr& reply, Transport::TransportPriority::Enum = Transport::TransportPriority::Normal) const;

    private:

        ClientServerPassThroughTransport &transport_;
        Common::AsyncOperationSPtr const &internalContext_;
    };
}

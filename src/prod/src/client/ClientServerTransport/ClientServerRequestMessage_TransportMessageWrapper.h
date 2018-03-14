// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class ClientServerRequestMessageTransportMessageWrapper : public ClientServerRequestMessage
    {
    public:

        ClientServerRequestMessageTransportMessageWrapper(Transport::MessageUPtr && message)
            : ClientServerRequestMessage(),
            message_(std::move(message))
        {
        }

        static ClientServerRequestMessageUPtr Create(Transport::MessageUPtr && message)
        {
            return std::move(Common::make_unique<ClientServerRequestMessageTransportMessageWrapper>(std::move(message)));
        }

        Transport::MessageUPtr GetTcpMessage() override
        {
            return message_->Clone();
        }

        Transport::MessageHeaders & get_Headers() override
        {
            return message_->Headers;
        }

        std::wstring const & get_Action() override
        {
            return message_->Action;
        }

        Transport::Actor::Enum get_Actor() override
        {
            return message_->Actor;
        }

        Common::ActivityId get_ActivityId()  override
        {
            return Transport::FabricActivityHeader::FromMessage(*message_).ActivityId;
        }

    private:

        Transport::MessageUPtr message_;
    };
}

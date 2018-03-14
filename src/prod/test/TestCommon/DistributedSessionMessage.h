// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestCommon
{
    class DistributedSessionMessage
    {
    public:
        //SessionMessage Actor
        static const Transport::Actor::Enum Actor;

        //SessionMessage Actions
        static const std::wstring ConnectAction;
        static const std::wstring ConnectReplyAction;
        static const std::wstring ExecuteCommandAction;

        static Transport::MessageUPtr GetReportClientDataMessage(std::wstring const & action, std::vector<byte> && data)
        {
            ClientDataBody clientDataBody(move(data));
            Transport::MessageUPtr message(Common::make_unique<Transport::Message>(clientDataBody));
            SetHeaders(*message, DistributedSessionMessage::Actor, action);
            return std::move(message);
        }

        static Transport::MessageUPtr GetExecuteCommandMessage(std::wstring const & command)
        {
            StringBody stringBody(command);
            Transport::MessageUPtr message(Common::make_unique<Transport::Message>(stringBody));
            SetHeaders(*message, DistributedSessionMessage::Actor, DistributedSessionMessage::ExecuteCommandAction);
            return std::move(message);
        }

        static Transport::MessageUPtr GetConnectMessage()
        {
            Transport::MessageUPtr message(Common::make_unique<Transport::Message>(ConnectMessageBody(GetCurrentProcessId())));
            SetHeaders(*message, DistributedSessionMessage::Actor, DistributedSessionMessage::ConnectAction);
            return std::move(message);
        }

        static Transport::MessageUPtr GetConnectReplyMessage()
        {
            Transport::MessageUPtr message(Common::make_unique<Transport::Message>());
            SetHeaders(*message, DistributedSessionMessage::Actor, DistributedSessionMessage::ConnectReplyAction);
            return std::move(message);
        }

        static void SetHeaders(Transport::Message &, Transport::Actor::Enum, std::wstring const &);
    };
};

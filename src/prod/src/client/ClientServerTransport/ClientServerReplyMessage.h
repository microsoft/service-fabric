// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class ClientServerReplyMessage
    {
    public:

        ClientServerReplyMessage(Transport::MessageUPtr msg)
            : transportMessage_(std::move(msg))
        {
        }

        // Needed to interop between HealthReportingTransport interface (which expects MessageUPtr) and fabricclient (which expects ClientServerReplyMessage) 
        // TODO: REMOVE after HealthReportingTransport interface is removed
        Transport::MessageUPtr GetTcpMessage()
        {
            Transport::MessageUPtr transportMessage = std::move(transportMessage_);
            transportMessage_ = nullptr;
            return std::move(transportMessage);
        }

        __declspec(property(get=get_Headers)) Transport::MessageHeaders & Headers;
        Transport::MessageHeaders & get_Headers()
        {
            // if (transportMessage_)
            return transportMessage_->Headers;
            // else return httpMessage_->...
        }

        __declspec(property(get=get_Action)) std::wstring const & Action;
        std::wstring const & get_Action()
        {
            // if (transportMessage_)
            return transportMessage_->Action;
            // else return httpMessage_->...
        }

        __declspec(property(get=get_Actor)) Transport::Actor::Enum Actor;
        Transport::Actor::Enum const get_Actor()
        {
            // if (transportMessage_)
            return transportMessage_->Actor;
            // else reutnr httpMessage_->...
        }

        __declspec(property(get=get_ActivityId)) Common::ActivityId const ActivityId;
        Common::ActivityId const get_ActivityId() 
        {
            // if (transportMessage_)
            return Transport::FabricActivityHeader::FromMessage(*transportMessage_).ActivityId;
            // else reutnr httpMessage_->...
        }

        template<class TBody>
        bool GetBody(TBody & body)
        {
            if (transportMessage_)
            {
                return transportMessage_->GetBody(body);
            }
            else
            {
                // return httpMessage_->...
                return false;
            }
        }

        NTSTATUS GetStatus()
        {
            // if (transportMessage_)
            return transportMessage_->GetStatus();
        }

    private:

        Transport::MessageUPtr transportMessage_;
        //  HttpMessageUPtr httpMessage_;
    };

    typedef std::unique_ptr<ClientServerReplyMessage> ClientServerReplyMessageUPtr;
}

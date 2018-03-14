// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class ClientServerRequestMessage
    {
    public:

        virtual Transport::MessageUPtr GetTcpMessage()
        {
            return std::move(CreateTcpMessage());
        }

        // TODO: implement Http transport message
        // virtual ... GetHttpMessage() = 0;

        __declspec(property(get=get_Headers)) Transport::MessageHeaders & Headers;
        virtual Transport::MessageHeaders & get_Headers()
        {
            return headers_;
        }

        __declspec(property(get=get_Action)) std::wstring const & Action;
        virtual std::wstring const & get_Action()
        {
            return headers_.Action;
        }

        __declspec(property(get=get_Actor)) Transport::Actor::Enum Actor;
        virtual Transport::Actor::Enum get_Actor()
        {
            return headers_.Actor;
        }

        __declspec(property(get=get_ActivityId)) Common::ActivityId ActivityId;
        virtual Common::ActivityId get_ActivityId() 
        {
            Transport::FabricActivityHeader activityHeader;
            if (! Headers.TryReadFirst(activityHeader))
            {
                 Common::Assert::TestAssert("FabricActivityHeader not found {0}", this);
            }
            return activityHeader.ActivityId;
        }

        virtual ~ClientServerRequestMessage() {}

    protected:

        ClientServerRequestMessage()
        {
        }

        ClientServerRequestMessage(
            std::wstring const & action,
            Transport::Actor::Enum const & actor,
            Common::ActivityId const & activityId = Common::ActivityId())
            : body_(nullptr)
        {
            SetHeaders(action, actor, activityId);
        }

        ClientServerRequestMessage(
            std::wstring const & action,
            Transport::Actor::Enum const & actor,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> && body,
            Common::ActivityId const & activityId = Common::ActivityId())
            : body_(std::move(body))
        {
            SetHeaders(action, actor, activityId);
        }

        Transport::MessageUPtr CreateTcpMessage()
        {
            Transport::MessageUPtr message;

            if (body_)
            {
                message = Common::make_unique<Transport::Message>(body_->GetTcpMessageBody());
            }
            else
            {
                message = Common::make_unique<Transport::Message>();
            }

            message->Headers.AppendFrom(headers_);

            return std::move(message);
        }

    private:

        void SetHeaders(std::wstring const & action, Transport::Actor::Enum const & actor, Common::ActivityId const & activityId)
        {
            headers_.Add(Transport::ActionHeader(action));
            headers_.Add(Transport::ActorHeader(actor));
            headers_.Add(Transport::FabricActivityHeader(activityId));
        }

        std::unique_ptr<ServiceModel::ClientServerMessageBody> body_;
        Transport::MessageHeaders headers_;
    };

    typedef std::unique_ptr<ClientServerRequestMessage> ClientServerRequestMessageUPtr;
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class ClientServerTransport
        : public Common::RootedObject
        , public Common::FabricComponent
    {
    public:
        ClientServerTransport(Common::ComponentRoot const &root)
            : Common::RootedObject(root)
        {
        }

        virtual ~ClientServerTransport() {}

        virtual Common::ErrorCode SetSecurity(Transport::SecuritySettings const &securitySettings) = 0;

        virtual Common::ErrorCode SetKeepAliveTimeout(Common::TimeSpan const &) = 0;

        virtual Transport::ISendTarget::SPtr ResolveTarget(
            std::wstring const & address,
            std::wstring const & targetId = L"",
            uint64 instance = 0) = 0;

        virtual Transport::ISendTarget::SPtr ResolveTarget(
            std::wstring const &address,
            std::wstring const &targetId,
            std::wstring const &sspiTarget) = 0;

        virtual void RegisterMessageHandler(
            Transport::Actor::Enum actor,
            Transport::Demuxer::MessageHandler const &messageHandler,
            bool dispatchOnTransportThread) = 0;
        virtual void UnregisterMessageHandler(Transport::Actor::Enum actor) = 0;

        virtual Common::AsyncOperationSPtr BeginRequestReply(
            ClientServerRequestMessageUPtr && request,
            Transport::ISendTarget::SPtr const &to,
            Transport::TransportPriority::Enum priority,
            Common::TimeSpan timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndRequestReply(
            Common::AsyncOperationSPtr const &operation,
            ClientServerReplyMessageUPtr & reply) = 0;

        virtual Common::ErrorCode SendOneWay(
            Transport::ISendTarget::SPtr const & target,
            ClientServerRequestMessageUPtr && message,
            Common::TimeSpan expiration = Common::TimeSpan::MaxValue,
            Transport::TransportPriority::Enum priority = Transport::TransportPriority::Normal) = 0;

        virtual void SetConnectionFaultHandler(Transport::IDatagramTransport::ConnectionFaultHandler const &handler) = 0;
        virtual void RemoveConnectionFaultHandler() = 0;

        virtual void SetNotificationHandler(Transport::DuplexRequestReply::NotificationHandler const &, bool) = 0;
        virtual void RemoveNotificationHandler() = 0;

        virtual void SetClaimsRetrievalHandler(Transport::TransportSecurity::ClaimsRetrievalHandler const &) = 0;
        virtual void RemoveClaimsRetrievalHandler() = 0;
    };

    typedef std::shared_ptr<ClientServerTransport> ClientServerTransportSPtr;
}

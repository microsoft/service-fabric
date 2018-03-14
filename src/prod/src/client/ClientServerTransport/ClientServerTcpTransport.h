// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class ClientServerTcpTransport 
        : public ClientServerTransport
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Client>
    {
    public:
        virtual ~ClientServerTcpTransport() {}

        explicit ClientServerTcpTransport(
            Common::ComponentRoot const &root,
            std::wstring const &traceContext,
            std::wstring const &owner,
            int maxMessageSize,
            Common::TimeSpan keepAliveInterval,
            Common::TimeSpan connectionIdleTimeout);

        explicit ClientServerTcpTransport(
            Common::ComponentRoot const &root,
            std::wstring const &traceContext,
            Transport::IDatagramTransportSPtr const &transport);

        Common::ErrorCode SetSecurity(Transport::SecuritySettings const & securitySettings);

        Common::ErrorCode SetKeepAliveTimeout(Common::TimeSpan const &);

        Transport::ISendTarget::SPtr ResolveTarget(
            std::wstring const & address,
            std::wstring const & targetId = L"",
            uint64 instance = 0);

        Transport::ISendTarget::SPtr ResolveTarget(
            std::wstring const & address,
            std::wstring const & targetId,
            std::wstring const & sspiTarget);

        void RegisterMessageHandler(
            Transport::Actor::Enum actor,
            Transport::Demuxer::MessageHandler const & messageHandler,
            bool dispatchOnTransportThread);
        void UnregisterMessageHandler(Transport::Actor::Enum actor);

        Common::AsyncOperationSPtr BeginRequestReply(
            ClientServerRequestMessageUPtr && request,
            Transport::ISendTarget::SPtr const &to,
            Transport::TransportPriority::Enum priority,
            Common::TimeSpan timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndRequestReply(
            Common::AsyncOperationSPtr const &operation,
            ClientServerReplyMessageUPtr &reply) override;

        Common::ErrorCode SendOneWay(
            Transport::ISendTarget::SPtr const & target,
            ClientServerRequestMessageUPtr && message,
            Common::TimeSpan expiration = Common::TimeSpan::MaxValue,
            Transport::TransportPriority::Enum priority = Transport::TransportPriority::Normal) override;

        void SetConnectionFaultHandler(Transport::IDatagramTransport::ConnectionFaultHandler const & handler);
        void RemoveConnectionFaultHandler();

        void SetNotificationHandler(Transport::DuplexRequestReply::NotificationHandler const &, bool);
        void RemoveNotificationHandler();

        void SetClaimsRetrievalHandler(Transport::TransportSecurity::ClaimsRetrievalHandler const &) override;
        void RemoveClaimsRetrievalHandler() override;

    protected:

        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:
        std::wstring traceContext_;
        Transport::IDatagramTransportSPtr transport_;
        Transport::DuplexRequestReplySPtr requestReply_;
        Transport::DemuxerUPtr demuxer_;
        Common::TimeSpan keepAliveInterval_;
        Common::TimeSpan connectionIdleTimeout_;
        bool externalTransport_;
    };
}

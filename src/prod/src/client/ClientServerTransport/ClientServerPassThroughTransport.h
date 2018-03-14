// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class PassThroughSendTarget;
    class PassThroughReceiverContext;

    //
    // This class exposes a client server transport using the INamingMessageProcessor interface.
    // Fabricclient calls that use this transport, directly go to this interface instead of going
    // via an actual transport(tcp or http). This is useful when we want to just use the fabricclient
    // layer that wraps the fabricclient api's user data as messages.
    //
    class ClientServerPassThroughTransport 
        : public ClientServerTransport
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Client>
    {
    public:
        virtual ~ClientServerPassThroughTransport() {}

        explicit ClientServerPassThroughTransport(
            Common::ComponentRoot const &root,
            std::wstring const &traceContext,
            std::wstring const &owner,
            Naming::INamingMessageProcessorSPtr const &namingMessageProcessorSPtr);

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
            ClientServerReplyMessageUPtr & reply) override;

        Common::ErrorCode SendOneWay(
            Transport::ISendTarget::SPtr const & target,
            ClientServerRequestMessageUPtr && message,
            Common::TimeSpan expiration = Common::TimeSpan::MaxValue,
            Transport::TransportPriority::Enum priority = Transport::TransportPriority::Normal) override;

        void SetConnectionFaultHandler(Transport::IDatagramTransport::ConnectionFaultHandler const & handler);
        void RemoveConnectionFaultHandler();

        void SetNotificationHandler(Transport::DuplexRequestReply::NotificationHandler const & handler, bool);
        void RemoveNotificationHandler();

        void SetClaimsRetrievalHandler(Transport::TransportSecurity::ClaimsRetrievalHandler const &) override { }
        void RemoveClaimsRetrievalHandler() override { }

    protected:

        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:

        friend PassThroughSendTarget;
        friend PassThroughReceiverContext;
        static Common::GlobalWString const FriendlyName;

        class RequestReplyAsyncOperation;

        //
        // Notification support for the passthrough transport.
        //
        class ProcessNotificationAsyncOperation;
        Common::AsyncOperationSPtr BeginReceiveNotification(
            Transport::MessageUPtr && message,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndReceiveNotification(
            Common::AsyncOperationSPtr const & operation,
            ClientServerReplyMessageUPtr & reply);

        void FinishNotification(
            Common::AsyncOperationSPtr const & operation, 
            Transport::MessageUPtr & reply);

        void InvokeHandler(
            Common::AsyncOperationSPtr const &operation, 
            Transport::MessageUPtr &&);

        Transport::DuplexRequestReply::NotificationHandler notificationHandler_;
        Common::RwLock notificationHandlerLock_;

        std::wstring traceContext_;
        Naming::INamingMessageProcessorSPtr namingMessageProcessorSPtr_;
        Transport::ISendTarget::SPtr sendTarget_;
    };
}

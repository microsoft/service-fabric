// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class UnreliableTransportConfig;

    class UnreliableTransport
        : public IDatagramTransport,
        public Common::TextTraceComponent<Common::TraceTaskCodes::UnreliableTransport>,
        Common::RootedObject
    {
        DENY_COPY(UnreliableTransport);
        friend class DatagramTransportFactory;

    public:
        std::wstring const & get_IdString() const override;

        UnreliableTransport(Common::ComponentRoot const & root, std::shared_ptr<IDatagramTransport> innerTransport);
        ~UnreliableTransport() override;

        Common::ErrorCode Start(bool completeStart = true) override;
        Common::ErrorCode CompleteStart() override;
        void Stop(Common::TimeSpan timeout = Common::TimeSpan::Zero) override;

        std::wstring const & TraceId() const override; 

        TransportSecuritySPtr Security() const override;

        void SetFrameHeaderErrorChecking(bool enabled) override { innerTransport_->SetFrameHeaderErrorChecking(enabled); }
        void SetMessageErrorChecking(bool enabled) override { innerTransport_->SetMessageErrorChecking(enabled); }

        void SetMessageHandler(MessageHandler const & handler) override;

        size_t SendTargetCount() const override;

        Common::ErrorCode SendOneWay(
            ISendTarget::SPtr const & target,
            MessageUPtr && message,
            Common::TimeSpan expiration = Common::TimeSpan::MaxValue,
            TransportPriority::Enum = TransportPriority::Normal) override;

        std::wstring const & ListenAddress() const override;

        void SetConnectionAcceptedHandler(ConnectionAcceptedHandler const &) override;
        void RemoveConnectionAcceptedHandler() override;

        DisconnectHHandler RegisterDisconnectEvent(DisconnectEventHandler eventHandler) override;
        bool UnregisterDisconnectEvent(DisconnectHHandler hHandler) override;

        void SetConnectionFaultHandler(ConnectionFaultHandler const & handler) override;
        void RemoveConnectionFaultHandler() override;

        Common::ErrorCode SetSecurity(SecuritySettings const &) override;
        void SetInstance(uint64 instance) override;

        void DisableSecureSessionExpiration() override;

        void DisableThrottle() override;
        void AllowThrottleReplyMessage() override;

        void DisableListenInstanceMessage() override { innerTransport_->DisableListenInstanceMessage(); }

        Common::ErrorCode SetPerTargetSendQueueLimit(ULONG limitInBytes) override;
        Common::ErrorCode SetOutgoingMessageExpiration(Common::TimeSpan expiration) override;

        void SetClaimsRetrievalMetadata(ClaimsRetrievalMetadata &&) override;
        void SetClaimsRetrievalHandler(TransportSecurity::ClaimsRetrievalHandler const &) override;
        void RemoveClaimsRetrievalHandler() override;

        void SetClaimsHandler(TransportSecurity::ClaimsHandler const & handler) override;
        void RemoveClaimsHandler() override;

        void SetMaxIncomingFrameSize(ULONG) override;
        void SetMaxOutgoingFrameSize(ULONG) override;

        Common::TimeSpan ConnectionOpenTimeout() const override;
        void SetConnectionOpenTimeout(Common::TimeSpan timeout) override;

        Common::TimeSpan ConnectionIdleTimeout() const override;
        void SetConnectionIdleTimeout(Common::TimeSpan idleTimeout) override;

        Common::TimeSpan KeepAliveTimeout() const override;
        void SetKeepAliveTimeout(Common::TimeSpan timeout) override;

        void EnableInboundActivityTracing() override;

        void SetBufferFactory(std::unique_ptr<IBufferFactory> && bufferFactory) override;

        void DisableAllPerMessageTraces() override;

        void Test_Reset() override;

#ifdef PLATFORM_UNIX
        Common::EventLoopPool* EventLoops() const override; 
        void SetEventLoopPool(Common::EventLoopPool* pool) override; 
        void SetEventLoopReadDispatch(bool asyncDispatch) override { innerTransport_->SetEventLoopReadDispatch(asyncDispatch); }
        void SetEventLoopWriteDispatch(bool asyncDispatch) override { innerTransport_->SetEventLoopWriteDispatch(asyncDispatch); }
#endif

        static void AddPartitionIdToMessageProperty(Transport::Message & message, Common::Guid const & partitionId);

    private:
        ISendTarget::SPtr Resolve(
            std::wstring const & address,
            std::wstring const & targetId,
            std::wstring const & sspiTarget,
            uint64 instance) override;

        std::shared_ptr<IDatagramTransport> innerTransport_;

        UnreliableTransportConfig & config_;
    };
}

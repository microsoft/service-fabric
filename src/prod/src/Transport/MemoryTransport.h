// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class MemoryTransport : public IDatagramTransport, public std::enable_shared_from_this<MemoryTransport>, public Common::TextTraceComponent<Common::TraceTaskCodes::Transport>
    {
        DENY_COPY(MemoryTransport);
        friend class DatagramTransportFactory;

    public:
        static size_t TransportCount();

        MemoryTransport(std::wstring const & name, std::wstring const & id);
        ~MemoryTransport() override;

        std::wstring const & get_IdString() const override;

        Common::ErrorCode Start(bool completeStart = true) override;
        Common::ErrorCode CompleteStart() override;
        void Stop(Common::TimeSpan timeout = Common::TimeSpan::Zero) override;

        std::wstring const & TraceId() const override; 

        void SetInstance(uint64 instance) override;

        TransportSecuritySPtr Security() const override;
        Common::ErrorCode SetSecurity(SecuritySettings const & securitySettings) override;

        void SetFrameHeaderErrorChecking(bool) override {}
        void SetMessageErrorChecking(bool) override {}

        void SetMessageHandler(MessageHandler const & handler) override;

        size_t SendTargetCount() const override;

        Common::ErrorCode SendOneWay(
            ISendTarget::SPtr const & target,
            MessageUPtr && message,
            Common::TimeSpan expiration = Common::TimeSpan::MaxValue,
            TransportPriority::Enum = TransportPriority::Normal) override;

        void SetConnectionAcceptedHandler(ConnectionAcceptedHandler const &) override;
        void RemoveConnectionAcceptedHandler() override;

        DisconnectHHandler RegisterDisconnectEvent(DisconnectEventHandler eventHandler) override;
        bool UnregisterDisconnectEvent(DisconnectHHandler hHandler) override;

        void SetConnectionFaultHandler(ConnectionFaultHandler const & handler) override;
        void RemoveConnectionFaultHandler() override;

        Common::ErrorCode SetPerTargetSendQueueLimit(ULONG limitInBytes) override;
        Common::ErrorCode SetOutgoingMessageExpiration(Common::TimeSpan expiration) override;

        std::wstring const & ListenAddress() const override;

        void DisableSecureSessionExpiration() override;

        void DisableThrottle() override;
        void AllowThrottleReplyMessage() override;

        void DisableListenInstanceMessage() override {}

        void SetClaimsRetrievalMetadata(ClaimsRetrievalMetadata &&) override { };
        void SetClaimsRetrievalHandler(TransportSecurity::ClaimsRetrievalHandler const &) override { };
        void RemoveClaimsRetrievalHandler() override { }

        void SetClaimsHandler(TransportSecurity::ClaimsHandler const & handler) override;
        void RemoveClaimsHandler() override;

        void SetMaxIncomingFrameSize(ULONG) override {}
        void SetMaxOutgoingFrameSize(ULONG) override {}

        Common::TimeSpan ConnectionOpenTimeout() const override;
        void SetConnectionOpenTimeout(Common::TimeSpan timeout) override;

        Common::TimeSpan ConnectionIdleTimeout() const override;
        void SetConnectionIdleTimeout(Common::TimeSpan idleTimeout) override;

        Common::TimeSpan KeepAliveTimeout() const override;
        void SetKeepAliveTimeout(Common::TimeSpan timeout) override;

        void EnableInboundActivityTracing() override;

        void SetBufferFactory(std::unique_ptr<IBufferFactory> && bufferFactory) override;

        void DisableAllPerMessageTraces() override;

#ifdef PLATFORM_UNIX
        Common::EventLoopPool* EventLoops() const override; 
        void SetEventLoopPool(Common::EventLoopPool* pool) override; 
        void SetEventLoopReadDispatch(bool) override {} 
        void SetEventLoopWriteDispatch(bool) override {}
#endif

    private:
        // Returns an empty pointer if the remote address does not already exists
        ISendTarget::SPtr Resolve(
            std::wstring const & address,
            std::wstring const & targetId,
            std::wstring const & sspiTarget,
            uint64 instance) override;

        void HandleMessage(MessageUPtr && message, ISendTarget::SPtr const & sender);

        void EnqueueMessage(std::shared_ptr<MemoryTransport> & transport, MessageUPtr && message, ISendTarget::SPtr const & sender);

        void PumpIncomingMessages();

        std::wstring const traceId_;
        bool started_;
        bool stopped_;
        std::wstring name_;
        MessageHandler messageHandler_;
        ConnectionFaultHandler faultHandler_;
        DisconnectEvent disconnectEvent_;
        ISendTarget::SPtr myTarget_;

        Common::ExclusiveLock messageTableLock_;
        std::queue<std::pair<MessageUPtr, ISendTarget::SPtr >> incomingMessages_;

        TransportSecuritySPtr security_;

        struct MemorySendTarget : public ISendTarget
        {
            DENY_COPY(MemorySendTarget);

            std::wstring name_;
            std::wstring const id_;
            std::wstring const traceId_;
            std::weak_ptr<MemoryTransport> target_;

            MemorySendTarget(std::wstring const & name, std::wstring const & id)
                : name_(name)
                , id_(id)
                , traceId_(id.empty()? Common::wformatString("{0}", TextTraceThis) : Common::wformatString("{0}-{1}", TextTraceThis, id))
            {
            }

            std::wstring const & Address() const override { return name_; }

            std::wstring const & LocalAddress() const override { Common::Assert::CodingError("Unsupported API"); }//jc

            std::wstring const & Id() const override { return id_; }
            std::wstring const & TraceId() const override { return traceId_; }

            bool IsAnonymous() const override { return name_.empty(); }

            size_t ConnectionCount() const override { return 1; }
        };
    };
}

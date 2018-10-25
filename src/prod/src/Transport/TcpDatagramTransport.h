// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    struct TcpAddressComparision : public std::binary_function<std::wstring, std::wstring, bool>
    {
        bool operator()(const std::wstring & lhs, const std::wstring &rhs) const
        {
            // _wcsicmp is chosen over much slower std::lexicographical_compare for address comparison, since
            // addresses in our system come (mostly) from the same configuration file, so internationalization
            // is mostly a non-issue, besides, we do not guarantee semantic equivalence in address comparison
            // anyway, e.g. "127.0.0.1:80" versus "localhost:80".
            return _wcsicmp(lhs.c_str(), rhs.c_str()) < 0;
        }
    };

    typedef std::map<std::wstring, TcpSendTargetWPtr, TcpAddressComparision> NamedSendTargetMap;
    typedef std::pair<std::wstring, TcpSendTargetWPtr> NamedSendTargetPair;

    typedef std::map<TcpSendTarget const*, TcpSendTargetWPtr> AnonymousSendTargetMap;
    typedef std::pair<TcpSendTarget const*, TcpSendTargetWPtr> AnonymousSendTargetPair;

    class TcpDatagramTransport
        : public IDatagramTransport, public std::enable_shared_from_this<TcpDatagramTransport>
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Transport>
    {
        DENY_COPY(TcpDatagramTransport);
        friend class TcpSendTarget;
        friend class TcpConnection;
        friend class AcceptThrottle;

    public:
        static TcpDatagramTransportSPtr Create(
            std::wstring const & address,
            std::wstring const & id = L"",
            std::wstring const & owner = L"");

        static TcpDatagramTransportSPtr CreateClient(std::wstring const & id = L"", std::wstring const & owner = L"");

        TcpDatagramTransport(std::wstring const & id, std::wstring const & owner);
        TcpDatagramTransport(std::wstring const & listenAddress, std::wstring const & id, std::wstring const & owner);
        ~TcpDatagramTransport() override;

        Common::ErrorCode Start(bool completeStart = true) override;
        Common::ErrorCode CompleteStart() override;
        void Stop(Common::TimeSpan timeout = Common::TimeSpan::Zero) override;

        std::wstring const & TraceId() const override; 

        void SetMessageHandler(MessageHandler const & handler) override;

        size_t SendTargetCount() const override;

        virtual Common::ErrorCode SendOneWay(
            ISendTarget::SPtr const & target,
            MessageUPtr && message,
            Common::TimeSpan expiration = Common::TimeSpan::MaxValue,
            TransportPriority::Enum = TransportPriority::Normal) override;

        void SetConnectionAcceptedHandler(ConnectionAcceptedHandler const &) override;
        void RemoveConnectionAcceptedHandler() override;

        DisconnectHHandler RegisterDisconnectEvent(DisconnectEventHandler eventHandler) override;
        bool UnregisterDisconnectEvent(DisconnectHHandler hHandler) override;

        void SetConnectionFaultHandler(ConnectionFaultHandler const &) override;
        void RemoveConnectionFaultHandler() override;

        std::wstring const & ListenAddress() const override;

        TransportSecuritySPtr Security() const override;
        Common::ErrorCode SetSecurity(SecuritySettings const & securitySettings);

        void SetFrameHeaderErrorChecking(bool enabled) override;
        void SetMessageErrorChecking(bool enabled) override;
        bool FrameHeaderErrorCheckingEnabled() const { return frameHeaderErrorCheckingEnabled_; }
        bool MessageErrorCheckingEnabled() const { return messageErrorCheckingEnabled_; }

        std::wstring const & get_IdString() const override;

        void DisableSecureSessionExpiration() override;

        ULONG PerTargetSendQueueLimit() const; // limit in bytes
        Common::ErrorCode SetPerTargetSendQueueLimit(ULONG limitInBytes) override;

        Common::ErrorCode SetOutgoingMessageExpiration(Common::TimeSpan expiration) override;

        void SetClaimsRetrievalMetadata(ClaimsRetrievalMetadata &&) override;
        void SetClaimsRetrievalHandler(TransportSecurity::ClaimsRetrievalHandler const & handler) override;
        void RemoveClaimsRetrievalHandler() override;

        void SetClaimsHandler(TransportSecurity::ClaimsHandler const & handler) override;
        void RemoveClaimsHandler() override;

        void SetMaxIncomingFrameSize(ULONG value) override;
        void SetMaxOutgoingFrameSize(ULONG value) override;

        Common::TimeSpan ConnectionOpenTimeout() const override;
        void SetConnectionOpenTimeout(Common::TimeSpan timeout) override;

        Common::TimeSpan ConnectionIdleTimeout() const override;
        void SetConnectionIdleTimeout(Common::TimeSpan timeout) override;

        Common::TimeSpan KeepAliveTimeout() const override; // how long TCP waits before sending keep alive packet for an idle connection
        void SetKeepAliveTimeout(Common::TimeSpan timeout) override;

        bool ShouldTraceInboundActivity() const;
        void EnableInboundActivityTracing() override;

        bool ShouldTracePerMessage() const;
        void DisableAllPerMessageTraces() override;

#ifdef PLATFORM_UNIX
        Common::EventLoopPool* EventLoops() const override; 
        void SetEventLoopPool(Common::EventLoopPool* pool) override; 
        void SetEventLoopReadDispatch(bool asyncDispatch) override { eventLoopDispatchReadAsync_ = asyncDispatch; }
        void SetEventLoopWriteDispatch(bool asyncDispatch) override { eventLoopDispatchWriteAsync_ = asyncDispatch; }
#endif

        void SetBufferFactory(std::unique_ptr<IBufferFactory> && bufferFactory) override;

        uint64 Instance() const;
        virtual void SetInstance(uint64 instance);

        std::wstring const & Owner() const;

        bool IsClientOnly() const;

        uint RecvBufferSize() const;
        void SetRecvBufferSize(uint size);

        void DisableThrottle();
        void AllowThrottleReplyMessage();

        bool ShouldSendListenInstance() const { return shouldSendListenInstance_; }
        void DisableListenInstanceMessage() override;

        void ValidateConnections();

        PerfCountersSPtr const & PerfCounters() const;

        static uint64 GetObjCount();

        // For testing only
        std::vector<Common::Endpoint> const & Test_ListenEndpoints() const { return listenEndpoints_; }

        void Test_DisableAccept();
        bool Test_AcceptSuspended() const;
        void Test_Reset() override;

        static bool Test_ReceiveMissingAssertDisabled;
        static bool Test_ReceiveMissingDetected;

    protected:
        ISendTarget::SPtr Resolve(
            std::wstring const & address,
            std::wstring const & targetId,
            std::wstring const & sspiTarget,
            uint64 instance) override;

    private:
        void Initialize();

        void SetMessageHandler_CallerHoldingLock(MessageHandler const & handler);

        Throttle* GetThrottle();
        bool AllowedToThrottleReply() const;

        IListenSocket::SPtr CreateListenSocket(Common::Endpoint const & endpoint);

        Common::ErrorCode CompleteStart_CallerHoldingLock();
        void EnsureMessageHandlerOnSendTargets_CallerHoldingLock();

        void StartTimers_CallerHoldingWLock();
        void CancelTimers();

        void StartConnectionValicationTimer_CallerHoldingWLock();
        void ConnectionValidationTimerCallback();

        void StartSendQueueCheckTimer_CallerHoldingWLock();
        void SendQueueCheckCallback();

        void OnConnectionFault(ISendTarget const & target, Common::ErrorCode const & fault);

        TcpSendTargetSPtr InnerResolveTarget(
            std::wstring const & address,
            std::wstring const & targetId,
            std::wstring const & sspiTarget,
            bool ensureSspiTarget,
            uint64 instance);

        void SubmitAccept(IListenSocket & listenSocket);
        void SubmitAcceptWithDelay(IListenSocket & listenSocket, Common::TimeSpan delay);

        TcpConnectionSPtr OnConnectionAccepted(Common::Socket & socket, Common::Endpoint const & remoteEndpoint);

        void OnListenInstanceMessage(
            Message & message,
            IConnection & connection,
            ISendTarget::SPtr const & target);

        void AcceptComplete(IListenSocket & listenSocket, Common::ErrorCode const & error);
        void OnAcceptFailure(Common::ErrorCode const & error, IListenSocket & listenSocket);

        void OnMessageReceived(
            MessageUPtr && message,
            IConnection & connection,
            ISendTarget::SPtr const & target);

        void RemoveSendTargetEntry(TcpSendTarget const & target);

        ResolveOptions::Enum HostnameResolveOption() { return this->hostnameResolveOption_; }

        Common::ErrorCode SetDynamicListenPortForHostName(std::vector<IListenSocket::SPtr> & listenSockets);

        void UpdateListenAddress(Common::Endpoint const & Endpoint);

        std::vector<TcpSendTargetSPtr> CreateSendTargetSnapshot_CallerHoldingLock();
        std::vector<TcpSendTargetSPtr> CreateSendTargetSnapshot();

        Common::ErrorCode CheckConfig();
        void EnsureSendQueueCapacity();
        void SetSendLimits(std::vector<TcpSendTargetSPtr> const & sendTargets, uint maxFrameSize, uint sendQueueLimit);
        void SetRecvLimit(std::vector<TcpSendTargetSPtr> const & sendTargets, uint maxFrameSize);

        void StartListenerStateTraceTimer_CallerHoldingWLock();
        void ListenerStateTraceCallback();

        void StartCertMonitorTimerIfNeeded_CallerHoldingWLock();
        void CertMonitorTimerCallback();
        void RefreshIssuersIfNeeded();

        void ResumeAcceptIfNeeded();

        void RemoveMessageHandlerCallerHoldingLock();

        IBufferFactory& BufferFactory() { return *bufferFactory_; }

    private:
        MUTABLE_RWLOCK(TcpDatagramTransport, lock_);

        std::wstring const id_;
        std::wstring const traceId_;
        std::wstring const owner_;
        std::wstring listenAddress_;
        uint64 instance_ = 0;
        ResolveOptions::Enum hostnameResolveOption_;
        std::vector<Common::Endpoint> listenEndpoints_;
        std::vector<IListenSocket::SPtr> listenSockets_;
        NamedSendTargetMap namedSendTargets_;
        AnonymousSendTargetMap anonymousSendTargets_;

        MessageHandlerSPtr handler_;

        ConnectionAcceptedHandler connectionAcceptedHandler_;

        DisconnectEvent disconnectEvent_;
        ConnectionFaultHandler connectionFaultHandler_;

        bool clientOnly_;
        bool stopping_ = false;
        bool started_ = false;
        bool starting_ = false;
        bool shouldTraceInboundActivity_ = false;
        bool listenOnHostname_ = false;
        bool allowThrottleReplyMessage_ = false;
        bool shouldSendListenInstance_ = true;
        bool test_acceptDisabled_ = false;
        bool shouldTracePerMessage_ = true;
        bool frameHeaderErrorCheckingEnabled_ = TransportConfig::GetConfig().FrameHeaderErrorCheckingEnabled;
        bool messageErrorCheckingEnabled_ = TransportConfig::GetConfig().MessageErrorCheckingEnabled;

#ifdef PLATFORM_UNIX
        bool eventLoopDispatchReadAsync_ = true; 
        bool eventLoopDispatchWriteAsync_ = false; 
#endif

        uint recvBufferSize_;
        uint perTargetSendQueueSizeLimitInBytes_;

        Common::TimeSpan outgoingMessageExpiration_;
        Common::TimerSPtr sendQueueCheckTimer_;

        Common::TimeSpan connectionOpenTimeout_;
        Common::TimeSpan connectionIdleTimeout_;
        Common::TimerSPtr connectionValidationTimer_;
 
        //how long TCP waits before sending keep alive packet for an idle connection
        Common::TimeSpan keepAliveTimeout_ = Common::TimeSpan::Zero;

        TransportSecuritySPtr security_;
        Common::TimerSPtr certMonitorTimer_;

        Common::TimerSPtr listenerStateTraceTimer_;

        Throttle * throttle_;
        PerfCountersSPtr perfCounters_;

        std::unique_ptr<IBufferFactory> bufferFactory_ = Common::make_unique<TcpBufferFactory>();

        static Common::atomic_uint64 objCount_;

#ifdef PLATFORM_UNIX
        Common::EventLoopPool* eventLoopPool_;
#endif
    };
}

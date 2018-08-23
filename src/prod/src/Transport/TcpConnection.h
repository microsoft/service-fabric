// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class TcpConnection
        : public IConnection
        , public std::enable_shared_from_this<TcpConnection>
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Transport>
    {
        friend class SendBuffer;
        friend class TcpSendBuffer;
        friend class ReceiveBuffer;
        friend class TcpReceiveBuffer;
        friend class LTSendBuffer;
        friend class LTReceiveBuffer;
        friend class TcpConnectOverlapped;
        friend class TcpSendOverlapped;
        friend class TcpReceiveOverlapped;
        friend class TcpSendTarget;
        friend class TcpDatagramTransport;
        friend class TcpTransportTests;
        friend class SecureTransportTests;

    public:
        static Common::ErrorCode Create(
            TcpDatagramTransportSPtr const & transport,
            TcpSendTargetSPtr const & owner,
            _Inout_ Common::Socket* acceptedSocket,
            IDatagramTransport::MessageHandlerWPtr const & msgHandlerWPtr,
            TransportSecuritySPtr const & transportSecurity,
            std::wstring const & sspiTarget,
            TransportPriority::Enum,
            TransportFlags const &,
            Common::TimeSpan openTimeout,
            _Out_ TcpConnectionSPtr & tcpConnection);

        TcpConnection(
            TcpSendTargetSPtr const & owner,
            IDatagramTransport::MessageHandler const & msgHandler,
            TransportPriority::Enum,
            TransportFlags const &,
            Common::TimeSpan openTimeout,
            _Inout_ Common::Socket* acceptedSocket);

        ~TcpConnection() override;

        std::wstring const & TraceId() const override;
        IConnectionWPtr GetWPtr() override;

        TransportPriority::Enum GetPriority() const { return priority_; }

        TransportFlags & GetTransportFlags() { return flags_; }

        void SetSecurityContext(TransportSecuritySPtr const & transportSecurity, std::wstring const & sspiTarget) override;
        void ScheduleSessionExpiration(Common::TimeSpan newExpiration, bool securitySettingsUpdated) override;
        void OnSessionExpired() override;
        void SetConnectionAuthStatus(Common::ErrorCode const & authStatus, RoleMask::Enum roleGranted) override;

        bool Open() override;

        void SetReadyCallback(ReadyCallback && readyCallback) override;
        void OnConnectionReady() override;

        void SuspendReceive(Common::ThreadpoolCallback const & action) override; // !!! can only be called in receive complete callback
        void ResumeReceive(bool incomingDataAllowed) override;

        void AddPendingSend() override { ++pendingSend_; }
        Common::ErrorCode SendOneWay(MessageUPtr && message, Common::TimeSpan expiration) override;
        Common::ErrorCode SendOneWay_Dedicated(MessageUPtr && message, Common::TimeSpan expiration) override;

        void Close() override;
        void Abort(Common::ErrorCode const & fault) override;
        void StopSendAndScheduleClose(
            Common::ErrorCode const & fault,
            bool notifyRemote,
            Common::TimeSpan delay) override;

        void ReportFault(Common::ErrorCode const & fault) override;
        Common::ErrorCode GetFault() const override;

        ISendTarget::SPtr SetTarget(ISendTarget::SPtr const & target) override;

        bool CanSend() const override;

        bool IsInbound() const override;
        bool IsLoopback() const override;

        void UpdateInstance(ListenInstance const & remoteListenInstance) override;

        Common::Guid const & ListenSideNonce() const override;

        bool IsConfirmed() const override;

        bool Validate(uint64 instanceLowerBound, Common::StopwatchTime now) override;

        void SetRecvLimit(ULONG messageSizeLimit) override;
        void SetSendLimits(ULONG messageSizeLimit, ULONG sendQueueLimit) override;
        ULONG BytesPendingForSend() const override;
        size_t MessagesPendingForSend() const override;

        size_t IncomingFrameSizeLimit() const override { return maxIncomingFrameSizeInBytes_; }
        size_t OutgoingFrameSizeLimit() const override { return maxOutgoingFrameSizeInBytes_; }
        void OnRemoteFrameSizeLimit(size_t remoteIncomingMax) override;

        void PurgeExpiredOutgoingMessages(Common::StopwatchTime now) override;

        std::wstring ToString() const override;

        void SetKeepAliveTimeout(Common::TimeSpan timeout);

        Common::Endpoint const & GroupId() const;
        void SetGroupId(Common::Endpoint const & groupId);

        Common::ErrorCode EnableKeepAliveIfNeeded(Common::Socket const & socket, Common::TimeSpan keepAliveTimeout);

        static uint64 GetObjCount();
        static uint64 OutgoingConnectionCount();

        static uint64 ConnectionFailureCount();
        static void Test_ResetConnectionFailureCount();

#ifdef DBG
        static Common::atomic_uint64 socketSendShutdownCount_;
        static Common::atomic_uint64 socketReceiveDrainedCount_;
#endif

    private:
        bool Initialize(
            TcpDatagramTransportSPtr const & transport,
            TcpConnectionSPtr const & thisSPtr,
            TransportSecuritySPtr const & transportSecurity,
            std::wstring const & sspiTarget);

        bool FinishSocketInit();

        bool CanSendMessagesAlreadyQueued() const;
        void TransitToState_CallerHoldingLock(TcpConnectionState::Enum newState);

        void Connect();
        void SubmitConnect();
        bool PrepareConnect_CallerHoldingLock(Common::Endpoint & sockName, Common::ErrorCode & error);
        void ConnectComplete(Common::ErrorCode const & error);
        void CancelOpenTimer();
        void ResolveTargetHostName();
        bool ValidateInstanceChl(uint64 instanceLowerBound) const;
        bool IsIdleTooLongChl(Common::StopwatchTime now) const;
        bool IsSendStuck(Common::StopwatchTime now) const;
        void CheckReceiveMissing_CallerHoldingLock(Common::StopwatchTime now);

        void CloseInternal(bool abort, Common::ErrorCode const & fault);
        void Close_CallerHoldingLock(bool abort, Common::ErrorCode const & fault);
        Common::ErrorCode Fault_CallerHoldingLock(Common::ErrorCode const & fault);
        void ScheduleCleanup_CallerHoldingLock();

        void SubmitSend();
        void SendComplete(Common::ErrorCode const & result, ULONG_PTR bytesTransferred);
        Common::ErrorCode Send(
            MessageUPtr && message,
            Common::TimeSpan expiration,
            bool shouldEncrypt,
            TcpConnectionState::Enum newState = TcpConnectionState::None);

        bool CanReceive() const;
        void SubmitReceive();
        void ReceiveComplete(Common::ErrorCode const & error, ULONG_PTR bytesTransferred);
        void ProcessReceivedBytes();

        void DispatchIncomingMessages(MessageUPtr && message);
        void RecordMessageDispatching(MessageUPtr const & message);

        void StartOpenTimerIfNeeded(Common::TimeSpan timeout);
        void OnConnectionOpenTimeout();

        bool IncomingFrameSizeLimitDisabled() const;
        bool OutgoingFrameSizeLimitDisabled() const;
        bool IsIncomingFrameSizeWithinLimit(size_t frameSize) const;
        bool IsOutgoingFrameSizeWithinLimit(size_t frameSize) const;
        void DisableIncomingFrameSizeLimit();
        void DisableOutgoingFrameSizeLimit();
        bool TrySetOutgoingFrameSizeLimit(size_t value, bool force = false);
        static size_t ToInternalFrameSizeLimit(uint value);

        std::wstring ToStringChl() const;

        void EnqueueListenInstanceMessage(TcpDatagramTransportSPtr const & transport);
        void EnqueueClaimsMessageIfNeeded();
        std::unique_ptr<Message> CreateClaimsMessage(std::wstring const & claimsToken);

        void OnSessionSecured(MessageUPtr && finalNegoMessage, bool started);
        void ReleaseMessagesOnSecuredConnection();
        void CompleteClaimsRetrieval(Common::ErrorCode const &, std::wstring const & claimsToken);

        // Incoming message handlers
        void OnNegotiationMessageReceived(MessageUPtr && message);
        void OnSecuredMessageReceived(MessageUPtr && message);
        void OnUnsecuredMessageReceived(MessageUPtr && message);

        void StartCloseDraining_CallerHoldingLock();
        Common::ErrorCode ShutdownSocketSend_CallerHoldingLock();

        void CreateCloseTimer();
        void StartCloseTimer_CallerHoldingLock();
        void OnCloseTimeout();

        bool CompletedAllSending_CallerHoldingLock() const;

        void AbortWithRetryableError();

    private:
        MUTABLE_RWLOCK(TcpConnection, lock_);

        TcpDatagramTransportWPtr transportWPtr_;

        TcpConnectionState::Enum state_;
        bool const inbound_;
        volatile bool receivePending_ = false;
        bool sendActive_ = false;
        bool connectActive_ = false;
        bool instanceConfirmed_ = false;
        bool shouldReportFault_ = true;
        bool shouldSuspendReceive_ = false;
        bool expectingConnectionAuthStatus_ = false;
        bool receiveDrained_ = false;
        bool incomingDataAllowed_ = true;
        bool sendShutdownDone_ = false;
        bool shouldTraceInboundActivity_ = false;
        bool allowThrottleReplyMessage_ = false;
        bool expectRemoteListenInstance_ = true;
        bool sessionExpired_ = false;
        volatile bool securitySetttingsUpdated_ = false;
        bool testAssertEnabled_;
        bool shouldTracePerMessage_ = true;
        bool outgoingFrameSizeLimitUpdatedForRemote_ = false;
#ifdef PLATFORM_UNIX
        bool eventLoopDispatchReadAsync_ = true;
        bool eventLoopDispatchWriteAsync_ = false;
#endif

        Common::ErrorCode fault_;
        SecurityContextSPtr securityContext_;

        TcpSendTargetSPtr tcpTarget_;
        ISendTarget::SPtr target_; // save this to avoid creating a new instance for every incoming message dispatch
        TransportPriority::Enum priority_;
        TransportFlags flags_;

        std::wstring const traceId_;
        Common::Guid listenSideNonce_; // for duplicate connection elimination
        uint64 instance_;
        Common::Endpoint groupId_;

        size_t maxIncomingFrameSizeInBytes_ = TcpFrameHeader::FrameSizeHardLimit();
        size_t maxOutgoingFrameSizeInBytes_ = TcpFrameHeader::FrameSizeHardLimit();
        const ULONG receiveChunkSize_;
        ULONG receiveBufferToReserve_ = 0;

        std::unique_ptr<SendBuffer> sendBuffer_;
        std::unique_ptr<ReceiveBuffer> receiveBuffer_;
        std::function<void(MessageUPtr & message/*&& not supported by compiler*/)> incomingMessageHandler_;
        IDatagramTransport::MessageHandler externalMessageHandler_;
        Throttle * throttle_ = nullptr;

        Common::Socket socket_;

        Common::atomic_long pendingSend_{0};

#ifdef PLATFORM_UNIX
        void RegisterEvtLoopIn();
        void RegisterEvtLoopOut();
        void UnregisterEvtLoopIn(bool waitForCallback);
        void UnregisterEvtLoopOut(bool waitForCallback);
        void ReadEvtCallback(int sd, uint events);
        void WriteEvtCallback(int sd, uint events);
        bool SocketErrorReported(int sd, uint events);
        void OnSocketWriteEvt();
        int get_iov_count(size_t bufferCount);

        Common::EventLoop* evtLoopIn_ = nullptr;
        Common::EventLoop* evtLoopOut_ = nullptr;
        Common::EventLoop::FdContext* fdCtxIn_ = nullptr;
        Common::EventLoop::FdContext* fdCtxOut_ = nullptr;
#else
        void CleanupThreadPoolIo();

        Common::ThreadpoolIo threadpoolIo_;
        TcpConnectOverlapped connectOverlapped_;
        TcpSendOverlapped sendOverlapped_;
        TcpReceiveOverlapped receiveOverlapped_;
#endif

        std::wstring localAddress_;
        std::wstring targetAddress_;
        Common::Endpoint remoteEndpoint_;
        ResolveOptions::Enum hostnameResolveOption_;
        std::vector<Common::Endpoint> targetEndpoints_;
        std::wstring connectToAddress_;
        ListenInstance localListenInstance_;

        Common::TimeSpan openTimeout_;
        Common::TimerSPtr openTimer_;
        Common::TimerSPtr closeTimer_;

        Common::TimeSpan idleTimeout_;
        Common::StopwatchTime lastRecvCompeteTime_ = Common::StopwatchTime::MaxValue;
        Common::StopwatchTime pendingSendStartTime_ = Common::StopwatchTime::MaxValue;
        static bool Test_IdleTimeoutHappened;

        Common::TimeSpan receiveMissingThreshold_;
        Common::ThreadpoolCallback onSuspendingReceive_;

        Common::TimeSpan keepAliveTimeout_;

        // For use with receive monitor debugging
        Common::StopwatchTime lastReceiveCompleteTime_;
        struct LastReceivedMessageInfo
        {
            MessageId Id;
            Actor::Enum Actor;
        } lastReceivedMessages_[2];
        uint lastReceivedIndex_ = 0;
        uint dispatchCountForThisReceive_;

        ReadyCallback readyCallback_;

        static Common::atomic_uint64 outgoingConnectionCount_;
        static Common::atomic_uint64 connectFailureCount_;
        static Common::atomic_uint64 connectionFailureCount_;
    };
}

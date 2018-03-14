// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class TcpSendTarget : public ISendTarget, public std::enable_shared_from_this<TcpSendTarget>
    {
        friend class TcpDatagramTransport;
        friend class TcpConnection;

    public:
        TcpSendTarget(
            TcpDatagramTransport & owner,
            IDatagramTransport::MessageHandlerSPtr const& msgHandler,
            std::wstring const & address,
            std::wstring const & targetId,
            std::wstring const & sspiTarget,
            uint64 instance,
            bool anonymous,
            TransportSecuritySPtr const & security);

        ~TcpSendTarget() override;

        Common::ErrorCode SendOneWay(
                MessageUPtr && message,
                Common::TimeSpan expiration,
                TransportPriority::Enum) override;

        std::wstring const & Address() const override;
        std::wstring const & LocalAddress() const override;
        std::wstring const & Id() const override;
        std::wstring const & TraceId() const override;
        bool IsAnonymous() const override;

        void TargetDown(uint64 instance = 0) override;
        void Reset() override; // Clean up cached TCP connection(s)

        size_t BytesPendingForSend() override;
        size_t MessagesPendingForSend() override;
        size_t ConnectionCount() const override;

        bool TryEnableDuplicateDetection() override;
        TransportFlags GetTransportFlags() const override;

        size_t MaxOutgoingMessageSize() const override { return maxOutgoingMessageSize_; }
        void SetMaxOutgoingMessageSize(size_t value);

        void Test_Block() override;
        void Test_Unblock() override;

        void Close();
        void Abort();

        TransportSecuritySPtr const & Security() const;
        std::wstring const & SspiTarget() const;
        void UpdateSspiTargetIfNeeded(std::wstring const & value);

        void StopSendAndScheduleClose(
            IConnection const & connection,
            Common::ErrorCode const & fault,
            bool notifyRemote,
            Common::TimeSpan delay);

        bool HasId() const;
        void UpdateId(std::wstring const & id);

        void UpdateSecurity(TransportSecuritySPtr const & value);

        uint64 Instance() const;
        void UpdateInstanceIfNeeded(uint64 instance);

        void UpdateConnectionInstance(
            IConnection & connection,
            ListenInstance const & remoteListenInstance);

        void ValidateConnections(Common::StopwatchTime now);

        void SetRecvLimit(ULONG messageSizeLimit);
        void SetSendLimits(ULONG messageSizeLimit, ULONG sendQueueLimit);

        TcpDatagramTransportSPtr Owner();
        TcpDatagramTransportWPtr const & OwnerWPtr() const;

        static uint64 GetObjCount();

        void Test_SuspendReceive();
        void Test_ResumeReceive();
        IConnection const* Test_ActiveConnection() const;

    private:
        void SetMessageHandler(IDatagramTransport::MessageHandlerSPtr const & msgHandler);

        void ValidateConnectionsChl(uint64 targetInstance, Common::StopwatchTime now);

        void TraceConnectionsChl();
        std::wstring ConnectionStatesToStringChl();

        TcpConnectionSPtr AddAcceptedConnection(Common::Socket & accepted);
        Common::ErrorCode AddConnection_CallerHoldingLock(
            _In_ Common::Socket * acceptedSocket, 
            TransportPriority::Enum,
            _Out_ TcpConnectionSPtr & connection);
        void AcquireConnection(
            TcpSendTarget & src,
            IConnection & connection,
            ListenInstance const & remoteListenInstance);

        IConnectionSPtr CreateConnectionChl(Common::Socket * socket = nullptr);
        Common::ErrorCode GetConnectionForSend(_Out_ IConnectionSPtr & connection, TransportPriority::Enum);
        IConnectionSPtr ChooseExistingConnectionForSendChl(TransportPriority::Enum);

        void Start();

        void CloseConnections(bool destructing);
        void CloseConnections(std::vector<IConnectionSPtr> const & connections, bool abortConnection);

        void OnMessageReceived(
            MessageUPtr && message,
            IConnection & connection,
            ISendTarget::SPtr const & target);

        void OnConnectionFault(Common::ErrorCode fault);
        void OnConnectionDisposed(IConnectionSPtr const & connection);

        void PurgeExpiredOutgoingMessages(Common::StopwatchTime now);

        IConnectionSPtr RemoveConnection(
            IConnection const & connection,
            bool saveToScheduledClose);

        IConnectionSPtr PruneScheduledCloseList(IConnectionSPtr const& connection);

        void StartConnectionRefresh(IConnection* expiringConnection);
        void ConnectionRefreshCompleted(IConnection* connection);
        void RetireExpiredConnection(IConnection & connection);

        void SendScheduleCloseMessage(IConnection & connection, Common::TimeSpan closeDelay);
        static MessageUPtr CreateScheduleCloseMessage(Common::ErrorCode fault, Common::TimeSpan closeDelay);

    private:
        static Common::atomic_uint64 objCount_;

        MUTABLE_RWLOCK(TcpSendTarget, lock_);

        volatile bool destructing_ = false;
        bool hasId_;
        bool isDown_ = false;
        bool anonymous_;
        bool test_blocked_ = false;
        bool sspiTargetToBeSet_ = false;

        TcpDatagramTransportWPtr ownerWPtr_;
        TcpDatagramTransport & owner_;
        IDatagramTransport::MessageHandlerWPtr msgHandler_;

        std::wstring const address_;
        std::wstring const localAddress_;

        std::wstring id_;
        std::wstring traceId_;
        uint64 instance_;

        volatile size_t maxOutgoingMessageSize_ = TcpFrameHeader::FrameSizeHardLimit();

        std::wstring sspiTarget_;
        TransportSecuritySPtr security_;

        Common::TimeSpan connectionIdleTimeout_;

        std::vector<IConnectionSPtr> connections_;
        std::vector<IConnectionSPtr> scheduledClose_;
        IConnectionSPtr newConnection_;
        IConnection const* expiringConnection_ = nullptr;

        TransportFlags flags_;
    };
}

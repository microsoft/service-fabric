// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

bool TcpConnection::Test_IdleTimeoutHappened = false;

atomic_uint64 TcpConnection::outgoingConnectionCount_(0);
atomic_uint64 TcpConnection::connectFailureCount_(0);
atomic_uint64 TcpConnection::connectionFailureCount_(0);

#ifdef DBG
atomic_uint64  TcpConnection::socketSendShutdownCount_(0);
atomic_uint64  TcpConnection::socketReceiveDrainedCount_(0);
#endif

#pragma region ConnectionCleanupThreadpool

namespace
{
    StringLiteral const TraceType("Connection");

    Global<Threadpool> threadpoolForCleanup;
    INIT_ONCE cleanupThreadpoolInitOnce = INIT_ONCE_STATIC_INIT;

    BOOL CALLBACK CleanupThreadpoolInitFunction(PINIT_ONCE, PVOID, PVOID *)
    {
        threadpoolForCleanup = Threadpool::CreateCustomPool(0, TransportConfig::GetConfig().ConnectionCleanupThreadMax);
        return TRUE;
    }

    Threadpool & GetCleanupThreadpool()
    {
        PVOID lpContext = NULL;
        BOOL  bStatus = FALSE;

        bStatus = ::InitOnceExecuteOnce(
            &cleanupThreadpoolInitOnce,
            CleanupThreadpoolInitFunction,
            NULL,
            &lpContext);

        ASSERT_IF(!bStatus, "Failed to initialize threadpoolForCleanup singleton");
        return *threadpoolForCleanup;
    }
}

#pragma endregion ConnectionCleanupThreadpool

_Use_decl_annotations_ ErrorCode TcpConnection::Create(
    TcpDatagramTransportSPtr const & transport,
    TcpSendTargetSPtr const & owner,
    Socket* acceptedSocket,
    IDatagramTransport::MessageHandlerWPtr const & msgHandlerWPtr,
    TransportSecuritySPtr const & transportSecurity,
    wstring const & sspiTarget,
    TransportPriority::Enum priority,
    TransportFlags const & flags,
    Common::TimeSpan openTimeout,
    TcpConnectionSPtr & tcpConnection)
{
    auto msgHandlerSPtr = msgHandlerWPtr.lock();
    if (!msgHandlerSPtr)
    {
        return ErrorCodeValue::ObjectClosed;
    }

    Invariant(*msgHandlerSPtr);
    tcpConnection = make_shared<TcpConnection>(owner, *msgHandlerSPtr, priority, flags, openTimeout, acceptedSocket);
    if (!tcpConnection->Initialize(transport, tcpConnection, transportSecurity, sspiTarget))
    {
        tcpConnection->AbortWithRetryableError();
        return ErrorCodeValue::OperationCanceled;
    }

    ErrorCode error;
    if (tcpConnection->state_ == TcpConnectionState::Connected)
    {
#ifdef PLATFORM_UNIX
        tcpConnection->eventLoopDispatchReadAsync_ = transport->eventLoopDispatchReadAsync_;
        tcpConnection->eventLoopDispatchWriteAsync_ = transport->eventLoopDispatchWriteAsync_;
        tcpConnection->RegisterEvtLoopIn();
        tcpConnection->RegisterEvtLoopOut();
#else
        error = tcpConnection->threadpoolIo_.Open((HANDLE)tcpConnection->socket_.GetHandle());
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, tcpConnection->traceId_,
                "{0}-{1} failed to create threadpool io: {2}",
                tcpConnection->localAddress_, tcpConnection->targetAddress_, error);

            tcpConnection->AbortWithRetryableError();
            return error;
        }
#endif
    }

    return error;
}

_Use_decl_annotations_ TcpConnection::TcpConnection(
    TcpSendTargetSPtr const & target, 
    IDatagramTransport::MessageHandler const & msgHandler,
    TransportPriority::Enum priority,
    TransportFlags const & flags,
    TimeSpan openTimeout,
    Socket* acceptedSocket)
    : tcpTarget_(target)
    , target_(target)
    , externalMessageHandler_(msgHandler)
    , transportWPtr_(target->OwnerWPtr())
    , priority_(priority)
    , flags_(flags)
    , openTimeout_(openTimeout)
    , traceId_(wformatString("{0:x}", TextTracePtrAs(this, IConnection)))
    , instance_(target->Instance())
    , inbound_(acceptedSocket != nullptr)
    , socket_((acceptedSocket == nullptr) ? Socket() : move(*acceptedSocket))
    , state_(TcpConnectionState::Created)
    , listenSideNonce_((acceptedSocket == nullptr) ? Guid() : Guid::NewGuid())
    , localAddress_(target->LocalAddress().empty() ? L"client" : target->LocalAddress())
    , targetAddress_(target->Address())
#ifndef PLATFORM_UNIX
    , connectOverlapped_(*this)
    , sendOverlapped_(*this)
    , receiveOverlapped_(*this)
#endif
    , receiveChunkSize_(
        SecurityContextSsl::Supports(target->Security()->SecurityProvider) ?
        TransportConfig::GetConfig().SslReceiveChunkSize :
        TransportConfig::GetConfig().DefaultReceiveChunkSize)
    , receiveMissingThreshold_(TransportConfig::GetConfig().ReceiveMissingThreshold)
    , testAssertEnabled_(target->Security()->SecurityProvider == SecurityProvider::None)
    , maxIncomingFrameSizeInBytes_(ToInternalFrameSizeLimit(target->Security()->MaxIncomingFrameSize()))
    , lastReceiveCompleteTime_(Message::NullReceiveTime())
{
    TrySetOutgoingFrameSizeLimit(ToInternalFrameSizeLimit(target->Security()->MaxOutgoingFrameSize()), true);

    if(!inbound_)
    {
        ++outgoingConnectionCount_;
    }

    if (acceptedSocket)
    {
        auto err = TcpTransportUtility::TryParseEndpointString(targetAddress_, remoteEndpoint_);
        Invariant2(err.IsSuccess(), "TryParseEndpointString({0}) failed: {1}", targetAddress_, err);
    }

    Invariant(!targetAddress_.empty());
    Invariant(externalMessageHandler_);

    trace.ConnectionCreated(
        traceId_,
        localAddress_,
        targetAddress_,
        target_->TraceId(),
        inbound_,
        receiveChunkSize_,
        priority_,
        inbound_ ?
            AcceptThrottle::GetThrottle()->CheckedIncomingConnectionCount_CallerHoldingLock() :
            outgoingConnectionCount_.load());
}

TcpConnection::~TcpConnection()
{
   if (inbound_)
    {
        auto incomingRemained = AcceptThrottle::GetThrottle()->OnConnectionDestructed(this, groupId_);
        trace.ConnectionDestroyed(traceId_, true, incomingRemained);
        return;
    }

    trace.ConnectionDestroyed(traceId_, false, --outgoingConnectionCount_);
}

bool TcpConnection::Initialize(
    TcpDatagramTransportSPtr const & transport,
    TcpConnectionSPtr const &,
    TransportSecuritySPtr const & transportSecurity,
    std::wstring const & sspiTarget)
{
#ifdef PLATFORM_UNIX
    transport->EventLoops()->AssignPair(&evtLoopIn_, &evtLoopOut_);
#endif

    if (!externalMessageHandler_)
    {
        WriteInfo(TraceType, traceId_, "Initialize: empty message handler, transport already stopped");
        return false;
    }

    if (transportSecurity->Settings().IsRemotePeer() && TransportConfig::GetConfig().MessageSizeCheckDisabledOnPeers)
    {
        DisableIncomingFrameSizeLimit();
        DisableOutgoingFrameSizeLimit();
    }

    localListenInstance_ = ListenInstance(
        transport->ListenAddress(),
        transport->Instance(),
        ListenSideNonce());

    shouldTraceInboundActivity_ = transport->ShouldTraceInboundActivity();
    allowThrottleReplyMessage_ = transport->AllowedToThrottleReply();
    throttle_ = transport->GetThrottle();

    sendBuffer_ = transport->BufferFactory().CreateSendBuffer(this);
    sendBuffer_->SetLimit(transport->PerTargetSendQueueLimit());
    sendBuffer_->SetPerfCounters(transport->PerfCounters());
    sendBuffer_->SetFrameHeaderErrorChecking(transport->FrameHeaderErrorCheckingEnabled());
    sendBuffer_->SetMessageErrorChecking(transport->MessageErrorCheckingEnabled());
    WriteInfo(
        TraceType, traceId_,
        "FrameHeaderErrorCheckingEnabled={0}, MessageErrorCheckingEnabled={1}",
        transport->FrameHeaderErrorCheckingEnabled(), transport->MessageErrorCheckingEnabled());

    receiveBuffer_ = transport->BufferFactory().CreateReceiveBuffer(this);
    receiveBufferToReserve_ = (receiveChunkSize_ > transport->RecvBufferSize()) ? receiveChunkSize_ : transport->RecvBufferSize();

    hostnameResolveOption_ = transport->HostnameResolveOption();
    idleTimeout_ = transport->ConnectionIdleTimeout();
    keepAliveTimeout_ = transport->KeepAliveTimeout();

    shouldTracePerMessage_ = transport->ShouldTracePerMessage();

    SetSecurityContext(transportSecurity, sspiTarget);
    CreateCloseTimer();
    EnqueueClaimsMessageIfNeeded();
    EnqueueListenInstanceMessage(transport);

    if (!inbound_ || expectRemoteListenInstance_ || securityContext_)
    {
        StartOpenTimerIfNeeded(openTimeout_);
    }

    if (inbound_ )
    {
        TransitToState_CallerHoldingLock(TcpConnectionState::Connected);

        if (!FinishSocketInit())
        {
            return false;
        }
    }

    return true;
}

bool TcpConnection::FinishSocketInit()
{
    auto tcpNoDelayEnabled = TransportConfig::GetConfig().TcpNoDelayEnabled;
    WriteInfo(
        TraceType, traceId_,
        "FinishSocketInit: local message size limits:(incoming={0}/0x{0:x}, outgoing={1}/0x{1:x}), TcpNoDelayEnabled = {2}",
        maxIncomingFrameSizeInBytes_, maxOutgoingFrameSizeInBytes_, tcpNoDelayEnabled);

    if (tcpNoDelayEnabled)
    {
        auto error = socket_.SetSocketOption(IPPROTO_TCP, TCP_NODELAY, 1);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType, traceId_,
                "{0}-{1} failed to enable TCP_NODELAY: {2}",
                localAddress_, targetAddress_, error);
        }
    }

    int recvBufSize = 0;
    auto error = socket_.GetSocketOption(SOL_SOCKET, SO_RCVBUF, recvBufSize);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, traceId_, "failed to get SO_RCVBUF: {0}", error);
    }

    int sendBufSize = 0;
    error = socket_.GetSocketOption(SOL_SOCKET, SO_SNDBUF, sendBufSize);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, traceId_, "failed to get SO_SNDBUF: {0}", error);
    }

    WriteInfo(TraceType, traceId_, "socket default: SO_RCVBUF={0}, SO_SNDBUF={1}", recvBufSize, sendBufSize);

    int sizeToSet = (int)(receiveBufferToReserve_);
    if (recvBufSize < sizeToSet)
    {
        error = socket_.SetSocketOption(SOL_SOCKET, SO_RCVBUF, sizeToSet);
        if (error.IsSuccess())
        {
            WriteInfo(TraceType, traceId_, "SO_RCVBUF set to {0}", sizeToSet);
        }
        else
        {
            WriteWarning(TraceType, traceId_, "failed to set SO_RCVBUF to {0}: {1}", sizeToSet, error);
        }
    }

    if (sendBufSize < sizeToSet)
    {
        error = socket_.SetSocketOption(SOL_SOCKET, SO_SNDBUF, sizeToSet);
        if (error.IsSuccess())
        {
            WriteInfo(TraceType, traceId_, "SO_SNDBUF set to {0}", sizeToSet);
        }
        else
        {
            WriteWarning(TraceType, traceId_, "failed to set SO_SNDBUF to {0}: {1}", sizeToSet, error);
        }
    }

 #ifndef PLATFORM_UNIX
    // Allow TCP window scaling as long as window size does not fall below the value set via SO_RCVBUF
    DWORD outputSize = 0;
    WSA_COMPATIBILITY_MODE cm = { WsaBehaviorReceiveBuffering, NTDDI_WIN8 };
    int retval = WSAIoctl(
        socket_.GetHandle(),
        SIO_SET_COMPATIBILITY_MODE,
        &cm,
        sizeof(cm),
        nullptr,
        0,
        &outputSize,
        nullptr,
        nullptr);

    error = ErrorCode::FromWin32Error(::WSAGetLastError());
    WriteTrace(
        (retval == 0) ? LogLevel::Info : LogLevel::Error,
        TraceType,
        traceId_,
        "WSAIoctl(BehaviorId=SIO_SET_COMPATIBILITY_MODE, TargetOsVersion={0:x}) returned {1}",
        cm.TargetOsVersion, error);

    if (!inbound_) //incoming socket inherits from listen socket already
    {
        TcpTransportUtility::EnableTcpFastLoopbackIfNeeded(socket_, traceId_);
    }

    error = socket_.SetSocketOption(SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType,
            traceId_,
            "{0}-{1} failed to set SO_UPDATE_CONNECT_CONTEXT: {2}, shutdown(SD_SEND) will fail",
            localAddress_,
            targetAddress_,
            error);
    }
#endif

    // Enable keep alive so that non-responsive remote side can be detected
    TcpConnection::EnableKeepAliveIfNeeded(socket_, keepAliveTimeout_);

    return true;
}

uint64 TcpConnection::GetObjCount()
{
    return outgoingConnectionCount_.load() + AcceptThrottle::GetThrottle()->IncomingConnectionCount();
}

uint64 TcpConnection::OutgoingConnectionCount()
{
    return outgoingConnectionCount_.load();
}

uint64 TcpConnection::ConnectionFailureCount()
{
    return connectionFailureCount_.load();
}

void TcpConnection::Test_ResetConnectionFailureCount()
{
    connectionFailureCount_.store(0);
}

bool TcpConnection::TrySetOutgoingFrameSizeLimit(size_t value, bool force)
{
    if (!force && outgoingFrameSizeLimitUpdatedForRemote_)
    {
        WriteInfo(
            TraceType, traceId_,
            "TrySetOutgoingFrameSizeLimit: ignore new value {0}/0x{0:x} per outgoingFrameSizeLimitUpdatedForRemote_",
            value);

        return false;
    }

    maxOutgoingFrameSizeInBytes_ = value;
    tcpTarget_->SetMaxOutgoingMessageSize(maxOutgoingFrameSizeInBytes_);
    return true;
}

void TcpConnection::OnRemoteFrameSizeLimit(size_t remoteIncomingMax)
{
    WriteInfo(
        TraceType,
        traceId_,
        "OnRemoteFrameSizeLimit: message size limits: remoteIncoming={0}/0x{0:x}, localOutgoing={1}/0x{1:x}",
        remoteIncomingMax,
        maxOutgoingFrameSizeInBytes_);

    if (!OutgoingFrameSizeLimitDisabled())
    {
        if (remoteIncomingMax &&( maxOutgoingFrameSizeInBytes_ != remoteIncomingMax))
        {
            WriteInfo(
                TraceType, traceId_,
                "updating outgoing message size limit according to remote incoming: {0}/0x{0:x} -> {1}/0x{1:x}",
                maxOutgoingFrameSizeInBytes_, remoteIncomingMax);

            TrySetOutgoingFrameSizeLimit(remoteIncomingMax);
            outgoingFrameSizeLimitUpdatedForRemote_ = true;
        }
    }
}

void TcpConnection::DisableOutgoingFrameSizeLimit()
{
    //TcpFrameHeader::FrameSizeHardLimit() means limit is disabled,
    //limit cannot go down from TcpFrameHeader::FrameSizeHardLimit()
    if (TrySetOutgoingFrameSizeLimit(TcpFrameHeader::FrameSizeHardLimit()), true)
    {
        WriteInfo(TraceType, traceId_, "outgoing message size limit disabled");
    }
}

void TcpConnection::DisableIncomingFrameSizeLimit()
{
    //TcpFrameHeader::FrameSizeHardLimit() means limit is disabled,
    //limit cannot go down from TcpFrameHeader::FrameSizeHardLimit()
    maxIncomingFrameSizeInBytes_ = TcpFrameHeader::FrameSizeHardLimit();
    WriteInfo(TraceType, traceId_, "incoming message size limit disabled");
}

bool TcpConnection::IncomingFrameSizeLimitDisabled() const
{
    //TcpFrameHeader::FrameSizeHardLimit() means limit is disabled,
    return maxIncomingFrameSizeInBytes_ == TcpFrameHeader::FrameSizeHardLimit();
}

bool TcpConnection::OutgoingFrameSizeLimitDisabled() const
{
    //TcpFrameHeader::FrameSizeHardLimit() means limit is disabled,
    return maxOutgoingFrameSizeInBytes_ == TcpFrameHeader::FrameSizeHardLimit();
}

size_t TcpConnection::ToInternalFrameSizeLimit(uint value)
{
    return value ? value :
        // use FrameSizeHardLimit - 1 instead of TcpFrameHeader::FrameSizeHardLimit() as default
        // value, because TcpFrameHeader::FrameSizeHardLimit() means limit is disabled
        (TcpFrameHeader::FrameSizeHardLimit() - 1);
}

void TcpConnection::SetRecvLimit(ULONG messageSizeLimit)
{
    if (IncomingFrameSizeLimitDisabled()) return;

    maxIncomingFrameSizeInBytes_ = ToInternalFrameSizeLimit(messageSizeLimit);
}

void TcpConnection::SetSendLimits(ULONG messageSizeLimit, ULONG sendQueueLimit)
{
    sendBuffer_->SetLimit(sendQueueLimit);

    if (OutgoingFrameSizeLimitDisabled()) return;

    TrySetOutgoingFrameSizeLimit(ToInternalFrameSizeLimit(messageSizeLimit));
}

void TcpConnection::PurgeExpiredOutgoingMessages(StopwatchTime now)
{
    AcquireWriteLock grab(lock_);
    sendBuffer_->PurgeExpiredMessages(now);
}

ISendTarget::SPtr TcpConnection::SetTarget(ISendTarget::SPtr const & target)
{
    // Need to lock for target_ read in ScheduleClose, which may be called from threads
    // not associated with socket threadpool IO completion, e.g. from some timer callback
    AcquireWriteLock grab(lock_);

    auto oldValue = std::move(target_);
    this->tcpTarget_ = static_pointer_cast<TcpSendTarget>(target);
    this->target_ = target;
    this->targetAddress_ = target->Address();
    trace.ConnectionSetTarget(traceId_, target_->TraceId(), localAddress_, targetAddress_);

    return oldValue;
}

ErrorCode TcpConnection::SendOneWay_Dedicated(MessageUPtr && message, TimeSpan expiration)
{
    KAssert(message != nullptr);
    ++pendingSend_;
    return SendOneWay(move(message), expiration);
}

ErrorCode TcpConnection::SendOneWay(MessageUPtr && message, TimeSpan expiration)
{
    KAssert(message != nullptr);
    KFinally([this]
    {
        auto pending = --pendingSend_;
        ASSERT_IF(pending < 0, "pending send = {0}", pending);
    });

    if (!message->IsValid)
    {
        trace.DroppingInvalidMessage(TracePtr(message.get()), message->Status);
        message->OnSendStatus(ErrorCodeValue::InvalidMessage, move(message));
        return ErrorCodeValue::InvalidMessage;
    }

    message->Headers.CompactIfNeeded();

    size_t outgoingFrameSize = sizeof(TcpFrameHeader) + message->SerializedHeaderSize() + message->SerializedBodySize();
    if (!IsOutgoingFrameSizeWithinLimit(outgoingFrameSize))
    {
        if (securityContext_ && IsConfirmed()) //outgoing message size limit may change during security negotiation, e.g. admin client has no outgoing message size limit
        {
            trace.OutgoingMessageTooLarge(
                traceId_,
                this->localAddress_,
                this->targetAddress_,
                outgoingFrameSize,
                maxOutgoingFrameSizeInBytes_,
                message->TraceId(),
                message->Actor,
                message->Action);

            TESTASSERT_IF(testAssertEnabled_);

            if (message->IsReply())
            {
                //Send fault back in place of reply
                auto faultMsg = make_unique<Message>();
                faultMsg->Headers.Add(RelatesToHeader(message->RelatesTo));
                faultMsg->Headers.Add(FaultHeader(ErrorCodeValue::MessageTooLarge));
                Send(move(faultMsg), expiration, securityContext_ != nullptr);
            }

            message->OnSendStatus(ErrorCodeValue::MessageTooLarge, move(message));
            return ErrorCodeValue::MessageTooLarge;
        }
    }

    auto status = message->Headers.FinalizeIdempotentHeader();
    if (status != STATUS_SUCCESS)
    {
        return ErrorCode::FromNtStatus(status);
    }

    return Send(move(message), expiration, securityContext_ != nullptr);
}

bool TcpConnection::Open()
{
    MessageUPtr negoMessageToSend = nullptr;
    if (securityContext_ && !inbound_)
    {
        SECURITY_STATUS status = securityContext_->NegotiateSecurityContext(nullptr, negoMessageToSend);
        if (securityContext_->NegotiationSucceeded())
        {
            OnSessionSecured(move(negoMessageToSend), false);
        }
        else
        {
            if (FAILED(status))
            {
                WriteError(
                    TraceType, traceId_,
                    "failed to initialize security context: 0x{0:x}",
                    (uint)status);

                AbortWithRetryableError();
                return false;
            }

            if (status != SEC_I_CONTINUE_NEEDED)
            {
                AbortWithRetryableError();
                return false;
            }
        }
    }

    if (!CanSend())
    {
        WriteWarning(
            TraceType, traceId_,
            "{0}-{1} failed to start, state = {2}", 
            localAddress_, targetAddress_, state_);

        // No need to report fault here as it must have been reported already
        AbortWithRetryableError();
        return false;
    }

    // Start send pump
    auto error = Send(move(negoMessageToSend), TimeSpan::MaxValue, false);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType, traceId_,
            "{0}-{1}: Transmit failed in Start: {2}",
            localAddress_, targetAddress_, error);

        AbortWithRetryableError();
        return false;
    }

    if (securityContext_ && securityContext_->ConnectionAuthorizationFailed())
    {
        ReportFault(securityContext_->FaultToReport());
        Close();
        return false;
    }

    if (inbound_)
    {
        SubmitReceive();
    }

    return true;
}

void TcpConnection::Close()
{
    CloseInternal(false, ErrorCode());
}

void TcpConnection::Abort(ErrorCode const & fault)
{
    CloseInternal(true, fault);
}

ErrorCode TcpConnection::ShutdownSocketSend_CallerHoldingLock()
{
    ErrorCode error;
    if (sendShutdownDone_) return error;

    if (state_ != TcpConnectionState::CloseDraining)
    {
        // not called from SendComplete
        TransitToState_CallerHoldingLock(TcpConnectionState::CloseDraining);
    }

    error = socket_.Shutdown(SocketShutdown::Send);
    sendShutdownDone_ = true;
    WriteInfo(
        TraceType, traceId_,
        "{0}-{1} shut down socket send to notify remote side of closing: {2}",
        localAddress_,
        targetAddress_,
        error);

    if (error.IsSuccess())
    {
        StartCloseTimer_CallerHoldingLock();

#ifdef DBG
        ++socketSendShutdownCount_;
#endif
    }

    return error;
}

bool TcpConnection::CompletedAllSending_CallerHoldingLock() const
{
    WriteNoise(
        TraceType, traceId_,
        "send status: sendBuffer_->Empty() = {0}, pending send= {1}",
        sendBuffer_->Empty(), pendingSend_.load());

    return sendBuffer_->Empty() && (pendingSend_.load() == 0);
}

void TcpConnection::AbortWithRetryableError()
{
    //report retryable error to allow retry at upper layer
    Abort(ErrorCodeValue::OperationCanceled);
}

_Use_decl_annotations_
void TcpConnection::Close_CallerHoldingLock(bool abort, ErrorCode const & fault)
{
    Fault_CallerHoldingLock(fault);

    if (state_ == TcpConnectionState::Closed)
    {
        return;
    }

    trace.ConnectionClose(traceId_, localAddress_, targetAddress_, abort, state_, sendBuffer_->Empty(), receiveDrained_);

    if (abort || state_ == TcpConnectionState::Created || state_ == TcpConnectionState::Connecting)
    {
        TransitToState_CallerHoldingLock(TcpConnectionState::Closed);
        ScheduleCleanup_CallerHoldingLock();
        return;
    }

    if ((state_ == TcpConnectionState::CloseDraining) && !(sendBuffer_->Empty() && receiveDrained_))
    {
        return;
    }

    if (state_ == TcpConnectionState::Connected)
    {
        if (!CompletedAllSending_CallerHoldingLock())
        {
            StartCloseDraining_CallerHoldingLock();
            return;
        }

        auto error = ShutdownSocketSend_CallerHoldingLock();
        if (error.IsSuccess() && !receiveDrained_) return;
    }

    TransitToState_CallerHoldingLock(TcpConnectionState::Closed);
    ScheduleCleanup_CallerHoldingLock();
}

void TcpConnection::StartCloseDraining_CallerHoldingLock()
{
    TransitToState_CallerHoldingLock(TcpConnectionState::CloseDraining);
    WriteInfo(
        TraceType, traceId_,
        "{0}-{1} start close draining, timeout = {2}",
        localAddress_,
        targetAddress_,
        TransportConfig::GetConfig().CloseDrainTimeout);

    StartCloseTimer_CallerHoldingLock();
}

void TcpConnection::CreateCloseTimer()
{
    auto thisSPtr = shared_from_this();
    closeTimer_ = Timer::Create(
        "ConnectionClose",
        [thisSPtr](TimerSPtr const&) { thisSPtr->OnCloseTimeout(); },
        true,
        Throttle::GetThrottle()->MonitorCallbackEnv());
}

void TcpConnection::StartCloseTimer_CallerHoldingLock()
{
    closeTimer_->Change(TransportConfig::GetConfig().CloseDrainTimeout);
}

void TcpConnection::OnCloseTimeout()
{
    WriteInfo(
        TraceType, traceId_,
        "{0}-{1} close timed out",
        localAddress_,
        targetAddress_);

    Abort(ErrorCodeValue::OperationCanceled);
}

void TcpConnection::SetReadyCallback(ReadyCallback && readyCallback)
{
    Invariant(!readyCallback_);
    readyCallback_ = move(readyCallback);
}

void TcpConnection::CheckReceiveMissing_CallerHoldingLock(StopwatchTime now)
{
    if (receivePending_ || shouldSuspendReceive_) return;

    if (receiveMissingThreshold_ <= TimeSpan::Zero) return;

    auto gap = now - lastReceiveCompleteTime_;
    if (gap <= receiveMissingThreshold_) return;

    if (dispatchCountForThisReceive_ > 1)
    {
        auto gapAveraged = TimeSpan::FromMilliseconds(gap.TotalMillisecondsAsDouble() / dispatchCountForThisReceive_);
        if (gapAveraged <= receiveMissingThreshold_)
        {
            WriteWarning(
                TraceType, traceId_,
                "{0}-{1} slow message dispatching detected",
                localAddress_,
                targetAddress_);
            return;
        }
    }

    if ((state_ != TcpConnectionState::Connected) && (state_ != TcpConnectionState::CloseDraining))
    {
        WriteWarning(TraceType, traceId_, "ignore receive missing at state {0}", state_);
        return;
    }

    wstring lastDispatchedMessages = wformatString(
        "({0}, Actor = {1}) ({2}, Actor = {3})",
        lastReceivedMessages_[0].Id,
        lastReceivedMessages_[0].Actor,
        lastReceivedMessages_[1].Id,
        lastReceivedMessages_[1].Actor);

    trace.ReceiveMissingDetected(
        traceId_,
        localAddress_,
        targetAddress_,
        receiveMissingThreshold_,
        GetCurrentProcessId(),
        lastDispatchedMessages);

    if (TcpDatagramTransport::Test_ReceiveMissingAssertDisabled)
    {
        TcpDatagramTransport::Test_ReceiveMissingDetected = true;
        Close_CallerHoldingLock(true, ErrorCodeValue::OperationFailed);
        return;
    }

    ASSERT_IF(
        Random().NextDouble() < TransportConfig::GetConfig().FailFastProbabilityOnReceiveMissingDetected,
        "{0}: " PENDING_RECEIVE_MISSING_DIAGNOSTIC_MESSAGE,
        traceId_,
        localAddress_,
        targetAddress_,
        receiveMissingThreshold_,
        GetCurrentProcessId(),
        lastDispatchedMessages);
}

bool TcpConnection::IsSendStuck(StopwatchTime now) const
{
    auto sendCompleteDue = pendingSendStartTime_ + TransportConfig::GetConfig().SendTimeout;
    return
        (StopwatchTime::Zero < sendCompleteDue) &&
        (sendCompleteDue < now);
}

bool TcpConnection::IsIdleTooLongChl(StopwatchTime now) const
{
    if (idleTimeout_ == TimeSpan::Zero)
    {
        return false; // idle timeout disabled
    }

    if (localAddress_ == targetAddress_)
    {
        // loopback connection to this transport, SendTarget has both incoming and outgoing
        // connections, they are two sides of the same socket, close one of them will close both.
        return false;
    }

    // Use substraction here as lastRecvCompeteTime_/pendingSendStartTime_ can be StopwatchTime::MaxValue,
    // in which case, adding idleTimeout_ will overflow.
    auto lastActionTimeLowerBound = now - idleTimeout_;
    if (lastActionTimeLowerBound < lastRecvCompeteTime_)
    {
        return false;
    }

    if (lastActionTimeLowerBound < pendingSendStartTime_)
    {
        return false;
    }

    Test_IdleTimeoutHappened = true;

    return true;
}

void TcpConnection::UpdateInstance(ListenInstance const & remoteListenInstance)
{
    AcquireWriteLock lockInScope(lock_);

    uint64 currentInstance = instance_;
    instance_ = remoteListenInstance.Instance();

    TESTASSERT_IF(instanceConfirmed_, "connection instance is confirmed already");
    if (instanceConfirmed_) return;

    instanceConfirmed_ = true;

    if (!inbound_)
    {
        listenSideNonce_ = remoteListenInstance.Nonce();
    }

    trace.ConnectionInstanceConfirmed(traceId_, currentInstance, remoteListenInstance);
}

wstring TcpConnection::ToStringChl() const
{
    wstring result;
    StringWriter(result).Write(
        "{0}: ({1}-{2}, Passive={3}, Instance={4}, Confirmed={5}, Nonce={6})",
        traceId_, 
        localAddress_,
        targetAddress_,
        inbound_,
        instance_,
        instanceConfirmed_,
        listenSideNonce_);

    return result;
}

wstring TcpConnection::ToString() const
{
    AcquireReadLock lockInScope(lock_);
    return ToStringChl();
}

Guid const & TcpConnection::ListenSideNonce() const
{
    // No need to take lock since this is always called under TcpSendTarget lock
    return listenSideNonce_;
}

bool TcpConnection::IsConfirmed() const
{
    // No need to take lock since this is always called under TcpSendTarget lock
    return instanceConfirmed_;
}

bool TcpConnection::ValidateInstanceChl(uint64 instanceLowerBound) const
{
    // When the current instance value is unconfirmed 0, we keep the connection. Because unconfirmed 0
    // means that we never knew the remote instance before, thus the connection is to an unknown instance,
    // unlikely to an obsolete instance. This is useful to avoid dropping initial connections to other
    // federation nodes, important for join, bootstrap ...
    return (instance_ >= instanceLowerBound) || ((instance_ == 0) && !instanceConfirmed_);
}

bool TcpConnection::Validate(uint64 instanceLowerBound, Common::StopwatchTime now)
{
    bool validated = true;
    {
        AcquireWriteLock grab(lock_);

        CheckReceiveMissing_CallerHoldingLock(now);

        if (!ValidateInstanceChl(instanceLowerBound))
        {
            trace.ConnectionInstanceObsolete(ToStringChl());
            validated = false;
            Close_CallerHoldingLock(false, ErrorCodeValue::ConnectionInstanceObsolete);
        }
        else if (IsSendStuck(now))
        {
            WriteWarning(
                TraceType, traceId_,
                "{0}-{1} send timed out: timeout = {2}",
                localAddress_, targetAddress_, TransportConfig::GetConfig().SendTimeout);

            validated = false;
            Close_CallerHoldingLock(true, ErrorCodeValue::OperationCanceled);
        }
        else if (IsIdleTooLongChl(now))
        {
            trace.ConnectionIdleTimeout(ToStringChl());
            validated = false;
            Close_CallerHoldingLock(false, ErrorCodeValue::ConnectionIdleTimeout);
        }
    }

    return validated;
}

void TcpConnection::CloseInternal(bool abort, ErrorCode const & fault)
{
    AcquireWriteLock grab(lock_);
    Close_CallerHoldingLock(abort, fault);
}

wstring const & TcpConnection::TraceId() const
{
    return traceId_;
}

IConnectionWPtr TcpConnection::GetWPtr()
{
    return shared_from_this();
}

void TcpConnection::ScheduleCleanup_CallerHoldingLock()
{
    trace.ConnectionCleanupScheduled(
        traceId_,
        localAddress_,
        targetAddress_);

#ifndef PLATFORM_UNIX
    ::CancelIoEx((HANDLE)socket_.GetHandle(), nullptr);
#endif

    auto tcpConnectionSPtr = this->shared_from_this();
    GetCleanupThreadpool().Submit(
        [tcpConnectionSPtr]()
        {
            tcpConnectionSPtr->CancelOpenTimer();

            if (tcpConnectionSPtr->closeTimer_)
            {
                tcpConnectionSPtr->closeTimer_->Cancel();
            }

#ifdef PLATFORM_UNIX
            tcpConnectionSPtr->UnregisterEvtLoopOut(true);
            tcpConnectionSPtr->UnregisterEvtLoopIn(true);
#else
            tcpConnectionSPtr->CleanupThreadPoolIo();
#endif

            // after this point there should be no more send/receive callbacks

            tcpConnectionSPtr->socket_.Close(SocketShutdown::None);

            TcpSendTargetSPtr tcpSendTarget;
            bool shouldReportFault = false;
            ErrorCode fault;
            {
                // Need to lock for target_ read in ScheduleClose, which may be called from threads
                // not associated with socket threadpool IO completion, e.g. from some timer callback
                AcquireWriteLock lockInScope(tcpConnectionSPtr->lock_);

                fault = tcpConnectionSPtr->fault_;

                // This must be called under lock as outgoing message purging timer may be active
                tcpConnectionSPtr->sendBuffer_->Abort();

                tcpSendTarget = move(tcpConnectionSPtr->tcpTarget_);
                tcpConnectionSPtr->target_ = nullptr;
                tcpConnectionSPtr->onSuspendingReceive_ = nullptr;

                shouldReportFault = tcpConnectionSPtr->shouldReportFault_;
                tcpConnectionSPtr->shouldReportFault_ = false;
            }

            if (tcpSendTarget)
            {
                tcpSendTarget->OnConnectionDisposed(tcpConnectionSPtr);
                if (shouldReportFault)
                {
                    // Report closure if nothing has been reported yet
                    tcpSendTarget->OnConnectionFault(fault);
                }
            }
        });
}

ErrorCode TcpConnection::Fault_CallerHoldingLock(ErrorCode const & fault)
{
    if (fault_.IsSuccess() && !fault.IsSuccess())
    {
        fault_ = fault;
        if (!fault_.IsWin32Error(WSAECONNABORTED))
        {
            ++connectionFailureCount_;
        }

        trace.ConnectionFaulted(
            traceId_,
            localAddress_,
            targetAddress_,
            fault_);
    }

    return fault_;
}

void TcpConnection::ReportFault(ErrorCode const & fault)
{
    ErrorCode connectionFault = fault;

    TcpSendTargetSPtr faultTarget;
    {
        AcquireWriteLock grab(lock_);

        if (!shouldReportFault_)
        {
            return;
        }

        shouldReportFault_ = false;
        faultTarget = tcpTarget_;

        connectionFault = Fault_CallerHoldingLock(connectionFault);
    }

    if (faultTarget)
    {
        faultTarget->OnConnectionFault(connectionFault);
    }
}

bool TcpConnection::CanReceive() const
{
    TcpConnectionState::Enum state = state_;
    return (state == TcpConnectionState::Connected) || (state == TcpConnectionState::CloseDraining);
}

bool TcpConnection::CanSend() const
{
    TcpConnectionState::Enum state = state_;
    return
        (state == TcpConnectionState::Created) ||
        (state == TcpConnectionState::Connecting) ||
        (state == TcpConnectionState::Connected);
}

bool TcpConnection::CanSendMessagesAlreadyQueued() const
{
    return CanSend() || (state_ == TcpConnectionState::CloseDraining);
}

void TcpConnection::Connect()
{
    Endpoint connectTo;
    auto error = TcpTransportUtility::TryParseEndpointString(targetAddress_, connectTo);
    if (error.IsSuccess())
    {
        // Target address has IP address
        targetEndpoints_.emplace_back(move(connectTo));
        SubmitConnect();
        return;
    }

    if (!error.IsWin32Error(WSAEINVAL))
    {
        WriteError(
            TraceType, traceId_,
            "{0}-{1}: failed to parse target address '{1}': {2}",
            localAddress_, targetAddress_, error);

        Abort(error);
        return;
    }

    // Post domain name resolving to threadpool, since there is no async resolving
    TcpConnectionSPtr thisSPtr = this->shared_from_this();
    Threadpool::Post([thisSPtr, this] { this->ResolveTargetHostName(); });
}

void TcpConnection::ResolveTargetHostName()
{
    auto error = TcpTransportUtility::TryResolveHostNameAddress(targetAddress_, hostnameResolveOption_, targetEndpoints_);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType, traceId_,
            "{0}-{1}: failed to resolve '{1}' (resolveOption={2}): {3}", 
            localAddress_,
            targetAddress_,
            hostnameResolveOption_,
            error);

        AbortWithRetryableError();
        return;
    }

    SubmitConnect();
}

ErrorCode TcpConnection::EnableKeepAliveIfNeeded(Socket const & socket, TimeSpan keepAliveTimeout)
{
    ErrorCode error;
    if (keepAliveTimeout <= TimeSpan::Zero)
    {
        return error;
    }

#ifdef PLATFORM_UNIX
    int keepalive = 1;
    if (setsockopt(socket.GetHandle(), SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)) < 0)
    {
        auto error = ErrorCode::FromErrno();
        WriteError(TraceType, traceId_, "setsockopt(SO_KEEPALIVE) failed: {0}", error);
        return error;
    }

    int keepidle = (int) keepAliveTimeout.TotalSeconds();
    if (setsockopt(socket.GetHandle(), SOL_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle)) < 0)
    {
        auto error = ErrorCode::FromErrno();
        WriteError(TraceType, traceId_, "setsockopt(TCP_KEEPIDLE) failed: {0}", error);
        return error;
    }

    int keepintvl = 1;
    if (setsockopt(socket.GetHandle(), SOL_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl)) < 0)
    {
        auto error = ErrorCode::FromErrno();
        WriteError(TraceType, traceId_, "setsockopt(TCP_KEEPINTVL) failed: {0}", error);
        return error;
    }
#else
    struct tcp_keepalive keepAliveParam =
    {
        TRUE,
        (ULONG)(keepAliveTimeout.TotalMilliseconds()),
        1000 /* 1 second retry interval if keep alive packet is not ACKed */
    };

    DWORD outputSize = 0;
    int retval = WSAIoctl(
        socket.GetHandle(),
        SIO_KEEPALIVE_VALS,
        &keepAliveParam,
        sizeof(keepAliveParam),
        nullptr,
        0,
        &outputSize,
        nullptr,
        nullptr);

    error = ErrorCode::FromWin32Error(::WSAGetLastError());
    WriteTrace(
        (retval == 0) ? LogLevel::Info : LogLevel::Error,
        TraceType, traceId_,
        "{0}-{1} enable TCP keep alive: {2}",
        localAddress_, targetAddress_, error);
#endif

    return error;
}

void TcpConnection::OnConnectionOpenTimeout()
{
    WriteInfo(
        TraceType, traceId_,
        "{0}-{1}: connection open timeout", 
        localAddress_,
        targetAddress_);

    Abort(ErrorCodeValue::Timeout);
}

void TcpConnection::StartOpenTimerIfNeeded(TimeSpan timeout)
{
    if (timeout <= TimeSpan::Zero) {
        WriteWarning(
            TraceType, traceId_,
            "ConnectionOpenTimeout = {0}, thus disabled",
            timeout);
        return;
    }

    auto connection = shared_from_this();
    openTimer_ = Timer::Create(
        "ConnectionOpen",
        [connection](TimerSPtr const&) { connection->OnConnectionOpenTimeout(); });

#ifdef PLATFORM_UNIX
    openTimer_->LimitToOneShot();
#endif

    WriteInfo(TraceType, traceId_, "ConnectionOpenTimeout = {0}", timeout);
    openTimer_->Change(timeout);
}

void TcpConnection::CancelOpenTimer()
{
    if (openTimer_)
    {
        openTimer_->Cancel();
    }
}

void TcpConnection::OnConnectionReady()
{
    if (inbound_ && securityContext_ && securityContext_->IsInRole(RoleMask::Admin))
    {
        //server side trusts admin clients completely
        DisableIncomingFrameSizeLimit();
    }

    CancelOpenTimer();

    if (readyCallback_) readyCallback_(this);
}

void TcpConnection::SuspendReceive(ThreadpoolCallback const & action)
{
    Invariant(!shouldSuspendReceive_);
    WriteInfo(
        TraceType, traceId_,
        "{0}-{1} suspending receive", 
        localAddress_,
        targetAddress_);

    onSuspendingReceive_ = action;
    shouldSuspendReceive_ = true;
}

void TcpConnection::ResumeReceive(bool incomingDataAllowed)
{
    Invariant(shouldSuspendReceive_);
    WriteInfo(
        TraceType, traceId_,
        "{0}-{1} resuming receive",
        localAddress_,
        targetAddress_);

    shouldSuspendReceive_ = false;
    incomingDataAllowed_ = incomingDataAllowed;
    ProcessReceivedBytes();
}

void TcpConnection::RecordMessageDispatching(MessageUPtr const & message)
{
    lastReceivedMessages_[lastReceivedIndex_].Id = message->TraceId();
    lastReceivedMessages_[lastReceivedIndex_].Actor = message->Actor;
    ++lastReceivedIndex_;
    lastReceivedIndex_ &= 1;
}

void TcpConnection::ProcessReceivedBytes()
{
    if (!incomingDataAllowed_)
    {
        SubmitReceive();
        return;
    }

    // state_ can change to Closed while we are in this loop, 
    // e.g. abort due to close timeout while draining send/receive 
    while ((state_ != TcpConnectionState::Closed))
    {
        if (!fault_.IsSuccess())
        {
            // stop draining a connection after it is faulted, e.g. auth failure 
            AcquireWriteLock grab(lock_);
            receiveDrained_ = true;
        }

        // return value is true if there are enough bytes for an entire message and
        // succeeded is true if the tcp frames were correct
        MessageUPtr message;
        NTSTATUS status = receiveBuffer_->GetNextMessage(message, lastReceiveCompleteTime_);
        if (status == STATUS_PENDING)
        {
            break;
        }

        if(status== STATUS_DATA_ERROR)
        {
            Abort(ErrorCodeValue::MessageTooLarge);
            return;
        }

        if (status != STATUS_SUCCESS)
        {
            AbortWithRetryableError();
            return;
        }

        if (!message->IsValid)
        {
            trace.DroppingInvalidMessage(TracePtr(message.get()), message->Status);
            AbortWithRetryableError();
            return;
        }

        ++dispatchCountForThisReceive_;
        incomingMessageHandler_(message);
        receiveBuffer_->ConsumeCurrentMessage();

        if (shouldSuspendReceive_)
        {
            Threadpool::Post(onSuspendingReceive_);
            return;
        }
    }

    SubmitReceive();
}

void TcpConnection::EnqueueListenInstanceMessage(TcpDatagramTransportSPtr const & transport)
{
    if (!transport->ShouldSendListenInstance())
    {
        expectRemoteListenInstance_ = false;
        return;
    }

    trace.CreateListenInstanceMessage(
        transport->TraceId(),
        localListenInstance_,
        traceId_);

    MessageUPtr listenInstanceMessage = make_unique<Message>(localListenInstance_);
    listenInstanceMessage->Headers.Add(MessageIdHeader());
    listenInstanceMessage->Headers.Add(ActorHeader(Actor::Transport));
    listenInstanceMessage->Headers.Add(HighPriorityHeader()); // Should not throttle on this message

    sendBuffer_->EnqueueMessage(move(listenInstanceMessage), TimeSpan::MaxValue, securityContext_ != nullptr);
}

void TcpConnection::EnqueueClaimsMessageIfNeeded()
{
    if (!securityContext_ || securityContext_->TransportSecurity().SecurityProvider != SecurityProvider::Claims)
    {
        return;
    }

    auto claimsMessage = CreateClaimsMessage(securityContext_->TransportSecurity().Settings().LocalClaimToken());

    // ClaimsMessage must be the first message to send once the channel is secured
    Invariant(sendBuffer_->HasNoMessageDelayedBySecurityNeogtiation());
    sendBuffer_->EnqueueMessage(move(claimsMessage), TimeSpan::MaxValue, true);
}

std::unique_ptr<Message> TcpConnection::CreateClaimsMessage(std::wstring const & claimsToken)
{
    ClaimsMessage claimsMessageBody(claimsToken);
    auto claimsMessage = make_unique<Message>(claimsMessageBody);
    claimsMessage->Headers.Add(ActorHeader(Actor::Transport));
    claimsMessage->Headers.Add(ActionHeader(*Constants::ClaimsMessageAction));
    claimsMessage->Headers.Add(MessageIdHeader());

    WriteInfo(
        TraceType, traceId_,
        "queueing claims message {0} '{1}'",
        claimsMessage->TraceId(), claimsMessageBody);

    return claimsMessage;
}

void TcpConnection::ReleaseMessagesOnSecuredConnection()
{
    auto handler = securityContext_->TransportSecurity().ClaimsRetrievalHandlerFunc();
    auto metadata = securityContext_->TransportSecurity().GetClaimsRetrievalMetadata();

    bool postedHandler = false;

    if (handler && metadata)
    {
        if (metadata->IsValid)
        {
            auto const & token = securityContext_->TransportSecurity().Settings().LocalClaimToken();

            if (!token.empty())
            {
                WriteInfo(TraceType, traceId_, "using claims token from security settings");
                securityContext_->CompleteClaimsRetrieval(ErrorCodeValue::Success, token);
            }
            else
            {
                WriteInfo(
                    TraceType, traceId_,
                    "firing claims retrieval handler({0})",
                    *metadata);

                handler(metadata, securityContext_);

                postedHandler = true;
            }
        }
        else
        {
            TRACE_AND_TESTASSERT(WriteWarning, TraceType, traceId_, "invalid claims retrieval metadata: {0}", *metadata);
            securityContext_->CompleteClaimsRetrieval(ErrorCodeValue::InvalidCredentials, L"");
        }
    }

    if (!postedHandler)
    {
        sendBuffer_->EnqueueMessagesDelayedBySecurityNegotiation(nullptr);
    }
}

void TcpConnection::CompleteClaimsRetrieval(ErrorCode const & error, std::wstring const & claimsToken)
{
    if (!error.IsSuccess())
    {
        Close_CallerHoldingLock(true, error);
        return;
    }

    WriteInfo(TraceType, traceId_, "CompleteClaimsRetrieval success");

    unique_ptr<Message> claimsMessage;
    if (!claimsToken.empty())
    {
        claimsMessage = CreateClaimsMessage(claimsToken);
    }

    sendBuffer_->EnqueueMessagesDelayedBySecurityNegotiation(move(claimsMessage));
}

void TcpConnection::ReceiveComplete(ErrorCode const & error, ULONG_PTR bytesTransferred)
{
    lastReceiveCompleteTime_ = Stopwatch::Now();
    receivePending_ = false;
    dispatchCountForThisReceive_ = 0;

    if (!error.IsSuccess())
    {
        TcpConnectionState::Enum state = state_;
        if (error.IsWin32Error(ERROR_OPERATION_ABORTED) && (state > TcpConnectionState::Connected))
        {
            WriteInfo(
                TraceType, traceId_,
                "{0}-{1} receive aborted at state {2}",
                localAddress_, targetAddress_, state);
            return;
        }

        trace.ReceiveFailed(traceId_, localAddress_, targetAddress_, error);
        AbortWithRetryableError();
        return;
    }

    if (bytesTransferred == 0) 
    {
#ifdef DBG
        ++socketReceiveDrainedCount_;
#endif

        AcquireWriteLock grab(lock_);

        receiveDrained_ = true;
        trace.ConnectionState(traceId_, localAddress_, targetAddress_, state_, L"receive drained", sendBuffer_->Empty(), receiveDrained_);
        Close_CallerHoldingLock(false, (state_ == TcpConnectionState::Connected)? ErrorCodeValue::ConnectionClosedByRemoteEnd : ErrorCodeValue::Success);
        return;
    }

    if (!incomingDataAllowed_)
    {
        WriteWarning(
            TraceType, traceId_,
            "{0}-{1} incoming data is not allowed, only expecting end-of-stream",
            localAddress_, targetAddress_);

        AbortWithRetryableError();
        return;
    }

    trace.ReceivedData(traceId_, bytesTransferred, receiveBuffer_->ReceivedByteTotal());
    receiveBuffer_->Commit(static_cast<size_t>(bytesTransferred));

    ProcessReceivedBytes();
}

void TcpConnection::ConnectComplete(ErrorCode const & error)
{
    bool triedAllAddresses = false;
    {
        AcquireWriteLock grab(lock_);

        if (state_ != TcpConnectionState::Connecting) return;

#ifdef PLATFORM_UNIX
        if (!connectActive_)
        {
            WriteWarning(TraceType, traceId_, "ignore extra connect completion");
            return;
        } 

        connectActive_ = false;

        if (error.IsSuccess())
        {
            RegisterEvtLoopIn();
        }
#endif

        targetEndpoints_.pop_back();
        triedAllAddresses = targetEndpoints_.empty();
    }

    if (error.IsSuccess())
    {        

        Endpoint socketLocalName;
        auto sockNameError = Endpoint::GetSockName(socket_, socketLocalName);
        if (!sockNameError.IsSuccess())
        {
            WriteError(
                TraceType, traceId_,
                "{0}-{1} : failed to retrieve local name of connected socket: {2}",
                localAddress_,
                targetAddress_,
                sockNameError);

            AbortWithRetryableError();
            return;
        }

        trace.ConnectionEstablished(traceId_, localAddress_, targetAddress_, socketLocalName.ToString());

        if (!expectRemoteListenInstance_ && !securityContext_)
        {
            OnConnectionReady();
        }

        if (!FinishSocketInit())
        {
            AbortWithRetryableError();
            return;
        }

        Send(nullptr, TimeSpan::MaxValue, false, TcpConnectionState::Connected);
        SubmitReceive();
        return;
    }

    wstring targetTcpAddress = IDatagramTransport::TargetAddressToTransportAddress(targetAddress_);
    auto failureCount = ++connectFailureCount_;
    trace.FailedToConnect(
        traceId_,
        localAddress_,
        targetAddress_,
        connectToAddress_,
        error,
        failureCount,
        wformatString("(type~Transport.St && ~\"(?i){0}\")", targetTcpAddress));

    if (triedAllAddresses)
    {
        WriteWarning(
            TraceType, traceId_,
            "{0}-{1} : connect failed, having tried all addresses",
            localAddress_,
            targetAddress_);

        Abort(ErrorCodeValue::CannotConnect);
        return;
    }

    SubmitConnect();
}

ErrorCode TcpConnection::Send(MessageUPtr && message, TimeSpan expiration, bool shouldEncrypt, TcpConnectionState::Enum newState)
{
    ErrorCode errorCode;
    bool shouldConnect = false;
    bool shouldSend = false;
    {
        AcquireWriteLock grab(lock_);

        bool canSendQueuedAtCloseDraining = CanSendMessagesAlreadyQueued() && !message;
        bool canSendPendingAtCloseDraining = CanSendMessagesAlreadyQueued() && (pendingSend_.load() > 0);
        bool canSend = CanSend() || canSendQueuedAtCloseDraining || canSendPendingAtCloseDraining;

        if (!canSend)
        {
            if (fault_.IsSuccess())
            {
                errorCode = ErrorCodeValue::OperationCanceled;
            }
            else
            {
                errorCode = fault_;
            }

            if (message)
            {
                trace.Msg_InvalidStateForSend(
                    traceId_,
                    localAddress_,
                    targetAddress_,
                    state_,
                    fault_,
                    message->TraceId(),
                    message->Actor,
                    message->Action);

                message->OnSendStatus(errorCode, move(message));
            }
            else
            {
                trace.Null_InvalidStateForSend(
                    traceId_,
                    localAddress_,
                    targetAddress_,
                    state_,
                    fault_);
            }

            return errorCode;
        }

        if (newState != TcpConnectionState::None)
        {
            TransitToState_CallerHoldingLock(newState);
        }

        if (message)
        {
            errorCode = sendBuffer_->EnqueueMessage(std::move(message), expiration, shouldEncrypt);
            if (!errorCode.IsSuccess())
            {
                return errorCode;
            }
        }

        if (sendActive_ || sendBuffer_->Empty())
        {
            return errorCode;
        }

        if (state_ == TcpConnectionState::Created)
        {
            TransitToState_CallerHoldingLock(TcpConnectionState::Connecting);
            shouldConnect = true;
        }
        else if ((state_ == TcpConnectionState::Connected) || (state_ == TcpConnectionState::CloseDraining))
        {
            auto error = sendBuffer_->Prepare();
            if (!error.IsSuccess())
            {
                  return error;
            }

            shouldSend = true;
            sendActive_ = true;
        }
        else
        {
            WriteNoise(
                TraceType, traceId_,
                "{0}-{1} Send: no action at state {2}",
                localAddress_, targetAddress_, state_);

            return errorCode;
        }

    }

    if (shouldConnect)
    {
        Connect();
        return errorCode;
    }

    if (shouldSend)
    {
        SubmitSend();
    }

    return errorCode;
}

void TcpConnection::SendComplete(ErrorCode const & error, ULONG_PTR bytesTransferred)
{
    // Upon overlapped IO completion, WSASend either has sent all the bytes successfully, or a failure
    // has occurred. Successful partial send can never occur, no need to handle partial send completion.
    if (!error.IsSuccess())
    {
        TcpConnectionState::Enum state = state_;
        if (error.IsWin32Error(ERROR_OPERATION_ABORTED) && (state > TcpConnectionState::Connected))
        {
            WriteInfo(
                TraceType, traceId_,
                "{0}-{1} send aborted at state {2}",
                localAddress_, targetAddress_, state);

            return;
        }

        WriteWarning(
            TraceType, traceId_,
            "{0}-{1} send failed at state {2}: {3}", 
            localAddress_, targetAddress_, state, error);

        AbortWithRetryableError();
        return;
    }

    {
        AcquireWriteLock grab(lock_);

        pendingSendStartTime_ = StopwatchTime::MaxValue;
        trace.SendCompleted(traceId_, bytesTransferred, sendBuffer_->SentByteTotal());
        sendBuffer_->Consume(bytesTransferred);
        sendActive_ = false;

        if (state_ == TcpConnectionState::Closed)
        {
            WriteNoise(
                TraceType, traceId_,
                "{0}-{1} SendComplete: no action at state {2}",
                localAddress_, targetAddress_, state_);

            return;
        }

        if (state_ == TcpConnectionState::CloseDraining)
        {
            if (CompletedAllSending_CallerHoldingLock())
            {
                trace.ConnectionState(traceId_, localAddress_, targetAddress_, state_, L"send drained", sendBuffer_->Empty(), receiveDrained_);
                auto innerError = ShutdownSocketSend_CallerHoldingLock();
                if (innerError.IsSuccess() && !receiveDrained_) return;

                TransitToState_CallerHoldingLock(TcpConnectionState::Closed);
                ScheduleCleanup_CallerHoldingLock();
                return;
            }

            trace.StartCloseDrainingSend(traceId_, localAddress_, targetAddress_);
        }
    }

    Send(nullptr, TimeSpan::MaxValue, false);
}

bool TcpConnection::IsOutgoingFrameSizeWithinLimit(size_t frameSize) const
{
    return frameSize <= maxOutgoingFrameSizeInBytes_;
}

bool TcpConnection::IsIncomingFrameSizeWithinLimit(size_t frameSize) const
{
    return frameSize <= maxIncomingFrameSizeInBytes_;
}

ULONG TcpConnection::BytesPendingForSend() const
{
    return sendBuffer_->BytesPendingForSend();
}

size_t TcpConnection::MessagesPendingForSend() const
{
    return sendBuffer_->MessageCount();
}

bool TcpConnection::IsInbound() const
{
    return inbound_;
}

bool TcpConnection::IsLoopback() const
{
    AcquireReadLock grab(lock_);
    return TcpTransportUtility::IsLocalEndpoint(remoteEndpoint_);
}

void TcpConnection::SetKeepAliveTimeout(Common::TimeSpan timeout)
{
    keepAliveTimeout_ = timeout;
}

ErrorCode TcpConnection::GetFault() const
{
    AcquireReadLock grab(lock_);
    return fault_;
}

void TcpConnection::StopSendAndScheduleClose(ErrorCode const & fault, bool notifyRemote, TimeSpan delay)
{
    TcpSendTargetSPtr tcpTarget;
    {
        AcquireReadLock grab(lock_);
        tcpTarget = tcpTarget_;
    }

    if (tcpTarget)
    {
        tcpTarget->StopSendAndScheduleClose(*this, fault, notifyRemote, delay);
    }
}

void TcpConnection::OnSessionExpired()
{
    TcpSendTargetSPtr target;
    {
        AcquireWriteLock grab(lock_);
        target = tcpTarget_;
    }

    if (!target) return;

    Invariant(securityContext_);

    bool shouldSendReconnect = inbound_;
    if (sessionExpired_)
    {
        if (securitySetttingsUpdated_)
        {
            WriteInfo(TraceType, traceId_, "OnSessionExpired: retire session at final expiration per security setting update");
            target->RetireExpiredConnection(*this);
            return;
        }

        shouldSendReconnect &= securityContext_->TransportSecurity().Settings().ReadyNewSessionBeforeExpiration();
    }
    else
    {
        sessionExpired_ = true;
        WriteInfo(TraceType, traceId_, "OnSessionExpired: expiration recorded, start connection refresh if needed");
    }

    if (shouldSendReconnect)
    {
        WriteInfo(TraceType, traceId_, "OnSessionExpired: notify client side for connection refresh");

        //Notify client side to reconnect to avoid message loss
        auto reconnectMsg = make_unique<Message>();
        reconnectMsg->Headers.Add(ActorHeader(Actor::TransportSendTarget));
        reconnectMsg->Headers.Add(ActionHeader(*Constants::ReconnectAction));
        reconnectMsg->Headers.Add(MessageIdHeader());
        SendOneWay_Dedicated(move(reconnectMsg), TimeSpan::MaxValue);

        //Schedule final session expiration in case connection refresh above failed
        //Use a longer timeout value on server side to avoid losing reply message for "long" reqeust&reply
        auto timeout = SecurityConfig::GetConfig().SessionExpirationCloseDelay;
        if (securityContext_->TransportSecurity().Settings().ReadyNewSessionBeforeExpiration())
        {
            timeout = timeout + SecurityConfig::GetConfig().SessionRefreshTimeout;
        }

        securityContext_->ScheduleSessionExpiration(timeout);
        return;
    }

    if (!securityContext_->TransportSecurity().Settings().ReadyNewSessionBeforeExpiration())
    {
        WriteInfo(TraceType, traceId_, "skipping connection refresh per settings");
        target->RetireExpiredConnection(*this);
        return;
    }

    target->StartConnectionRefresh(this);

    if (securitySetttingsUpdated_)
    {
        WriteInfo(TraceType, traceId_, "schedule final session expiration in case connection refresh failed");
        securityContext_->ScheduleSessionExpiration(SecurityConfig::GetConfig().SessionRefreshTimeout);
    }
    else
    {
        WriteInfo(TraceType, traceId_, "schedule retry of connection refresh");
        securityContext_->ScheduleSessionExpiration(SecurityConfig::GetConfig().SessionRefreshRetryDelay);
    }
}

void TcpConnection::ScheduleSessionExpiration(Common::TimeSpan newExpiration, bool securitySettingsUpdated)
{
    if (!securityContext_)
    {
        return;
    }

    if (securitySettingsUpdated) securitySetttingsUpdated_ = true;

    securityContext_->ScheduleSessionExpiration(newExpiration);
}

void TcpConnection::SetSecurityContext(TransportSecuritySPtr const & transportSecurity, wstring const & sspiTarget)
{
    Invariant(!securityContext_);
    securityContext_ = SecurityContext::Create(
        shared_from_this(),
        transportSecurity,
        sspiTarget,
        localListenInstance_);

    if (securityContext_)
    {
        sendBuffer_->SetSecurityProviderMask(transportSecurity->SecurityProvider);
        incomingMessageHandler_ = [this](MessageUPtr & message) { OnNegotiationMessageReceived(move(message)); };
    }
    else
    {
        incomingMessageHandler_ = [this](MessageUPtr & message) { OnUnsecuredMessageReceived(move(message)); };
    }
}

void TcpConnection::OnSessionSecured(MessageUPtr && finalNegoMessage, bool started)
{
    testAssertEnabled_ = true;

    ErrorCode error;
    {
        AcquireWriteLock lockInScope(lock_);

        if (finalNegoMessage)
        {
            trace.SecurityNegotiationSend(traceId_, finalNegoMessage->TraceId());
            error = sendBuffer_->EnqueueMessage(move(finalNegoMessage), TimeSpan::MaxValue, false);
            KAssert((finalNegoMessage == nullptr) || !error.IsSuccess());
        }

        if (error.IsSuccess())
        {
            sendBuffer_->EnableEncryptEnqueue();

            if (securityContext_->ConnectionAuthorizationFailed())
            {
                incomingMessageHandler_ = [this](MessageUPtr &) {}; // stop receiving
                if (inbound_ && sendBuffer_->CanSendAuthStatusMessage())
                {
                    error = sendBuffer_->EnqueueMessage(
                        securityContext_->CreateConnectionAuthMessage(securityContext_->FaultToReport()),
                        TimeSpan::MaxValue,
                        true);
                }
            }
            else
            {
                incomingMessageHandler_ = [this](MessageUPtr & message) { OnSecuredMessageReceived(move(message)); };
                if (inbound_ && sendBuffer_->CanSendAuthStatusMessage())
                {
                    error = sendBuffer_->EnqueueMessage(
                        securityContext_->CreateConnectionAuthMessage(ErrorCodeValue::Success),
                        TimeSpan::MaxValue,
                        true);
                }

                if (securityContext_->ShouldWaitForConnectionAuthStatus() &&
                    sendBuffer_->CanSendAuthStatusMessage() /* no wait in lease transport mode, in which auth status is not sent */)
                {
                    expectingConnectionAuthStatus_ = true;
                    sendBuffer_->DelayEncryptEnqueue();
                }
                else
                {
                    this->ReleaseMessagesOnSecuredConnection();
                }
            }
        }
    }

    if (!error.IsSuccess())
    {
        AbortWithRetryableError();
        return;
    }

    if (!expectRemoteListenInstance_)
    {
        OnConnectionReady();
    }

    if (started)
    {
        error = Send(nullptr, TimeSpan::MaxValue, false);
        if (!error.IsSuccess())
        {
            AbortWithRetryableError();
            return;
        }
    }
}

void TcpConnection::OnNegotiationMessageReceived(MessageUPtr && message)
{
    RecordMessageDispatching(message);
    trace.SecurityNegotiationDispatch(traceId_, message->TraceId(), message->IsReply(), message->Actor);

    unique_ptr<Message> negoMessageToSend;
    SECURITY_STATUS status = securityContext_->NegotiateSecurityContext(message, negoMessageToSend);
    if (securityContext_->NegotiationSucceeded())
    {
        OnSessionSecured(move(negoMessageToSend), true);
        if (securityContext_->ConnectionAuthorizationFailed())
        {
            if (inbound_)
            {
                ReportFault(securityContext_->FaultToReport());
                Close();
            }
            else
            {
                Abort(securityContext_->FaultToReport());
            }
        }

        return;
    }

    if (negoMessageToSend)
    {
        auto error = Send(move(negoMessageToSend), TimeSpan::MaxValue, false);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, traceId_, "failed to send negotiation message: {0}", error);
            AbortWithRetryableError();
            return;
        }

        if (FAILED(status))
        {
            ReportFault(securityContext_->FaultToReport());
            Close();
            return;
        }
    }

    if (FAILED(status))
    {
        AbortWithRetryableError();
    }
}

void TcpConnection::OnSecuredMessageReceived(MessageUPtr && message)
{
//LINUXTODO remove this conditional compile and always call ProcessClaimsMessage here
#ifdef PLATFORM_UNIX
    SECURITY_STATUS status = securityContext_->ProcessClaimsMessage(message);
#else
    SECURITY_STATUS status =
        securityContext_->FramingProtectionEnabled() ?
        securityContext_->ProcessClaimsMessage(message) : securityContext_->DecodeMessage(message);
#endif

    if (FAILED(status))
    {
        AbortWithRetryableError();
        return;
    }

    if (message) // claims message is consumed inside ProcessClaimsMessage/DecodeMessage
    {
        DispatchIncomingMessages(move(message));
    }
}

void TcpConnection::OnUnsecuredMessageReceived(MessageUPtr && message)
{
    DispatchIncomingMessages(move(message));
}

void TcpConnection::DispatchIncomingMessages(MessageUPtr && message)
{
    RecordMessageDispatching(message);

    if (shouldTracePerMessage_ && !IDatagramTransport::IsPerMessageTraceDisabled(message->Actor))
    {
        if (shouldTraceInboundActivity_)
        {
            FabricActivityHeader activityHeader;
            if (message->Headers.TryReadFirst(activityHeader))
            {
                trace.DispatchActivity(
                    traceId_,
                    message->TraceId(),
                    message->IsReply(),
                    dispatchCountForThisReceive_,
                    message->SerializedSize(),
                    message->Actor,
                    message->Action,
                    activityHeader.ActivityId);
            }
            else
            {
                trace.DispatchMsg(
                    traceId_,
                    message->TraceId(),
                    message->IsReply(),
                    dispatchCountForThisReceive_,
                    message->SerializedSize());
            }
        }
        else
        {
            trace.DispatchMsg(
                traceId_,
                message->TraceId(),
                message->IsReply(),
                dispatchCountForThisReceive_,
                message->SerializedSize());
        }
    }

    if ((message->Actor == Actor::TransportSendTarget) || (message->Actor == Actor::Transport))
    {
        // No need to lock to read owner_/target_, because, while threadpool IO is active for this connection,
        // owner_/target_ is only updated once in receive complete callback (TcpSendTarget::AcquireConnection),
        // and we never start multiple async receive operations in parallel
        tcpTarget_->OnMessageReceived(move(message), *this, target_);
        return;
    }

    if (throttle_ && throttle_->ShouldThrottle(*message, allowThrottleReplyMessage_))
    {
        return;
    }

    externalMessageHandler_(message, target_);
}

void TcpConnection::TransitToState_CallerHoldingLock(TcpConnectionState::Enum newState)
{
    trace.ConnectionStateTransit(traceId_, localAddress_, targetAddress_, state_, newState, sendBuffer_->Empty(), receiveDrained_);
    Invariant(state_ < newState); // state transition can only go in one direction
    state_ = newState;
}

void TcpConnection::SetConnectionAuthStatus(ErrorCode const &  authStatus, RoleMask::Enum roleGranted)
{
    if (!authStatus.IsSuccess())
    {
        Abort(authStatus);
        return;
    }

    if (!inbound_ && securityContext_->TransportSecurity().IsClientOnly() && (roleGranted & RoleMask::Enum::Admin))
    {
        WriteInfo(TraceType, traceId_, "disable outgoing message size limit as this connection is granted admin role on server side");
        DisableOutgoingFrameSizeLimit();
    }

    if (!expectingConnectionAuthStatus_)
    {
        return;
    }

    {
        AcquireWriteLock grab(lock_);
        expectingConnectionAuthStatus_ = false;
        this->ReleaseMessagesOnSecuredConnection();
    }

    auto error = Send(nullptr, TimeSpan::MaxValue, false); // Start send pump
    if (error.IsSuccess())
    {
        return;
    }

    AbortWithRetryableError();
}

Endpoint const & TcpConnection::GroupId() const
{
    return groupId_;
}

void TcpConnection::SetGroupId(Endpoint const & groupId)
{
    groupId_ = groupId;
}

bool TcpConnection::PrepareConnect_CallerHoldingLock(Endpoint & sockName, ErrorCode & error)
{
    if (state_ != TcpConnectionState::Connecting)
    {
        return false;
    }

    ASSERT_IFNOT(state_ == TcpConnectionState::Connecting,
        "Invalid connection state in Connect(): {0}", state_);

    remoteEndpoint_ = targetEndpoints_.back();
    remoteEndpoint_.ToString(connectToAddress_);

    trace.BeginConnect(traceId_, localAddress_, targetAddress_, connectToAddress_);

    Endpoint const * connectFrom = &::IPv4AnyAddress;
    if (!remoteEndpoint_.IsIPv4())
    {
        ASSERT_IFNOT(remoteEndpoint_.IsIPv6(), "unexpected Endpoint type");
        connectFrom = &::IPv6AnyAddress;
    }

    if (!socket_.closed())
    {
#ifdef PLATFORM_UNIX
        UnregisterEvtLoopOut(false);

        // Ensure this connection object be kept alive for a while as we cannot wait for
        // callback to complete in UnregisterEvtLoopOut call above due to possible deadlock
        auto thisSPtr=shared_from_this();
        Threadpool::Post([thisSPtr] {}, TransportConfig::GetConfig().EventLoopCleanupDelay);
#endif
        socket_.Close(SocketShutdown::None); // this is not the first connect attempt
    }

    error = socket_.Open(Common::SocketType::Tcp, connectFrom->AddressFamily);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType, traceId_,
            "{0}-{1} failed to open socket to connect: {2}",
            localAddress_, targetAddress_, error);
        return false;
    }

    error = socket_.Bind(*connectFrom);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType, traceId_,
            "{0}-{1} failed to bind to local port for connecting: {2}",
            localAddress_, targetAddress_, error);
        return false;
    }

    error = Endpoint::GetSockName(socket_, sockName);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType, traceId_,
            "{0}-{1} failed to get local name of connecting socket: {2}",
            localAddress_, targetAddress_, error);
        return false;
    }

    return true;
}

#ifdef PLATFORM_UNIX

void TcpConnection::RegisterEvtLoopIn()
{
    fdCtxIn_ = evtLoopIn_->RegisterFd(
        socket_.GetHandle(),
        EPOLLIN,
        eventLoopDispatchReadAsync_,
        [this] (int sd, uint evts) { ReadEvtCallback(sd, evts); });
}

void TcpConnection::RegisterEvtLoopOut()
{
    fdCtxOut_ = evtLoopOut_->RegisterFd(
        socket_.GetHandle(),
        EPOLLOUT,
        eventLoopDispatchWriteAsync_,
        [this] (int sd, uint evts) { WriteEvtCallback(sd, evts); });
}

void TcpConnection::UnregisterEvtLoopIn(bool waitForCallback)
{
    if(!fdCtxIn_) return;

    evtLoopIn_->UnregisterFd(fdCtxIn_, waitForCallback);
    fdCtxIn_ = nullptr;
}

void TcpConnection::UnregisterEvtLoopOut(bool waitForCallback)
{
    if(!fdCtxOut_) return;

    evtLoopOut_->UnregisterFd(fdCtxOut_, waitForCallback);
    fdCtxOut_ = nullptr;
}

int TcpConnection::get_iov_count(size_t bufferCount)
{
    if (bufferCount > IOV_MAX)
    {
        WriteNoise(TraceType, traceId_, "limit to {0} of {1} due to IOV_MAX", IOV_MAX, bufferCount);
        return IOV_MAX;
    }

    return bufferCount;
}

void TcpConnection::ReadEvtCallback(int sd, uint events)
{
    WriteNoise(TraceType, traceId_, "ReadEvtCallback: sd = {0:x}, events = {1:x}", sd, events);
    if(SocketErrorReported(sd, events)) return;

    auto const & buffers = receiveBuffer_->GetBuffers(receiveBufferToReserve_);
    for(;;)
    {
        auto received = readv(socket_.GetHandle(), buffers.data(), get_iov_count(buffers.size()));
        if ((received < 0) && (errno == EINTR)) continue;

        ErrorCode error = (received < 0)? ErrorCode::FromErrno() : ErrorCode();
        ReceiveComplete(error, received);
        break;
    }
}

void TcpConnection::OnSocketWriteEvt()
{
    int sent = 0;
    bool sendCompleted = false;
    uint totalPreparedBytes = 0;
    int writevErrno = 0;
    int bufferCount = 0;
    {
        AcquireWriteLock grab(lock_); //LINUXTODO read lock enough?

        if (state_ > TcpConnectionState::CloseDraining) return;

        totalPreparedBytes = sendBuffer_->TotalPreparedBytes();
        if (totalPreparedBytes > 0)
        {
            auto const & buffers = sendBuffer_->PreparedBuffers();
            auto bufferIndex = sendBuffer_->FirstBufferToSend();
            bufferCount = get_iov_count(buffers.size() - bufferIndex);

//#ifdef DBG
//            WriteNoise(TraceType, traceId_, "OnSocketWriteEvt: bufferIndex = {0}, bufferCount = {1}", bufferIndex, bufferCount);
//            for (uint i = bufferIndex; i < bufferIndex + bufferCount; ++i)
//            {
//                WriteNoise(TraceType, traceId_, "OnSocketWriteEvt: buffer[{0}] has {1} bytes", i, buffers[i].iov_len);
//            }
//#endif 

            do
            {
                sent = writev(socket_.GetHandle(), &(buffers[bufferIndex]), bufferCount);
            }
            while((sent < 0) && (errno == EINTR));

            if (sent < 0)
            {
                writevErrno = errno;
                Invariant(writevErrno);
            }

            if (sent > 0)
            {
                sendCompleted = sendBuffer_->ConsumePreparedBuffers(sent);
            }
        }
        else
        {
            // We may get here when connection is aborted while connecting
            WriteInfo(TraceType, traceId_, "ignore unexpected write event");
            return;
        }
    }

    // SendComplete must be called outside lock_ scope to avoid deadlock

    if (sent < 0)
    {
        WriteInfo(
            TraceType, traceId_,
            "writev({0}) returned {1}, errno = {2}",
            bufferCount,
            sent,
            writevErrno);

        SendComplete(ErrorCode::FromErrno(writevErrno), sent);
        return;
    }

    WriteNoise(
        TraceType, traceId_,
        "writev({0}) returned {1}",
        bufferCount,
        sent);

    if (sendCompleted)
    {
        SendComplete(ErrorCode(), totalPreparedBytes); 
        return;
    }

    // continue with the rest next time socket can be written 
    auto error = evtLoopOut_->Activate(fdCtxOut_);
    if (!error.IsSuccess())
    {
        AbortWithRetryableError();
    }
}

bool TcpConnection::SocketErrorReported(int sd, uint events)
{
    if ((events & EPOLLERR) == 0) return false;

    WriteInfo(TraceType, traceId_, "socket error reported");
    int sockError;
    socklen_t sockErrorSize = sizeof(sockError);
    if (getsockopt(sd, SOL_SOCKET, SO_ERROR, &sockError, &sockErrorSize) < 0)
    {
        auto error = ErrorCode::FromErrno();
        WriteError(
            TraceType, traceId_,
            "getsockopt({0:x}) failed: {1}",
            sd,
            error);

        AbortWithRetryableError();
        return true;
    }

    auto error = sockError? ErrorCode::FromErrno(sockError) : ErrorCodeValue::OperationCanceled;
    if (state_ == TcpConnectionState::Connecting)
    {
        ConnectComplete(ErrorCodeValue::CannotConnect);
        return true;
    }

    AbortWithRetryableError();
    return true;
}

void TcpConnection::WriteEvtCallback(int sd, uint events)
{
    WriteNoise(TraceType, traceId_, "WriteEvtCallback: sd = {0:x}, events = {1:x}", sd, events);
    if(SocketErrorReported(sd, events)) return;

    if (state_ == TcpConnectionState::Connecting)
    {
        ErrorCode err;
        if (events & EPOLLHUP)
        {
            err = ErrorCodeValue::CannotConnect;
        }

        ConnectComplete(err);
        return;
    }

    OnSocketWriteEvt();
}

void TcpConnection::SubmitConnect()
{
    ErrorCode error;
    for(;;)
    {
        AcquireWriteLock grab(lock_);

        Invariant(!connectActive_);
        connectActive_ = true;

        Endpoint sockName;
        if (!PrepareConnect_CallerHoldingLock(sockName, error)) break;
         
        trace.ConnectionBoundLocally(traceId_, sockName.ToString());

        error = socket_.SetNonBlocking();
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType, traceId_,
                "{0}-{1} : failed to make connecting socket nonblocking: {2}",
                localAddress_, targetAddress_, error);
            break;
        }

        auto retval = connect(socket_.GetHandle(), remoteEndpoint_.as_sockaddr(), remoteEndpoint_.AddressLength);
        RegisterEvtLoopOut();
        if (retval < 0)
        {
            if (errno == EINPROGRESS)
            {
                WriteInfo(TraceType, traceId_, "connect in progress");
                error = evtLoopOut_->Activate(fdCtxOut_);
                if (error.IsSuccess()) return; // connect will complete asynchronously

                error = ErrorCodeValue::CannotConnect;
                break;
            }

            error = ErrorCode::FromErrno();
            WriteError(
                TraceType, traceId_,
                "{0}-{1} : connect() failed: {2}",
                localAddress_, targetAddress_, error);

            error = ErrorCodeValue::CannotConnect;
            break;
        }

        WriteInfo(TraceType, traceId_, "connected synchronously");
        break;
    }

    ConnectComplete(error);
}

void TcpConnection::SubmitReceive()
{
    ErrorCode error;
    {
        AcquireReadLock grab(lock_);

        if (!CanReceive())
        {
            return;
        }

        Invariant(!receivePending_);
        receivePending_ = true;
        lastRecvCompeteTime_ = Stopwatch::Now();
        trace.BeginReceive(traceId_);
        error = evtLoopIn_->Activate(fdCtxIn_);
    }

    if (!error.IsSuccess())
    {
        AbortWithRetryableError();
    }
}

void TcpConnection::SubmitSend()
{
    ErrorCode error;
    {
        AcquireReadLock grab(lock_);

        if (!CanSendMessagesAlreadyQueued() || sendBuffer_->PreparedBuffers().empty() /* all messages expired*/)
        {
            sendActive_ = false;
            return;
        }

        pendingSendStartTime_ = Stopwatch::Now();
        trace.BeginSend(traceId_);
        error = evtLoopOut_->Activate(fdCtxOut_);
    }

    if (!error.IsSuccess())
    {
        AbortWithRetryableError();
    }
}

#else

void TcpConnection::SubmitConnect()
{
    DWORD bytesTransferred = 0;
    ErrorCode error;
    bool createdIo = false;
    bool success = false;
    KFinally([&]
    {
        if (success) return;

        if (createdIo)
        {
            connectOverlapped_.Complete(error, bytesTransferred);
            return;
        }

        AbortWithRetryableError();
    });

    {
        AcquireWriteLock grab(lock_);

        Endpoint sockName;
        if (!PrepareConnect_CallerHoldingLock(sockName, error)) return;

        if (TcpTransportUtility::IsLocalEndpoint(remoteEndpoint_))
        {
            // When connecting to local machine, if OS happens to assign the same value to local port as the remote port, then a single
            // socket will be created in OS, instead of two. This rarely happens, but it can happen when we are constantly starting/
            // stopping TCP listeners in dynamic port range, which is what we do in FederationTest and FabricTest in order to avoid 
            // port conflicts. Such a rare "local port equals remote port in a loopback connection" situation will cause issues, 
            // because both connecting side and accepting side make receive calls on the same socket. Thus, the receive calls will be
            // interleaved, we may receive partial messages, also, receive call may retrieve data sent by the receive caller. Bug 
            // 714773 is a case where the connecting side read back the connection frame it just sent. So, we need to avoid having such
            // "local port equals remote port in a loopback connection" situation.
            while (sockName.Port == remoteEndpoint_.Port)
            {
                WriteInfo(
                    TraceType, traceId_,
                    "discarding candidate local address {0}, because we are connecting to local endpoint {1}",
                    sockName,
                    remoteEndpoint_);

                auto const * connectFrom = remoteEndpoint_.IsIPv4() ? (&::IPv4AnyAddress) : (&::IPv6AnyAddress);
                Socket socket;
                error = socket.Open(Common::SocketType::Tcp, connectFrom->AddressFamily);
                if (!error.IsSuccess())
                {
                    WriteError(
                        TraceType, traceId_,
                        "{0}-{1} failed to bind to local port for connecting: {2}",
                        localAddress_, targetAddress_, error);
                    return;
                }

                error = socket.Bind(*connectFrom);
                if (!error.IsSuccess())
                {
                    WriteError(
                        TraceType, traceId_,
                        "{0}-{1} failed to bind to local port for connecting: {2}",
                        localAddress_, targetAddress_, error);
                    return;
                }

                error = Endpoint::GetSockName(socket_, sockName);
                if (!error.IsSuccess())
                {
                    WriteError(
                        TraceType, traceId_,
                        "{0}-{1} failed to get local name of connecting socket: {2}",
                        localAddress_, targetAddress_, error);
                    return;
                }

                socket_.swap(socket);
            }
        }

        trace.ConnectionBoundLocally(traceId_, sockName.ToString());

        error = threadpoolIo_.Open((HANDLE)socket_.GetHandle());
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType, traceId_,
                "{0}-{1} failed to open threadpool io: {2}",
                localAddress_,
                targetAddress_,
                error);
            return;
        }

        createdIo = true;
        threadpoolIo_.StartIo();

        error = socket_.ConnectEx(
            remoteEndpoint_,
            nullptr,
            0,
            &bytesTransferred,
            connectOverlapped_.OverlappedPtr());

        if (!error.IsSuccess() && !error.IsWin32Error(WSA_IO_PENDING))
        {
            threadpoolIo_.CancelIo();
            return;
        }

        success = true;
    }
}

void TcpConnection::SubmitReceive()
{
    ErrorCode error;
    DWORD bytesTransferred = 0;
    {
        // read lock is fine here as threadpoolIo_ and socket_ operations performed
        // below are thread safe, and we do not issue concurrent receive operations.
        AcquireReadLock grab(lock_);

        if (!CanReceive())
        {
            return;
        }

        if (!threadpoolIo_)
        {
            return;
        }

        Invariant(!receivePending_);
        receivePending_ = true;
        lastRecvCompeteTime_ = Stopwatch::Now();
        trace.BeginReceive(traceId_);
        threadpoolIo_.StartIo();

        error = socket_.AsyncReceive(receiveBuffer_->GetBuffers(receiveBufferToReserve_), receiveOverlapped_.OverlappedPtr());
        if (error.IsSuccess() || error.IsWin32Error(WSA_IO_PENDING))
        {
            return;
        }

        threadpoolIo_.CancelIo();
    }

    receiveOverlapped_.Complete(error, bytesTransferred);
}

void TcpConnection::SubmitSend()
{
    DWORD bytesTransferred = 0;
    ErrorCode error;
    {
        // read lock is fine here as threadpoolIo_ and socket_ operations performed
        // below are thread safe, and we do not issue concurrent send operations.
        AcquireReadLock grab(lock_);

        if (!CanSendMessagesAlreadyQueued() || !threadpoolIo_ || sendBuffer_->PreparedBuffers().empty() /* all messages expired*/)
        {
            sendActive_ = false;
            return;
        }

        pendingSendStartTime_ = Stopwatch::Now();
        trace.BeginSend(traceId_);
        threadpoolIo_.StartIo();

        error = socket_.AsyncSend(sendBuffer_->PreparedBuffers(), sendOverlapped_.OverlappedPtr());
        if (error.IsSuccess() || error.IsWin32Error(WSA_IO_PENDING))
        {
            return;
        }

        threadpoolIo_.CancelIo();
    }

    sendOverlapped_.Complete(error, bytesTransferred);
}

void TcpConnection::CleanupThreadPoolIo()
{
    if (socket_.GetHandle() != INVALID_SOCKET)
    {
        ThreadpoolIo ioToClose;
        {
            AcquireWriteLock grab(lock_);
            threadpoolIo_.Swap(ioToClose);
        }
    }

    trace.ConnectionThreadpoolIoCleanedUp(traceId_, localAddress_, targetAddress_);
}

#endif

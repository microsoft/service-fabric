// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;

static const StringLiteral TraceType("Client");
static const StringLiteral ReconnectTimerTag("IpcReconnect");
static const IpcEventSource ipcTrace;

namespace
{
    IDatagramTransportSPtr CreateTransport(
        Common::ComponentRoot const & root,
        std::wstring const & clientId,
        std::wstring const & owner,
        bool useUnreliableTransport)
    {
        IDatagramTransportSPtr transport = DatagramTransportFactory::CreateTcpClient(clientId, owner + L".IpcClient");

        //Support for Unreliable transport for request reply over IPC
        if (useUnreliableTransport && TransportConfig::GetConfig().UseUnreliableForRequestReply)
        {
            IpcClient::WriteInfo(TraceType, "Unreliable Transport client enabled for RequestReply over IPC");
            transport = DatagramTransportFactory::CreateUnreliable(root, transport);
        }

        return transport;
    }
}

IpcClient::IpcClient(
    ComponentRoot const & root,
    wstring const & clientId,
    wstring const & serverTransportAddress,
    bool useUnreliableTransport,
    wstring const & owner)
    : clientId_(clientId),
    traceId_(clientId.empty()? wformatString("{0}", TextTraceThis) : wformatString("{0}-{1}", TextTraceThis, clientId)),
    processId_(GetCurrentProcessId()),
    serverTransportAddress_(serverTransportAddress),
    transport_(CreateTransport(root, clientId, owner, useUnreliableTransport)),
    demuxer_(root, transport_),
    requestReply_(root, transport_, false /* dispatchOnTransportThread */),
    disconnectCount_(0)
{
    /*
    1. If we want to use Unreliable transport but we want RequestReply communication to happen over reliable transport
       then we create the transport_ with TCP, initialize the demux and RequestReply with transport_ and later switch the
       transport to Unreliable.
    2. If we want to use RequestReply over IPC but the support for Unreliable transport is disabled then we create both the transport
       and RequestReply over the TCP
    3. If the support for Unreliable transport and RequestReply over IPC is enabled we create both the transport and RequestReply
       over Unreliable.
    */

    ipcTrace.ClientCreated(traceId_, owner);

    // Disable connection idle timeout and session expiration, since IpcServer cannot connect back to IpcClient
    transport_->SetConnectionIdleTimeout(TimeSpan::Zero);
    transport_->DisableSecureSessionExpiration();

    this->transport_->DisableThrottle();
    this->transport_->SetMaxOutgoingFrameSize(TransportConfig::GetConfig().IpcMaxMessageSize);
    this->transport_->SetKeepAliveTimeout(TransportConfig::GetConfig().IpcKeepaliveIdleTime);
    this->demuxer_.SetReplyHandler(this->requestReply_);
        
    //If Unreliable transport is disabled for request reply over IPC
    if (useUnreliableTransport && !TransportConfig::GetConfig().UseUnreliableForRequestReply)
    {
        this->transport_ = DatagramTransportFactory::CreateUnreliable(root, this->transport_);
    }
    
}

IpcClient::~IpcClient()
{
    Abort();
}

void IpcClient::OnDisconnect(ErrorCode error)
{
    // Retry on both connection failure and proper close
    WriteInfo(
        TraceType, traceId_,
        "disconnected from IpcServer, error = {0}, will reconnect in {1}",
        error, TransportConfig::GetConfig().IpcReconnectDelay);

    if ((0 < TransportConfig::GetConfig().IpcClientDisconnectLimit) &&
        !error.IsError(ErrorCodeValue::CannotConnect) &&
        (TransportConfig::GetConfig().IpcClientDisconnectLimit <= uint(++ disconnectCount_)))
    {
        WriteError(
            TraceType, traceId_,
            "reaching IpcClientDisconnectLimit {0}, exiting process",
            TransportConfig::GetConfig().IpcClientDisconnectLimit);

        ::ExitProcess((uint)ErrorCodeValue::TooManyIpcDisconnect);
    }

    reconnectTimer_->Change(TransportConfig::GetConfig().IpcReconnectDelay);
}

void IpcClient::RegisterMessageHandler(Actor::Enum actor, IpcMessageHandler const & messageHandler, bool dispatchOnTransportThread)
{
    this->demuxer_.RegisterMessageHandler(actor, messageHandler, dispatchOnTransportThread);
}

void IpcClient::UnregisterMessageHandler(Actor::Enum actor)
{
    this->demuxer_.UnregisterMessageHandler(actor);
}

ErrorCode IpcClient::OnOpen()
{
    requestReply_.Open();

    reconnectTimer_ = Timer::Create(
        ReconnectTimerTag,
        [this] (TimerSPtr const&)
        {
            Reconnect();
        },
        false);

    reconnectTimer_->SetCancelWait();

    transport_->SetConnectionFaultHandler(
        [this](ISendTarget const &, ErrorCode error)
        {
            OnDisconnect(error);
        });

    auto errorCode = this->demuxer_.Open();
    if (!errorCode.IsSuccess())
    {
        return errorCode;
    }

    errorCode = this->transport_->Start();
    if (!errorCode.IsSuccess())
    {
        return errorCode;
    }

    serverSendTarget_ = transport_->ResolveTarget(serverTransportAddress_);
    return errorCode;
}

void IpcClient::Reconnect()
{
    auto message = make_unique<Message>();
    message->Headers.Add(MessageIdHeader());
    message->Headers.Add(ActorHeader(Actor::Enum::Ipc));
    ipcTrace.ClientReconnect(traceId_, message->TraceId());
    SendOneWay(move(message));
}

ErrorCode IpcClient::OnClose()
{
    Cleanup();
    return ErrorCode( ErrorCodeValue::Success );
}

void IpcClient::OnAbort()
{
   Cleanup();
}

void IpcClient::SetMaxOutgoingFrameSize(ULONG value)
{
    transport_->SetMaxOutgoingFrameSize(value);
}

void IpcClient::Cleanup()
{
    transport_->RemoveConnectionFaultHandler();
    if (reconnectTimer_)
    {
        WriteInfo(TraceType, traceId_, "canceling reconnect timer");
        reconnectTimer_->Cancel();
    }

    auto errorCode = this->demuxer_.Close();
    if (!errorCode.IsSuccess())
    {
        errorCode.ReadValue();
        WriteWarning(TraceType, traceId_, "demuxer_.Close() failed: {0}", errorCode);
    }

    transport_->Stop();

    this->requestReply_.Close();
}

AsyncOperationSPtr IpcClient::BeginRequest(
    MessageUPtr && request, 
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return BeginRequest(move(request), TransportPriority::Normal, timeout, callback, parent);
}

AsyncOperationSPtr IpcClient::BeginRequest(
    MessageUPtr && request, 
    TransportPriority::Enum priority,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    OnSending(request);
    return this->requestReply_.BeginRequest(move(request), this->serverSendTarget_, priority, timeout, callback, parent);
}

ErrorCode IpcClient::EndRequest(AsyncOperationSPtr const & operation, MessageUPtr & reply)
{
    return this->requestReply_.EndRequest(operation, reply);
}

Common::ErrorCode IpcClient::SendOneWay(MessageUPtr && message, TimeSpan expiration, TransportPriority::Enum priority)
{
    OnSending(message);
    return this->transport_->SendOneWay(this->serverSendTarget_, move(message), expiration, priority);
}

void IpcClient::OnSending(MessageUPtr & message)
{
    if (message->MessageId.IsEmpty())
    {
        message->Headers.Add(MessageIdHeader());
    }

    message->Headers.Add(IpcHeader(clientId_, processId_));
}

void IpcClient::set_SecuritySettings(Transport::SecuritySettings const & value)
{ 
    transport_->SetSecurity(value);
}

Transport::SecuritySettings const & IpcClient::get_SecuritySettings() const
{
    return transport_->Security()->Settings();
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

static const  StringLiteral TraceType("Server");
static const IpcEventSource ipcTrace;

namespace
{
    IDatagramTransportSPtr CreateTransport(
        ComponentRoot const & root,
        wstring const & transportListenAddress,
        wstring const & serverId,
        std::wstring const & owner,
        bool useUnreliableTransport)
    {
        if (transportListenAddress.empty())
        {
            return nullptr;
        }

        auto transport = DatagramTransportFactory::CreateTcp(transportListenAddress, serverId, owner + L".IpcServer");

        //Support for Unreliable transport for request reply over IPC
        if (useUnreliableTransport && TransportConfig::GetConfig().UseUnreliableForRequestReply)
        {
            IpcServer::WriteInfo(TraceType, "Unreliable Transport server enabled for RequestReply over IPC");
            transport = DatagramTransportFactory::CreateUnreliable(root, transport);
        }

        return transport;
    }
}

class IpcServer::ClientTable
{
    DENY_COPY(ClientTable);
public:
    ClientTable(wstring const & traceId);

    void UpdateIfNeeded(wstring const & clientId, ISendTarget::SPtr const & sendTarget, DWORD clientProcessId);
    bool Remove(wstring const & clientId);
    ISendTarget::SPtr GetSendTarget(wstring const & clientId, _Out_ DWORD & clientProcessId);

    void Close();

private:
    wstring const traceId_;
    RwLock lock_;
    unordered_map<wstring /* client ID */, pair<ISendTarget::SPtr, DWORD/*ClientProcessId*/>> clients_;
};

IpcServer::ClientTable::ClientTable(wstring const & traceId) : traceId_(traceId)
{
}

void IpcServer::ClientTable::UpdateIfNeeded(wstring const & clientId, ISendTarget::SPtr const & sendTarget, DWORD clientProcessId)
{
    AcquireWriteLock lockInScope(lock_);

    auto iter = clients_.find(clientId);
    if (iter == clients_.end())
    {
        clients_.emplace(pair<wstring, pair<ISendTarget::SPtr, DWORD>>(clientId, pair<ISendTarget::SPtr, DWORD>(sendTarget, clientProcessId)));
    }
    else if ((sendTarget != iter->second.first) || (clientProcessId != iter->second.second))
    {
        iter->second = pair<ISendTarget::SPtr, DWORD>(sendTarget, clientProcessId);
    }
}

bool IpcServer::ClientTable::Remove(wstring const & clientId)
{
    AcquireWriteLock lockInScope(lock_);

    auto iter = clients_.find(clientId);
    if (iter == clients_.cend())
    {
        ipcTrace.ServerFailedToRemoveClient(traceId_, clientId);
        return false;
    }

    clients_.erase(iter);
    ipcTrace.ServerRemovedClient(traceId_, clientId);
    return true;
}

_Use_decl_annotations_
ISendTarget::SPtr IpcServer::ClientTable::GetSendTarget(wstring const & clientId, DWORD & clientProcessId)
{
    AcquireReadLock lockInScope(lock_);

    auto iter = clients_.find(clientId);
    if (iter == clients_.cend())
    {
        clientProcessId = 0;
        return nullptr;
    }

    clientProcessId = iter->second.second;
    return iter->second.first;
}

void IpcServer::ClientTable::Close()
{
    AcquireWriteLock lockInScope(lock_);
    clients_.clear();
}

IpcServer::TransportUnit::TransportUnit(
    IpcServer* ipcServer,
    Common::ComponentRoot const & root,
    std::wstring const & listenAddress,
    std::wstring const & serverId,
    std::wstring const & owner,
    std::wstring const & traceId,
    bool useUnreliableTransport) :
    ipcServer_(ipcServer),
    listenAddress_(listenAddress),
    transport_(CreateTransport(root, listenAddress, serverId, owner, useUnreliableTransport)),
    demuxer_(root, transport_),
    requestReply_(root, transport_, /* dispatchOnTransportThread = */false),
    clientTable_(make_unique<ClientTable>(traceId))
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

    // Disable connection idle timeout and session expiration, since IpcServer cannot connect back to IpcClient
    transport_->SetConnectionIdleTimeout(TimeSpan::Zero);
    transport_->DisableSecureSessionExpiration();
    transport_->DisableThrottle();
    transport_->SetMaxIncomingFrameSize(TransportConfig::GetConfig().IpcMaxMessageSize);
    transport_->SetKeepAliveTimeout(TransportConfig::GetConfig().IpcKeepaliveIdleTime);

    demuxer_.SetReplyHandler(requestReply_);

    //If Unreliable transport is disabled for request reply over IPC
    if (useUnreliableTransport && !TransportConfig::GetConfig().UseUnreliableForRequestReply)
    {
        WriteInfo(TraceType, traceId, "Unreliable Transport server enabled");
        transport_ = DatagramTransportFactory::CreateUnreliable(root, transport_);
    }
}

ErrorCode IpcServer::TransportUnit::Open()
{
    requestReply_.Open();

    demuxer_.RegisterMessageHandler(
        Actor::Enum::Ipc,
        [this](MessageUPtr & message, IpcReceiverContextUPtr & context)
        {
            this->ipcServer_->OnReconnectMessageReceived(message, context, *clientTable_);
        },
        true);

    auto errorCode = demuxer_.Open();
    if (!errorCode.IsSuccess())
    {
        WriteError(TraceType, ipcServer_->traceId_, "demuxer_.Open() failed: {0}", errorCode);
        return errorCode;
    }

    errorCode = transport_->Start();
    if (errorCode.IsSuccess())
    {
        listenAddress_ = transport_->ListenAddress();
    }

    return errorCode;
}

void IpcServer::TransportUnit::Close()
{
    auto errorCode = demuxer_.Close();
    if (!errorCode.IsSuccess())
    {
        errorCode.ReadValue();
        WriteWarning(TraceType, ipcServer_->traceId_, "demuxer_.Close() failed: {0}", errorCode);
    }

    transport_->Stop();
    requestReply_.Close();
    clientTable_->Close();
}

IpcServer::IpcServer(
    ComponentRoot const & root,
    wstring const & listenAddress,
    wstring const & listenAddressTls,
    wstring const & serverId,
    bool useUnreliableTransport,
    wstring const & owner) :
    serverId_(serverId),
    traceId_(serverId.empty() ? wformatString("{0}", TextTraceThis) : wformatString("{0}-{1}", TextTraceThis, serverId)),
    localUnit_(this, root, listenAddress, serverId, owner, traceId_, useUnreliableTransport),
    tlsUnit_(listenAddressTls.empty() ? nullptr : make_unique<TransportUnit>(this, root, listenAddressTls, serverId, owner, traceId_, useUnreliableTransport))
{
    ipcTrace.ServerCreated(traceId_, owner);
}

IpcServer::IpcServer(
    Common::ComponentRoot const & root,
    std::wstring const & transportListenAddress,
    std::wstring const & serverId,
    bool useUnreliableTransport,
    std::wstring const & owner) : IpcServer(root, transportListenAddress, L"", serverId, useUnreliableTransport, owner)
{
}

IpcServer::~IpcServer()
{
    Abort();
}

void IpcServer::RegisterMessageHandler(Actor::Enum actor, IpcMessageHandler const & messageHandler, bool dispatchOnTransportThread)
{
    localUnit_.demuxer_.RegisterMessageHandler(
        actor,
        [this, messageHandler](MessageUPtr & message, IpcReceiverContextUPtr & context)
        {
            localUnit_.clientTable_->UpdateIfNeeded(context->From, context->ReplyTarget, context->FromProcessId);
            messageHandler(message, context);
        },
        dispatchOnTransportThread);

    if (tlsUnit_)
    {
        tlsUnit_->demuxer_.RegisterMessageHandler(
            actor,
            [this, messageHandler](MessageUPtr & message, IpcReceiverContextUPtr & context)
            {
                tlsUnit_->clientTable_->UpdateIfNeeded(context->From, context->ReplyTarget, context->FromProcessId);
                messageHandler(message, context);
            },
            dispatchOnTransportThread);
    }
}

void IpcServer::UnregisterMessageHandler(Actor::Enum actor)
{
    localUnit_.demuxer_.UnregisterMessageHandler(actor);
    if (tlsUnit_)
    {
        tlsUnit_->demuxer_.UnregisterMessageHandler(actor);
    }
}

void IpcServer::OnReconnectMessageReceived(MessageUPtr & message, IpcReceiverContextUPtr & receiverContext, ClientTable & clientTable)
{
    IpcHeader ipcHeader;
    if (!message->Headers.TryReadFirst(ipcHeader))
    {
        ipcTrace.ServerReceivedBadReconnect(traceId_, message->TraceId(), receiverContext->ReplyTarget->Address());
        receiverContext->ReplyTarget->Reset();
        Assert::TestAssert();
        return;
    }

    ipcTrace.ServerReceivedReconnect(
        traceId_,
        receiverContext->From,
        ipcHeader.FromProcessId,
        receiverContext->ReplyTarget->Address());

    clientTable.UpdateIfNeeded(ipcHeader.From, receiverContext->ReplyTarget, ipcHeader.FromProcessId);
}

ErrorCode IpcServer::OnOpen()
{
    if (!TcpTransportUtility::IsLoopbackAddress(localUnit_.listenAddress_))
    {
        WriteError(TraceType, traceId_, "IPC requires loopback addresses, {0} is not", localUnit_.listenAddress_);
        return ErrorCodeValue::InvalidAddress;
    }

    auto errorCode = localUnit_.Open();
    if (!errorCode.IsSuccess()) return errorCode;

    if (tlsUnit_)
    {
        errorCode = tlsUnit_->Open();
        if (!errorCode.IsSuccess()) return errorCode;
    }

    return errorCode;
}

ErrorCode IpcServer::OnClose()
{
    Cleanup();
    return ErrorCode( ErrorCodeValue::Success );
}

void IpcServer::OnAbort()
{
    Cleanup();
}

void IpcServer::Cleanup()
{
    localUnit_.Close();
    if (tlsUnit_)
    {
        tlsUnit_->Close();
    }
    tlsSecurityUpdatedEvent_.Close();
}

AsyncOperationSPtr IpcServer::BeginRequest(
    MessageUPtr && request,
    wstring const & client,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    DWORD clientProcessId;
    ISendTarget::SPtr sendTarget = localUnit_.clientTable_->GetSendTarget(client, clientProcessId);
    if (sendTarget)
    {
        OnSending(request);
        auto operation = localUnit_.requestReply_.BeginRequest(move(request), sendTarget, timeout, callback, parent);
        return operation;
    }

    if (tlsUnit_)
    {
        sendTarget = tlsUnit_->clientTable_->GetSendTarget(client, clientProcessId);
        if (sendTarget)
        {
            OnSending(request);
            return tlsUnit_->requestReply_.BeginRequest<bool>(move(request), sendTarget, true, timeout, callback, parent);
        }
    }

    ipcTrace.ServerCannotSendToUnknownClient(traceId_, client, request->TraceId(), request->Actor, request->Action);
    return localUnit_.requestReply_.BeginRequest(move(request), sendTarget, timeout, callback, parent);
}

ErrorCode IpcServer::EndRequest(AsyncOperationSPtr const & operation, MessageUPtr & reply)
{
    auto ctx = operation->PopOperationContext<bool>();
    if (ctx && ctx->Context)
    {
        return tlsUnit_->requestReply_.EndRequest(operation, reply);
    }

    return localUnit_.requestReply_.EndRequest(operation, reply);
}

Common::ErrorCode IpcServer::SendOneWay(wstring const & client, MessageUPtr && message, TimeSpan expiration)
{
    DWORD clientProcessId;
    ISendTarget::SPtr sendTarget = localUnit_.clientTable_->GetSendTarget(client, clientProcessId);
    if (sendTarget)
    {
        OnSending(message);
        auto error = localUnit_.transport_->SendOneWay(sendTarget, move(message), expiration);
        if (!error.IsSuccess())
        {
            ipcTrace.ServerFailedToSend(traceId_, client, clientProcessId);
        }

        return error;
    }

    if (tlsUnit_)
    {
        sendTarget = tlsUnit_->clientTable_->GetSendTarget(client, clientProcessId);
        if (sendTarget)
        {
            OnSending(message);
            auto error = tlsUnit_->transport_->SendOneWay(sendTarget, move(message), expiration);
            if (!error.IsSuccess())
            {
                ipcTrace.ServerFailedToSend(traceId_, client, clientProcessId);
            }

            return error;
        }
    }

    ipcTrace.ServerCannotSendToUnknownClient(traceId_, client, message->TraceId(), message->Actor, message->Action);
    return ErrorCodeValue::NotFound;
}

void IpcServer::OnSending(MessageUPtr & message) const
{
    if (message->MessageId.IsEmpty())
    {
        message->Headers.Add(MessageIdHeader());
    }
}

void IpcServer::RemoveClient(wstring const & client)
{
    if (localUnit_.clientTable_->Remove(client)) return;

    if (tlsUnit_) tlsUnit_->clientTable_->Remove(client);
}

ErrorCode IpcServer::SetSecurity(Transport::SecuritySettings const & value)
{
    auto copy = value;
    copy.SetDefaultRemoteRole(RoleMask::User); //when client role is not enabled, all clients are treated as non-admin client for message size limit checking
    return localUnit_.transport_->SetSecurity(copy);
}

Transport::SecuritySettings const & IpcServer::get_SecuritySettings() const
{
    return localUnit_.transport_->Security()->Settings();
}

ErrorCode IpcServer::SetSecurityTls(Transport::SecuritySettings const & value)
{
    Invariant(tlsUnit_);

    if (value.SecurityProvider() != SecurityProvider::Ssl)
    {
        WriteError(TraceType, traceId_, "SetSecurityTls: invalid security provider: {0}", value.SecurityProvider());
        return ErrorCodeValue::InvalidArgument;
    }

    auto error = tlsUnit_->transport_->SetSecurity(value);
    OnTlsSecuritySettingsUpdated();
    return error;
}

SecuritySettings const & IpcServer::get_SecuritySettingsTls() const
{
    Invariant(tlsUnit_);
    return tlsUnit_->transport_->Security()->Settings();
}

Common::Thumbprint const & IpcServer::get_ServerCertThumbprintTls() const
{
    Invariant(tlsUnit_);
    return tlsUnit_->transport_->Security()->Credentails()->X509CertThumbprint();
}

void IpcServer::SetMaxIncomingMessageSize(uint value)
{
    localUnit_.transport_->SetMaxIncomingFrameSize(value);
    if (tlsUnit_)
    {
        tlsUnit_->transport_->SetMaxIncomingFrameSize(value);
    }
}

void IpcServer::DisableIncomingMessageSizeLimit()
{
    SetMaxIncomingMessageSize((uint)TcpFrameHeader::FrameSizeHardLimit());
}

HHandler IpcServer::RegisterTlsSecuritySettingsUpdatedEvent(EventHandler const & handler)
{
    return tlsSecurityUpdatedEvent_.Add(handler);
}

bool IpcServer::UnRegisterTlsSecuritySettingsUpdatedEvent(HHandler hHandler)
{
    return tlsSecurityUpdatedEvent_.Remove(hHandler);
}

void IpcServer::OnTlsSecuritySettingsUpdated()
{
    tlsSecurityUpdatedEvent_.Fire(EventArgs());
}

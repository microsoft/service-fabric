// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Client;
using namespace ClientServerTransport;
using namespace ServiceModel;

static const Common::StringLiteral TraceType("ClientServerTcpTransport");

#define STATE_CHECK_ASSERT() TESTASSERT_IF(                                                                        \
    (this->State.Value == FabricComponentState::Created) || (this->State.Value == FabricComponentState::Opening),  \
    "Attempt to access ClientServerTcpTransport when it is not opened")

ClientServerTcpTransport::ClientServerTcpTransport(
    Common::ComponentRoot const &root,
    std::wstring const &traceContext,
    std::wstring const &owner,
    int maxMessageSize,
    TimeSpan keepAliveInterval,
    TimeSpan connectionIdleTimeout)
    : ClientServerTransport(root)
    , traceContext_(traceContext)
    , keepAliveInterval_(keepAliveInterval)
    , connectionIdleTimeout_(connectionIdleTimeout)
    , externalTransport_(false)
{
    transport_ = DatagramTransportFactory::CreateTcpClient(traceContext_, owner);
    transport_->SetMaxOutgoingFrameSize(maxMessageSize);
    //No need to set max incoming message size as client trusts authenticated server completely

    demuxer_ = make_unique<Transport::Demuxer>(root, transport_);
    requestReply_ = make_shared<Transport::DuplexRequestReply>(root, transport_);
    demuxer_->SetReplyHandler(*(requestReply_));

    WriteInfo(
        TraceType,
        traceContext_,
        "Created for Owner:{0}",
        owner);
}

ClientServerTcpTransport::ClientServerTcpTransport(
    Common::ComponentRoot const &root,
    std::wstring const &traceContext,
    Transport::IDatagramTransportSPtr const &transport)
    : ClientServerTransport(root)
    , traceContext_(traceContext)
    , transport_(transport)
    , demuxer_(nullptr)
    , requestReply_(nullptr)
    , externalTransport_(true)
{
}

ErrorCode ClientServerTcpTransport::OnOpen()
{
    if (requestReply_) requestReply_->Open();

    if (externalTransport_) { return ErrorCodeValue::Success; }

    auto error = demuxer_->Open();
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceType,
            traceContext_,
            "Demuxer::Open failed",
            error);
        return error;
    }

    transport_->SetConnectionIdleTimeout(connectionIdleTimeout_);

    transport_->SetKeepAliveTimeout(keepAliveInterval_);

    error = transport_->Start();
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceType,
            traceContext_,
            "Transport::Start failed",
            error);
    }

    return error;
}

ErrorCode ClientServerTcpTransport::OnClose()
{
    if (externalTransport_) { return ErrorCodeValue::Success; }

    transport_->Stop();

    if (demuxer_)
    {
        auto error = demuxer_->Close();
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                traceContext_,
                "Demuxer::Close failed",
                error);
            return error;
        }
    }

    if (requestReply_)
    {
        requestReply_->Close();
    }

    return ErrorCodeValue::Success;
}

void ClientServerTcpTransport::OnAbort()
{
    if (externalTransport_) { return; }

    demuxer_->Abort();
    requestReply_->Close();
}

void ClientServerTcpTransport::SetConnectionFaultHandler(IDatagramTransport::ConnectionFaultHandler const & handler)
{
    transport_->SetConnectionFaultHandler(handler);
}

void ClientServerTcpTransport::RemoveConnectionFaultHandler()
{
    transport_->RemoveConnectionFaultHandler();
}

ErrorCode ClientServerTcpTransport::SetSecurity(SecuritySettings const & securitySettings)
{
    return transport_->SetSecurity(securitySettings);
}

ErrorCode ClientServerTcpTransport::SetKeepAliveTimeout(TimeSpan const &keepAlive)
{
    transport_->SetKeepAliveTimeout(keepAlive);
    return ErrorCodeValue::Success;
}

ISendTarget::SPtr ClientServerTcpTransport::ResolveTarget(
    wstring const & address,
    wstring const & targetId,
    uint64 instance)
{
    STATE_CHECK_ASSERT();
    return transport_->ResolveTarget(address, targetId, instance);
}

ISendTarget::SPtr ClientServerTcpTransport::ResolveTarget(
    wstring const &address,
    wstring const &targetId,
    wstring const & sspiTarget)
{
    STATE_CHECK_ASSERT();
    return transport_->ResolveTarget(address, targetId, sspiTarget);
}

void ClientServerTcpTransport::RegisterMessageHandler(
    Actor::Enum actor,
    Demuxer::MessageHandler const & messageHandler,
    bool dispatchOnTransportThread)
{
    STATE_CHECK_ASSERT();
    ASSERT_IF(demuxer_ == nullptr, "Demuxer not set")

    demuxer_->RegisterMessageHandler(actor, messageHandler, dispatchOnTransportThread);
}

void ClientServerTcpTransport::UnregisterMessageHandler(Actor::Enum actor)
{
    STATE_CHECK_ASSERT();
    ASSERT_IF(demuxer_ == nullptr, "Demuxer not set")

    demuxer_->UnregisterMessageHandler(actor);
}

AsyncOperationSPtr ClientServerTcpTransport::BeginRequestReply(
    ClientServerRequestMessageUPtr && request,
    ISendTarget::SPtr const &to,
    TransportPriority::Enum priority,
    TimeSpan timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    STATE_CHECK_ASSERT();
    ASSERT_IF(requestReply_ == nullptr, "RequestReply not set")

    return requestReply_->BeginRequest(request->GetTcpMessage(), to, priority, timeout, callback, parent);
}

ErrorCode ClientServerTcpTransport::EndRequestReply(
    AsyncOperationSPtr const &operation,
    ClientServerReplyMessageUPtr &reply)
{
    MessageUPtr replyMessage;

    ErrorCode error = requestReply_->EndRequest(operation, replyMessage);
    if (error.IsSuccess())
    {
        reply = move(make_unique<ClientServerReplyMessage>(move(replyMessage)));
    }

    return error;
}

ErrorCode ClientServerTcpTransport::SendOneWay(
    ISendTarget::SPtr const & target,
    ClientServerRequestMessageUPtr && message,
    TimeSpan expiration,
    TransportPriority::Enum priority)
{
    STATE_CHECK_ASSERT();
    ASSERT_IF(transport_ == nullptr, "Transport is not set")

    return transport_->SendOneWay(target, message->GetTcpMessage(), expiration, priority);
}

void ClientServerTcpTransport::SetNotificationHandler(Transport::DuplexRequestReply::NotificationHandler const &handler, bool dispatchOnTransportThread)
{
    requestReply_->SetNotificationHandler(handler, dispatchOnTransportThread);
}

void ClientServerTcpTransport::RemoveNotificationHandler()
{
    requestReply_->RemoveNotificationHandler();
}

void ClientServerTcpTransport::SetClaimsRetrievalHandler(TransportSecurity::ClaimsRetrievalHandler const & handler)
{
    transport_->SetClaimsRetrievalHandler(handler);
}

void ClientServerTcpTransport::RemoveClaimsRetrievalHandler()
{
    transport_->RemoveClaimsRetrievalHandler();
}

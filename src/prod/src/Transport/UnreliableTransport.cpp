// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

UnreliableTransportEventSource const UnreliableTransportEventSource::Trace;

UnreliableTransport::UnreliableTransport(ComponentRoot const & root, shared_ptr<IDatagramTransport> innerTransport)
: RootedObject(root),
innerTransport_(innerTransport),
config_(UnreliableTransportConfig::GetConfig())
{
    ASSERT_IF(!this->innerTransport_, "the inner transport is invalid");
}

UnreliableTransport::~UnreliableTransport()
{
}

ErrorCode UnreliableTransport::Start(bool completeStart)
{
    return this->innerTransport_->Start(completeStart);
}

ErrorCode UnreliableTransport::CompleteStart()
{
    return this->innerTransport_->CompleteStart();
}

void UnreliableTransport::Stop(TimeSpan timeout)
{
    this->innerTransport_->Stop(timeout);
}

void UnreliableTransport::SetMessageHandler(MessageHandler const & handler)
{
    this->innerTransport_->SetMessageHandler(handler);
}

ISendTarget::SPtr UnreliableTransport::Resolve(
    wstring const & address, wstring const & targetId, wstring const & sspiTarget, uint64 instance)
{
    return this->innerTransport_->Resolve(address, targetId, sspiTarget, instance);
}

void UnreliableTransport::SetConnectionAcceptedHandler(ConnectionAcceptedHandler const & handler)
{
    this->innerTransport_->SetConnectionAcceptedHandler(handler);
}

void UnreliableTransport::RemoveConnectionAcceptedHandler()
{
    this->innerTransport_->RemoveConnectionAcceptedHandler();
}

void UnreliableTransport::SetConnectionFaultHandler(ConnectionFaultHandler const & handler)
{
    this->innerTransport_->SetConnectionFaultHandler(handler);
}

void UnreliableTransport::RemoveConnectionFaultHandler()
{
    this->innerTransport_->RemoveConnectionFaultHandler();
}

IDatagramTransport::DisconnectHHandler UnreliableTransport::RegisterDisconnectEvent(DisconnectEventHandler eventHandler)
{
    return innerTransport_->RegisterDisconnectEvent(eventHandler);
}

bool UnreliableTransport::UnregisterDisconnectEvent(DisconnectHHandler hHandler)
{
    return innerTransport_->UnregisterDisconnectEvent(hHandler);
}

Common::ErrorCode UnreliableTransport::SendOneWay(
    ISendTarget::SPtr const & target, 
    MessageUPtr && message, 
    TimeSpan expiration,
    TransportPriority::Enum)
{
    // bypass UnreliableTransport in case it is disabled
    if (config_.IsDisabled() || message->Actor == Actor::Ipc)
    {
        // cannot delay/drop IPC handshake messages, which leads to drop of IPC "application" messages,
        // because IPC handshake messages carry client ID and must be delivered ahead of other messages.
        return this->innerTransport_->SendOneWay(target, std::move(message), expiration);
    }

    TimeSpan delay = config_.GetDelay(IdString, target->Id(), *message);

    if (delay == TimeSpan::Zero)
    {
        return this->innerTransport_->SendOneWay(target, std::move(message), expiration);
    }
    else if (delay == TimeSpan::MaxValue)
    {
        UnreliableTransportEventSource::Trace.Drop(innerTransport_->IdString, message->Action, message->TraceId());
        return Common::ErrorCodeValue::Success;
    }

    UnreliableTransportEventSource::Trace.Delay(innerTransport_->IdString, message->Action, message->TraceId(), delay);

    auto root = Root.CreateComponentRoot();        
    auto transport = innerTransport_;
    MoveUPtr<Message> mover(std::move(message));

    Threadpool::Post([this, root, transport, target, mover, expiration]() mutable
    {
        transport->SendOneWay(target, mover.TakeUPtr(), expiration);
    }, 
    delay);

    return Common::ErrorCodeValue::Success;  //simply return success since we don't know the return value of dalayed SendOneWay
}

void UnreliableTransport::AddPartitionIdToMessageProperty(Message & message, Common::Guid const & partitionId)
{
    if (!UnreliableTransportConfig::GetConfig().IsDisabled())
    {
        if (message.TryGetProperty<Common::Guid>(Constants::PartitionIdString.begin()) == nullptr)
        {
            message.AddProperty(partitionId, Constants::PartitionIdString.begin());
        }
    }
}

ErrorCode UnreliableTransport::SetSecurity(SecuritySettings const & securitySetting)
{
    return this->innerTransport_->SetSecurity(securitySetting);
}

ErrorCode UnreliableTransport::SetPerTargetSendQueueLimit(ULONG limitInBytes)
{
    return innerTransport_->SetPerTargetSendQueueLimit(limitInBytes);
}

ErrorCode UnreliableTransport::SetOutgoingMessageExpiration(TimeSpan expiration)
{
    return innerTransport_->SetOutgoingMessageExpiration(expiration);
}

wstring const & UnreliableTransport::get_IdString() const
{
    return innerTransport_->get_IdString();
}

TransportSecuritySPtr UnreliableTransport::Security() const
{
    return innerTransport_->Security();
}

wstring const & UnreliableTransport::ListenAddress() const
{
    return innerTransport_->ListenAddress();
}

void UnreliableTransport::SetInstance(uint64 instance)
{
    innerTransport_->SetInstance(instance);
}

TimeSpan UnreliableTransport::ConnectionIdleTimeout() const
{
    return innerTransport_->ConnectionIdleTimeout();
}

void UnreliableTransport::SetConnectionIdleTimeout(TimeSpan idleTimeout)
{
    innerTransport_->SetConnectionIdleTimeout(idleTimeout);
}

void UnreliableTransport::DisableSecureSessionExpiration()
{
    innerTransport_->DisableSecureSessionExpiration();
}

void UnreliableTransport::DisableThrottle()
{
    innerTransport_->DisableThrottle();
}

void UnreliableTransport::AllowThrottleReplyMessage()
{
    innerTransport_->AllowThrottleReplyMessage();
}

void UnreliableTransport::SetMaxIncomingFrameSize(ULONG limit)
{
    innerTransport_->SetMaxIncomingFrameSize(limit);
}

void UnreliableTransport::SetMaxOutgoingFrameSize(ULONG limit)
{
    innerTransport_->SetMaxOutgoingFrameSize(limit);
}

void UnreliableTransport::SetClaimsRetrievalMetadata(ClaimsRetrievalMetadata && metadata)
{
    innerTransport_->SetClaimsRetrievalMetadata(move(metadata));
}

void UnreliableTransport::SetClaimsRetrievalHandler(TransportSecurity::ClaimsRetrievalHandler const & handler)
{
    innerTransport_->SetClaimsRetrievalHandler(handler);
}

void UnreliableTransport::RemoveClaimsRetrievalHandler()
{
    innerTransport_->RemoveClaimsRetrievalHandler();
}

void UnreliableTransport::SetClaimsHandler(TransportSecurity::ClaimsHandler const & handler)
{
    innerTransport_->SetClaimsHandler(handler);
}

void UnreliableTransport::RemoveClaimsHandler()
{
    innerTransport_->RemoveClaimsHandler();
}

TimeSpan UnreliableTransport::KeepAliveTimeout() const
{
    return innerTransport_->KeepAliveTimeout();
}

void UnreliableTransport::SetKeepAliveTimeout(Common::TimeSpan timeout)
{
    innerTransport_->SetKeepAliveTimeout(timeout);
}

size_t UnreliableTransport::SendTargetCount() const
{
    return innerTransport_->SendTargetCount();
}

void UnreliableTransport::EnableInboundActivityTracing()
{
    innerTransport_->EnableInboundActivityTracing();
}

void UnreliableTransport::Test_Reset()
{
    innerTransport_->Test_Reset();
}

TimeSpan UnreliableTransport::ConnectionOpenTimeout() const
{
    return innerTransport_->ConnectionOpenTimeout();
}

void UnreliableTransport::SetConnectionOpenTimeout(Common::TimeSpan timeout)
{
    innerTransport_->SetConnectionOpenTimeout(timeout);
}

wstring const & UnreliableTransport::TraceId() const
{
    return innerTransport_->TraceId();
}

void UnreliableTransport::SetBufferFactory(unique_ptr<IBufferFactory> && bufferFactory)
{
    innerTransport_->SetBufferFactory(move(bufferFactory));
}

void UnreliableTransport::DisableAllPerMessageTraces()
{
    innerTransport_->DisableAllPerMessageTraces();
}

#ifdef PLATFORM_UNIX

EventLoopPool* UnreliableTransport::EventLoops() const
{
    return innerTransport_->EventLoops();
}

void UnreliableTransport::SetEventLoopPool(EventLoopPool* pool)
{
    innerTransport_->SetEventLoopPool(pool);
}

#endif

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

static Global<ExclusiveLock> transportTableLock = make_global<ExclusiveLock>();
static Global<TransportTable> transportTable = make_global<TransportTable>();

// static
size_t MemoryTransport::TransportCount()
{
    return transportTable->Count();
}

MemoryTransport::MemoryTransport(wstring const & name, std::wstring const & id)
    : started_(false)
    , stopped_(false)
    , traceId_(id.empty()? wformatString("{0}", TraceTransport) : wformatString("{0}-{1}", TraceTransport, id))
    , name_(name)
    , messageHandler_()
    , myTarget_(make_shared<MemorySendTarget>(name, id))
    , messageTableLock_()
    , incomingMessages_()
    , security_(make_shared<TransportSecurity>())
{
    Sockets::Startup();
}

MemoryTransport::~MemoryTransport()
{
    if (!stopped_)
    {
        Stop();
    }

    WriteInfo(Constants::MemoryTrace, "Released: {0} {1}", this->name_, this->get_IdString());
}

ErrorCode MemoryTransport::Start(bool)
{
    // lock the check/add
    AcquireExclusiveLock lock(*transportTableLock);

    // create empty pointer
    shared_ptr<MemoryTransport> transport;
    if (StringUtility::EndsWith<wstring>(this->name_, L":0"))
    {
        Random random;

        this->name_ = this->name_.substr(0, this->name_.size() - 2);
        wstring name;
        do
        {
            int port = random.Next(25536) + 40000;
            name = wformatString("{0}:{1}", this->name_, port);
        } while (transportTable->Contains(name));
        this->name_ = name;
    }
    else if (transportTable->Contains(this->name_))
    {
        WriteWarning(Constants::MemoryTrace, "address already exists: {0}", this->name_);

        return Common::ErrorCode(Common::ErrorCodeValue::AddressAlreadyInUse);
    }

    // set the weak pointer in the send target
    shared_ptr<MemorySendTarget> memorySendTarget = dynamic_pointer_cast<MemorySendTarget>(this->myTarget_);

    auto thisSPtr = this->shared_from_this();

    memorySendTarget->target_ = weak_ptr<MemoryTransport>(thisSPtr);
    memorySendTarget->name_ = this->name_;

    if (this->name_.size() > 0)
    {
        transportTable->Add(this->name_, weak_ptr<MemoryTransport>(thisSPtr));
    }

    WriteInfo(Constants::MemoryTrace, "Started {0} {1}", name_, get_IdString());

    this->started_ = true;

    return ErrorCode::Success();
}

ErrorCode MemoryTransport::CompleteStart()
{
    return ErrorCodeValue::Success;
}

void MemoryTransport::SetMessageHandler(MessageHandler const & handler)
{
    AcquireExclusiveLock lock(*transportTableLock);
    if (!this->stopped_)
    {
        this->messageHandler_ = handler;
        WriteInfo(Constants::MemoryTrace, "Set msg handler {0} {1}", name_, get_IdString());
    }
}

ISendTarget::SPtr MemoryTransport::Resolve(
    wstring const & address, wstring const & targetId, wstring const & sspiTarget, uint64 instance)
{
    instance;
    sspiTarget;

    ASSERT_IFNOT(this->started_, "Transport has not been started");

    WriteNoise(Constants::MemoryTrace, "Resolving: {0}", address);
    wstring effectiveAddress = TargetAddressToTransportAddress(address);

    AcquireExclusiveLock lock(*transportTableLock);

    weak_ptr<IDatagramTransport> item;
    if (transportTable->TryGet(effectiveAddress, item))
    {
        // if we found the transport, return its target
        shared_ptr<IDatagramTransport> transport = item.lock();
        if (transport)
        {
            shared_ptr<MemoryTransport> memoryTarget = dynamic_pointer_cast<MemoryTransport>(transport);
            return memoryTarget->myTarget_;
        }
    }

    auto memoryTarget =  make_shared<MemorySendTarget>(targetId, targetId);
    memoryTarget->name_ = effectiveAddress;
    return memoryTarget;
}

Common::ErrorCode MemoryTransport::SendOneWay(
    ISendTarget::SPtr const & target, 
    MessageUPtr && message, 
    TimeSpan,
    TransportPriority::Enum)
{
    ASSERT_IFNOT(this->started_, "Transport has not been started");

    shared_ptr<MemorySendTarget> memorySendTarget = dynamic_pointer_cast<MemorySendTarget>(target);
    shared_ptr<MemoryTransport> lockedTarget = memorySendTarget->target_.lock();
    ConnectionFaultHandler faultHandler = nullptr;

    if(!lockedTarget || lockedTarget->stopped_)
    {
        AcquireExclusiveLock lock(*transportTableLock);

        weak_ptr<IDatagramTransport> item;
        bool foundTransport = false;
        if (transportTable->TryGet(memorySendTarget->name_, item))
        {
            shared_ptr<IDatagramTransport> transport = item.lock();
            if (transport)
            {
                // Update the weak_ptr to the valid one
                memorySendTarget->target_ = weak_ptr<MemoryTransport>(dynamic_pointer_cast<MemoryTransport>(transport));

                lockedTarget = memorySendTarget->target_.lock();
                foundTransport = true;
            }
        }

        if(!foundTransport)
        {
            lockedTarget = nullptr;
            WriteWarning(Constants::MemoryTrace, "target {0} does not exist, dropping message {1}, Actor = {2}, Action = '{3}'", 
                memorySendTarget->name_, message->TraceId(), message->Actor, message->Action);
            faultHandler = faultHandler_;
        }
    }

    if (!lockedTarget)
    {
        if (faultHandler)
        {
            faultHandler(*memorySendTarget, ErrorCodeValue::OperationFailed);
        }
        disconnectEvent_.Fire(DisconnectEventArgs(memorySendTarget.get(), ErrorCodeValue::OperationFailed));
        return ErrorCodeValue::OperationFailed;
    }

    // the target of the message will add the message into it's queue for processing and start a thread if needed.  Pass myTarget_ so the receiver knows who sent it
    ASSERT_IFNOT(this->myTarget_, "Send target is not set");
    lockedTarget->EnqueueMessage(lockedTarget, make_unique<Message>(*message), this->myTarget_);

    message = nullptr;

    return ErrorCodeValue::Success;
}

void MemoryTransport::HandleMessage(MessageUPtr && message, ISendTarget::SPtr const & sender)
{
    ASSERT_IFNOT(this->started_, "Transport has not been started");

    // incoming message, handle it with default handler
    MessageHandler tempHandler;
    {
        AcquireReadLock grab(*transportTableLock);
        tempHandler = messageHandler_;
    }

    if (tempHandler)
    {
        tempHandler(message, sender);
    }
    else
    {
        // TODO: handle message dropping
        WriteWarning(Constants::MemoryTrace, "null handler, dropping message {0}, Actor = {1}, Action = '{2}' stopped={3}", message->TraceId(), message->Actor, message->Action, this->stopped_);
    }
}

// place message in target transport's queue and invoke a thread to drain it
void MemoryTransport::EnqueueMessage(shared_ptr<MemoryTransport> & transport, MessageUPtr && message, ISendTarget::SPtr const & sender)
{
    ASSERT_IFNOT(this->started_, "Transport has not been started");

    WriteNoise(Constants::MemoryTrace, traceId_, "enqueue message {0}", message->TraceId());

    AcquireExclusiveLock lock(this->messageTableLock_);

    this->incomingMessages_.push(std::make_pair(std::move(message), sender));

    if (this->incomingMessages_.size() == 1)
    {
        // = will copy the shared ptr by ref to keep it alive
        Common::Threadpool::Post([=]() { transport->PumpIncomingMessages();});
    }
    else
    {
        WriteNoise(Constants::MemoryTrace, traceId_, "message pump already running");
    }
}

// while the queue has messages, dispatch them 1 at a time and exit when it is empty
void MemoryTransport::PumpIncomingMessages()
{
    ASSERT_IFNOT(this->started_, "Transport has not been started");

    WriteNoise(Constants::MemoryTrace, traceId_, "starting message pump");

    for( ;; )
    {
        MessageUPtr message;
        ISendTarget::SPtr target;

        {   // scope the lock
            AcquireExclusiveLock lock(this->messageTableLock_);

            if (this->incomingMessages_.empty())
            {
                WriteNoise(Constants::MemoryTrace, traceId_, "leaving message pump");
                break;
            }

            message = std::move(this->incomingMessages_.front().first);
            target = std::move(this->incomingMessages_.front().second);

            this->incomingMessages_.pop();
        }

        // dispatch outside lock
        this->HandleMessage(std::move(message), target);
    }
}

void MemoryTransport::SetConnectionAcceptedHandler(ConnectionAcceptedHandler const & handler)
{
    UNREFERENCED_PARAMETER(handler);
}

void MemoryTransport::RemoveConnectionAcceptedHandler()
{
}

IDatagramTransport::DisconnectHHandler MemoryTransport::RegisterDisconnectEvent(DisconnectEventHandler eventHandler)
{
    AcquireExclusiveLock lock(*transportTableLock);

    if (stopped_) return DisconnectEvent::InvalidHHandler;

    return disconnectEvent_.Add(eventHandler);
}

bool MemoryTransport::UnregisterDisconnectEvent(DisconnectHHandler hHandler)
{
    AcquireExclusiveLock lock(*transportTableLock);
    return disconnectEvent_.Remove(hHandler);
}

void MemoryTransport::SetConnectionFaultHandler(ConnectionFaultHandler const & handler)
{
    AcquireExclusiveLock lock(*transportTableLock);
    if (!this->stopped_)
    {
        faultHandler_ = handler;
        WriteInfo(Constants::MemoryTrace, "Set fault handler {0} {1}", name_, get_IdString());
    }
}

void MemoryTransport::RemoveConnectionFaultHandler()
{
    AcquireExclusiveLock lock(*transportTableLock);
    faultHandler_ = nullptr;
}

ErrorCode MemoryTransport::SetPerTargetSendQueueLimit(ULONG limitInBytes)
{
    limitInBytes;
    return ErrorCode();
}

ErrorCode MemoryTransport::SetOutgoingMessageExpiration(Common::TimeSpan expiration)
{
    expiration;
    return ErrorCode();
}

wstring const & MemoryTransport::ListenAddress() const
{
    return name_;
}

wstring const & MemoryTransport::get_IdString() const
{
    return myTarget_->Id();
}

void MemoryTransport::Stop(TimeSpan)
{
    AcquireExclusiveLock lock(*transportTableLock);

    messageHandler_ = nullptr;
    disconnectEvent_.Close();
    faultHandler_ = nullptr;

    if (this->started_ && !this->stopped_ && name_.size() > 0)
    {
        transportTable->Remove(this->name_);
    }

    this->stopped_ = true;
    WriteInfo(Constants::MemoryTrace, "Stop {0} {1}", this->name_, get_IdString());
}

TransportSecuritySPtr MemoryTransport::Security() const
{
    return security_;
}

ErrorCode MemoryTransport::SetSecurity(SecuritySettings const &)
{
    return ErrorCode::Success();
}

void MemoryTransport::SetInstance(uint64)
{
}

TimeSpan MemoryTransport::ConnectionIdleTimeout() const
{
    return TimeSpan::MaxValue;
}

void MemoryTransport::SetConnectionIdleTimeout(Common::TimeSpan)
{
}

void MemoryTransport::DisableSecureSessionExpiration() 
{
}

void MemoryTransport::DisableThrottle() 
{
}

void MemoryTransport::AllowThrottleReplyMessage() 
{
}

void MemoryTransport::SetClaimsHandler(TransportSecurity::ClaimsHandler const &)
{
}

void MemoryTransport::RemoveClaimsHandler()
{
}

TimeSpan MemoryTransport::KeepAliveTimeout() const
{
    return TimeSpan::Zero;
}

void MemoryTransport::SetKeepAliveTimeout(Common::TimeSpan)
{
}

size_t MemoryTransport::SendTargetCount() const
{
    return 0;
}

void MemoryTransport::EnableInboundActivityTracing()
{
}

TimeSpan MemoryTransport::ConnectionOpenTimeout() const
{
    return TimeSpan::Zero;
}

void MemoryTransport::SetConnectionOpenTimeout(Common::TimeSpan)
{
}

wstring const & MemoryTransport::TraceId() const
{
    return traceId_;
}

void MemoryTransport::SetBufferFactory(unique_ptr<IBufferFactory> &&)
{
}

void MemoryTransport::DisableAllPerMessageTraces()
{
}

#ifdef PLATFORM_UNIX

EventLoopPool* MemoryTransport::EventLoops() const
{
    return nullptr;
}

void MemoryTransport::SetEventLoopPool(EventLoopPool*)
{
}

#endif

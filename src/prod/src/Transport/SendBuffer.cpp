// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include"stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

namespace
{
    const StringLiteral TraceType("SendBuf");
    atomic_uint64 frameCount(0);
}

SendBuffer::SendBuffer(TcpConnection* connectionPtr)
    : connection_(connectionPtr)
    , sendBatchLimitInBytes_(TransportConfig::GetConfig().SendBatchSizeLimit)
{
    preparedBuffers_.reserve(1024 * 4);
    KAssert(connection_->target_ != nullptr);
    transportPtr_ = ((TcpSendTarget*)connection_->target_.get())->Owner().get();
    if (transportPtr_ != nullptr)
    {
        transportId_ = transportPtr_->TraceId();
        transportOwner_ = transportPtr_->Owner();
    }
}

void SendBuffer::DelayEncryptEnqueue()
{
    canEnqueueEncrypt_ = false;
}

void SendBuffer::EnableEncryptEnqueue()
{
    canEnqueueEncrypt_ = true;
}

void SendBuffer::SetLimit(ULONG limitInBytes)
{
    limitInBytes_ = limitInBytes ? limitInBytes : std::numeric_limits<decltype(limitInBytes_)>::max();
}

void SendBuffer::SetSecurityProviderMask(SecurityProvider::Enum securityProvider)
{
    securityProviderMask_ = SecurityProvider::EnumToMask(securityProvider);
    shouldAddSecNegoHeader_ = (securityProviderMask_ != SecurityProvider::None);
}

#ifdef PLATFORM_UNIX

uint SendBuffer::TotalPreparedBytes() const
{
    return sendingLength_;
}

uint SendBuffer::FirstBufferToSend() const
{
    return firstBufferToSend_;
}

bool SendBuffer::ConsumePreparedBuffers(ssize_t sent)
{
    for(uint i = firstBufferToSend_; i < preparedBuffers_.size(); ++i)
    {
        if (sent == preparedBuffers_[i].size()) 
        {
            if ((i+1) == preparedBuffers_.size()) return true; // all prepared buffers have been sent

            firstBufferToSend_ = i+1;
            return false;
        }
        else if (sent < preparedBuffers_[i].size())
        {
            preparedBuffers_[i] += sent;
            firstBufferToSend_ = i;
            return false;
        }

        sent -= preparedBuffers_[i].size();
    }

    return false;
}

#endif

SendBuffer::Buffers const & SendBuffer::PreparedBuffers() const
{
    return preparedBuffers_;
}

ULONG SendBuffer::BytesPendingForSend() const
{
    return (ULONG)totalBufferedBytes_;
}

ErrorCode SendBuffer::TrackMessageIdIfNeeded(MessageId const & messageId)
{
    if (connection_->GetTransportFlags().IsDuplicateDetectionSet() && !messageId.IsEmpty())
    {
        if (!messageIdTable_)
        {
            messageIdTable_ = make_unique<MessageIdHashSet>();
        }

        auto result = messageIdTable_->insert(messageId);

        if (!result.second)
        {
            return ErrorCodeValue::DuplicateMessage;
        }
    }

    return ErrorCodeValue::Success;
}

void SendBuffer::UntrackMessageIdIfNeeded(MessageId const & messageId)
{
    if (connection_->GetTransportFlags().IsDuplicateDetectionSet() && !messageId.IsEmpty())
    {
        if (messageIdTable_)
        {
            messageIdTable_->erase(messageId);
        }
    }
}

void SendBuffer::EnqueueMessagesDelayedBySecurityNegotiation(unique_ptr<Message> && claimsMessage)
{
    KAssert(messagesDelayedBySecurityNegotiation_.size() == expirationOfDelayedMessages_.size());

    // Although AAD re-uses some of the DSTS "claims" framework, the metadata retrieval is different.
    //
    // For DSTS, the claims token is obtained out-of-band and applied to the transport security
    // settings. The transport connection enqueues the claims message upfront during initialization
    // (TcpConnection::Initialize). The claims retrieval handler will not fire.
    //
    // For AAD, the claims retrieval handler will fire as part of secure connection negotiation.
    // The claims message is only available upon completion of the handler. If a claims message
    // was already enqueued during connection initialization, then it should be replaced.
    //
    size_t enqueueStartIndex = 0;

    if (claimsMessage)
    {
        if (!messagesDelayedBySecurityNegotiation_.empty())
        {
            auto const & firstMessage = messagesDelayedBySecurityNegotiation_[0];

            ActionHeader header;
            if (firstMessage->Headers.TryReadFirst(header))
            {
                if (header.Action == Constants::ClaimsMessageAction)
                {
                    enqueueStartIndex = 1;
                } 
            }
        }

        Enqueue(move(claimsMessage), TimeSpan::MaxValue, true);
    }

    for (size_t i = enqueueStartIndex; i < messagesDelayedBySecurityNegotiation_.size(); ++i)
    {
        Enqueue(
            move(messagesDelayedBySecurityNegotiation_[i]),
            expirationOfDelayedMessages_[i],
            true);
    }

    messagesDelayedBySecurityNegotiation_.clear();
    expirationOfDelayedMessages_.clear();
    totalDelayedBytes_ = 0;
    EnableEncryptEnqueue();
}

ErrorCode SendBuffer::EnqueueMessage(MessageUPtr && message, TimeSpan expiration, bool shouldEncrypt)
{
    if (firstEnqueueCall_)
    {
        firstEnqueueCall_ = false; 
        BeforeFirstEnqueue(shouldEncrypt);
    }

    size_t bytesToAdd = message->SerializedSize() + sizeof(TcpFrameHeader);
    if (shouldEncrypt && !canEnqueueEncrypt_)
    {
        size_t delayedBytesAfter = totalDelayedBytes_ + bytesToAdd;
        if ((limitInBytes_ < delayedBytesAfter) && 
            //when frame size limit is disabled, ignore queue limit on empty queue to allow one large message
            (!connection_->OutgoingFrameSizeLimitDisabled() ||
            totalDelayedBytes_)) 
        {
            trace.SendQueueFull(
                connection_->TraceId(),
                connection_->localAddress_,
                connection_->targetAddress_,
                messagesDelayedBySecurityNegotiation_.size(),
                totalDelayedBytes_,
                limitInBytes_,
                message->TraceId(),
                message->SerializedSize(),
                message->Actor,
                message->Action);

            message->OnSendStatus(ErrorCodeValue::TransportSendQueueFull, move(message));

            return ErrorCodeValue::TransportSendQueueFull;
        }

        trace.SecurityNegotiationOngoing(connection_->TraceId(), message->TraceId());
        totalDelayedBytes_ = delayedBytesAfter;
        messagesDelayedBySecurityNegotiation_.emplace_back(move(message));
        expirationOfDelayedMessages_.push_back(expiration);
        return ErrorCode();
    }

    size_t bufferedBytesAfter = totalBufferedBytes_ + bytesToAdd;
    if ((bufferedBytesAfter > limitInBytes_) &&
        //when frame size limit is disabled, ignore queue limit on empty queue to allow one large message
        (!connection_->OutgoingFrameSizeLimitDisabled() ||
        totalBufferedBytes_))
    {
        trace.SendQueueFull(
            connection_->TraceId(),
            connection_->localAddress_,
            connection_->targetAddress_,
            MessageCount(),
            totalBufferedBytes_,
            limitInBytes_,
            message->TraceId(),
            message->SerializedSize(),
            message->Actor,
            message->Action);

        message->OnSendStatus(ErrorCodeValue::TransportSendQueueFull, move(message));

        return ErrorCodeValue::TransportSendQueueFull;
    }

    return Enqueue(move(message), expiration, shouldEncrypt);
}

ErrorCode SendBuffer::Enqueue(MessageUPtr && message, TimeSpan expiration, bool shouldEncrypt)
{
    if (shouldAddSecNegoHeader_)
    {
        shouldAddSecNegoHeader_ = false;
        connection_->securityContext_->AddSecurityNegotiationHeader(*message);
    }

    if (shouldEncrypt && shouldAddVerificationHeaders_)
    {
        shouldAddVerificationHeaders_ = false;
        connection_->securityContext_->AddVerificationHeaders(*message);
    }

    auto error = this->TrackMessageIdIfNeeded(message->MessageId);
    if (!error.IsSuccess())
    {
        message->OnSendStatus(error, move(message));
        return error;
    }

    EnqueueImpl(move(message), expiration, shouldEncrypt);

    if ((uint64)totalBufferedBytes_ > queuedBytesMax_)
    {
        // NOTE: frame header size is not counted in queuedBytesMax_, which is fine for tracing purpose
        queuedBytesMax_ = totalBufferedBytes_;
        trace.SendQueueSizeMaxIncreased(connection_->TraceId(), transportId_, transportOwner_, queuedBytesMax_);
    }

    return ErrorCodeValue::Success;
}

bool SendBuffer::HasNoMessageDelayedBySecurityNeogtiation() const
{
    return messagesDelayedBySecurityNegotiation_.empty();
}

void SendBuffer::SetPerfCounters(PerfCountersSPtr const & perfCounters)
{
    perfCounters_ = perfCounters;
}

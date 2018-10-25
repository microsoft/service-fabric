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
    const StringLiteral TraceType("LeaseSendBuf");

    atomic_uint64 frameCount(0);

    const static size_t FrameQueueBiqueChunkSize = (1024 * 32) / sizeof(LTSendBuffer::Frame);
    const static size_t SendBatchBufferCountLimit = 1024; // writev on Linux limit buffer count to 1024
}

LTSendBuffer::Frame::Frame(
    MessageUPtr && message,
    byte,
    TimeSpan expiration,
    bool shouldEncrypt)
: message_(std::move(message))
, messageId_(message_->SerializedBodySize()? LTMessageHeader::GetMessageId(message_->BeginBodyChunks()->cbegin(), message_->SerializedBodySize()) : 0)
, shouldEncrypt_(shouldEncrypt)
, preparedForSending_(false)
{
    if (message_->Actor == Actor::SecurityContext)
    {
        auto fs = message_->SerializedBodySize();
        Invariant(fs < numeric_limits<uint>::max());
        header_.FrameSize = (uint)fs;
        header_.FrameType = 0; // sec nego message, frame will not get on wire
    }
    else
    {
        auto fs = message_->SerializedBodySize();
        fs += sizeof(header_);
        Invariant(message_->SerializedBodySize() < fs);
        Invariant(fs < numeric_limits<uint>::max());
        header_.SetFrameSize((uint)fs);
    }

    auto count = ++frameCount;
    TcpConnection::WriteNoise(
        TraceType,
        "Frame ctor: count = {0}, FrameType = {1}, FrameSize = {2}",
        count, header_.FrameType, header_.FrameSize);

    if (expiration == TimeSpan::MaxValue)
    {
        expiration_ = StopwatchTime::MaxValue;
        return;
    }

    StopwatchTime now = Stopwatch::Now();
    expiration_ = now + expiration;
    if (expiration_ < now) // overflow?
    {
        expiration_ = StopwatchTime::MaxValue;
    }
}

LTSendBuffer::Frame::~Frame()
{
    auto count = --frameCount;
    TcpConnection::WriteNoise(TraceType, "Frame dtor: count = {0}", count);
}

size_t LTSendBuffer::Frame::FrameLength() const
{
#ifdef PLATFORM_UNIX
    if (encrypted_.empty())
    {
        return header_.FrameSize;
    }

    Invariant(shouldEncrypt_);
    return encrypted_.size();
#else
    return header_.FrameSize;
#endif
}

MessageUPtr LTSendBuffer::Frame::Dispose()
{
    auto message = move(message_);
    Frame::~Frame();
    return message; 
}

MessageUPtr const & LTSendBuffer::Frame::Message() const
{
    return message_;
}

bool LTSendBuffer::Frame::HasExpired(StopwatchTime now) const
{
    return !preparedForSending_ && expiration_ <= now;
}

bool LTSendBuffer::Frame::IsInUse() const
{
    return preparedForSending_;
}

ErrorCode LTSendBuffer::Frame::EncryptIfNeeded(LTSendBuffer & sendBuffer)
{
    if (!shouldEncrypt_)
    {
        return ErrorCode();
    }

#ifdef PLATFORM_UNIX
    auto securityContext = sendBuffer.connection_->securityContext_.get();
    Invariant(securityContext->TransportSecurity().SecurityProvider == SecurityProvider::Ssl);
    auto securityContextSsl = (SecurityContextSsl*)securityContext;
    
    //LINUXTODO avoid data copying during encryption
    auto reserveSize = sizeof(header_) + message_->SerializedBodySize();
    ByteBuffer2 buffer(reserveSize);
    TcpConnection::WriteNoise(
        TraceType, sendBuffer.connection_->TraceId(),
        "Encrypt: {0}, plaintext length (including frame header) = {1}",
        message_->TraceId(), buffer.size());

    buffer.append(&header_, sizeof(header_));

    for (BufferIterator chunk = message_->BeginBodyChunks(); chunk != message_->EndBodyChunks(); ++chunk)
    {
        buffer.append(chunk->cbegin(), chunk->size());
    }

    Invariant(buffer.size() == reserveSize); 

    auto error = securityContextSsl->Encrypt(buffer.data(), buffer.size());
    if (!error.IsSuccess()) return error;

    encrypted_ = securityContextSsl->EncryptFinal();
   
    // adjust for size change due to encryption
    auto bufferedBefore = sendBuffer.totalBufferedBytes_; 
    sendBuffer.totalBufferedBytes_ -= buffer.size(); 
    sendBuffer.totalBufferedBytes_ += encrypted_.size(); 
    TcpConnection::WriteNoise(
        TraceType, sendBuffer.connection_->TraceId(),
        "Encrypt: plain = {0}, encrypted = {1}, totalBufferedBytes_: before = {2}, after = {3}",
        buffer.size(), encrypted_.size(), bufferedBefore, sendBuffer.totalBufferedBytes_);
 
    return error;
#else
    sendBuffer;
    Assert::CodingError("not implemented");
#endif
}

ErrorCode LTSendBuffer::Frame::PrepareForSending(LTSendBuffer & sendBuffer)
{
    Invariant(!preparedForSending_);
    preparedForSending_ = true;

    ErrorCode error = EncryptIfNeeded(sendBuffer);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (!encrypted_.empty())
    {
        Invariant(shouldEncrypt_);
        sendBuffer.preparedBuffers_.emplace_back(ConstBuffer(encrypted_.data(), encrypted_.size()));
        sendBuffer.sendingLength_ += encrypted_.size();
        return error;
    }

    // add frame header
    if (message_->Actor != Actor::SecurityContext)
    {
        sendBuffer.preparedBuffers_.emplace_back(ConstBuffer(&header_, sizeof(header_)));
        sendBuffer.sendingLength_ += sizeof(header_);
    }

    // add message body
    for (BufferIterator chunk = message_->BeginBodyChunks(); chunk != message_->EndBodyChunks(); ++chunk)
    {
        if (chunk->size() == 0) continue;

        sendBuffer.preparedBuffers_.emplace_back(ConstBuffer(chunk->cbegin(), chunk->size()));
        sendBuffer.sendingLength_ += chunk->size();
    }

    return error;
}

LTSendBuffer::LTSendBuffer(TcpConnection* connectionPtr)
    : SendBuffer(connectionPtr)
    , messageQueue_(FrameQueueBiqueChunkSize)
{
}

size_t LTSendBuffer::MessageCount() const
{
    return messageQueue_.size();
}

void LTSendBuffer::DropExpiredMessage(Frame & frame)
{
    trace.OutgoingMessageExpired(
        connection_->TraceId(),
        connection_->localAddress_,
        connection_->targetAddress_,
        frame.Message()->TraceId(),
        frame.Message()->Actor,
        frame.Message()->Action);

    totalBufferedBytes_ -= frame.FrameLength();
    KAssert(totalBufferedBytes_ >= 0);
    KAssert(!frame.IsInUse());

    // message Id will be untracked if needed when the
    // corresponding message is actually consumed from
    // the queue in Consume() 

    if (frame.Message()->HasSendStatusCallback())
    {
        auto msg = frame.Dispose();
        msg->OnSendStatus(ErrorCodeValue::MessageExpired, move(msg));
    }
    else
    {
        //no send status callback, so message is not used to keep things alive
        frame.Dispose();
    }
}

ErrorCode LTSendBuffer::Prepare()
{
    sendingLength_ = 0;
    preparedBuffers_.resize(0);
#ifdef PLATFORM_UNIX
    firstBufferToSend_ = 0;
#endif

    StopwatchTime now = Stopwatch::Now();
    ErrorCode error;
    for (auto cur = messageQueue_.begin(); cur != messageQueue_.end(); ++cur)
    {
        // cap large sends
        if ((sendingLength_ >= sendBatchLimitInBytes_) || (preparedBuffers_.size() >= SendBatchBufferCountLimit))
        {
            break;
        }

        if (!cur->Message())
        {
            continue; // message had been dropped due to expiration
        }

        if (cur->HasExpired(now))
        {
            DropExpiredMessage(*cur);
            continue;
        }

        error = cur->PrepareForSending(*this);
        if (!error.IsSuccess())
        {
            //TcpConnection Send Method Acuires lock before calling Prepare on SendBuffer.
            connection_->Close_CallerHoldingLock(true,error);
            return error;
        }
    }

    perfCounters_->AverageTcpSendSizeBase.Increment();
    perfCounters_->AverageTcpSendSize.IncrementBy(sendingLength_);
    return error;
}

void LTSendBuffer::Consume(size_t length)
{
    if (length == 0)
    {
        return;
    }

    ASSERT_IFNOT(totalBufferedBytes_ > 0, "totalBufferedBytes_ = {0}", totalBufferedBytes_);
    ASSERT_IFNOT((size_t)totalBufferedBytes_ >= length, "totalBufferedBytes_ = {0}, length = {1}", totalBufferedBytes_, length);
    ASSERT_IFNOT(length == sendingLength_, "length = {0}, sendingLength_ = {1}", length, sendingLength_);

    FrameQueue::iterator frame = messageQueue_.begin();
    size_t consumedBytes = 0;
    while ((consumedBytes < length) && (frame != messageQueue_.end()))
    {
        if (frame->Message())
        {
            KAssert(frame->IsInUse());
            consumedBytes += frame->FrameLength();
            ++messageSentCount_;

            this->UntrackMessageIdIfNeeded(frame->Message()->MessageId);

            trace.LTDequeue(
                connection_->TraceId(),
                frame->LTMessageId(),
                messageSentCount_);

            if (frame->Message()->HasSendStatusCallback())
            {
                auto msg = frame->Dispose();
                msg->OnSendStatus(ErrorCodeValue::Success, move(msg));
            }
            else
            {
                //no send status callback, so message is not used to keep things alive
                frame->Dispose(); // This is required as bique will not call ~Frame()
            }
        }

        ++frame;
    }

    Invariant(consumedBytes == length);
    messageQueue_.truncate_before(frame);
    sendingLength_ = 0;
    totalBufferedBytes_ -= (ULONG)length;
    sentByteTotal_ += length;
    TcpConnection::WriteNoise(
        TraceType, connection_->TraceId(),
        "leaving Consume: totalBufferedBytes_ = {0}",
        totalBufferedBytes_);
}

void LTSendBuffer::Abort()
{
    if (++this->abortCount_ != 1)
    {
        Common::Assert::CodingError("We are doing multiple aborts: {0}", this->abortCount_.load());
    }

    ErrorCode sendError = connection_->fault_.IsSuccess() ? ErrorCodeValue::OperationCanceled : connection_->fault_;

    const uint dropTraceLimit = 9;
    uint dropCount = 0;
    for (auto & message : messagesDelayedBySecurityNegotiation_)
    {

        if (dropCount++ < dropTraceLimit)
        {
             trace.DropMessageOnAbort(
                message->TraceId(),
                message->Actor,
                message->Action,
                sendError);
        }

        if (message->HasSendStatusCallback())
        {
            message->OnSendStatus(sendError, move(message));
        }
    }

    auto frame = messageQueue_.begin();
    while (frame != messageQueue_.end())
    {
        if (frame->Message())
        {
            if (dropCount++ < dropTraceLimit)
            {
                trace.DropMessageOnAbort(
                    frame->Message()->TraceId(),
                    frame->Message()->Actor,
                    frame->Message()->Action,
                    sendError);
            }

            if (frame->Message()->HasSendStatusCallback())
            {
                auto msg = frame->Dispose();
                msg->OnSendStatus(sendError, move(msg));
            }
            else
            {
                //no send status callback, so message is not used to keep things alive
                frame->Dispose(); // This is required as bique will not call ~Frame()
            }
        }

        ++frame;
    }

    TcpConnection::WriteInfo(
        TraceType, connection_->TraceId(),
        "abort send buffer with {0}, dropping {1} messages",
        sendError,
        dropCount);

    messageQueue_.truncate_before(messageQueue_.cend());
    totalBufferedBytes_ = 0;
    sendingLength_ = 0;
}

bool LTSendBuffer::Empty() const
{
    return messageQueue_.empty();
}

bool LTSendBuffer::PurgeExpiredMessages(StopwatchTime now)
{
    // todo, leikong, consider adding expiration multimap to avoid always going through all messages
    bool purged = false;
    for (auto & frame : messageQueue_)
    {
        if (frame.Message() && frame.HasExpired(now))
        {
            DropExpiredMessage(frame);
            purged = true;
        }
    }

    return purged;
}

void LTSendBuffer::EnqueueImpl(MessageUPtr && message, TimeSpan expiration, bool shouldEncrypt)
{
    if ((message->Actor == Actor::Transport) || (message->Actor == Actor::TransportSendTarget))
    {
        //these actors are only used by non-lease mode
        return;
    }

    uint32 messageCount = MessageCount();
    messageQueue_.emplace_back(move(message), securityProviderMask_, expiration, shouldEncrypt);
    trace.LTEnqueue(
        connection_->TraceId(),
        messageQueue_.back().LTMessageId(),
        messageQueue_.back().Message()->SerializedBodySize(),
        messageCount, 
        (uint32)totalBufferedBytes_);

    totalBufferedBytes_ += messageQueue_.back().FrameLength();
}

void LTSendBuffer::BeforeFirstEnqueue(bool shouldEncrypt)
{
    auto emptyMsg = make_unique<Message>(); // trigger connect frame for lease transport
    Invariant(emptyMsg->SerializedSize() == 0);
    EnqueueMessage(move(emptyMsg), TimeSpan::MaxValue, shouldEncrypt);
}

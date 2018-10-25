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

    const static size_t FrameQueueBiqueChunkSize = (1024 * 32) / sizeof(TcpSendBuffer::Frame);
    const static size_t SendBatchBufferCountLimit = 1024; // writev on Linux limit buffer count to 1024
}

TcpSendBuffer::Frame::Frame(
    MessageUPtr && message,
    byte securityProviderMask,
    TimeSpan expiration,
    bool shouldEncrypt)
: header_(message, securityProviderMask)
, message_(std::move(message))
, shouldEncrypt_(shouldEncrypt)
, preparedForSending_(false)
{
    auto count = ++frameCount;
    TcpConnection::WriteNoise(TraceType, "Frame ctor: count = {0}", count);

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

TcpSendBuffer::Frame::~Frame()
{
    auto count = --frameCount;
    TcpConnection::WriteNoise(TraceType, "Frame dtor: count = {0}", count);
}

size_t TcpSendBuffer::Frame::FrameLength() const
{
    if (encrypted_.empty())
    {
        return header_.FrameLength();
    }

    Invariant(shouldEncrypt_);
    return encrypted_.size();
}

MessageUPtr TcpSendBuffer::Frame::Dispose()
{
    auto message = move(message_);
    Frame::~Frame();
    return message; 
}

MessageUPtr const & TcpSendBuffer::Frame::Message() const
{
    return message_;
}

bool TcpSendBuffer::Frame::HasExpired(StopwatchTime now) const
{
    return !preparedForSending_ && expiration_ <= now;
}

bool TcpSendBuffer::Frame::IsInUse() const
{
    return preparedForSending_;
}

ErrorCode TcpSendBuffer::Frame::EncryptIfNeeded(TcpSendBuffer & sendBuffer)
{
    if (!shouldEncrypt_)
    {
        return ErrorCode();
    }

#ifdef PLATFORM_UNIX
    auto securityContext = sendBuffer.connection_->securityContext_.get();
    auto provider = securityContext->TransportSecurity().SecurityProvider;
    Invariant(provider == SecurityProvider::Ssl || provider == SecurityProvider::Claims);
    auto securityContextSsl = (SecurityContextSsl*)securityContext;
    
    //LINUXTODO avoid data copying during encryption
    ByteBuffer2 buffer(header_.FrameLength());
    TcpConnection::WriteNoise(
        TraceType, sendBuffer.connection_->TraceId(),
        "Encrypt: {0}, plaintext length (including frame header) = {1}",
        message_->TraceId(), buffer.size());

    buffer.append(&header_, sizeof(header_));

    for (BiqueChunkIterator chunk = message_->BeginHeaderChunks(); chunk != message_->EndHeaderChunks(); ++chunk)
    {
        buffer.append(chunk->cbegin(), chunk->size());
    }

    for (BufferIterator chunk = message_->BeginBodyChunks(); chunk != message_->EndBodyChunks(); ++chunk)
    {
        buffer.append(chunk->cbegin(), chunk->size());
    }

    auto error = securityContextSsl->Encrypt(buffer.data(), buffer.size());
    if (!error.IsSuccess()) return error;

    encrypted_ = securityContextSsl->EncryptFinal();
    TcpConnection::WriteNoise(
        TraceType, sendBuffer.connection_->TraceId(),
        "Encrypt: ciphertext length = {0}",
        encrypted_.size());

    // adjust for size change due to encryption
    sendBuffer.totalBufferedBytes_ -= buffer.size(); 
    sendBuffer.totalBufferedBytes_ += encrypted_.size(); 

    return error;

#else

    if (sendBuffer.connection_->securityContext_->FramingProtectionEnabled())
    {
//#if DBG
//        ByteBuffer2 buffer(header_.FrameLength());
//        buffer.append(&header_, sizeof(header_));
//
//        for (BiqueChunkIterator chunk = message_->BeginHeaderChunks(); chunk != message_->EndHeaderChunks(); ++chunk)
//        {
//            buffer.append(chunk->cbegin(), (uint)(chunk->size()));
//        }
//
//        for (BufferIterator chunk = message_->BeginBodyChunks(); chunk != message_->EndBodyChunks(); ++chunk)
//        {
//            buffer.append(chunk->cbegin(), (uint)(chunk->size()));
//        }
//
//        TcpConnection::WriteNoise(TraceType, sendBuffer.connection_->TraceId(), "Encrypt: plaintext: {0:l}", buffer);
//#endif

        auto status = sendBuffer.connection_->securityContext_->EncodeMessage(header_, *message_, encrypted_);
        //TcpConnection::WriteTrace(
        //    SUCCEEDED(status) ? LogLevel::Noise : LogLevel::Warning,
        //    TraceType,
        //    sendBuffer.connection_->TraceId(),
        //    "Encrypt: ciphertext: {0:l}",
        //    encrypted_);

        if (SUCCEEDED(status))
        {
            // adjust for size change due to encryption
            sendBuffer.totalBufferedBytes_ -= header_.FrameLength();
            sendBuffer.totalBufferedBytes_ += encrypted_.size();
            return ErrorCodeValue::Success;
        }

        return ErrorCodeValue::OperationFailed;
    }

    auto sizeBeforeEncryption = message_->SerializedSize();

    auto status = sendBuffer.connection_->securityContext_->EncodeMessage(message_);
    if (FAILED(status))
    {
        return ErrorCode::FromHResult(status);
    }

    // adjust for size change due to encryption
    sendBuffer.totalBufferedBytes_ -= sizeBeforeEncryption;
    sendBuffer.totalBufferedBytes_ += message_->SerializedSize();

    header_ = TcpFrameHeader(message_, sendBuffer.securityProviderMask_);
    return ErrorCode();

#endif
}

ErrorCode TcpSendBuffer::Frame::PrepareForSending(TcpSendBuffer & sendBuffer)
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

    if (!shouldEncrypt_ && sendBuffer.FrameHeaderErrorCheckingEnabled())
    {
#if DBG
        TcpConnection::WriteNoise(
            TraceType,
            "frame header CRC: 0x{0:x}",
            header_.SetFrameHeaderCRC());
#else
        header_.SetFrameHeaderCRC();
#endif
    }

    // add frame header
    sendBuffer.preparedBuffers_.emplace_back(ConstBuffer(&header_, sizeof(header_)));
    sendBuffer.sendingLength_ += sizeof(header_);

    // add message header
    Common::crc32 msgCRC; 
    for (BiqueChunkIterator chunk = message_->BeginHeaderChunks(); chunk != message_->EndHeaderChunks(); ++chunk)
    {
        sendBuffer.preparedBuffers_.emplace_back(ConstBuffer(chunk->cbegin(), chunk->size()));
        sendBuffer.sendingLength_ += chunk->size();
        if (!shouldEncrypt_ && sendBuffer.MessageErrorCheckingEnabled())
        {
            msgCRC.AddData(chunk->cbegin(), chunk->size());
        }
    }

    // add message body
    for (BufferIterator chunk = message_->BeginBodyChunks(); chunk != message_->EndBodyChunks(); ++chunk)
    {
        if (chunk->size() == 0) continue;

        sendBuffer.preparedBuffers_.emplace_back(ConstBuffer(chunk->cbegin(), chunk->size()));
        sendBuffer.sendingLength_ += chunk->size();

        if (!shouldEncrypt_ && sendBuffer.MessageErrorCheckingEnabled())
        {
            msgCRC.AddData(chunk->cbegin(), chunk->size());
        }
    }

    if (!shouldEncrypt_ && sendBuffer.MessageErrorCheckingEnabled())
    {
        header_.SetFrameBodyCRC(msgCRC.Value());
#if DBG
        TcpConnection::WriteNoise(TraceType, "message CRC: 0x{0:x}", msgCRC.Value());
#endif
    }

    return error;
}

TcpSendBuffer::TcpSendBuffer(TcpConnection* connectionPtr)
    : SendBuffer(connectionPtr)
    , messageQueue_(FrameQueueBiqueChunkSize)
{
}

size_t TcpSendBuffer::MessageCount() const
{
    return messageQueue_.size();
}

void TcpSendBuffer::DropExpiredMessage(Frame & frame)
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

ErrorCode TcpSendBuffer::Prepare()
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

void TcpSendBuffer::Consume(size_t length)
{
    if (length == 0)
    {
        return;
    }

    Invariant(totalBufferedBytes_ > 0 && (size_t)totalBufferedBytes_ >= length);
    Invariant(length == sendingLength_);

    auto messageCountBefore = MessageCount();
    auto totalBufferedBytesBefore = totalBufferedBytes_;

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

            trace.Dequeue(
                connection_->TraceId(),
                frame->Message()->TraceId(),
                frame->Message()->IsReply(),
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
        "leaving Consume: MessageCount: {0} -> {1}, delta = {2}; totalBufferedBytes_ = {3} -> {4}, delta = {5}",
        messageCountBefore,
        MessageCount(),
        messageCountBefore - MessageCount(),
        totalBufferedBytesBefore,
        totalBufferedBytes_,
        totalBufferedBytesBefore - totalBufferedBytes_);
}

void TcpSendBuffer::Abort()
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

bool TcpSendBuffer::Empty() const
{
    return messageQueue_.empty();
}

void TcpSendBuffer::EnqueueImpl(MessageUPtr && message, TimeSpan expiration, bool shouldEncrypt)
{
    if (connection_->shouldTracePerMessage_ && !IDatagramTransport::IsPerMessageTraceDisabled(message->Actor))
    {
        trace.Enqueue(
            connection_->TraceId(),
            message->TraceId(),
            message->IsReply(),
            message->LocalTraceContext(),
            (uint32)message->SerializedSize(),
            (uint32)MessageCount(),
            (uint32)totalBufferedBytes_);
    }

    messageQueue_.emplace_back(move(message), securityProviderMask_, expiration, shouldEncrypt);
    totalBufferedBytes_ += messageQueue_.back().FrameLength();
}

bool TcpSendBuffer::PurgeExpiredMessages(StopwatchTime now)
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

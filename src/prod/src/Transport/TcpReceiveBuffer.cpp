// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include"stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

static StringLiteral const TraceType("TcpRecvBuf");

TcpReceiveBuffer::TcpReceiveBuffer(TcpConnection* connectionPtr) : ReceiveBuffer(connectionPtr)
{
}

_Use_decl_annotations_
NTSTATUS TcpReceiveBuffer::GetNextMessage(MessageUPtr & message, StopwatchTime recvTime)
{
    message = nullptr;
    size_t totalBufferUsed = receiveQueue_.end() - receiveQueue_.begin();
    auto securityContext = connectionPtr_->securityContext_.get();

    TcpConnection::WriteNoise(
        TraceType, connectionPtr_->TraceId(),
        "GetNextMessage: enter: totalBufferUsed = {0}",
        totalBufferUsed);

    if (securityContext && securityContext->FramingProtectionEnabled() && securityContext->NegotiationSucceeded())
    {
        msgBuffers_ = &decrypted_;

        if (totalBufferUsed > 0)
        {
            auto status = securityContext->DecodeMessage(receiveQueue_, decrypted_);
            if (FAILED(status)) return STATUS_UNSUCCESSFUL;
        }

        totalBufferUsed = decrypted_.cend() - decrypted_.cbegin();
        TcpConnection::WriteNoise(
            TraceType, connectionPtr_->TraceId(),
            "GetNextMessage: finished security processing: totalBufferUsed = {0}",
            totalBufferUsed);
    }

    if (!haveFrameHeader_)
    {
        if (totalBufferUsed < sizeof(TcpFrameHeader))
        {
            return STATUS_PENDING;
        }

        // a frameheader's worth of stuff is in the buffer
        auto iter = msgBuffers_->begin();
        byte * dest = static_cast<byte*>(static_cast<void*>(&currentFrame_));
        size_t toCopy = sizeof(TcpFrameHeader);
        while (toCopy > 0)
        {
            size_t copyFromCurrentFragment = (iter.fragment_size() <= toCopy) ? iter.fragment_size() : toCopy;
            auto destLimit = static_cast<byte*>(static_cast<void*>(&currentFrame_)) + sizeof(currentFrame_)-dest;

            Invariant(destLimit >= 0);
            KMemCpySafe(dest, static_cast<size_t>(destLimit), &(*iter),  copyFromCurrentFragment);

            dest += copyFromCurrentFragment;
            iter += copyFromCurrentFragment;
            toCopy -= copyFromCurrentFragment;
        }

        haveFrameHeader_ = true;
        TcpConnection::WriteNoise(
            TraceType, connectionPtr_->TraceId(),
            "currentFrame_ = {0}",
            currentFrame_);

        if (!currentFrame_.Validate(
            receiveCount_==0,
            firstFrameHeader_.SecurityProviderMask(),
            connectionPtr_->TraceId()))
        {
            trace.InvalidFrame(
                connectionPtr_->TraceId(),
                connectionPtr_->localAddress_,
                connectionPtr_->targetAddress_,
                connectionPtr_->target_->TraceId(),
                currentFrame_);

            return STATUS_UNSUCCESSFUL;
        }

        if (!firstFrameHeaderSaved_)
        {
            firstFrameHeader_ = currentFrame_;
            firstFrameHeaderSaved_ = true;
            TcpConnection::WriteInfo(
                    TraceType, connectionPtr_->TraceId(),
                    "SecurityProviderMask in first frame header: {0:x}",
                    firstFrameHeader_.SecurityProviderMask());
        }

        if (connectionPtr_->securityContext_)
        {
            if (!connectionPtr_->securityContext_->ConnectionAuthorizationSucceeded() ||
                // Use conservative size limit for the first secured incoming message. The first secured message must be a small
                // transport internal message, e.g. claims message, or auth status message, or listen instance message ...
                // This is important for SecurityNegotiationHeader, which contains FramingProtectionEnabled. All headers exchanged
                // during negotiation are unprotected, thus it is possible that attackers change FramingProtectionEnabled to false
                // so that they can later abuse frame size. SecurityContext::CheckVerificationHeadersIfNeeded will verify if 
                // SecurityNegotiationHeader has been tampered
                connectionPtr_->securityContext_->ShouldCheckVerificationHeaders())
            {
                if (currentFrame_.FrameLength() > SecurityConfig::GetConfig().MaxMessageSizeBeforeSessionIsSecured)
                {
                    trace.IncomingMessageTooLarge(
                        connectionPtr_->TraceId(),
                        connectionPtr_->localAddress_,
                        connectionPtr_->targetAddress_,
                        connectionPtr_->target_->TraceId(),
                        currentFrame_.FrameLength(),
                        SecurityConfig::GetConfig().MaxMessageSizeBeforeSessionIsSecured);

                    return STATUS_DATA_ERROR;
                }
            }
            else
            {
                if (!connectionPtr_->IsIncomingFrameSizeWithinLimit(currentFrame_.FrameLength()))
                {
                    trace.IncomingMessageTooLarge(
                        connectionPtr_->TraceId(),
                        connectionPtr_->localAddress_,
                        connectionPtr_->targetAddress_,
                        connectionPtr_->target_->TraceId(),
                        currentFrame_.FrameLength(),
                        connectionPtr_->maxIncomingFrameSizeInBytes_);

                    return STATUS_DATA_ERROR;
                }
            }
        }
    }

    if (totalBufferUsed < static_cast<size_t>(currentFrame_.FrameLength()))
    {
        currentFrameMissing_ = currentFrame_.FrameLength() - static_cast<uint32>(totalBufferUsed);
        // don't have a message yet
        return STATUS_PENDING;
    }

    currentFrameMissing_ = 0;

    // TODO verify the crc/checksum

    ASSERT_IF(!haveFrameHeader_, "complete frame header should have been received");

    // have a message; construct a message around the buffers
    auto loc = msgBuffers_->begin();
    auto headers = loc + sizeof(TcpFrameHeader);
    auto body = headers + static_cast<size_t>(currentFrame_.HeaderLength());
    auto end = loc + static_cast<size_t>(currentFrame_.FrameLength());

    ByteBiqueRange headerRange(headers, body, false);
    ByteBiqueRange bodyRange(body, end, false);

    size_t bodyLength = currentFrame_.FrameLength() - currentFrame_.HeaderLength();
    size_t bodyBufferToReserve = (bodyLength + connectionPtr_->receiveChunkSize_ - 1) / connectionPtr_->receiveChunkSize_ + 1;

    // construct the message with refs to the headers and body
    message = Common::make_unique<Message>(std::move(headerRange), std::move(bodyRange), recvTime, bodyBufferToReserve);

    if (currentFrame_.FrameBodyCRC())
    {
        Common::crc32 msgCRC; 
        for (auto chunk = message->BeginHeaderChunks(); chunk != message->EndHeaderChunks(); ++chunk)
        {
            msgCRC.AddData(chunk->cbegin(), chunk->size());
        }

        for (auto chunk = message->BeginBodyChunks(); chunk != message->EndBodyChunks(); ++chunk)
        {
            msgCRC.AddData(chunk->cbegin(), chunk->size());
        }

        if (msgCRC.Value() != currentFrame_.FrameBodyCRC())
        {
            TcpConnection::WriteError(
                TraceType, connectionPtr_->TraceId(),
                "message CRC verification failure: message CRC = 0x{0:x}, frame header = {1}",
                msgCRC.Value(), currentFrame_);

            return E_FAIL;
        }

#if DBG
        TcpConnection::WriteNoise(TraceType, connectionPtr_->TraceId(), "message CRC verified");
#endif
    }

    if (securityContext && connectionPtr_->inbound_)
    {
        message->SetSecurityContext(securityContext);
    }

    if (message->Headers.DeletedHeaderByteCount >= 4096)
    {
        TcpConnection::WriteWarning(
            TraceType, connectionPtr_->TraceId(),
            "message {0} : DeletedHeaderByteCount = {1}",
            message->TraceId(),
            message->Headers.DeletedHeaderByteCount);
    }

    if ((receiveCount_++ == 0) && connectionPtr_->securityContext_ /* todo, leikong, revisit when implementing sec provider nego */)
    {
        connectionPtr_->securityContext_->TryGetIncomingSecurityNegotiationHeader(*message);
    }

    if (connectionPtr_->securityContext_ &&
        connectionPtr_->securityContext_->FramingProtectionEnabled() &&
        connectionPtr_->securityContext_->ShouldCheckVerificationHeaders())
    {
        if (!connectionPtr_->securityContext_->CheckVerificationHeadersIfNeeded(*message))
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    return STATUS_SUCCESS;
}

void TcpReceiveBuffer::ConsumeCurrentMessage()
{
    ASSERT_IF(!haveFrameHeader_, "deleting message in unexpected state");

    auto end = msgBuffers_->begin() + static_cast<size_t>(currentFrame_.FrameLength());
    msgBuffers_->truncate_before(end);

    haveFrameHeader_ = false;
}


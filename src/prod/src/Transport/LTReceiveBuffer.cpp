// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include"stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

static StringLiteral const TraceType("LTRecvBuf");

LTReceiveBuffer::LTReceiveBuffer(TcpConnection* connectionPtr) : ReceiveBuffer(connectionPtr)
{
}

void LTReceiveBuffer::AssembleMessage(MessageUPtr & message, StopwatchTime recvTime, bool hasFrameHeader)
{
    // have a message; construct a message around the buffers
    auto loc = msgBuffers_->begin();
    auto start = hasFrameHeader? (loc + sizeof(LTFrameHeader)) : loc;
    auto end = loc + static_cast<size_t>(currentFrame_.FrameSize);

    ByteBiqueRange headerRange(start, start, false);
    ByteBiqueRange bodyRange(start, end, false);

    size_t bodyLength = end - start;
    size_t bodyBufferToReserve = (bodyLength + connectionPtr_->receiveChunkSize_ - 1) / connectionPtr_->receiveChunkSize_ + 1;

    // construct the message with refs to the headers and body
    message = Common::make_unique<Message>(std::move(headerRange), std::move(bodyRange), recvTime, bodyBufferToReserve);
}

_Use_decl_annotations_
NTSTATUS LTReceiveBuffer::GetNextMessage(MessageUPtr & message, StopwatchTime recvTime)
{
    message = nullptr;

    for(;;)
    {
        size_t totalBufferUsed = receiveQueue_.end() - receiveQueue_.begin();

        TcpConnection::WriteNoise(
            TraceType, connectionPtr_->TraceId(),
            "GetNextMessage: enter: totalBufferUsed = {0}",
            totalBufferUsed);

        auto securityContext = connectionPtr_->securityContext_.get();
        if (securityContext)
        {
            if (!securityContext->NegotiationSucceeded())
            {
                if (!totalBufferUsed)
                {
                    return STATUS_PENDING;
                }

                // On Linux, partial incoming negotiate messages will be handled by BIO.
                // If this code is ever used on Windows (unlikely), then we will need to handle partial incoming nego
                // messages, for example, something like decryptionInputMergeBuffer_ can be used to save incomplete
                // incoming nego message and merge with more incoming bytes for next ISC/ASC call
#ifndef PLATFORM_UNIX
                static_assert(false, "need to handle partial incoming negotiation message");
#endif

                currentFrame_.FrameSize = totalBufferUsed;
                AssembleMessage(message, recvTime, false);
                return STATUS_SUCCESS; 
            }

            msgBuffers_ = &decrypted_;

            Invariant(securityContext->TransportSecurity().SecurityProvider == SecurityProvider::Ssl);
            auto securityContextSsl = (SecurityContextSsl*)securityContext;
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
            if (totalBufferUsed < sizeof(LTFrameHeader))
            {
                return STATUS_PENDING;
            }

            // copy frame header
            auto iter = msgBuffers_->begin();
            byte * dest = static_cast<byte*>(static_cast<void*>(&currentFrame_));
            size_t toCopy = sizeof(LTFrameHeader);
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
        }

        if (currentFrame_.FrameSize == sizeof(currentFrame_))
        {
            TcpConnection::WriteNoise(TraceType, connectionPtr_->TraceId(), "received empty connect frame");
            ConsumeCurrentMessage();
            continue;
        }

        if (totalBufferUsed < static_cast<size_t>(currentFrame_.FrameSize))
        {
            currentFrameMissing_ = currentFrame_.FrameSize - static_cast<uint32>(totalBufferUsed);
            return STATUS_PENDING; //incomplete message
        }

        currentFrameMissing_ = 0;

        // have a message; construct a message around the buffers
        AssembleMessage(message, recvTime);
        //LINUXTODO handle LT_FRAMETYPE_CONNECT within transport
        return STATUS_SUCCESS;
    }
}

void LTReceiveBuffer::ConsumeCurrentMessage()
{
    auto end = msgBuffers_->begin() + static_cast<size_t>(currentFrame_.FrameSize);
    msgBuffers_->truncate_before(end);

    haveFrameHeader_ = false;
}


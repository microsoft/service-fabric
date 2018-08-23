// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class ReceiveBuffer
    {
        DENY_COPY(ReceiveBuffer);

    public:
        typedef std::vector<MutableBuffer> Buffers;

        ReceiveBuffer(TcpConnection* connectionPtr);
        virtual ~ReceiveBuffer() = default;

        Buffers const & GetBuffers(size_t count);
        void Commit(size_t count);
        auto ReceivedByteTotal() const { return receivedByteTotal_; }

        virtual NTSTATUS GetNextMessage(_Out_ MessageUPtr & message, Common::StopwatchTime recvTime) = 0;
        virtual void ConsumeCurrentMessage() = 0;

    protected:
        TcpConnection* const connectionPtr_;
        uint64 receiveCount_ = 0;
        uint64 receivedByteTotal_ = 0;
        Common::bique<byte> receiveQueue_;
        Common::bique<byte>* msgBuffers_;
        Buffers buffers_;
        uint32 currentFrameMissing_ = 0;
        bool haveFrameHeader_ = false;
        bool firstFrameHeaderSaved_ = false;

        Common::bique<byte> decrypted_;
    };
}

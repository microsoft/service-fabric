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

        virtual NTSTATUS GetNextMessage(_Out_ MessageUPtr & message, Common::StopwatchTime recvTime) = 0;
        virtual void ConsumeCurrentMessage() = 0;

        virtual bool VerifySecurityProvider() = 0;
        void DisableSecurityProviderCheck();

    protected:
        TcpConnection* const connectionPtr_;
        uint64 receiveCount_ = 0;
        Common::bique<byte> receiveQueue_;
        Common::bique<byte>* msgBuffers_;
        Buffers buffers_;
        uint32 currentFrameMissing_ = 0;
        bool haveFrameHeader_ = false;
        bool shouldVerifySecurityProvider_ = true;
        bool firstFrameHeaderSaved_ = false;

        Common::bique<byte> decrypted_;
    };
}

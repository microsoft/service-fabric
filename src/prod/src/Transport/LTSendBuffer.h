// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class LTSendBuffer : public SendBuffer
    {
    public:

        class Frame
        {
        public:
            Frame(
                MessageUPtr && message,
                byte securityProviderMask,
                Common::TimeSpan expiration,
                bool shouldEncrypt);

            ~Frame();

            MessageUPtr const & Message() const;
            uint64 LTMessageId() const { return messageId_; }
            size_t FrameLength() const;

            Common::ErrorCode PrepareForSending(LTSendBuffer & sendBuffer);

            //bique does not call destructor, thus explicit cleanup is needed for bique<Frame> (FrameQueue below)
            MessageUPtr Dispose();

            bool HasExpired(Common::StopwatchTime now) const; // HasExpired => ! IsInUse
            bool IsInUse() const;

        private:
            Common::ErrorCode EncryptIfNeeded(LTSendBuffer & sendBuffer);

            LTFrameHeader header_;
            MessageUPtr message_;
            uint64 messageId_;
            Common::StopwatchTime expiration_;
            bool shouldEncrypt_;
            bool preparedForSending_;

            //LINUXTODO should this be a list of buffers? or using buffered BIO?
            Common::ByteBuffer2 encrypted_;
        };

        LTSendBuffer(TcpConnection* connectionPtr);

        size_t MessageCount() const override;
        bool Empty() const override;

        Common::ErrorCode Prepare() override;
        void Consume(size_t length) override;

        bool CanSendAuthStatusMessage() const override { return false; }

        bool PurgeExpiredMessages(Common::StopwatchTime now) override;

        void Abort() override;

    protected:
        void EnqueueImpl(MessageUPtr && message, Common::TimeSpan expiration, bool shouldEncrypt) override;
        void BeforeFirstEnqueue(bool shouldEncrypt) override;

    private:
        void DropExpiredMessage(Frame & frame);

        using FrameQueue = Common::bique<Frame>;
        FrameQueue messageQueue_;
    };
}

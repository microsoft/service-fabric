// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class SendBuffer
    {
        DENY_COPY(SendBuffer);

    public:
        typedef std::vector<ConstBuffer> Buffers;

        SendBuffer(TcpConnection* connectionPtr);
        virtual ~SendBuffer() = default;

        virtual bool Empty() const = 0;
        virtual size_t MessageCount() const = 0;
        Common::ErrorCode EnqueueMessage(MessageUPtr && message, Common::TimeSpan expiration, bool shouldEncrypt);

        Buffers const & PreparedBuffers() const;
        virtual Common::ErrorCode Prepare() = 0;
        virtual void Consume(size_t length) = 0;

        auto SentByteTotal() const { return sentByteTotal_; }

        void SetPerfCounters(PerfCountersSPtr const & perfCounters);
        void SetSecurityProviderMask(SecurityProvider::Enum securityProvider);

#ifdef PLATFORM_UNIX
        uint TotalPreparedBytes() const;
        uint FirstBufferToSend() const; //only need for Linux
        bool ConsumePreparedBuffers(ssize_t sent);
#endif
        void SetLimit(ULONG limitInBytes);
        ULONG BytesPendingForSend() const;
        
        virtual bool PurgeExpiredMessages(Common::StopwatchTime now) = 0;

        bool HasNoMessageDelayedBySecurityNeogtiation() const;
        void EnqueueMessagesDelayedBySecurityNegotiation(std::unique_ptr<Message> && claimsMessage);
        virtual bool CanSendAuthStatusMessage() const { return true; }

        void DelayEncryptEnqueue();
        void EnableEncryptEnqueue();

        auto FrameHeaderErrorCheckingEnabled() const { return frameHeaderErrorCheckingEnabled_; } 
        auto MessageErrorCheckingEnabled() const { return messageErrorCheckingEnabled_; }
        void SetFrameHeaderErrorChecking(bool value) { frameHeaderErrorCheckingEnabled_ = value; }
        void SetMessageErrorChecking(bool value) { messageErrorCheckingEnabled_ = value; }

        virtual void Abort() = 0;

    protected:
        typedef std::unordered_set<MessageId, MessageId::Hasher, MessageId::Hasher> MessageIdHashSet;

        Common::ErrorCode Enqueue(MessageUPtr && message, Common::TimeSpan expiration, bool shouldEncrypt);
        virtual void EnqueueImpl(MessageUPtr && message, Common::TimeSpan expiration, bool shouldEncrypt) = 0;
        virtual void BeforeFirstEnqueue(bool shouldEncrypt) { shouldEncrypt; }

        Common::ErrorCode TrackMessageIdIfNeeded(MessageId const &);
        void UntrackMessageIdIfNeeded(MessageId const &);

        TcpConnection * const connection_;
        Common::atomic_long abortCount_{0};
        size_t sendingLength_ = 0;
        Buffers preparedBuffers_;
#ifdef PLATFORM_UNIX
        uint firstBufferToSend_ = 0; // index of first buffer to send
#endif
        uint64 limitInBytes_ = 0;
        byte securityProviderMask_ = SecurityProvider::None;
        bool shouldAddSecNegoHeader_ = false;
        // security headers need to be added again for verification after session is secured, as they are unprotected during negotiation
        bool shouldAddVerificationHeaders_ = true;
        bool canEnqueueEncrypt_ = false; // can enqueue messages that requires encryption
        bool shouldSendConnect_ = false;
        bool firstEnqueueCall_ = false;
        bool frameHeaderErrorCheckingEnabled_ = false;
        bool messageErrorCheckingEnabled_ = false;
        const uint sendBatchLimitInBytes_;
        int64 totalBufferedBytes_ = 0;
        uint64 messageSentCount_ = 0;
        uint64 sentByteTotal_ = 0;
        ULONGLONG queuedBytesMax_ = 0;
        std::vector<MessageUPtr> messagesDelayedBySecurityNegotiation_;
        std::vector<Common::TimeSpan> expirationOfDelayedMessages_;
        uint64 totalDelayedBytes_ = 0;
        TcpDatagramTransport* transportPtr_ = nullptr;
        std::wstring transportId_;
        std::wstring transportOwner_;

        std::unique_ptr<MessageIdHashSet> messageIdTable_;
        PerfCountersSPtr perfCounters_;
    };
}

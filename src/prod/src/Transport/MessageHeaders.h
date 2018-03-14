// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    typedef unsigned short MessageHeaderSize;

    class MessageHeaders : public TransportObject
    {
        DENY_COPY(MessageHeaders);
        friend class TcpConnection;
        friend class SecurityContextSsl;
        friend class SecurityContextWin;

    public:
        MessageHeaders();
        explicit MessageHeaders(
            ByteBiqueRange && shared,
            Common::StopwatchTime recvTime);

        MessageId const & TraceId() const;
        void SetTraceId(MessageId const & value);

        static Common::StopwatchTime NullReceiveTime() { return Common::StopwatchTime::MaxValue; }
        bool HasReceiveTime() const { return recvTime_ != NullReceiveTime(); };
        Common::StopwatchTime ReceiveTime() const { return recvTime_; }

        // For tracing
        bool IsReply() const;

        // Size limit for all serialized message headers combined
        static constexpr ULONG SizeLimitStatic();
        ULONG SizeLimit() const;

        static void Test_SetOutgoingChunkSize(size_t value);
        static void Test_SetOutgoingChunkSizeToDefault();

        // todo: consider returning const & of actual header from these common header properties and add a "initialized" boolean flag for each.
        // also dicussed, have the base header class have a flag for default value so that tracing will print empty but using it requires explicit check
        __declspec(property(get=get_Action)) std::wstring const & Action;
        __declspec(property(get=get_Actor)) Actor::Enum Actor;
        __declspec(property(get=get_MessageId)) Transport::MessageId const & MessageId;
        __declspec(property(get=get_RelatesTo)) Transport::MessageId const & RelatesTo;
        __declspec(property(get=get_RetryCount)) LONG const & RetryCount;
        __declspec(property(get=get_ExpectsReply)) bool ExpectsReply;
        __declspec(property(get=get_Idempotent, put=set_Idempotent)) bool Idempotent;
        __declspec(property(get=get_HighPriority)) bool HighPriority;
        __declspec(property(get=get_FaultErrorCodeValue)) Common::ErrorCodeValue::Enum FaultErrorCodeValue;
        __declspec(property(get=get_HasFaultBody)) bool HasFaultBody;
        __declspec(property(get=get_IsUncorrelatedReply)) bool IsUncorrelatedReply;
        __declspec(property(get=get_DeletedHeaderByteCount)) size_t DeletedHeaderByteCount;

        std::wstring const & get_Action() const;
        Actor::Enum get_Actor() const;
        Transport::MessageId const & get_MessageId() const;
        Transport::MessageId const & get_RelatesTo() const;
        LONG const & get_RetryCount() const;
        bool get_ExpectsReply() const;
        bool get_Idempotent() const;
        void set_Idempotent(bool value);
        bool get_HighPriority() const;
        Common::ErrorCodeValue::Enum get_FaultErrorCodeValue() const;
        bool get_HasFaultBody() const;
        bool get_IsUncorrelatedReply() const;
        size_t get_DeletedHeaderByteCount() const;

        // Support iteration over headers.
        class HeaderIterator;
        HeaderIterator Begin();
        HeaderIterator End();

        // Methods for C++ "for each".
        HeaderIterator begin();  // alias for Begin.
        HeaderIterator end();    // alias for End.

        // Iterate over the serialized chunks of the headers.
        BiqueChunkIterator BeginChunks();
        BiqueChunkIterator EndChunks();

        uint SerializedSize(); // The total size of serialized headers, incoming plus locally added

        // Serializes and adds the header to the collection. Allows multiple instances of the same type.
        // A message header object is serialized into byte stream in the following way: MessageHeaderId
        // comes first, followed by the size of the MessageHeader object, then followed by the actual
        // MessageHeader object.
        // Adds to the end of the header.
        template <class T> NTSTATUS Add(T const & header);

        // Headers can be serialized once and add in serialized form to multiple messages, instead of serializing
        // multiple times for different messages. This helps when a set of headers need to be added to more than
        // one messages with different message body. If message body is also shared, then CheckPoint() and Clone()
        // should be used.
        template <class T> static NTSTATUS Serialize(Serialization::IFabricSerializableStream& stream, T const & header);

        NTSTATUS Add(Serialization::IFabricSerializableStream const& serializedHeaders, bool addingCommonHeaders);

        // If serialized headers in a stream needs to be added to many messages, then it is better to merge
        // buffers in the stream to a single continuous buffer, instead of looping through buffer list everytime
        NTSTATUS Add(KBuffer const& buffer, bool addingCommonHeaders);

        // Serialize into buffers that can be shared across messages, no copy
        static void MakeSharedBuffers(
            _In_ Serialization::IFabricSerializableStream& serializedHeaders,
            _Out_ ByteBique& sharedBuffers);

        // Replace sharedHeaders_, no copy, only increase ref count on "buffers".
        void SetShared(ByteBique & buffers, bool setCommonHeaders);

        void AppendFrom(MessageHeaders & headers);

        // Replace the current headers with new ones. This is used for security (encrypting and decrypting).
        // If header doesn't currently exist, then just add it.
        template <class T> void Replace(T const & header);

        // Specialization for common headers
        template <> inline NTSTATUS Add<ActionHeader>(ActionHeader const & header);
        template <> inline NTSTATUS Add<ActorHeader>(ActorHeader const & header);
        template <> inline NTSTATUS Add<MessageIdHeader>(MessageIdHeader const & header);
        template <> inline NTSTATUS Add<RelatesToHeader>(RelatesToHeader const & header);
        template <> inline NTSTATUS Add<RetryHeader>(RetryHeader const & header);
        template <> inline NTSTATUS Add<ExpectsReplyHeader>(ExpectsReplyHeader const & header);
        template <> inline NTSTATUS Add<HighPriorityHeader>(HighPriorityHeader const & header);
        template <> inline NTSTATUS Add<FaultHeader>(FaultHeader const & header);
        template <> inline NTSTATUS Add<UncorrelatedReplyHeader>(UncorrelatedReplyHeader const & header);

        // Removes a header at position and returns an iterator that
        // points at the following element.
        HeaderIterator& Remove(HeaderIterator& position);
        void RemoveAll();

        template<typename THeader>
        bool TryGetAndRemoveHeader(__out THeader & header)
        {
            bool success = false;

            for (auto header_iter = Begin(); header_iter != End(); ++header_iter)
            {
                if (header_iter->Id() == THeader::Id)
                {
                    header = header_iter->Deserialize<THeader>();
                    success = header_iter->IsValid;
                    Remove(header_iter);
                    return success;
                }
            }

            return success;
        }

        template<typename THeader>
        bool TryRemoveHeader()
        {
            bool removed = false;

            for (auto header_iter = Begin(); header_iter != End(); ++header_iter)
            {
                if (header_iter->Id() == THeader::Id)
                {
                    Remove(header_iter);
                    removed = true;
                }
            }

            return removed;
        }

        // Helper: Finds the first header whose id matches T::Id, and
        // deserializes it into header.
        template <class T> bool TryReadFirst(T& header);

        bool Contains(MessageHeaderId::Enum headerId);

        // Compact headers eliminating deleted headers
        void Compact();
        void CompactIfNeeded();
        static size_t SetCompactThreshold(size_t threshold); //set threshold for CompactIfNeeded,
        //which compacts if the byte count of all headers marked for deletion is beyond threshold

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        void UpdateTraceId();

        class HeaderIterator;

        // Represents an immutable reference to a particular header
        // inside the MessageHeaders data structure.  The lifetime of
        // this object's members is scoped to the lifetime of the
        // source MessageHeaders.  Also, if the referenced header is
        // deleted, the members of this object become invalid.
        struct HeaderReference : public TransportObject
        {
            HeaderReference(HeaderIterator* headerIterator, BiqueRangeStream* biqueStream, size_t skippedBytes = 0);

            MessageHeaderId::Enum Id() const;

            // The total number of bytes occupied in the stream, serialzed MessageHeader object plus type and size fields
            size_t ByteTotalInStream() const;

            MessageHeaderSize SerializedHeaderObjectSize() const;

            ByteBiqueIterator const Start() const;

            // The number of deleted bytes skipped when iterating from previous header
            size_t DeletedBytesBetweenThisAndPredecessor() const;

            template <class T> T Deserialize();

            template <class T> bool TryDeserialize(T & result);

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        private:
            virtual void OnFault(NTSTATUS status);

        private:
            bool TryDeserializeInternal(Serialization::IFabricSerializable* header);

            HeaderIterator* headerIterator_;
            BiqueRangeStream* bufferStream_;
            MessageHeaderId::Enum id_;

            // The total number of bytes occupied in the stream, serialzed MessageHeader object plus type and size fields
            size_t byteTotalInStream_; // Cannot use MessageHeaderSize as type due to possible overflow

            size_t serializedHeaderObjectSize_;
            ByteBiqueIterator start_;
            size_t deletedBytesBetweenThisAndPredecessor_;
        };

        // Enables iteration over all headers in the MessageHeaders.
        typedef std::unique_ptr<HeaderIterator> HeaderIteratorUPtr;
        class HeaderIterator : public TransportObject
        {
            DENY_COPY_ASSIGNMENT(HeaderIterator);

        public:
            static HeaderIterator Begin(BiqueRangeStream& first, BiqueRangeStream& second, MessageHeaders & parent);
            static HeaderIterator End(BiqueRangeStream& chunkBufferStream, BiqueRangeStream& biqueBufferStream, MessageHeaders & parent);
            HeaderIterator(HeaderIterator const & source);

            HeaderIterator & operator ++ ();
            HeaderReference & operator * ();
            HeaderReference * operator -> ();
            bool operator == (HeaderIterator const & rhs) const;
            bool operator != (HeaderIterator const & rhs) const;
            HeaderIterator & Remove();

            size_t CurrentDeletedBytesCount() const;

            MessageHeaders const & Parent() const;

            bool InFirstStream() const;

        private:
            HeaderIterator(const BiqueRangeStream& chunkBufferStream, const BiqueRangeStream& biqueBufferStream, MessageHeaders & parent);
            bool ErrorOccurred();
            virtual void OnFault(NTSTATUS status);

        private:
            MessageHeaders & parent_;
            BiqueRangeStream first_;
            BiqueRangeStream second_;
            BiqueRangeStream* currentStream_;
            HeaderReference headerReference_;
            size_t currentDeletedBytesCount_;
        };

    private:
        static NTSTATUS Serialize(
            Serialization::IFabricSerializableStream& stream,
            Serialization::IFabricSerializable const & header,
            MessageHeaderId::Enum id);

        static NTSTATUS Add(
            _Out_ BiqueWriteStream& bws,
            _In_ Serialization::IFabricSerializableStream const& serializedHeaders);

        NTSTATUS Add(
            Serialization::IFabricSerializableStream & stream,
            Serialization::IFabricSerializable const & header,
            MessageHeaderId::Enum id);

        NTSTATUS CheckSizeLimitBeforeAdd(size_t toAdd);
        void CheckSize();

        void CheckPoint();
        void CopyFrom(const MessageHeaders & other);
        void CopyNonSharedHeadersFrom(const MessageHeaders & other);
        NTSTATUS UpdateCommonHeaders();
        void ResetCommonHeaders();
        void RaiseSizeLimitForTracingUnsafe();

        void Replace(ByteBiqueRange && headers); // it is assumed that headers do not contain IdempotentHeader
        void ReplaceUnsafe(ByteBiqueRange && headers); // No common header updates

        NTSTATUS FinalizeIdempotentHeader();

        static const size_t defaultOutgoingChunkSize_ = 1024;
        static size_t outgoingChunkSize_;
        static size_t MessageHeaders::compactThreshold_;

        // common header fields
        Transport::MessageId messageId_ {Common::Guid::Empty(), 0};
        Transport::MessageId relatesTo_ {Common::Guid::Empty(), 0};
        Transport::MessageId traceId_ {messageId_};

        std::wstring action_;
        Actor::Enum actor_ =  Actor::Empty;

        bool expectsReply_ = false;
        bool idempotent_ = false;
        bool highPriority_ = false;
        bool hasFaultBody_ = false;
        bool isUncorrelatedReply_ = false;
        bool isReply_ = false; // Need this extra field since relatesTo_ can be reset when security is enabled
        bool checkpointDone_ = false;

        Common::ErrorCodeValue::Enum faultErrorCodeValue_ = Common::ErrorCodeValue::Success;
        size_t deletedHeaderByteCount_ = 0;

        ULONG sizeLimit_ = SizeLimitStatic();
        LONG retryCount_ = 0;

        ByteBiqueRange sharedHeaders_;
        ByteBique nonSharedHeaders_;
        BiqueWriteStream appendStream_; // for appending new headers

        Common::StopwatchTime recvTime_ = NullReceiveTime();

        friend class Message;
    };

    inline constexpr ULONG MessageHeaders::SizeLimitStatic()
    {
        return std::numeric_limits<MessageHeaderSize>::max();
    }

    inline ULONG MessageHeaders::SizeLimit() const
    {
        return sizeLimit_;
    }

    inline void MessageHeaders::RaiseSizeLimitForTracingUnsafe()
    {
        sizeLimit_ = std::numeric_limits<ULONG>::max();
    }

    inline void MessageHeaders::UpdateTraceId()
    {
        // Prefer relatesTo_
        if (!relatesTo_.IsEmpty())
        {
            traceId_ = relatesTo_;
            isReply_ = true;
            return;
        }

        if (!messageId_.IsEmpty())
        {
            traceId_ = messageId_;
        }
    }

    inline MessageHeaders::HeaderIterator MessageHeaders::Begin()
    {
        BiqueRangeStream first(sharedHeaders_.Begin, sharedHeaders_.End);
        BiqueRangeStream second(nonSharedHeaders_.begin(), nonSharedHeaders_.end());
        return HeaderIterator::Begin(first, second, *this);
    }

    inline MessageHeaders::HeaderIterator MessageHeaders::End()
    {
        BiqueRangeStream first(sharedHeaders_.Begin, sharedHeaders_.End);
        BiqueRangeStream second(nonSharedHeaders_.begin(), nonSharedHeaders_.end());
        return HeaderIterator::End(first, second, *this);
    }

    inline MessageHeaders::HeaderIterator MessageHeaders::begin()  // alias for Begin.
    {
        return Begin();
    }

    inline MessageHeaders::HeaderIterator MessageHeaders::end()    // alias for End.
    {
        return End();
    }

    // Iterate over the serialized chunks of the headers.
    inline BiqueChunkIterator MessageHeaders::BeginChunks()
    {
        ByteBiqueRange newHeaderRange(nonSharedHeaders_.begin(), nonSharedHeaders_.end(), false);
        BiqueChunkIterator result(sharedHeaders_, newHeaderRange);
        return result;
    }

    inline BiqueChunkIterator MessageHeaders::EndChunks()
    {
        ByteBiqueRange newHeaderRange(nonSharedHeaders_.begin(), nonSharedHeaders_.end(), false);
        BiqueChunkIterator result(BiqueChunkIterator::End(sharedHeaders_, newHeaderRange));
        return result;
    }

    template <class T> NTSTATUS MessageHeaders::Serialize(Serialization::IFabricSerializableStream& stream, T const & header)
    {
        return Serialize(stream, header, header.Id);
    }

    // Serializes and adds the header to the collection. Allows multiple instances of the same type.
    // A message header object is serialized into byte stream in the following way: MessageHeaderId
    // comes first, followed by the size of the MessageHeader object, then followed by the actual
    // MessageHeader object.
    template <class T> NTSTATUS MessageHeaders::Add(T const & header)
    {
        return Add(*Common::FabricSerializer::CreateSerializableStream(), header, header.Id);
    }

    template <> inline NTSTATUS MessageHeaders::Add<ActionHeader>(ActionHeader const & header)
    {
        auto status = Add(*Common::FabricSerializer::CreateSerializableStream(), header, header.Id);

        //
        // todo, we need to consider adding Add with r-value reference parameter to reduce copying for common headers
        // also, we probably need to check whether there is already an action header
        //
        action_ = header.Action;
        return status;
    }

    template <> inline NTSTATUS MessageHeaders::Add<ActorHeader>(ActorHeader const & header)
    {
        auto status = Add(*Common::FabricSerializer::CreateSerializableStream(), header, header.Id);
        actor_ = header.Actor;
        return status;
    }

    template <> inline NTSTATUS MessageHeaders::Add<MessageIdHeader>(MessageIdHeader const & header)
    {
        auto status = Add(*Common::FabricSerializer::CreateSerializableStream(), header, header.Id);
        messageId_ = header.MessageId;
        UpdateTraceId();
        return status;
    }

    template <> inline NTSTATUS MessageHeaders::Add<RelatesToHeader>(RelatesToHeader const & header)
    {
        auto status = Add(*Common::FabricSerializer::CreateSerializableStream(), header, header.Id);
        relatesTo_ = header.MessageId;
        UpdateTraceId();
        return status;
    }

    template <> inline NTSTATUS MessageHeaders::Add<RetryHeader>(RetryHeader const & header)
    {
        auto status = Add(*Common::FabricSerializer::CreateSerializableStream(), header, header.Id);
        retryCount_ = header.RetryCount;
        return status;
    }

    template <> inline NTSTATUS MessageHeaders::Add<ExpectsReplyHeader>(ExpectsReplyHeader const & header)
    {
        auto status = Add(*Common::FabricSerializer::CreateSerializableStream(), header, header.Id);
        expectsReply_ = header.ExpectsReply;
        return status;
    }

    template <> inline NTSTATUS MessageHeaders::Add<HighPriorityHeader>(HighPriorityHeader const & header)
    {
        auto status = Add(*Common::FabricSerializer::CreateSerializableStream(), header, header.Id);
        highPriority_ = header.HighPriority;
        return status;
    }

    template <> inline NTSTATUS MessageHeaders::Add<FaultHeader>(FaultHeader const & header)
    {
        auto status = Add(*Common::FabricSerializer::CreateSerializableStream(), header, header.Id);
        faultErrorCodeValue_ = header.ErrorCodeValue;
        hasFaultBody_ = header.HasFaultBody;
        return status;
    }

    template <> inline NTSTATUS MessageHeaders::Add<UncorrelatedReplyHeader>(UncorrelatedReplyHeader const & header)
    {
        auto status = Add(*Common::FabricSerializer::CreateSerializableStream(), header, header.Id);
        isUncorrelatedReply_ = header.IsUncorrelatedReply;
        return status;
    }

    template <class T> bool MessageHeaders::TryReadFirst(T& header)
    {
        for(auto headerIter = Begin(); this->IsValid && (headerIter != End()); ++ headerIter)
        {
            if (headerIter->Id() == T::Id)
            {
                header = headerIter->Deserialize<T>();
                return headerIter->IsValid;
            }
        }

        return false;
    }

    template <class THeader>
    void MessageHeaders::Replace(THeader const & header)
    {
        bool exists = false;
        auto iter = this->Begin();
        while (iter != this->End())
        {
            if (iter->Id() == THeader::Id)
            {
                exists = true;
                break;
            }

            ++iter;
            if (!iter->IsValid)
            {
                return;
            }
        }

        if (exists)
        {
            this->Remove(iter);
        }

        this->Add(header);
    }

    inline bool MessageHeaders::HeaderIterator::InFirstStream() const
    {
        return currentStream_ == &first_;
    }

    inline MessageHeaderId::Enum MessageHeaders::HeaderReference::Id() const
    {
        return id_;
    }

    inline size_t MessageHeaders::HeaderReference::ByteTotalInStream() const
    {
        return byteTotalInStream_;
    }

    inline MessageHeaderSize MessageHeaders::HeaderReference::SerializedHeaderObjectSize() const
    {
        return (MessageHeaderSize)serializedHeaderObjectSize_;
    }

    inline const ByteBiqueIterator MessageHeaders::HeaderReference::Start() const
    {
        return start_;
    }

    inline size_t MessageHeaders::HeaderReference::DeletedBytesBetweenThisAndPredecessor() const
    {
        return deletedBytesBetweenThisAndPredecessor_;
    }

    template <class T>
    T MessageHeaders::HeaderReference::Deserialize()
    {
        T result;
        TryDeserialize(result);

        return result;
    }

    template <class T>
    bool MessageHeaders::HeaderReference::TryDeserialize(T & t)
    {
        ASSERT_IF(T::Id != this->id_, "MessageHeader Id mismatch!");

        if (!TryDeserializeInternal(&t))
        {
            t = T();
            return false;
        }

        return true;
    }

    inline MessageHeaders::HeaderReference & MessageHeaders::HeaderIterator::operator * ()
    {
        return headerReference_;
    }

    inline MessageHeaders::HeaderReference * MessageHeaders::HeaderIterator::operator -> ()
    {
        return &headerReference_;
    }

    inline size_t MessageHeaders::HeaderIterator::CurrentDeletedBytesCount() const
    {
        return currentDeletedBytesCount_;
    }

    inline MessageHeaders const & MessageHeaders::HeaderIterator::Parent() const
    {
        return parent_;
    }

    inline bool MessageHeaders::HeaderIterator::operator != (HeaderIterator const & rhs) const
    {
        return !(*this == rhs);
    }

    inline std::wstring const & MessageHeaders::get_Action() const
    {
        return action_;
    }

    inline Actor::Enum MessageHeaders::get_Actor() const
    {
        return actor_;
    }

    inline Transport::MessageId const & MessageHeaders::get_MessageId() const
    {
        return messageId_;
    }

    inline Transport::MessageId const & MessageHeaders::get_RelatesTo() const
    {
        return relatesTo_;
    }

    inline LONG const & MessageHeaders::get_RetryCount() const
    {
        return retryCount_;
    }

    inline bool MessageHeaders::get_ExpectsReply() const
    {
        return expectsReply_;
    }

    inline bool MessageHeaders::get_Idempotent() const
    {
        return idempotent_;
    }

    inline void MessageHeaders::set_Idempotent(bool value)
    {
        idempotent_ = value;
    }

    inline bool MessageHeaders::get_HighPriority() const
    {
        return highPriority_;
    }

    inline Common::ErrorCodeValue::Enum MessageHeaders::get_FaultErrorCodeValue() const
    {
        return faultErrorCodeValue_;
    }

    inline bool MessageHeaders::get_HasFaultBody() const
    {
        return hasFaultBody_;
    }

    inline bool MessageHeaders::get_IsUncorrelatedReply() const
    {
        return isUncorrelatedReply_;
    }

    inline size_t MessageHeaders::get_DeletedHeaderByteCount() const
    {
        return this->deletedHeaderByteCount_;
    }
}

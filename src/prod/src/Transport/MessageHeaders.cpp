// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

size_t MessageHeaders::compactThreshold_ = 1024;
size_t MessageHeaders::outgoingChunkSize_ = MessageHeaders::defaultOutgoingChunkSize_;

MessageHeaders::MessageHeaders() 
    : sharedHeaders_(EmptyByteBique.begin(), EmptyByteBique.end(), false),
    nonSharedHeaders_(outgoingChunkSize_),
    appendStream_(nonSharedHeaders_)
{
}

MessageHeaders::MessageHeaders(ByteBiqueRange && shared, StopwatchTime recvTime)
    : sharedHeaders_(std::move(shared))
    , nonSharedHeaders_(outgoingChunkSize_)
    , appendStream_(nonSharedHeaders_)
    , recvTime_(recvTime)
{
    CheckSize();
    UpdateCommonHeaders();
}

uint MessageHeaders::SerializedSize()
{
    // size overflow checking is done at other places, no need to repeat here
    return (uint)(sharedHeaders_.End - sharedHeaders_.Begin + nonSharedHeaders_.size());
}

void MessageHeaders::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    // todo, remove this const_cast
    MessageHeaders* thisPtr = const_cast<MessageHeaders*>(this);

    w.Write("Headers[");
    for(auto iter = thisPtr->Begin(); this->IsValid && iter != thisPtr->End(); ++ iter)
    {
        MessageHeaderTrace::Trace(w, *iter);
    }
    w.Write(" ~ {0} deleted bytes ~]", deletedHeaderByteCount_);
}

wstring MessageHeaders::ToString() const
{
    wstring result;
    StringWriter(result).Write(*this);
    return result;
}

NTSTATUS MessageHeaders::CheckSizeLimitBeforeAdd(size_t toAdd)
{
    auto beforeSize = SerializedSize(); 
    auto afterSize = beforeSize + toAdd;
    if (afterSize <= MessageHeaders::SizeLimitStatic()) return STATUS_SUCCESS;

    Compact();

#if DBG
    textTrace.WriteNoise("MessageHeaders", "CheckSizeLimitBeforeAdd: serializedSize = {0}, toAdd = {1}, serializedSizeAfterCompact = {2}", beforeSize, toAdd, SerializedSize());
#endif

    beforeSize = SerializedSize();
    afterSize = beforeSize + toAdd;
    if (afterSize <= MessageHeaders::SizeLimitStatic()) return STATUS_SUCCESS;

    RaiseSizeLimitForTracingUnsafe();
    trace.MessageHeaderAddFailure(TraceThis, beforeSize, afterSize, ToString());
    Fault(STATUS_BUFFER_OVERFLOW);
    Common::Assert::TestAssert("Exceeding header size limit");
    return STATUS_BUFFER_OVERFLOW;
}

void MessageHeaders::CheckSize()
{
    if (SerializedSize() > SizeLimitStatic())
    {
        Fault(STATUS_BUFFER_OVERFLOW);
    }
}

_Use_decl_annotations_
void MessageHeaders::MakeSharedBuffers(
    Serialization::IFabricSerializableStream& serializedHeaders,
    ByteBique& sharedBuffers)
{
    BiqueWriteStream bws(sharedBuffers);
    auto status = Add(bws, serializedHeaders);
    Invariant(NT_SUCCESS(status));
}

NTSTATUS MessageHeaders::Add(KBuffer const& buffer, bool addingCommonHeaders)
{
    auto status = CheckSizeLimitBeforeAdd(buffer.QuerySize());
    if (!NT_SUCCESS(status)) return status;

    appendStream_.SeekToEnd();
    appendStream_.WriteBytes(buffer.GetBuffer(), buffer.QuerySize());

    if (addingCommonHeaders)
    {
        status = UpdateCommonHeaders();
    }

    return status;
}

NTSTATUS MessageHeaders::Add(Serialization::IFabricSerializableStream const& serializedHeaders, bool addingCommonHeaders)
{
    auto status = CheckSizeLimitBeforeAdd(serializedHeaders.Size());
    if (!NT_SUCCESS(status)) return status;

    status = Add(appendStream_, serializedHeaders);
    if (!NT_SUCCESS(status)) return status;

    if (addingCommonHeaders)
    {
        status = UpdateCommonHeaders();
    }

    return status;
}

_Use_decl_annotations_
NTSTATUS MessageHeaders::Add(
    BiqueWriteStream& bws,
    Serialization::IFabricSerializableStream const& serializedHeaders)
{
    Serialization::FabricIOBuffer bufferListOnStack[16];
    Serialization::FabricIOBuffer* bufferList = bufferListOnStack;

    size_t count = sizeof(bufferListOnStack) / sizeof(Serialization::FabricIOBuffer);
    auto status = serializedHeaders.GetAllBuffers(bufferList, count);
    if (!NT_SUCCESS(status))
    {
        Invariant(status == K_STATUS_BUFFER_TOO_SMALL);
        auto bufferListInHeap = std::make_unique<Serialization::FabricIOBuffer[]>(count);
        bufferList = bufferListInHeap.get();

        status = serializedHeaders.GetAllBuffers(bufferList, count);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    bws.SeekToEnd();
    for (uint i = 0; i < count; ++i)
    {
        bws.WriteBytes(bufferList[i].buffer, bufferList[i].length);
    }

    return status;
}

NTSTATUS MessageHeaders::Serialize(Serialization::IFabricSerializableStream& stream, Serialization::IFabricSerializable const & header, MessageHeaderId::Enum id)
{
    // WriteRawBytes to avoid compression and meta data
    stream.SeekToEnd();
    stream.WriteRawBytes(sizeof(id), &id);
    auto headerSizeCursor = stream.Position();
    MessageHeaderSize headerSize = 0;
    stream.WriteRawBytes(sizeof(headerSize), &headerSize);

    size_t beforeSize = stream.Size();

    Serialization::IFabricSerializable * serializable = const_cast<Serialization::IFabricSerializable*>(&header);
    NTSTATUS status = stream.WriteSerializable(serializable);
    if (NT_ERROR(status)) return status;

    headerSize = (MessageHeaderSize)(stream.Size() - beforeSize);

    // Write actual header size
    stream.Seek(headerSizeCursor);
    stream.WriteRawBytes(sizeof(headerSize), &headerSize);
    stream.SeekToEnd();

    return status;
}

NTSTATUS MessageHeaders::Add(
    Serialization::IFabricSerializableStream & stream,
    Serialization::IFabricSerializable const & header,
    MessageHeaderId::Enum id)
{
    if (!this->IsValid)
    {
        return this->Status;
    }

    auto status = Serialize(stream, header, id);
    ASSERT_IF(NT_ERROR(status), "Serialize failed: {0}", status);

    return Add(stream, false);
}

bool MessageHeaders::HeaderReference::TryDeserializeInternal(Serialization::IFabricSerializable* header)
{
    if (!this->IsValid)
    {
        Common::Assert::TestAssert();
        return false;
    }

    KMemChannel::UPtr memoryStream = KMemChannel::UPtr(
        _new (WF_SERIALIZATION_TAG, Common::GetSFDefaultKtlSystem().NonPagedAllocator()) 
            KMemChannel(Common::GetSFDefaultKtlSystem().NonPagedAllocator()));

    if (!memoryStream)
    {
        Fault(STATUS_INSUFFICIENT_RESOURCES);
        trace.MessageHeaderDeserializationFailure(TraceThis, this->Status);
        Common::Assert::TestAssert();
        return false;
    }

    NTSTATUS status = memoryStream->Status();
    if (NT_ERROR(status))
    {
        Fault(status);
        trace.MessageHeaderDeserializationFailure(TraceThis, this->Status);
        Common::Assert::TestAssert();
        return false;
    }

    // Tell the stream not to delete what it is looking at
    memoryStream->DetachOnDestruct();

    status = memoryStream->StartCapture();
    if (NT_ERROR(status))
    {
        Fault(status);
        trace.MessageHeaderDeserializationFailure(TraceThis, this->Status);
        Common::Assert::TestAssert();
        return false;
    }

    ULONG bytesLeft = static_cast<ULONG>(this->serializedHeaderObjectSize_);

    while (bytesLeft > 0)
    {
        Common::const_buffer buffer;
        this->bufferStream_->GetCurrentSegment(buffer);

        KMemRef memoryReference(buffer.len, buffer.buf);
        memoryReference._Param = std::min(buffer.len, bytesLeft);

        status = memoryStream->Capture(memoryReference);

        if (NT_ERROR(status))
        {
            Fault(status);
            trace.MessageHeaderDeserializationFailure(TraceThis, this->Status);
            Common::Assert::TestAssert();
            return false;
        }

        this->bufferStream_->SeekForward(buffer.len);
        bytesLeft -= std::min(buffer.len, bytesLeft);
    }

    status = memoryStream->EndCapture();
    if (NT_ERROR(status))
    {
        Fault(status);
        trace.MessageHeaderDeserializationFailure(TraceThis, this->Status);
        Common::Assert::TestAssert();
        return false;
    }

    KArray<Serialization::FabricIOBuffer> receivingExtensionBuffers(Common::GetSFDefaultKtlSystem().NonPagedAllocator());

    status = receivingExtensionBuffers.Status();
    if (NT_ERROR(status))
    {
        Fault(status);
        trace.MessageHeaderDeserializationFailure(TraceThis, this->Status);
        Common::Assert::TestAssert();
        return false;
    }

    Serialization::IFabricStream * receiveByteStream;

    status = Serialization::FabricStream::Create(&receiveByteStream, Ktl::Move(memoryStream), Ktl::Move(receivingExtensionBuffers));
    if (NT_ERROR(status))
    {
        Fault(status);
        trace.MessageHeaderDeserializationFailure(TraceThis, this->Status);
        Common::Assert::TestAssert();
        return false;
    }

    Serialization::IFabricStreamUPtr receiveByteStreamUPtr(receiveByteStream);
    if (receiveByteStream == nullptr)
    {
        Fault(STATUS_INSUFFICIENT_RESOURCES);
        trace.MessageHeaderDeserializationFailure(TraceThis, this->Status);
        Common::Assert::TestAssert();
        return false;
    }

    Serialization::IFabricSerializableStream * readStream;
    status = Serialization::FabricSerializableStream::Create(&readStream, Ktl::Move(receiveByteStreamUPtr));
    if (NT_ERROR(status))
    {
        Fault(status);
        trace.MessageHeaderDeserializationFailure(TraceThis, this->Status);
        Common::Assert::TestAssert();
        return false;
    }

    Serialization::IFabricSerializableStreamUPtr readStreamUPtr(readStream);
    if (readStream == nullptr)
    {
        Fault(STATUS_INSUFFICIENT_RESOURCES);
        trace.MessageHeaderDeserializationFailure(TraceThis, this->Status);
        Common::Assert::TestAssert();
        return false;
    }

    KAssert(serializedHeaderObjectSize_ == readStreamUPtr->Size());

    status = readStreamUPtr->ReadSerializable(header);
    if (NT_ERROR(status))
    {
        Fault(status);
        trace.MessageHeaderDeserializationFailure(TraceThis, this->Status);
        Common::Assert::TestAssert();
        return false;
    }

    return true;
}

MessageHeaders::HeaderIterator::HeaderIterator(const BiqueRangeStream& first, const BiqueRangeStream& second, MessageHeaders & parent)
    : parent_(parent),
    first_(first),
    second_(second),
    currentStream_(&first_),
    headerReference_(this, &first_),
    currentDeletedBytesCount_(0)
{
    if ((first_.Size() > parent_.SizeLimit()) ||
        (second_.Size() > parent_.SizeLimit()) ||
        ((first_.Size() + second_.Size()) > parent_.SizeLimit()))
    {
        this->Fault(STATUS_INVALID_PARAMETER);
        ErrorOccurred();
        return;
    }

    if (first_.Eos())
    {
        currentStream_ = &second_;
        headerReference_ = HeaderReference(this, &second_);
    }

    if (ErrorOccurred())
    {
        return;
    }

    currentDeletedBytesCount_ += headerReference_.DeletedBytesBetweenThisAndPredecessor();
}

MessageHeaders::HeaderIterator::HeaderIterator(HeaderIterator const & source)
    : TransportObject(source),
    first_(source.first_),
    second_(source.second_),
    currentStream_(&first_),
    headerReference_(source.headerReference_),
    currentDeletedBytesCount_(source.currentDeletedBytesCount_),
    parent_(source.parent_)
{
    if (ErrorOccurred())
    {
        return;
    }

    if (!source.InFirstStream())
    {
        currentStream_ = &second_;
    }
}

void MessageHeaders::HeaderReference::OnFault(NTSTATUS status)
{
    bufferStream_->Fault(status);
    if (headerIterator_)
    {
        headerIterator_->Fault(status);
    }
}

MessageHeaders::HeaderIterator MessageHeaders::HeaderIterator::Begin(
    BiqueRangeStream& first,
    BiqueRangeStream& second,
    MessageHeaders & parent)
{
    first.SeekToBegin();
    second.SeekToBegin();
    return HeaderIterator(first, second, parent);
}

MessageHeaders::HeaderIterator MessageHeaders::HeaderIterator::End(
    BiqueRangeStream& first,
    BiqueRangeStream& second,
    MessageHeaders & parent)
{
    first.SeekToEnd();
    second.SeekToEnd();
    return HeaderIterator(first, second, parent);
}

bool MessageHeaders::HeaderIterator::ErrorOccurred()
{
    if (!this->IsValid)
    {
        currentStream_ = &second_;
        currentStream_->SeekToEnd();

        return true;
    }

    if (!currentStream_->IsValid)
    {
        Fault(currentStream_->Status);
        currentStream_ = &second_;
        currentStream_->SeekToEnd();

        return true;
    }

    return false;
}

MessageHeaders::HeaderIterator & MessageHeaders::HeaderIterator::operator ++ ()
{
    if (ErrorOccurred())
    {
        return *this;
    }

    // move to next header
    currentStream_->SeekToBookmark();
    currentStream_->SeekForward(headerReference_.ByteTotalInStream());

    if (ErrorOccurred())
    {
        return *this;
    }

    if (InFirstStream() && first_.Eos())
    {
        // We are at the end of the first stream, so we need to switch to the second stream
        currentStream_ = &second_;
    }

    headerReference_ = HeaderReference(this, currentStream_);
    if (ErrorOccurred())
    {
        return *this;
    }

    // We might have skipped some removed headers, so again we need to check if we need to switch to the second stream
    if (InFirstStream() && first_.Eos())
    {
        currentStream_ = &second_;
        headerReference_ = HeaderReference(this, currentStream_, headerReference_.DeletedBytesBetweenThisAndPredecessor());
        if (ErrorOccurred())
        {
            return *this;
        }

        currentDeletedBytesCount_ += headerReference_.DeletedBytesBetweenThisAndPredecessor();
    }
    else
    {
        currentDeletedBytesCount_ += headerReference_.DeletedBytesBetweenThisAndPredecessor();
    }

    return *this;
}

bool MessageHeaders::HeaderIterator::operator == (HeaderIterator const & rhs) const
{
    return (InFirstStream() == rhs.InFirstStream()) && currentStream_->HasBookmarkEqualTo(*(rhs.currentStream_));
}

MessageHeaders::HeaderIterator & MessageHeaders::HeaderIterator::Remove()
{
    if (ErrorOccurred())
    {
        return *this;
    }

    currentStream_->SeekToBookmark();
    if (!currentStream_->Eos())
    {
        *currentStream_ << MessageHeaderId::INVALID_SYSTEM_HEADER_ID;
        if (ErrorOccurred())
        {
            return *this;
        }
    }

    ++ (*this);
    return *this;
}

void MessageHeaders::HeaderIterator::OnFault(NTSTATUS status)
{
    headerReference_.Fault(status);
    parent_.Fault(status);
}

MessageHeaders::HeaderReference::HeaderReference(HeaderIterator* headerIterator, BiqueRangeStream* biqueStream, size_t skippedBytes)
    : headerIterator_(headerIterator), bufferStream_(biqueStream), start_(biqueStream->GetBookmark()), deletedBytesBetweenThisAndPredecessor_(skippedBytes)
{
    while (!bufferStream_->Eos())
    {
        bufferStream_->SetBookmark();

        start_ = bufferStream_->GetBookmark();
    
        *bufferStream_ >> id_;
        if(!bufferStream_->IsValid) break;

        MessageHeaderSize messageHeaderSize;
        *bufferStream_ >> messageHeaderSize;
        if(!bufferStream_->IsValid) break;

        if (messageHeaderSize == 0)
        {
            this->Fault(STATUS_INVALID_PARAMETER);
            return;
        }

        byteTotalInStream_ = sizeof(MessageHeaderId::Enum) + sizeof(MessageHeaderSize) + messageHeaderSize;
        if (byteTotalInStream_  > headerIterator_->Parent().SizeLimit())
        {
            this->Fault(STATUS_INVALID_PARAMETER);
            return;
        }

        serializedHeaderObjectSize_ = messageHeaderSize;

        if(id_ == MessageHeaderId::INVALID_SYSTEM_HEADER_ID) // Need to skip removed headers
        {
            this->deletedBytesBetweenThisAndPredecessor_ += this->byteTotalInStream_;
            bufferStream_->SeekForward(messageHeaderSize);
            if(!bufferStream_->IsValid) break;
            continue;
        }

        return;
    }

    serializedHeaderObjectSize_ = 0;
    byteTotalInStream_ = 0;
    bufferStream_->SetBookmark();
}

void MessageHeaders::HeaderReference::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "address = {0}, InFirstStream = {1}, headerSize = {2}, headerId = {3}, data =",
        TextTracePtr(&(*start_)), headerIterator_->InFirstStream(), serializedHeaderObjectSize_, id_);

    ByteBiqueIterator iter = start_;
    for (ULONG i = 0; i < byteTotalInStream_; ++ i, ++ iter)
    {
        byte byteValue = *iter;
        w.Write(" {0:x}", byteValue);
    }
    w.WriteLine();
}

void MessageHeaders::CheckPoint()
{
    nonSharedHeaders_.insert(nonSharedHeaders_.begin(), sharedHeaders_.Begin, sharedHeaders_.End);
    sharedHeaders_ = std::move(ByteBiqueRange(std::move(nonSharedHeaders_)));
    checkpointDone_ = true;
    // todo, consider clearing message properties here, so that later clone operation don't have to copy them
}

void MessageHeaders::CompactIfNeeded()
{
    if (DeletedHeaderByteCount > compactThreshold_)
    {
        Compact();
    }
}

size_t MessageHeaders::SetCompactThreshold(size_t threshold)
{
    compactThreshold_ = threshold;
    return compactThreshold_;
}

void MessageHeaders::Compact()
{
    //okay to compact "cloned" messages, as sharedHeaders_ is completed replaced, instead of updating in place

    if (deletedHeaderByteCount_ == 0)
    {
        return;
    }

    ByteBique compacted(outgoingChunkSize_);

    // TODO this is somewhat redundant with CheckPoint, and could be skipped if enumerating header chunks did not include
    // deleted headers.

    // iterating headers skips deleted headers
    for (auto iter = Begin(); iter != End(); ++iter)
    {
        compacted.append(iter->Start(), iter->ByteTotalInStream());
    }

    if (!this->IsValid) return;

    sharedHeaders_ = std::move(ByteBiqueRange(std::move(compacted)));
    nonSharedHeaders_.truncate_before(nonSharedHeaders_.end());

    // all headers marked as deleted have been purged
    this->deletedHeaderByteCount_ = 0;
}

void MessageHeaders::AppendFrom(MessageHeaders & other)
{
    // Checkpoint other headers
    other.CheckPoint();
    other.deletedHeaderByteCount_ = 0; // all headers in "other" will be moved to "this"

    // Steal all shared headers from other into nonSharedHeaders_
    ByteBiqueRange localBiqueRange(std::move(other.sharedHeaders_));
    nonSharedHeaders_.insert(nonSharedHeaders_.begin(), localBiqueRange.Begin, localBiqueRange.End);
    CheckSize();

    // Update all common
    other.ResetCommonHeaders();
    UpdateCommonHeaders();
}

void MessageHeaders::SetShared(ByteBique & buffers, bool setCommonHeaders)
{
    sharedHeaders_ = ByteBiqueRange(buffers.begin(), buffers.end(), true);
    CheckSize();
    if (setCommonHeaders)
    {
        ResetCommonHeaders();
        UpdateCommonHeaders();
    }
}

void MessageHeaders::Replace(ByteBiqueRange && otherRange)
{
    ReplaceUnsafe(std::move(otherRange));

    ResetCommonHeaders();
    UpdateCommonHeaders();
}

void MessageHeaders::ReplaceUnsafe(ByteBiqueRange && headers)
{
    sharedHeaders_ = std::move(ByteBiqueRange(std::move(headers)));
    nonSharedHeaders_.truncate_before(nonSharedHeaders_.end());
    CheckSize();
}

MessageHeaders::HeaderIterator& MessageHeaders::Remove(HeaderIterator& iter)
{
    ASSERT_IF(&iter.Parent() != this, "Only allow iterators from this");
    ASSERT_IF(checkpointDone_ && iter.InFirstStream(), "after CheckPoint, shared header {0} cannot be removed from {1}", *iter, *this); 
    // todo, leikong, it may be unsafe to remove from sharedHeaders_ if a message is cloned but no checkpoint, e.g.
    // removing a shared header from a clone for retry will also remove the header from the clone for the original
    // send, which may be still on-going due to slow connection setup. Possible solutions:
    // 1. copy all remaining headers when removing a shared header. Performance may suffer, e.g. federation 
    //    routing, multicasting and broadcasting, where headers may be removed after cloning
    // 2. record the removed locations into a list local to each message object, and skip these removed headers
    //    when doing chunk iteration

    // todo: the following needs to updated when common header shortcut properties become real header instances
    switch (iter->Id())
    {
    case MessageHeaderId::Action:
        action_.clear();
        break;

    case MessageHeaderId::Actor:
        actor_ = Actor::Enum::Empty;
        break;

    case MessageHeaderId::MessageId:
        messageId_ = Transport::MessageId(Common::Guid::Empty(), 0);
        break;

    case MessageHeaderId::RelatesTo:
        relatesTo_ = Transport::MessageId(Common::Guid::Empty(), 0);
        break;

    case MessageHeaderId::Retry:
        retryCount_ = 0;
        break;

    case MessageHeaderId::ExpectsReply:
        expectsReply_ = false;
        break;

    case MessageHeaderId::Idempotent:
        idempotent_ = false;
        break;

    case MessageHeaderId::HighPriority:
        highPriority_ = false;
        break;

    case MessageHeaderId::Fault:
        faultErrorCodeValue_ = Common::ErrorCodeValue::Success;
        hasFaultBody_ = false;
        break;

    case MessageHeaderId::UncorrelatedReply:
        isUncorrelatedReply_ = false;
        break;
    }

    this->deletedHeaderByteCount_ += iter->ByteTotalInStream();

    return iter.Remove();
}

void MessageHeaders::RemoveAll()
{
    sharedHeaders_ = ByteBiqueRange(EmptyByteBique.begin(), EmptyByteBique.end(), false);
    nonSharedHeaders_.truncate_before(nonSharedHeaders_.end());

    deletedHeaderByteCount_ = 0;
    ResetCommonHeaders();
}

void MessageHeaders::ResetCommonHeaders()
{
    action_.clear();
    actor_ = Actor::Enum::Empty;
    messageId_ = Transport::MessageId(Common::Guid::Empty(), 0);
    relatesTo_ = Transport::MessageId(Common::Guid::Empty(), 0);
    retryCount_ = 0;
    expectsReply_ = false;
    idempotent_ = false;
    highPriority_ = false;
    faultErrorCodeValue_ = Common::ErrorCodeValue::Success;
    hasFaultBody_ = false;
    isUncorrelatedReply_ = false;
}

NTSTATUS MessageHeaders::UpdateCommonHeaders()
{
    // Search for common headers
    // todo, consider moving this to HeaderReference.Deserialize<T> specilization.
    // we only need to do this when the requested common header is not already cached
    HeaderIterator iter = Begin();
    for ( ; this->IsValid && iter != End(); ++ iter)
    {
        switch (iter->Id())
        {
        case MessageHeaderId::Action:
            {
                ActionHeader header = iter->Deserialize<ActionHeader>();
                if (!iter->IsValid)
                {
                    KAssert(!this->IsValid);
                    return this->Status;
                }

                action_ = header.Action;
                break;
            }
        case MessageHeaderId::Actor:
            {
                ActorHeader header = iter->Deserialize<ActorHeader>();
                if (!iter->IsValid)
                {
                    KAssert(!this->IsValid);
                    return this->Status;
                }

                actor_ = header.Actor;
                break;
            }
        case MessageHeaderId::MessageId:
            {
                MessageIdHeader header = iter->Deserialize<MessageIdHeader>();
                if (!iter->IsValid)
                {
                    KAssert(!this->IsValid);
                    return this->Status;
                }

                messageId_ = header.MessageId;
                UpdateTraceId();
                break;
            }
        case MessageHeaderId::RelatesTo:
            {
                RelatesToHeader header = iter->Deserialize<RelatesToHeader>();
                if (!iter->IsValid)
                {
                    KAssert(!this->IsValid);
                    return this->Status;
                }

                relatesTo_ = header.MessageId;
                UpdateTraceId();
                break;
            }
        case MessageHeaderId::Retry:
            {
                RetryHeader header = iter->Deserialize<RetryHeader>();
                if (!iter->IsValid)
                {
                    KAssert(!this->IsValid);
                    return this->Status;
                }

                retryCount_ = header.RetryCount;
                break;
            }
        case MessageHeaderId::ExpectsReply:
            {
                ExpectsReplyHeader header = iter->Deserialize<ExpectsReplyHeader>();
                if (!iter->IsValid)
                {
                    KAssert(!this->IsValid);
                    return this->Status;
                }

                expectsReply_ = header.ExpectsReply;
                break;
            }
        case MessageHeaderId::Idempotent:
            {
                IdempotentHeader header = iter->Deserialize<IdempotentHeader>();
                if (!iter->IsValid)
                {
                    KAssert(!this->IsValid);
                    return this->Status;
                }

                idempotent_ = header.Idempotent;
                break;
            }
        case MessageHeaderId::HighPriority:
            {
                HighPriorityHeader header = iter->Deserialize<HighPriorityHeader>();
                if (!iter->IsValid)
                {
                    KAssert(!this->IsValid);
                    return this->Status;
                }

                highPriority_ = header.HighPriority;
                break;
            }
        case MessageHeaderId::Fault:
            {
                FaultHeader header = iter->Deserialize<FaultHeader>();
                if (!iter->IsValid)
                {
                    KAssert(!this->IsValid);
                    return this->Status;
                }

                faultErrorCodeValue_ = header.ErrorCodeValue;
                hasFaultBody_ = header.HasFaultBody;
                break;
            }
        case MessageHeaderId::UncorrelatedReply:
            {
                UncorrelatedReplyHeader header = iter->Deserialize<UncorrelatedReplyHeader>();
                if (!iter->IsValid)
                {
                    KAssert(!this->IsValid);
                    return this->Status;
                }

                isUncorrelatedReply_ = header.IsUncorrelatedReply;
                break;
            }
        }
    }

    if (!this->IsValid) return this->Status;

    this->deletedHeaderByteCount_ = iter.CurrentDeletedBytesCount();
    return this->Status;
}

void MessageHeaders::Test_SetOutgoingChunkSize(size_t value)
{
    outgoingChunkSize_ = value;
}

void MessageHeaders::Test_SetOutgoingChunkSizeToDefault()
{
    outgoingChunkSize_ = defaultOutgoingChunkSize_;
}

MessageId const & MessageHeaders::TraceId() const
{
    return traceId_;
}

void MessageHeaders::SetTraceId(Transport::MessageId const & value)
{
    traceId_ = value;
}

bool MessageHeaders::IsReply() const
{
    return isReply_;
}

void MessageHeaders::CopyFrom(const MessageHeaders & other)
{
    recvTime_ = other.recvTime_;

    // todo, leikong, the assumption is there is no shared header, may need to clear first
    nonSharedHeaders_.append(other.nonSharedHeaders_.begin(), other.nonSharedHeaders_.size());
    nonSharedHeaders_.append(other.sharedHeaders_.Begin, other.sharedHeaders_.End - other.sharedHeaders_.Begin);
    UpdateCommonHeaders();
}

void MessageHeaders::CopyNonSharedHeadersFrom(const MessageHeaders & other)
{
    CODING_ERROR_ASSERT(nonSharedHeaders_.empty());
    nonSharedHeaders_.append(other.nonSharedHeaders_.begin(), other.nonSharedHeaders_.size());

    UpdateCommonHeaders();
}

bool MessageHeaders::Contains(MessageHeaderId::Enum headerId)
{
	for (auto headerIter = Begin(); this->IsValid && (headerIter != End()); ++headerIter)
	{
		if (headerIter->Id() == headerId)
		{
			return true;
		}
	}

	return false;
}

NTSTATUS MessageHeaders::FinalizeIdempotentHeader()
{
    if (idempotent_)
    {
        IdempotentHeader header;
        return Add(*Common::FabricSerializer::CreateSerializableStream(), header, header.Id);
    }

    return STATUS_SUCCESS;
}

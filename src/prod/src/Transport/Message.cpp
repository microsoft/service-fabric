// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Serialization;
using namespace Common;
using namespace std;

Message::Message()
    : body_(EmptyByteBique.begin(), EmptyByteBique.end(), false)
    , bodyBuffers_(std::make_shared<BodyBuffers>(body_))
{ 
}

Message::Message(Message const & other)
    : properties_(other.properties_)
    , body_(EmptyByteBique.begin(), EmptyByteBique.end(), false)
    , securityContext_(other.securityContext_)
    , localTraceContext_(other.localTraceContext_)
{
    this->Headers.CopyFrom(((Message&)other).Headers);

    shared_ptr<vector<byte const*>> bufferRoot = make_shared<vector<byte const*>>();
    auto bufferList = other.bodyBuffers_->GetBuffers();
    int i = 0;
    for (auto const & buffer : other.bodyBuffers_->GetBuffers())
    {
        byte* byteBuffer = new byte[buffer.len];
        KMemCpySafe(byteBuffer, buffer.len, buffer.buf, buffer.len);
        bufferList[i++].buf = (char*)byteBuffer;
        bufferRoot->push_back(byteBuffer);
    }

    bodyBuffers_ = make_shared<BodyBuffers>(
        bufferList,
        [bufferRoot](vector<Common::const_buffer> const &, void *)
        {
            for (auto const & buffer : *bufferRoot)
            {
                delete buffer;
            }
        },
        nullptr);
}

Message::Message(
    std::vector<Common::const_buffer> const & buffers,
    DeleteCallback const & deleteCallback, void * state)
    : body_(EmptyByteBique.begin(), EmptyByteBique.end(), false)
    , bodyBuffers_(std::make_shared<BodyBuffers>(buffers, deleteCallback, state))
{
}

Message::Message(ByteBiqueRange && headersRange, ByteBiqueRange && bodyRange, StopwatchTime recvTime, size_t reserveBodyBufferCount)
    : headers_(std::move(headersRange), recvTime)
    , body_(std::move(bodyRange))
    , bodyBuffers_(std::make_shared<BodyBuffers>(body_, reserveBodyBufferCount))
{
}

Message::Message(
    ByteBiqueRange && headersRange,
    ByteBiqueRange && bodyRange,
    std::shared_ptr<BodyBuffers> const & buffers,
    StopwatchTime recvTime)
    : headers_(std::move(headersRange), recvTime)
    , body_(std::move(bodyRange))
    , bodyBuffers_(buffers)
{
}

Message::~Message()
{
}

Transport::MessageId const & Message::TraceId() const
{
    return headers_.TraceId();
}

void Message::SetTraceId(Transport::MessageId const & value)
{
    headers_.SetTraceId(value);
}

wstring const & Message::LocalTraceContext() const
{
    return localTraceContext_;
}

void Message::SetLocalTraceContext(std::wstring const & context)
{
    localTraceContext_ = context;
}

void Message::SetLocalTraceContext(std::wstring && context)
{
    localTraceContext_ = move(context);
}

void Message::MoveLocalTraceContextFrom(Message & rhs)
{
    localTraceContext_ = move(rhs.localTraceContext_);
}

// For tracing
bool Message::IsReply() const
{
    return headers_.IsReply();
}

NTSTATUS Message::GetStatus() const
{
    if (!headers_.IsValid)
    {
        return headers_.GetStatus();
    }

    return TransportObject::GetStatus();
}

uint Message::SerializedHeaderSize()
{
    return headers_.SerializedSize();
}

uint Message::SerializedBodySize()
{
    if (bodyBuffers_->size() > numeric_limits<uint>::max())
    {
        Fault(STATUS_BUFFER_OVERFLOW);
    }

    return (uint)bodyBuffers_->size();
}

uint Message::SerializedSize()
{
    uint size;
    auto status = UIntAdd(SerializedHeaderSize(), SerializedBodySize(), &size);
    if (status != S_OK)
    {
        Fault(STATUS_BUFFER_OVERFLOW);
    }

    return size;
}

void Message::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << L"Message{";
    w.Write(headers_);
    w << L"}Message";
}

void Message::CheckPoint()
{
    headers_.CheckPoint();
}

MessageUPtr Message::Clone()
{
    MessageUPtr clone = CloneSharedParts();
    clone->headers_.CopyNonSharedHeadersFrom(headers_);
    clone->headers_.Idempotent = headers_.Idempotent;
    clone->properties_.insert(properties_.begin(), properties_.end());

    return clone;
}

MessageUPtr Message::Clone(MessageHeaders && extraHeaders)
{
    if (!headers_.nonSharedHeaders_.empty())
    {
        headers_.CheckPoint();
    }

    MessageUPtr clone = CloneSharedParts();

    CODING_ERROR_ASSERT(extraHeaders.sharedHeaders_.IsEmpty());
    clone->headers_.nonSharedHeaders_ = std::move(extraHeaders.nonSharedHeaders_);
    clone->headers_.UpdateCommonHeaders();
    clone->headers_.Idempotent = headers_.Idempotent;

    clone->properties_.insert(properties_.begin(), properties_.end());

    return clone;
}

MessageUPtr Message::CloneSharedParts()
{
    // share body and sharedHeaders_
    ByteBiqueRange headersToShare(headers_.sharedHeaders_); // copy leads to addRef on underlying buffers
    ByteBiqueRange body(body_); // copy leads to addRef on underlying buffers

    auto clone = unique_ptr<Message>(new Message(move(headersToShare), move(body), bodyBuffers_, headers_.ReceiveTime()));
    clone->securityContext_ = securityContext_;

    return clone;
}

BufferIterator Message::BeginBodyChunks()
{
    return bodyBuffers_->begin();
}

BufferIterator Message::EndBodyChunks()
{
    return bodyBuffers_->end();
}

MessageHeaders& Message::get_Headers()
{
    return headers_;
}

// Iterate over the serialized chunks of the headers.
BiqueChunkIterator Message::BeginHeaderChunks()
{
    return headers_.BeginChunks();
}

BiqueChunkIterator Message::EndHeaderChunks()
{
    return headers_.EndChunks();
}

std::wstring const & Message::get_Action() const
{
    return headers_.Action;
}

SendStatusCallback const & Message::get_SendStatusCallback() const
{
    return sendStatusCallback_;
}

Actor::Enum Message::get_Actor() const
{
    return headers_.Actor;
}

Transport::MessageId const & Message::get_MessageId() const
{
    return headers_.MessageId;
}

Transport::MessageId const & Message::get_RelatesTo() const
{
    return headers_.RelatesTo;
}

LONG const & Message::get_RetryCount() const
{
    return headers_.RetryCount;
}

bool Message::get_ExpectsReply() const
{
    return headers_.ExpectsReply;
}

bool Message::get_HighPriority() const
{
    return headers_.HighPriority;
}

Common::ErrorCodeValue::Enum Message::get_FaultErrorCodeValue() const
{
    return headers_.FaultErrorCodeValue;
}

bool Message::get_HasFaultBody() const
{
    return headers_.HasFaultBody;
}

bool Message::get_IsUncorrelatedReply() const
{
    return headers_.IsUncorrelatedReply;
}

void Message::UpdateBodyBufferList(BodyBuffers && other)
{
    bodyBuffers_->UpdateBufferList(move(other));
}

Serialization::IFabricSerializableStreamUPtr Message::GetBodyStream()
{
    if (!this->IsValid)
    {
        Common::Assert::TestAssert();
        return nullptr;
    }

    std::vector<Common::const_buffer> buffers = this->bodyBuffers_->GetBuffers();

    KMemChannel::UPtr memoryStream = KMemChannel::UPtr(
        _new(WF_SERIALIZATION_TAG, Common::GetSFDefaultKtlSystem().NonPagedAllocator()) 
            KMemChannel(Common::GetSFDefaultKtlSystem().NonPagedAllocator(), static_cast<ULONG>(this->bodyBuffers_->size())));

    if (!memoryStream)
    {
        Fault(STATUS_INSUFFICIENT_RESOURCES);
        trace.MessageBodyDeserializationFailure(TraceThis, TransportObject::Status);
        Common::Assert::TestAssert();
        return nullptr;
    }

    NTSTATUS status = memoryStream->Status();
    if (NT_ERROR(status))
    {
        Fault(status);
        trace.MessageBodyDeserializationFailure(TraceThis, TransportObject::Status);
        Common::Assert::TestAssert();
        return nullptr;
    }

    // Tell the stream not to delete what it is looking at
    memoryStream->DetachOnDestruct();

    status = memoryStream->StartCapture();
    if (NT_ERROR(status))
    {
        Fault(status);
        trace.MessageBodyDeserializationFailure(TraceThis, TransportObject::Status);
        Common::Assert::TestAssert();
        return nullptr;
    }

    for (ULONG i = 0; i < buffers.size(); ++i)
    {
        auto buffer = buffers[i];

        KMemRef memoryReference(buffer.len, buffer.buf);
        memoryReference._Param = buffer.len;

        status = memoryStream->Capture(memoryReference);

        if (NT_ERROR(status))
        {
            Fault(status);
            trace.MessageBodyDeserializationFailure(TraceThis, TransportObject::Status);
            Common::Assert::TestAssert();
            return nullptr;
        }
    }

    status = memoryStream->EndCapture();
    if (NT_ERROR(status))
    {
        Fault(status);
        trace.MessageBodyDeserializationFailure(TraceThis, TransportObject::Status);
        Common::Assert::TestAssert();
        return nullptr;
    }

    KArray<Serialization::FabricIOBuffer> receivingExtensionBuffers(Common::GetSFDefaultKtlSystem().NonPagedAllocator());

    status = receivingExtensionBuffers.Status();
    if (NT_ERROR(status))
    {
        Fault(status);
        trace.MessageBodyDeserializationFailure(TraceThis, TransportObject::Status);
        Common::Assert::TestAssert();
        return nullptr;
    }

    Serialization::IFabricStream * receiveByteStream;

    status = Serialization::FabricStream::Create(&receiveByteStream, Ktl::Move(memoryStream), Ktl::Move(receivingExtensionBuffers));
    if (NT_ERROR(status))
    {
        Fault(status);
        trace.MessageBodyDeserializationFailure(TraceThis, TransportObject::Status);
        Common::Assert::TestAssert();
        return nullptr;
    }

    Serialization::IFabricStreamUPtr receiveByteStreamUPtr(receiveByteStream);
    if (receiveByteStream == nullptr)
    {
        Fault(STATUS_INSUFFICIENT_RESOURCES);
        trace.MessageBodyDeserializationFailure(TraceThis, TransportObject::Status);
        Common::Assert::TestAssert();
        return nullptr;
    }

    Serialization::IFabricSerializableStream * readStream;
    status = Serialization::FabricSerializableStream::Create(&readStream, Ktl::Move(receiveByteStreamUPtr));
    if (NT_ERROR(status))
    {
        Fault(status);
        trace.MessageBodyDeserializationFailure(TraceThis, TransportObject::Status);
        Common::Assert::TestAssert();
        return nullptr;
    }

    KAssert(readStream->Size() == SerializedBodySize());

    Serialization::IFabricSerializableStreamUPtr readStreamUPtr(readStream);
    if (readStream == nullptr)
    {
        Fault(STATUS_INSUFFICIENT_RESOURCES);
        trace.MessageBodyDeserializationFailure(TraceThis, TransportObject::Status);
        Common::Assert::TestAssert();
        return nullptr;
    }

    return readStreamUPtr;
}

bool Message::GetBodyInternal(Serialization::IFabricSerializable* body)
{
    Serialization::IFabricSerializableStreamUPtr readStreamUPtr = GetBodyStream();
    if (readStreamUPtr == nullptr)
    {
        return false;
    }

    NTSTATUS status = readStreamUPtr->ReadSerializable(body);
    if (NT_ERROR(status))
    {
        Fault(status);
        trace.MessageBodyDeserializationFailure(TraceThis, TransportObject::Status);
        Common::Assert::TestAssert();
        return false;
    }

    return true;
}

std::wstring Message::ToString() const
{
    wstring result;
    result.reserve(const_cast<Message*>(this)->SerializedHeaderSize() * 2);
    StringWriter(result).Write(*this);
    return result;
}

bool Message::HasSendStatusCallback() const
{
    return sendStatusCallback_ != nullptr;
}

void Message::OnSendStatus(ErrorCode const & error, MessageUPtr && msg)
{
    if (sendStatusCallback_)
    {
        auto thisPtr = msg.release();
        Invariant(this == thisPtr);
        Threadpool::Post([error, this] ()
        {
            this->sendStatusCallback_(error, MessageUPtr(this));
        });
    }
}

void Message::SetSendStatusCallback(SendStatusCallback const & sendFailureCallback)
{
    KAssert(sendStatusCallback_ == nullptr);
    sendStatusCallback_ = sendFailureCallback;
}

Message::BodyBuffers::BodyBuffers(Serialization::IFabricSerializableStreamUPtr && streamUPtr)
    : state_(nullptr), totalSize_(0)
{
    this->streamUPtr_ = Ktl::Move(streamUPtr);
}

Message::BodyBuffers::BodyBuffers(ByteBiqueRange & body, size_t reserveCount)
    : state_(nullptr), totalSize_(0)
{
    buffers_.reserve(reserveCount);

    auto iter = body.Begin;
    auto endIter = body.End;

    while (iter != endIter)
    {
        if ((iter + iter.fragment_size()) <= endIter)
        {
            size_t size = iter.fragment_size();

            this->buffers_.push_back(Common::const_buffer(iter.fragment_begin(), size));
            this->totalSize_ += size;
            iter += size;
        }
        else
        {
            // This is the last chunk and it is not filled yet
            size_t size = endIter - iter;
            ASSERT_IF(endIter < iter, "Size cannot be less than 0: {0}", size);

            this->buffers_.push_back(Common::const_buffer(iter.fragment_begin(), size));
            this->totalSize_ += size;
            break;
        }
    }
}

Message::BodyBuffers::BodyBuffers(std::vector<Common::const_buffer> const & buffers, DeleteCallback const & callback, void * state)
    : buffers_(/*std::move*/(buffers)), callback_(callback), state_(state), totalSize_(0)  // TODO: make buffers movable, bug in our stl implementation
{
    ASSERT_IFNOT(callback, "callback must be provided");

    for(auto & item : buffers_)
    {
        this->totalSize_ += item.len;
    }
}

Message::BodyBuffers::BodyBuffers(size_t reserveCount) : state_(nullptr), totalSize_(0)
{
    buffers_.reserve(reserveCount);
}

void Message::BodyBuffers::AddBuffer(void const *buffer, ULONG size)
{
    KAssert(buffers_.size() < buffers_.capacity());
    totalSize_ += size;
    buffers_.push_back(const_buffer(buffer, size));
}

Message::BodyBuffers::~BodyBuffers()
{
    if (this->callback_)
    {
        this->callback_(this->buffers_, this->state_);
    }

    if (this->streamUPtr_)
    {
        this->streamUPtr_->InvokeCallbacks();
    }
}

BufferIterator Message::BodyBuffers::begin()
{
    return this->buffers_.begin();
}

BufferIterator Message::BodyBuffers::end()
{
    return this->buffers_.end();
}

size_t Message::BodyBuffers::size()
{
    return totalSize_;
}

std::vector<Common::const_buffer> const & Message::BodyBuffers::GetBuffers() const
{
    return this->buffers_;
}

void Message::BodyBuffers::UpdateBufferList(BodyBuffers && other)
{
    // Usage scenario: after SSL decryption, this function allows skipping SSL header and trailer without copying decrypted data.
    // This operation does not affect lifetime management of the buffers, which is still managed by ByteBiqueRange Message::body_.
    KAssert(other.callback_ == nullptr);
    KAssert(other.state_ == nullptr);

    buffers_ = move(other.buffers_);
    totalSize_ = other.totalSize_;
    other.totalSize_ = 0;
}

void Message::BodyBuffers::PrepareForSend()
{
    this->buffers_.clear();

    Serialization::FabricIOBuffer buffer;
    bool isExtensionBuffer;

    NTSTATUS status = this->streamUPtr_->GetNextBuffer(buffer, isExtensionBuffer);

    while (NT_SUCCESS(status))
    {
        ASSERT_IF(isExtensionBuffer, "Extension buffers not supported yet in fabric user mode");

        this->buffers_.push_back(Common::const_buffer(buffer.buffer, buffer.length));
        this->totalSize_ += buffer.length;

        status = this->streamUPtr_->GetNextBuffer(buffer, isExtensionBuffer);
    }

    if (status != (NTSTATUS)K_STATUS_NO_MORE_EXTENSION_BUFFERS)
    {
        Assert::CodingError("GetNextBuffer returned an error other than K_STATUS_NO_MORE_EXTENSION_BUFFERS: {0}", status);
    }
}

bool Message::IsInRole(RoleMask::Enum roleMask)
{
    if (securityContext_)
    {
        auto effectiveRole = RoleMask::None;
        ClientRoleHeader header;
        if (Headers.TryReadFirst(header))
        {
            effectiveRole = header.Role();
        }

        return securityContext_->IsInRole(roleMask, effectiveRole);
    }

    return true;
}

void Message::SetSecurityContext(SecurityContext* context)
{
    ASSERT_IFNOT(context, "SecurityContext cannot be null");

    if (!context->TransportSecurity().Settings().IsClientRoleInEffect())
    {
        return;
    }

    ASSERT_IF(securityContext_, "security context already set");
    securityContext_ = context->shared_from_this();
}

bool Message::AccessCheck(AccessControl::FabricAcl const & acl, DWORD desiredAccess) const
{
#ifndef PLATFORM_UNIX
    if (securityContext_)
    {
        return securityContext_->AccessCheck(acl, desiredAccess);
    }
#endif

    return true;
}


// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::TStore;
using namespace Common;

MemoryBuffer::MemoryBuffer() : 
    isResizable_(true),
    bufferSPtr_(nullptr),
    binaryWriterSPtr_(nullptr),
    position_(0)
{
    NTSTATUS status = SharedBinaryWriter::Create(GetThisAllocator(), binaryWriterSPtr_);
    Diagnostics::Validate(status);
}

MemoryBuffer::MemoryBuffer(__in ULONG capacity) : 
    isResizable_(false), 
    bufferSPtr_(nullptr), 
    binaryWriterSPtr_(nullptr), 
    position_(0)
{
    NTSTATUS status = KBuffer::Create(capacity, bufferSPtr_, GetThisAllocator());
    Diagnostics::Validate(status);
}

MemoryBuffer::MemoryBuffer(__in KBuffer & buffer) : 
    isResizable_(false), 
    bufferSPtr_(&buffer), 
    binaryWriterSPtr_(nullptr), 
    position_(0)
{
}

MemoryBuffer::~MemoryBuffer()
{
}

NTSTATUS MemoryBuffer::Create(__in KAllocator & allocator, __out SPtr & result)
{
    NTSTATUS status;

    SPtr output = _new(MEMORY_STREAM_TAG, allocator) MemoryBuffer();

    if (!output)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = output->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    result = Ktl::Move(output);
    return STATUS_SUCCESS;
}

NTSTATUS MemoryBuffer::Create(__in ULONG capacity, __in KAllocator & allocator, __out SPtr & result)
{
    NTSTATUS status;

    SPtr output = _new(MEMORY_STREAM_TAG, allocator) MemoryBuffer(capacity);

    if (!output)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = output->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    result = Ktl::Move(output);
    return STATUS_SUCCESS;
}

NTSTATUS MemoryBuffer::Create(__in KBuffer & buffer, __in KAllocator & allocator, __out SPtr & result)
{
    NTSTATUS status;

    SPtr output = _new(MEMORY_STREAM_TAG, allocator) MemoryBuffer(buffer);

    if (!output)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = output->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    result = Ktl::Move(output);
    return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> MemoryBuffer::ReadAsync(
    __in KBuffer& buffer,
    __out ULONG & bytesRead,
    __in ULONG offsetIntoBuffer,
    __in ULONG count) 
{
    if (isResizable_)
    {
        // TODO: Implement if needed - Writing as necessary and then calling GetBuffer should cover a lot of cases
        co_return STATUS_NOT_IMPLEMENTED;
    }

    KBuffer::SPtr sourceBufferSPtr = bufferSPtr_;
    KBuffer::SPtr targetBufferSPtr = &buffer;

    if (position_ >= sourceBufferSPtr->QuerySize())
    {
        bytesRead = 0;
        co_return STATUS_SUCCESS;
    }

    if (count == 0)
    {
        count = targetBufferSPtr->QuerySize() - offsetIntoBuffer;
    }

    ULONG bytesAbleToRead = sourceBufferSPtr->QuerySize() - position_;

    bytesRead = count < bytesAbleToRead ? count : bytesAbleToRead;

    byte* dstBuffer = (byte *)targetBufferSPtr->GetBuffer();
    byte* srcBuffer = (byte *)sourceBufferSPtr->GetBuffer();
    KMemCpySafe(&dstBuffer[offsetIntoBuffer], bytesRead, &srcBuffer[position_], bytesRead);

    position_ += bytesRead;

    co_return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> MemoryBuffer::WriteAsync(
    __in KBuffer const & buffer,
    __in ULONG offsetIntoBuffer,
    __in ULONG count)
{
    KBuffer::CSPtr sourceBufferCSPtr = &buffer;
    if (count == 0)
    {
        count = sourceBufferCSPtr->QuerySize() - offsetIntoBuffer;
    }

    if (offsetIntoBuffer + count > sourceBufferCSPtr->QuerySize())
    {
        co_return STATUS_BUFFER_TOO_SMALL;
    }

    if (isResizable_)
    {
        KBuffer::SPtr chunkSPtr;
        NTSTATUS status = KBuffer::CreateOrCopyFrom(chunkSPtr, *sourceBufferCSPtr, offsetIntoBuffer, count, GetThisAllocator());
        if (!NT_SUCCESS(status))
        {
            co_return status;
        }
        binaryWriterSPtr_->Write(&*chunkSPtr, count);
        position_ = binaryWriterSPtr_->Position;
    }
    else
    {
        ULONG capacity = bufferSPtr_->QuerySize();
        ULONG endPosition = position_ + count;
        if (endPosition > capacity)
        {
            co_return STATUS_INSUFFICIENT_RESOURCES;
        }

        // Copy from buffer[offset:offset+count] to buffer_[position_:position_+count]
        byte* dstBuffer = (byte *)bufferSPtr_->GetBuffer();
        byte* srcBuffer = (byte *)sourceBufferCSPtr->GetBuffer();
        KMemCpySafe(&dstBuffer[position_], count, &srcBuffer[offsetIntoBuffer], count);

        position_ += count;
    }

    co_return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> MemoryBuffer::WriteAsync(__in byte value)
{
    KBuffer::SPtr sourceBufferSPtr;
    NTSTATUS status = KBuffer::Create(sizeof(byte), sourceBufferSPtr, GetThisAllocator());
    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    byte* buffer = static_cast<byte *>(sourceBufferSPtr->GetBuffer());
    *buffer = value;
    status = co_await WriteAsync(*sourceBufferSPtr, 0, sizeof(byte));
    co_return status;
}

ktl::Awaitable<NTSTATUS> MemoryBuffer::WriteAsync(__in ULONG32 value)
{
    KBuffer::SPtr sourceBufferSPtr;
    NTSTATUS status = KBuffer::Create(sizeof(ULONG32), sourceBufferSPtr, GetThisAllocator());
    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    ULONG32* buffer = static_cast<ULONG32 *>(sourceBufferSPtr->GetBuffer());
    *buffer = value;
    status = co_await WriteAsync(*sourceBufferSPtr, 0, sizeof(ULONG32));
    co_return status;
}

ktl::Awaitable<NTSTATUS> MemoryBuffer::FlushAsync()
{
    co_return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> MemoryBuffer::CloseAsync()
{
    co_return STATUS_SUCCESS;
}

LONGLONG MemoryBuffer::GetPosition() const
{
    return position_;
}

void MemoryBuffer::SetPosition(__in LONGLONG Position)
{
    position_ = static_cast<ULONG>(Position);
    if (isResizable_)
    {
        binaryWriterSPtr_->Position = position_;
    }
}

LONGLONG MemoryBuffer::GetLength() const
{
    // Note: For a resizable stream, just returns the current position
    return isResizable_ ? binaryWriterSPtr_->Position : bufferSPtr_->QuerySize();
}


KBuffer::SPtr MemoryBuffer::GetBuffer()
{
    return isResizable_ ? binaryWriterSPtr_->GetBuffer(0) : bufferSPtr_;
}

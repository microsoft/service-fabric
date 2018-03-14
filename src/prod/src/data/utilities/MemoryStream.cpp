// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::Utilities;

#define MEMORY_STREAM_TAG 'tsMM'

NTSTATUS MemoryStream::Create(
    __in KBuffer const& buffer, 
    __in KAllocator& allocator, 
    __out SPtr& result) noexcept
{
    result = _new(MEMORY_STREAM_TAG, allocator) MemoryStream(buffer);

    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        return (SPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> MemoryStream::CloseAsync()
{
    co_await suspend_never();
    co_return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> MemoryStream::ReadAsync(
    __in KBuffer& buffer, 
    __out ULONG& bytesRead,
    __in ULONG offsetIntoBuffer, 
    __in ULONG count)
{
    KShared$ApiEntry();

    ULONG currentPosition = static_cast<ULONG>(position_);
    if (currentPosition + count > buffer_->QuerySize())
    {
        throw ktl::Exception(K_STATUS_OUT_OF_BOUNDS);
    }

    buffer.CopyFrom(offsetIntoBuffer, *buffer_, currentPosition, count);
    bytesRead = count;

    position_ += count;

    co_await suspend_never();
    co_return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> MemoryStream::WriteAsync(
    __in KBuffer const& Buffer, 
    __in ULONG OffsetIntoBuffer,
    __in ULONG Count)
{
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(OffsetIntoBuffer);
    UNREFERENCED_PARAMETER(Count);

    throw ktl::Exception(STATUS_NOT_IMPLEMENTED);
}

ktl::Awaitable<NTSTATUS> MemoryStream::FlushAsync()
{
    throw ktl::Exception(STATUS_NOT_IMPLEMENTED);
}

LONGLONG MemoryStream::GetPosition() const
{
    return position_;
}

void MemoryStream::SetPosition(
    __in LONGLONG Position)
{
    position_ = Position;
}

LONGLONG MemoryStream::GetLength() const
{
    return static_cast<LONGLONG>(buffer_->QuerySize());
}

MemoryStream::MemoryStream(__in KBuffer const& buffer) noexcept
    : KObject<MemoryStream>()
    , KShared<MemoryStream>()
    , buffer_(&buffer)
    , position_(0)
{
}

MemoryStream::~MemoryStream()
{
}

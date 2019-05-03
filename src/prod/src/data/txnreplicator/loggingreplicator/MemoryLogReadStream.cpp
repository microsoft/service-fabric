// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Log;
using namespace Data::LoggingReplicator;
using namespace Data::Utilities;

MemoryLog::ReadStream::ReadStream(__in MemoryLog & parent)
    : log_(&parent)
    , readPosition_(0)
{
}

MemoryLog::ReadStream::~ReadStream()
{
    Dispose();
}

NTSTATUS MemoryLog::ReadStream::Create(
    __in MemoryLog& parent,
    __in KAllocator& allocator,
    __out ReadStream::SPtr & result)
{
    result = _new(MEMORYLOGMANAGER_TAG, allocator) ReadStream(parent);

    if (!result)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> MemoryLog::ReadStream::ReadAsync(
    __in KBuffer& Buffer,
    __out ULONG& BytesRead,
    __in ULONG OffsetIntoBuffer,
    __in ULONG Count)
{
    BytesRead = log_->InternalRead(Buffer, readPosition_, OffsetIntoBuffer, Count);
    readPosition_ += BytesRead;
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> MemoryLog::ReadStream::WriteAsync(
    __in KBuffer const & Buffer,
    __in ULONG OffsetIntoBuffer,
    __in ULONG Count)
{
    co_return STATUS_NOT_IMPLEMENTED;
}

ktl::Awaitable<NTSTATUS> MemoryLog::ReadStream::FlushAsync()
{
    co_return STATUS_NOT_IMPLEMENTED;
}

ktl::Awaitable<NTSTATUS> MemoryLog::ReadStream::CloseAsync()
{
    Dispose();
    co_return STATUS_SUCCESS;
}

void MemoryLog::ReadStream::Dispose()
{
    log_ = nullptr;
}

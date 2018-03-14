// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Log;

Common::StringLiteral const TraceComponent("LogManagerStream");

LogicalLogStream::LogicalLogStream(
    __in LogicalLog& parent,
    __in USHORT interfaceVersion,
    __in LONG sequentialAccessReadSize,
    __in ULONG streamArrayIndex,
    __in Data::Utilities::PartitionedReplicaId const & prId)
    : KObject()
    , KShared()
    , KWeakRefType<LogicalLogStream>()
    , PartitionedReplicaTraceComponent(prId)
    , parent_(&parent)
    , readContext_()
    , sequentialAccessReadSize_(interfaceVersion >= 1 ? sequentialAccessReadSize : 0)
    , interfaceVersion_(interfaceVersion)
    , streamArrayIndex_(streamArrayIndex)
    , addedToStreamArray_(FALSE)
{
}

LogicalLogStream::~LogicalLogStream()
{
    Cleanup();
}

VOID LogicalLogStream::Dispose()
{
    Cleanup();
}

VOID
LogicalLogStream::Cleanup()
{
    if (parent_ != nullptr)
    {
        if (addedToStreamArray_)
        {
            parent_->RemoveLogicalLogStream(*this);
            addedToStreamArray_ = FALSE;
        }
        
        parent_ = nullptr;
    }

    if (readContext_.ReadBuffer != nullptr)
    {
        readContext_.ReadBuffer->Dispose();
        readContext_.ReadBuffer = nullptr;
    }
}

NTSTATUS LogicalLogStream::Create(
    __in ULONG allocationTag,
    __in KAllocator& allocator,
    __in LogicalLog& parent,
    __in USHORT interfaceVersion,
    __in LONG sequentialAccessReadSize,
    __in ULONG streamArrayIndex,
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __out LogicalLogStream::SPtr& stream)
{
    NTSTATUS status;

    stream = _new(allocationTag, allocator) LogicalLogStream(parent, interfaceVersion, sequentialAccessReadSize, streamArrayIndex, prId);
    if (!stream)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, nullptr, 0, 0);

        WriteError(
            TraceComponent,
            "{0} - Create - Failed to allocate LogicalLogStream",
            prId.TraceId);

        return status;
    }

    status = stream->Status();
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - Create - LogicalLogStream constructed with error status. Status: {1}",
            prId.TraceId,
            status);

        return Ktl::Move(stream)->Status();
    }

    return STATUS_SUCCESS;
}

VOID LogicalLogStream::InvalidateReadAhead()
{
    KShared$ApiEntry();

    readContext_.ReadBuffer = nullptr;
}

VOID LogicalLogStream::SetSequentialAccessReadSize(__in LONG sequentialAccessReadSize)
{
    KShared$ApiEntry();

    if (interfaceVersion_ >= 1)
    {
        //
        // Can only use sequential mode when interface is version 1 or higher
        //
        sequentialAccessReadSize_ = sequentialAccessReadSize;
    }
    else
    {
        WriteWarning(
            TraceComponent,
            "{0} - SetSequentialAccessReadSize - ignoring call to SetSequentialAccessReadSize as interface version is < 1. interfaceVersion: {1}",
            TraceId,
            interfaceVersion_);
    }
}

LONGLONG LogicalLogStream::GetLength() const { return parent_->WritePosition; }

Awaitable<NTSTATUS> LogicalLogStream::CloseAsync()
{
    KShared$ApiEntry();

    Cleanup();

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> LogicalLogStream::FlushAsync()
{
    KShared$ApiEntry();

    readContext_.ReadBuffer = nullptr;

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> LogicalLogStream::ReadAsync(
    __in KBuffer& buffer,
    __out ULONG& bytesRead,
    __in ULONG offsetIntoBuffer,
    __in ULONG count)
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_SUCCESS;

    LONG bytesToRead = sequentialAccessReadSize_;
    ReadAsyncResults results;

    status = co_await parent_->InternalReadAsync(
        readContext_,
        buffer,
        offsetIntoBuffer,
        count,
        bytesToRead,
        CancellationToken::None,
        results);

    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceComponent,
            "{0} - ReadAsync - InternalRead failed.  offsetIntoBuffer: {1}, count: {2}, bytesToRead: {3}, status: {4}",
            offsetIntoBuffer,
            count,
            bytesToRead,
            status);

        co_return status;
    }

    readContext_ = results.Context;
    bytesRead = results.TotalDone;
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> LogicalLogStream::WriteAsync(
    __in KBuffer const &,
    __in ULONG,
    __in ULONG)
{
    KShared$ApiEntry();

    co_return STATUS_NOT_IMPLEMENTED;
}

LONGLONG LogicalLogStream::Seek(
    __in LONGLONG offset,
    __in Common::SeekOrigin::Enum seekOrigin)
{
    KShared$ApiEntry();

    LONGLONG newReadLocation = -1;

    switch(seekOrigin)
    {
    case Common::SeekOrigin::Begin:
        newReadLocation = offset;
        break;

    case Common::SeekOrigin::End:
        newReadLocation = parent_->WritePosition + offset;
        break;

    case Common::SeekOrigin::Current:
        newReadLocation = readContext_.ReadLocation + offset;
        break;
    default: 
        KInvariant(FALSE);
        break;
    }

    if (readContext_.ReadBuffer != nullptr)
    {
        readContext_.ReadBuffer->Dispose();
        readContext_.ReadBuffer = nullptr;
    }

    if (readContext_.NextReadTask != nullptr)
    {
        readContext_.NextReadTask->InvalidateRead();
    }

    readContext_.ReadLocation = newReadLocation;
    return readContext_.ReadLocation;
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Log;

NTSTATUS LogicalLogReadTask::Create(
    __in ULONG allocationTag,
    __in KAllocator& allocator,
    __in KtlLogStream& underlyingStream,
    __out LogicalLogReadTask::SPtr& readTask)
{
    NTSTATUS status;

    readTask = _new(allocationTag, allocator) LogicalLogReadTask(underlyingStream);
    if (!readTask)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, nullptr, 0, 0);
        return status;
    }

    status = readTask->Status();
    if (!NT_SUCCESS(status))
    {
        return Ktl::Move(readTask)->Status();
    }

    return STATUS_SUCCESS;
}

FAILABLE
LogicalLogReadTask::LogicalLogReadTask(__in KtlLogStream& underlyingStream)
    : underlyingStream_(&underlyingStream),
      isValid_(FALSE),
      endOffset_(-1),
      offset_(-1),
      length_(-1)
{
    AwaitableCompletionSource<NTSTATUS>::SPtr readTcs;
    NTSTATUS status = AwaitableCompletionSource<NTSTATUS>::Create(GetThisAllocator(), GetThisAllocationTag(), readTcs);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    readTcs_ = Ktl::Move(readTcs);
}

LogicalLogReadTask::~LogicalLogReadTask()
{
}

BOOLEAN LogicalLogReadTask::IsInRange(__in LONGLONG offset) const
{
    return ((offset >= offset_) && (offset <= endOffset_));
}

LONG LogicalLogReadTask::GetLength() const
{
    return length_;
}

LONGLONG LogicalLogReadTask::GetOffset() const
{
    return offset_;
}

BOOLEAN LogicalLogReadTask::IsValid() const
{
    return isValid_;
}

VOID LogicalLogReadTask::InvalidateRead()
{
    isValid_ = FALSE;
}

BOOLEAN LogicalLogReadTask::HandleWriteThrough(__in LONGLONG writeOffset, __in LONG writeLength)
{
    LONGLONG endOffset = writeOffset + writeLength;

    if ((isValid_) &&
        (IsInRange(writeOffset) || IsInRange(endOffset)))
    {
        isValid_ = false;
    }

    return isValid_;
}

VOID LogicalLogReadTask::StartRead(__in LONGLONG offset, __in LONG length)
{
    offset_ = offset;
    length_ = length;
    endOffset_ = offset_ + length_;
    isValid_ = TRUE;

    StartReadInternal();
}

Task LogicalLogReadTask::StartReadInternal()
{
    KCoShared$ApiEntry(); // explicitly keep this alive

    NTSTATUS status = STATUS_SUCCESS;
    KtlLogStream::AsyncMultiRecordReadContext::SPtr readContext;

    KIoBuffer::SPtr metadataBuffer;
    const ULONG metadataSize = 4096;
    PVOID metadataBufferPtr;
    status = KIoBuffer::CreateSimple(metadataSize, metadataBuffer, metadataBufferPtr, GetThisAllocator());
    if (!NT_SUCCESS(status))
    {
        readTcs_->SetResult(status);
        co_return;
    }

    KIoBuffer::SPtr ioBuffer;
    ULONG paddedBytesToRead = ((length_ + (metadataSize - 1)) / metadataSize) * metadataSize;
    PVOID ioBufferPtr;
    status = KIoBuffer::CreateSimple(paddedBytesToRead, ioBuffer, ioBufferPtr, GetThisAllocator());
    if (!NT_SUCCESS(status))
    {
        readTcs_->SetResult(status);
        co_return;
    }

    status = underlyingStream_->CreateAsyncMultiRecordReadContext(readContext);
    if (!NT_SUCCESS(status))
    {
        readTcs_->SetResult(status);
        co_return;
    }

    status = co_await readContext->ReadAsync(offset_ + 1, *metadataBuffer, *ioBuffer, nullptr);
    if (!NT_SUCCESS(status))
    {
        readTcs_->SetResult(status);
        co_return;
    }

    results_.ResultingAsn = 0;
    results_.Version = 0;
    results_.ResultingMetadataSize = 0;
    results_.ResultingMetadata = Ktl::Move(metadataBuffer);
    results_.ResultingIoPageAlignedBuffer = Ktl::Move(ioBuffer);

    readTcs_->SetResult(STATUS_SUCCESS);
    co_return;
}

Awaitable<NTSTATUS> LogicalLogReadTask::GetReadResults(__out PhysicalLogStreamReadResults& results)
{
    NTSTATUS status = co_await readTcs_->GetAwaitable();
    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    results = results_;
    co_return STATUS_SUCCESS;
}

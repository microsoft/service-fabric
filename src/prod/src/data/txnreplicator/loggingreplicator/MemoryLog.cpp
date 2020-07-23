// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Log;
using namespace Data::LoggingReplicator;
using namespace Data::Utilities;

ULONG MemoryLog::ChunkSize = 64 * 1024;

MemoryLog::MemoryLog(__in Data::Utilities::PartitionedReplicaId const & traceId)
    : head_(0)
    , tail_(0)
    , PartitionedReplicaTraceComponent(traceId)
{
    NTSTATUS status = ConcurrentHashTable<ULONG, KBuffer::SPtr>::Create(K_DefaultHashFunction, 32, this->GetThisAllocator(), chunksSPtr_);

    if (!NT_SUCCESS(status))
    {
        this->SetConstructorStatus(status);
        return;
    }

    KBuffer::SPtr newChunk = nullptr;
    status = KBuffer::Create(static_cast<ULONG>(ChunkSize), newChunk, GetThisAllocator(), MEMORYLOGMANAGER_TAG);

    if (!NT_SUCCESS(status))
    {
        this->SetConstructorStatus(status);
        return;
    }

    bool added = chunksSPtr_->TryAdd(0, newChunk);
    ASSERT_IFNOT(added, "{0}: Failed to add first chunk", TraceId);
}

MemoryLog::~MemoryLog()
{
}

NTSTATUS MemoryLog::Create(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in KAllocator& allocator,
    __out MemoryLog::SPtr& memoryLogicalLog)
{
    NTSTATUS status;
    MemoryLog::SPtr context;

    context = _new(MEMORYLOGMANAGER_TAG, allocator) MemoryLog(traceId);
    if (!context)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, nullptr, 0, 0);
        return status;
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    memoryLogicalLog = Ktl::Move(context);

    return STATUS_SUCCESS;
}

void MemoryLog::Dispose()
{
    chunksSPtr_ = nullptr;
}

Awaitable<NTSTATUS> MemoryLog::CloseAsync(__in CancellationToken const &)
{
    co_return STATUS_SUCCESS;
}

void MemoryLog::Abort()
{
    Dispose();
}

BOOLEAN MemoryLog::GetIsFunctional()
{
    return chunksSPtr_ != nullptr;
}

LONGLONG MemoryLog::GetLength() const
{
    ULONG64 length = tail_ - head_;
    ASSERT_IFNOT(length < LONGLONG_MAX, "{0}: length={0} < {1}", length, LONGLONG_MAX);
    return static_cast<LONGLONG>(length);
}

LONGLONG MemoryLog::GetWritePosition() const
{
    return tail_;
}

LONGLONG MemoryLog::GetReadPosition() const
{
    throw Exception(STATUS_NOT_IMPLEMENTED);
}

LONGLONG MemoryLog::GetHeadTruncationPosition() const
{
    throw Exception(STATUS_NOT_IMPLEMENTED);
}

LONGLONG MemoryLog::GetMaximumBlockSize() const
{
    throw Exception(STATUS_NOT_IMPLEMENTED);
}

ULONG MemoryLog::GetMetadataBlockHeaderSize() const
{
    throw Exception(STATUS_NOT_IMPLEMENTED);
}

ULONGLONG MemoryLog::GetSize() const
{
    LONGLONG length = GetLength();
    return static_cast<ULONGLONG>(length);
}

ULONGLONG MemoryLog::GetSpaceRemaining() const
{
    return ULONGLONG_MAX - GetSize();
}

NTSTATUS MemoryLog::CreateReadStream(__out ILogicalLogReadStream::SPtr& Stream, __in LONG SequentialAccessReadSize)
{
    UNREFERENCED_PARAMETER(SequentialAccessReadSize);

    ReadStream::SPtr stream = nullptr;
    NTSTATUS status = ReadStream::Create(*this, GetThisAllocator(), stream);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    Stream = stream.RawPtr();

    return STATUS_SUCCESS;
}

ULONG MemoryLog::InternalRead(__in KBuffer & buffer, __in ULONG64 readPosition, __in ULONG offset, __in ULONG count)
{
    if (readPosition < head_ || readPosition >= tail_)
    {
        return 0;
    }
    
    ULONG numBytesLeftToRead = count;
    
    // Requesting to read more than we have - read up until tail
    if (readPosition + numBytesLeftToRead > tail_)
    {
        ULONG64 bytesToRead64 = tail_ - readPosition;
        ASSERT_IFNOT(bytesToRead64 < ULONG_MAX, "Cannot read more than ULONG_MAX bytes at once. Requested={0} bytes", bytesToRead64);
        numBytesLeftToRead = static_cast<ULONG>(bytesToRead64);
    }

    ULONG chunkIndex = static_cast<ULONG>(readPosition / ChunkSize);

    ULONG offsetWithinChunk = readPosition % ChunkSize;

    ULONG bytesRead = numBytesLeftToRead;

    do
    {
        ULONG readSize = (offsetWithinChunk + numBytesLeftToRead) > ChunkSize ? (ChunkSize - offsetWithinChunk) : numBytesLeftToRead;
        CopyToBufferFromChunk(buffer, offset, chunkIndex, offsetWithinChunk, readSize);

        offsetWithinChunk = 0; // Subsequent reads start from the beginning of the chunk
        numBytesLeftToRead -= readSize;
        offset += readSize;
        chunkIndex++;
    } while (numBytesLeftToRead > 0);

    return bytesRead;
}

void MemoryLog::CopyToBufferFromChunk(
    __in KBuffer & buffer,
    __in ULONG offset,
    __in ULONG chunkIndex,
    __in ULONG chunkStartPosition,
    __in ULONG count)
{
    KBuffer::SPtr chunkBuffer = nullptr;
    bool found = chunksSPtr_->TryGetValue(chunkIndex, chunkBuffer);
    ASSERT_IFNOT(found && chunkBuffer != nullptr, "{0}: Unable to get buffer for chunk at index {1}", TraceId, chunkIndex);

    buffer.CopyFrom(offset, *chunkBuffer, chunkStartPosition, count);
}

void MemoryLog::SetSequentialAccessReadSize(__in ILogicalLogReadStream& LogStream, __in LONG SequentialAccessReadSize)
{
    UNREFERENCED_PARAMETER(LogStream);
    UNREFERENCED_PARAMETER(SequentialAccessReadSize);
}

Awaitable<NTSTATUS> MemoryLog::ReadAsync(
    __out LONG& BytesRead,
    __in KBuffer& Buffer,
    __in ULONG Offset,
    __in ULONG Count,
    __in ULONG BytesToRead,
    __in CancellationToken const & cancellationToken)
{
    co_return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS MemoryLog::SeekForRead(__in LONGLONG, __in Common::SeekOrigin::Enum)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS MemoryLog::UpdateTail(__in ULONG appendSize)
{
    NTSTATUS status = STATUS_SUCCESS;

#if DEBUG
    ULONG64 chunkEnd = GetChunkEndOffset(tail_);
    ASSERT_IFNOT(appendSize <= tail_ - chunkEnd);
#endif

    tail_ += appendSize;

    if (tail_ % ChunkSize == 0)
    {
        KBuffer::SPtr newChunk = nullptr;
        status = KBuffer::Create(ChunkSize, newChunk, GetThisAllocator(), MEMORYLOGMANAGER_TAG);
        RETURN_ON_FAILURE(status);

        if (newChunk == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ULONG newChunkIndex = static_cast<ULONG>(tail_ / ChunkSize);
        
        bool added = chunksSPtr_->TryAdd(newChunkIndex, newChunk);
        if (!added)
        {
            status = STATUS_OBJECT_NAME_COLLISION;
        }
    }

    return status;
}

void MemoryLog::CopyFromBufferToTailChunk(__in KBuffer const & buffer, __in ULONG offset, __in ULONG count)
{
    ULONG chunkIndex = static_cast<ULONG>(tail_ / ChunkSize);
    ULONG offsetInChunk = static_cast<ULONG>(tail_ % ChunkSize);

    KBuffer::SPtr chunkBuffer = nullptr;
    bool found = chunksSPtr_->TryGetValue(chunkIndex, chunkBuffer);
    ASSERT_IFNOT(found && chunkBuffer != nullptr, "{0}: Unable to get buffer for chunk at index {1}", TraceId, chunkIndex);

    chunkBuffer->CopyFrom(offsetInChunk, buffer, offset, count);
}

Awaitable<NTSTATUS> MemoryLog::AppendAsync(
    __in KBuffer const & Buffer,
    __in LONG OffsetIntoBuffer,
    __in ULONG Count,
    __in CancellationToken const & cancellationToken)
{
    ASSERT_IFNOT(OffsetIntoBuffer >= 0, "OfsetIntoBuffer={0} > 0", OffsetIntoBuffer);
    ULONG uOffsetIntoBuffer = static_cast<ULONG>(OffsetIntoBuffer);

    ULONG64 offsetOfTailChunkEnd = GetChunkEndOffset(tail_);

    ULONG appendSize = __min(static_cast<ULONG>(offsetOfTailChunkEnd - tail_), Count);

    do
    {
        CopyFromBufferToTailChunk(Buffer, uOffsetIntoBuffer, appendSize);
        NTSTATUS status = UpdateTail(appendSize);
        CO_RETURN_ON_FAILURE(status);

        Count -= appendSize;
        uOffsetIntoBuffer += appendSize;
        appendSize = __min(Count, ChunkSize);
    } while (Count > 0);

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> MemoryLog::FlushWithMarkerAsync(__in CancellationToken const &)
{
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> MemoryLog::FlushAsync(__in CancellationToken const &) 
{
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> MemoryLog::TruncateHead(__in LONGLONG StreamOffset) 
{
    ULONG64 uStreamOffset = static_cast<ULONG64>(StreamOffset);

    if (uStreamOffset < head_ || uStreamOffset > tail_)
    {
        // TruncateHead is best effort - this could be a duplicate request
        co_return STATUS_SUCCESS;
    }

    ULONG truncateStartChunkIndex = static_cast<ULONG>(head_ / ChunkSize);
    ULONG truncateEndChunkIndex = static_cast<ULONG>(uStreamOffset / ChunkSize);

    ASSERT_IFNOT(truncateStartChunkIndex <= truncateEndChunkIndex, "{0}: Invalid StreamOffset during TruncateHead {1}. Head = {2}, Tail = {3}", TraceId, uStreamOffset, head_, tail_);
    head_ = uStreamOffset;

    if (truncateStartChunkIndex == truncateEndChunkIndex)
    {
        // nothing to clean up as there are no chunks before the current head chunk
        co_return STATUS_SUCCESS;
    }
    
    for (ULONG chunkIndex = truncateStartChunkIndex; chunkIndex < truncateEndChunkIndex; chunkIndex++)
    {
        bool removed = chunksSPtr_->Remove(chunkIndex);
        ASSERT_IFNOT(removed, "{0}: Failed to remove chunk index = {1}", TraceId, chunkIndex);
    }

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> MemoryLog::TruncateTail(__in LONGLONG StreamOffset, __in CancellationToken const &) 
{
    ULONG64 uStreamOffset = static_cast<ULONG64>(StreamOffset);

    if (uStreamOffset < head_ || uStreamOffset > tail_)
    {
        // TruncateTail is best effort - could be a duplicate request
        co_return STATUS_SUCCESS;
    }

    ULONG truncateStartChunkIndex = static_cast<ULONG>(tail_ / ChunkSize);
    ULONG truncateEndChunkIndex = static_cast<ULONG>(uStreamOffset / ChunkSize);

    ASSERT_IFNOT(truncateStartChunkIndex >= truncateEndChunkIndex, "{0}: Invalid StreamOffset during TruncateHead {1}. Head = {2}, Tail = {3}", TraceId, uStreamOffset, head_, tail_);
    tail_ = uStreamOffset;

    if (truncateStartChunkIndex == truncateEndChunkIndex)
    {
        // nothing to clean up as there are no chunks after the current tail chunk
        co_return STATUS_SUCCESS;
    }

    for (ULONG chunkIndex = truncateStartChunkIndex; chunkIndex > truncateEndChunkIndex; chunkIndex--)
    {
        bool removed = chunksSPtr_->Remove(chunkIndex);
        ASSERT_IFNOT(removed, "{0}: Failed to remove chunk index = {1}", TraceId, chunkIndex);
    }

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> MemoryLog::WaitCapacityNotificationAsync(__in ULONG PercentOfSpaceUsed, __in CancellationToken const & cancellationToken) 
{
    co_return STATUS_NOT_IMPLEMENTED;
}

Awaitable<NTSTATUS> MemoryLog::WaitBufferFullNotificationAsync(__in CancellationToken const & cancellationToken) 
{
    co_return STATUS_NOT_IMPLEMENTED;
}

Awaitable<NTSTATUS> MemoryLog::ConfigureWritesToOnlyDedicatedLogAsync(__in CancellationToken const & cancellationToken) 
{
    co_return STATUS_NOT_IMPLEMENTED;
}

Awaitable<NTSTATUS> MemoryLog::ConfigureWritesToSharedAndDedicatedLogAsync(__in CancellationToken const & cancellationToken) 
{
    co_return STATUS_NOT_IMPLEMENTED;
}

Awaitable<NTSTATUS> MemoryLog::QueryLogUsageAsync(__out ULONG& PercentUsage, __in CancellationToken const & cancellationToken) 
{
    co_return STATUS_NOT_IMPLEMENTED;
}

ULONG64 MemoryLog::GetChunkEndOffset(ULONG64 offset)
{
    ULONG64 quotient = offset / ChunkSize;
    return (quotient + 1) * ChunkSize;
}



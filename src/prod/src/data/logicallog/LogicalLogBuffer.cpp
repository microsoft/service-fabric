// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::Log;

Common::StringLiteral const TraceComponent("LogicalLogBuffer");

LogicalLogBuffer::LogicalLogBuffer(
    __in ULONG blockMetadataSize,
    __in LONGLONG startingStreamPosition,
    __in KIoBuffer& metadataBuffer,
    __in KIoBuffer& pageAlignedKIoBuffer,
    __in Data::Utilities::PartitionedReplicaId const & prId)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(prId)
    , metadataSize_(blockMetadataSize)
    , metadataKIoBuffer_(&metadataBuffer)
    , pageAlignedKIoBuffer_(&pageAlignedKIoBuffer)
    , streamBlockHeader_(nullptr)
    , pageAlignedKIoBufferView_(nullptr)
    , offsetToData_(ULONG_MAX)
{
    NTSTATUS status = STATUS_SUCCESS;

    KIoBuffer::SPtr combined;
    KIoBufferView::SPtr view;
    
    status = KIoBuffer::CreateEmpty(combined, GetThisAllocator(), GetThisAllocationTag());
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    status = KIoBufferView::Create(view, GetThisAllocator(), GetThisAllocationTag());
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    combinedKIoBuffer_ = Ktl::Move(combined);
    combinedView_ = Ktl::Move(view);

    status = OpenForRead(startingStreamPosition);
    if (!NT_SUCCESS(status))
    {
        combinedView_ = nullptr;
        combinedKIoBuffer_ = nullptr;

        SetConstructorStatus(status);
        return;
    }
}

LogicalLogBuffer::LogicalLogBuffer(
    __in ULONG blockMetadataSize,
    __in ULONG maxBlockSize,
    __in LONGLONG streamLocation,
    __in LONGLONG opNumber,
    __in KGuid streamId,
    __in Data::Utilities::PartitionedReplicaId const & prId)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(prId)
    , metadataSize_(blockMetadataSize)
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN res = TRUE;

    KIoBuffer::SPtr metadata;
    KIoBuffer::SPtr pageAligned;
    KIoBuffer::SPtr combined;
    KIoBuffer::SPtr pageAlignedView;

    VOID* b;
    status = KIoBuffer::CreateSimple(
        KLogicalLogInformation::FixedMetadataSize,
        metadata,
        b,
        GetThisAllocator(),
        GetThisAllocationTag());
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    //
    // Allocate the write KIoBuffer to be the full block size requested by the caller despite the fact that some
    // data will live in the metadata portion. Consider the fact that when the block is completely full, there will be some
    // amount of data in the metadata and then the data portion will be full as well except for a gap at the end of the block
    // which is the size of the data in the metadata portion. When rounding up we will need this last block despite it not being
    // completely full.
    //
    status = KIoBuffer::CreateSimple(
        maxBlockSize,
        pageAligned,
        b,
        GetThisAllocator(),
        GetThisAllocationTag());
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    status = KIoBuffer::CreateEmpty(combined, GetThisAllocator(), GetThisAllocationTag());
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    status = combined->AddIoBufferReference(*metadata, 0, KLogicalLogInformation::FixedMetadataSize);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    status = combined->AddIoBufferReference(
        *pageAligned,
        0,
        maxBlockSize - KLogicalLogInformation::FixedMetadataSize);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    status = KIoBuffer::CreateEmpty(pageAlignedView, GetThisAllocator(), GetThisAllocationTag());
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    combinedBufferStream_ = { *combined, blockMetadataSize };

    KLogicalLogInformation::MetadataBlockHeader* mdHdr;
    mdHdr = static_cast<KLogicalLogInformation::MetadataBlockHeader*>(combinedBufferStream_.GetBufferPointer());
    metadataBlockHeader_ = mdHdr;
    res = combinedBufferStream_.Skip(sizeof(KLogicalLogInformation::MetadataBlockHeader));
    KInvariant(res); // Unexpected Skip failure

    streamBlockHeader_ = static_cast<KLogicalLogInformation::StreamBlockHeader*>(combinedBufferStream_.GetBufferPointer());
    mdHdr->OffsetToStreamHeader = combinedBufferStream_.GetPosition();

    // Position the stream at 1st byte of user record
    res = combinedBufferStream_.Skip(sizeof(KLogicalLogInformation::StreamBlockHeader));
    KInvariant(res); // Unexpected skip failure

    streamBlockHeader_->Signature = KLogicalLogInformation::StreamBlockHeader::Sig;
    streamBlockHeader_->StreamOffset = streamLocation + 1;
    streamBlockHeader_->HighestOperationId = opNumber;
    streamBlockHeader_->StreamId = streamId;
    streamBlockHeader_->HeaderCRC64 = 0;
    streamBlockHeader_->DataCRC64 = 0;
    streamBlockHeader_->DataSize = 0;
    streamBlockHeader_->Reserved = 0;

    offsetToData_ = mdHdr->OffsetToStreamHeader + sizeof(KLogicalLogInformation::StreamBlockHeader);

    metadataKIoBuffer_ = Ktl::Move(metadata);
    pageAlignedKIoBuffer_ = Ktl::Move(pageAligned);
    combinedKIoBuffer_ = Ktl::Move(combined);
    pageAlignedKIoBufferView_ = Ktl::Move(pageAlignedView);
}

LogicalLogBuffer::~LogicalLogBuffer()
{
}

NTSTATUS LogicalLogBuffer::CreateWriteBuffer(
    __in ULONG allocationTag,
    __in KAllocator& allocator,
    __in ULONG blockMetadataSize,
    __in ULONG maxBlockSize,
    __in LONGLONG streamLocation,
    __in LONGLONG opNumber,
    __in KGuid streamId,
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __out LogicalLogBuffer::SPtr& writeBuffer)
{
    NTSTATUS status;

    writeBuffer = _new(allocationTag, allocator) LogicalLogBuffer(
        blockMetadataSize,
        maxBlockSize,
        streamLocation,
        opNumber,
        streamId,
        prId);

    if (!writeBuffer)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, nullptr, 0, 0);
        return status;
    }

    status = writeBuffer->Status();
    if (!NT_SUCCESS(status))
    {
        return Ktl::Move(writeBuffer)->Status();
    }

    return STATUS_SUCCESS;
}

NTSTATUS LogicalLogBuffer::CreateReadBuffer(
    __in ULONG allocationTag,
    __in KAllocator& allocator,
    __in ULONG blockMetadataSize,
    __in LONGLONG startingStreamPosition,
    __in KIoBuffer& metadataBuffer,
    __in KIoBuffer& pageAlignedKIoBuffer,
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __out LogicalLogBuffer::SPtr& readBuffer)
{
    NTSTATUS status;

    readBuffer = _new(allocationTag, allocator) LogicalLogBuffer(
        blockMetadataSize,
        startingStreamPosition,
        metadataBuffer,
        pageAlignedKIoBuffer,
        prId);

    if (!readBuffer)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, nullptr, 0, 0);
        return status;
    }

    status = readBuffer->Status();
    if (!NT_SUCCESS(status))
    {
        return Ktl::Move(readBuffer)->Status();
    }

    return STATUS_SUCCESS;
}

NTSTATUS LogicalLogBuffer::OpenForRead(__in LONGLONG streamPosition)
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN res = TRUE;

    combinedKIoBuffer_->Clear();
    status = combinedKIoBuffer_->AddIoBufferReference(
        *metadataKIoBuffer_,
        0,
        metadataKIoBuffer_->QuerySize());
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - OpenForRead - Failed to add reference to combined buffer for metadata. Size {1}, Status {2}",
            TraceId,
            metadataKIoBuffer_->QuerySize(),
            status);

        return status;
    }

    status = combinedKIoBuffer_->AddIoBufferReference(
        *pageAlignedKIoBuffer_,
        0,
        pageAlignedKIoBuffer_->QuerySize());
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - OpenForRead - Failed to add reference to combined buffer for pagealignedbuffer. Size {1}, Status {2}",
            TraceId,
            pageAlignedKIoBuffer_->QuerySize(),
            status);

        return status;
    }

    combinedBufferStream_ = { *combinedKIoBuffer_, metadataSize_ };

    res = combinedBufferStream_.Reuse(*combinedKIoBuffer_, metadataSize_);
    KInvariant(res);

    // Parse the log stream physical record
    KLogicalLogInformation::MetadataBlockHeader* mdHdr;
    mdHdr = static_cast<KLogicalLogInformation::MetadataBlockHeader*>(combinedBufferStream_.GetBufferPointer());

    res = combinedBufferStream_.PositionTo(mdHdr->OffsetToStreamHeader);
    KInvariant(res);

    streamBlockHeader_ = static_cast<KLogicalLogInformation::StreamBlockHeader*>(combinedBufferStream_.GetBufferPointer());

    // Position the stream at 1st byte of user record
    res = combinedBufferStream_.Skip(sizeof(KLogicalLogInformation::StreamBlockHeader));
    KInvariant(res);

    offsetToData_ = mdHdr->OffsetToStreamHeader + sizeof(KLogicalLogInformation::StreamBlockHeader);

    // Compute position and limits of the KIoBufferStream
    KInvariant(streamBlockHeader_->StreamOffset > 0);
    LONGLONG recordOffsetToDesiredPosition = streamPosition - (streamBlockHeader_->StreamOffset - 1);

    if ((recordOffsetToDesiredPosition < 0) || (recordOffsetToDesiredPosition >= streamBlockHeader_->DataSize))
    {
        WriteInfo(
            TraceComponent,
            "{0} - OpenForRead - streamPosition: {1}, streamOffset: {2}, dataSize: {3}, recordOffsetToDesiredPosition: {4}, headTruncationPoint: {5}, highestOperationId: {6}",
            TraceId,
            streamPosition,
            streamBlockHeader_->StreamOffset,
            streamBlockHeader_->DataSize,
            recordOffsetToDesiredPosition,
            streamBlockHeader_->HeadTruncationPoint,
            streamBlockHeader_->HighestOperationId);
    }

    // Rather than building a "size" into the stream, supply the stream with a properly-sized KIoBufferView
    combinedView_->SetView(*combinedKIoBuffer_, 0, offsetToData_ + streamBlockHeader_->DataSize);

    LONGLONG initialOffset = recordOffsetToDesiredPosition + offsetToData_;
    KAssert(initialOffset >= 0);
    KAssert(initialOffset <= ULONG_MAX);
    res = combinedBufferStream_.Reuse(*combinedView_, static_cast<ULONG>(initialOffset));
    KInvariant(res);

    return STATUS_SUCCESS;
}

VOID LogicalLogBuffer::Dispose()
{
    Dispose(TRUE);
}

VOID LogicalLogBuffer::Dispose(__in BOOLEAN isDisposing)
{
    if (isDisposing)
    {
        if (metadataKIoBuffer_ != nullptr)
        {
            metadataKIoBuffer_->Clear();
        }

        if (pageAlignedKIoBuffer_ != nullptr)
        {
            pageAlignedKIoBuffer_->Clear();
        }

        if (combinedKIoBuffer_ != nullptr)
        {
            combinedKIoBuffer_->Clear();
        }

        if (combinedView_ != nullptr)
        {
            combinedView_->Clear();
        }

        combinedBufferStream_.Reset();

        streamBlockHeader_ = nullptr;
        metadataBlockHeader_ = nullptr;
    }
}

NTSTATUS LogicalLogBuffer::Intersects(
    __out BOOLEAN& intersects,
    __in LONGLONG streamOffset,
    __in LONG size) const
{
    if (size <= 0)
    {
        WriteWarning(
            TraceComponent,
            "{0} - Intersects - size <= 0. size {1}",
            TraceId,
            size);

        return K_STATUS_OUT_OF_BOUNDS;
    }

    if (!IsSealed)
    {
        LONGLONG sizeOfUserDataInBuffer = combinedBufferStream_.GetPosition() - offsetToData_;
        LONGLONG bufferBaseStreamOffset = streamBlockHeader_->StreamOffset - 1;

        intersects = !((streamOffset >= (bufferBaseStreamOffset + sizeOfUserDataInBuffer) ||
            ((streamOffset + size) <= bufferBaseStreamOffset)));
    }
    else
    {
        intersects = FALSE;
    }

    return STATUS_SUCCESS;
}

NTSTATUS LogicalLogBuffer::SetPosition(__in LONGLONG bufferOffset)
{
    if ((bufferOffset > ULONG_MAX) || (bufferOffset < 0))
    {
        WriteWarning(
            TraceComponent,
            "{0} - SetPosition - invalid bufferOffset (> ULONG_MAX or < 0). bufferOffset: {1}, ULONG_MAX: {2}",
            TraceId,
            bufferOffset,
            ULONG_MAX);

        return K_STATUS_OUT_OF_BOUNDS;
    }

    ULONG requestedOffset = static_cast<ULONG>(bufferOffset);
    if (requestedOffset >= (combinedBufferStream_.Length - offsetToData_))
    {
        WriteWarning(
            TraceComponent,
            "{0} - SetPosition - invalid bufferOffset (past data region). bufferOffset: {1}, combinedBufferStream.Length: {2}, offsetToData: {3}",
            TraceId,
            bufferOffset,
            combinedBufferStream_.Length,
            offsetToData_);

        return K_STATUS_OUT_OF_BOUNDS;
    }

    BOOLEAN res = combinedBufferStream_.PositionTo(offsetToData_ + requestedOffset);
    KInvariant(res);  // Internal size or offset inconsistency

    return STATUS_SUCCESS;
}


NTSTATUS LogicalLogBuffer::Put(
    __out ULONG& bytesWritten,
    __in BYTE const * toWrite,
    __in ULONG size)
{
    NTSTATUS status = STATUS_SUCCESS;

    ULONG todo = min(size, combinedBufferStream_.Length - combinedBufferStream_.Position);
    if (todo > 0)
    {
        status = combinedBufferStream_.Put(toWrite, todo);
        if (!NT_SUCCESS(status))
        {
            WriteWarning(
                TraceComponent,
                "{0} - Put - combinedBufferStream put failure.  size: {1}, combinedBufferStream.Length: {2}, combinedBufferStream.GetPosition: {3}, todo: {4}, status: {5}",
                TraceId,
                size,
                combinedBufferStream_.Length,
                combinedBufferStream_.Position,
                todo,
                status);

            return status;
        }
    }

    bytesWritten = todo;
    return STATUS_SUCCESS;
}

NTSTATUS LogicalLogBuffer::Put(
    __out ULONG& bytesWritten,
    __in BYTE const * buffer,
    __in LONG offset,
    __in ULONG count)
{
    KAssert(offset >= 0);
    NTSTATUS status = STATUS_SUCCESS;
    ULONG done;
    BYTE const * inPtr = &buffer[offset];

    status = Put(done, inPtr, count);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceComponent,
            "{0} - Put - internal put failure. offset: {1}, count: {2}, status: {3}",
            TraceId,
            offset,
            count,
            status);

        return status;
    }

    bytesWritten = done;
    return STATUS_SUCCESS;
}

NTSTATUS LogicalLogBuffer::Get(
    __out PBYTE dest,
    __out ULONG& bytesRead,
    __in ULONG size)
{
    ULONG todo = min(size, combinedBufferStream_.Length - combinedBufferStream_.GetPosition());
    if (todo > 0)
    {
        BOOLEAN res = combinedBufferStream_.Pull(dest, todo);
        if (!res)
        {
            WriteWarning(
                TraceComponent,
                "{0} - Get - combinedBufferStream get failure. size: {1}, combinedBufferStream.Length: {2}, combinedBufferStream.GetPosition: {3}, todo: {4}",
                TraceId,
                size,
                combinedBufferStream_.Length,
                combinedBufferStream_.Position,
                todo);

            return K_STATUS_OUT_OF_BOUNDS;
        }
    }

    bytesRead = todo;
    return STATUS_SUCCESS;
}

NTSTATUS LogicalLogBuffer::Get(
    __out PBYTE buffer,
    __out ULONG& bytesRead,
    __in LONG offset,
    __in ULONG size)
{
    KAssert(offset >= 0);
    NTSTATUS status = STATUS_SUCCESS;
    ULONG done;
    PBYTE outPtr = &buffer[offset];

    status = Get(outPtr, done, size);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceComponent,
            "{0} - Get - internal get failure. offset {1}, size {2}, status {3}",
            TraceId,
            offset,
            size,
            status);

        return status;
    }

    bytesRead = done;
    return STATUS_SUCCESS;
}

class CrcIterator
{
public:

    CrcIterator() : dataCrc_(0) {}

    NTSTATUS
    HashFunc(
        __in PVOID ioBufferFragment,
        __inout ULONG& fragmentSize)
    {
        dataCrc_ = KChecksum::Crc64(ioBufferFragment, fragmentSize, dataCrc_);
        return STATUS_SUCCESS;
    }

    __declspec(property(get = get_DataCrc)) ULONGLONG DataCrc;
    ULONGLONG get_DataCrc() const { return dataCrc_; }

private:

    ULONGLONG dataCrc_;
};

NTSTATUS LogicalLogBuffer::SealForWrite(
    __in LONGLONG currentHeadTruncationPoint,
    __in BOOLEAN isBarrier,
    __out KIoBuffer::SPtr& metadataBuffer,
    __out ULONG& metadataSize,
    __out KIoBuffer::SPtr& pageAlignedBuffer,
    __out LONGLONG& userSizeOfStreamData,
    __out LONGLONG& asnOfRecord,
    __out LONGLONG& opOfRecord)
{
    NTSTATUS status = STATUS_SUCCESS;

    ULONG trimSize = 0;
    ULONG dataResidingOutsideMetadata = 0;
    CrcIterator crcIterator;

    metadataBlockHeader_->Flags = isBarrier ? KLogicalLogInformation::MetadataBlockHeader::IsEndOfLogicalRecord : 0;

    streamBlockHeader_->DataSize = combinedBufferStream_.GetPosition() - offsetToData_;
    streamBlockHeader_->HeadTruncationPoint = currentHeadTruncationPoint;
    pageAlignedKIoBufferView_->Clear();

    // Compute CRC64 for record data
    if ((offsetToData_ < KLogicalLogInformation::FixedMetadataSize) &&
        ((offsetToData_ + streamBlockHeader_->DataSize) <= KLogicalLogInformation::FixedMetadataSize))
    {
        // no data outside the metadata buffer 
        metadataSize = offsetToData_ + streamBlockHeader_->DataSize;
    }
    else
    {
        //
        // Compute how much of the metadata is being used. It should be the entire 4K block minus the
        // space reserved by the physical logger
        //
        metadataSize = KLogicalLogInformation::FixedMetadataSize - metadataSize_;
        BOOLEAN res = combinedBufferStream_.PositionTo(offsetToData_);
        KInvariant(res);

        // todo: fix this typo in KIoBufferStream...
        KIoBufferStream::InterationCallback localHashFunc(&crcIterator, &CrcIterator::HashFunc);
        status = combinedBufferStream_.Iterate(localHashFunc, streamBlockHeader_->DataSize);
        KInvariant(NT_SUCCESS(status));

        //
        // Compute the number of data blocks that are needed to hold the data that is within the payload 
        // data buffer. This excludes the payload data in the metadata portion
        //
        KInvariant(!(offsetToData_ > MetadataBlockSize));
        dataResidingOutsideMetadata = streamBlockHeader_->DataSize - (MetadataBlockSize - offsetToData_);
        trimSize = ((((dataResidingOutsideMetadata)+(MetadataBlockSize - 1)) / MetadataBlockSize)*MetadataBlockSize);
        status = pageAlignedKIoBufferView_->AddIoBufferReference(
            *pageAlignedKIoBuffer_,
            0,
            trimSize);
        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - SealForWrite - Failed to add reference to pagealigned buffer view for pagealigned buffer. Status {1}",
                TraceId,
                status);

            return status;
        }
    }
    streamBlockHeader_->DataCRC64 = crcIterator.DataCrc;

    // Now compute blk heard crc
    streamBlockHeader_->HeaderCRC64 = KChecksum::Crc64(streamBlockHeader_, sizeof(KLogicalLogInformation::StreamBlockHeader));

    // Build results
    metadataBuffer = metadataKIoBuffer_;
    pageAlignedBuffer = pageAlignedKIoBufferView_;
    userSizeOfStreamData = streamBlockHeader_->DataSize;
    asnOfRecord = streamBlockHeader_->StreamOffset;
    opOfRecord = streamBlockHeader_->HighestOperationId;

    return STATUS_SUCCESS;
}

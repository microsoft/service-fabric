// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Log
    {
        class LogicalLogBuffer
            : public KObject<LogicalLogBuffer>
            , public KShared<LogicalLogBuffer>
            , public Utilities::IDisposable
            , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LogicalLog>
        {
            K_FORCE_SHARED(LogicalLogBuffer)
            K_SHARED_INTERFACE_IMP(IDisposable)

        public:

            VOID Dispose() override;

            NTSTATUS Intersects(
                __out BOOLEAN& intersects,
                __in LONGLONG streamOffset,
                __in LONG size = 1) const;

            NTSTATUS SetPosition(__in LONGLONG bufferOffset);

            NTSTATUS Put(
                __out ULONG& bytesWritten,
                __in BYTE const * toWrite,
                __in ULONG size);

            NTSTATUS Put(
                __out ULONG& bytesWritten,
                __in BYTE const * buffer,
                __in LONG offset,
                __in ULONG count);

            NTSTATUS Get(
                __out PBYTE dest,
                __out ULONG& bytesRead,
                __in ULONG size);

            NTSTATUS Get(
                __out PBYTE buffer,
                __out ULONG& bytesRead,
                __in LONG offset,
                __in ULONG size);

            NTSTATUS SealForWrite(
                __in LONGLONG currentHeadTruncationPoint,
                __in BOOLEAN isBarrier,
                __out KIoBuffer::SPtr& metadataBuffer,
                __out ULONG& metadataSize,
                __out KIoBuffer::SPtr& pageAlignedBuffer,
                __out LONGLONG& userSizeOfStreamData,
                __out LONGLONG& asnOfRecord,
                __out LONGLONG& opOfRecord);

            static NTSTATUS CreateReadBuffer(
                __in ULONG allocationTag,
                __in KAllocator& allocator,
                __in ULONG blockMetadataSize,
                __in LONGLONG startingStreamPosition,
                __in KIoBuffer& metadataBuffer,
                __in KIoBuffer& pageAlignedKIoBuffer,
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __out LogicalLogBuffer::SPtr& readBuffer);

            static NTSTATUS CreateWriteBuffer(
                __in ULONG allocationTag,
                __in KAllocator& allocator,
                __in ULONG blockMetadataSize,
                __in ULONG maxBlockSize,
                __in LONGLONG streamLocation,
                __in LONGLONG opNumber,
                __in KGuid streamId,
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __out LogicalLogBuffer::SPtr& writeBuffer);

            __declspec(property(get = get_IsSealed)) BOOLEAN IsSealed;
            BOOLEAN get_IsSealed() const { return streamBlockHeader_->HeaderCRC64 != 0; }

            __declspec(property(get = get_BasePosition)) LONGLONG BasePosition;
            LONGLONG get_BasePosition() const { return streamBlockHeader_->StreamOffset - 1; }

            __declspec(property(get = get_SizeWritten)) LONGLONG SizeWritten;
            LONGLONG get_SizeWritten() const { return combinedBufferStream_.GetPosition() - offsetToData_; }

            __declspec(property(get = get_SizeLeftToRead)) ULONG SizeLeftToRead;
            ULONG get_SizeLeftToRead() const { return combinedBufferStream_.Length - combinedBufferStream_.GetPosition(); }

            __declspec(property(get = get_MetadataBuffer)) KIoBuffer& MetadataBufffer;
            KIoBuffer& get_MetadataBuffer() const { return *metadataKIoBuffer_; }

            __declspec(property(get = get_pageAlignedBuffer)) KIoBuffer& PageAlignedBufffer;
            KIoBuffer& get_PageAlignedBuffer() const { return *pageAlignedKIoBuffer_; }

            __declspec(property(get = get_NumberOfBytes)) ULONG NumberOfBytes;
            ULONG get_NumberOfBytes() const { return combinedBufferStream_.Length; }

        private:

            VOID Dispose(__in BOOLEAN isDisposing);

            FAILABLE
            LogicalLogBuffer(
                __in ULONG blockMetadataSize,
                __in LONGLONG startingStreamPosition,
                __in KIoBuffer& metadataBuffer,
                __in KIoBuffer& pageAlignedKIoBuffer,
                __in Data::Utilities::PartitionedReplicaId const & prId);

            FAILABLE
            LogicalLogBuffer(
                __in ULONG blockMetadataSize,
                __in ULONG maxBlockSize,
                __in LONGLONG streamLocation,
                __in LONGLONG opNumber,
                __in KGuid streamId,
                __in Data::Utilities::PartitionedReplicaId const & prId);

            NTSTATUS OpenForRead(__in LONGLONG streamPosition);

            const ULONG MetadataBlockSize = 4096;

            KIoBuffer::SPtr metadataKIoBuffer_;
            KIoBuffer::SPtr pageAlignedKIoBuffer_;
            KIoBuffer::SPtr pageAlignedKIoBufferView_;
            KIoBuffer::SPtr combinedKIoBuffer_;
            ULONG const metadataSize_;
            LogicalLogKIoBufferStream combinedBufferStream_;
            ULONG offsetToData_;
            KLogicalLogInformation::StreamBlockHeader* streamBlockHeader_;
            KLogicalLogInformation::MetadataBlockHeader* metadataBlockHeader_;
            KIoBufferView::SPtr combinedView_;
        };
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        // Represents a checksum block of data within a file.
        // It can serialize/deserialize a block from the given stream.
        template <typename TBlockSPtr>
        class FileBlock
        {
        public:
            typedef KDelegate<TBlockSPtr(
                __in BinaryReader & binaryReader, 
                __in BlockHandle const & handle, 
                __in KAllocator & allocator)> DeserializerFunc;
            typedef KDelegate<void(__in BinaryWriter & binaryWriter)> SerializerFunc;

            static ktl::Awaitable<TBlockSPtr> ReadBlockAsync(
                __in ktl::io::KStream & stream,
                __in BlockHandle & blockHandle,
                __in DeserializerFunc deserializerFunc,
                __in KAllocator & allocator,
                __in ktl::CancellationToken const & cancellationToken)
            {
                NTSTATUS status = STATUS_UNSUCCESSFUL;

                KBuffer::SPtr buffer = nullptr;
                ULONG bytesRead = 0;
                ULONG bytesToRead = 0;

                stream.SetPosition(static_cast<LONGLONG>(blockHandle.Offset));

                ULONG64 blockSize = blockHandle.Size;
                ULONG64 blockSizeWithChecksum = blockSize + sizeof(ULONG64);

                status = KBuffer::Create(
                    static_cast<ULONG>(blockSizeWithChecksum),
                    buffer,
                    allocator);
                THROW_ON_FAILURE(status);

                bytesToRead = static_cast<ULONG>(blockSizeWithChecksum);

                status = co_await stream.ReadAsync(
                    *buffer,
                    bytesRead,
                    0,
                    bytesToRead);
                THROW_ON_FAILURE(status);

                ASSERT_IFNOT(
                    bytesRead == bytesToRead, 
                    "Bytes read: {0} not equal to bytes to read: {1}", 
                    bytesRead, bytesToRead);

                ULONG64 blockChecksum = CRC64::ToCRC64(static_cast<byte *>(static_cast<void*>(buffer->GetBuffer())), 0, static_cast<ULONG32>(blockSize));
                ULONG64 checksum = ToUInt64(static_cast<byte *>(static_cast<void*>(buffer->GetBuffer())), static_cast<ULONG32>(blockSize));
                if(blockChecksum != checksum)
                {
                    throw ktl::Exception(STATUS_INTERNAL_DB_CORRUPTION);
                }

                BinaryReader reader(*buffer, allocator);

                BlockHandle::SPtr newBlockHandle = nullptr;
                status = BlockHandle::Create(0, blockHandle.Size, allocator, newBlockHandle);

                cancellationToken.ThrowIfCancellationRequested();

                co_return deserializerFunc(reader, *newBlockHandle, allocator);
            }

            static ktl::Awaitable<NTSTATUS> WriteBlockAsync(
                __in ktl::io::KStream & stream,
                __in SerializerFunc serializerFunc,
                __in KAllocator & allocator,
                __in ktl::CancellationToken const & cancellationToken,
                __out BlockHandle::SPtr & result) noexcept
            {
                NTSTATUS status = STATUS_UNSUCCESSFUL;

                BinaryWriter binaryWriter(allocator);
                serializerFunc(binaryWriter);

                if (cancellationToken.IsCancellationRequested)
                {
                    co_return STATUS_CANCELLED;
                }

                ULONG32 blockSize = static_cast<ULONG32>(binaryWriter.get_Position());

                KBuffer::SPtr buffer;
                status = KBuffer::Create(blockSize, buffer, allocator);
                buffer = binaryWriter.GetBuffer(0);

                ULONG64 checksum = CRC64::ToCRC64(static_cast<byte *>(static_cast<void*>(buffer->GetBuffer())), 0, blockSize);

                LONGLONG position = stream.GetPosition();
                status  = BlockHandle::Create(position, blockSize, allocator, result);

                binaryWriter.Write(checksum);
                ULONG32 blockSizeWithChecksum = static_cast<ULONG32>(binaryWriter.get_Position());

                status = co_await stream.WriteAsync(*binaryWriter.GetBuffer(0), 0, blockSizeWithChecksum);

                co_return status;
            }

            static ULONG64 ToUInt64 (__in byte value[], __in ULONG32 startIndex)
            {
                byte * pbyte = &(value[startIndex]);
                return  *(static_cast<ULONG64 *>(static_cast<void*>(pbyte)));
            }
        };
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define MEMORY_STREAM_TAG 'msTG'

namespace Data
{
    namespace TStore
    {
        //
        // Currently, this class implements the bare minimum to get StoreCopyStream to work
        //
        class MemoryBuffer
            : public KObject<MemoryBuffer>
            , public KShared<MemoryBuffer>
            , public ktl::io::KStream
        {
            K_FORCE_SHARED(MemoryBuffer)
            K_SHARED_INTERFACE_IMP(KStream)

        public:
            static NTSTATUS Create(__in KAllocator & allocator, __out SPtr & result);
            static NTSTATUS Create(__in ULONG capacity, __in KAllocator & allocator, __out SPtr & result);
            static NTSTATUS Create(__in KBuffer & buffer, __in KAllocator & allocator, __out SPtr & result);

        public:
            __declspec(property(get = GetPosition, put = SetPosition)) LONGLONG Position;
            LONGLONG GetPosition() const override;
            void SetPosition(__in LONGLONG Position) override;

            __declspec(property(get = GetLength)) LONGLONG Length;
            LONGLONG GetLength() const override;
        
            ktl::Awaitable<NTSTATUS> ReadAsync(
                __in KBuffer& buffer,
                __out ULONG & bytesRead,
                __in ULONG offsetIntoBuffer = 0,
                __in ULONG count = 0) override;

            ktl::Awaitable<NTSTATUS> WriteAsync(
                __in KBuffer const & buffer,
                __in ULONG offsetIntoBuffer = 0,
                __in ULONG count = 0) override;

            ktl::Awaitable<NTSTATUS> WriteAsync(__in byte value);
            ktl::Awaitable<NTSTATUS> WriteAsync(__in ULONG32 value);

            ktl::Awaitable<NTSTATUS> FlushAsync() override;

            ktl::Awaitable<NTSTATUS> CloseAsync() override;

            KBuffer::SPtr GetBuffer();

        private:
            MemoryBuffer(__in ULONG capacity);
            MemoryBuffer(__in KBuffer & buffer);

            bool isResizable_;
            KBuffer::SPtr bufferSPtr_;
            SharedBinaryWriter::SPtr binaryWriterSPtr_;
            ULONG position_;
        };
    }
}

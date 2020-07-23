// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        class MemoryLog::ReadStream
            : public KObject<ReadStream>
            , public KShared<ReadStream>
            , public Data::Log::ILogicalLogReadStream
        {
            K_FORCE_SHARED(ReadStream)
            K_SHARED_INTERFACE_IMP(KStream)
            K_SHARED_INTERFACE_IMP(ILogicalLogReadStream)
            K_SHARED_INTERFACE_IMP(IDisposable)

        public:
            static NTSTATUS Create(
                __in MemoryLog& parent,
                __in KAllocator& allocator,
                __out ReadStream::SPtr & result);
            
            LONGLONG GetPosition() const override
            {
                return readPosition_;
            }

            void SetPosition(__in LONGLONG position) override
            {
                readPosition_ = position;
            }

            LONGLONG GetLength() const override
            {
                return log_->GetLength();
            }

            ktl::Awaitable<NTSTATUS> ReadAsync(
                __in KBuffer& Buffer,
                __out ULONG& BytesRead,
                __in ULONG OffsetIntoBuffer,
                __in ULONG Count);
            

            ktl::Awaitable<NTSTATUS> WriteAsync(
                __in KBuffer const & Buffer,
                __in ULONG OffsetIntoBuffer,
                __in ULONG Count);

            ktl::Awaitable<NTSTATUS> FlushAsync();

            ktl::Awaitable<NTSTATUS> CloseAsync();

            void Dispose();

        private:
            ReadStream(__in MemoryLog & parent);

            MemoryLog::SPtr log_;
            LONG64 readPosition_;
        };
    }
}

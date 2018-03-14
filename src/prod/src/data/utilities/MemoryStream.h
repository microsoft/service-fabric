// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        class MemoryStream 
            : public KObject<MemoryStream>
            , public KShared<MemoryStream>
            , public ktl::io::KStream
        {
            K_FORCE_SHARED(MemoryStream);
            K_SHARED_INTERFACE_IMP(KStream);

        public: // public static.
            static NTSTATUS Create(
                __in KBuffer const & buffer,
                __in KAllocator & allocator,
                __out SPtr & result) noexcept;

        public: // KStream APIS.
            ktl::Awaitable<NTSTATUS> CloseAsync() override;

            ktl::Awaitable<NTSTATUS> ReadAsync(
                __in KBuffer & Buffer,
                __out ULONG& BytesRead,
                __in ULONG OffsetIntoBuffer = 0,
                __in ULONG Count = 0) override;

            ktl::Awaitable<NTSTATUS> WriteAsync(
                __in KBuffer const & Buffer,
                __in ULONG OffsetIntoBuffer = 0,
                __in ULONG Count = 0) override;

            ktl::Awaitable<NTSTATUS> FlushAsync() override;

            LONGLONG GetPosition() const override;

            void SetPosition(__in LONGLONG Position) override;

            LONGLONG GetLength() const override;

        private:
            NOFAIL MemoryStream(__in KBuffer const & buffer) noexcept;

        private: // Initialize in the initializer list.
            const KBuffer::CSPtr buffer_;
            LONGLONG position_;
        };
    }
}

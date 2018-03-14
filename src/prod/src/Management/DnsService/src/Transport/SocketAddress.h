// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    using ::_delete;

    class SocketAddress :
        public KShared<SocketAddress>,
        public ISocketAddress
    {
        K_FORCE_SHARED(SocketAddress);
        K_SHARED_INTERFACE_IMP(ISocketAddress);

    public:
        static void Create(
            __out SocketAddress::SPtr& spAddress,
            __in KAllocator& allocator
        );

    public:
        // ISocketAddress
        virtual PVOID Address() override { return reinterpret_cast<PVOID>(&_address); }
        virtual socklen_t Size() const override { return _size; }
        virtual socklen_t* SizePtr() override { return &_size; }

    public:
        void Set(
            __in ULONG address,
            __in USHORT port
        );

    private:
        SOCKADDR_STORAGE _address;
        socklen_t _size;
    };
}

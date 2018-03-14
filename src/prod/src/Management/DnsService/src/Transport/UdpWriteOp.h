// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "UdpOverlap.h"

namespace DNS
{
    using ::_delete;

    class UdpWriteOp :
        public KAsyncContextBase,
        public IUdpAsyncOp
    {
        friend class UdpListener;

        K_FORCE_SHARED(UdpWriteOp);
        K_SHARED_INTERFACE_IMP(IUdpAsyncOp);

    private:
        static void Create(
            __out UdpWriteOp::SPtr& spWriteOp,
            __in KAllocator& allocator,
            __in SOCKET socket,
            __in INetIoManager& netIoManager
        );

    private:
        UdpWriteOp(
            __in SOCKET socket,
            __in INetIoManager& netIoManager
        );

    private:
        // KAsyncContextBase Impl.
        using KAsyncContextBase::Start;

        virtual void OnStart() override;
        virtual void OnCompleted() override;

    public:
        void StartWrite(
            __in_opt KAsyncContextBase* const parent,
            __in KBuffer& buffer,
            __in ULONG numberOfBytesToWrite,
            __in ISocketAddress& address,
            __in UdpWriteOpCompletedCallback callback
        );

    private:
        // IUdpAsyncOp Impl.
        virtual void IOCP_Completion(
            __in ULONG status,
            __in ULONG bytesTransferred
        ) override;

        virtual KBuffer::SPtr GetBuffer() override { return _spBuffer; }

        virtual ULONG GetBufferDataLength() override { return _numberOfBytesToWrite; }

        virtual ISocketAddress::SPtr GetAddress() override { return _spAddress; }

    private:
        SOCKET _socket;
        INetIoManager& _netIoManager;
        ISocketAddress::SPtr _spAddress;
        KBuffer::SPtr _spBuffer;
        ULONG _numberOfBytesToWrite;
        ULONG _bytesTransferred;
        UDP_OVERLAPPED _olap;
        UdpWriteOpCompletedCallback _callback;
#if !defined(PLATFORM_UNIX)
        WSABUF _wsaBuf;
#endif
    };
}

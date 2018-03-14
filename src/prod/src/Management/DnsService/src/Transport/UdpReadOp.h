// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "UdpOverlap.h"

namespace DNS
{
    using ::_delete;

    class UdpReadOp :
        public KAsyncContextBase,
        public IUdpAsyncOp
    {
        friend class UdpListener;

        K_FORCE_SHARED(UdpReadOp);
        K_SHARED_INTERFACE_IMP(IUdpAsyncOp);

    private:
        static void Create(
            __out UdpReadOp::SPtr& spReadOp,
            __in KAllocator& allocator,
            __in SOCKET socket,
            __in INetIoManager& netIoManager
        );

    private:
        UdpReadOp(
            __in SOCKET socket,
            __in INetIoManager& netIoManager
        );

    private:
        // KAsyncContextBase Impl.
        using KAsyncContextBase::Start;

        virtual void OnStart() override;
        virtual void OnCancel() override;
        virtual void OnCompleted() override;

    private:
        void StartRead(
            __in_opt KAsyncContextBase* const parent,
            __out KBuffer& buffer,
            __out ISocketAddress& address,
            __in UdpReadOpCompletedCallback callback,
            __in ULONG timeoutInMs
        );

    private:
        // IUdpAsyncOp Impl.
        virtual void IOCP_Completion(
            __in ULONG status,
            __in ULONG bytesTransferred
        ) override;

        virtual KBuffer::SPtr GetBuffer() override { return _spBuffer; }

        virtual ULONG GetBufferDataLength() override { return _bytesTransferred; }

        virtual ISocketAddress::SPtr GetAddress() override { return _spAddress; }

    private:
        void OnTimer(
            __in_opt KAsyncContextBase* const,
            __in KAsyncContextBase&
        );

    private:
        SOCKET _socket;
        INetIoManager& _netIoManager;
        ISocketAddress::SPtr _spAddress;
        KBuffer::SPtr _spBuffer;
        ULONG _bytesTransferred;
        UDP_OVERLAPPED _olap;
        UdpReadOpCompletedCallback _callback;
#if !defined(PLATFORM_UNIX)
        WSABUF _wsaBuf;
#endif
        KTimer::SPtr _spTimer;
        DWORD _flags;
        ULONG _timeoutInMs;
    };
}

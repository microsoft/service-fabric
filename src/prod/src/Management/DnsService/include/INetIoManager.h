// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "IDnsTracer.h"

namespace DNS
{
#if defined(PLATFORM_UNIX)
#define WSA_IO_PENDING (ERROR_IO_PENDING)
#define WSA_OPERATION_ABORTED (WSAEINTR)
#endif

    typedef KDelegate<void(__in NTSTATUS)> UdpListenerCallback;
    typedef KDelegate<void(__in NTSTATUS, __in KBuffer&, __in ULONG)> UdpReadOpCompletedCallback;
    typedef KDelegate<void(__in NTSTATUS, __in ULONG)> UdpWriteOpCompletedCallback;

    interface ISocketAddress
    {
        K_SHARED_INTERFACE(ISocketAddress);

    public:
        virtual PVOID Address() = 0;
        virtual socklen_t Size() const = 0;
        virtual socklen_t* SizePtr() = 0;
    };

    void CreateSocketAddress(
        __out ISocketAddress::SPtr& spAddress,
        __in KAllocator& allocator
    );

    void CreateSocketAddress(
        __out ISocketAddress::SPtr& spAddress,
        __in KAllocator& allocator,
        __in ULONG address,
        __in USHORT port
    );

    interface IUdpListener
    {
        K_SHARED_INTERFACE(IUdpListener);

    public:
        virtual bool StartListener(
            __in_opt KAsyncContextBase* const parent,
            __inout USHORT& port,
            __in UdpListenerCallback completionCallback
        ) = 0;

        virtual bool CloseAsync(
        ) = 0;

        virtual void ReadAsync(
            __out KBuffer& buffer,
            __out ISocketAddress& addressFrom,
            __in UdpReadOpCompletedCallback readCallback,
            __in ULONG timeoutInMs
        ) = 0;

        virtual void WriteAsync(
            __in KBuffer& buffer,
            __in ULONG numberOfBytesToWrite,
            __in ISocketAddress& addressTo,
            __in UdpWriteOpCompletedCallback writeCallback
        ) = 0;
    };

    interface IUdpAsyncOp
    {
        K_SHARED_INTERFACE(IUdpAsyncOp);

    public:
        virtual void IOCP_Completion(
            __in ULONG status,
            __in ULONG bytesTransferred
        ) = 0;

        virtual KBuffer::SPtr GetBuffer() = 0;

        virtual ULONG GetBufferDataLength() = 0;

        virtual ISocketAddress::SPtr GetAddress() = 0;
    };

    interface INetIoManager
    {
        K_SHARED_INTERFACE(INetIoManager);

    public:
        virtual void StartManager(
            __in_opt KAsyncContextBase* const parent
        ) = 0;

        virtual void CloseAsync() = 0;

        virtual void CreateUdpListener(
            __out IUdpListener::SPtr& spListener,
            __in bool fEnableSocketAddressReuse
        ) = 0;

#if defined(PLATFORM_UNIX)
        virtual bool RegisterSocket(
            __in SOCKET socket
        ) = 0;

        virtual void UnregisterSocket(
            __in SOCKET socket
        ) = 0;

        virtual int ReadAsync(
            __in SOCKET socket,
            __in IUdpAsyncOp& overlapOp,
            __out ULONG& bytesRead
        ) = 0;

        virtual int WriteAsync(
            __in SOCKET socket,
            __in KBuffer& buffer,
            __in ULONG count,
            __in ISocketAddress& address,
            __in IUdpAsyncOp& overlapOp,
            __out ULONG& bytesWritten
        ) = 0;
#endif
    };

    void CreateNetIoManager(
        __out INetIoManager::SPtr& spManager,
        __in KAllocator& allocator,
        __in const IDnsTracer::SPtr& spTracer,
        __in ULONG numberOfConcurrentQueries
    );
}



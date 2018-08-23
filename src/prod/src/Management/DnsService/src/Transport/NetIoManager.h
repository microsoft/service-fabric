// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    using ::_delete;

    class NetIoManager :
        public KAsyncContextBase,
        public INetIoManager
    {
        K_FORCE_SHARED(NetIoManager);
        K_SHARED_INTERFACE_IMP(INetIoManager);

    public:
        static void Create(
            __out NetIoManager::SPtr& spManager,
            __in KAllocator& allocator,
            __in const IDnsTracer::SPtr& spTracer,
            __in ULONG numberOfConcurrentQueries
        );

    private:
        NetIoManager(
            __in const IDnsTracer::SPtr& spTracer,
            __in ULONG numberOfConcurrentQueries
        );

    public:
        // INetIoManager Impl.
        virtual void StartManager(
            __in_opt KAsyncContextBase* const parent
        ) override;

        virtual void CloseAsync() override;

        virtual void CreateUdpListener(
            __out IUdpListener::SPtr& spListener,
            __in bool fEnableSocketAddressReuse
        ) override;

#if defined(PLATFORM_UNIX)
        virtual bool RegisterSocket(
            __in SOCKET socket
        ) override;

        virtual void UnregisterSocket(
            __in SOCKET socket
        ) override;

        virtual int ReadAsync(
            __in SOCKET socket,
            __in IUdpAsyncOp& overlapOp,
            __out ULONG& bytesRead
        ) override;

        virtual int WriteAsync(
            __in SOCKET socket,
            __in KBuffer& buffer,
            __in ULONG count,
            __in ISocketAddress& address,
            __in IUdpAsyncOp& overlapOp,
            __out ULONG& bytesWritten
        ) override;
#endif

    private:
        // KAsyncContextBase Impl.
        using KAsyncContextBase::Start;

        virtual void OnStart() override;
        virtual void OnCancel() override;
        virtual void OnCompleted() override;

    private:
        static void MainLoop(
            __inout_opt void* parameter
        );

    private:
        void HandleEpollEvents();
        IDnsTracer& Tracer() { return *_spTracer; }

#if defined(PLATFORM_UNIX)
        int ReadInternal(
            __in SOCKET socket,
            __in IUdpAsyncOp& overlapOp,
            __out ULONG& bytesRead
        );

        int WriteInternal(
            __in SOCKET socket,
            __in KBuffer& buffer,
            __in ULONG count,
            __in ISocketAddress& address,
            __out ULONG& bytesWritten
        );
#endif

    private:
        int _efd;
        IDnsTracer::SPtr _spTracer;
        DnsServiceParams _params;
        KThread::SPtr _spEpollThread;

        typedef KSharedType<KQueue<IUdpAsyncOp::SPtr>> Queue;
        struct Queues
        {
            Queue::SPtr ReadQueue;
            Queue::SPtr WriteQueue;
        };
        KHashTable<ULONG_PTR, Queues> _htSockets;
        KSpinLock _lockSockets;
    };
}

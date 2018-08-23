// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    using ::_delete;

    class UdpListener :
        public KAsyncContextBase,
        public IUdpListener
    {
        K_FORCE_SHARED(UdpListener);
        K_SHARED_INTERFACE_IMP(IUdpListener);

    public:
        static void Create(
            __out UdpListener::SPtr& spServer,
            __in KAllocator& allocator,
            __in const IDnsTracer::SPtr& spTracer,
            __in const INetIoManager::SPtr& spManager,
            __in bool fEnableSocketAddressReuse
        );

    private:
        UdpListener(
            __in const IDnsTracer::SPtr& spTracer,
            __in const INetIoManager::SPtr& spManager,
            __in bool fEnableSocketAddressReuse
        );

    public:
        // IUdpListener Impl.
        virtual bool StartListener(
            __in_opt KAsyncContextBase* const parent,
            __inout USHORT& port,
            __in UdpListenerCallback completionCallback
        ) override;

        virtual bool CloseAsync() override;

        virtual void ReadAsync(
            __out KBuffer& buffer,
            __out ISocketAddress& addressFrom,
            __in UdpReadOpCompletedCallback readCallback,
            __in ULONG timeoutInMs
        ) override;

        virtual void WriteAsync(
            __in KBuffer& buffer,
            __in ULONG numberOfBytesToWrite,
            __in ISocketAddress& addressTo,
            __in UdpWriteOpCompletedCallback writeCallback
        ) override;

    private:
        // KAsyncContextBase Impl.
        using KAsyncContextBase::Start;

        virtual void OnStart() override;
        virtual void OnCancel() override;
        virtual void OnCompleted() override;

    private:
        static void Raw_IOCP_Completion(
            __in_opt void* Context,
            __inout OVERLAPPED* Overlapped,
            __in ULONG Status,
            __in ULONG BytesTransferred
        );

    private:
        void Cleanup();

        IDnsTracer& Tracer() { return *_spTracer; }

    private:
        INetIoManager::SPtr _spManager;
        IDnsTracer::SPtr _spTracer;
        USHORT _port;
        UdpListenerCallback _openCallback;
        UdpListenerCallback _closeCallback;
        SOCKET _socket;
        PVOID _ioRegistrationContext;
        bool _fEnableSocketAddressReuse;
    };
}

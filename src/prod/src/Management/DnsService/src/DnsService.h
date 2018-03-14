// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "DnsHealthMonitor.h"
#include "DnsNodeCacheMonitor.h"

namespace DNS
{
    using ::_delete;

    class DnsService :
        public KAsyncContextBase,
        public IDnsService
    {
        K_FORCE_SHARED(DnsService);
        K_SHARED_INTERFACE_IMP(IDnsService);

    public:
        static void Create(
            __out DnsService::SPtr& spDnsService,
            __in KAllocator& allocator,
            __in const IDnsTracer::SPtr& spTracer,
            __in const IDnsParser::SPtr& spDnsParser,
            __in const INetIoManager::SPtr& spNetIoManager,
            __in const IFabricResolve::SPtr& spFabricResolve,
            __in const IDnsCache::SPtr& spDnsCache,
            __in IFabricStatelessServicePartition2& fabricHealth,
            __in const DnsServiceParams& params
        );

    private:
        DnsService(
            __in const IDnsTracer::SPtr& spTracer,
            __in const IDnsParser::SPtr& spDnsParser,
            __in const INetIoManager::SPtr& spNetIoManager,
            __in const IFabricResolve::SPtr& spFabricResolve,
            __in const IDnsCache::SPtr& spDnsCache,
            __in IFabricStatelessServicePartition2& fabricHealth,
            __in const DnsServiceParams& params
        );

    public:
        // IDnsService Impl.
        virtual bool Open(
            __inout USHORT& port,
            __in DnsServiceCallback callback
        );

        virtual void CloseAsync();

    private:
        // KAsyncContextBase Impl.
        using KAsyncContextBase::Start;

        virtual void OnStart() override;
        virtual void OnCancel() override;
        virtual void OnCompleted() override;

    private:
        void OnUdpListenerClosed(
            __in NTSTATUS status
        );

        void OnExchangeOpCompleted(
            __in_opt KAsyncContextBase* const parent,
            __in KAsyncContextBase& instance
        );

        void OnOpenCompleted(
            __in_opt KAsyncContextBase* const parent,
            __in KAsyncServiceBase& instance,
            __in NTSTATUS status
        );

        void OnCloseCompleted(
            __in_opt KAsyncContextBase* const parent,
            __in KAsyncServiceBase& instance,
            __in NTSTATUS status
        );

    private:
        void StartExchangeOps();

        IDnsTracer& Tracer() { return *_spTracer; }

    private:
        IDnsTracer::SPtr _spTracer;
        IDnsParser::SPtr _spDnsParser;
        INetIoManager::SPtr _spNetIoManager;
        IUdpListener::SPtr _spUdpListener;
        IFabricResolve::SPtr _spFabricResolve;
        INetworkParams::SPtr _spNetworkParams;
        DnsServiceParams _params;
        IDnsCache::SPtr _spDnsCache;
        ComPointer<IFabricStatelessServicePartition2> _spFabricHealth;
        DnsHealthMonitor::SPtr _spHealthMonitor;
        DnsNodeCacheMonitor::SPtr _spCacheMonitor;

        bool _fActive;
        KSpinLock _lockExchangeOp;
        USHORT _port;
        KArray<DnsExchangeOp::SPtr> _arrOps;

        DnsServiceCallback _completionCallback;
    };
}

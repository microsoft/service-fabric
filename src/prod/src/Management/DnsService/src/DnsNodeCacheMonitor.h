// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    using ::_delete;

    class DnsNodeCacheMonitor :
        public KAsyncContextBase,
        public IDnsCacheNotification
    {
        K_FORCE_SHARED(DnsNodeCacheMonitor);
        K_SHARED_INTERFACE_IMP(IDnsCacheNotification);

    public:
        static void Create(
            __out DnsNodeCacheMonitor::SPtr& spCache,
            __in KAllocator& allocator,
            __in const DnsServiceParams& params,
            __in IDnsTracer& tracer,
            __in IDnsCache& dnsCache
        );

    private:
        DnsNodeCacheMonitor(
            __in const DnsServiceParams& params,
            __in IDnsTracer& tracer,
            __in IDnsCache& dnsCache
        );

    public:
        bool StartMonitor(
            __in_opt KAsyncContextBase* const parent
        );

    public:
        // IDnsCacheNotification Impl.
        virtual void OnDnsCachePut(
            __in KString& dnsName
        ) override;

    private:
        // KAsyncContextBase Impl.
        using KAsyncContextBase::Start;

        virtual void OnStart() override;
        virtual void OnCancel() override;

    private:
        bool FlushCache();

    private:
        const DnsServiceParams& _params;
        IDnsTracer& _tracer;
        IDnsCache& _cache;

        KLocalString<MAX_PATH> _strHostsFilePath;
    };
}

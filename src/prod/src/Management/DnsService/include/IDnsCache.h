// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "IDnsTracer.h"

namespace DNS
{
    enum DnsNameType
    {
        DnsNameTypeUnknown = 1,
        DnsNameTypeFabric,
        DnsNameTypePublic
    };

    interface IDnsCacheNotification
    {
        K_SHARED_INTERFACE(IDnsCacheNotification);

    public:
        virtual void OnDnsCachePut(
            __in KString& dnsName
        ) = 0;
    };

    interface IDnsCache
    {
        K_SHARED_INTERFACE(IDnsCache);

    public:
        virtual DnsNameType Read(
            __in KString& dnsName,
            __out KString::SPtr& serviceName
        ) = 0;

        virtual bool IsDnsNameKnown(
            __in KString& dnsName
        ) = 0;

        virtual bool IsServiceKnown(
            __in KString& serviceName
        ) = 0;

        virtual void MarkNameAsPublic(
            __in KString& dnsName
        ) = 0;

        virtual void Put(
            __in KString& dnsName,
            __in KString& serviceName
        ) = 0;

        virtual void Remove(
            __in KString& serviceName
        ) = 0;

        virtual void RegisterNotification(
            __in IDnsCacheNotification& notification
        ) = 0;

        virtual void UnregisterNotification(
            __in IDnsCacheNotification& notification
        ) = 0;
    };

    void CreateDnsCache(
        __out IDnsCache::SPtr& spCache,
        __in KAllocator& allocator,
        __in ULONG maxSize
    );
}

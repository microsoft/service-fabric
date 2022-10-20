// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "IDnsTracer.h"

namespace DNS
{
    typedef KDelegate<void(__in NTSTATUS)> DnsServiceCallback;

    interface IDnsService
    {
        K_SHARED_INTERFACE(IDnsService);

    public:
        virtual bool Open(
            __inout USHORT& port,
            __in DnsServiceCallback callback
        ) = 0;

        virtual void CloseAsync() = 0;
    };

    struct DnsServiceParams
    {
        DnsServiceParams() :
            Port(53),
            SetAsPreferredDns(true),
            TimeToLiveInSeconds(1),
            NDots(1),
            IsRecursiveQueryEnabled(true),
            FabricQueryTimeoutInSeconds(5),
            RecursiveQueryTimeoutInSeconds(5),
            NodeDnsCacheHealthCheckIntervalInSeconds(5),
            NumberOfConcurrentQueries(1),
            MaxMessageSizeInKB(8),
            MaxCacheSize(5000),
            AllowMultipleListeners(false),
            EnablePartitionedQuery(false),
            EnableOnCloseTimeout(true),
            OnCloseTimeoutInSeconds(60)
        {
        }

        USHORT Port;
        bool SetAsPreferredDns;             // Set DnsService as preferred DNS server on the node
        ULONG TimeToLiveInSeconds;          // TTL of the answer record
        ULONG NDots;                        // Absolute query must have at least 'NDots' number of dots. Otherwise it is treated as unqualified query and DNS suffixes are appended.
        bool IsRecursiveQueryEnabled;       // TRUE if query needs to be forwarded to public DNS
        ULONG NodeDnsCacheHealthCheckIntervalInSeconds; // Checks node DNS cache for incorrect entries and flushes them. Disabled when set to zero.
        ULONG FabricQueryTimeoutInSeconds;
        ULONG RecursiveQueryTimeoutInSeconds;
        ULONG NumberOfConcurrentQueries;
        ULONG MaxMessageSizeInKB;
        ULONG MaxCacheSize;
        bool AllowMultipleListeners;      // Set socket SO_REUSEADDR flag.
        KString::SPtr PartitionPrefix;
        KString::SPtr PartitionSuffix;
        bool EnablePartitionedQuery;
        bool EnableOnCloseTimeout;
        ULONG OnCloseTimeoutInSeconds;
    };

    void CreateDnsService(
        __out IDnsService::SPtr& spDnsService,
        __in KAllocator& allocator,
        __in const IDnsTracer::SPtr& spTracer,
        __in const IDnsParser::SPtr& spDnsParser,
        __in const INetIoManager::SPtr& spNetIoManager,
        __in const IFabricResolve::SPtr& spFabricResolve,
        __in const IDnsCache::SPtr& spDnsCache,
        __in IFabricStatelessServicePartition2& fabricHealth,
        __in const DnsServiceParams& params
    );
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    using ::_delete;

    class NetworkParams :
        public KShared<NetworkParams>,
        public INetworkParams
    {
        K_FORCE_SHARED(NetworkParams);
        K_SHARED_INTERFACE_IMP(INetworkParams);

    public:
        static void Create(
            __out NetworkParams::SPtr& spParams,
            __in KAllocator& allocator,
            __in IDnsTracer& tracer
        );

    private:
        NetworkParams(
            __in IDnsTracer& tracer
        );

    public:
        // INetworkParams Impl.
        virtual const KArray<ULONG>& DnsServers() const override { return _arrDnsServers; }
        virtual const KArray<KString::SPtr> DnsSearchList() const override { return _arrDnsSearchList; }

    public:
        void Initialize();

        static void GetSearchList(
            __out KArray<KString::SPtr>& arrSearchList,
            __in KAllocator& allocator,
            __in IDnsTracer& tracer
        );

        static void GetDnsServerList(
            __out KArray<ULONG>& arrDnsServers,
            __in KAllocator& allocator
        );

    private:
        KArray<ULONG> _arrDnsServers;
        KArray<KString::SPtr> _arrDnsSearchList;
        IDnsTracer& _tracer;
    };
}

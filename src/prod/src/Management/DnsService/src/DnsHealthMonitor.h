// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    using ::_delete;

    class DnsHealthMonitor :
        public KAsyncContextBase
    {
        K_FORCE_SHARED(DnsHealthMonitor);

    public:
        static void Create(
            __out DnsHealthMonitor::SPtr& spHealth,
            __in KAllocator& allocator,
            __in const DnsServiceParams& params,
            __in IFabricStatelessServicePartition2& fabricHealth
        );

    private:
        DnsHealthMonitor(
            __in const DnsServiceParams& params,
            __in IFabricStatelessServicePartition2& fabricHealth
        );

    public:
        bool StartMonitor(
            __in_opt KAsyncContextBase* const parent
        );

    private:
        // KAsyncContextBase Impl.
        using KAsyncContextBase::Start;

        virtual void OnStart() override;
        virtual void OnCancel() override;

    private:
        void CheckEnvironment();

    private:
        void StartTimer();
        void OnTimer(
            __in_opt KAsyncContextBase* const,
            __in KAsyncContextBase&
        );

    private:
        const DnsServiceParams& _params;
        IFabricStatelessServicePartition2& _fabricHealth;
        KTimer::SPtr _spTimer;

        FABRIC_HEALTH_STATE _lastStateEnvironment;
    };
}

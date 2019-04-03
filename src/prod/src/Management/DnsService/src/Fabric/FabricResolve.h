// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    using ::_delete;

    class FabricResolve :
        public KAsyncContextBase,
        public IFabricResolve
    {
        K_FORCE_SHARED(FabricResolve);
        K_SHARED_INTERFACE_IMP(IFabricResolve);

    public:
        static void Create(
            __out FabricResolve::SPtr& spResolveOp,
            __in KAllocator& allocator,
            __in const KString::SPtr& spServiceName,
            __in const IDnsTracer::SPtr& spTracer,
            __in const IFabricData::SPtr& spData,
            __in const IDnsCache::SPtr& spCache,
            __in IFabricServiceManagementClient* pFabricServiceClient,
            __in IFabricPropertyManagementClient* pFabricPropertyClient
        );

    private:
        FabricResolve(
            __in const KString::SPtr& spServiceName,
            __in const IDnsTracer::SPtr& spTracer,
            __in const IFabricData::SPtr& spData,
            __in const IDnsCache::SPtr& spCache,
            __in IFabricServiceManagementClient* pFabricServiceClient,
            __in IFabricPropertyManagementClient* pFabricPropertyClient
        );

    private:
        // KAsyncContextBase Impl.
        using KAsyncContextBase::Start;

        virtual void OnStart() override;
        virtual void OnCompleted() override;
        virtual void OnReuse() override;
        virtual void OnCancel() override;

    public:
        // IFabricResolve Impl.
        virtual void Open(
            __in KAsyncContextBase* const parent
        ) override;

        virtual void CloseAsync() override;

        virtual IFabricResolveOp::SPtr CreateResolveOp(
            __in ULONG fabricQueryTimeoutInSeconds,
            __in const KString::SPtr& spStrPartitionPrefix,
            __in const KString::SPtr& spStrPartitionSuffix,
            __in bool fIsPartitionedQueryEnabled
        ) const override;

        virtual void NotifyServiceChanged(
            __in KString& serviceName,
            __in bool fServiceDeleted
        ) override;

    private:
        IDnsTracer & Tracer() { return *_spTracer; }

    private:
        bool _fActive;
        KSpinLock _lock;
        KString::SPtr _spServiceName;
        IDnsTracer::SPtr _spTracer;
        IFabricData::SPtr _spData;
        IDnsCache::SPtr _spCache;
        ComPointer<IFabricServiceManagementClient> _spFabricServiceClient;
        ComPointer<IFabricPropertyManagementClient> _spFabricPropertyClient;
    };
}

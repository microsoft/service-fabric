// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    using ::_delete;

    class FabricDnsServiceFactory :
        public KShared<FabricDnsServiceFactory>,
        public IFabricStatelessServiceFactory
    {
        K_FORCE_SHARED(FabricDnsServiceFactory);

        K_BEGIN_COM_INTERFACE_LIST(FabricDnsServiceFactory)
            K_COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatelessServiceFactory)
            K_COM_INTERFACE_ITEM(IID_IFabricStatelessServiceFactory, IFabricStatelessServiceFactory)
        K_END_COM_INTERFACE_LIST()

    public:
        static void Create(
            __out FabricDnsServiceFactory::SPtr& spFactory,
            __in KAllocator& allocator,
            __in IDnsTracer& tracer
            );

    private:
        FabricDnsServiceFactory(
            __in IDnsTracer& tracer
        );

    public:
        virtual HRESULT CreateInstance(
            __in LPCWSTR serviceTypeName,
            __in FABRIC_URI serviceName,
            __in ULONG initializationDataLength,
            __in_ecount(initializationDataLength) const byte* initializationData,
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_INSTANCE_ID instanceId,
            __out IFabricStatelessServiceInstance** serviceInstance
            ) override;

    private:
        IDnsTracer& _tracer;
    };
}

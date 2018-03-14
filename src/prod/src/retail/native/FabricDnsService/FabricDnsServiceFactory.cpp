// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FabricDnsServiceFactory.h"
#include "FabricDnsService.h"

/*static*/
void FabricDnsServiceFactory::Create(
    __out FabricDnsServiceFactory::SPtr& spFactory,
    __in KAllocator& allocator,
    __in IDnsTracer& tracer
    )
{
    spFactory = _new(TAG, allocator) FabricDnsServiceFactory(tracer);
    KInvariant(spFactory != nullptr);
}

FabricDnsServiceFactory::FabricDnsServiceFactory(
    __in IDnsTracer& tracer
    ) : _tracer(tracer)
{
}

FabricDnsServiceFactory::~FabricDnsServiceFactory()
{
}

HRESULT FabricDnsServiceFactory::CreateInstance(
    __in LPCWSTR serviceTypeName,
    __in FABRIC_URI serviceName,
    __in ULONG initializationDataLength,
    __in_ecount(initializationDataLength) const byte* initializationData,
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_INSTANCE_ID instanceId,
    __out IFabricStatelessServiceInstance** serviceInstance
    )
{
    FabricDnsService::SPtr spService;
    FabricDnsService::Create(
        /*out*/spService,
        GetThisAllocator(),
        _tracer,
        serviceTypeName,
        serviceName,
        initializationDataLength,
        initializationData,
        partitionId,
        instanceId
    );

    *serviceInstance = spService.Detach();

    return S_OK;
}

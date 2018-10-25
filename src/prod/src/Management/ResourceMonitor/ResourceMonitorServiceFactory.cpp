// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Transport;
using namespace Hosting2;
using namespace Federation;

ResourceMonitorServiceFactory::ResourceMonitorServiceFactory(Common::ComPointer<IFabricRuntime> const & fabricRuntimeCPtr)
    : ComponentRoot(),
    fabricRuntimeCPtr_(fabricRuntimeCPtr)
{
}

ResourceMonitorServiceFactory::~ResourceMonitorServiceFactory()
{
}

void ResourceMonitorServiceFactory::Initialize()
{
}

StringLiteral const TraceComponent("ResourceMonitoringAgent");

HRESULT ResourceMonitorServiceFactory::CreateInstance(
    /* [in] */ LPCWSTR serviceType,
    /* [in] */ FABRIC_URI serviceName,
    /* [in] */ ULONG initializationDataLength,
    /* [size_is][in] */ const byte *initializationData,
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_INSTANCE_ID instanceId,
    /* [retval][out] */ IFabricStatelessServiceInstance **serviceInstance)
{
    UNREFERENCED_PARAMETER(initializationDataLength);
    UNREFERENCED_PARAMETER(initializationData);

    wstring serviceTypeStr(serviceType);
    wstring serviceNameStr(serviceName);
    Guid partitionIdGuid(partitionId);

    WriteInfo(
        TraceComponent,
        "ResourceMonitorServiceFactory ::Create instance with serviceType={0} serviceName={1} partition={2} instanceId={3}",
        serviceTypeStr,
        serviceNameStr,
        partitionIdGuid,
        instanceId);

    auto comServiceInstance =
        make_com<ResourceMonitorServiceInstance, IFabricStatelessServiceInstance>(
            serviceNameStr,
            serviceTypeStr,
            partitionIdGuid,
            instanceId,
            fabricRuntimeCPtr_);

    *serviceInstance = comServiceInstance.DetachNoRelease();

    WriteInfo(TraceComponent, "ResourceMonitorServiceFactory end");

    return S_OK;
}

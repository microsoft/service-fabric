//+---------------------------------------------------------------------------
//  Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Naming;
using namespace Reliability;
using namespace Transport;
using namespace Store;
using namespace SystemServices;
using namespace Api;
using namespace Hosting2;
using namespace Federation;
using namespace Client;
using namespace TestHooks;
using namespace Management;
using namespace Management::LocalSecretService;
using namespace Management::ClusterManager;

LocalSecretServiceFactory::LocalSecretServiceFactory()
{
}

LocalSecretServiceFactory::~LocalSecretServiceFactory()
{
}

HRESULT LocalSecretServiceFactory::CreateInstance(
    __in LPCWSTR serviceTypeName,
    __in FABRIC_URI serviceName,
    __in ULONG initializationDataLength,
    __in_ecount(initializationDataLength) const byte* initializationData,
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_INSTANCE_ID instanceId,
    __out IFabricStatelessServiceInstance** serviceInstance
    )
{
    UNREFERENCED_PARAMETER(serviceTypeName);
    UNREFERENCED_PARAMETER(serviceName);
    UNREFERENCED_PARAMETER(initializationDataLength);
    UNREFERENCED_PARAMETER(initializationData);
    UNREFERENCED_PARAMETER(partitionId);
    UNREFERENCED_PARAMETER(instanceId);

    ComPointer<IFabricStatelessServiceInstance> instance = make_com<LocalSecretServiceInstance, IFabricStatelessServiceInstance>();
    *serviceInstance = instance.DetachNoRelease();

    return S_OK;
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Hosting2/DummyStatelessServiceFactory.Test.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

DummyStatelessServiceFactory::DummyStatelessServiceFactory(wstring supportedServiceType)
    : IFabricStatelessServiceFactory(),
    ComUnknownBase(),
    supportedServiceType_(supportedServiceType)
{
}

DummyStatelessServiceFactory::~DummyStatelessServiceFactory()
{
}

HRESULT DummyStatelessServiceFactory::CreateInstance( 
    /* [in] */ LPCWSTR serviceType,
    /* [in] */ FABRIC_URI serviceName,
    /* [in] */ ULONG initializationDataLength,
    /* [size_is][in] */ const byte *initializationData,
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_INSTANCE_ID instanceId,
    /* [retval][out] */ IFabricStatelessServiceInstance **serviceInstance)
{
    UNREFERENCED_PARAMETER(serviceType);
    UNREFERENCED_PARAMETER(serviceName);
    UNREFERENCED_PARAMETER(initializationDataLength);
    UNREFERENCED_PARAMETER(initializationData);
    UNREFERENCED_PARAMETER(partitionId);
    UNREFERENCED_PARAMETER(instanceId);
    UNREFERENCED_PARAMETER(serviceInstance);

    return E_NOTIMPL;
}

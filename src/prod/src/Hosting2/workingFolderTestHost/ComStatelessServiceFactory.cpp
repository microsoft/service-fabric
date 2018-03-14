// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

StringLiteral const TraceType("WorkingFolderTestHost-ServiceFactory");

ComStatelessServiceFactory::ComStatelessServiceFactory()
{
    traceId_ = wformatString("{0}", static_cast<void*>(this));

    WriteNoise(TraceType, traceId_, "ctor");
}

ComStatelessServiceFactory::~ComStatelessServiceFactory()
{
    WriteNoise(TraceType, traceId_, "dtor");
}

HRESULT ComStatelessServiceFactory::CreateInstance( 
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

    *serviceInstance = NULL;
    return E_NOTIMPL;
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "MyStatelessServiceInstance.h"
#include "MyStatelessServiceFactory.h"

using namespace std;

MyStatelessServiceFactory::MyStatelessServiceFactory()
    : FabricUnknownBase()
{
}

MyStatelessServiceFactory::~MyStatelessServiceFactory()
{
}

HRESULT STDMETHODCALLTYPE MyStatelessServiceFactory::QueryInterface(
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    if (riid == IID_IFabricStatelessServiceFactory)
    {
        return FabricUnknownBase::GetAndAddRef(ppvObject);
    }
    else
    {
        return FabricUnknownBase::QueryInterface(riid, ppvObject);
    }
}

ULONG STDMETHODCALLTYPE MyStatelessServiceFactory::AddRef(void)
{
    return FabricUnknownBase::AddRef();
}

ULONG STDMETHODCALLTYPE MyStatelessServiceFactory::Release(void)
{
    return FabricUnknownBase::Release();
}

HRESULT STDMETHODCALLTYPE MyStatelessServiceFactory::CreateInstance(
    /* [in] */ LPCWSTR serviceTypeName,
    /* [in] */ FABRIC_URI serviceName,
    /* [in] */ ULONG initializationDataLength,
    /* [size_is][in] */ const byte *initializationData,
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_INSTANCE_ID instanceId,
    /* [retval][out] */ IFabricStatelessServiceInstance **serviceInstance)
{
    printf("Service instance %u: create\n", instanceId);

    *serviceInstance = new MyStatelessServiceInstance(partitionId, instanceId);

    return S_OK;
}


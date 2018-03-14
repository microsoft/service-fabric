// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FabricCompletedAsyncOperationContext.h"
#include "FabricString.h"
#include "MyStatelessServiceInstance.h"

MyStatelessServiceInstance::MyStatelessServiceInstance(
    FABRIC_PARTITION_ID partitionId, 
    FABRIC_INSTANCE_ID instanceId)
    : FabricUnknownBase()
    , _partitionId(partitionId)
    , _instanceId(instanceId)
{
    printf("Service instance %u: ctor\n", _instanceId);
}

MyStatelessServiceInstance::~MyStatelessServiceInstance()
{
    printf("Service instance %u: dtor\n", _instanceId);
}

HRESULT STDMETHODCALLTYPE MyStatelessServiceInstance::QueryInterface(
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    if (riid == IID_IFabricStatelessServiceInstance)
    {
        return FabricUnknownBase::GetAndAddRef(ppvObject);
    }
    else
    {
        return FabricUnknownBase::QueryInterface(riid, ppvObject);
    }
}

ULONG STDMETHODCALLTYPE MyStatelessServiceInstance::AddRef(void)
{
    return FabricUnknownBase::AddRef();
}

ULONG STDMETHODCALLTYPE MyStatelessServiceInstance::Release(void)
{
    return FabricUnknownBase::Release();
}

HRESULT STDMETHODCALLTYPE MyStatelessServiceInstance::BeginOpen(
    /* [in] */ IFabricStatelessServicePartition *partition,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    printf("Service instance %u: BeginOpen\n", _instanceId);

    *context = new FabricCompletedAsyncOperationContext(callback);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE MyStatelessServiceInstance::EndOpen(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **serviceAddress)
{
    printf("Service instance %u: EndOpen\n", _instanceId);

    *serviceAddress = new FabricString(L"MyEndpoint");
    return S_OK;
}

HRESULT STDMETHODCALLTYPE MyStatelessServiceInstance::BeginClose(
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    printf("Service instance %u: BeginClose\n", _instanceId);

    *context = new FabricCompletedAsyncOperationContext(callback);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE MyStatelessServiceInstance::EndClose(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    printf("Service instance %u: EndClose\n", _instanceId);

    return S_OK;
}

void STDMETHODCALLTYPE MyStatelessServiceInstance::Abort(void)
{
    printf("Service instance %u: Abort\n", _instanceId);
}


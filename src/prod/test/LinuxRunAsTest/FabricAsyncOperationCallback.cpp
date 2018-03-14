// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FabricAsyncOperationCallback.h"

using namespace std;

FabricAsyncOperationCallback::FabricAsyncOperationCallback() 
    : FabricUnknownBase()
    , _callback()
{
}

FabricAsyncOperationCallback::~FabricAsyncOperationCallback()
{
}

FabricAsyncOperationCallback * FabricAsyncOperationCallback::Create(function<void(void)> const & callback)
{
    auto * obj = new FabricAsyncOperationCallback();
    obj->SetCallback(callback);
    return obj;
}

void FabricAsyncOperationCallback::SetCallback(function<void(void)> const & callback)
{
    _callback = callback;
}

HRESULT STDMETHODCALLTYPE FabricAsyncOperationCallback::QueryInterface(
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    if (riid == IID_IFabricAsyncOperationCallback)
    {
        return FabricUnknownBase::GetAndAddRef(ppvObject);
    }
    else
    {
        return FabricUnknownBase::QueryInterface(riid, ppvObject);
    }
}

ULONG STDMETHODCALLTYPE FabricAsyncOperationCallback::AddRef(void)
{
    return FabricUnknownBase::AddRef();
}

ULONG STDMETHODCALLTYPE FabricAsyncOperationCallback::Release(void)
{
    return FabricUnknownBase::Release();
}

void STDMETHODCALLTYPE FabricAsyncOperationCallback::Invoke(
        /* [in] */ IFabricAsyncOperationContext *context)
{
    _callback();
}


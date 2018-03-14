// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FabricAsyncOperationCallback.h"
#include "FabricCompletedAsyncOperationContext.h"

FabricCompletedAsyncOperationContext::FabricCompletedAsyncOperationContext(IFabricAsyncOperationCallback * callback)
    : FabricUnknownBase()
    , _callback(callback)
{
}

FabricCompletedAsyncOperationContext::~FabricCompletedAsyncOperationContext()
{
}

HRESULT STDMETHODCALLTYPE FabricCompletedAsyncOperationContext::QueryInterface(
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    if (riid == IID_IFabricAsyncOperationContext)
    {
        return FabricUnknownBase::GetAndAddRef(ppvObject);
    }
    else
    {
        return FabricUnknownBase::QueryInterface(riid, ppvObject);
    }
}

ULONG STDMETHODCALLTYPE FabricCompletedAsyncOperationContext::AddRef(void)
{
    return FabricUnknownBase::AddRef();
}

ULONG STDMETHODCALLTYPE FabricCompletedAsyncOperationContext::Release(void)
{
    return FabricUnknownBase::Release();
}

BOOLEAN STDMETHODCALLTYPE FabricCompletedAsyncOperationContext::IsCompleted(void)
{
    return TRUE;
}

BOOLEAN STDMETHODCALLTYPE FabricCompletedAsyncOperationContext::CompletedSynchronously(void)
{
    return TRUE;
}

HRESULT STDMETHODCALLTYPE FabricCompletedAsyncOperationContext::get_Callback(
    /* [retval][out] */ IFabricAsyncOperationCallback **callback)
{
    *callback = _callback;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FabricCompletedAsyncOperationContext::Cancel(void)
{
    return S_OK;
}


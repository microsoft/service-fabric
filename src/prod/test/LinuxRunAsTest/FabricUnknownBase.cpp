// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FabricUnknownBase.h"

EXTERN_GUID(IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

FabricUnknownBase::FabricUnknownBase() : _refCount()
{
    this->AddRef();
}

FabricUnknownBase::~FabricUnknownBase()
{
}

HRESULT STDMETHODCALLTYPE FabricUnknownBase::GetAndAddRef(
    /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    *ppvObject = this;
    this->AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FabricUnknownBase::QueryInterface(
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    if (riid == IID_IUnknown)
    {
        return this->GetAndAddRef(ppvObject);
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE FabricUnknownBase::AddRef(void)
{
    return ++_refCount;
}

ULONG STDMETHODCALLTYPE FabricUnknownBase::Release(void)
{
    auto count = --_refCount;
    if (count == 0)
    {
        delete this;
    }
    return count;
}


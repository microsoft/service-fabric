// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FabricString.h"

using namespace std;

FabricString::FabricString(wstring const & value)
    : FabricUnknownBase()
    , _string(value)
{
}

FabricString::~FabricString()
{
}

HRESULT STDMETHODCALLTYPE FabricString::QueryInterface(
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    if (riid == IID_IFabricStringResult)
    {
        return FabricUnknownBase::GetAndAddRef(ppvObject);
    }
    else
    {
        return FabricUnknownBase::QueryInterface(riid, ppvObject);
    }
}

ULONG STDMETHODCALLTYPE FabricString::AddRef(void)
{
    return FabricUnknownBase::AddRef();
}

ULONG STDMETHODCALLTYPE FabricString::Release(void)
{
    return FabricUnknownBase::Release();
}

LPCWSTR STDMETHODCALLTYPE FabricString::get_String(void)
{
    return _string.c_str();
}


// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"
#include "FabricUnknownBase.h"

class FabricString 
    : public IFabricStringResult
    , public FabricUnknownBase
{
public:
    FabricString(std::wstring const &);
    
    virtual ~FabricString();

public: // IUknown

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override;

    virtual ULONG STDMETHODCALLTYPE AddRef(void) override;

    virtual ULONG STDMETHODCALLTYPE Release(void) override;

public: // IFabricStringResult

    virtual LPCWSTR STDMETHODCALLTYPE get_String(void) override;

private:
    std::wstring _string;
};


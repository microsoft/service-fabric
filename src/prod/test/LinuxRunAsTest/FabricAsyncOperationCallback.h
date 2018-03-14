// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"
#include "FabricUnknownBase.h"

class FabricAsyncOperationCallback 
    : public IFabricAsyncOperationCallback
    , public FabricUnknownBase
{
public:

    static FabricAsyncOperationCallback * Create(std::function<void(void)> const & callback);

    virtual ~FabricAsyncOperationCallback();

public: // IUknown

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override;

    virtual ULONG STDMETHODCALLTYPE AddRef(void) override;

    virtual ULONG STDMETHODCALLTYPE Release(void) override;

public: // IFabricAsyncOperationCallback

    virtual void STDMETHODCALLTYPE Invoke(
        /* [in] */ IFabricAsyncOperationContext *context) override;

protected:

    FabricAsyncOperationCallback();

    void SetCallback(std::function<void(void)> const & callback);

private:

    std::function<void(void)> _callback;
};


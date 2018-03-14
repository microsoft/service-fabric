// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"
#include "FabricUnknownBase.h"

class FabricCompletedAsyncOperationContext 
    : public IFabricAsyncOperationContext
    , public FabricUnknownBase
{
public:
    
    FabricCompletedAsyncOperationContext(IFabricAsyncOperationCallback *);

    virtual ~FabricCompletedAsyncOperationContext();

public: // IUknown

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override;

    virtual ULONG STDMETHODCALLTYPE AddRef(void) override;

    virtual ULONG STDMETHODCALLTYPE Release(void) override;

public: // IFabricAsyncOperationContext

    virtual BOOLEAN STDMETHODCALLTYPE IsCompleted(void) override;

    virtual BOOLEAN STDMETHODCALLTYPE CompletedSynchronously(void) override;

    virtual HRESULT STDMETHODCALLTYPE get_Callback(
        /* [retval][out] */ IFabricAsyncOperationCallback **callback) override;

    virtual HRESULT STDMETHODCALLTYPE Cancel(void) override;

private:

    IFabricAsyncOperationCallback * _callback;
};


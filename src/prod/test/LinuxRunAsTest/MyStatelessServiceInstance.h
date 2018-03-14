// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"
#include "FabricUnknownBase.h"

class MyStatelessServiceInstance 
    : public IFabricStatelessServiceInstance
    , public FabricUnknownBase
{

public:

    MyStatelessServiceInstance(FABRIC_PARTITION_ID, FABRIC_INSTANCE_ID);
    virtual ~MyStatelessServiceInstance();

public: // IUknown

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override;

    virtual ULONG STDMETHODCALLTYPE AddRef(void) override;

    virtual ULONG STDMETHODCALLTYPE Release(void) override;

public: // IFabricStatelessServiceInstance

    virtual HRESULT STDMETHODCALLTYPE BeginOpen(
        /* [in] */ IFabricStatelessServicePartition *partition,
        /* [in] */ IFabricAsyncOperationCallback *callback,
        /* [retval][out] */ IFabricAsyncOperationContext **context) override;

    virtual HRESULT STDMETHODCALLTYPE EndOpen(
        /* [in] */ IFabricAsyncOperationContext *context,
        /* [retval][out] */ IFabricStringResult **serviceAddress) override;

    virtual HRESULT STDMETHODCALLTYPE BeginClose(
        /* [in] */ IFabricAsyncOperationCallback *callback,
        /* [retval][out] */ IFabricAsyncOperationContext **context) override;

    virtual HRESULT STDMETHODCALLTYPE EndClose(
        /* [in] */ IFabricAsyncOperationContext *context) override;

    virtual void STDMETHODCALLTYPE Abort(void) override;

private:

    FABRIC_PARTITION_ID _partitionId;
    FABRIC_INSTANCE_ID _instanceId;
};


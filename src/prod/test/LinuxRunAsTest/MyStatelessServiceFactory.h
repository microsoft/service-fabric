// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"
#include "FabricUnknownBase.h"

class MyStatelessServiceFactory 
    : public IFabricStatelessServiceFactory
    , public FabricUnknownBase
{
public:
    MyStatelessServiceFactory();

    virtual ~MyStatelessServiceFactory();

public: // IUknown

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override;

    virtual ULONG STDMETHODCALLTYPE AddRef(void) override;

    virtual ULONG STDMETHODCALLTYPE Release(void) override;

public: // IFabricStatelessServiceFactory

    virtual HRESULT STDMETHODCALLTYPE CreateInstance(
        /* [in] */ LPCWSTR serviceTypeName,
        /* [in] */ FABRIC_URI serviceName,
        /* [in] */ ULONG initializationDataLength,
        /* [size_is][in] */ const byte *initializationData,
        /* [in] */ FABRIC_PARTITION_ID partitionId,
        /* [in] */ FABRIC_INSTANCE_ID instanceId,
        /* [retval][out] */ IFabricStatelessServiceInstance **serviceInstance) override;
};


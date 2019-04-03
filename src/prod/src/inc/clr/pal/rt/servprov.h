// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "rpc.h"
#include "rpcndr.h"

#include "unknwn.h"

#ifndef __IServiceProvider_INTERFACE_DEFINED__
#define __IServiceProvider_INTERFACE_DEFINED__

// 6d5140c1-7436-11ce-8034-00aa006009fa
EXTERN_C const IID IID_IServiceProvider;

interface IServiceProvider : public IUnknown
{
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE QueryService( 
        /* [in] */ REFGUID guidService,
        /* [in] */ REFIID riid,
        /* [out] */ void **ppvObject) = 0;    
};

#endif // __IServiceProvider_INTERFACE_DEFINED__

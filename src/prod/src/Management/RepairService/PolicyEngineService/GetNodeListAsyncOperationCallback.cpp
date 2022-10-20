// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace RepairPolicyEngine;
using namespace Common;

HRESULT STDMETHODCALLTYPE GetNodeListAsyncOperationCallback::Wait(void)
{
    _completed.WaitOne();
    return _hresult;
}

void GetNodeListAsyncOperationCallback::Initialize(ComPointer<IFabricQueryClient> fabricQueryClientCPtr)
{
    _hresult = S_OK;
    _completed.Reset();
    _fabricQueryClientCPtrCPtr = fabricQueryClientCPtr;
}

void GetNodeListAsyncOperationCallback::Invoke(/* [in] */ IFabricAsyncOperationContext *context)
{
    _hresult = _fabricQueryClientCPtrCPtr->EndGetNodeList(context, Result.InitializationAddress());
    _completed.Set();
}

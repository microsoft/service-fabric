// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace RepairPolicyEngine;
using namespace Common;

HRESULT STDMETHODCALLTYPE GetNodeHealthAsyncOperationCallback::Wait(void)
{
    _completed.WaitOne();
    return _hresult;
}

void GetNodeHealthAsyncOperationCallback::Initialize(ComPointer<IFabricHealthClient> fabricHealthClientCPtr)
{
    _completed.Reset();
    _fabricHealthClientCPtrCPtr = fabricHealthClientCPtr;
    _hresult = S_OK;
}

void GetNodeHealthAsyncOperationCallback::Invoke(/* [in] */ IFabricAsyncOperationContext *context)
{
    CODING_ERROR_ASSERT(_fabricHealthClientCPtrCPtr.GetRawPointer() != nullptr);
    _hresult = _fabricHealthClientCPtrCPtr->EndGetNodeHealth(context, Result.InitializationAddress());
    _completed.Set();
}

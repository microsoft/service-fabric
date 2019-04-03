// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace RepairPolicyEngine;
using namespace Common;

FabricAsyncOperationContext::FabricAsyncOperationContext()
{
    Trace.WriteNoise(ComponentName, "FabricAsyncOperationContext::FabricAsyncOperationContext()");
}

FabricAsyncOperationContext::~FabricAsyncOperationContext()
{
    Trace.WriteNoise(ComponentName, "FabricAsyncOperationContext::~FabricAsyncOperationContext()");
}

void FabricAsyncOperationContext::Initialize(IFabricAsyncOperationCallback* callback)
{
    Trace.WriteNoise(ComponentName, "FabricAsyncOperationContext::Initialize()");

    if (callback != NULL)
    {
        callbackCPtr_.SetAndAddRef(callback);

        // we execute synchronously in RepairPolicyEngine
        callbackCPtr_->Invoke(this); 
    }
}

BOOLEAN STDMETHODCALLTYPE FabricAsyncOperationContext::IsCompleted(void)
{
    Trace.WriteNoise(ComponentName, "FabricAsyncOperationContext::IsCompleted()");
    return TRUE;
}

BOOLEAN STDMETHODCALLTYPE FabricAsyncOperationContext::CompletedSynchronously(void)
{
    Trace.WriteNoise(ComponentName, "FabricAsyncOperationContext::CompletedSynchronously()");
    return TRUE;
}

HRESULT STDMETHODCALLTYPE FabricAsyncOperationContext::get_Callback( 
    /* [retval][out] */ IFabricAsyncOperationCallback **callback)
{
    Trace.WriteNoise(ComponentName, "FabricAsyncOperationContext::get_Callback()");
    ComPointer<IFabricAsyncOperationCallback> copy = callbackCPtr_;
    *callback = copy.DetachNoRelease();
    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT STDMETHODCALLTYPE FabricAsyncOperationContext::Cancel(void)
{
    Trace.WriteNoise(ComponentName, "FabricAsyncOperationContext::Cancel()");
    return ComUtility::OnPublicApiReturn(S_OK);
}

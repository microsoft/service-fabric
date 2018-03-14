// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FabricAsyncOperationContext.h"

/*static*/
void FabricAsyncOperationContext::Create(
    __out FabricAsyncOperationContext::SPtr& spContext,
    __in KAllocator& allocator,
    __in IFabricAsyncOperationCallback* pCallback
    )
{
    spContext = _new(TAG, allocator) FabricAsyncOperationContext(pCallback);
    KInvariant(spContext != nullptr);
}

FabricAsyncOperationContext::FabricAsyncOperationContext(
    __in IFabricAsyncOperationCallback* pCallback
    ) : _spCallback(pCallback),
    _fIsCompleted(FALSE),
    _fCompletedSync(FALSE)
{
}

FabricAsyncOperationContext::~FabricAsyncOperationContext()
{
}

HRESULT FabricAsyncOperationContext::get_Callback(
    __out IFabricAsyncOperationCallback **callback
    )
{
    ComPointer<IFabricAsyncOperationCallback> spCallback = _spCallback;
    *callback = spCallback.Detach();

    return S_OK;
}

HRESULT FabricAsyncOperationContext::Cancel(void)
{
    return S_OK;
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FabricAsyncOperationCallback.h"

/*static*/
void FabricAsyncOperationCallback::Create(
    __out FabricAsyncOperationCallback::SPtr& spContext,
    __in KAllocator& allocator
    )
{
    spContext = _new(TAG, allocator) FabricAsyncOperationCallback();
    KInvariant(spContext != nullptr);
}

FabricAsyncOperationCallback::FabricAsyncOperationCallback(
    ) : _event(false)
{
}

FabricAsyncOperationCallback::~FabricAsyncOperationCallback()
{
}

void FabricAsyncOperationCallback::Invoke(
    __in IFabricAsyncOperationContext *context
    )
{
    UNREFERENCED_PARAMETER(context);

    _event.Set();
}

bool FabricAsyncOperationCallback::Wait(
    __in ULONG timeoutInMilliseconds
    )
{
    Common::ErrorCode error = _event.Wait(timeoutInMilliseconds);
    if (error.IsSuccess())
    {
        return true;
    }

    return false;
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "OperationCallback.h"

/*static*/
void OperationCallback::Create(
    __out OperationCallback::SPtr& spCallback,
    __in KAllocator& allocator,
    __in FabricAsyncOperationCallback callback
    )
{
    spCallback = _new(TAG, allocator) OperationCallback(callback);
    KInvariant(spCallback != nullptr);
}

OperationCallback::OperationCallback(
    __in FabricAsyncOperationCallback callback
) : _callback(callback)
{
}

OperationCallback::~OperationCallback()
{
}

void OperationCallback::Invoke(
    __in_opt IFabricAsyncOperationContext *context
    )
{
    _callback(context);
}

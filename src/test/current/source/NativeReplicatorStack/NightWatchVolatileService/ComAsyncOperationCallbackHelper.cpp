// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace V1ReplPerf;

ComAsyncOperationCallbackHelper::ComAsyncOperationCallbackHelper(std::function<void(IFabricAsyncOperationContext *)> const & callback)
    : callback_(callback)
{
}

void STDMETHODCALLTYPE ComAsyncOperationCallbackHelper::Invoke(IFabricAsyncOperationContext * aoc)
{
    if (callback_)
    {
        callback_(aoc);
    }
}

BOOLEAN ComAsyncOperationCallbackHelper::GetCompletedSynchronously(IFabricAsyncOperationContext * context)
{
    return context->CompletedSynchronously();
}

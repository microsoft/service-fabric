// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "testFabricAsyncCallbackHelper.h"

namespace ReplicationUnitTest
{
    TestFabricAsyncCallbackHelper::TestFabricAsyncCallbackHelper(std::function<void(IFabricAsyncOperationContext *)> const & callback)
        : callback_(callback)
    {
    }

    void STDMETHODCALLTYPE TestFabricAsyncCallbackHelper::Invoke(IFabricAsyncOperationContext * aoc)
    {
        if (callback_)
        {
            callback_(aoc);
        }
    }
}

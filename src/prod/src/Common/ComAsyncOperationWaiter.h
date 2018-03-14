// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

#include "FabricCommon.h"

namespace Common
{
    typedef std::function<void(IFabricAsyncOperationContext *context)> ComAsyncCallback;

    class ComAsyncOperationWaiter : public ManualResetEvent
    {
        DENY_COPY(ComAsyncOperationWaiter)

    public:
        ComAsyncOperationWaiter();
        virtual ~ComAsyncOperationWaiter();

        inline HRESULT GetHRESULT() { return hr_; }
        inline void SetHRESULT(HRESULT hr) { hr_ = hr; }

        IFabricAsyncOperationCallback * GetAsyncOperationCallback(ComAsyncCallback const & userCallback);

    private:
        class ComAsyncOperationCallback;

    private:
        ComPointer<ComAsyncOperationCallback> callback_;
        HRESULT hr_;
    };
}

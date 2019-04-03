// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace V1ReplPerf
{
    class ComAsyncOperationCallbackHelper
        : public IFabricAsyncOperationCallback
        , private Common::ComUnknownBase
    {
        COM_INTERFACE_LIST1(ComAsyncOperationCallbackHelper, IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback);

    public:
        ComAsyncOperationCallbackHelper(std::function<void(IFabricAsyncOperationContext *)> const &);

        void STDMETHODCALLTYPE Invoke(IFabricAsyncOperationContext *);

        static BOOLEAN GetCompletedSynchronously(IFabricAsyncOperationContext * context);
    private:
        std::function<void(IFabricAsyncOperationContext *)> callback_;
    };
}

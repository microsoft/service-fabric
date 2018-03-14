// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ComAsyncOperationCallbackTestHelper
        : public IFabricAsyncOperationCallback
        // IFabricAsyncOperationCallback derives from IUnknown
        , private Common::ComUnknownBase
    {
        COM_INTERFACE_LIST1(ComAsyncOperationCallbackTestHelper, IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback);

    public:
        ComAsyncOperationCallbackTestHelper(std::function<void(IFabricAsyncOperationContext *)> const &);

        void STDMETHODCALLTYPE Invoke(IFabricAsyncOperationContext *);

    private:
        std::function<void(IFabricAsyncOperationContext *)> callback_;
    };
}

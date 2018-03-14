// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TXRStatefulServiceBase
{
    class TestComProxyDataLossHandler :
         public IFabricDataLossHandler,
         public Common::ComUnknownBase
    {
        BEGIN_COM_INTERFACE_LIST(TestComStateProvider2Factory)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricDataLossHandler)
            COM_INTERFACE_ITEM(IID_IFabricDataLossHandler, IFabricDataLossHandler)
        END_COM_INTERFACE_LIST()

    public:
        STDMETHOD_(HRESULT, BeginOnDataLoss)(
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [retval][out] */ IFabricAsyncOperationContext ** context) override
        {
            UNREFERENCED_PARAMETER(callback);
            UNREFERENCED_PARAMETER(context);
            return S_OK;
        }

        STDMETHOD_(HRESULT, EndOnDataLoss)(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [retval][out] */ BOOLEAN * isStateChanged) override
        {
            UNREFERENCED_PARAMETER(isStateChanged);
            UNREFERENCED_PARAMETER(context);
            return S_OK;
        }

        // Constructor
        TestComProxyDataLossHandler(__in BOOLEAN isStateChanged)
        {
            isStateChanged_ = isStateChanged;
        }

    private:
        BOOLEAN isStateChanged_;
    };
}

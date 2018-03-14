// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComDataLossHandler :
        public IFabricDataLossHandler,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComDataLossHandler)

        BEGIN_COM_INTERFACE_LIST(ComDataLossHandler)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricDataLossHandler)
            COM_INTERFACE_ITEM(IID_IFabricDataLossHandler, IFabricDataLossHandler)
        END_COM_INTERFACE_LIST()

    public:
        ComDataLossHandler(IDataLossHandlerPtr const & impl);
        virtual ~ComDataLossHandler();

        IDataLossHandlerPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricDataLossHandler methods
        // 
        HRESULT STDMETHODCALLTYPE BeginOnDataLoss(
            IFabricAsyncOperationCallback * callback,
            IFabricAsyncOperationContext ** context);

        HRESULT STDMETHODCALLTYPE EndOnDataLoss(
            IFabricAsyncOperationContext * context,
            BOOLEAN * isStateChanged);

    private:
        class OnDataLossAsyncOperation;

        IDataLossHandlerPtr impl_;
    };
}

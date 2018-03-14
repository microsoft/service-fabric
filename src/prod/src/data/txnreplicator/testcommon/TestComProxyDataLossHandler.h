// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    namespace TestCommon
    {
        class TestComProxyDataLossHandler final
            : public KObject<TestComProxyDataLossHandler>
            , public KShared<TestComProxyDataLossHandler>
            , public IFabricDataLossHandler
        {
            K_FORCE_SHARED(TestComProxyDataLossHandler)

            K_BEGIN_COM_INTERFACE_LIST(TestComStateProvider2Factory)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricDataLossHandler)
                COM_INTERFACE_ITEM(IID_IFabricDataLossHandler, IFabricDataLossHandler)
            K_END_COM_INTERFACE_LIST()

        public:
            static Common::ComPointer<IFabricDataLossHandler> Create(
                __in KAllocator & allocator,
                __in_opt TxnReplicator::IDataLossHandler * const dataLossHandler = nullptr);

            STDMETHOD_(HRESULT, BeginOnDataLoss)(
                /* [in] */ IFabricAsyncOperationCallback * callback,
                /* [retval][out] */ IFabricAsyncOperationContext ** context) override;

            STDMETHOD_(HRESULT, EndOnDataLoss)(
                /* [in] */ IFabricAsyncOperationContext * context,
                /* [retval][out] */ BOOLEAN * isStateChanged) override;

        private:
            // Constructor
            TestComProxyDataLossHandler(
                __in_opt TxnReplicator::IDataLossHandler * const dataLossHandler);

        private:
            // Parent. Note that this can be pointing at nullptr.
            TxnReplicator::IDataLossHandler::SPtr dataLossHandlerSPtr_;
        };
    }
}

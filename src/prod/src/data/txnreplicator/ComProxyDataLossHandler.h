// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{ 
    // Data Loss proxy between ktl async and common async call
    class ComProxyDataLossHandler
        : public KObject<ComProxyDataLossHandler>
        , public KShared<ComProxyDataLossHandler>
        , public IDataLossHandler
    {
        K_FORCE_SHARED(ComProxyDataLossHandler)
        K_SHARED_INTERFACE_IMP(IDataLossHandler)

    public:
        // Create the ComProxyDataLossHandlerSPtr
        static NTSTATUS Create(
            __in Common::ComPointer<IFabricDataLossHandler> & dataLossHandler,
            __in KAllocator & allocator,
            __out ComProxyDataLossHandler::SPtr & result);

    public:
        // ktl Async DataLossAsync call
        ktl::Awaitable<bool> DataLossAsync(__in ktl::CancellationToken const & cancellationToken) override;

    private:
        // Constructor
        ComProxyDataLossHandler(__in Common::ComPointer<IFabricDataLossHandler> & dataLossHandler);

        class AsyncDataLossContextBase :
            public Ktl::Com::FabricAsyncContextBase
        {
        };

        // DataLoss Async operation
        class AsyncDataLossContext
            : public Ktl::Com::AsyncCallOutAdapter<AsyncDataLossContextBase>
        {
            friend ComProxyDataLossHandler;

            K_FORCE_SHARED(AsyncDataLossContext)

        public:
            ktl::Awaitable<NTSTATUS> DataLossAsync(__out BOOLEAN * isStateChanged);

        protected:
            HRESULT OnEnd(__in IFabricAsyncOperationContext & context);

        private:
            BOOLEAN isStateChanged_;
            ComProxyDataLossHandler::SPtr parent_;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr tcs_;
        };

    private:
        Common::ComPointer<IFabricDataLossHandler> dataLossHandler_;
    };
}

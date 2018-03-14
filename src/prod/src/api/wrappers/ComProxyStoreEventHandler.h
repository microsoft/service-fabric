// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxyStoreEventHandler  :
        public Common::ComponentRoot,
        public IStoreEventHandler
    {
        DENY_COPY(ComProxyStoreEventHandler );

    public:
        ComProxyStoreEventHandler(Common::ComPointer<IFabricStoreEventHandler> const & comImpl);
        ComProxyStoreEventHandler(Common::ComPointer<IFabricStoreEventHandler2> const & comImpl2);
        virtual ~ComProxyStoreEventHandler();

        //
        // IStoreEventHandler methods
        //
        void OnDataLoss(void);

        Common::AsyncOperationSPtr BeginOnDataLoss(
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & state);

        Common::ErrorCode EndOnDataLoss(
            __in Common::AsyncOperationSPtr const & asyncOperation,
            __out bool & isStateChanged);
            
    private:
        class OnDataLossAsyncOperation;

        Common::ComPointer<IFabricStoreEventHandler> const comImpl_;
        Common::ComPointer<IFabricStoreEventHandler2> const comImpl2_;
    };
}

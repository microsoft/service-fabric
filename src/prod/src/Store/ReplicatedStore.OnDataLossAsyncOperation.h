// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStore::OnDataLossAsyncOperation : public Common::AsyncOperation
    {
    public:
        OnDataLossAsyncOperation(
            __in Api::IStoreEventHandlerPtr const & storeEventHandler,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation, __out bool & isStateChanged);

    protected:
        void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

    private:
        void OnDataLossCompleted(Common::AsyncOperationSPtr const & operation);

        Api::IStoreEventHandlerPtr const & storeEventHandler_;
        bool isStateChanged_;
    };
}


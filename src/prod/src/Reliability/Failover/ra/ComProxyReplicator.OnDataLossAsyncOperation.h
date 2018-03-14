// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ComProxyReplicator::OnDataLossAsyncOperation : public Common::ComProxyAsyncOperation
        {
            DENY_COPY(OnDataLossAsyncOperation);
        public:
            OnDataLossAsyncOperation(
                ComProxyReplicator & owner,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            static Common::ErrorCode End(Common::AsyncOperationSPtr const & asyncOperation, __out bool & isStateChanged);

        protected:
            HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context);
            HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context);

        private:
            ComProxyReplicator & owner_;
            bool isStateChanged_;
        };
    }
}

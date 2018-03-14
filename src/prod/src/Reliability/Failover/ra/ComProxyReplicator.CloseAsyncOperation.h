// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ComProxyReplicator::CloseAsyncOperation : public Common::ComProxyAsyncOperation
        {
            DENY_COPY(CloseAsyncOperation);
        public:
            CloseAsyncOperation(
                ComProxyReplicator & owner,
                Common::AsyncCallback callback,
                Common::AsyncOperationSPtr const & parent) :
            Common::ComProxyAsyncOperation(callback, parent, true),
                owner_(owner)
            {
                // Empty
            }

            static Common::ErrorCode End(Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context);
            HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context);

        private:
            ComProxyReplicator & owner_;
        };
    }
}

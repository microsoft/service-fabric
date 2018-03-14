// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ComProxyReplicator::OpenAsyncOperation : public Common::ComProxyAsyncOperation
        {
            DENY_COPY(OpenAsyncOperation);
        public:
            OpenAsyncOperation(
                ComProxyReplicator & owner,
                Common::AsyncCallback callback,
                Common::AsyncOperationSPtr const & parent) :
            Common::ComProxyAsyncOperation(callback, parent, true),
                owner_(owner),
                replicationEndpoint_()
            {
                // Empty
            }

            static Common::ErrorCode End(Common::AsyncOperationSPtr const & asyncOperation, __out std::wstring & replicationEndpoint);

        protected:
            HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context);
            HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context);

        private:
            ComProxyReplicator & owner_;
            std::wstring replicationEndpoint_;
        };
    }
}

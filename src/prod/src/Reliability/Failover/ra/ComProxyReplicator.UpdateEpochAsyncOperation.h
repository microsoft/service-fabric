// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ComProxyReplicator::UpdateEpochAsyncOperation : public Common::ComProxyAsyncOperation
        {
            DENY_COPY(UpdateEpochAsyncOperation);
        public:
            UpdateEpochAsyncOperation(
                ComProxyReplicator & owner,
                FABRIC_EPOCH const & epoch,
                Common::AsyncCallback callback,
                Common::AsyncOperationSPtr const & parent);

            static Common::ErrorCode End(Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context);
            HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context);

        private:
            FABRIC_EPOCH epoch_;
            ComProxyReplicator & owner_;
        };
    }
}

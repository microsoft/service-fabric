// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ComProxyReplicator::CatchupReplicaSetAsyncOperation : public Common::ComProxyAsyncOperation
        {
            DENY_COPY(CatchupReplicaSetAsyncOperation);
        public:
            CatchupReplicaSetAsyncOperation(
                ::FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
                ComProxyReplicator & owner,
                Common::AsyncCallback callback,
                Common::AsyncOperationSPtr const & parent);

            static Common::ErrorCode End(Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context);
            HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context);

        private:
            ::FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode_;
            ComProxyReplicator & owner_;
        };
    }
}

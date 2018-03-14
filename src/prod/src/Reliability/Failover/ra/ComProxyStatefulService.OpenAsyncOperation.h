// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ComProxyStatefulService::OpenAsyncOperation : public Common::ComProxyAsyncOperation
        {
            DENY_COPY(OpenAsyncOperation);
        public:
            OpenAsyncOperation(
                ::FABRIC_REPLICA_OPEN_MODE openMode,
                Common::ComPointer<::IFabricStatefulServicePartition> const & partition, 
                ComProxyStatefulService & owner,
                Common::AsyncCallback callback,
                Common::AsyncOperationSPtr const & parent) :
            Common::ComProxyAsyncOperation(callback, parent, true),
                partition_(partition),
                owner_(owner),
                replicator_(),
                openMode_(openMode)
            {
                // Empty
            }

            static Common::ErrorCode End(Common::AsyncOperationSPtr const & asyncOperation, __out ComProxyReplicatorUPtr & replication);

        protected:
            HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context);
            HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context);

        private:
            Common::ComPointer<::IFabricStatefulServicePartition> partition_;
            ComProxyStatefulService & owner_;
            Common::ComPointer<::IFabricReplicator> replicator_;
            ::FABRIC_REPLICA_OPEN_MODE openMode_;
        };
    }
}

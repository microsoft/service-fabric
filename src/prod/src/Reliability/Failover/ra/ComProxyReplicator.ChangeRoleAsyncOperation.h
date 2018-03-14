// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ComProxyReplicator::ChangeRoleAsyncOperation : public Common::ComProxyAsyncOperation
        {
            DENY_COPY(ChangeRoleAsyncOperation);
        public:
            ChangeRoleAsyncOperation(
                ::FABRIC_EPOCH const & epoch,
                ::FABRIC_REPLICA_ROLE newRole, 
                ComProxyReplicator & owner,
                Common::AsyncCallback callback,
                Common::AsyncOperationSPtr const & parent) :
            Common::ComProxyAsyncOperation(callback, parent, true),
                epoch_(epoch),
                newRole_(newRole),
                owner_(owner)
            {
                // Empty
            }

            static Common::ErrorCode End(Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context);
            HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context);

        private:
            ::FABRIC_EPOCH epoch_;
            ::FABRIC_REPLICA_ROLE newRole_;
            ComProxyReplicator & owner_;
        };
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ComProxyStatefulService::ChangeRoleAsyncOperation : public Common::ComProxyAsyncOperation
        {
            DENY_COPY(ChangeRoleAsyncOperation);
        public:
            ChangeRoleAsyncOperation(
                ::FABRIC_REPLICA_ROLE newRole, 
                ComProxyStatefulService & owner,
                Common::AsyncCallback callback,
                Common::AsyncOperationSPtr const & parent) :
            Common::ComProxyAsyncOperation(callback, parent, true),
                newRole_(newRole),
                owner_(owner),
                serviceLocation_()
            {
                // Empty
            }

            static Common::ErrorCode End(Common::AsyncOperationSPtr const & asyncOperation, __out std::wstring & serviceLocation);

        protected:
            HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context);
            HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context);

        private:
            ::FABRIC_REPLICA_ROLE newRole_;
            ComProxyStatefulService & owner_;
            std::wstring serviceLocation_;
        };
    }
}

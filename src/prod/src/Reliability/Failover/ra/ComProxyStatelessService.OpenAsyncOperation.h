// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ComProxyStatelessService::OpenAsyncOperation : public Common::ComProxyAsyncOperation
        {
            DENY_COPY(OpenAsyncOperation);
        public:

            OpenAsyncOperation(
                Common::ComPointer<::IFabricStatelessServicePartition> const & partition, 
                ComProxyStatelessService & owner,
                Common::AsyncCallback callback,
                Common::AsyncOperationSPtr const & parent) :
            Common::ComProxyAsyncOperation(callback, parent, true),
                partition_(partition),
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
            Common::ComPointer<::IFabricStatelessServicePartition> partition_;
            ComProxyStatelessService & owner_;
            std::wstring serviceLocation_;
        };
    }
}

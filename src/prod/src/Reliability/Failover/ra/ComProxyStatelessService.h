// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ComProxyStatelessService
        {
            DENY_COPY(ComProxyStatelessService);

        public:
            ComProxyStatelessService()
            {
                // Empty
            }

            ComProxyStatelessService(Common::ComPointer<::IFabricStatelessServiceInstance> const & service)
                : service_(service)
            {
                // Empty
            }

            Common::AsyncOperationSPtr BeginOpen(
                Common::ComPointer<::IFabricStatelessServicePartition> const & partition,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode EndOpen(Common::AsyncOperationSPtr const & asyncOperation, __out std::wstring & serviceLocation);

            Common::AsyncOperationSPtr BeginClose(
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode EndClose(Common::AsyncOperationSPtr const & asyncOperation);

            void Abort();

        private:
            Common::ComPointer<::IFabricStatelessServiceInstance> service_;

            class OpenAsyncOperation;
            class CloseAsyncOperation;
        };
    }
}

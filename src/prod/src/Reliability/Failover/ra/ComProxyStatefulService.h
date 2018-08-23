// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ComProxyStatefulService
        {
            DENY_COPY(ComProxyStatefulService);

        public:
            ComProxyStatefulService();

            ComProxyStatefulService(Common::ComPointer<::IFabricStatefulServiceReplica> const &);

            Common::AsyncOperationSPtr BeginOpen(
                ::FABRIC_REPLICA_OPEN_MODE openMode,
                Common::ComPointer<::IFabricStatefulServicePartition> const & partition,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode EndOpen(Common::AsyncOperationSPtr const & asyncOperation, __out ComProxyReplicatorUPtr & replication);

            Common::AsyncOperationSPtr BeginChangeRole(
                ::FABRIC_REPLICA_ROLE newRole,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent);       
            Common::ErrorCode EndChangeRole(Common::AsyncOperationSPtr const & asyncOperation, __out std::wstring & serviceLocation);

            Common::AsyncOperationSPtr BeginClose(
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode EndClose(Common::AsyncOperationSPtr const & asyncOperation);

            void Abort();

            Common::ErrorCode GetQueryResult(__out ServiceModel::ReplicaStatusQueryResultSPtr & result);
            void UpdateInitializationData(std::vector<byte> const &);

        private:
            Common::ComPointer<::IFabricStatefulServiceReplica> service_;
            Common::ComPointer<::IFabricInternalStatefulServiceReplica2> internalService_;

            class OpenAsyncOperation;
            class ChangeRoleAsyncOperation;
            class CloseAsyncOperation;
        };
    }
}

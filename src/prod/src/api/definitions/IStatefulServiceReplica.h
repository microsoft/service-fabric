// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IStatefulServiceReplica
    {
    public:
        virtual ~IStatefulServiceReplica() {};

        virtual Common::AsyncOperationSPtr BeginOpen(
            ::FABRIC_REPLICA_OPEN_MODE openMode,
            Common::ComPointer<::IFabricStatefulServicePartition> const & partition,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndOpen(
            Common::AsyncOperationSPtr const & asyncOperation, 
            __out Common::ComPointer<::IFabricReplicator> & replicator) = 0;

        virtual Common::AsyncOperationSPtr BeginChangeRole(
            ::FABRIC_REPLICA_ROLE newRole,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent) = 0;    

        virtual Common::ErrorCode EndChangeRole(
            Common::AsyncOperationSPtr const & asyncOperation, 
            __out std::wstring & serviceLocation) = 0;

        virtual Common::AsyncOperationSPtr BeginClose(
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndClose(
            Common::AsyncOperationSPtr const & asyncOperation) = 0;

        virtual void Abort() = 0;

        virtual Common::ErrorCode GetQueryStatus(
            __out IStatefulServiceReplicaStatusResultPtr &) = 0;

        virtual Common::ErrorCode UpdateInitializationData(std::vector<byte> &&) = 0;
    };
}

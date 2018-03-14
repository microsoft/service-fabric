// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IInfrastructureServiceAgent
    {
    public:
        virtual ~IInfrastructureServiceAgent() {};

        virtual void Release() = 0;

        virtual Common::ErrorCode RegisterInfrastructureServiceFactory(
            IFabricStatefulServiceFactory *) = 0;

        virtual void RegisterInfrastructureService(
            ::FABRIC_PARTITION_ID,
            ::FABRIC_REPLICA_ID,
            IInfrastructureServicePtr const &,
            __out std::wstring & serviceAddress) = 0;

        virtual void UnregisterInfrastructureService(
            ::FABRIC_PARTITION_ID,
            ::FABRIC_REPLICA_ID) = 0;

        virtual Common::AsyncOperationSPtr BeginStartInfrastructureTask(
            ::FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION * description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndStartInfrastructureTask(
            Common::AsyncOperationSPtr const & asyncOperation) = 0;

        virtual Common::AsyncOperationSPtr BeginFinishInfrastructureTask(
            std::wstring const & taskId,
            uint64 instanceId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndFinishInfrastructureTask(
            Common::AsyncOperationSPtr const & asyncOperation) = 0;

        virtual Common::AsyncOperationSPtr BeginQueryInfrastructureTask(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndQueryInfrastructureTask(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out ServiceModel::QueryResult &) = 0;
    };
}

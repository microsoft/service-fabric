// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IHealthClient
    {
    public:
        virtual ~IHealthClient() {};

        virtual Common::ErrorCode ReportHealth(
            ServiceModel::HealthReport && healthReport,
            ServiceModel::HealthReportSendOptionsUPtr && sendOptions) = 0;

        virtual Common::AsyncOperationSPtr BeginGetClusterHealth(
            ServiceModel::ClusterHealthQueryDescription const & queryDescription,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndGetClusterHealth(
            Common::AsyncOperationSPtr const & operation,
            __inout ServiceModel::ClusterHealth & clusterHealth) = 0;

        virtual Common::AsyncOperationSPtr BeginGetNodeHealth(
            ServiceModel::NodeHealthQueryDescription const & queryDescription,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndGetNodeHealth(
            Common::AsyncOperationSPtr const & operation,
            __inout ServiceModel::NodeHealth & nodeHealth) = 0;

        virtual Common::AsyncOperationSPtr BeginGetApplicationHealth(
            ServiceModel::ApplicationHealthQueryDescription const & queryDescription,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndGetApplicationHealth(
            Common::AsyncOperationSPtr const & operation,
            __inout ServiceModel::ApplicationHealth & applicationHealth) = 0;

        virtual Common::AsyncOperationSPtr BeginGetServiceHealth(
            ServiceModel::ServiceHealthQueryDescription const & queryDescription,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndGetServiceHealth(
            Common::AsyncOperationSPtr const & operation,
            __inout ServiceModel::ServiceHealth & serviceHealth) = 0;

        virtual Common::AsyncOperationSPtr BeginGetPartitionHealth(
            ServiceModel::PartitionHealthQueryDescription const & queryDescription, 
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndGetPartitionHealth(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::PartitionHealth & partitionHealth) = 0;

        virtual Common::AsyncOperationSPtr BeginGetReplicaHealth(
            ServiceModel::ReplicaHealthQueryDescription const & queryDescription,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndGetReplicaHealth(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::ReplicaHealth & replicaHealth) = 0;

        virtual Common::AsyncOperationSPtr BeginGetDeployedApplicationHealth(
            ServiceModel::DeployedApplicationHealthQueryDescription const & queryDescription,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndGetDeployedApplicationHealth(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::DeployedApplicationHealth & deployedApplicationHealth) = 0;

        virtual Common::AsyncOperationSPtr BeginGetDeployedServicePackageHealth(
            ServiceModel::DeployedServicePackageHealthQueryDescription const & queryDescription,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndGetDeployedServicePackageHealth(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::DeployedServicePackageHealth & deployedServicePackageHealth) = 0;

        virtual Common::AsyncOperationSPtr BeginGetClusterHealthChunk(
            ServiceModel::ClusterHealthChunkQueryDescription && queryDescription,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndGetClusterHealthChunk(
            Common::AsyncOperationSPtr const & operation,
            __inout ServiceModel::ClusterHealthChunk & clusterHealthChunk) = 0;
                
        // Internal HealthClient methods
        virtual Common::ErrorCode InternalReportHealth(
            std::vector<ServiceModel::HealthReport> && reports,
            ServiceModel::HealthReportSendOptionsUPtr && sendOptions) = 0;

        virtual Common::ErrorCode HealthPreInitialize(
            std::wstring const & sourceId, 
            FABRIC_HEALTH_REPORT_KIND kind,
            FABRIC_INSTANCE_ID instance) = 0;
        virtual Common::ErrorCode HealthPostInitialize(
            std::wstring const & sourceId, 
            FABRIC_HEALTH_REPORT_KIND kind, 
            FABRIC_SEQUENCE_NUMBER sequence, 
            FABRIC_SEQUENCE_NUMBER invalidateSequence) = 0;
        virtual Common::ErrorCode HealthGetProgress(
            std::wstring const & sourceId, 
            FABRIC_HEALTH_REPORT_KIND kind, 
            __inout FABRIC_SEQUENCE_NUMBER & progress) = 0;
        virtual Common::ErrorCode HealthSkipSequence(
            std::wstring const & sourceId, 
            FABRIC_HEALTH_REPORT_KIND kind, 
            FABRIC_SEQUENCE_NUMBER sequence) = 0;
    };
}

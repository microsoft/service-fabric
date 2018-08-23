// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class IQueryClient
    {
    public:
        virtual ~IQueryClient() {};

        virtual Common::AsyncOperationSPtr BeginInternalQuery(
            std::wstring const &queryName,
            ServiceModel::QueryArgumentMap const &queryArgs,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndInternalQuery(
            Common::AsyncOperationSPtr const & operation,
            __inout ServiceModel::QueryResult &queryResult) = 0;

        virtual Common::AsyncOperationSPtr BeginGetNodeList(
            ServiceModel::NodeQueryDescription const &queryDescription,
            bool excludeStoppedNodeInfo,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::AsyncOperationSPtr BeginGetNodeList(
            std::wstring const &nodeNameFilter,
            std::wstring const &continuationToken,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::AsyncOperationSPtr BeginGetNodeList(
            std::wstring const &nodeNameFilter,
            DWORD nodeStatusFilter,
            bool excludeStoppedNodeInfo,
            std::wstring const &continuationToken,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::AsyncOperationSPtr BeginGetFMNodeList(
            std::wstring const &nodeNameFilter,
            std::wstring const &continuationToken,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetNodeList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::NodeQueryResult> &nodeList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus) = 0;

        virtual Common::AsyncOperationSPtr BeginGetApplicationTypeList(
            std::wstring const &applicationtypeNameFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetApplicationTypeList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::ApplicationTypeQueryResult> &applicationtypeList) = 0;

        virtual Common::AsyncOperationSPtr BeginGetApplicationTypePagedList(
            ServiceModel::ApplicationTypeQueryDescription const & applicationTypeQueryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetApplicationTypePagedList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::ApplicationTypeQueryResult> &applicationtypeList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus) = 0;

        virtual Common::AsyncOperationSPtr BeginGetServiceTypeList(
            std::wstring const &applicationTypeName,
            std::wstring const &applicationTypeVersion,
            std::wstring const &serviceTypeNameFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetServiceTypeList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::ServiceTypeQueryResult> &servicetypeList) = 0;

        virtual Common::AsyncOperationSPtr BeginGetServiceGroupMemberTypeList(
            std::wstring const &applicationTypeName,
            std::wstring const &applicationTypeVersion,
            std::wstring const &serviceGroupTypeNameFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetServiceGroupMemberTypeList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::ServiceGroupMemberTypeQueryResult> &servicegroupmembertypeList) = 0;

        virtual Common::AsyncOperationSPtr BeginGetApplicationList(
            ServiceModel::ApplicationQueryDescription const & applicationQueryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetApplicationList(
            Common::AsyncOperationSPtr const &,
            __inout std::vector<ServiceModel::ApplicationQueryResult> &applicationList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus) = 0;

        virtual Common::AsyncOperationSPtr BeginGetServiceList(
            ServiceModel::ServiceQueryDescription const & serviceQueryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetServiceList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::ServiceQueryResult> &serviceList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus) = 0;

        virtual Common::AsyncOperationSPtr BeginGetServiceGroupMemberList(
            Common::NamingUri const &applicationName,
            Common::NamingUri const &serviceGroupNameFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetServiceGroupMemberList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::ServiceGroupMemberQueryResult> &serviceGroupMemberList) = 0;

        virtual Common::AsyncOperationSPtr BeginGetPartitionList(
            Common::NamingUri const &serviceName,
            Common::Guid const &partitionIdFilter,
            std::wstring const &continuationToken,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetPartitionList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::ServicePartitionQueryResult> &partitionList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus) = 0;

       virtual Common::AsyncOperationSPtr BeginGetReplicaList(
            Common::Guid const &partitionId,
            int64 replicaOrInstanceIdFilter,
            DWORD replicaStatusFilter,
            std::wstring const &continuationToken,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetReplicaList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::ServiceReplicaQueryResult> &replicaList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus) = 0;

        virtual Common::AsyncOperationSPtr BeginGetDeployedApplicationList(
            std::wstring const &nodeName,
            Common::NamingUri const &applicationNameFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetDeployedApplicationList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::DeployedApplicationQueryResult> &deployedApplicationList) = 0;

        virtual Common::AsyncOperationSPtr BeginGetDeployedApplicationPagedList(
            ServiceModel::DeployedApplicationQueryDescription const & deployedApplicationQueryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetDeployedApplicationPagedList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::DeployedApplicationQueryResult> &deployedApplicationList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus) = 0;

        virtual Common::AsyncOperationSPtr BeginGetDeployedServicePackageList(
            std::wstring const &nodeName,
            Common::NamingUri const &applicationName,
            std::wstring const &serviceManifestNameFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetDeployedServicePackageList(
            Common::AsyncOperationSPtr const &operation,
            __out std::vector<ServiceModel::DeployedServiceManifestQueryResult> &deployedServiceManifestList) = 0;

        virtual Common::AsyncOperationSPtr BeginGetDeployedServiceTypeList(
            std::wstring const &nodeName,
            Common::NamingUri const &applicationName,
            std::wstring const &serviceManifestNameFilter,
            std::wstring const &serviceTypeNameFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetDeployedServiceTypeList(
            Common::AsyncOperationSPtr const &operation,
            __out std::vector<ServiceModel::DeployedServiceTypeQueryResult> &deployedServiceTypeList) = 0;

       virtual Common::AsyncOperationSPtr BeginGetDeployedCodePackageList(
            std::wstring const &nodeName,
            Common::NamingUri const &applicationName,
            std::wstring const &serviceManifestNameFilter,
            std::wstring const &codePackageNameFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetDeployedCodePackageList(
            Common::AsyncOperationSPtr const &operation,
            __out std::vector<ServiceModel::DeployedCodePackageQueryResult> &deployedCodePackageList) = 0;

       virtual Common::AsyncOperationSPtr BeginGetDeployedReplicaList(
            std::wstring const &nodeName,
            Common::NamingUri const &applicationName,
            std::wstring const &serviceManifestNameFilter,
            Common::Guid const &partitionIdFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetDeployedReplicaList(
            Common::AsyncOperationSPtr const &operation,
            __out std::vector<ServiceModel::DeployedServiceReplicaQueryResult> &deployedReplicaList) = 0;

        virtual Common::AsyncOperationSPtr BeginGetDeployedServiceReplicaDetail(
            std::wstring const & nodeName,
            Common::Guid const & partitionId,
            FABRIC_REPLICA_ID replicaId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::ErrorCode EndGetDeployedServiceReplicaDetail(
            Common::AsyncOperationSPtr const &operation,
            __out ServiceModel::DeployedServiceReplicaDetailQueryResult &queryResult) = 0;

        virtual Common::AsyncOperationSPtr BeginGetClusterLoadInformation(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetClusterLoadInformation(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::ClusterLoadInformationQueryResult & clusterLoadInformationQueryResult) = 0;

        virtual Common::AsyncOperationSPtr BeginGetPartitionLoadInformation(
            Common::Guid const &partitionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetPartitionLoadInformation(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::PartitionLoadInformationQueryResult & partitionLoadInformationQueryResult) = 0;

        virtual Common::AsyncOperationSPtr BeginGetProvisionedFabricCodeVersionList(
            std::wstring const & codeVersionFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetProvisionedFabricCodeVersionList(
            Common::AsyncOperationSPtr const &operation,
            __out std::vector<ServiceModel::ProvisionedFabricCodeVersionQueryResultItem> &codeVersionList) = 0;

        virtual Common::AsyncOperationSPtr BeginGetProvisionedFabricConfigVersionList(
            std::wstring const & configVersionFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetProvisionedFabricConfigVersionList(
            Common::AsyncOperationSPtr const &operation,
            __out std::vector<ServiceModel::ProvisionedFabricConfigVersionQueryResultItem> &configVersionList) = 0;

        virtual Common::AsyncOperationSPtr BeginGetNodeLoadInformation(
            std::wstring const &nodeName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetNodeLoadInformation(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::NodeLoadInformationQueryResult & nodeLoadInformationQueryResult) = 0;

        virtual Common::AsyncOperationSPtr BeginGetReplicaLoadInformation(
            Common::Guid const &partitionId,
            int64 replicaOrInstanceId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetReplicaLoadInformation(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::ReplicaLoadInformationQueryResult & replicaLoadInformationQueryResult) = 0;

        virtual Common::AsyncOperationSPtr BeginGetUnplacedReplicaInformation(
            std::wstring const &serviceName,
            Common::Guid const &partitionId,
            bool onlyQueryPrimary,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetUnplacedReplicaInformation(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::UnplacedReplicaInformationQueryResult & unplacedReplicaInformationQueryResult) = 0;

        virtual Common::AsyncOperationSPtr BeginGetApplicationLoadInformation(
            std::wstring const &applicationName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetApplicationLoadInformation(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::ApplicationLoadInformationQueryResult & applicationLoadInformationQueryResult) = 0;

        virtual Common::AsyncOperationSPtr BeginGetServiceName(
            Common::Guid const &partitionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetServiceName(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::ServiceNameQueryResult & serviceNameQueryResult) = 0;

        virtual Common::AsyncOperationSPtr BeginGetApplicationName(
            Common::NamingUri const &serviceName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetApplicationName(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::ApplicationNameQueryResult & applicationNameQueryResult) = 0;
    };
}

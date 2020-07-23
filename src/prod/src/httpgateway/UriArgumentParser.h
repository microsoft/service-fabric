// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class UriArgumentParser
    {
        DENY_COPY(UriArgumentParser);

    public:
        UriArgumentParser(GatewayUri const & uri);

        Common::ErrorCode TryGetRequiredPartitionId(__inout Common::Guid & partitionId);
        Common::ErrorCode TryGetOptionalPartitionId(__inout Common::Guid & partitionId);

        Common::ErrorCode TryGetOperationId(__out Common::Guid & operationId);

        Common::ErrorCode TryGetServicePackageActivationId(__out std::wstring & servicePackageActivationId);

        Common::ErrorCode TryGetRequiredReplicaId(__inout int64 & replicaId);

        Common::ErrorCode TryGetReplicaId(__inout int64 & replicaId);

        Common::ErrorCode TryGetNodeName(__out std::wstring & nodeName);

        Common::ErrorCode TryGetNodeInstanceId(__out uint64 & nodeInstanceId);

        Common::ErrorCode TryGetNodeStatusFilter(
            __out DWORD& nodeStatus);

        Common::ErrorCode TryGetCodePackageName(__out std::wstring & codePackageName);

        Common::ErrorCode TryGetContinuationToken(__out std::wstring & continuationToken);

        Common::ErrorCode TryGetMaxResults(
            __out int64 &maxResults);

        Common::ErrorCode TryGetPagingDescription(
            __out ServiceModel::QueryPagingDescription & pagingDescription);

        Common::ErrorCode TryGetApplicationName(
            __out Common::NamingUri &applicationName);

        Common::ErrorCode TryGetDeploymentName(
            __out std::wstring &deploymentName);

        Common::ErrorCode TryGetContainerName(
            __out std::wstring &containerName);

        Common::ErrorCode TryGetApplicationNameInQueryParam(
            __out Common::NamingUri &applicationName);

        Common::ErrorCode TryGetServiceName(
            __out Common::NamingUri &serviceName);

        Common::ErrorCode TryGetServiceTypeName(
            __out std::wstring & serviceTypeName);

        Common::ErrorCode TryGetServiceManifestName(
            __out std::wstring & serviceManifestName);

        Common::ErrorCode TryGetMode(
            __out std::wstring &marker);

        Common::ErrorCode TryGetReplicaStatusFilter(
            __out DWORD& replicaStatus);

        Common::ErrorCode TryGetPartitionKey(
            __out Naming::PartitionKey& partitionKey);

        Common::ErrorCode TryGetForceRemove(
            __inout bool& forceRemove);

        Common::ErrorCode TryGetImmediateOption(
            __inout bool& immediate);

        Common::ErrorCode TryGetCreateFabricDump(
            __inout bool& createFabricDump);

        Common::ErrorCode TryGetForce(
            __inout bool& force);

        Common::ErrorCode TryGetExcludeApplicationParameters(
            __out bool & excludeApplicationParameters) const;

        Common::ErrorCode GetExcludeApplicationParameters(
            __out bool & excludeApplicationParameters) const;

        Common::ErrorCode TryGetIncludeHealthState(
            __out bool & includeHealthState) const;

        Common::ErrorCode GetIncludeHealthState(
            __out bool & includeHealthState) const;

        Common::ErrorCode TryGetRecursive(
            __out bool & recursive) const;

        Common::ErrorCode TryGetIncludeValues(
            __out bool & includeValues) const;

        Common::ErrorCode TryGetServiceKind(
            bool isRequired,
            __out FABRIC_SERVICE_KIND & serviceKind) const;

        Common::ErrorCode TryGetApplicationDefinitionKindFilter(
            __out DWORD & applicationDefinitionKindFilter) const;

        Common::ErrorCode TryGetApplicationTypeDefinitionKindFilter(
            __out DWORD & definitionKindFilter) const;

        Common::ErrorCode TryGetApplicationTypeVersion(
            __out std::wstring &appTypeVersion) const;

        Common::ErrorCode TryGetApplicationTypeName(
            __out std::wstring & applicationTypeName) const;

        Common::ErrorCode TryGetClusterManifestVersion(
            __out std::wstring &clusterManifestVersion) const;

        Common::ErrorCode TryGetClusterConfigurationApiVersion(
            __out std::wstring &apiVersion) const;

        Common::ErrorCode TryGetImageStoreRelativePath(
            __out std::wstring & relativePath);

        Common::ErrorCode TryGetInstanceId(
            __out std::wstring & instanceId) const;

        Common::ErrorCode TryGetTestCommandProgressType(
            __out DWORD& );

        Common::ErrorCode TryGetStartTimeUtc(__out Common::DateTime & startTimeUtc) const;

        Common::ErrorCode TryGetEndTimeUtc(__out Common::DateTime & endTimeUtc) const;

        Common::ErrorCode TryGetUploadSessionId(
            __out Common::Guid & sessionId);

        Common::ErrorCode TryGetDataLossMode(
             __out Management::FaultAnalysisService::DataLossMode::Enum & mode);

        Common::ErrorCode TryGetQuorumLossMode(
            __out Management::FaultAnalysisService::QuorumLossMode::Enum& );

        Common::ErrorCode TryGetRestartPartitionMode(
            __out Management::FaultAnalysisService::RestartPartitionMode::Enum& );

        Common::ErrorCode TryGetQuorumLossDuration(
            __out DWORD& );

        Common::ErrorCode TryGetNodeTransitionType(
            __out FABRIC_NODE_TRANSITION_TYPE &marker);

        Common::ErrorCode TryGetStopDurationInSeconds(
            __out DWORD& duration);

        Common::ErrorCode TryGetExcludeStatisticsFilter(__inout bool & filter);

        Common::ErrorCode TryGetIncludeSystemApplicationHealthStatisticsFilter(__inout bool & filter);

        Common::ErrorCode TryGetName(
            __out Common::NamingUri & name);

        Common::ErrorCode TryGetPropertyName(
            __out std::wstring & propertyName);

        Common::ErrorCode TryGetVolumeName(
            __out std::wstring & volumeName);

        Common::ErrorCode TryGetGatewayName(
            __out std::wstring & gatewayName);

        Common::ErrorCode TryGetAbsoluteServiceName(
            __out Common::NamingUri &name);

        Common::ErrorCode TryGetNetworkName(
            __out std::wstring & networkName);

        Common::ErrorCode TryGetNetworkStatusFilter(
            __out DWORD & networkStatusFilter);

    private:
        Common::ErrorCode TryGetPathId(
            std::wstring const & paramName,
            __out NamingUri & name) const;

        Common::ErrorCode TryGetPartitionId(
            bool isRequired,
            __inout Common::Guid & partitionId);

        Common::ErrorCode TryGetOptionalBool(
            std::wstring const & paramName,
            __out bool & value) const;

        Common::ErrorCode TryGetBoolOrError(
            std::wstring const & paramName,
            __inout bool & value) const;

        Common::ErrorCode TryGetRepositoryUserName(
            __out std::wstring & repoUserName) const;

        Common::ErrorCode TryGetRepositoryPassword(
            __out std::wstring & repoPassword) const;

        Common::ErrorCode TryGetPasswordEncrypted(
            __out bool & passwordEncrypted);

    private:
        GatewayUri const & uri_;
    };
}
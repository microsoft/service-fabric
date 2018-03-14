// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    typedef map<wstring, wstring> VerifyResultRow;
    typedef map<wstring, VerifyResultRow> VerifyResultTable;

    class FabricTestSession;

    class TestFabricClientQuery
    {
        DENY_COPY(TestFabricClientQuery);

    public:
        TestFabricClientQuery(){};
        virtual ~TestFabricClientQuery() {}

        static bool Query(Common::StringCollection const & params);
        static bool Query(Common::StringCollection const & params, Common::ComPointer<IFabricQueryClient> const & client);
        static ComPointer<IFabricQueryClient> CreateFabricQueryClient(__in FabricTestFederation & testFederation);
        static Api::IQueryClientPtr CreateInternalFabricQueryClient(__in FabricTestFederation & testFederation);
        static VerifyResultTable ProcessQueryResult(FABRIC_QUERY_RESULT const * queryResult, FABRIC_PAGING_STATUS const * pagingStatus);
        static VerifyResultTable ProcessGetComposeDeploymentUpgradeProgressResult(FABRIC_QUERY_RESULT const * queryResult);
        static VerifyResultTable ProcessGetDeployedServiceReplicaDetailQueryResult(FABRIC_QUERY_RESULT const * queryResult);

    private:
        static bool VerifyQueryResult(std::wstring const & queryName, VerifyResultTable const & expectQueryResult, VerifyResultTable const & actualQueryResult, bool verifyFromFM);

        static bool ProcessQueryArg(wstring const & queryName, wstring const & param, vector<FABRIC_STRING_PAIR> & queryArgsVector, ScopedHeap & heap, HRESULT &expectedError, bool & expectEmpty, bool & requiresEmptyValidation, bool & ignoreResultVerification);
        static bool ProcessVerifyArg(wstring const & param, VerifyResultTable & expectQueryResult);
        static bool ProcessVerifyFromFMArg(wstring const &queryName, vector<FABRIC_STRING_PAIR> & queryArgsVector, VerifyResultTable & expectQueryResult);

        static void UpdateParamTokensForGetNodesQuery(StringCollection & paramTokens);

        static VerifyResultTable ProcessQueryResultQueryMetadata(FABRIC_QUERY_METADATA_QUERY_RESULT_ITEM const * queryMetadataResult, ULONG resultCount);
        static VerifyResultTable ProcessQueryResultNode(FABRIC_NODE_QUERY_RESULT_ITEM const * nodeResult, ULONG resultCount);
        static VerifyResultTable ProcessQueryResultApplicationType(FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM const * applicationTypeResult, ULONG resultCount);
        static VerifyResultTable ProcessQueryResultApplication(FABRIC_APPLICATION_QUERY_RESULT_ITEM const * applicationResult, ULONG resultCount);
        static VerifyResultTable ProcessQueryResultComposeDeployment(FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_ITEM const * composeDeploymentResult, ULONG resultCount);
        static VerifyResultTable ProcessQueryResultDeployedApplication(FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM const * applicationResult, ULONG resultCount);
        static VerifyResultTable ProcessQueryResultDeployedServiceManifest(FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_ITEM const * serviceManifestResult, ULONG resultCount);
        static VerifyResultTable ProcessQueryResultDeployedServiceType(FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_RESULT_ITEM const * serviceTypeResult, ULONG resultCount);
        static VerifyResultTable ProcessQueryResultDeployedCodePackage(FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_ITEM const * codePackageResult, ULONG resultCount);
        static VerifyResultTable ProcessQueryResultDeployedServiceReplica(FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_ITEM const * serviceReplicaResult, ULONG resultCount);
        static VerifyResultTable ProcessQueryResultService(FABRIC_SERVICE_QUERY_RESULT_ITEM const * serviceResult, ULONG resultCount);
        static VerifyResultTable ProcessQueryResultStatefulPartition(FABRIC_SERVICE_PARTITION_QUERY_RESULT_ITEM const * partitionResult, ULONG resultCount);
        static VerifyResultTable ProcessQueryResultStatelessPartition(FABRIC_SERVICE_PARTITION_QUERY_RESULT_ITEM const * partitionResult, ULONG resultCount);
        static VerifyResultTable ProcessQueryResultStatefulReplica(FABRIC_SERVICE_REPLICA_QUERY_RESULT_ITEM const * replicaResult, ULONG resultCount);
        static VerifyResultTable ProcessQueryResultStatelessInstance(FABRIC_SERVICE_REPLICA_QUERY_RESULT_ITEM const * replicaResult, ULONG resultCount);
        static VerifyResultTable ProcessQueryResultServiceType(FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM const * serviceTypeResult, ULONG resultCount);

        static std::wstring GetServiceStatusString(FABRIC_QUERY_SERVICE_STATUS);
        static std::wstring GetComposeDeploymentUpgradeState(FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE const);
        static ServiceModel::ServicePartitionInformation ToServicePartitionInformation(FABRIC_SERVICE_PARTITION_INFORMATION const & servicePartionInformation, VerifyResultRow & actualResultRow);
        static Reliability::ReplicaRole::Enum ToReplicaRole(FABRIC_REPLICA_ROLE replicaRole);
        static Reliability::ReplicaStatus::Enum ToReplicaStatus(FABRIC_REPLICA_STATUS replicaStatus);

        static std::wstring const NodeName;
        static std::wstring const NodeId;
        static std::wstring const NodeInstanceId;
        static std::wstring const IpAddressOrFQFN;
        static std::wstring const NodeType;
        static std::wstring const CodeVersion;
        static std::wstring const ConfigVersion;
        static std::wstring const NodeStatus;
        static std::wstring const IsSeedNode;
        static std::wstring const NextUpgradeDomain;
        static std::wstring const UpgradeDomain;
        static std::wstring const FaultDomain;
        static std::wstring const DeploymentName;
        static std::wstring const ApplicationDefinitionKind;
        static std::wstring const ApplicationTypeName;
        static std::wstring const ApplicationTypeVersion;
        static std::wstring const TargetApplicationTypeVersion;
        static std::wstring const ApplicationName;
        static std::wstring const ApplicationStatus;
        static std::wstring const ApplicationParameters;
        static std::wstring const DefaultParameters;
        static std::wstring const ApplicationTypeDefinitionKind;
        static std::wstring const GetApplicationTypeList;
        static std::wstring const GetApplicationTypePagedList;
        static std::wstring const Status;
        static std::wstring const StatusDetails;
        static std::wstring const Unprovisioning;
        static std::wstring const ServiceManifestName;
        static std::wstring const ServiceManifestVersion;
        static std::wstring const ServiceName;
        static std::wstring const ServiceStatus;
        static std::wstring const ServiceType;
        static std::wstring const ServiceTypeName;
        static std::wstring const CodePackageName;
        static std::wstring const CodePackageVersion;
        static std::wstring const HasPersistedState;
        static std::wstring const PartitionId;
        static std::wstring const PartitionKind;
        static std::wstring const PartitionName;
        static std::wstring const PartitionLowKey;
        static std::wstring const PartitionHighKey;
        static std::wstring const TargetReplicaSetSize;
        static std::wstring const MinReplicaSetSize;
        static std::wstring const PartitionStatus;
        static std::wstring const InstanceCount;
        static std::wstring const ReplicaId;
        static std::wstring const ReplicaRole;
        static std::wstring const ReplicaStatus;
        static std::wstring const ReplicaState;
        static std::wstring const ReplicaAddress;
        static std::wstring const InstanceId;
        static std::wstring const InstanceStatus;
        static std::wstring const InstanceState;
        static std::wstring const UseImplicitHost;
        static std::wstring const DeploymentStatus;
        static std::wstring const EntryPointStatus;
        static std::wstring const RunFrequencyInterval;
        static std::wstring const HealthState;
        static std::wstring const ContinuationToken;
        static std::wstring const ActivationId;

        static std::wstring const SkipValidationToken;

        static std::wstring const NodeStatusFilterDefaultString;
        static std::wstring const NodeStatusFilterAllString;
        static std::wstring const NodeStatusFilterUpString;
        static std::wstring const NodeStatusFilterDownString;
        static std::wstring const NodeStatusFilterEnablingString;
        static std::wstring const NodeStatusFilterDisablingString;
        static std::wstring const NodeStatusFilterDisabledString;
        static std::wstring const NodeStatusFilterUnknownString;
        static std::wstring const NodeStatusFilterRemovedString;
        static std::wstring const NodeStatusFilterDefault;
        static std::wstring const NodeStatusFilterAll;
        static std::wstring const NodeStatusFilterUp;
        static std::wstring const NodeStatusFilterDown;
        static std::wstring const NodeStatusFilterEnabling;
        static std::wstring const NodeStatusFilterDisabling;
        static std::wstring const NodeStatusFilterDisabled;
        static std::wstring const NodeStatusFilterUnknown;
        static std::wstring const NodeStatusFilterRemoved;
    };
}

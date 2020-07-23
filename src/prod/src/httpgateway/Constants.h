// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class Constants
    {
    public:
        static double DefaultFabricTimeoutMin;
        static double DefaultAccessCheckTimeoutMin;
        static ULONG MaximumActiveListeners;
        static ULONG DefaultEntityBodyChunkSize;
        static ULONG DefaultEntityBodyForUploadChunkSize;
        static ULONG MaxEntityBodyForUploadChunkSize;
        static ULONG DefaultHeaderBufferSize;
        static Common::GlobalWString HttpGatewayTraceId;
        static double V1ApiVersion;
        static double V2ApiVersion;
        static double V3ApiVersion;
        static double V4ApiVersion;
        static double V5ApiVersion;
        static double V6ApiVersion;
        static double V61ApiVersion;
        static double V62ApiVersion;
        static double V63ApiVersion;
        static double V64ApiVersion;
        static Common::GlobalWString PreviewApiVersion;

        static ULONG AllocationTag;
        static Common::GlobalWString ApplicationsHandlerPath;
        static Common::GlobalWString ApplicationTypesHandlerPath;
        static Common::GlobalWString ClusterManagementHandlerPath;
        static Common::GlobalWString ComposeDeploymentsHandlerPath;
        static Common::GlobalWString VolumesHandlerPath;
        static Common::GlobalWString NodesHandlerPath;
        static Common::GlobalWString ServicesHandlerPath;
        static Common::GlobalWString PartitionsHandlerPath;
        static Common::GlobalWString ImageStoreHandlerPath;
        static Common::GlobalWString TestCommandsHandlerPath;
        static Common::GlobalWString FaultsHandlerPath;
        static Common::GlobalWString NamesHandlerPath;
        static Common::GlobalWString BackupRestoreHandlerPath;
        static Common::GlobalWString NetworksHandlerPath;
        static Common::GlobalWString ContentTypeHeader;
        static Common::GlobalWString ContentLengthHeader;
        static Common::GlobalWString ContentRangeHeader;
        static Common::GlobalWString ContentRangeUnit;
        static Common::GlobalWString WWWAuthenticateHeader;
        static Common::GlobalWString TransferEncodingHeader;
        static Common::GlobalWString LocationHeader;
        static Common::GlobalWString AuthorizationHeader;
        static Common::GlobalWString Bearer;
        static Common::GlobalWString SecretsResourceHandlerPath;
        static Common::GlobalWString ClusterIdHeader;
        static Common::GlobalWString ServiceFabricHttpClientRequestIdHeader;
        static Common::GlobalWString ServerRedirectResponse;
        static Common::GlobalWString ContentTypeOptionsHeader;
        static Common::GlobalWString ContentTypeNoSniffValue;
        static Common::GlobalWString HSTSHeader;
        static Common::GlobalWString ApplicationsResourceHandlerPath;
        static Common::GlobalWString GatewaysHandlerPath;

        static Common::GlobalWString JsonContentType;
        static Common::GlobalWString HtmlContentType;
        static Common::GlobalWString CssContentType;
        static Common::GlobalWString JavaScriptContentType;
        static Common::GlobalWString PngContentType;
        static Common::GlobalWString IcoContentType;
        static Common::GlobalWString EotContentType;
        static Common::GlobalWString SvgContentType;
        static Common::GlobalWString TtfContentType;
        static Common::GlobalWString WoffContentType;
        static Common::GlobalWString Woff2ContentType;
        static Common::GlobalWString MapContentType;

        static Common::GlobalWString ServiceKindString;
        static Common::GlobalWString ApplicationDefinitionKindFilterString;
        static Common::GlobalWString ApplicationIdString;
        static Common::GlobalWString DeploymentNameString;
        static Common::GlobalWString ContainerNameString;
        static Common::GlobalWString ApplicationNameString;
        static Common::GlobalWString ServiceIdString;
        static Common::GlobalWString PartitionIdString;
        static Common::GlobalWString ServicePackageActivationIdString;
        static Common::GlobalWString ReplicaIdString;
        static Common::GlobalWString ReplicaStatusFilterString;
        static Common::GlobalWString NodeStatusFilterString;
        static Common::GlobalWString MaxResultsString;
        static Common::GlobalWString ExcludeApplicationParametersString;
        static Common::GlobalWString IncludeHealthStateString;
        static Common::GlobalWString NodeIdString;
        static Common::GlobalWString NodeInstanceIdString;
        static Common::GlobalWString OperationIdString;
        static Common::GlobalWString StartTimeUtcString;
        static Common::GlobalWString EndTimeUtcString;
        static Common::GlobalWString ApplicationTypeDefinitionKindFilterString;
        static Common::GlobalWString ApplicationTypeNameString;
        static Common::GlobalWString ServiceManifestNameIdString;
        static Common::GlobalWString CodePackageNameIdString;
        static Common::GlobalWString CodePackageInstanceIdString;
        static Common::GlobalWString ServiceTypeNameIdString;
        static Common::GlobalWString ServiceTypeNameString;
        static Common::GlobalWString CommandString;
        static Common::GlobalWString SessionIdString;
        static Common::GlobalWString ApiVersionString;
        static Common::GlobalWString TimeoutString;
        static Common::GlobalWString ApplicationTypeVersionString;
        static Common::GlobalWString ClusterManifestVersionString;
        static Common::GlobalWString ConfigurationApiVersionString;
        static Common::GlobalWString ServiceManifestNameString;
        static Common::GlobalWString UpgradeDomainNameString;
        static Common::GlobalWString ContinuationTokenString;
        static Common::GlobalWString NativeImageStoreRelativePathString;
        static Common::GlobalWString TestCommandsString;
        static Common::GlobalWString StateFilterString;
        static Common::GlobalWString TypeFilterString;
        static Common::GlobalWString PartitionSelectorTypeString;
        static Common::GlobalWString ModeString;
        static Common::GlobalWString NameString;
        static Common::GlobalWString RecursiveString;
        static Common::GlobalWString IncludeValuesString;
        static Common::GlobalWString PropertyNameString;
        static Common::GlobalWString InstanceIdString;
        static Common::GlobalWString VolumeNameString;
        static Common::GlobalWString SecretNameString;
        static Common::GlobalWString SecretVersionString;
        static Common::GlobalWString IncludeValueString;
        static Common::GlobalWString NetworkNameString;
        static Common::GlobalWString NetworkStatusFilterString;
        static Common::GlobalWString GatewayNameString;

        static Common::GlobalWString CommandType;

        static Common::GlobalWString SegmentDelimiter;
        static Common::GlobalWString MetadataSegment;
        static Common::GlobalWString QueryStringDelimiter;

        static Common::GlobalWString Services;
        static Common::GlobalWString OperationType;
        static Common::GlobalWString Mode;
        static Common::GlobalWString DataLossMode;
        static Common::GlobalWString QuorumLossMode;
        static Common::GlobalWString RestartPartitionMode;

        static Common::GlobalWString QuorumLossDuration;

        //
        // EntitySet/EntityKey path suffixes.
        //
        static Common::GlobalWString ApplicationsEntitySetPath;
        static Common::GlobalWString ApplicationsEntityKeyPath;
        static Common::GlobalWString ComposeDeploymentsEntitySetPath;
        static Common::GlobalWString ComposeDeploymentsEntityKeyPath;
        static Common::GlobalWString ContainerEntityKeyPath;
        static Common::GlobalWString ApplicationTypesEntitySetPath;
        static Common::GlobalWString TestCommandProgressPath;
        static Common::GlobalWString VolumesEntitySetPath;
        static Common::GlobalWString VolumesEntityKeyPath;
        static Common::GlobalWString ApplicationsResourceEntitySetPath;
        static Common::GlobalWString ApplicationsResourceEntityKeyPath;
        static Common::GlobalWString ServicesResourceEntitySetPath;
        static Common::GlobalWString ServicesResourceEntityKeyPath;
        static Common::GlobalWString ReplicasResourceEntitySetPath;
        static Common::GlobalWString ReplicasResourceEntityKeyPath;
        static Common::GlobalWString ContainerCodePackageKeyPath;
        static Common::GlobalWString ContainerCodePackageLogsKeyPath;
        static Common::GlobalWString GatewaysResourceEntitySetPath;
        static Common::GlobalWString GatewaysResourceEntityKeyPath;

        // path suffixes for backup restore
        static Common::GlobalWString BackupRestoreServiceName;
        static Common::GlobalWString BackupRestorePath;
        static Common::GlobalWString BackupRestorePrefix;

        static Common::GlobalWString EnableBackup;
        static Common::GlobalWString DisableBackup;
        static Common::GlobalWString GetBackupConfigurationInfo;
        static Common::GlobalWString GetBackups;
        static Common::GlobalWString SuspendBackup;
        static Common::GlobalWString ResumeBackup;
        static Common::GlobalWString Backup;
        static Common::GlobalWString GetBackupProgress;
        static Common::GlobalWString Restore;
        static Common::GlobalWString GetRestoreProgress;

        static Common::GlobalWString RequestMetadataHeaderName;
        static Common::GlobalWString ClientRoleHeaderName;

        static Common::GlobalWString ApplicationTypesEntityKeyPath;
        static Common::GlobalWString ServiceTypesEntitySetPath;
        static Common::GlobalWString ServiceTypesEntityKeyPath;
        static Common::GlobalWString ServicesEntitySetPathViaApplication;
        static Common::GlobalWString ServiceGroupMembersEntitySetPathViaApplication;
        static Common::GlobalWString ServiceGroupsEntitySetPathViaApplication;
        static Common::GlobalWString ServicesEntityKeyPathViaApplication;
        static Common::GlobalWString ServiceGroupsEntityKeyPathViaApplication;
        static Common::GlobalWString ServicesEntitySetPath;
        static Common::GlobalWString ServiceGroupsEntitySetPath;
        static Common::GlobalWString ServicesEntityKeyPath;
        static Common::GlobalWString ServiceGroupsEntityKeyPath;
        static Common::GlobalWString SystemServicesEntitySetPath;
        static Common::GlobalWString SystemServicesEntityKeyPath;
        static Common::GlobalWString ServicePartitionEntitySetPathViaApplication;
        static Common::GlobalWString ServicePartitionEntityKeyPathViaApplication;
        static Common::GlobalWString ServicePartitionEntitySetPath;
        static Common::GlobalWString ServicePartitionEntityKeyPath;
        static Common::GlobalWString ServicePartitionEntitySetPathViaService;
        static Common::GlobalWString ServicePartitionEntityKeyPathViaService;
        static Common::GlobalWString SystemServicePartitionEntitySetPath;
        static Common::GlobalWString ServiceReplicaEntitySetPathViaApplication;
        static Common::GlobalWString ServiceReplicaEntityKeyPathViaApplication;
        static Common::GlobalWString ServiceReplicaEntitySetPathViaPartition;
        static Common::GlobalWString ServiceReplicaEntityKeyPathViaPartition;
        static Common::GlobalWString ServiceReplicaEntitySetPathViaService;
        static Common::GlobalWString ServiceReplicaEntityKeyPathViaService;
        static Common::GlobalWString SecretsResourceEntitySetPath;
        static Common::GlobalWString SecretsResourceEntityKeyPath;
        static Common::GlobalWString SecretsResourceVersionEntitySetPath;
        static Common::GlobalWString SecretsResourceVersionEntityKeyPath;
        static Common::GlobalWString SecretsResourceVersionEntityKeyValuePath;
        static Common::GlobalWString SecretsResourceValueListAction;
        static Common::GlobalWString SystemServiceReplicaEntitySetPath;
        static Common::GlobalWString SystemEntitySetPath;
        static Common::GlobalWString NodesEntitySetPath;
        static Common::GlobalWString NodesEntityKeyPath;

        static Common::GlobalWString NetworksEntitySetPath;
        static Common::GlobalWString NetworksEntityKeyPath;
        static Common::GlobalWString ApplicationsEntitySetPathViaNetwork;
        static Common::GlobalWString ApplicationsEntityKeyPathViaNetwork;
        static Common::GlobalWString NodesEntitySetPathViaNetwork;
        static Common::GlobalWString NodesEntityKeyPathViaNetwork;
        static Common::GlobalWString NetworksEntitySetPathViaApplication;
        static Common::GlobalWString NetworksOnNodeEntitySetPath;
        static Common::GlobalWString NetworksOnNodeEntityKeyPath;
        static Common::GlobalWString CodePackagesEntitySetPathViaNetworkViaNode;
        static Common::GlobalWString CodePackagesEntityKeyPathViaNetworkViaNode;

        static Common::GlobalWString TestCommandsSetPath;
        static Common::GlobalWString TestCommandsEntityKeyPath;

        static Common::GlobalWString ToolsEntitySetPath;
        static Common::GlobalWString ToolsEntityKeyPath;
        static Common::GlobalWString ToolsHandlerPath;

        static Common::GlobalWString ChaosEntityKeyPath;
        static Common::GlobalWString ChaosEventSegmentsSetPath;
        static Common::GlobalWString ChaosScheduleKeyPath;

        static Common::GlobalWString FaultsEntitySetPath;

        static Common::GlobalWString NamesEntitySetPath;
        static Common::GlobalWString NamesEntityKeyPath;
        static Common::GlobalWString PropertyEntityKeyPathViaName;
        static Common::GlobalWString PropertiesEntityKeyPathViaName;

        // Start fault-related constants
        static Common::GlobalWString StartPartitionFaultPrefix;
        static Common::GlobalWString StartDataLoss;
        static Common::GlobalWString StartQuorumLoss;
        static Common::GlobalWString StartRestart;

        // Get fault progress-related constants
        static Common::GlobalWString GetDataLossProgress;
        static Common::GlobalWString GetQuorumLossProgress;
        static Common::GlobalWString GetRestartProgress;

        static Common::GlobalWString FaultServicesEntityKeyPath;

        static Common::GlobalWString FaultServicePartitionEntitySetPathViaService;

        static Common::GlobalWString FaultServicePartitionEntityKeyPathViaService;

        static Common::GlobalWString StartNodeFaultPrefix;

        static Common::GlobalWString StartNodeTransition;
        static Common::GlobalWString StartNodeTransitionProgress;

        static Common::GlobalWString ApplicationsOnNodeEntitySetPath;
        static Common::GlobalWString ApplicationsOnNodeEntityKeyPath;
        static Common::GlobalWString ServicePackagesOnNodeEntitySetPath;
        static Common::GlobalWString ServicePackagesOnNodeEntityKeyPath;
        static Common::GlobalWString CodePackagesOnNodeEntitySetPath;
        static Common::GlobalWString CodePackagesOnNodeEntityKeyPath;
        static Common::GlobalWString ServiceReplicasOnNodeEntitySetPath;
        static Common::GlobalWString ServiceTypesOnNodeEntitySetPath;
        static Common::GlobalWString ServiceTypesOnNodeEntityKeyPath;
        static Common::GlobalWString ApplicationServiceTypesEntitySetPath;
        static Common::GlobalWString ApplicationServiceTypesEntityKeyPath;
        static Common::GlobalWString ImageStoreRootPath;
        static Common::GlobalWString ImageStoreRelativePath;

        // /Nodes/{NodeId}/$/Partitions/{PartitionId}
        static Common::GlobalWString ServicePartitionEntitySetPathViaNode;
        static Common::GlobalWString ServicePartitionEntityKeyPathViaNode;

        // /Nodes/{NodeId}/$/Partitions/{PartitionId}/$/Replicas/{ReplicaId}
        static Common::GlobalWString ServiceReplicaEntitySetPathViaPartitionViaNode;
        static Common::GlobalWString ServiceReplicaEntityKeyPathViaPartitionViaNode;

        static Common::GlobalWString GetHealth;
        static Common::GlobalWString ReportHealth;
        static Common::GlobalWString StartPartitionRestart;
        static Common::GlobalWString StartPartitionQuorumLoss;
        static Common::GlobalWString StartPartitionDataLoss;
        static Common::GlobalWString GetPartitionRestartProgress;
        static Common::GlobalWString GetTestCommands;
        static Common::GlobalWString Cancel;
        static Common::GlobalWString Report;
        static Common::GlobalWString Instances;
        static Common::GlobalWString Schedule;

        static Common::GlobalWString StartClusterConfigurationUpgrade;
        static Common::GlobalWString GetClusterConfigurationUpgradeStatus;
        static Common::GlobalWString GetClusterConfiguration;
        static Common::GlobalWString GetUpgradesPendingApproval;
        static Common::GlobalWString StartApprovedUpgrades;
        static Common::GlobalWString GetUpgradeOrchestrationServiceState;
        static Common::GlobalWString SetUpgradeOrchestrationServiceState;

        static Common::GlobalWString GetClusterHealth;
        static Common::GlobalWString GetClusterHealthChunk;
        static Common::GlobalWString ReportClusterHealth;
        static Common::GlobalWString GetdSTSMetadata;
        static Common::GlobalWString GetAadMetadata;
        static Common::GlobalWString GetLoadInformation;
        static Common::GlobalWString GetUnplacedReplicaInformation;
        static Common::GlobalWString InvokeInfrastructureCommand;
        static Common::GlobalWString InvokeInfrastructureQuery;

        // Service Fabric Explorer names
        static Common::GlobalWString HtmlRootPath;
        static Common::GlobalWString ExplorerPath;
        static Common::GlobalWString ExplorerRootPath;
        static Common::GlobalWString ExplorerRedirectPath;
        static Common::GlobalWString ExplorerRedirectPath2;
        static Common::GlobalWString StaticContentType;
        static Common::GlobalWString StaticContentPathArg;
        static Common::GlobalWString StaticContentFileNameType;
        static Common::GlobalWString DirectorySeparator;
        static Common::GlobalWString StaticFilesRootPath;
        static Common::GlobalWString ExplorerFilesRootPath;
        static Common::GlobalWString RootPageName;
        static Common::GlobalWString PathTraversalDisallowedString;
        static Common::GlobalWString HtmlExtension;
        static Common::GlobalWString CssExtension;
        static Common::GlobalWString JavaScriptExtension;
        static Common::GlobalWString PngExtension;
        static Common::GlobalWString IcoExtension;
        static Common::GlobalWString EotExtension;
        static Common::GlobalWString SvgExtension;
        static Common::GlobalWString TtfExtension;
        static Common::GlobalWString WoffExtension;
        static Common::GlobalWString Woff2Extension;
        static Common::GlobalWString MapExtension;
        static uint MaxSingleThreadedReadSize;


        //
        // Method/Action names
        //
        static Common::GlobalWString CreateServiceFromTemplate;
        static Common::GlobalWString Create;
        static Common::GlobalWString CreateServiceGroup;
        static Common::GlobalWString Update;
        static Common::GlobalWString Copy;
        static Common::GlobalWString UpdateServiceGroup;
        static Common::GlobalWString Delete;
        static Common::GlobalWString DeleteServiceGroup;
        static Common::GlobalWString GetServiceDescription;
        static Common::GlobalWString GetServiceGroupDescription;
        static Common::GlobalWString GetServiceGroupMembers;
        static Common::GlobalWString UploadChunk;
        static Common::GlobalWString DeleteUploadSession;
        static Common::GlobalWString GetUploadSession;
        static Common::GlobalWString CommitUploadSession;
        static Common::GlobalWString Provision;
        static Common::GlobalWString UnProvision;
        static Common::GlobalWString Upgrade;
        static Common::GlobalWString UpdateUpgrade;
        static Common::GlobalWString RollbackUpgrade;
        static Common::GlobalWString GetUpgradeProgress;
        static Common::GlobalWString MoveToNextUpgradeDomain;
        static Common::GlobalWString GetClusterManifest;
        static Common::GlobalWString GetClusterVersion;
        static Common::GlobalWString GetApplicationManifest;
        static Common::GlobalWString GetServiceManifest;
        static Common::GlobalWString GetServicePackage;
        static Common::GlobalWString Activate;
        static Common::GlobalWString Restart;
        static Common::GlobalWString Logs;
        static Common::GlobalWString ContainerLogs;
        static Common::GlobalWString ContainerApi;
        static Common::GlobalWString ContainerApiPathString;
        static Common::GlobalWString ContainerApiPath;
        static Common::GlobalWString Remove;
        static Common::GlobalWString Stop;
        static Common::GlobalWString Start;
        static Common::GlobalWString Deactivate;
        static Common::GlobalWString RemoveNodeState;
        static Common::GlobalWString GetServiceTypes;
        static Common::GlobalWString GetServices;
        static Common::GlobalWString GetServiceGroups;
        static Common::GlobalWString GetSystemServices;
        static Common::GlobalWString GetPartitions;
        static Common::GlobalWString GetReplicas;
        static Common::GlobalWString GetApplications;
        static Common::GlobalWString Applications;
        static Common::GlobalWString GetNodes;
        static Common::GlobalWString Nodes;
        static Common::GlobalWString GetNetworks;
        static Common::GlobalWString GetCodePackages;
        static Common::GlobalWString RecoverAllPartitions;
        static Common::GlobalWString Recover;
        static Common::GlobalWString RecoverSystemPartitions;
        static Common::GlobalWString ResetLoad;
        static Common::GlobalWString ToggleServicePlacementHealthReportingVerbosity;
        static Common::GlobalWString GetHealthStateList;
        static Common::GlobalWString GetReplicaHealthStateList;
        static Common::GlobalWString GetDeployedApplicationHealthStateList;
        static Common::GlobalWString GetDeployedServicePackageHealthStateList;
        static Common::GlobalWString GetDeployedReplicaDetail;
        static Common::GlobalWString GetProvisionedFabricCodeVersions;
        static Common::GlobalWString GetProvisionedFabricConfigVersions;
        static Common::GlobalWString ResolvePartition;
        static Common::GlobalWString EventsHealthStateFilterString;
        static Common::GlobalWString NodesHealthStateFilterString;
        static Common::GlobalWString ReplicasHealthStateFilterString;
        static Common::GlobalWString PartitionsHealthStateFilterString;
        static Common::GlobalWString ServicesHealthStateFilterString;
        static Common::GlobalWString ApplicationsHealthStateFilterString;
        static Common::GlobalWString DeployedApplicationsHealthStateFilterString;
        static Common::GlobalWString DeployedServicePackagesHealthStateFilterString;
        static Common::GlobalWString ExcludeHealthStatisticsString;
        static Common::GlobalWString IncludeSystemApplicationHealthStatisticsString;
        static Common::GlobalWString CreateRepairTask;
        static Common::GlobalWString CancelRepairTask;
        static Common::GlobalWString ForceApproveRepairTask;
        static Common::GlobalWString DeleteRepairTask;
        static Common::GlobalWString UpdateRepairExecutionState;
        static Common::GlobalWString GetRepairTaskList;
        static Common::GlobalWString UpdateRepairTaskHealthPolicy;
        static Common::GlobalWString TaskIdFilter;
        static Common::GlobalWString StateFilter;
        static Common::GlobalWString ExecutorFilter;
        static Common::GlobalWString GetServiceName;
        static Common::GlobalWString GetApplicationName;
        static Common::GlobalWString GetSubNames;
        static Common::GlobalWString GetProperty;
        static Common::GlobalWString GetProperties;
        static Common::GlobalWString SubmitBatch;

        static Common::GlobalWString PartitionKeyType;
        static Common::GlobalWString PartitionKeyValue;
        static Common::GlobalWString PreviousRspVersion;

        static Common::GlobalWString ForceRemove;
        static Common::GlobalWString CreateFabricDump;
        static Common::GlobalWString DeployServicePackageToNode;

        //
        // Http verbs
        //
        static Common::GlobalWString HttpGetVerb;
        static Common::GlobalWString HttpPostVerb;
        static Common::GlobalWString HttpDeleteVerb;
        static Common::GlobalWString HttpPutVerb;

        //
        // Http status codes
        //
        static USHORT StatusOk;
        static USHORT StatusCreated;
        static USHORT StatusNoContent;
        static USHORT StatusBadRequest;
        static USHORT StatusAccepted;
        static USHORT StatusAuthenticate;
        static USHORT StatusUnauthorized;
        static USHORT StatusMovedPermanently;
        static USHORT StatusNotFound;
        static USHORT StatusConflict;
        static USHORT StatusPreconditionFailed;
        static USHORT StatusRangeNotSatisfiable;
        static USHORT StatusServiceUnavailable;
        static Common::GlobalWString StatusDescriptionOk;
        static Common::GlobalWString StatusDescriptionCreated;
        static Common::GlobalWString StatusDescriptionNoContent;
        static Common::GlobalWString StatusDescriptionBadRequest;
        static Common::GlobalWString StatusDescriptionAccepted;
        static Common::GlobalWString StatusDescriptionClientCertificateRequired;
        static Common::GlobalWString StatusDescriptionClientCertificateInvalid;
        static Common::GlobalWString StatusDescriptionUnauthorized;
        static Common::GlobalWString StatusDescriptionNotFound;
        static Common::GlobalWString StatusDescriptionConflict;
        static Common::GlobalWString StatusDescriptionServiceUnavailable;

        static Common::GlobalWString NegotiateHeaderValue;

        // Tag for ktl async context allocation
        static ULONG KtlAsyncDataContextTag;

        // /Nodes?NodeStatusFilter={nodeStatusFilter}
        static Common::GlobalWString NodeStatusFilterDefaultString;
        static Common::GlobalWString NodeStatusFilterAllString;
        static Common::GlobalWString NodeStatusFilterUpString;
        static Common::GlobalWString NodeStatusFilterDownString;
        static Common::GlobalWString NodeStatusFilterEnablingString;
        static Common::GlobalWString NodeStatusFilterDisablingString;
        static Common::GlobalWString NodeStatusFilterDisabledString;
        static Common::GlobalWString NodeStatusFilterUnknownString;
        static Common::GlobalWString NodeStatusFilterRemovedString;

        static Common::GlobalWString NodeStatusFilterDefault;
        static Common::GlobalWString NodeStatusFilterAll;
        static Common::GlobalWString NodeStatusFilterUp;
        static Common::GlobalWString NodeStatusFilterDown;
        static Common::GlobalWString NodeStatusFilterEnabling;
        static Common::GlobalWString NodeStatusFilterDisabling;
        static Common::GlobalWString NodeStatusFilterDisabled;
        static Common::GlobalWString NodeStatusFilterUnknown;
        static Common::GlobalWString NodeStatusFilterRemoved;

        static Common::GlobalWString TestCommandTypeDataLossString;
        static Common::GlobalWString TestCommandTypeQuorumLossString;
        static Common::GlobalWString TestCommandTypeRestartPartitionString;

        static Common::GlobalWString TestCommandTypeDataLoss;
        static Common::GlobalWString TestCommandTypeQuorumLoss;
        static Common::GlobalWString TestCommandTypeRestartPartition;

        // /Networks?NetworkStatusFilter={networkStatusFilter}
        static Common::GlobalWString NetworkStatusFilterDefaultString;
        static Common::GlobalWString NetworkStatusFilterAllString;
        static Common::GlobalWString NetworkStatusFilterReadyString;
        static Common::GlobalWString NetworkStatusFilterCreatingString;
        static Common::GlobalWString NetworkStatusFilterDeletingString;
        static Common::GlobalWString NetworkStatusFilterUpdatingString;
        static Common::GlobalWString NetworkStatusFilterStoppingString;
        static Common::GlobalWString NetworkStatusFilterStoppedString;
        static Common::GlobalWString NetworkStatusFilterStartingString;
        static Common::GlobalWString NetworkStatusFilterFailedString;

        static Common::GlobalWString NetworkStatusFilterDefault;
        static Common::GlobalWString NetworkStatusFilterAll;
        static Common::GlobalWString NetworkStatusFilterReady;
        static Common::GlobalWString NetworkStatusFilterCreating;
        static Common::GlobalWString NetworkStatusFilterDeleting;
        static Common::GlobalWString NetworkStatusFilterUpdating;
        static Common::GlobalWString NetworkStatusFilterStopping;
        static Common::GlobalWString NetworkStatusFilterStopped;
        static Common::GlobalWString NetworkStatusFilterStarting;
        static Common::GlobalWString NetworkStatusFilterFailed;

        // Data loss modes
        static Common::GlobalWString PartialDataLoss;
        static Common::GlobalWString FullDataLoss;

        // Quorum loss modes
        static Common::GlobalWString QuorumLossReplicas;
        static Common::GlobalWString AllReplicas;

        static Common::GlobalWString AllReplicasOrInstances;
        static Common::GlobalWString OnlyActiveSecondaries;

        static Common::GlobalWString NodeTransitionType;
        static Common::GlobalWString StopDurationInSeconds;

        static Common::GlobalWString State;
        static Common::GlobalWString Type;
        static Common::GlobalWString Force;
        static Common::GlobalWString Immediate;

        static Common::GlobalWString EventsStoreHandlerPath;
        static Common::GlobalWString EventsStoreServiceName;
        static Common::GlobalWString EventsStorePrefix;

        #define EMPTY_URI_QUERY_FILTER Common::NamingUri::RootNamingUri
        #define EMPTY_STRING_QUERY_FILTER std::wstring()
    };
}

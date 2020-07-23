// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Fabric;

    internal static class Constants
    {
        public const string WindowsFabricRepairTask = "WindowsFabricRepairTask";
        public const string ServiceFabricRepairTask = "ServiceFabricRepairTask";
        public const string WindowsFabricRepairTaskHealthPolicy = "WindowsFabricRepairTaskHealthPolicy";
        public const string ServiceFabricRepairTaskHealthPolicy = "ServiceFabricRepairTaskHealthPolicy";

        public const string PropertyNameTaskId = "TaskId";
        public const string PropertyNameVersion = "Version";

        /// These two fields required to be const type.
        public const string SendReplicaHealthReportstatefulServiceParamSetName = "StatefulService";
        public const string SendReplicaHealthReportStatelessServiceParamSetName = "StatelessService";

        // Secret Store Service
        public const int SecretNameMaxLength = 256;
        public const int SecretVersionMaxLength = 256;
        public const int SecretValueMaxSize = 4 * 1024 * 1024; // 4MB

        #region Chaos
        public const string MaxClusterStabilizationTimeoutSecPropertyName = "MaxClusterStabilizationTimeoutSec";
        public const string WaitTimeBetweenIterationsSecPropertyName = "WaitTimeBetweenIterationsSec";
        public const string MaxConcurrentFaultsProprtyName = "MaxConcurrentFaults";
        public const string TimeToRunSecPropertyName = "TimeToRunSec";
        public const string ChaosEventPropertyName = "ChaosEvent";
        public const string ChaosScheduleJobsPropertyName = "Jobs";
        public const string ChaosParametersDictionaryPropertyName = "ChaosParametersDictionary";
        public const string ChaosSchedulePropertyName = "Schedule";

        public const int MaxClusterStabilizationTimeoutInSecondsDefault = 60;
        public const uint MaxConcurrentFaultsDefault = 1;
        public const uint WaitTimeBetweenIterationsInSecondsDefault = 30;
        public const uint TimeToRunInMinutesDefault = 10;
        #endregion

        // OrchestrationUpgrade Apis Constants
        public const string StartClusterConfigurationUpgrade = "ServiceFabricClusterConfigurationUpgrade";
        public const string GetClusterConfigurationUpgradeStatus = "ServiceFabricClusterConfigurationUpgradeStatus";
        public const string GetRuntimeSupportedVersion = "ServiceFabricRuntimeSupportedVersion";
        public const string GetRuntimeUpgradeVersion = "ServiceFabricRuntimeUpgradeVersion";
        public const string GetClusterConfiguration = "ServiceFabricClusterConfiguration";
        public const string GetOrchestrationUpgradesPendingApproval = "ServiceFabricPendingApprovalUpgrades";
        public const string StartOrchestrationApprovedUpgrades = "ServiceFabricApprovedUpgrades";
        public const string GetUpgradeOrchestrationServiceState = "ServiceFabricUpgradeOrchestrationServiceState";

        public const string ServiceFabricTestCommand = "ServiceFabricTestCommand";
        public const string StartNodeTransitionName = "ServiceFabricNodeTransition";
        public const string GetNodeTransitionProgressName = "ServiceFabricNodeTransitionProgress";

        public static readonly string TestApplicationPackageErrorId = "TestApplicationPackageErrorId";
        public static readonly string CopyApplicationPackageErrorId = "CopyApplicationPackageErrorId";
        public static readonly string RemoveApplicationPackageErrorId = "RemoveApplicationPackageErrorId";
        public static readonly string RegisterApplicationTypeErrorId = "RegisterApplicationTypeErrorId";
        public static readonly string CreateApplicationInstanceErrorId = "CreateApplicationInstanceErrorId";
        public static readonly string CreateComposeDeploymentInstanceErrorId = "CreateComposeDeploymentInstanceErrorId";
        public static readonly string UpdateApplicationInstanceErrorId = "UpdateApplicationInstanceErrorId";
        public static readonly string GetNameValueCollectionErrorId = "GetNameValueCollectionErrorId";
        public static readonly string UnregisterApplicationTypeErrorId = "UnregisterApplicationTypeErrorId";
        public static readonly string RemoveApplicationInstanceErrorId = "RemoveApplicationInstanceErrorId";
        public static readonly string RemoveComposeDeploymentInstanceErrorId = "RemoveComposeDeploymentInstanceErrorId";
        public static readonly string UpgradeApplicationErrorId = "UpgradeApplicationErrorId";
        public static readonly string UpgradeComposeDeploymentErrorId = "UpgradeComposeDeploymentErrorId";
        public static readonly string UpdateApplicationUpgradeErrorId = "UpdateApplicationUpgradeErrorId";
        public static readonly string RollbackApplicationUpgradeErrorId = "RollbackApplicationUpgradeErrorId";
        public static readonly string UpdateClusterUpgradeErrorId = "UpdateClusterUpgradeErrorId";
        public static readonly string RollbackClusterUpgradeErrorId = "RollbackClusterUpgradeErrorId";
        public static readonly string RollbackComposeDeploymentUpgradeErrorId = "RollbackComposeDeploymentUpgradeErrorId";
        public static readonly string GetApplicationUpgradeProgressErrorId = "GetApplicationUpgradeProgressErrorId";
        public static readonly string GetComposeDeploymentUpgradeProgressErrorId = "GetComposeDeploymentUpgradeProgressErrorId";
        public static readonly string ResumeApplicationUpgradeDomainErrorId = "ResumeApplicationUpgradeDomainErrorId";
        public static readonly string GetApplicationManifestErrorId = "GetApplicationManifestErrorId";
        public static readonly string TestClusterManifestErrorId = "TestClusterManifestErrorId";
        public static readonly string GetClusterManifestErrorId = "GetClusterManifestErrorId";
        public static readonly string GetClusterConnectionErrorId = "GetClusterConnectionErrorId";
        public static readonly string CreateClusterConnectionErrorId = "CreateClusterConnectionErrorId";
        public static readonly string NewNodeConfigurationErrorId = "ConfigureNodeErrorId";
        public static readonly string NewNetworkConfigurationErrorId = "ConfigureNetworkErrorId";
        public static readonly string GetNetworkErrorId = "GetNetworkErrorId";
        public static readonly string NetworkDescriptionNetworkNamePropertyName = "NetworkName";
        public static readonly string NetworkDescriptionNetworkStatusPropertyName = "NetworkStatus";
        public static readonly string NetworkDescriptionNetworkTypePropertyName = "NetworkType";
        public static readonly string NetworkDescriptionNetworkAddressPrefixPropertyName = "NetworkAddressPrefix";
        public static readonly string UpdateNodeConfigurationErrorId = "UpdateNodeConfigurationErrorId";
        public static readonly string RemoveNodeConfigurationErrorId = "RemoveNodeConfigurationErrorId";
        public static readonly string CreateClusterErrorId = "CreateClusterErrorId";
        public static readonly string RemoveClusterErrorId = "RemoveClusterErrorId";
        public static readonly string TestClusterConnectionErrorId = "TestClusterConnectionErrorId";
        public static readonly string GetApplicationErrorId = "GetApplicationErrorId";
        public static readonly string GetComposeDeploymentStatusErrorId = "GetComposeDeploymentStatusErrorId";
        public static readonly string GetDeployedApplicationErrorId = "GetDeployedApplicationErrorId";
        public static readonly string GetDeployedApplicationServiceTypeErrorId = "GetDeployedApplicationServiceTypeErrorId";
        public static readonly string GetApplicationTypeErrorId = "GetApplicationTypeErrorId";
        public static readonly string GetNodeErrorId = "GetNodeErrorId";
        public static readonly string GetServiceErrorId = "GetServiceErrorId";
        public static readonly string GetServiceManifestErrorId = "GetServiceManifestErrorId";
        public static readonly string GetServiceDescriptionErrorId = "GetServiceDescriptionErrorId";
        public static readonly string GetServiceGroupDescriptionErrorId = "GetServiceGroupDescriptionErrorId";
        public static readonly string GetImageStoreContentErrorId = "GetImageStoreContentErrorId";
        public static readonly string ResolveServiceErrorId = "ResolveServiceErrorId";
        public static readonly string GetDeployedServiceManifestErrorId = "GetDeployedServiceManifestErrorId";
        public static readonly string GetPartitionErrorId = "GetPartitionErrorId";
        public static readonly string GetReplicaErrorId = "GetReplicaErrorId";
        public static readonly string GetDeployedReplicaErrorId = "GetDeployedReplicaErrorId";
        public static readonly string GetDeployedReplicaDetailErrorId = "GetDeployedReplicaDetailErrorId";
        public static readonly string GetServiceTypeErrorId = "GetServiceTypeErrorId";
        public static readonly string GetDeployedServiceTypeErrorId = "GetDeployedReplicaErrorId";
        public static readonly string GetSystemServiceErrorId = "GetSystemServiceErrorId";
        public static readonly string GetPartitionSchemeErrorId = "GetPartitionSchemeErrorId";
        public static readonly string CreateServiceErrorId = "CreateServiceErrorId";
        public static readonly string CreateServiceGroupErrorId = "CreateServiceGroupErrorId";
        public static readonly string UpdateServiceErrorId = "UpdateServiceErrorId";
        public static readonly string UpdateServiceGroupErrorId = "UpdateServiceGroupErrorId";
        public static readonly string RemoveServiceErrorId = "RemoveServiceErrorId";
        public static readonly string RemoveServiceGroupErrorId = "RemoveServiceGroupErrorId";
        public static readonly string InvokeEncryptSecretErrorId = "InvokeEncryptSecretErrorId";
        public static readonly string InvokeEncryptTextErrorId = "InvokeEncryptTextErrorId";
        public static readonly string InvokeDecryptTextErrorId = "InvokeDecryptTextErrorId";
        public static readonly string GetClusterUpgradeProgressErrorId = "GetClusterUpgradeProgressErrorId";
        public static readonly string ResumeClusterUpgradeDomainErrorId = "ResumeClusterUpgradeDomainErrorId";
        public static readonly string UpgradeClusterErrorId = "UpgradeClusterErrorId";
        public static readonly string RegisterClusterPackageErrorId = "RegisterClusterPackageErrorId";
        public static readonly string UnRegisterClusterPackageErrorId = "UnRegisterClusterPackageErrorId";
        public static readonly string CopyClusterPackageErrorId = "CopyClusterPackageErrorId";
        public static readonly string RemoveClusterPackageErrorId = "RemoveClusterPackageErrorId";
        public static readonly string RemoveNodeStateErrorId = "RemoveNodeStateErrorId";
        public static readonly string GetApplicationHealthErrorId = "GetApplicationHealthErrorId";
        public static readonly string GetServiceHealthErrorId = "GetServiceHealthErrorId";
        public static readonly string GetPartitionHealthErrorId = "GetPartitionHealthErrorId";
        public static readonly string GetReplicaHealthErrorId = "GetReplicaHealthErrorId";
        public static readonly string GetNodeHealthErrorId = "GetNodeHealthErrorId";
        public static readonly string GetClusterHealthErrorId = "GetClusterHealthErrorId";
        public static readonly string GetClusterHealthChunkErrorId = "GetClusterHealthChunkErrorId";
        public static readonly string GetDeployedApplicationHealthErrorId = "GetDeployedApplicationHealthErrorId";
        public static readonly string GetDeployedServicePackageHealthErrorId = "GetDeployedServicePackageHealthErrorId";
        public static readonly string DisableNodeErrorId = "DisableNodeErrorId";
        public static readonly string EnableNodeErrorId = "EnableNodeErrorId";
        public static readonly string StartNodeErrorId = "StartNodeErrorId";
        public static readonly string RepairPartitionErrorId = "RepairPartitionErrorId";
        public static readonly string ResetPartitionLoadErrorId = "ResetPartitionLoadErrorId";
        public static readonly string SendClusterHealthReportErrorId = "SendClusterHealthReportErrorId";
        public static readonly string SendApplicationHealthReportErrorId = "SendApplicationHealthReportErrorId";
        public static readonly string SendNodeHealthReportErrorId = "SendNodeHealthReportErrorId";
        public static readonly string SendPartitionHealthReportErrorId = "SendPartitionHealthReportErrorId";
        public static readonly string SendInstanceHealthReportErrorId = "SendInstanceHealthReportErrorId";
        public static readonly string SendReplicaHealthReportErrorId = "SendReplicaHealthReportErrorId";
        public static readonly string SendServiceHealthReportErrorId = "SendServiceHealthReportErrorId";
        public static readonly string SendDeployedApplicationHealthReportErrorId = "SendDeployedApplicationHealthReportErrorId";
        public static readonly string SendDeployedServicePackageHealthReportErrorId = "SendDeployedServicePackageHealthReportErrorId";
        public static readonly string ToggleVerboseServicePlacementHealthReportingErrorId = "ToggleVerboseServicePlacementHealthReportingErrorId";
        public static readonly string ReportFaultCommandErrorId = "ReportFaultErrorId";
        public static readonly string MoveReplicaCommandErrorId = "MoveReplicaErrorId";
        public static readonly string NodeOperationCommandErrorId = "NodeOperationErrorId";
        public static readonly string CleanTestStateCommandErrorId = "CleanTestStateOperationErrorId";
        public static readonly string CodePackageOperationCommandErrorId = "CodePackageOperationErrorId";
        public static readonly string GetClusterCodeVersionErrorId = "GetClusterCodeVersionErrorId";
        public static readonly string GetClusterConfigVersionErrorId = "GetClusterConfigVersionErrorId";
        public static readonly string InvokeInfrastructureCommandErrorId = "InvokeInfrastructureCommandErrorId";
        public static readonly string InvokeInfrastructureQueryErrorId = "InvokeInfrastructureQueryErrorId";
        public static readonly string DeployServicePackageToNodeErrorId = "DeployServicePackageToNodeErrorId";
        public static readonly string GetApplicationLoadInformationErrorId = "GetApplicationLoadInformationErrorId";
        public static readonly string TestClusterConfigurationErrorId = "TestClusterConfigurationErrorId";
        public static readonly string GetImageStoreConnectionStringErrorId = "GetImageStoreConnectionStringErrorId";
        public static readonly string GetAbnormalProcessTermination = "GetAbnormalProcessTermination";

        public static readonly string ReconfigurationInformation = "ReconfigurationInformation";
        public static readonly string ReconfigurationStartTimeUtc = "ReconfigurationStartTimeUtc";
        public static readonly string DeployedServiceReplica = "DeployedServiceReplica";
        public static readonly string DeployedServiceReplicaInstance = "DeployedServiceReplicaInstance";
        public static readonly string ServiceName = "ServiceName";
        public static readonly string ServiceTypeName = "ServiceTypeName";
        public static readonly string ServiceManifestVersion = "ServiceManifestVersion";
        public static readonly string ServiceKind = "ServiceKind";
        public static readonly string ReplicaId = "ReplicaId";
        public static readonly string InstanceId = "InstanceId";
        public static readonly string PartitionId = "PartitionId";

        public static readonly string InvokeDataLossCommandErrorId = "InvokeDataLossErrorId";
        public static readonly string GetInvokeDataLossProgressCommandErrorId = "GetInvokeDataLossProgressCommandErrorId";
        public static readonly string RestartPartitionCommandErrorId = "RestartPartitionErrorId";
        public static readonly string GetRestartPartitionProgressCommandErrorId = "GetRestartPartitionProgressCommandErrorId";
        public static readonly string InvokeFailoverTestCommandErrorId = "InvokeFailoverTestErrorId";
        public static readonly string InvokeChaosTestCommandErrorId = "InvokeChaosTestErrorId";
        public static readonly string GetTestCommandStatusListCommandErrorId = "GetTestCommandStatusListCommandErrorId";
        public static readonly string InvokeQuorumLossCommandErrorId = "InvokeQuorumLossErrorId";
        public static readonly string GetInvokeQuorumLossProgressCommandErrorId = "GetInvokeQuorumLossProgressCommandErrorId";
        public static readonly string StopTestCommandCommandErrorId = "StopTestCommandCommandErrorId";
        public static readonly string StartNodeTransitionCommandErrorId = "StartNodeTransitionCommandErrorId";
        public static readonly string GetNodeTransitionProgressCommandErrorId = "GetNodeTransitionProgressCommandErrorId";

        public static readonly string StartDataLossCommandErrorId = "StartDataLossCommandErrorId";
        public static readonly string StartQuorumLossCommandErrorId = "StartQuorumLossCommandErrorId";
        public static readonly string StartRestartPartitionCommandErrorId = "StartRestartPartitionCommandErrorId";

        public static readonly string StartChaosCommandErrorId = "StartChaosCommandErrorId";
        public static readonly string GetChaosReportCommandErrorId = "GetChaosReportCommandErrorId";
        public static readonly string GetChaosEventsCommandErrorId = "GetChaosEventsCommandErrorId";
        public static readonly string GetChaosCommandErrorId = "GetChaosCommandErrorId";
        public static readonly string GetChaosScheduleCommandErrorId = "GetChaosScheduleCommandErrorId";
        public static readonly string SetChaosScheduleCommandErrorId = "SetChaosScheduleCommandErrorId";
        public static readonly string StopChaosCommandErrorId = "StopChaosCommandErrorId";
        public static readonly string ChaosParametersPropertyName = "ChaosParameters";

        public static readonly string StartRepairTaskErrorId = "StartRepairTaskErrorId";
        public static readonly string StopRepairTaskErrorId = "StopRepairTaskErrorId";
        public static readonly string RemoveRepairTaskErrorId = "RemoveRepairTaskErrorId";
        public static readonly string ApproveRepairTaskErrorId = "ApproveRepairTaskErrorId";
        public static readonly string CompleteRepairTaskErrorId = "CompleteRepairTaskErrorId";
        public static readonly string GetRepairTaskErrorId = "GetRepairTaskErrorId";
        public static readonly string UpdateRepairTaskHealthPolicyErrorId = "UpdateRepairTaskHealthPolicyErrorId";

        public static readonly string BaseTestRootFolderName = @"TestApplicationPackage";
        public static readonly string ImageStoreConnectionFileType = "file:";
        public static readonly string ImageStoreConnectionXStoreType = "xstore:";
        public static readonly string ImageStoreConnectionFabricType = "fabric:";

        public static readonly string ClusterConnectionVariableName = "ClusterConnection";
        public static readonly string FabricHostServiceName = "FabricHostSvc";
        public static readonly string DummyUri = @"fabric:/DummyUri";
        public static readonly string SystemApplicationUri = @"fabric:/System";
        public static readonly string FailoverManagerUri = SystemApplicationUri + @"/FailoverManagerService";
        public static readonly string OutputExistCodeScriptPattern = @"; $lastexitcode";
        public static readonly string FabricSetupFileName = "FabricSetup.exe";
        public static readonly string FabricSetupRemoveNodeStateScriptPattern = @"&""{0}"" /operation:removenodestate";
        public static readonly string FabricSetupUninstallScriptPattern = @"&""{0}"" /operation:uninstall";
        public static readonly string FabricSetupInstallScriptPattern = @"&""{0}"" /operation:install";
        public static readonly string FabricSetupRelativePathToRoot = @"bin\fabric\fabric.code";
        public static readonly string DefaultFabricRoot = @"%ProgramFiles%\Micorosoft Service Fabric";
        public static readonly string DeleteLogArgumentPattern = @" /deleteLog:true";

        public static readonly int DefaultTimeoutSec = 120;
        public static readonly string TimeoutSecParameterName = "TimeoutSec";

        public static readonly string TestPathCommandName = "Test-Path";
        public static readonly string JoinPathCommandName = "Join-Path";
        public static readonly string GetItemPropertyCommandName = "Get-ItemProperty";
        public static readonly string FabricCodePathRegistrySubkey = "FabricCodePath";

        public static readonly string ConfigurationCsvFileName = "Configurations.csv";
        public static readonly string ServiceModelSchemaFileName = "ServiceFabricServiceModel.xsd";
        public static readonly string FabricNamespace = "http://schemas.microsoft.com/2011/01/fabric";

        public static readonly string NodeNamePropertyName = "NodeName";
        public static readonly string ApplicationNamePropertyName = "ApplicationName";
        public static readonly string ApplicationParametersPropertyName = "ApplicationParameters";
        public static readonly string ComposeFilePathsPropertyName = "ComposeFilePaths";
        public static readonly string DefaultParametersPropertyName = "DefaultParameters";
        public static readonly string ApplicationTypePropertyName = "ApplicationTypeName";
        public static readonly string ApplicationTypeVersionPropertyName = "ApplicationTypeVersion";
        public static readonly string UpgradeTypeVersionPropertyName = "UpgradeTypeVersion";
        public static readonly string UpgradeParametersPropertyName = "UpgradeParameters";
        public static readonly string ServiceNamePropertyName = "ServiceName";
        public static readonly string ServiceDnsNamePropertyName = "ServiceDnsName";
        public static readonly string ScalingPoliciesPropertyName = "ScalingPolicies";
        public static readonly string ServiceTypeNamePropertyName = "ServiceTypeName";
        public static readonly string ServiceGroupTypeNamePropertyName = "ServiceGroupTypeName";
        public static readonly string IsServiceGroupPropertyName = "IsServiceGroup";
        public static readonly string ServiceTypeKindPropertyName = "ServiceTypeKind";
        public static readonly string ServiceKindPropertyName = "ServiceKind";
        public static readonly string ServiceGroupListPropertyName = "ServiceGroupList";
        public static readonly string ServiceGroupMembersPropertyName = "ServiceGroupMembers";
        public static readonly string KindPropertyName = "Kind";
        public static readonly string ServiceStatusPropertyName = "ServiceStatus";
        public static readonly string LoadMetricsPropertyName = "LoadMetrics";
        public static readonly string LoadMetricReportPropertyName = "LoadMetricReport";
        public static readonly string LoadMetricInformationPropertyName = "LoadMetricInformation";
        public static readonly string LoadMetricInformationNamePropertyName = "LoadMetricName";
        public static readonly string LoadMetricInformationIsBalancedBeforePropertyName = "IsBalancedBefore";
        public static readonly string LoadMetricInformationIsBalancedAfterPropertyName = "IsBalancedAfter";
        public static readonly string LoadMetricInformationDeviationBeforePropertyName = "DeviationBefore";
        public static readonly string LoadMetricInformationDeviationAfterPropertyName = "DeviationAfter";
        public static readonly string LoadMetricInformationBalancingThresholdPropertyName = "BalancingThreshold";
        public static readonly string LoadMetricInformationActionPropertyName = "Action";
        public static readonly string LoadMetricInformationActivityThresholdPropertyName = "ActivityThreshold";
        public static readonly string LoadMetricInformationClusterCapacityPropertyName = "ClusterCapacity";
        public static readonly string LoadMetricInformationClusterLoadPropertyName = "ClusterLoad";
        public static readonly string LoadMetricInformationClusterRemainingCapacityPropertyName = "ClusterRemainingCapacity";
        public static readonly string LoadMetricInformationNodeBufferPercentagePropertyName = "NodeBufferPercentage";
        public static readonly string LoadMetricInformationClusterBufferedCapacityPropertyName = "ClusterBufferedCapacity";
        public static readonly string LoadMetricInformationClusterRemainingBufferedCapacityPropertyName = "ClusterRemainingBufferedCapacity";
        public static readonly string LoadMetricInformationClusterCapacityViolationPropertyName = "ClusterCapacityViolation";
        public static readonly string LoadMetricInformationMinNodeLoadValue = "MinNodeLoadValue";
        public static readonly string LoadMetricInformationMinNodeLoadNodeId = "MinNodeLoadNodeId";
        public static readonly string LoadMetricInformationMaxNodeLoadValue = "MaxNodeLoadValue";
        public static readonly string LoadMetricInformationMaxNodeLoadNodeId = "MaxNodeLoadNodeId";
        public static readonly string LoadMetricReportsPropertyName = "LoadMetricReports";
        public static readonly string PrimaryLoadMetricReportsPropertyName = "PrimaryLoadMetricReports";
        public static readonly string SecondaryLoadMetricReportsPropertyName = "SecondaryLoadMetricReports";
        public static readonly string ExtensionsPropertyName = "Extensions";
        public static readonly string PlacementConstraintsPropertyName = "PlacementConstraints";
        public static readonly string PlacementPoliciesPropertyName = "PlacementPolicies";
        public static readonly string IsDefaultMoveCostSpecifiedPropertyName = "IsDefaultMoveCostSpecified";
        public static readonly string DefaultMoveCostPropertyName = "DefaultMoveCost";
        public static readonly string ServiceCorrelationPropertyName = "CorrelatedServices";
        public static readonly string HasPersistedStatePropertyName = "HasPersistedState";
        public static readonly string UseImplicitHostPropertyName = "UseImplicitHost";
        public static readonly string NodeStatusPropertyName = "NodeStatus";
        public static readonly string NodeUpTimePropertyName = "NodeUpTime";
        public static readonly string NodeDeactivationInfoProperyName = "NodeDeactivationInfo";
        public static readonly string NodeDeactivationIntent = "Intent";
        public static readonly string NodeDeactivationEffectiveIntent = "EffectiveIntent";
        public static readonly string NodeDeactivationStatus = "Status";
        public static readonly string NodeDeactivationTaskId = "TaskId";
        public static readonly string NodeDeactivationTaskType = "TaskType";
        public static readonly string NodeLoadMetricsPropertyName = "NodeLoad";
        public static readonly string NodeLoadMetricsPropertyMetricName = "MetricName";
        public static readonly string NodeLoadMetricsPropertyNodeCapacity = "NodeCapacity";
        public static readonly string NodeLoadMetricsPropertyNodeLoad = "NodeLoad";
        public static readonly string NodeLoadMetricsPropertyNodeRemainingCapacity = "NodeRemainingCapacity";
        public static readonly string NodeLoadMetricsPropertyNodeBufferedCapacity = "NodeBufferedCapacity";
        public static readonly string NodeLoadMetricsPropertyNodeRemainingBufferedCapacity = "NodeRemainingBufferedCapacity";
        public static readonly string NodeLoadMetricsPropertyIsCapacityViolation = "IsCapacityViolation";
        public static readonly string LastQuorumLossDurationPropertyName = "LastQuorumLossDuration";
        public static readonly string PartitionStatusPropertyName = "PartitionStatus";
        public static readonly string HealthStatePropertyName = "HealthState";
        public static readonly string MinReplicaSetSizePropertyName = "MinReplicaSetSize";
        public static readonly string TargetReplicaSetSizePropertyName = "TargetReplicaSetSize";
        public static readonly string ReplicaRestartWaitDurationPropertyName = "ReplicaRestartWaitDuration";
        public static readonly string QuorumLossWaitDurationPropertyName = "QuorumLossWaitDuration";
        public static readonly string StandByReplicaKeepDurationPropertyName = "StandByReplicaKeepDuration";
        public static readonly string PartitionIdPropertyName = "PartitionId";
        public static readonly string PartitionLowKeyPropertyName = "PartitionLowKey";
        public static readonly string PartitionHighKeyPropertyName = "PartitionHighKey";
        public static readonly string PartitionCountPropertyName = "PartitionCount";
        public static readonly string PartitionNamePropertyName = "PartitionName";
        public static readonly string PartitionNamesPropertyName = "PartitionNames";
        public static readonly string PartitionSchemePropertyName = "PartitionScheme";
        public static readonly string PartitionKindPropertyName = "PartitionKind";
        public static readonly string MemberDescriptionsPropertyName = "MemberDescriptions";
        public static readonly string MemberTypesPropertyName = "MemberTypes";
        public static readonly string ReplicaIdPropertyName = "ReplicaId";
        public static readonly string InstanceIdPropertyName = "InstanceId";
        public static readonly string ReplicaOrInstanceIdPropertyName = "ReplicaOrInstanceId";
        public static readonly string InstanceCountPropertyName = "InstanceCount";
        public static readonly string IdPropertyName = "Id";
        public static readonly string ForceRestartPropertyName = "ForceRestart";
        public static readonly string UpgradeModePropertyName = "UpgradeMode";
        public static readonly string UpgradeKindPropertyName = "UpgradeKind";
        public static readonly string UpgradeReplicaSetCheckTimeoutPropertyName = "UpgradeReplicaSetCheckTimeout";
        public static readonly string UpgradeDomainsPropertyName = "UpgradeDomainsStatus";
        public static readonly string UpgradeDomainNamePropertyName = "UpgradeDomain";
        public static readonly string UpgradeDescription = "UpgradeDescription";
        public static readonly string UpgradeUpdateDescription = "UpgradeUpdateDescription";
        public static readonly string PartitionSchemeDescriptionPropertyName = "PartitionSchemeDescription";
        public static readonly string AggregatedHealthStatePropertyName = "AggregatedHealthState";
        public static readonly string ServiceHealthStatesPropertyName = "ServiceHealthStates";
        public static readonly string DeployedApplicationHealthStatesPropertyName = "DeployedApplicationHealthStates";
        public static readonly string DeployedServicePackageHealthStatesPropertyName = "DeployedServicePackageHealthStates";
        public static readonly string PartitionHealthStatesPropertyName = "PartitionHealthStates";
        public static readonly string ReplicaHealthStatesPropertyName = "ReplicaHealthStates";
        public static readonly string HealthEventsPropertyName = "HealthEvents";
        public static readonly string HealthStatisticsPropertyName = "HealthStatistics";
        public static readonly string HealthEventDescriptionPropertyName = "Description";
        public static readonly string HealthEventLastModifiedUtcTimestampPropertyName = "ReceivedAt";
        public static readonly string HealthEventPropertyPropertyName = "Property";
        public static readonly string HealthEventSequenceNumberPropertyName = "SequenceNumber";
        public static readonly string HealthEventSourceIdPropertyName = "SourceId";
        public static readonly string HealthEventSourceUtcTimestampPropertyName = "SentAt";
        public static readonly string HealthEventStatePropertyName = "HealthState";
        public static readonly string HealthEventTimeToLivePropertyName = "TTL";
        public static readonly string HealthEventIsExpiredPropertyName = "IsExpired";
        public static readonly string HealthEventTransitions = "Transitions";
        public static readonly string HealthEventLastOkTransitionAt = "LastOk";
        public static readonly string HealthEventLastWarningTransitionAt = "LastWarning";
        public static readonly string HealthEventLastErrorTransitionAt = "LastError";
        public static readonly string HealthEventRemoveWhenExpiredPropertyName = "RemoveWhenExpired";
        public static readonly string ServiceManifestNamePropertyName = "ServiceManifestName";
        public static readonly string ServicePackageActivationIdPropertyName = "ServicePackageActivationId";
        public static readonly string EntryPointPropertyName = "EntryPoint";
        public static readonly string ServiceManifestVersionPropertyName = "ServiceManifestVersion";
        public static readonly string SetupEntryPointPropertyName = "SetupEntryPoint";
        public static readonly string EntryPointStatusPropertyName = "EntryPointStatus";
        public static readonly string CodePackageInstanceIdPropertyName = "CodePackageInstanceId";
        public static readonly string EntryPointLocationPropertyName = "EntryPointLocation";
        public static readonly string RunAsUserNamePropertyName = "RunAsUserName";
        public static readonly string ProcessIdPropertyName = "ProcessId";
        public static readonly string NextActivationUtcPropertyName = "NextActivationUtc";
        public static readonly string EndPointsPropertyName = "Endpoints";
        public static readonly string AddressPropertyName = "Address";
        public static readonly string RolePropertyName = "Role";
        public static readonly string FailureActionPropertyName = "FailureAction";
        public static readonly string HealthCheckRetryTimeoutPropertyName = "HealthCheckRetryTimeout";
        public static readonly string HealthCheckWaitDurationPropertyName = "HealthCheckWaitDuration";
        public static readonly string HealthCheckStableDurationPropertyName = "HealthCheckStableDuration";
        public static readonly string UpgradeDomainTimeoutPropertyName = "UpgradeDomainTimeout";
        public static readonly string UpgradeTimeoutPropertyName = "UpgradeTimeout";
        public static readonly string ConsiderWarningAsErrorPropertyName = "ConsiderWarningAsError";
        public static readonly string MaxPercentUnhealthyPartitionsPerServicePropertyName = "MaxPercentUnhealthyPartitionsPerService";
        public static readonly string MaxPercentUnhealthyReplicasPerPartitionPropertyName = "MaxPercentUnhealthyReplicasPerPartition";
        public static readonly string MaxPercentUnhealthyServicesPropertyName = "MaxPercentUnhealthyServices";
        public static readonly string MaxPercentUnhealthyApplicationsPropertyName = "MaxPercentUnhealthyApplications";
        public static readonly string MaxPercentUnhealthyDeployedApplicationsPropertyName = "MaxPercentUnhealthyDeployedApplications";
        public static readonly string DefaultConstantName = "Default";
        public static readonly string MaxPercentUnhealthyNodesPropertyName = "MaxPercentUnhealthyNodes";
        public static readonly string EnableDeltaHealthEvaluationPropertyName = "EnableDeltaHealthEvaluation";
        public static readonly string MaxPercentDeltaUnhealthyNodesPropertyName = "MaxPercentDeltaUnhealthyNodes";
        public static readonly string MaxPercentUpgradeDomainDeltaUnhealthyNodesPropertyName = "MaxPercentUpgradeDomainDeltaUnhealthyNodes";
        public static readonly string ServiceTypeHealthPolicyMapPropertyName = "ServiceTypeHealthPolicyMap";
        public static readonly string ServiceTypeHealthPolicyPropertyName = "ServiceTypeHealthPolicy";
        public static readonly string DefaultServiceTypeHealthPolicyPropertyName = "DefaultServiceTypeHealthPolicy";
        public static readonly string LoadMetricsNamePropertyName = "Name";
        public static readonly string LoadMetricsValuePropertyName = "Value";
        public static readonly string LoadMetricsLastReportedUtcPropertyName = "LastReportedUtc";
        public static readonly string ReportedLoadPropertyName = "ReportedLoad";
        public static readonly string DataLossNumberPropertyName = "DataLossNumber";
        public static readonly string ConfigurationNumberPropertyName = "ConfigurationNumber";
        public static readonly string ApplicationTypeHealthPolicyMapPropertyName = "ApplicationTypeHealthPolicyMap";
        public static readonly string ApplicationHealthPolicyPropertyName = "ApplicationHealthPolicy";
        public static readonly string ApplicationHealthPolicyMapPropertyName = "ApplicationHealthPolicyMap";
        public static readonly string ApplicationLoadMetricPropertyName = "MetricName";
        public static readonly string ApplicationLoadMetricPropertyApplicationLoad = "ApplicationLoad";
        public static readonly string ApplicationLoadMetricPropertyApplicationCapacity = "ApplicationCapacity";
        public static readonly string ApplicationLoadMetricPropertyReservedCapacity = "ReservedCapacity";

        public static readonly string ApplicationTypeName = "ApplicationTypeName";
        public static readonly string ApplicationTypeVersion = "ApplicationTypeVersion";
        public static readonly string ApplicationTypeStatus = "ApplicationTypeStatus";
        public static readonly string ApplicationTypeStatusDetails = "ApplicationTypeStatusDetails";
        public static readonly string ApplicationTypeDefaultParameters = "ApplicationTypeDefaultParameters";

        public static readonly string StoreRelativePathPropertyName = "StoreRelativePath";
        public static readonly string FileVersionPropertyName = "FileVersion";
        public static readonly string FileSizePropertyName = "FileSize";
        public static readonly string ModifiedDatePropertyName = "ModifiedDate";
        public static readonly string ApplicationVersionPropertyName = "ApplicationVersion";

        public static readonly string ActivationCountPropertyName = "ActivationCount";
        public static readonly string ActivationFailureCountPropertyName = "ActivationFailureCount";
        public static readonly string ContinuousActivationFailureCountPropertyName = "ContinuousActivationFailureCount";
        public static readonly string ContinuousExitFailureCountPropertyName = "ContinuousExitFailureCount";
        public static readonly string ExitCountPropertyName = "ExitCount";
        public static readonly string ExitFailureCountPropertyName = "ExitFailureCount";
        public static readonly string LastActivationUtcPropertyName = "LastActivationUtc";
        public static readonly string LastExitCodePropertyName = "LastExitCode";
        public static readonly string LastExitUtcPropertyName = "LastExitUtc";
        public static readonly string LastSuccessfulActivationUtcPropertyName = "LastSuccessfulActivationUtc";
        public static readonly string LastSuccessfulExitUtcPropertyName = "LastSuccessfulExitUtc";

        public static readonly string NodeHealthStateChunksPropertyName = "NodeHealthStateChunks";
        public static readonly string ApplicationHealthStateChunksPropertyName = "ApplicationHealthStateChunks";
        public static readonly string ServiceHealthStateChunksPropertyName = "ServiceHealthStateChunks";
        public static readonly string PartitionHealthStateChunksPropertyName = "PartitionHealthStateChunks";
        public static readonly string ReplicaHealthStateChunksPropertyName = "ReplicaHealthStateChunks";
        public static readonly string DeployedApplicationHealthStateChunksPropertyName = "DeployedApplicationHealthStateChunks";
        public static readonly string DeployedServicePackageHealthStateChunksPropertyName = "DeployedServicePackageHealthStateChunks";
        public static readonly string TotalCountPropertyName = "TotalCount";
        public static readonly string OkPropertyName = "Ok";
        public static readonly string WarningPropertyName = "Warning";
        public static readonly string ErrorPropertyName = "Error";

        public static readonly string NodeHealthStatesPropertyName = "NodeHealthStates";
        public static readonly string ApplicationHealthStatesPropertyName = "ApplicationHealthStates";
        public static readonly string ClusterConnectionFabricClientSettingsPropertyName = "FabricClientSettings";
        public static readonly string GatewayInformationPropertyName = "GatewayInformation";
        public static readonly string AzureActiveDirectoryMetadataPropertyName = "AzureActiveDirectoryMetadata";

        public static readonly string UnhealthyEvaluationsPropertyName = "UnhealthyEvaluations";
        public static readonly string ContinuationTokenPropertyName = "ContinuationToken";

        public static readonly string ReplicatorStatusPropertyName  = "ReplicatorStatus";
        public static readonly string RemoteReplicatorsPropertyName  = "RemoteReplicators";
        public static readonly string QueueUtilizationPercentagePropertyName  = "QueueUtilizationPercentage";
        public static readonly string ReplicaStatus  = "ReplicaStatus";

        public static readonly string CurrentUpgradeDomainProgressPropertyName = "CurrentUpgradeDomainProgress";
        public static readonly string UpgradeDomainProgressAtFailurePropertyName = "UpgradeDomainProgressAtFailure";
        public static readonly string UpgradePhasePropertyName = "UpgradePhase";
        public static readonly string PendingSafetyCheckPropertyName = "PendingSafetyChecks";

        public static readonly string FirstSequenceNumberPropertyName  = "FirstSequenceNumber";
        public static readonly string CompletedSequenceNumberPropertyName = "CompletedSequenceNumber";
        public static readonly string CommittedSequenceNumberPropertyName = "CommittedSequenceNumber";
        public static readonly string LastSequenceNumberPropertyName = "LastSequenceNumber";
        public static readonly string QueueMemorySizePropertyName = "QueueMemorySize";

        public static readonly string LastAcknowledgementProcessedTimeUtcPropertyName = "LastAcknowledgementProcessedTimeUtc";
        public static readonly string LastReceivedReplicationSequenceNumberPropertyName = "LastReceivedReplicationSequenceNumber";
        public static readonly string LastAppliedReplicationSequenceNumberPropertyName = "LastAppliedReplicationSequenceNumber";
        public static readonly string LastReceivedCopySequenceNumberPropertyName = "LastReceivedCopySequenceNumber";
        public static readonly string LastAppliedCopySequenceNumberPropertyName = "LastAppliedCopySequenceNumber";
        public static readonly string IsInBuild = "IsInBuild";

        public static readonly string AverageReceiveDuration = "AverageReceiveDuration";
        public static readonly string AverageApplyDuration = "AverageApplyDuration";
        public static readonly string NotReceivedCount = "NotReceivedCount";
        public static readonly string ReceivedAndNotAppliedCount = "ReceivedAndNotAppliedCount";

        public static readonly string CopyStreamAcknowledgementDetail = "CopyStreamAcknowledgementDetail";
        public static readonly string ReplicationStreamAcknowledgementDetail = "ReplicationStreamAcknowledgementDetail";

        public static readonly string LastAcknowledgementSentTimeUtcPropertyName = "LastAcknowledgementSentTimeUtc";
        public static readonly string LastReplicationOperationReceivedTimeUtcPropertyName = "LastReplicationOperationReceivedTimeUtc";
        public static readonly string LastCopyOperationReceivedTimeUtcPropertyName = "LastCopyOperationReceivedTimeUtc";

        public static readonly string ProviderKind = "ProviderKind";
        public static readonly string DatabaseRowCountEstimatePropertyName = "DatabaseRowCountEstimate";
        public static readonly string DatabaseLogicalSizeEstimatePropertyName = "DatabaseLogicalSizeEstimate";
        public static readonly string CopyNotificationCurrentKeyFilterPropertyName = "CopyNotificationCurrentKeyFilter";
        public static readonly string CopyNotificationCurrentProgressPropertyName = "CopyNotificationCurrentProgress";
        public static readonly string StatusDetails = "StatusDetails";
        public static readonly string CurrentMigrationPhase = "CurrentMigrationPhase";
        public static readonly string CurrentMigrationState = "CurrentMigrationState";
        public static readonly string NextMigrationPhase = "NextMigrationPhase";

        public static readonly string StoreFolder = "Store";
        public static readonly string WindowsFabricStoreFolder = "WindowsFabricStore";
        public static readonly string ReservedImageStoreFoldersName = "ReservedImageStoreFolders";
        public static readonly string ToStringMethodName = "ToString";
        public static readonly string FormatObjectMethodName = "FormatObject";
        public static readonly string DefaultStoreName = X509Credentials.StoreNameDefault;
        public static readonly string NullObjectOutput = "Null";
        public static readonly string NoneResultOutput = "None";
        public static readonly string MaxTTLOutput = "Infinite";
        public static readonly string NeverOutput = "Never";

        public static readonly string SelectedPartitionPropertyName = "SelectedPartition";

        public static readonly string SelectedReplicaPropertyName = "SelectedReplica";

        public static readonly string NodeResult = "NodeResult";

        public static readonly string RestartNodeResult = "RestartNodeResult";

        public static readonly string StartNodeResult = "StartNodeResult";

        public static readonly string StopNodeResult = "StopNodeResult";

        public static readonly string RemoveReplicaResult = "RemoveReplicaResult";

        public static readonly string RestartReplicaResult = "RestartReplicaResult";

        public static readonly string RestartPartitionResult = "RestartPartitionResult";

        public static readonly string RestartDeployedCodePackageResult = "RestartDeployedCodePackageResult";

        public static readonly string MovePrimaryReplicaResult = "MovePrimaryReplicaResult";

        public static readonly string MoveSecondaryReplicaResult = "MoveSecondaryReplicaResult";

        public static readonly string StatePropertyName = "State";
        public static readonly string ExceptionPropertyName = "Exception";
        public static readonly string ProgressResultPropertyName = "ProgressResult";
        public static readonly string ActionOperationIdPropertyName = "ActionOperationId";

        public static readonly UpgradeFailureAction DefaultUpgradeFailureAction = UpgradeFailureAction.Manual;

        // OrchestrationUpgrade Apis
        public static readonly string StartClusterConfigurationUpgradeErrorId = "StartClusterConfigurationUpgradeErrorId";
        public static readonly string GetClusterConfigurationUpgradeStatusErrorId = "GetClusterConfigurationUpgradeStatusErrorId";
        public static readonly string GetOrchestrationUpgradesPendingApprovalErrorId = "UpgradesPendingApprovalErrorId";
        public static readonly string StartOrchestrationApprovedUpgradesErrorId = "ApprovedUpgradesErrorId";
        public static readonly string GetClusterConfigurationErrorId = "GetClusterConfigurationErrorId";
        public static readonly string GetUpgradeOrchestrationServiceStateErrorId = "GetUpgradeOrchestrationServiceStateErrorId";

        public static readonly string UpgradeStatePropertyName = "UpgradeState";

        // Goal State Model
        public static readonly string GoalRuntimeVersionPropertyName = "GoalRuntimeVersion";
        public static readonly string GoalRuntimeLocationPropertyName = "GoalRuntimeLocation";
        public static readonly string RuntimePackagesPropertyName = "RuntimePackages";
        public static readonly string GetRuntimeVersionsErrorId = "GetRuntimeVersionsErrorId";
    }
}

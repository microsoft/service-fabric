// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.ClientSwitch
{
    public static class PowerShellConstants
    {
        public const string PowerShellAssembly = "Microsoft.Fabric.PowerShell.dll";
        public const string BinCachePath = "ServiceFabric_BuildDestinationCache";
        public const string PSModulePath = "PSModulePath";
        public const string ImportModuleCommand = "Import-Module";
        public const string FabricPsModuleName = "ServiceFabric";
        public const string FabricPsModulPathEnvExt = @"\..\ServiceFabric";
        public const string SendClusterHealthReportCommand = "Send-ServiceFabricClusterHealthReport";
        public const string SendNodeHealthReportCommand = "Send-ServiceFabricNodeHealthReport";
        public const string SendApplicationHealthReportCommand = "Send-ServiceFabricApplicationHealthReport";
        public const string SendDeployedApplicationHealthReportCommand = "Send-ServiceFabricDeployedApplicationHealthReport";
        public const string SendDeployedServicePackageHealthReportCommand = "Send-ServiceFabricDeployedServicePackageHealthReport";
        public const string SendServiceHealthReportCommand = "Send-ServiceFabricServiceHealthReport";
        public const string SendPartitionHealthReportCommand = "Send-ServiceFabricPartitionHealthReport";
        public const string SendReplicaHealthReportCommand = "Send-ServiceFabricReplicaHealthReport";

        public const string CreateFabricDumpSwitch = "CreateFabricDump";
        public const string IgnoreConstraintsParameter = "IgnoreConstraints";
        public const string AsyncSwitchParameter = "Async";
        public const string ShowProgressParameter = "ShowProgress";
        public const string StartPartitionDataLossCommand = "Start-ServiceFabricPartitionDataLoss";
        public const string GetPartitionDataLossProgressCommand = "Get-ServiceFabricPartitionDataLossProgress";
        public const string QuorumLossDurationInSeconds = "QuorumLossDurationInSeconds";
        public const string OperationId = "OperationId";
        public const string GetPartitionQuorumLossProgressCommand = "Get-ServiceFabricPartitionQuorumLossProgress";
        public const string TestCommandTypeFilter = "TypeFilter";
        public const string GetNodeTransitionProgressCommand = "Get-ServiceFabricNodeTransitionProgress";
        public const string StartPartitionQuorumLossCommand = "Start-ServiceFabricPartitionQuorumLoss";
        public const string StartPartitionRestartCommand = "Start-ServiceFabricPartitionRestart";
        public const string ForceCancel = "ForceCancel";
        public const string TestCommandStateFilter = "StateFilter";
        public const string GetTestCommandStatusListCommand = "Get-ServiceFabricTestCommandStatusList";
        public const string GetPartitionRestartProgressCommand = "Get-ServiceFabricPartitionRestartProgress";
        public const string StopTestCommandCommand = "Stop-ServiceFabricTestCommand";

        public const string ConnectClusterCommand = "Connect-ServiceFabricCluster";
        public const string GetNodeCommand = "Get-ServiceFabricNode";
        public const string GetApplicationCommand = "Get-ServiceFabricApplication";
        public const string GetDeployedApplicationCommand = "Get-ServiceFabricDeployedApplication";
        public const string GetDeployedCodePackageCommand = "Get-ServiceFabricDeployedCodePackage";
        public const string GetDeployedReplicaCommand = "Get-ServiceFabricDeployedReplica";
        public const string GetDeployedServiceManifestCommand = "Get-ServiceFabricDeployedServicePackage";
        public const string GetDeployedServiceTypeCommand = "Get-ServiceFabricDeployedServiceType";

        public const string GetNodeLoadInformationCommand = "Get-ServiceFabricNodeLoadInformation";
        public const string GetClusterLoadInformationCommand = "Get-ServiceFabricClusterLoadInformation";
        public const string GetReplicaLoadInformationCommand = "Get-ServiceFabricReplicaLoadInformation";
        public const string GetPartitionLoadInformationCommand = "Get-ServiceFabricPartitionLoadInformation";

        public const string GetApplicationTypeCommand = "Get-ServiceFabricApplicationType";
        public const string GetApplicationTypePagedCommand = "Get-ServiceFabricApplicationTypePaged";
        public const string GetClusterManifestCommand = "Get-ServiceFabricClusterManifest";
        public const string GetServiceManifestCommand = "Get-ServiceFabricServiceManifest";
        public const string InvokeEncryptSecretCommand = "Invoke-ServiceFabricEncryptSecret";
        public const string GetServiceCommand = "Get-ServiceFabricService";
        public const string GetPartitionCommand = "Get-ServiceFabricPartition";
        public const string GetReplicaCommand = "Get-ServiceFabricReplica";
        public const string RegisterApplicationTypeCommand = "Register-ServiceFabricApplicationType";
        public const string RegisterClusterPackageCommand = "Register-ServiceFabricClusterPackage";
        public const string UnregisterApplicationTypeCommand = "Unregister-ServiceFabricApplicationType";
        public const string UnregisterServiceFabricClusterPackageCommand = "Unregister-ServiceFabricClusterPackage";
        public const string NewApplicationCommand = "New-ServiceFabricApplication";
        public const string NewServiceFromTemplateCommand = "New-ServiceFabricServiceFromTemplate";
        public const string NewServiceCommand = "New-ServiceFabricService";
        public const string RemoveServiceCommand = "Remove-ServiceFabricService";
        public const string UpdateServiceCommand = "Update-ServiceFabricService";
        public const string RemoveApplicationCommand = "Remove-ServiceFabricApplication";
        public const string NewNodeConfigurationCommand = "New-ServiceFabricNodeConfiguration";
        public const string UpdateNodeConfigurationCommand = "Update-ServiceFabricNodeConfiguration";
        public const string RemoveNodeConfigurationCommand = "Remove-ServiceFabricNodeConfiguration";
        public const string TestClusterManifestCommand = "Test-ServiceFabricClusterManifest";
        public const string TestClusterConnectionCommand = "Test-ServiceFabricClusterConnection";
        public const string EnableServiceFabricNode = "Enable-ServiceFabricNode";
        public const string DisableServiceFabricNode = "Disable-ServiceFabricNode";
        public const string StartServiceFabricApplicationUpgradeCommand = "Start-ServiceFabricApplicationUpgrade";
        public const string ResumeServiceFabricApplicationUpgradeCommand = "Resume-ServiceFabricApplicationUpgrade";
        public const string UpdateServiceFabricApplicationUpgradeCommand = "Update-ServiceFabricApplicationUpgrade";

        public const string CopyServiceFabricApplicationPackageCommand = "Copy-ServiceFabricApplicationPackage";
        public const string CopyServiceFabricClusterPackageCommand = "Copy-ServiceFabricClusterPackage";
        public const string CopyServiceFabricServicePackageToNodeCommand = "Copy-ServiceFabricServicePackageToNode";
        public const string GetServiceFabricClusterHealthCommand = "Get-ServiceFabricClusterHealth";
        public const string GetServiceFabricClusterHealthChunkCommand = "Get-ServiceFabricClusterHealthChunk";
        public const string GetServiceFabricApplicationHealthCommand = "Get-ServiceFabricApplicationHealth";
        public const string GetServiceFabricApplicationManifestCommand = "Get-ServiceFabricApplicationManifest";
        public const string GetServiceFabricApplicationUpgradeCommand = "Get-ServiceFabricApplicationUpgrade";
        public const string GetServiceFabricApplicationLoadInformationCommand = "Get-ServiceFabricApplicationLoadInformation";
        public const string GetServiceFabricClusterUpgradeCommand = "Get-ServiceFabricClusterUpgrade";
        public const string GetServiceFabricNodeHealthCommand = "Get-ServiceFabricNodeHealth";
        public const string GetServiceFabricPartitionHealthCommand = "Get-ServiceFabricPartitionHealth";
        public const string GetServiceFabricReplicaHealthCommand = "Get-ServiceFabricReplicaHealth";
        public const string GetServiceFabricServiceHealthCommand = "Get-ServiceFabricServiceHealth";
        public const string GetServiceFabricDeployedApplicationHealthCommand = "Get-ServiceFabricDeployedApplicationHealth";
        public const string GetServiceFabricDeployedServicePackageHealthCommand = "Get-ServiceFabricDeployedServicePackageHealth";
        public const string RemoveServiceFabricNodeStateCommand = "Remove-ServiceFabricNodeState";
        public const string ResumeServiceFabricClusterUpgradeCommand = "Resume-ServiceFabricClusterUpgrade";
        public const string StartServiceFabricClusterUpgradeCommand = "Start-ServiceFabricClusterUpgrade";
        public const string TestServiceFabricApplicationPackageCommand = "Test-ServiceFabricApplicationPackage";
        public const string UpdateServiceFabricServiceGroupCommand = "Update-ServiceFabricServiceGroup";
        public const string MovePrimaryReplicaCommand = "Move-ServiceFabricPrimaryReplica";
        public const string MoveSecondaryReplicaCommand = "Move-ServiceFabricSecondaryReplica";
        public const string InvokeDataLossCommand = "Invoke-ServiceFabricPartitionDataLoss";

        public const string RestartNodeCommand = "Restart-ServiceFabricNode";
        public const string RestartPartitionCommand = "Restart-ServiceFabricPartition";
        public const string CleanTestStateCommand = "Remove-ServiceFabricTestState";
        public const string StopNodeCommand = "Stop-ServiceFabricNode";
        public const string StartNodeCommand = "Start-ServiceFabricNode";
        public const string StartNodeTransitionCommand = "Start-ServiceFabricNodeTransition";
        public const string RestartCodePackageCommand = "Restart-ServiceFabricDeployedCodePackage";
        public const string ValidateApplicationCommand = "Test-ServiceFabricApplication";
        public const string ValidateServiceCommand = "Test-ServiceFabricService";
        public const string RemoveReplicaCommand = "Remove-ServiceFabricReplica";
        public const string RestartReplicaCommand = "Restart-ServiceFabricReplica";

        public const string StartChaosCommand = "Start-ServiceFabricChaos";
        public const string TimeToRunMinuteParameter = "TimeToRunMinute";
        public const string MaxConcurrentFaultsParameter = "MaxConcurrentFaults";
        public const string MaxClusterStabilizationTimeoutSecParameter = "MaxClusterStabilizationTimeoutSec";
        public const string WaitTimeBetweenIterationsSecParameter = "WaitTimeBetweenIterationsSec";
        public const string WaitTimeBetweenFaultsSecParameter = "WaitTimeBetweenFaultsSec";
        public const string EnableMoveReplicaFaultsSwitchParameter = "EnableMoveReplicaFaults";
        public const string ContextParameter = "Context";
        public const string ClusterHealthPolicyParameter = "ClusterHealthPolicy";

        public const string StopChaosCommand = "Stop-ServiceFabricChaos";

        public const string GetImageStoreContentCommand = "Get-ServiceFabricImageStoreContent";
        public const string RemoveApplicationPackageCommand = "Remove-ServiceFabricApplicationPackage";

        public const string GetChaosReportCommand = "Get-ServiceFabricChaosReport";
        public const string StartTimeUtcParameter = "StartTimeUtc";
        public const string EndTimeUtcParameter = "EndTimeUtc";
        public const string ContinuationTokenParameter = "ContinuationToken";
        public const string ChaosScheduleDescriptionParameter = "ChaosScheduleDescription";

        public const string GetChaosCommand = "Get-ServiceFabricChaos";
        public const string GetChaosScheduleCommand = "Get-ServiceFabricChaosSchedule";
        public const string SetChaosScheduleCommand = "Set-ServiceFabricChaosSchedule";
        public const string GetChaosEventsCommand = "Get-ServiceFabricChaosEventsSegment";

        public const string StartServiceFabricRepairTaskCommand = "Start-ServiceFabricRepairTask";
        public const string NodeActionParameter = "NodeAction";
        public const string NodeNamesParameter = "NodeNames";
        public const string CustomActionParameter = "CustomAction";
        public const string NodeImpactParameter = "NodeImpact";
        public const string TaskIdParameter = "TaskId";


        public const string ConnectionEndpointParameter = "ConnectionEndpoint";
        public const string X509CredentialParameter = "X509Credential";
        public const string WindowsCredentialParameter = "WindowsCredential";
        public const string HealthReportSendIntervalInSec = "HealthReportSendIntervalInSec";
        public const string HealthReportRetrySendIntervalInSec = "HealthReportRetrySendIntervalInSec";
        public const string AADCredentialParameter = "AzureActiveDirectory";
        public const string FindTypeParameter = "FindType";
        public const string FindValueParameter = "FindValue";
        public const string ServerCommonNameParameter = "ServerCommonName";
        public const string ServerCertThumbprintParameter = "ServerCertThumbprint";
        public const string StoreLocationParameter = "StoreLocation";
        public const string StoreNameParameter = "StoreName";
        public const string SecurityToken = "SecurityToken";
        public const string NameParameter = "Name";
        public const string NodeNameParameter = "NodeName";
        public const string IPAddressOrFQDNParameter = "IPAddressOrFQDN";
        public const string ClusterConnectionPortParameter = "ClusterConnectionPort";
        public const string NodeInstanceIdParameter = "NodeInstanceId";
        public const string Intent = "Intent";
        public const string StopParameter = "Stop";
        public const string StartParameter = "Start";
        public const string MonitoredParameter = "Monitored";
        public const string UnmonitoredManualParameter = "UnmonitoredManual";
        public const string UnmonitoredAutoParameter = "UnmonitoredAuto";
        public const string ReplicaQuorumTimeoutSecParameter = "UpgradeReplicaSetCheckTimeoutSec";
        public const string RestartProcessSwitch = "ForceRestart";
        public const string ReplicaRestartWaitDurationParameter = "ReplicaRestartWaitDuration";
        public const string QuorumLossWaitDurationParameter = "QuorumLossWaitDuration";
        public const string ApplicationPackagePathParameter = "ApplicationPackagePath";
        public const string ApplicationPackagePathInImageStoreParameter = "ApplicationPackagePathInImageStore";
        public const string CompressPackageParameter = "CompressPackage";

        public const string CodePackagePathParameter = "CodePackagePath";
        public const string CodePackagePathInImageStoreParameter = "CodePackagePathInImageStore";
        public const string UpgradeModeParameter = "UpgradeMode";
        public const string NodeTransitionTypeParameter = "NodeTransitionType";
        public const string StopSwitch = "Stop";
        public const string StartSwitch = "Start";
        public const string StopDurationInSecondsParameter = "StopDurationInSeconds";
        public const string DescriptionParameter = "Description";
        public const string HealthPropertyParameter = "HealthProperty";
        public const string HealthStateParameter = "HealthState";
        public const string RemoveWhenExpiredParameter = "RemoveWhenExpired";
        public const string ImmediateParameter = "Immediate";
        public const string SequenceNumberParameter = "SequenceNumber";
        public const string SourceIdParameter = "SourceId";
        public const string TimeToLiveSecParameter = "TimeToLiveSec";

        public const string ApplicationNameParameter = "ApplicationName";
        public const string UpgradeDomainNameParameter = "UpgradeDomainName";
        public const string ServiceNameParameter = "ServiceName";
        public const string ServiceDnsNameParameter = "ServiceDnsName";
        public const string ServiceManifestNameParameter = "ServiceManifestName";
        public const string ServiceManifestTypeNameParameter = "ServiceTypeName";
        public const string PartitionIdParameter = "PartitionId";
        public const string ReplicaIdParameter = "ReplicaId";
        public const string InstanceIdParameter = "InstanceId";
        public const string ReplicaOrInstanceIdParameter = "ReplicaOrInstanceId";
        public const string ApplicationDefinitionKindFilterParameter = "ApplicationDefinitionKindFilter";
        public const string ApplicationPathParameter = "ApplicationPathInImageStore";
        public const string ApplicationTypeDefinitionKindFilterParameter = "ApplicationTypeDefinitionKindFilter";
        public const string ApplicationTypeNameParameter = "ApplicationTypeName";
        public const string ApplicationPackageDownloadUriParameter = "ApplicationPackageDownloadUri";
        public const string ApplicationPackageCleanupPolicyParameter = " ApplicationPackageCleanupPolicy";
        public const string ApplicationTypeVersionParameter = "ApplicationTypeVersion";
        public const string ServiceTypeNameParameter = "ServiceTypeName";
        public const string ServicePackageActivationModeParameter = "ServicePackageActivationMode";
        public const string ServicePackageActivationIdParameter = "ServicePackageActivationId";
        public const string PartitionSchemeNamedParameter = "PartitionSchemeNamed";
        public const string PartitionSchemeSingletonParameter = "PartitionSchemeSingleton";
        public const string PartitionSchemeUniformInt64Parameter = "PartitionSchemeUniformInt64";
        public const string PartitionNamesParameter = "PartitionNames";
        public const string PartitionCountParameter = "PartitionCount";
        public const string ApplicationParameter = "ApplicationParameter";
        public const string ForceParameter = "Force";
        public const string CodeParameter = "Code";
        public const string ConfigParameter = "Config";
        public const string InitializationDataParameter = "InitializationData";
        public const string HasPersistedStateParameter = "HasPersistedState";
        public const string PlacementConstraintsParameter = "PlacementConstraint";
        public const string CertificateThumbprintParameter = "CertThumbPrint";
        public const string CertificateStoreLocationParameter = "CertStoreLocation";
        public const string TextParameter = "Text";
        public const string ClusterManifestPathParameter = "ClusterManifestPath";
        public const string ClusterManifestPathInImageStoreParameter = "ClusterManifestPathInImageStore";
        public const string ComputerNameParameter = "ComputerName";
        public const string CodePackageVersionParameter = "CodePackageVersion";
        public const string ClusterManifestVersionParameter = "ClusterManifestVersion";
        public const string CodePackageNameParameter = "CodePackageName";
        public const string CodePackageInstanceIdParameter = "CodePackageInstanceId";
        public const string ImageStoreConnectionStringParameter = "ImageStoreConnectionString";
        public const string ServiceGroupNameParameter = "ServiceGroupName";

        public const string ConsiderWarningAsErrorParameter = "ConsiderWarningAsError";
        public const string MaxPercentUnhealthyDeployedApplicationsParameter = "MaxPercentUnhealthyDeployedApplications";
        public const string MaxPercentUnhealthyServicesParameter = "MaxPercentUnhealthyServices";
        public const string MaxPercentUnhealthyPartitionsPerServiceParameter = "MaxPercentUnhealthyPartitionsPerService";
        public const string MaxPercentUnhealthyReplicasPerPartitionParameter = "MaxPercentUnhealthyReplicasPerPartition";
        public const string MaxPercentUnhealthyApplicationsParameter = "MaxPercentUnhealthyApplications";
        public const string MaxPercentUnhealthyNodesParameter = "MaxPercentUnhealthyNodes";
        public const string ApplicationTypeHealthPolicyMapParameter = "ApplicationTypeHealthPolicyMap";
        public const string ApplicationHealthPolicyMapParameter = "ApplicationHealthPolicyMap";
        public const string FailureActionParameter = "FailureAction";
        public const string HealthCheckRetryTimeoutSecParameter = "HealthCheckRetryTimeoutSec";
        public const string HealthCheckStableDurationSecParameter = "HealthCheckStableDurationSec";
        public const string HealthCheckWaitDurationSecParameter = "HealthCheckWaitDurationSec";
        public const string UpgradeDomainTimeoutSecParameter = "UpgradeDomainTimeoutSec";
        public const string UpgradeTimeoutSecParameter = "UpgradeTimeoutSec";
        public const string DefaultServiceTypeHealthPolicyParameter = "DefaultServiceTypeHealthPolicy";
        public const string ServiceTypeHealthPolicyMapParameter = "ServiceTypeHealthPolicyMap";
        public const string EventsHealthStateFilter = "EventsHealthStateFilter";
        public const string ServicesHealthStateFilter = "ServicesHealthStateFilter";
        public const string PartitionsHealthStateFilter = "PartitionsHealthStateFilter";
        public const string ReplicasHealthStateFilter = "ReplicasHealthStateFilter";
        public const string DeployedApplicationsHealthStateFilter = "DeployedApplicationsHealthStateFilter";
        public const string DeployedServicePackagesHealthStateFilter = "DeployedServicePackagesHealthStateFilter";
        public const string ApplicationsHealthStateFilter = "ApplicationsHealthStateFilter";
        public const string NodesHealthStateFilter = "NodesHealthStateFilter";
        public const string NodeFilters = "NodeFilters";
        public const string ApplicationFilters = "ApplicationFilters";
        public const string ApplicationHealthPoliciesParameter = "ApplicationHealthPolicies";
        public const string IncludeHealthState = "IncludeHealthState";
        public const string ExcludeHealthStatistics = "ExcludeHealthStatistics";
        public const string IncludeSystemApplicationHealthStatistics = "IncludeSystemApplicationHealthStatistics";
        public const string GetSinglePage = "GetSinglePage";

        public const string MaxResultsParameter = "MaxResults";
        public const string ExcludeApplicationParametersParameter = "ExcludeApplicationParameters";
        public const string UsePagingParameter = "UsePaging";

        public const string EnableTestabilityParameter = "EnableTestability";

        public const string PackageSharingPoliciesParameter = "PackageSharingPolicies";
        public const string Id = "Id";
        public const string InstanceCount = "InstanceCount";
        public const string MinimumReplicaSetSize = "MinReplicaSetSize";
        public const string TargetReplicaSetSize = "TargetReplicaSetSize";
        public const string LastQuorumLossDuration = "LastQuorumLossDuration";
        public const string HighKey = "HighKey";
        public const string LowKey = "LowKey";
        public const string Name = "Name";
        public const string Kind = "Kind";
        public const string Stateful = "Stateful";
        public const string Stateless = "Stateless";
        public const string TimeoutSec = "TimeoutSec";
        public const string MaxStabilizationTimeoutSec = "MaxStabilizationTimeoutSec";

        public const string PlacementPolicy = "PlacementPolicy";
        public const string Correlation = "Correlation";
        public const string Metric = "Metric";
        public const string AlignedAffinity = "AlignedAffinity";
        public const string OnEveryNode = "OnEveryNode";
        public const string ScalingPolicies = "ScalingPolicies";

        public const string PartitionHighKey = "PartitionHighKey";
        public const string PartitionLowKey = "PartitionLowKey";
        public const string PartitionName = "PartitionName";
        public const string PartitionId = "PartitionId";
        public const string PartitionKey = "PartitionKey";
        public const string PartitionKind = "PartitionKind";
        public const string PartitionStatus = "PartitionStatus";
        public const string CurrentSecondaryNodeName = "CurrentSecondaryNodeName";
        public const string NewSecondaryNodeName = "NewSecondaryNodeName";
        public const string DataLossNumber = "DataLossNumber";
        public const string ConfigurationNumber = "ConfigurationNumber";


        public const string CommandCompletionMode = "CommandCompletionMode";
        public const string PartitionKindSingleton = "PartitionKindSingleton";
        public const string PartitionKindUniformInt64 = "PartitionKindUniformInt64";
        public const string PartitionKindNamed = "PartitionKindNamed";

        public const string ReplicaKind = "ReplicaKind";
        public const string ReplicaKindPrimary = "ReplicaKindPrimary";
        public const string ReplicaKindRandomSecondary = "ReplicaKindRandomSecondary";
        public const string RestartPartitionMode = "RestartPartitionMode";

        public const string InvokeDataLossMode = "DataLossMode";
        public const string InvokeQuorumLossCommand = "Invoke-ServiceFabricPartitionQuorumLoss";
        public const string InvokeQuorumLossMode = "QuorumLossMode";
        public const string InvokeQuorumLossDuration = "QuorumLossDuration";

        public const string ForceRemoveParameter = "ForceRemove";

        public const string RemoteRelativePathParameter = "RemoteRelativePath";

        public const int NumberOfRequestsInOperation = 10;

        public const string GetPagedResultsParameter = "GetPagedResults";
    }
}


#pragma warning restore 1591
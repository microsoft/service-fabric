// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    internal static class FabricValidatorConstants
    {
        public const string FabricUpgradeDefaultInstanceId = "0";
        public const string FabricUpgradeDefaultTargetVersion = "0";
        public const string SQLVoteType = "SqlServer";
        public const string FabricImageStoreConnectionStringPrefix = "fabric:";
        public const string XStoreImageStoreConnectionStringPrefix = "xstore:";
        public const string FileImageStoreConnectionStringPrefix = "file:";
        public const string LocalHostAddressPrefix = "localhost";
        public const string DefaultTag = "_default_";
        public const string DefaultTableStoreTableName = "fabriclogs";
        public const string DefaultFileStoreTraceLocation = "fabriclogs";
        public const string DefaultFileStoreCrashDumpLocation = "fabriccrashdumps";
        public const string DCAService = "DCA";
        public const string NodeType = "NodeType";
        public const string NodeName = "NodeName";
        public const string UpgradeDomain = "UpgradeDomain";
        public const string FaultDomain = "FaultDomain";
        public const string Certificates = "Certificates";
        public const string ClusterCertificate = "ClusterCertificate";
        public const string ClientCertificate = "ClientCertificate";
        public const string ServerCertificate = "ServerCertificate";
        public const string Endpoints = "Endpoints";
        public const string ApplicationEndpoints = "ApplicationEndpoints";
        public const string CredentialTypeNone = "None";
        public const string CredentialTypeX509 = "X509";
        public const string CredentialTypeWindows = "Windows";
        public const string RunAsAccountTypeDomainUser = "DomainUser";
        public const string RunAsAccountTypeManagedService = "ManagedServiceAccount";
        public const string RunAsAccountTypeNetworkService = "NetworkService";
        public const string RunAsAccountTypeLocalService = "LocalService";
        public const string RunAsAccountTypeLocalSystem = "LocalSystem";
#if DotNetCoreClr
        public const string ConfigurationsFileName = "System.Fabric.Management.Configurations.csv";
#else
        public const string ConfigurationsFileName = "Configurations.csv";
#endif
        public const string WindowsUpdateServiceCoordinatorType = "WindowsUpdateService";
        public const string WindowsAutoBaseUpgradeCoordinatorType = "WindowsAutoBaseUpgrade";
        public const string ServerRestartCoordinatorType = "ServerRestart";
        public const string Provider_DSTS = "DSTS";
        public const string HttpAppGatewayPolicyNone = "None";
        public const string HttpAppGatewayServiceCommonNameAndIssuer= "ServiceCommonNameAndIssuer";

        internal static class ParameterNames
        {
            public const string IsEnabled = "IsEnabled";
            public const string StoreConnectionString = "StoreConnectionString";
            public const string LogContainerName = "LogContainerName";
            public const string CrashDumpContainerName = "CrashDumpContainerName";
            public const string TableName = "TableName";
            public const string LocalLogDeletionEnabled = "LocalLogDeletionEnabled";
            public const string LogDeletionAgeInDays = "LogDeletionAgeInDays";
            public const string UploadIntervalInMinutes = "UploadIntervalInMinutes";
            public const string LogFilter = "LogFilter";
            public const string UploadConcurrencyCount = "UploadConcurrencyCount";
            public const string ProducerInstances = "ProducerInstances";
            public const string ImageStoreConnectionString = "ImageStoreConnectionString";
            public const string AllowImageStoreConnectionStringChange = "AllowImageStoreConnectionStringChange";
            public const string EnableClusterManagerAffinity = "EnableClusterManagerAffinity";
            public const string ConsumerInstances = "ConsumerInstances";
            public const string ConsumerType = "ConsumerType";
            public const string ProducerInstance = "ProducerInstance";
            public const string ProducerType = "ProducerType";
            public const string PluginAssembly = "Assembly";
            public const string PluginType = "Type";
            public const string PluginValidationAssembly = "ValidationAssembly";
            public const string PluginValidationType = "ValidationType";
            public const string ExpectedClusterSize = "ExpectedClusterSize";
            public const string ClusterCredentialType = "ClusterCredentialType";
            public const string ServerAuthCredentialType = "ServerAuthCredentialType";
            public const string ClientAuthAllowedCommonNames = "ClientAuthAllowedCommonNames";
            public const string ClientCertThumbprints = "ClientCertThumbprints";
            public const string ClientRoleEnabled = "ClientRoleEnabled";
            public const string AdminClientCertThumbprints = "AdminClientCertThumbprints";
            public const string AdminClientCommonNames = "AdminClientCommonNames";
            public const string AdminClientIdentities = "AdminClientIdentities";
            public const string ClusterSpn = "ClusterSpn";
            public const string IsAppLogCollectionEnabled = "IsAppLogCollectionEnabled";
            public const string AppLogDirectoryQuotaInMB = "AppLogDirectoryQuotaInMB";
            public const string NTLMAuthenticationPasswordSecret = "NTLMAuthenticationPasswordSecret";
            public const string NTLMAuthenticationEnabled = "NTLMAuthenticationEnabled";
            public const string RunAsPolicyEnabled = "RunAsPolicyEnabled";
            public const string EndpointProviderEnabled = "EndpointProviderEnabled";
            public const string ActivationMaxFailureCount = "ActivationMaxFailureCount";
            public const string ActivatorServiceAddress = "ActivatorServiceAddress";
            public const string NewCounterBinaryFileCreationIntervalMinutes = "NewCounterBinaryFileCreationIntervalInMinutes";
            public const string Counters = "Counters";
            public const string MaxCounterBinaryFileSizeInMB = "MaxCounterBinaryFileSizeInMB";
            public const string TestOnlyCounterFilePath = "TestOnlyCounterFilePath";
            public const string TestOnlyCounterFileNamePrefix = "TestOnlyCounterFileNamePrefix";
            public const string TestOnlyIncludeMachineNameInCounterFileName = "TestOnlyIncludeMachineNameInCounterFileName";
            public const string ApplicationEtwTraceLevel = "ApplicationEtwTraceLevel";
            public const string LeaseAgentAddress = "LeaseAgentAddress";
            public const string ClientConnectionAddress = "ClientConnectionAddress";
            public const string HttpGatewayListenAddress = "HttpGatewayListenAddress";
            public const string HttpGatewayProtocol = "HttpGatewayProtocol";
            public const string HttpApplicationGatewayListenAddress = "HttpApplicationGatewayListenAddress";
            public const string HttpApplicationGatewayProtocol = "HttpApplicationGatewayProtocol";
            public const string ClusterManagerReplicatorAddress = "ClusterManagerReplicatorAddress";
            public const string RepairManagerReplicatorAddress = "RepairManagerReplicatorAddress";
            public const string ImageStoreServiceReplicatorAddress = "ImageStoreServiceReplicatorAddress";
            public const string UpgradeServiceReplicatorAddress = "UpgradeServiceReplicatorAddress";
            public const string FaultAnalysisServiceReplicatorAddress = "FaultAnalysisServiceReplicatorAddress";
            public const string BackupRestoreServiceReplicatorAddress = "BackupRestoreServiceReplicatorAddress";
            public const string CentralSecretServiceReplicatorAddress = "CentralSecretServiceReplicatorAddress";
            public const string UpgradeOrchestrationServiceReplicatorAddress = "UpgradeOrchestrationServiceReplicatorAddress";
            public const string GatewayResourceManagerReplicatorAddress = "GatewayResourceManagerReplicatorAddress";
            public const string NamingReplicatorAddress = "NamingReplicatorAddress";
            public const string FailoverManagerReplicatorAddress = "FailoverManagerReplicatorAddress";
            public const string EventStoreServiceReplicatorAddress = "EventStoreServiceReplicatorAddress";
            public const string RuntimeServiceAddress = "RuntimeServiceAddress";
            public const string StartApplicationPortRange = "StartApplicationPortRange";
            public const string EndApplicationPortRange = "EndApplicationPortRange";
            public const string StartDynamicPortRange = "StartDynamicPortRange";
            public const string EndDynamicPortRange = "EndDynamicPortRange";
            public const string NodeAddress = "NodeAddress";
            public const string RunAsAccountName = "RunAsAccountName";
            public const string RunAsAccountType = "RunAsAccountType";
            public const string RunAsPassword = "RunAsPassword";
            public const string MaxPercentUnhealthyNodes = "MaxPercentUnhealthyNodes";
            public const string MaxPercentUnhealthyApplications = "MaxPercentUnhealthyApplications";
            public const string DSTSDnsName = "DSTSDnsName";
            public const string DSTSRealm = "DSTSRealm";
            public const string DSTSWinFabServiceDnsName = "CloudServiceDnsName";
            public const string DSTSWinFabServiceName = "CloudServiceName";
            public const string HttpAppGatewayCredentialType = "GatewayAuthCredentialType";
            public const string HttpAppGatewayCertificateFindValue = "GatewayX509CertificateFindValue";
            public const string HttpAppGatewayCertificateValidationPolicy = "ApplicationCertificateValidationPolicy";
            public const string HttpAppGatewayServiceCertificateThumbprints = "ServiceCertificateThumbprints";
            public const string PublicCertificateFindValue = "PublicCertificateFindValue";
            public const string PublicCertificateFindType = "PublicCertificateFindType";
            public const string UseV2NodeIdGenerator = "UseV2NodeIdGenerator";
            public const string NodeIdGeneratorVersion = "NodeIdGeneratorVersion";
            public const string ClientClaimAuthEnabled = "ClientClaimAuthEnabled";
            public const string ClientClaims = "ClientClaims";
            public const string AdminClientClaims = "AdminClientClaims";
            public const string Enabled = "Enabled";
            public const string TargetReplicaSetSize = "TargetReplicaSetSize";
            public const string MinReplicaSetSize = "MinReplicaSetSize";
            public const string MaxUserStandByReplicaCount = "MaxUserStandByReplicaCount";
            public const string MaxSystemStandByReplicaCount = "MaxSystemStandByReplicaCount";
            public const string PartitionCount = "PartitionCount";
            public const string MessageContentBufferRatio = "MessageContentBufferRatio";
            public const string DeploymentDirectory = "DeploymentDirectory";
            public const string PlacementConstraints = "PlacementConstraints";
            public const string AzureStorageMaxWorkerThreads = "AzureStorageMaxWorkerThreads";
            public const string AzureStorageMaxConnections = "AzureStorageMaxConnections";
            public const string ApplicationLogsFormatVersion = "ApplicationLogsFormatVersion";
            public const string DisableFirewallRuleForPublicProfile = "DisableFirewallRuleForPublicProfile";
            public const string DisableFirewallRuleForPrivateProfile = "DisableFirewallRuleForPrivateProfile";
            public const string DisableFirewallRuleForDomainProfile = "DisableFirewallRuleForDomainProfile";
            public const string ApplicationTypeMaxPercentUnhealthyApplicationsPrefix = "ApplicationTypeMaxPercentUnhealthyApplications-";
            public const string CoordinatorType = "CoordinatorType";
            public const string EnableServiceFabricAutomaticUpdates = "EnableServiceFabricAutomaticUpdates";
            public const string EnableServiceFabricBaseUpgrade = "EnableServiceFabricBaseUpgrade";
            public const string OnlyBaseUpgrade = "OnlyBaseUpgrade";
            public const string EnableRestartManagement = "EnableRestartManagement";
            public const string ReplicaRestartWaitDuration = "ReplicaRestartWaitDuration";
            public const string FullRebuildWaitDuration = "FullRebuildWaitDuration";
            public const string StandByReplicaKeepDuration = "StandByReplicaKeepDuration";
            public const string Providers = "Providers";
            public const string DataDeletionAgeInDays = "DataDeletionAgeInDays";
            public const string DeploymentId = "DeploymentId";
            public const string TableNamePrefix = "TableNamePrefix";
            public const string EtlReadIntervalInMinutes = "EtlReadIntervalInMinutes";

            internal static class Common
            {
                public const string EnableUnsupportedPreviewFeatures = "EnableUnsupportedPreviewFeatures";
#if DotNetCoreClrLinux
                // TODO - Following code will be removed once fully transitioned to structured traces in Linux
                public const string LinuxStructuredTracesEnabled = "LinuxStructuredTracesEnabled";
#endif
            }

            internal static class DNSService
            {
                public const string PartitionPrefix = "PartitionPrefix";
                public const string PartitionSuffix = "PartitionSuffix";
            }

            internal static class Replicator
            {
                public const string RetryInterval = "RetryInterval";
                public const string BatchAckInterval = "BatchAcknowledgementInterval";
                public const string InitialReplicationQueueSize = "InitialReplicationQueueSize";
                public const string MaxReplicationQueueSize = "MaxReplicationQueueSize";
                public const string InitialCopyQueueSize = "InitialCopyQueueSize";
                public const string MaxCopyQueueSize = "MaxCopyQueueSize";
                public const string MaxReplicationQueueMemorySize = "MaxReplicationQueueMemorySize";
                public const string MaxReplicationMessageSize = "MaxReplicationMessageSize";
                public const string InitialSecondaryReplicationQueueSize = "InitialSecondaryReplicationQueueSize";
                public const string MaxSecondaryReplicationQueueSize = "MaxSecondaryReplicationQueueSize";
                public const string MaxSecondaryReplicationQueueMemorySize = "MaxSecondaryReplicationQueueMemorySize";
                public const string InitialPrimaryReplicationQueueSize = "InitialPrimaryReplicationQueueSize";
                public const string MaxPrimaryReplicationQueueSize = "MaxPrimaryReplicationQueueSize";
                public const string MaxPrimaryReplicationQueueMemorySize = "MaxPrimaryReplicationQueueMemorySize";
                public const string QueueHealthWarningAtUsagePercent = "QueueHealthWarningAtUsagePercent";
                public const string SlowIdleRestartAtQueueUsagePercent = "SlowIdleRestartAtQueueUsagePercent";
                public const string SlowActiveSecondaryRestartAtQueueUsagePercent = "SlowActiveSecondaryRestartAtQueueUsagePercent";
                public const string SecondaryProgressRateDecayFactor = "SecondaryProgressRateDecayFactor";
            }

            internal static class TransactionalReplicatorCommon
            {
                public const string MaxStreamSizeInMB = "MaxStreamSizeInMB";
                public const string MaxRecordSizeInKB = "MaxRecordSizeInKB";
                public const string MaxMetadataSizeInKB = "MaxMetadataSizeInKB";
                public const string CheckpointThresholdInMB = "CheckpointThresholdInMB";
                public const string MaxAccumulatedBackupLogSizeInMB = "MaxAccumulatedBackupLogSizeInMB";
                public const string OptimizeForLocalSSD = "OptimizeForLocalSSD";
                public const string OptimizeLogForLowerDiskUsage = "OptimizeLogForLowerDiskUsage";
                public const string MaxWriteQueueDepthInKB = "MaxWriteQueueDepthInKB";
                public const string SlowApiMonitoringDuration = "SlowApiMonitoringDuration";
                public const string MinLogSizeInMB = "MinLogSizeInMB";
                public const string TruncationThresholdFactor = "TruncationThresholdFactor";
                public const string ThrottlingThresholdFactor = "ThrottlingThresholdFactor";
                public const string Test_LoggingEngine = "Test_LoggingEngine";
                public const string Test_LogMinDelayIntervalMilliseconds = "Test_LogMinDelayIntervalMilliseconds";
                public const string Test_LogMaxDelayIntervalMilliseconds = "Test_LogMaxDelayIntervalMilliseconds";
                public const string Test_LogDelayRatio = "Test_LogDelayRatio";
                public const string Test_LogDelayProcessExitRatio = "Test_LogDelayProcessExitRatio";
                public const string EnableIncrementalBackupsAcrossReplicas = "EnableIncrementalBackupsAcrossReplicas";
                public const string LogTruncationIntervalSeconds = "LogTruncationIntervalSeconds";
            }

            internal static class TransactionalReplicator
            {
                public const string SharedLogId = "SharedLogId";
                public const string SharedLogPath = "SharedLogPath";
                public const string DefaultSharedLogContainerGuid = "{3CA2CCDA-DD0F-49c8-A741-62AAC0D4EB62}";
            }

            internal static class TransactionalReplicator2
            {
                public const string CopyBatchSizeInKb = "CopyBatchSizeInKb";
                public const string ReadAheadCacheSizeInKb = "ReadAheadCacheSizeInKb";
                public const string SerializationVersion = "SerializationVersion";
            }

            internal static class NativeRunConfiguration
            {
                public const string EnableNativeReliableStateManager = "EnableNativeReliableStateManager";
                public const string Test_LoadEnableNativeReliableStateManager = "Test_LoadEnableNativeReliableStateManager";
            }

            internal static class ReliableStateProvider
            {
                // Sweep threshold sub-section name.
                public const string SweepThreshold = "SweepThreshold";
                public const string EnableStrict2PL = "EnableStrict2PL";
            }

            internal static class PLB
            {
                public const string YieldDurationPer10ms = "YieldDurationPer10ms";
                public const string PLBRefreshInterval = "PLBRefreshInterval";
                public const string MinLoadBalancingInterval = "MinLoadBalancingInterval";
                public const string SwapPrimaryProbability = "SwapPrimaryProbability";
                public const string LoadDecayFactor = "LoadDecayFactor";
                public const string AffinityMoveParentReplicaProbability = "AffinityMoveParentReplicaProbability";
                public const string CapacityConstraintPriority = "CapacityConstraintPriority";
                public const string AffinityConstraintPriority = "AffinityConstraintPriority";
                public const string FaultDomainConstraintPriority = "FaultDomainConstraintPriority";
                public const string UpgradeDomainConstraintPriority = "UpgradeDomainConstraintPriority";
                public const string LocalBalancingThreshold = "LocalBalancingThreshold";
                public const string LocalDomainWeight = "LocalDomainWeight";
                public const string MinPlacementInterval = "MinPlacementInterval";
                public const string MinConstraintCheckInterval = "MinConstraintCheckInterval";
                public const string PLBRefreshGap = "PLBRefreshGap";
                public const string BalancingDelayAfterNewNode = "BalancingDelayAfterNewNode";
                public const string BalancingDelayAfterNodeDown = "BalancingDelayAfterNodeDown";
                public const string GlobalMovementThrottleCountingInterval = "GlobalMovementThrottleCountingInterval";
                public const string MovementPerPartitionThrottleCountingInterval = "MovementPerPartitionThrottleCountingInterval";
                public const string PlacementSearchTimeout = "PlacementSearchTimeout";
                public const string ConstraintCheckSearchTimeout = "ConstraintCheckSearchTimeout";
                public const string FastBalancingSearchTimeout = "FastBalancingSearchTimeout";
                public const string SlowBalancingSearchTimeout = "SlowBalancingSearchTimeout";
                public const string UseScoreInConstraintCheck = "UseScoreInConstraintCheck";
                public const string MaxPercentageToMove = "MaxPercentageToMove";
                public const string MaxPercentageToMoveForPlacement = "MaxPercentageToMoveForPlacement";
                public const string PlacementConstraintPriority = "PlacementConstraintPriority";
                public const string PreferredLocationConstraintPriority = "PreferredLocationConstraintPriority";
                public const string ScaleoutCountConstraintPriority = "ScaleoutCountConstraintPriority";
                public const string ApplicationCapacityConstraintPriority = "ApplicationCapacityConstraintPriority";
                public const string MoveParentToFixAffinityViolation = "MoveParentToFixAffinityViolation";
                public const string MoveParentToFixAffinityViolationTransitionPercentage = "MoveParentToFixAffinityViolationTransitionPercentage";
                public const string StatisticsTracingInterval = "StatisticsTracingInterval";
                public const string QuorumBasedLogicAutoSwitch = "QuorumBasedLogicAutoSwitch";
                public const string ThrottlingConstraintPriority = "ThrottlingConstraintPriority";
            }

            internal static class FileStoreService
            {
                public const string PrimaryAccountType = "PrimaryAccountType";
                public const string PrimaryAccountUserName = "PrimaryAccountUserName";
                public const string PrimaryAccountUserPassword = "PrimaryAccountUserPassword";
                public const string PrimaryAccountNTLMPasswordSecret = "PrimaryAccountNTLMPasswordSecret";
                public const string SecondaryAccountType = "SecondaryAccountType";
                public const string SecondaryAccountUserName = "SecondaryAccountUserName";
                public const string SecondaryAccountUserPassword = "SecondaryAccountUserPassword";
                public const string SecondaryAccountNTLMPasswordSecret = "SecondaryAccountNTLMPasswordSecret";
                public const string AnonymousAccessEnabled = "AnonymousAccessEnabled";
                public const string CommonNameNtlmPasswordSecret = "CommonNameNtlmPasswordSecret";
                public const string CommonName1Ntlmx509CommonName = "CommonName1Ntlmx509CommonName";
                public const string CommonName2Ntlmx509CommonName = "CommonName2Ntlmx509CommonName";
            }

            internal static class KtlLogger
            {
                public const string PeriodicFlushTime = "PeriodicFlushTime";
                public const string PeriodicTimerInterval = "PeriodicTimerInterval";
                public const string AllocationTimeout = "AllocationTimeout";
                public const string WriteBufferMemoryPoolMinimumInKB = "WriteBufferMemoryPoolMinimumInKB";
                public const string WriteBufferMemoryPoolMaximumInKB = "WriteBufferMemoryPoolMaximumInKB";
                public const string WriteBufferMemoryPoolPerStreamInKB = "WriteBufferMemoryPoolPerStreamInKB";
                public const string PinnedMemoryLimitInKB = "PinnedMemoryLimitInKB";
                public const string SharedLogId = "SharedLogId";
                public const string SharedLogPath = "SharedLogPath";
                public const string SharedLogSizeInMB = "SharedLogSizeInMB";
                public const string SharedLogNumberStreams = "SharedLogNumberStreams";
                public const string SharedLogMaximumRecordSizeInKB = "SharedLogMaximumRecordSizeInKB";
                public const string SharedLogCreateFlags = "SharedLogCreateFlags";
            }

            internal static class Hosting
            {
                public const string ActivationTimeout = "ActivationTimeout";
                public const string ApplicationUpgradeTimeout = "ApplicationUpgradeTimeout";
                public const string IsSFVolumeDiskServiceEnabled = "IsSFVolumeDiskServiceEnabled";
                public const string IPProviderEnabled = "IPProviderEnabled";
                public const string LocalNatIPProviderEnabled = "LocalNatIPProviderEnabled";
            }

            internal static class UpgradeService
            {
                public const string ContinuousFailureWarningThreshold = "ContinuousFailureWarningThreshold";
                public const string ContinuousFailureErrorThreshold = "ContinuousFailureErrorThreshold";
                public const string ContinuousFailureFaultThreshold = "ContinuousFailureFaultThreshold";
            }

            internal static class ReconfigurationAgent
            {
                public const string ServiceApiHealthDuration = "ServiceApiHealthDuration";
                public const string ServiceReconfigurationApiHealthDuration = "ServiceReconfigurationApiHealthDuration";
                public const string GracefulReplicaShutdownMaxDuration = "GracefulReplicaShutdownMaxDuration";
                public const string ReplicaChangeRoleFailureRestartThreshold = "ReplicaChangeRoleFailureRestartThreshold";
                public const string ReplicaChangeRoleFailureWarningReportThreshold = "ReplicaChangeRoleFailureWarningReportThreshold";
                public const string ApplicationUpgradeMaxReplicaCloseDuration = "ApplicationUpgradeMaxReplicaCloseDuration";
                public const string PeriodicApiSlowTraceInterval = "PeriodicApiSlowTraceInterval";
                public const string NodeDeactivationMaxReplicaCloseDuration = "NodeDeactivationMaxReplicaCloseDuration";
                public const string FabricUpgradeMaxReplicaCloseDuration = "FabricUpgradeMaxReplicaCloseDuration";
                public const string IsDeactivationInfoEnabled = "IsDeactivationInfoEnabled";
            }

            internal static class CentralSecretService {
                public const string IsEnabled = "IsEnabled";
                public const string EncryptionCertificateThumbprint = "EncryptionCertificateThumbprint";
            }

            internal static class NetworkInventoryManager {
                public const string IsEnabled = "IsEnabled";
            }

            internal static class Setup {
                public const string ContainerNetworkSetup = "ContainerNetworkSetup";
                public const string IsolatedNetworkSetup = "IsolatedNetworkSetup";
            }
        }

        internal static class SectionNames
        {
            public const string Votes = "Votes";
            public const string FabricTestQueryWeights = "FabricTestQueryWeights";
            public const string FailoverManager = "FailoverManager";
            public const string FabricHost = "FabricHost";
            public const string FabricNode = "FabricNode";
            public const string Federation = "Federation";
            public const string NodeProperties = "NodeProperties";
            public const string NodeCapacities = "NodeCapacities";
            public const string PlacementProperties = "PlacementProperties";
            public const string NodeDomainIds = "NodeDomainIds";
            public const string Trace = "Trace";
            public const string NamingService = "NamingService";
            public const string ClusterManager = "ClusterManager";
            public const string DnsService = "DnsService";
            public const string ImageStoreService = "ImageStoreService";
            public const string FileStoreService = "FileStoreService";
            public const string Management = "Management";
            public const string RepairManager = "RepairManager";
            public const string HealthManager_ClusterHealthPolicy = "HealthManager/ClusterHealthPolicy";
            public const string Hosting = "Hosting";
            public const string LocalLogStore = "LocalLogStore";
            public const string DiagnosticFileStore = "DiagnosticFileStore";
            public const string DiagnosticTableStore = "DiagnosticTableStore";
            public const string Diagnostics = "Diagnostics";
            public const string Security = "Security";
            public const string UnreliableTransport = "UnreliableTransport";
            public const string MetricActivityThresholds = "MetricActivityThresholds";
            public const string MetricBalancingThresholds = "MetricBalancingThresholds";
            public const string GlobalMetricWeights = "GlobalMetricWeights";
            public const string DefragmentationMetrics = "DefragmentationMetrics";
            public const string DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold = "DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold";
            public const string DefragmentationEmptyNodeDistributionPolicy = "DefragmentationEmptyNodeDistributionPolicy";
            public const string PlacementHeuristicIncomingLoadFactor = "PlacementHeuristicIncomingLoadFactor";
            public const string PlacementHeuristicEmptySpacePercent = "PlacementHeuristicEmptySpacePercent";
            public const string NodeBufferPercentage = "NodeBufferPercentage";
            public const string DefragmentationScopedAlgorithmEnabled = "DefragmentationScopedAlgorithmEnabled";
            public const string DefragmentationEmptyNodeWeight = "DefragmentationEmptyNodeWeight";
            public const string DefragmentationNonEmptyNodeWeight = "DefragmentationNonEmptyNodeWeight";
            public const string PlacementStrategy = "PlacementStrategy";
            public const string MetricEmptyNodeThresholds = "MetricEmptyNodeThresholds";
            public const string ReservedLoadPerNode = "ReservedLoadPerNode";
            public const string BalancingByPercentage = "BalancingByPercentage";
            public const string MaximumInBuildReplicasPerNode = "MaximumInBuildReplicasPerNode";
            public const string MaximumInBuildReplicasPerNodeBalancingThrottle = "MaximumInBuildReplicasPerNodeBalancingThrottle";
            public const string MaximumInBuildReplicasPerNodeConstraintCheckThrottle = "MaximumInBuildReplicasPerNodeConstraintCheckThrottle";
            public const string MaximumInBuildReplicasPerNodePlacementThrottle = "MaximumInBuildReplicasPerNodePlacementThrottle";
            public const string PlacementAndLoadBalancing = "PlacementAndLoadBalancing";
            public const string PerformanceCounterLocalStore = "PerformanceCounterLocalStore";
            public const string HttpGateway = "HttpGateway";
            public const string HttpAppGateway = "ApplicationGateway/Http";
            public const string RunAs = "RunAs";
            public const string RunAs_DCA = "RunAs_DCA";
            public const string RunAs_HttpGateway = "RunAs_HttpGateway";
            public const string RunAs_Fabric = "RunAs_Fabric";
            public const string InfrastructureService = "InfrastructureService";
            public const string DSTSTokenValidationService = "DSTSTokenValidationService";
            public const string TokenValidationService = "TokenValidationService";
            public const string TransactionalReplicator = "TransactionalReplicator";
            public const string ReliableStateProvider = "ReliableStateProvider";
            public const string ReconfigurationAgent = "ReconfigurationAgent";
            public const string ClusterX509Names = "Security/ClusterX509Names";
            public const string ServerX509Names = "Security/ServerX509Names";
            public const string ClientX509Names = "Security/ClientX509Names";
            public const string AdminClientX509Names = "Security/AdminClientX509Names";
            public const string ClusterCertificateIssuerStores = "Security/ClusterCertificateIssuerStores";
            public const string ServerCertificateIssuerStores = "Security/ServerCertificateIssuerStores";
            public const string ClientCertificateIssuerStores = "Security/ClientCertificateIssuerStores";
            public const string ServiceCommonNameAndIssuer = "ApplicationGateway/Http/ServiceCommonNameAndIssuer";
            public const string UpgradeService = "UpgradeService";
            public const string BackupRestoreService = "BackupRestoreService";
            public const string KtlLogger = "KtlLogger";
            public const string Setup = "Setup";
            public const string NativeRunConfiguration = "NativeRunConfiguration";
            public const string Common = "Common";
            public const string CentralSecretService = "CentralSecretService";
            public const string NetworkInventoryManager = "NetworkInventoryManager";

            internal static class Replication
            {
                public const string Default = "Replication";
                public const string ClusterManager = "ClusterManager/Replication";
                public const string FileStoreService = "FileStoreService/Replication";
                public const string Naming = "Naming/Replication";
                public const string Failover = "Failover/Replication";
                public const string RepairManager = "RepairManager/Replication";
                public const string TransactionalReplicator = "TransactionalReplicator";
            }

            internal static class TransactionalReplicator2
            {
                public const string Default = "TransactionalReplicator2";
                public const string ClusterManager = "ClusterManager/TransactionalReplicator2";
                public const string FileStoreService = "FileStoreService/TransactionalReplicator2";
                public const string Naming = "Naming/TransactionalReplicator2";
                public const string Failover = "Failover/TransactionalReplicator2";
                public const string RepairManager = "RepairManager/TransactionalReplicator2";
            }
        }

        internal static class DefaultValues
        {
            public const string CommonTimeSpanFromSeconds = "Common::TimeSpan::FromSeconds";
            public const string TimeSpanFromSeconds = "TimeSpan::FromSeconds";

            public const string CommonTimeSpanFromMinutes = "Common::TimeSpan::FromMinutes";
            public const string TimeSpanFromMinutes = "TimeSpan::FromMinutes";

            public const string CommonTimeSpanFromHours = "Common::TimeSpan::FromHours";
            public const string TimeSpanFromHours = "TimeSpan::FromHours";

            public const string CommonTimeSpanFromTicks = "Common::TimeSpan::FromTicks";
            public const string TimeSpanFromTicks = "TimeSpan::FromTicks";

            public const string CommonTimeSpanFromMilliseconds = "Common::TimeSpan::FromMilliseconds";
            public const string TimeSpanFromMilliseconds = "TimeSpan::FromMilliseconds";

            public const string TimeSpanZero = "TimeSpan::Zero";
            public const string CommonTimeSpanZero = "Common::TimeSpan::Zero";

            public const string TimeSpanMaxValue = "TimeSpan::MaxValue";
            public const string CommonTimeSpanMaxValue = "Common::TimeSpan::MaxValue";

            public const string TimeSpanMinValue = "TimeSpan::MinValue";
            public const string CommonTimeSpanMinValue = "Common::TimeSpan::MinValue";
        }

    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    public class StringConstants
    {
        public static readonly string ServiceType = "WRPServiceType";
        public static readonly string ServiceEndpointName = "WRPServiceEndpoint";
        public static readonly string ReplicatorEndpointName = "WRPReplicatorEndpoint";
        public static readonly string DefaultStoreName = "WRPStore";
        public static readonly string ClusterManifestName = "WRP_Generated_ClusterManifest";
        public static readonly string NativeImageStoreConnectionString = "fabric:ImageStore";
        public static readonly string DefaultClusterWrpConfigFolderName = "DefaultClusterWrpConfig";
        public static readonly string DefaultClusterWrpConfigFileName = "ClusterWrpConfig.json";
        public static readonly string DefaultClusterUpgradeDescriptionFolderName = "DefaultClusterUpgradeDescription";
        public static readonly string DefaultClusterUpgradeDescriptionFileName = "ClusterUpgradeDescription.json";
        public static readonly string CoordinatorTypeValue = "Paas";
        public static readonly string X509CredentialType = "X509";
        public static readonly string NodeIdGeneratorVersionValue = "V4";
        public static readonly string ClusterConnectionSequenceNumber = "ClusterConnectionSequenceNumber";
        public static readonly string EtlFileProducer = "EtlFileProducer";
        public static readonly string PropertyGroupParameter = "PropertyGroup";
        public static readonly string DefaultPlacementConstraintsKey = "NodeTypeName";
        public static readonly string FileShareEtwCsvUploader = "FileShareEtwCsvUploader";
        public static readonly string FileShareFolderUploader = "FileShareFolderUploader";
        public static readonly string FolderProducer = "FolderProducer";
        public static readonly string WindowsFabricCrashDumps = "WindowsFabricCrashDumps";
        public static readonly string WindowsFabricPerformanceCounters = "WindowsFabricPerformanceCounters";
        public static readonly string ManagedServiceAccount = "ManagedServiceAccount";

        public class ConfigPackage
        {
            public static readonly string ConfigPackageName = "Config";
            public static readonly string RequiredParametersSection = "RequiredParameters";
            public static readonly string SystemServicesForScaleSection = "SystemServicesForScale";
            public static readonly string WrpSettingsSection = "WrpSettings";
            public static readonly string FeatureEligibilityBySubscription = "FeatureEligibilityBySubscription";
            public static readonly string ReplicatorSecuritySettingsSection = "ReplicatorSecuritySettings";
        }

        public class SectionName
        {
            public const string UnreliableTransport = "UnreliableTransport";
            public const string Votes = "Votes";
            public const string MetricActivityThresholds = "MetricActivityThresholds";
            public const string MetricBalancingThresholds = "MetricBalancingThresholds";
            public const string GlobalMetricWeights = "GlobalMetricWeights";
            public const string DefragmentationMetrics = "DefragmentationMetrics";
            public const string NodeBufferPercentage = "NodeBufferPercentage";
            public const string FabricTestQueryWeights = "FabricTestQueryWeights";
            public const string RunAsDca = "RunAs_DCA";
            public const string RunAsHttpGateway = "RunAs_HttpGateway";
            public const string RunAsFabric = "RunAs_Fabric";
            public const string InfrastructureService = "InfrastructureService";
            public const string TokenValidationService = "DSTSTokenValidationService";
            public const string NodeProperties = "NodeProperties";
            public const string NodeCapacities = "NodeCapacities";
            public const string NodeSfssRgPolicies = "NodeSfssRgPolicies";
            public const string HealthManagerClusterHealthPolicy = "HealthManager/ClusterHealthPolicy";
            public const string WinFabPerfCtrFolder = "WinFabPerfCtrFolder";
            public const string WinFabEtlFile = "WinFabEtlFile";
            public const string WinFabCrashDump = "WinFabCrashDump";
            public const string UpgradeOrchestrationService = "UpgradeOrchestrationService";
            public const string Common = "Common";

            public static readonly string FailoverManager = "FailoverManager";
            public static readonly string Failover = "Failover";
            public static readonly string ClusterManager = "ClusterManager";
            public static readonly string Hosting = "Hosting";
            public static readonly string Management = "Management";
            public static readonly string UpgradeService = "UpgradeService";
            public static readonly string BackupRestoreService = "BackupRestoreService";
            public static readonly string ImageStoreService = "ImageStoreService";
            public static readonly string FaultAnalysisService = "FaultAnalysisService";
            public static readonly string InfrastructureServicePrefix = "InfrastructureService";
            public static readonly string HttpGateway = "HttpGateway";
            public static readonly string HttpApplicationGateway = "ApplicationGateway/Http";
            public static readonly string FileStoreService = "FileStoreService";
            public static readonly string Security = "Security";
            public static readonly string SecurityClientX509Names = "Security/ClientX509Names";
            public static readonly string SecurityAdminClientX509Names = "Security/AdminClientX509Names";
            public static readonly string SecurityClusterX509Names = "Security/ClusterX509Names";
            public static readonly string SecurityServerX509Names = "Security/ServerX509Names";
            public static readonly string SecurityClusterCertificateIssuerStores = "Security/ClusterCertificateIssuerStores";
            public static readonly string SecurityServerCertificateIssuerStores = "Security/ServerCertificateIssuerStores";
            public static readonly string SecurityClientCertificateIssuerStores = "Security/ClientCertificateIssuerStores";
            public static readonly string Setup = "Setup";
            public static readonly string RunAs = "RunAs";
            public static readonly string Paas = "Paas";
            public static readonly string Federation = "Federation";
            public static readonly string Diagnostics = "Diagnostics";
            public static readonly string PlacementAndLoadBalancing = "PlacementAndLoadBalancing";
            public static readonly string ReconfigurationAgent = "ReconfigurationAgent";
            public static readonly string NamingService = "NamingService";
            public static readonly string TraceEtw = "Trace/Etw";
            public static readonly string TransactionalReplicator = "TransactionalReplicator";
            public static readonly string RepairManager = "RepairManager";
            public static readonly string CentralSecretService = "CentralSecretService";
            public static readonly string LocalSecretService = "LocalSecretService";
            public static readonly string DnsService = "DnsService";
            public static readonly string FabricClient = "FabricClient";
            public static readonly string ResourceMonitorService = "ResourceMonitorService";
            public static readonly string SFVolumeDiskService = "SFVolumeDiskService";
            public static readonly string EventStoreService = "EventStoreService";
            public static readonly string GatewayResourceManager = "GatewayResourceManager";

            // DCA section Names
            public static readonly string FileShareWinFabEtw = "FileShareWinFabEtw";
            public static readonly string FileShareWinFabCrashDump = "FileShareWinFabCrashDump";
            public static readonly string FileShareWinFabPerfCtr = "FileShareWinFabPerfCtr";
            public static readonly string WinFabLttProducer = "WinFabLttProducer";
            public static readonly string WinFabEtlFileQueryable = "WinFabEtlFileQueryable";
            public static readonly string WinFabPerfCounter = "WinFabPerfCounter";
            public static readonly string AzureWinFabCsv = "AzureWinFabCsv";
            public static readonly string AzureBlobWinFabEtw = "AzureBlobWinFabEtw";
            public static readonly string AzureTableWinFabEtwQueryable = "AzureTableWinFabEtwQueryable";
            public static readonly string AzureBlobWinFabCrashDump = "AzureBlobWinFabCrashDump";
            public static readonly string AzureBlobWinFabPerfCounter = "AzureBlobWinFabPerfCounter";            
            public static readonly string AzureBlobServiceFabricEtw = "AzureBlobServiceFabricEtw";
            public static readonly string AzureTableServiceFabricEtwQueryable = "AzureTableServiceFabricEtwQueryable";
            public static readonly string AzureBlobServiceFabricCrashDump = "AzureBlobServiceFabricCrashDump";
            public static readonly string AzureBlobServiceFabricPerfCounter = "AzureBlobServiceFabricPerfCounter";
            public static readonly string ServiceFabricEtlFile = "ServiceFabricEtlFile";
            public static readonly string ServiceFabricEtlFileQueryable = "ServiceFabricEtlFileQueryable";
            public static readonly string ServiceFabricCrashDump = "ServiceFabricCrashDump";
            public static readonly string ServiceFabricPerfCounter = "ServiceFabricPerfCounter";
            public static readonly string ServiceFabricPerfCtrFolder = "ServiceFabricPerfCtrFolder";
        }

        public class ParameterName
        {
            public static readonly string ExpectedClusterSize = "ExpectedClusterSize";
            public static readonly string ImageStoreConnectionString = "ImageStoreConnectionString";
            public static readonly string BaseUrl = "BaseUrl";
            public static readonly string CoordinatorType = "CoordinatorType";
            public static readonly string ClusterId = "ClusterId";
            public static readonly string IsEnabled = "IsEnabled";
            public static readonly string X509StoreName = "X509StoreName";
            public static readonly string X509StoreLocation = "X509StoreLocation";
            public static readonly string X509FindType = "X509FindType";
            public static readonly string X509FindValue = "X509FindValue";
            public static readonly string X509SecondaryFindValue = "X509SecondaryFindValue";
            public static readonly string PrimaryAccountType = "PrimaryAccountType";
            public static readonly string PrimaryAccountNtlmPasswordSecret = "PrimaryAccountNTLMPasswordSecret";
            public static readonly string PrimaryAccountNtlmx509StoreLocation = "PrimaryAccountNTLMX509StoreLocation";
            public static readonly string PrimaryAccountNtlmx509StoreName = "PrimaryAccountNTLMX509StoreName";
            public static readonly string PrimaryAccountNtlmx509Thumbprint = "PrimaryAccountNTLMX509Thumbprint";
            public static readonly string CommonNameNtlmPasswordSecret = "CommonNameNtlmPasswordSecret";
            public static readonly string CommonName1Ntlmx509StoreLocation = "CommonName1Ntlmx509StoreLocation";
            public static readonly string CommonName1Ntlmx509StoreName = "CommonName1Ntlmx509StoreName";
            public static readonly string CommonName1Ntlmx509CommonName = "CommonName1Ntlmx509CommonName";
            public static readonly string SecondaryAccountType = "SecondaryAccountType";
            public static readonly string SecondaryAccountNtlmPasswordSecret = "SecondaryAccountNTLMPasswordSecret";
            public static readonly string SecondaryAccountNtlmx509StoreLocation = "SecondaryAccountNTLMX509StoreLocation";
            public static readonly string SecondaryAccountNtlmx509StoreName = "SecondaryAccountNTLMX509StoreName";
            public static readonly string SecondaryAccountNtlmx509Thumbprint = "SecondaryAccountNTLMX509Thumbprint";
            public static readonly string CommonName2Ntlmx509StoreLocation = "CommonName2Ntlmx509StoreLocation";
            public static readonly string CommonName2Ntlmx509StoreName = "CommonName2Ntlmx509StoreName";
            public static readonly string CommonName2Ntlmx509CommonName = "CommonName2Ntlmx509CommonName";
            public static readonly string AnonymousAccessEnabled = "AnonymousAccessEnabled";
            public static readonly string FirewallPolicyEnabled = "FirewallPolicyEnabled";
            public static readonly string DisableFirewallRuleForDomainProfile = "DisableFirewallRuleForDomainProfile";
            public static readonly string DisableFirewallRuleForPublicProfile = "DisableFirewallRuleForPublicProfile";
            public static readonly string DisableFirewallRuleForPrivateProfile = "DisableFirewallRuleForPrivateProfile";
            public static readonly string ServerAuthAllowedCommonNames = "ServerAuthAllowedCommonNames";
            public static readonly string ClusterAllowedCommonNames = "ClusterAllowedCommonNames";

            public static readonly string GatewayAuthCredentialType = "GatewayAuthCredentialType";
            public static readonly string GatewayX509CertificateStoreName = "GatewayX509CertificateStoreName";
            public static readonly string GatewayX509CertificateFindType = "GatewayX509CertificateFindType";
            public static readonly string GatewayX509CertificateFindValue = "GatewayX509CertificateFindValue";
            public static readonly string GatewayX509CertificateFindValueSecondary = "GatewayX509CertificateFindValueSecondary";

            public static readonly string StoreConnectionString = "StoreConnectionString";
            public static readonly string ContainerName = "ContainerName";
            public static readonly string TableNamePrefix = "TableNamePrefix";
            public static readonly string ClusterCredentialType = "ClusterCredentialType";
            public static readonly string ServerAuthCredentialType = "ServerAuthCredentialType";
            public static readonly string ClientAuthAllowedCommonNames = "ClientAuthAllowedCommonNames";
            public static readonly string ClusterCertThumbprints = "ClusterCertThumbprints";
            public static readonly string ServerCertThumbprints = "ServerCertThumbprints";
            public static readonly string ClientCertThumbprints = "ClientCertThumbprints";
            public static readonly string AdminClientCertThumbprints = "AdminClientCertThumbprints";
            public static readonly string IgnoreCrlOfflineError = "IgnoreCrlOfflineError";
            public static readonly string UseSecondaryIfNewer = "UseSecondaryIfNewer";
            public static readonly string RunAsAccountType = "RunAsAccountType";
            public static readonly string RunAsAccountName = "RunAsAccountName";
            public static readonly string NodeIdGeneratorVersion = "NodeIdGeneratorVersion";
            public static readonly string MinReplicaSetSize = "MinReplicaSetSize";
            public static readonly string TargetReplicaSetSize = "TargetReplicaSetSize";
            public static readonly string PlacementConstraints = "PlacementConstraints";
            public static readonly string ProducerInstance = "ProducerInstance";
            public static readonly string ProducerInstances = "ProducerInstances";
            public static readonly string ConsumerInstances = "ConsumerInstances";
            public static readonly string ProducerType = "ProducerType";
            public static readonly string ConsumerType = "ConsumerType";
            public static readonly string DataDeletionAgeInDays = "DataDeletionAgeInDays";
            public static readonly string EnableRepairExecution = "WindowsAzure.EnableRepairExecution";

            public static readonly string EnableHealthChecks = "EnableHealthChecks";
            public static readonly string InstanceCount = "InstanceCount";

            public static readonly string AadTenantId = "AADTenantId";
            public static readonly string AadClusterApplication = "AADClusterApplication";
            public static readonly string AadClientApplication = "AADClientApplication";
            public static readonly string FabricHostSpn = "FabricHostSpn";
            public static readonly string ClientIdentities = "ClientIdentities";
            public static readonly string AdminClientIdentities = "AdminClientIdentities";
            public static readonly string ClusterIdentities = "ClusterIdentities";
            public static readonly string ClusterSpn = "ClusterSpn";
            public static readonly string AllowDefaultClient = "AllowDefaultClient";
            public static readonly string ClientRoleEnabled = "ClientRoleEnabled";
            public static readonly string MaxDiskQuotaInMb = "MaxDiskQuotaInMB";
            public static readonly string FolderType = "FolderType";
            public static readonly string EnableTelemetry = "EnableTelemetry";
            public static readonly string AutoupgradeEnabled = "AutoupgradeEnabled";
            public static readonly string AutoupgradeInstallEnabled = "AutoupgradeInstallEnabled";
            public static readonly string GoalStateExpirationReminderInDays = "GoalStateExpirationReminderInDays";
            public static readonly string GoalStateFileUrl = "GoalStateFileUrl";
            public static readonly string GoalStateFetchIntervalInSeconds = "GoalStateFetchIntervalInSeconds";
            public static readonly string GoalStateProvisioningTimeOfDay = "GoalStateProvisioningTimeOfDay";
            public static readonly string FaultFlow = "FaultFlow";
            public static readonly string FaultStep = "FaultStep";
            public static readonly string EnableEndpointV2 = "EnableEndpointV2";
            public static readonly string NodesToBeRemoved = "NodesToBeRemoved";
            public static readonly string ExpectedNodeDeactivationDuration = "ExpectedNodeDeactivationDuration";
            public static readonly string FabricDataRoot = "FabricDataRoot";
            public static readonly string FabricLogRoot = "FabricLogRoot";
            public static readonly string IsDevCluster = "IsDevCluster";
            public static readonly string IsTestCluster = "IsTestCluster";
            public static readonly string EnableCircularTraceSession = "EnableCircularTraceSession";
            public static readonly string QuorumBasedReplicaDistributionPerUpgradeDomains = "QuorumBasedReplicaDistributionPerUpgradeDomains";
            public static readonly string IsSingletonReplicaMoveAllowedDuringUpgrade = "IsSingletonReplicaMoveAllowedDuringUpgrade";

            public static readonly string EnableAutomaticBaseline = "EnableAutomaticBaseline";
            public static readonly string DisableContainerServiceStartOnContainerActivatorOpen = "DisableContainerServiceStartOnContainerActivatorOpen";

            public static readonly string EnableUnsupportedPreviewFeatures = "EnableUnsupportedPreviewFeatures";

            // Flag inserted in the Hosting section to indicate if SFVolumeDisk service is enabled, or not, when the add-on service section is processed.
            public static readonly string IsSFVolumeDiskServiceEnabled = "IsSFVolumeDiskServiceEnabled";

            // Parameters in the GatewayResourceManager config
            public static readonly string WindowsProxyImageName = "WindowsProxyImageName";
            public static readonly string LinuxProxyImageName = "LinuxProxyImageName";
            public static readonly string ProxyReplicaCount = "ProxyReplicaCount";
        }
    }
}
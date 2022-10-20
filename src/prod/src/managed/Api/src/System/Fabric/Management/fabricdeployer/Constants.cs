// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Collections.Generic;
    using System.IO;
    using System.Reflection;

    internal enum HostedServiceState
    {
        Up,
        Down,
        Killed
    }

    /// <summary>
    /// <see cref="DockerDnsHelper"/>
    /// </summary>
    internal enum ContainerDnsSetup
    {
        /// <summary>
        /// Perform setup, but continue with Fabric deployment on setup failures.
        /// </summary>
        Allow,

        /// <summary>
        /// Perform setup, but don't proceed with Fabric deployment in case of failures.
        /// </summary>
        Require,

        /// <summary>
        /// Don't perform setup. Also try and clean up the previous installation.
        /// </summary>
        Disallow,
    }

    internal static class Constants
    {
        public const string ServiceFabricAdminEventLogName = "Microsoft-ServiceFabric/Admin";
        public const string ServiceFabricOperationalEventLogName = "Microsoft-ServiceFabric/Operational";
        public const string EventLogsFileName = "ServiceFabricEventLog.txt";
        public const string EmptyErrorStringToBeReplaced = "";
        public const string FabricService = "Fabric";
        public const string DCAService = "DCA";
        public const string HttpGatewayService = "HttpGateway";
        public const string FabricGatewayService = "FabricGateway";
        public const string FabricApplicationGatewayService = "FabricApplicationGateway";
        public const string CtrlCSenderExe = "CtrlCSender.exe";
        public const string FabricExe = "Fabric.exe";
        public const string DefaultImageStoreRootFolderName = "ImageStore";
        public const string DefaultTag = "_default_";
        public const string DefaultImageStoreIncomingFolderName = "incoming";
        public const string ConfigName = "Fabric.Config";
        public const string DataName = "Fabric.Data";
        public const string FabricHostServiceName = "FabricHostSvc";
        public const string FabricNamespace = "http://schemas.microsoft.com/2011/01/fabric";
        public const string FabricTestAdminConfigPath = "FabricTestAdminConfigPath";
        public const string WusaBinaryName = "wusa.exe";
        public const string ManagedServiceAccount = "ManagedServiceAccount";
        public const int InstallationTimeoutMinutes = 5;

        public const string FileStoreService = "FileStoreService";
        public const string FaultAnalysisService = "FAS";
        public const string BackupRestoreService = "BRS";
        public const string FabricUpgradeService = "US";
        public const string FabricRepairManagerService = "RM";
        public const string FabricInfrastructureService = "IS";
        public const string SystemServiceCodePackageName = "Code";
        public const string SystemServiceCodePackageVersion = "Current";
        public const string SystemApplicationId = "__FabricSystem_App4294967295";
        public const string UpgradeOrchestrationService = "UOS";
        public const string CentralSecretService = "CSS";
        public const string EventStoreService = "ES";
        public const string GatewayResourceManager = "GRM";

        public const int FileOpenDefaultRetryAttempts = 10;
        public const int FileOpenDefaultRetryIntervalMilliSeconds = 100;

        public const int ApiRetryAttempts = 5;
        public const int ApiRetryIntervalMilliSeconds = 3000;
        public const int MaxApiRetryAttempts = 20;
        public const int ErrorCode_Success = 0;

        // Failure Error Code
#if DotNetCoreClrLinux
        public const int ErrorCode_Failure = 3;
#else
        public const int ErrorCode_Failure = -1;
#endif
        // Success codes with context
        public const int ErrorCode_RestartRequired = 1;
        public const int ErrorCode_RestartNotRequired = 2;

        public const uint ExitCode_UpdateNotApplicable = 0x80240017;
        public const uint ExitCode_UpdateAlreadyInstalled = 0x240006;

        public static readonly string[] RequiredServices = { DCAService, FabricService, FabricGatewayService };

#if DotNetCoreClrIOT
        public static readonly Dictionary<string, string> ServiceExes = new Dictionary<string, string>() { { DCAService, "FabricDCA.bat" }, { FabricService, "Fabric.exe" }, { FabricGatewayService, "FabricGateway.exe" }, { FileStoreService, "FileStoreService.exe" }, { FabricApplicationGatewayService, "FabricApplicationGateway.exe" }, { FaultAnalysisService, "FabricFAS.exe" }, { FabricUpgradeService, "FabricUS.bat" }, { FabricRepairManagerService, "FabricRM.exe" }, { FabricInfrastructureService, "FabricIS.exe" }, { UpgradeOrchestrationService, "FabricUOS.exe" }, { BackupRestoreService, "FabricBRS.exe" } };
#elif DotNetCoreClrLinux
        public static readonly Dictionary<string, string> ServiceExes = new Dictionary<string, string>() { { DCAService, "FabricDCA.sh" }, { FabricService, "Fabric" }, { FabricGatewayService, "FabricGateway.exe" }, { FileStoreService, "FileStoreService.exe" }, { FabricApplicationGatewayService, "FabricApplicationGateway.exe" }, { FaultAnalysisService, "FabricFAS.exe" }, { FabricUpgradeService, "FabricUS.exe" }, { FabricRepairManagerService, "FabricRM.exe" }, { FabricInfrastructureService, "FabricIS.exe" }, { GatewayResourceManager, "FabricGRM.exe" }  };
#else        
        public static readonly Dictionary<string, string> ServiceExes = new Dictionary<string, string>() { { DCAService, "FabricDCA.exe" }, { FabricService, "Fabric.exe" }, { FabricGatewayService, "FabricGateway.exe" }, { FileStoreService, "FileStoreService.exe" }, { FabricApplicationGatewayService, "FabricApplicationGateway.exe" }, { FaultAnalysisService, "FabricFAS.exe" }, { FabricUpgradeService, "FabricUS.exe" }, { FabricRepairManagerService, "FabricRM.exe" }, { FabricInfrastructureService, "FabricIS.exe" }, { UpgradeOrchestrationService, "FabricUOS.exe" }, { BackupRestoreService, "FabricBRS.exe" }, { CentralSecretService, "FabricCSS.exe" }, { EventStoreService, "EventStore.Service.exe" }, { GatewayResourceManager, "FabricGRM.exe" } };
#endif

#if DotNetCoreClr
        public static readonly string FabricEtcConfigPath = "/etc/servicefabric/";
#endif

        public const string DefaultFabricDataRoot = @"%ProgramData%\Windows Fabric\";
        public const string FabricBinRootRelativePath = @"bin";
        public const string FabricCodePathRelativePath = @"bin\fabric\fabric.code";
        public const string DefaultDiagnosticsStoreRootFolderName = "DiagnosticsStore";
        public const string TracesFolderName = "traces";

        public const string FabricInstallerServiceName = "FabricInstallerSvc";
        public const string FabricInstallerDisplayName = "Service Fabric Installer Service";
        public const string FabricInstallerServiceDescription = "Performs reliable code upgrades for Service Fabric.";
        public const int FabricInstallerServiceDelayInSecondsBeforeRestart = 5;
        public const int FabricUpgradeFailureCountResetPeriodInDays = 1;
        public const string FabricInstallerServiceRelativePathFromRoot = "FabricInstallerService.Code\\FabricInstallerService.exe";
        public const string FabricInstallerServiceAutocleanArg = "--autoclean";
        public const int FabricServiceStopTimeoutInSeconds = 60;
        public const int FabricUninstallTimeoutInMinutes = 20;
        public const int FabricInstallerServiceStartTimeoutInMinutes = 3;
        public const int ServiceManagementTimeoutInSeconds = 60;

        public const int UriReachableTimeoutInMs = 50000;
        public const int UriRequestTimeoutInMs = 15000;
        public const int UriReachableRetryIntervalInMs = 500;
               
        /// <summary>
        /// We use this prefix to setup all services in Wrp, and this is used by fabric client to setup primary change notifications.
        /// </summary>
        public static string WrpApplicationNamePrefix = "fabric:/WRP-";

        /// <summary>
        /// Partition Service Application Name
        /// </summary>
        public static string PartitionServiceApplicationName = WrpApplicationNamePrefix + "Partition";

        /// <summary>
        /// Partition Service  Name
        /// </summary>
        public static string PartitionServiceName = "PartitionService";

        /// <summary>
        /// Backend Service Application Name Prefix
        /// </summary>
        public static string BackendServiceApplicationNamePrefix = WrpApplicationNamePrefix + "Backend_";

        /// <summary>
        /// Backend  Service Name
        /// </summary>
        public static string BackendServiceName = "BackendService";

        // Cache batch query settings
        public static int MaxPerMapRequestRecords = 500;
        public static readonly string ProviderNamespace = "Microsoft.ServiceFabric";
        public static readonly string ResourceType = ProviderNamespace + "/clusters";

        /// <summary>
        /// Request URI paramater to filter on the given expression.
        /// </summary>
        public static string QueryFilter = "Filter";

        /// <summary>
        /// Feature name used to determine if environment supports windows containers
        /// </summary>
        public static string ContainersFeatureName = "Containers";

        /// <summary>
        /// Used to distinguish if the target machine is local, to avoid remote calls.
        /// </summary>
        public const string LocalHostMachineName = "localhost";

        /// <summary>
        /// Docker host dns name
        /// </summary>
        public const string DockerHostDnsName = "host.docker.internal";

#if DotNetCoreClrLinux
        public const string PackageName_ServiceFabric = "servicefabric";
        public const string PackageName_DockerCE = "docker-ce";
        public const string PackageName_ContainerdIO = "containerd.io";
        public const string PackageName_MobyEngine = "moby-engine";
        public const string PackageName_MobyCLI = "moby-cli";

        public const string DpkgPackageCleanlyInstalledCommandFormat = "dpkg -l {0} | grep ^ii[[:space:]]*{0}";
        public const string DpkgPackageStateVisibleCommandFormat = "dpkg -l {0} | grep [[:space:]]*{0}";
        public const string DpkgConfigureCommand = "dpkg --configure -a";
        public const string AptPackageDepencenciesContainsFormat = "apt-cache depends {0} | grep \"Depends:[[:space:]]*{1}$\"";
        public const string AptPackageResolveCommand = "apt -o Debug::pkgProblemResolver=yes install -fy";
        public const string AptPackageInstallCommand = "apt -o Debug::pkgProblemResolver=yes install {0} -fy";
        public const string AptUpdateCommand = "apt update";
        public const string AptUpgradePackageCommandFormat = "apt -o Debug::pkgProblemResolver=yes install --only-upgrade {0} -y";
        public const string AptPurgePackageCommandFormat = "apt -o Debug::pkgProblemResolver=yes purge {0} -fy";
        public const string AptAutoremoveCommand = "apt autoremove -y";
#endif

        /// <summary>
        /// APIPA subnet
        /// </summary>
        public const string ApipaSubnet = "169.254.0.0";
        
        public static class FileNames
        {
            public const string Schema = "ServiceFabricServiceModel.xsd";
            public const string FabricHostSettings = "FabricHostSettings.xml";
            public const string HostedServiceState = "HostedServiceState.csv";
            public const string Settings = "Settings.xml";
            public const string SeedInfo = "SeedInfo.ini";
            public const string TargetInformation = "TargetInformation.xml";
            public const string BaselineCab = "ServiceFabric.cab";
            public const string BaselineJsonClusterConfig = "ClusterConfig.json";
            public const string ClusterSettings = "ClusterSettings.json";
        }

        public static class Registry
        {
            public const string TestFailDeployerKey = "__Test_Fail_FirstDeployment";
            public const string SkipDeleteFabricDataRoot = "SkipDeleteFabricDataRoot";
            public const string RemoveNodeConfigurationValue = "RemoveNodeConfiguration";
            public const string DynamicTopologyKindValue = "DynamicTopologyKind";
#if !DotNetCoreClrLinux
            public const string NodeLastBootUpTime = "NodeLastBootUpTime";
#endif
        }

        public static class ParameterNames
        {
            public const string InstanceName = "InstanceName";
            public const string IPAddressOrFQDN = "IPAddressOrFQDN";
            public const string WorkingDir = "WorkingDir";
            public const string ServerCertType = "ServerAuth";
            public const string ClientCertType = "ClientAuth";
            public const string UserRoleClientCertType = "UserRoleClient";
            public const string ClusterCertType = "Cluster";
            public const string X509StoreNamePattern = "{0}X509StoreName";
            public const string X509FindTypePattern = "{0}X509FindType";
            public const string X509FindValuePattern = "{0}X509FindValue";
            public const string X509FindValueSecondaryPattern = "{0}X509FindValueSecondary";
            public const string NodeVersion = "NodeVersion";
            public const string NodeType = "NodeType";
            public const string UpgradeDomainId = "UpgradeDomainId";
            public const string NodeFaultDomainId = "NodeFaultDomainId";
            public const string ImageStoreConnectionString = "ImageStoreConnectionString";
            public const string MonitoringAgentStorageAccount = "MonitoringAgentStorageAccount";
            public const string MonitoringAgentTransferInterval = "MonitoringAgentTransferInterval";
            public const string MonitoringAgentDirectoryQuota = "MonitoringAgentDirectoryQuota";
            public const string ExePath = "ExePath";
            public const string CtrlCSenderPath = "CtrlCSenderPath";
            public const string SamplingIntervalInSeconds = "SamplingIntervalInSeconds";
            public const string MaxCounterBinaryFileSizeInMB = "MaxCounterBinaryFileSizeInMB";
            public const string TestOnlyIncludeMachineNameInCounterFileName = "TestOnlyIncludeMachineNameInCounterFileName";
            public const string EnvironmentMap = "Environment";
            public const string RunAsAccountName = "RunAsAccountName";
            public const string RunAsAccountType = "RunAsAccountType";
            public const string RunAsPassword = "RunAsPassword";
            public const string SkipFirewallConfiguration = "SkipFirewallConfiguration";
            public const string IsScaleMin = "IsScaleMin";
            public const string SharedLogFilePath = "SharedLogFilePath";
            public const string SharedLogFileId = "SharedLogFileId";
            public const string SharedLogFileSizeInMB = "SharedLogFileSizeInMB";
            public const string Backup = "Backup";
            public const string ApplicationCheckPointFiles = "ApplicationCheckPointFiles";
            public const string Log = "Log";
            public const string UserDefinedDirsKeyValuePairString = "UserDefinedDirsKeyValuePairString";
            public const string Port = "Port";
            public const string Protocol = "Protocol";
            public const string Disabled = "Disabled";
            public const string FabricLogRoot = "FabricLogRoot";
            public const string FabricDataRoot = "FabricDataRoot";
            public const string NodesToBeRemoved = "NodesToBeRemoved";
            public const string EnableCircularTraceSession = "EnableCircularTraceSession";
            public const string ServiceRunAsAccountName = "ServiceRunAsAccountName";
            public const string ServiceRunAsPassword = "ServiceRunAsPassword";
            public const string ServiceStartupType = "ServiceStartupType";
            public const string DeploymentDirectory = "DeploymentDirectory";
            public const string ConsumerInstances = "ConsumerInstances";
            public const string StoreConnectionString = "StoreConnectionString";
            public const string ServiceNodeName = "ServiceNodeName";
            public const string DisableFirewallRuleForPublicProfile = "DisableFirewallRuleForPublicProfile";
            public const string DisableFirewallRuleForPrivateProfile = "DisableFirewallRuleForPrivateProfile";
            public const string DisableFirewallRuleForDomainProfile = "DisableFirewallRuleForDomainProfile";
            public const string ContainerDnsSetup = "ContainerDnsSetup";
            public const string ContainerNetworkSetup = "ContainerNetworkSetup";
            public const string ContainerNetworkName = "ContainerNetworkName";
            public const string IsolatedNetworkSetup = "IsolatedNetworkSetup";
            public const string IsolatedNetworkName = "IsolatedNetworkName";
            public const string IsolatedNetworkInterfaceName = "IsolatedNetworkInterfaceName";
            public const string ProcessCpusetCpus = "ProcessCpusetCpus";
            public const string ProcessCpuShares = "ProcessCpuShares";
            public const string ProcessMemoryInMB = "ProcessMemoryInMB";
            public const string ProcessMemorySwapInMB = "ProcessMemorySwapInMB";
#if !DotNetCoreClrLinux
            public const string SkipContainerNetworkResetOnReboot = "SkipContainerNetworkResetOnReboot";
            public const string SkipIsolatedNetworkResetOnReboot = "SkipIsolatedNetworkResetOnReboot";
#endif
            public const string IsSFVolumeDiskServiceEnabled = "IsSFVolumeDiskServiceEnabled";
            public const string EnableUnsupportedPreviewFeatures = "EnableUnsupportedPreviewFeatures";
            public const string ContainerServiceArguments = "ContainerServiceArguments";
            public const string UseContainerServiceArguments = "UseContainerServiceArguments";
            public const string EnableContainerServiceDebugMode = "EnableContainerServiceDebugMode";
        }

        public static class SectionNames
        {
            public const string Votes = "Votes";
            public const string FailoverManager = "FailoverManager";
            public const string FabricHost = "FabricHost";
            public const string SeedMapping = "[SeedMapping]";
            public const string FabricNode = "FabricNode";
            public const string SeedNodeClientConnectionAddresses = "SeedNodeClientConnectionAddresses";
            public const string NodeProperties = "NodeProperties";
            public const string NodeCapacities = "NodeCapacities";
            public const string NodeSfssRgPolicies = "NodeSfssRgPolicies";
            public const string NodeDomainIds = "NodeDomainIds";
            public const string Management = "Management";
            public const string Hosting = "Hosting";
            public const string HostSettingsSectionPattern = "HostedService/{0}_{1}";
            public const string LocalLogStore = "LocalLogStore";
            public const string DiagnosticFileStore = "DiagnosticFileStore";
            public const string Diagnostics = "Diagnostics";
            public const string Security = "Security";
            public const string PerformanceCounterLocalStore = "PerformanceCounterLocalStore";
            public const string RunAs = "RunAs";
            public const string RunAs_Fabric = "RunAs_Fabric";
            public const string RunAs_DCA = "RunAs_DCA";
            public const string RunAs_HttpGateway = "RunAs_HttpGateway";
            public const string Setup = "Setup";
            public const string LogicalApplicationDirectories = "LogicalApplicationDirectories";
            public const string LogicalNodeDirectories = "LogicalNodeDirectories";
            public const string Common = "Common";
        }

        public static class FabricVersions
        {
            public const string CodeVersionPattern = "{0}.{1}.{2}.{3}";
        }

        public static class ClaimsAuthentication
        {
            public const string PS_SetExecutionPolicy = "Set-ExecutionPolicy";
            public const string Param_AllSigned = "AllSigned";
            public const string PS_EnableFeature = "Enable-WindowsOptionalFeature";
            public const string Param_WIFName = "Windows-Identity-Foundation";
            public const string PS_GetWindowsFeature = "Get-WindowsOptionalFeature";
            public const string PS_DISMOnlineOption = "-Online";
            public const int Srv08R2_Minor_Version = 1;
            public const int Srv12_Minor_Version = 2;
            public const string WIFHotfixPackageName = "Windows6.1-KB974405-x64.msu";
        }

        public static class WindowsSecurity
        {
            public const string ServiceClass = "WindowsFabric";
            public const string RegValueName = "SpnRegistered";
        }
    }
}
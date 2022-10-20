// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    internal class Constants
    {
        #region Deployment Manager Constants

        public const int FabricServiceStopTimeoutInSeconds = 60;
        public const int FabricInstallerServiceStartTimeoutInMinutes = 3;
        public const int FabricInstallerServiceHostCreationTimeoutInMinutes = 5;
        public const int FabricHostAwaitRunningTimeoutInMinutes = 10;
        public const int FabricDataRootMaxPath = 80;
        public const int FabricDownloadFileTimeoutInHours = 3;
        public const string FabricDeploymentCabsDirectory = "DeploymentRuntimePackages";
        public const string FabricInstallerServiceName = "FabricInstallerSvc";
        public const string FabricInstallerServiceFolderName = "FabricInstallerService.Code";
        public const string FabricHostServiceName = "FabricHostSvc";
        public const string FabricDeploymentTracesDirectory = "DeploymentTraces";
        public const string UninstallProgramsBaseKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
        public const string FabricDataRootString = "FabricDataRoot";
        public const string FabricLogRootString = "FabricLogRoot";
        public const string SetupString = "Setup";
        public const string FabricUnitTestNoTraceName = "FabricUnitTestNoTrace";
        public const string ServiceFabricEventLogsDirectoryName = "ServiceFabricEventLogs";
        public const string EventLogsFileName = "ServiceFabricEventLog.txt";

        #endregion

        #region Ancillary System Constants

        public const string RemoteRegistryServiceName = "RemoteRegistry";
        public const string FirewallServiceName = "MpsSvc";
        public const int SmbFileSharePort = 445;

        public const string RemoteRegistryNamedPipeName = "winreg";
        public const int NamedPipeConnectTimeoutInMs = 30000;

        public const string HttpMethodHead = "HEAD";
        public const string HttpMethodOptions = "OPTIONS";
        public const string LocalHostMachineName = "localhost";

        #endregion

        #region IOTcore Common

        public const string IOTCurrentVersionRegKeyPath = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
        public const string IOTEditionIDKeyName = "EditionID";
        public const string IOTUAP = "IoTUAP";
        public const string Path = "Path"; 
        public const string SystemRoot = "SystemRoot";
        public const string EnvironmentRegKeyPath = @"SYSTEM\CurrentControlSet\Control\Session Manager\Environment";

        #endregion

        #region Standalone Common

        public const string PreBaselineVersion = "0.0.0.0";
        public const string AutoupgradeCodeVersion = "*";
        public const string UpgradeOrchestrationServiceConfigSectionName = "UpgradeOrchestrationService";
        public const string AutoupgradeEnabledName = "AutoupgradeEnabled";
        public const string AutoupgradeInstallEnabledName = "AutoupgradeInstallEnabled";
        public const string GoalStateFileUrlName = "GoalStateFileUrl";
        public const string GoalStateFetchIntervalInSecondsName = "GoalStateFetchIntervalInSeconds";
        public const string GoalStateProvisioningTimeOfDayName = "GoalStateProvisioningTimeOfDay";
        public const string GoalStateExpirationReminderInDays = "GoalStateExpirationReminderInDays";
        public const string BaselineJsonMetadataFileName = "ClusterConfigMetadata.json";

        public const string DefaultGoalStateFileUrl = "https://go.microsoft.com/fwlink/?LinkID=824848&clcid=0x409";
        public const int DefaultGoalStateFetchIntervalInSeconds = 86400;
        public const int DefaultGoalStateExpirationReminderInDays = 30;
        public const int FabricOperationTimeoutInMinutes = 10;
        public const int FabricUpgradeStatePollIntervalInMinutes = 2;
        public const int FabricGoalStateReachablePollIntervalInMinutes = 5;

        public const string SFPackageDropNameFormat = "ServiceFabric.{0}.cab";
        public const int UriReachableTimeoutInMs = 50000;
        public const int UriRequestTimeoutInMs = 15000;
        public const int UriReachableRetryIntervalInMs = 500;

        public const string UpgradeOrchestrationHealthSourceId = "System.UpgradeOrchestrationService";
        public const string ClusterVersionSupportHealthProperty = "ClusterVersionSupport";
        public const string ClusterGoalStateReachableHealthProperty = "ClusterGoalStateReachable";

        public const string ConfigurationsFileName = "Configurations.csv";
        public const string ClusterManifestNameFormat = "{0}.{1}.ClusterManifest.xml";
        public const string ClusterSettingsFileName = "ClusterSettings.json";
        public const string WrpSettingsFilename = "WRPSettings.xml";

        public static readonly TimeSpan DefaultProvisioningTimeOfDay = new TimeSpan(22, 00, 00);
        public static readonly TimeSpan DefaultRetryInterval = TimeSpan.FromSeconds(1);
        public static readonly int DefaultRetryCount = 5;
        public static readonly TimeSpan FabricQueryNodeListRetryTimeout = TimeSpan.FromMinutes(5);
        public static readonly TimeSpan FabricQueryNodeListTimeout = TimeSpan.FromMinutes(2);
        public static readonly TimeSpan BpaRpcRetryTimeout = TimeSpan.FromMinutes(5);

        public static readonly ulong DriveMinAvailableSpace = 1073741824;
        #endregion
    }
}
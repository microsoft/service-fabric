// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMdsAgentSvc
{
    internal static class Constants
    {
        internal const string ServiceTypeName = "FabricMdsAgentServiceType";

        internal const string MdsAgentDataPackageName = "MdsAgent.Data";
        internal const string MdsAgentLauncherExeName = "MonAgentLauncher.exe";
        internal const string MdsAgentWorkSubFolderName = "MA";

        internal const string ConfigSectionName = "FabricMdsAgentServiceConfig";
        internal const string MAConfigFileParamName = "MAConfigFileName";
        internal const string ClusterNameParamName = "ClusterName";
        internal const string DataCenterParamName = "DataCenterName";
        internal const string MAXStoreAccountsParamName = "MAXStoreAccounts";
        internal const string MAMdmAccountNameParamName = "MAMdmAccountName";
        internal const string MAMdmNamespaceParamName = "MAMdmNamespace";
        internal const string MARestartDelayInSecondsParamName = "MARestartDelayInSeconds";

        // Optional parameters for Geneva Config Service
        internal const string MAConnectionInfoParamName = "MAConnectionInfo";
        internal const string MAServiceIdentityParamName = "MAServiceIdentity";
        internal const string MAConfigVersionParamName = "MAConfigVersion";
        internal const string MAUseGenevaConfigServiceParamName = "MAUseGenevaConfigService";

        internal const string EventsElement = "Events";
        internal const string DynamicEventsElement = "DynamicEvents";
        internal const string DirectoryWatchItemElement = "DirectoryWatchItem";
        internal const string DirectoryElement = "Directory";

        internal const string MonitoringDataDirEnvVar = "MONITORING_DATA_DIRECTORY";
        internal const string MonitoringIdentityEnvVar = "MONITORING_IDENTITY";
        internal const string MonitoringInitConfigEnvVar = "MONITORING_INIT_CONFIG";
        internal const string MonitoringVersionEnvVar = "MONITORING_VERSION";

        internal const string MonitoringTenantEnvVar = "MONITORING_TENANT";
        internal const string MonitoringRoleEnvVar = "MONITORING_ROLE";
        internal const string MonitoringAppEnvVar = "MONITORING_APP";
        internal const string MonitoringRoleInstanceEnvVar = "MONITORING_ROLE_INSTANCE";
        internal const string MonitoringNodeNameEnvVar = "MONITORING_NODENAME";
        internal const string MonitoringDeploymentIdEnvVar = "MONITORING_DEPLOYMENT_ID";
        internal const string MonitoringDataCenterEnvVar = "MONITORING_DATACENTER";

        internal const string MonitoringXStoreAccountsEnvVar = "MONITORING_XSTORE_ACCOUNTS";
        internal const string MonitoringMdmAccountNameEnvVar = "MONITORING_MDM_ACCOUNT_NAME";
        internal const string MonitoringMdmNamespaceEnvVar = "MONITORING_MDM_NAMESPACE";
        internal const string LogDirectoryEnvVar = "MONITORING_SF_LOG_DIRECTORY";

        internal const string DefaultMAConfigFileName = "MdsConfig.xml";
        internal const string DefaultClusterName = "ClusterNameUnknown";
        internal const string DefaultDataCenterName = "DataCenterNameUnknown";
        internal const int DefaultMARestartDelayInSeconds = 180;

        internal const string MonitoringVersionValue = "1.0";
        internal const string MonitoringIdentityValue = "use_ip_address";
    }
}
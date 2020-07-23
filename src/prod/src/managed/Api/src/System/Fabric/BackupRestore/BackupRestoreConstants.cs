// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore
{
    internal class BackupRestoreContants
    {
        public const string BrsConfigSectionName = "BackupRestoreService";
        public const string SecurityConfigSectionName = "Security";
        public const string SecurityServerX509NamesConfigSectionName = "Security/ServerX509Names";
        public const string FabricNodeConfigSectionName = "FabricNode";
        public const string FabricNodeRunAsSectionName = "RunAs";

        public const string TargetReplicaSetSizeKeyName = "TargetReplicaSetSize";
        public const string ApiTimeoutKeyName = "ApiTimeoutInSeconds";
        public const string StoreApiTimeoutKeyName = "StoreApiTimeoutInSeconds";
        public const string ApiRetryIntervalKeyName = "ApiRetryIntervalInSeconds";
        public const string MaxApiRetryIntervalKeyName = "MaxApiRetryIntervalInSeconds";
        public const string MaxApiRetryCountKeyName = "MaxApiRetryCount";
        public const string EnableCompressionKeyName = "EnableCompression";
        public const string JitterInBackupsKeyName = "JitterInBackupsInSeconds";

        public const string DsmsAutopilotServiceNameKeyName = "DsmsAutopilotServiceName";

        public const string ServerAuthCredentialTypeName = "ServerAuthCredentialType";
        public const string ServerCertThumbprints = "ServerCertThumbprints";
        public const string ClusterIdentities = "ClusterIdentities";

        public const string ServerAuthX509StoreName = "ServerAuthX509StoreName";
        public const string ServerAuthX509FindType = "ServerAuthX509FindType";
        public const string ServerAuthX509FindValue = "ServerAuthX509FindValue";
        public const string ServerAuthX509FindValueSecondary = "ServerAuthX509FindValueSecondary";
        public const string RunAsAccountNameConfig = "RunAsAccountName";

        public const string RestEndPointName = "RestEndpoint";

        public const string HealthSourceId = "System.BackupRestoreManager";
        public const string HealthBackupProperty = "Backup";
        public const string HealthRestoreProperty = "Restore";

        public static TimeSpan ApiTimeoutInSecondsDefault = TimeSpan.FromSeconds(60);
        public static TimeSpan StoreApiTimeoutInSecondsDefault = TimeSpan.FromHours(1);
        public static TimeSpan ApiRetryIntervalInSecondsDefault = TimeSpan.FromSeconds(15);
        public static TimeSpan MaxApiRetryIntervalInSecondsDefault = TimeSpan.FromSeconds(60);
        public const int MaxRetryCountDefault = 3;
        public const int JitterInBackupsDefault = 10;

        public static readonly TimeSpan HealthInformationDeltaTimeToLive = TimeSpan.FromMinutes(5);
        public static readonly TimeSpan RestoreHealthInformationTimeToLive = TimeSpan.FromMinutes(120);

        public const int MaxHealthDescriptionLength = 4 * 1024 - 1;

        public const string SystemFabricBackupRestoreDsmsAssemblyName = "System.Fabric.BackupRestore.Dsms";
        public const string DsmsAzureStorageHelperClassFullName = "System.Fabric.BackupRestore.Dsms.DsmsAzureStorageHelper";
        public const string DsmsGetStorageAccountMethodName = "GetStorageAccount";

    }
}
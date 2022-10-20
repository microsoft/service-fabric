// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System;
    using System.Fabric.Description;

    internal static class Constants
    {
        public const string UpgradeServiceType = "UpgradeServiceType";
        public static readonly TimeSpan MaxOperationTimeout = TimeSpan.FromMinutes(2);
        public static readonly TimeSpan MaxTimeoutForWaitTasksExit = TimeSpan.FromSeconds(10);
        public static readonly TimeSpan KvsCommitTimeout = TimeSpan.FromSeconds(30);
        public static readonly TimeSpan KvsCreateTransactionRetryTime = TimeSpan.FromSeconds(5);
        public const int PartitionReadyWaitInMilliseconds = 10000;
        public const string SystemServicePrefix = @"fabric:/System/";
        public const string ImageStoreConnectionString = "fabric:ImageStore";
        public const string PrimaryNodeTypesDelimiters = ",";

        public static class ConfigurationSection
        {
            public const string PaasSectionName = "Paas";            

            public const string CoordinatorType = "CoordinatorType";
            public const string WUTestCoordinator = "WUTest";
            public const string WindowsUpdateServiceCoordinator = "WindowsUpdateService";
            public const string PaasCoordinator = "Paas";
            public const string BaseUrl = "BaseUrl";
            public const string ClusterId = "ClusterId";
            public const string X509StoreName = "X509StoreName";
            public const string X509StoreLocation = "X509StoreLocation";
            public const string X509FindType = "X509FindType";
            public const string X509FindValue = "X509FindValue";
            public const string X509SecondaryFindValue = "X509SecondaryFindValue";
            public const string MaxContinousFailureCount = "MaxContinousFailureCount";
            public const string PollIntervalInSeconds = "PollIntervalInSeconds";
            public const string HealthReportTTLInSeconds = "HealthReportTTLInSeconds";
            public const string ContinuousFailureWarningThreshold = "ContinuousFailureWarningThreshold";
            public const string ContinuousFailureErrorThreshold = "ContinuousFailureErrorThreshold";
            public const string ContinuousFailureFaultThreshold = "ContinuousFailureFaultThreshold";
            public const string PrimaryNodeTypes = "PrimaryNodeTypes";
            public const string NodeStatusBatchSize = "NodeStatusBatchSize";

            public const string EnableTStore = "EnableTStore"; 
            public const string ReplicatedStore = "ReplicatedStore";

            public const string CopyClusterPackageTimeoutInSeconds = "CopyClusterPackageTimeoutInSeconds";
            public const string ProvisionClusterPackageTimeoutInSeconds = "ProvisionClusterPackageTimeoutInSeconds";
            public const string StartClusterUpgradeTimeoutInSeconds = "StartClusterUpgradeTimeoutInSeconds";            
        };

        public static class ManagementSection
        {
            public const string ManagementSectionName = "Management";

            public const string ImageStoreConnectionString = "ImageStoreConnectionString";
        }

        public static class SecuritySection
        {
            public const string SecuritySectionName = "Security";

            public const string ServerCertThumbprints = "ServerCertThumbprints";
        }

        public static class ServerX509NamesSection
        {
            public const string ServerX509NamesSectionName = "Security/ServerX509Names";
        }

        public static class WUSCoordinator
        {
            public const string TestCabFolderParam = "TestCabFolder";
            public const string OnlyBaseUpgradeParam = "OnlyBaseUpgrade";
            public const string CategoryId = "c80cb2c8-6c51-4732-b615-f4762d0e9feb";
            public const string HealthProperty = "WUSCoordinator";

            public const int PollingIntervalInMinutesDefault = 15;
            public const int WuApiTimeoutInMinutesDefault = 30;            
        }

        public static class PaasCoordinator
        {
            public const string AnonymousCertDistiguishedName = "CN=AzureServiceFabric-AnonymousClient";
            public const string ApplicationUriSegment = "applications";

            public readonly static string HealthSourceId = "UpgradeService.Primary";
            public readonly static string SFRPPollHealthProperty = "SFRPPoll";
            public readonly static string SFRPStreamChannelHealthProperty = "SFRPStreamChannel";
            public readonly static string ClusterCommandProcessorHealthProperty  = "ClusterCommandProcessor";
        }

        public static class ApplicationCoordinator
        {
            public static TimeSpan DefaultAppPollIntervalInSeconds = TimeSpan.FromSeconds(60);
        }
    }
}
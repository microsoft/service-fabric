// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Collections.Generic;

    internal static class Constants
    {
        public const string EventStoreConnection = "EventStoreConnection";
        public const string TestabilityFabricClientName = "TestabiltyServiceFabricClient1";
        public const string ActionReliableDictionaryName = "ActionTable";
        public const string HistoryReliableDictionaryName = "HistoryTable";
        public const string PathToFabricCode = @"bin\Fabric\Fabric.Code";
        public const string FabricExe = "Fabric.exe";
        public const string FabricManagementServiceModelAssemblyName = "System.Fabric.Management.ServiceModel";
        public const string TargetInformationFileName = "TargetInformation.xml";

        public static readonly string StoreFolder = "Store";
        public static readonly string WindowsFabricStoreFolder = "WindowsFabricStore";

        #region System Service Constants

        public static readonly string SystemApplicationName = "fabric:/System";
        public static readonly Guid FmPartitionId = new Guid("00000000-0000-0000-0000-000000000001");

        public const string ClusterManagerServiceName       = "fabric:/System/ClusterManagerService";
        public const string DSTSTokenValidationServiceName  = "fabric:/System/DSTSTokenValidationService";
        public const string FailoverManagerServiceName      = "fabric:/System/FailoverManagerService";
        public const string FaultAnalysisServiceName        = "fabric:/System/FaultAnalysisService";
        public const string FileStoreServiceName            = "fabric:/System/FileStoreService";
        public const string ImageStoreServiceName           = "fabric:/System/ImageStoreService";
        public const string InfrastructureServiceName       = "fabric:/System/InfrastructureService";
        public const string NamespaceManagerServiceName     = "fabric:/System/NamespaceManagerService";
        public const string NamingServiceName               = "fabric:/System/NamingService";
        public const string RepairManagerServiceName        = "fabric:/System/RepairManagerService";
        public const string TokenValidationServiceName      = "fabric:/System/TokenValidationService";
        public const string UpgradeServiceName              = "fabric:/System/UpgradeService";
        public const string DnsServiceName                  = "fabric:/System/DnsService";
        public const string BackupRestoreService            = "fabric:/System/BackupRestoreService";
        public const string ResourceMonitorServce           = "fabric:/System/ResourceMonitorService";
        public const string CentralSecretService            = "fabric:/System/CentralSecretService";
        public const string LocalSecretService              = "fabric:/System/LocalSecretService";
        public const string EventStoreService               = "fabric:/System/EventStoreService";
        public const string GatewayResourceManager          = "fabric:/System/GatewayResourceManager";

        public static readonly List<Uri> SystemServiceList  = new List<Uri>()
                                                             {
                                                                 new Uri(ClusterManagerServiceName),
                                                                 new Uri(DSTSTokenValidationServiceName),
                                                                 new Uri(FailoverManagerServiceName),
                                                                 new Uri(FaultAnalysisServiceName),
                                                                 new Uri(FileStoreServiceName),
                                                                 new Uri(ImageStoreServiceName),
                                                                 new Uri(InfrastructureServiceName),
                                                                 new Uri(NamespaceManagerServiceName),
                                                                 new Uri(NamingServiceName),
                                                                 new Uri(RepairManagerServiceName),
                                                                 new Uri(TokenValidationServiceName),
                                                                 new Uri(UpgradeServiceName),
                                                                 new Uri(DnsServiceName),
                                                                 new Uri(BackupRestoreService),
                                                                 new Uri(ResourceMonitorServce),
                                                                 new Uri(CentralSecretService),
                                                                 new Uri(LocalSecretService),
                                                                 new Uri(EventStoreService),
                                                                 new Uri(GatewayResourceManager)
                                                             };

        #endregion

        // Approximate number of characters allowed in an Error message before truncation of the
        // message will occur in the JSON result. This number is below the limit.
        public const int ErrorMessageMaxCharLength = 460;

        #region Chaos Engine
        public const string PreAssertAmble = "Pre-assert snapshot...";
        public const string PostAssertAmble = "Post-assert snapshot...";

        public const string DummyFMCodePackageVersion = "-1.0.0.0";
        public const string DummyCMCodePackageVersion = "0.-1.0.0";
        public const string DummyNSCodePackageVersion = "0.0.-1.0";

        public const string NodeNameUsingIdPrefix = "nodeid:";

        public static readonly TimeSpan DefaultChaosSnapshotRecaptureBackoffInterval = TimeSpan.FromSeconds(60.0);
        #endregion
    }
}
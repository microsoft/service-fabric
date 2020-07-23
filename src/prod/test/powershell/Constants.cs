// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
namespace Microsoft.ServiceFabric.Powershell.Test
{
    internal static class Constants
    {
        public const string TestDllName = "Powershell";
        public const string TestCaseName = "PowershellTest";
        public const string PowershellAssemblyName = "Microsoft.ServiceFabric.Powershell.dll";

        public const string ConnectClusterCommand = "Connect-ServiceFabricCluster";
        public const string GetClusterConnectionCommand = "Get-ServiceFabricClusterConnection";
        public const string TestClusterConnectionCommand = "Test-ServiceFabricClusterConnection";
        public const string GetNodeCommand = "Get-ServiceFabricNode";
        public const string GetApplicationCommand = "Get-ServiceFabricApplication";
        public const string GetApplicationTypeCommand = "Get-ServiceFabricApplicationType";
        public const string GetApplicationTypePagedCommand = "Get-ServiceFabricApplicationTypePaged";
        public const string GetServiceCommand = "Get-ServiceFabricService";
        public const string GetServiceDescriptionCommand = "Get-ServiceFabricServiceDescription";
        public const string GetServiceGroupMemberCommand = "Get-ServiceFabricServiceGroupMember";
        public const string GetServiceGroupMemberTypeCommand = "Get-ServiceFabricServiceGroupMemberType";
        public const string GetPartitionCommand = "Get-ServiceFabricPartition";
        public const string GetReplicaCommand = "Get-ServiceFabricReplica";
        public const string TestApplicationPackageCommand = "Test-ServiceFabricApplicationPackage";
        public const string CopyApplicationPackageCommand = "Copy-ServiceFabricApplicationPackage";
        public const string RegisterApplicationTypeCommand = "Register-ServiceFabricApplicationType";
        public const string NewApplicationCommand = "New-ServiceFabricApplication";
        public const string UpdateApplicationCommand = "Update-ServiceFabricApplication";
        public const string RemoveApplicationCommand = "Remove-ServiceFabricApplication";
        public const string UnregisterApplicationTypeCommand = "Unregister-ServiceFabricApplicationType";
        public const string NewServiceCommand = "New-ServiceFabricService";
        public const string NewServiceGroupCommand = "New-ServiceFabricServiceGroup";
        public const string NewServiceFromTemplateCommand = "New-ServiceFabricServiceFromTemplate";
        public const string UpdateServiceCommand = "Update-ServiceFabricService";
        public const string RemoveServiceCommand = "Remove-ServiceFabricService";
        public const string RemoveServiceGroupCommand = "Remove-ServiceFabricServiceGroup";
        public const string StartApplicationUpgradeCommand = "Start-ServiceFabricApplicationUpgrade";
        public const string GetApplicationUpgradeCommand = "Get-ServiceFabricApplicationUpgrade";
        public const string ResumeApplicationUpgradeCommand = "Resume-ServiceFabricApplicationUpgrade";
        public const string InvokeEncryptSecretCommand = "Invoke-ServiceFabricEncryptSecret";
        public const string TestClusterManifestCommand = "Test-ServiceFabricClusterManifest";
        public const string RemoveNodeStateCommand = "Remove-ServiceFabricNodeState";
        public const string NewNodeConfigurationCommand = "New-ServiceFabricNodeConfiguration";
        public const string GetNodeHealthCommand = "Get-ServiceFabricNodeHealth";
        public const string GetApplicationHealthCommand = "Get-ServiceFabricApplicationHealth";
        public const string GetServiceHealthCommand = "Get-ServiceFabricServiceHealth";
        public const string GetReplicaHealthCommand = "Get-ServiceFabricReplicaHealth";
        public const string GetPartitionHealthCommand = "Get-ServiceFabricPartitionHealth";
        public const string GetClusterHealthCommand = "Get-ServiceFabricClusterHealth";
        public const string RemoveNodeConfigurationCommand = "Remove-ServiceFabricNodeConfiguration";
        public const string GetApplicationLoadInformationCommand = "Get-ServiceFabricApplicationLoadInformation";
        public const string RemoveApplicationPackageCommand = "Remove-ServiceFabricApplicationPackage";

        public const string RemoveReplicaCommand = "Remove-ServiceFabricReplica";
        public const string RestartReplicaCommand = "Restart-ServiceFabricReplica";

        public const string SendReplicaHealthReportCommand = "Send-ServiceFabricReplicaHealthReport";

        public static readonly Guid FmPartitionId = new Guid("{00000000-0000-0000-0000-000000000001}");
        public static readonly Uri SystemApplicationName = new Uri("fabric:/System");
        public static readonly Uri CmServiceName = new Uri("fabric:/System/ClusterManagerService");
        public static readonly Uri NamingServiceName = new Uri("fabric:/System/NamingService");
        public static readonly Uri FmServiceName = new Uri("fabric:/System/FailoverManagerService");
        public const string NamingServiceType = "NamingStoreService";
        public const string FmServiceType = "FMServiceType";
        public const string CmServiceType = "ClusterManagerServiceType";

        public const string TestRootPath = @"%_NTTREE%\FabricUnitTests\";
        public const string RingCertSetupFileName = "RingCertSetup.exe";
        public const string BaseScriptPath = @"%SDXROOT%\services\winfab\prod\Setup\scripts\";
        public const string MakeDropScriptFileName = "MakeDrop.cmd";
        public const string FabricDropPath = @"%_NTTREE%\FabricDrop";
        public const string BaseDrop = @"BaseDrop";
        public const string MakeDropArgumentPattern = "/dest:{0} /symbolsDestination:nul";
        public const string FabricCodePath = "FabricCodePath";
        
        public const int FabricNodeCount = 5;

        public const int MaxRetryCount = 10;
        public const double RetryIntervalInSeconds = 5;

        public const string FabricDataRoot = "FabricDataRoot";
        public const string FabricBinRoot = "FabricBinRoot";
        public const string FabricLogRoot = "FabricLogRoot";
        public const string FabricRoot = "FabricRoot";

        public const string TargetInformationFileName = "TargetInformation.xml";

        public const string ServiceControllerFileName = "sc.exe";
        public const string FabricHostSerivceName = "FabricHostSvc";
        public const string FabricHostFileName = "FabricHost.exe";

        public const string FabricClientPackageRelativePathToDrop = "ServiceFabricClientPackage\\ServiceFabric";

    }
}
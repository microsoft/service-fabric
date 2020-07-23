// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageBuilderExe
{
    internal static class StringConstants
    {        
        public static readonly string DefaultSchemaPath = "ServiceFabricServiceModel.xsd";
        public static readonly string ConfigurationsFileName = "Configurations.csv";

        //Command Line Arguments Key
        public static readonly string SchemaPath = "/schemaPath";
        public static readonly string WorkingDir = "/workingDir";
        public static readonly string Operation = "/operation";
        public static readonly string StoreRoot = "/storeRoot";
        public static readonly string AppName = "/appName";
        public static readonly string AppTypeName = "/appTypeName";
        public static readonly string AppTypeVersion = "/appTypeVersion";
        public static readonly string AppId = "/appId";
        public static readonly string NameUri = "/nameUri";
        public static readonly string AppParam = "/appParam";
        public static readonly string BuildPath = "/buildPath";
        public static readonly string DownloadPath = "/downloadPath";
        public static readonly string Output = "/output";
        public static readonly string Input = "/input";
        public static readonly string Progress = "/progress";
        public static readonly string ErrorDetails = "/errorDetails";
        public static readonly string SecuritySettingsParam = "/security";
        public static readonly string CurrentAppInstanceVersion = "/currentAppInstanceVersion";

        public static readonly string CurrentAppTypeVersion = "/currentAppTypeVersion";
        public static readonly string TargetAppTypeVersion = "/targetAppTypeVersion";
        public static readonly string RepositoryUserName = "/repoUserName";
        
        // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Identifier name")]
        public static readonly string RepositoryPassword = "/repoPwd";
        public static readonly string IsRepositoryPasswordEncrypted = "/pwdEncrypted";
        public static readonly string ComposeFilePath = "/cf";
        public static readonly string OverrideFilePath = "/of";
        public static readonly string OutputComposeFilePath = "/ocf";
        public static readonly string CleanupComposeFiles = "/cleanup";
        public static readonly string GenerateDnsNames = "/generateDnsNames";
        public static readonly string SFVolumeDiskServiceEnabled = "/sfVolumeDiskServiceEnabled";
        public static readonly string SingleInstanceApplicationDescription = "/singleInstanceApplicationDescription";
        public static readonly string UseOpenNetworkConfig = "/useOpenNetworkConfig";
        public static readonly string UseLocalNatNetworkConfig = "/useLocalNatNetworkConfig";
        public static readonly string GenerationConfig = "/generationConfig";
        public static readonly string MountPointForSettings = "/mountPointForSettings";

        public static readonly string CodePath = "/codePath";
        public static readonly string ConfigPath = "/configPath";
        public static readonly string ConfigVersion = "/configVersion";
        public static readonly string CurrentFabricVersion = "/currentFabricVersion";
        public static readonly string TargetFabricVersion = "/targetFabricVersion";
        public static readonly string InfrastructureManifestFile = "/im";
        public static readonly string Timeout = "/timeout";
        public static readonly string DisableChecksumValidation = "/disableChecksumValidation";
        public static readonly string DisableServerSideCopy = "/disableServerSideCopy";

        //Command Line Arguments Value
        public static readonly string OperationDownloadAndBuildApplicationType = "DownloadAndBuildApplicationType";
        public static readonly string OperationBuildApplicationTypeInfo = "BuildApplicationTypeInfo";
        public static readonly string OperationBuildApplicationType = "BuildApplicationType";
        public static readonly string OperationBuildApplication = "BuildApplication";        
        public static readonly string OperationUpgradeApplication = "UpgradeApplication";
        public static readonly string OperationValidateComposeDeployment = "ValidateComposeDeployment";
        public static readonly string OperationBuildComposeDeployment = "BuildComposeDeployment";
        public static readonly string OperationBuildComposeApplicationForUpgrade = "BuildComposeApplicationForUpgrade";
        public static readonly string OperationCleanupApplicationPackage = "CleanupApplicationPackage";
        public static readonly string OperationBuildSingleInstanceApplication = "BuildSingleInstanceApplication";
        public static readonly string OperationBuildSingleInstanceApplicationForUpgrade = "BuildSingleInstanceApplicationForUpgrade";

        public static readonly string OperationGetFabricVersion = "GetFabricVersion";
        public static readonly string OperationProvisionFabric = "ProvisionFabric";
        public static readonly string OperationUpgradeFabric = "UpgradeFabric";
        public static readonly string OperationGetManifests = "GetManifests";
        public static readonly string OperationGetClusterManifest = "GetClusterManifest";

        public static readonly string OperationDelete = "Delete";

        public static readonly string TestErrorDetails = "TestErrorDetails";
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Fabric.InfrastructureWrapper
{
    internal class Constants
    {
        public const string FabricFaultDomainPrefix = "fd:/RACK";
        public const string FabricUpgradeDomainPrefix = "UD";

        public const string FabricHostServiceName = "FabricHostSvc";
        public const string FabricInstallerServiceName = "FabricInstallerSvc";
        public const string FabricDeployerName = "FabricDeployer.exe";
        public const string FabricExeName = "Fabric.exe";

        public const string OneboxBaseIPTemplate = "127.255.0.{0}";

        public const string TargetInformationFileName = "TargetInformation.xml";

        public const string CurrentClusterManifestFileName = "ClusterManifest.current.xml";
        public const string FabricFolderName = "Fabric";

        public const string FabricDeployerParametersTemplate = "/operation:{0} /fabricBinRoot:\"{1}\" /fabricDataRoot:\"{2}\" /cm:\"{3}\" /im:\"{4}\"";
        public const string FabricDeployerParametersTemplateClusterManifestOnly = "/operation:{0} /fabricBinRoot:\"{1}\" /fabricDataRoot:\"{2}\" /cm:\"{3}\"";
        public const string ConfigureNodeParametersTemplate = "/operation:{0} /fabricDataRoot:\"{1}\" /cm:\"{2}\" /im:\"{3}\"";
        public const string ConfigureNodeParametersTemplateClusterManifestOnly = "/operation:{0} /fabricDataRoot:\"{1}\" /cm:\"{2}\"";
        public const string ConfigureNodeRunFabricHostServiceAsManualSwitch = "/serviceStartupType:Manual";

        public const string FabricInstalledRegPath = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Service Fabric";
        public const string FabricInstalledRegSubPath = "SOFTWARE\\Microsoft\\Service Fabric";
        public const string FabricInstalledRegPathDepreciated = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Fabric";
        public const string FabricInstalledRegSubPathDepreciated = "SOFTWARE\\Microsoft\\Windows Fabric";
        public const string FabricInstalledVersionRegKey = "FabricVersion";
        public static string FabricRootRegKey = "FabricRoot";
        public static string FabricBinRootRegKey = "FabricBinRoot";
        public static string FabricDataRootRegKey = "FabricDataRoot";
        public static string FabricCodePathRegKey = "FabricCodePath";

        public const string FabricV1InstalledVersionRegKey = "WinFabInstalled";
        public const string RegKeyV1Prefix = "1";

        public const string NodeConfigurationCompletedRegKey = "NodeConfigurationCompleted";
        public const string DynamicTopologyKindRegKey = "DynamicTopologyKind";

        public class EnvironmentVariables
        {
            public const string FabricBinRoot = "FabricBinRoot";
            public const string FabricDataRoot = "FabricDataRoot";
            public const string FabricRoot = "FabricRoot";
            public const string FabricCodePath = "FabricCodePath";
        }

        public static class FabricVersions
        {
            public const string CodeVersionPattern = "{0}.{1}.{2}.{3}";
        }

        public const string FabricPackageCabinetFile = @"MicrosoftServiceFabric.Internal.cab";
        public const string FabricRootPath = @"%systemdrive%\Program Files\Microsoft Service Fabric";
        public const string PathToFabricCode = @"bin\Fabric\Fabric.Code";
        public const string UnCabOptionForAllFiles = @"""{0}""  -F:* ""{1}""";

        public const int FabricInstallerServiceStartTimeoutInMinutes = 3;
        public const int FabricInstallerServiceHostCreationTimeoutInMinutes = 20;
        public const int FabricHostAwaitRunningTimeoutInMinutes = 10;
    }
}
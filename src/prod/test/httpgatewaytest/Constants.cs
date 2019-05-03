// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Security.Cryptography.X509Certificates;

namespace System.Fabric.Test.HttpGatewayTest
{
    internal static class Constants
    {
        // Executables.
        public const string FabricHostFileName = "FabricHost.exe";
        public const string FabricDeployerFileName = @"FabricDeployer.exe";

        public const string FabricDropPath = @"%_NTTREE%\FabricDrop";

        public const string FabricUnitTestsSystemAppsPath = @"%_NTTREE%\FabricUnitTests\__FabricSystem_App4294967295";

        /// <summary>
        /// There exists 2 configurations of the app manifest and <see cref="ISPackageManifestFileName"/> for InfrastructureService (IS).
        /// One for production and one for test.
        /// In production environment, IS needs to run in System context. However, for local box testing, it needs to run in 
        /// NetworkService context (as there are other problems around the test infrastructure which doesn't allow IS to be
        /// launched as System).
        /// Hence, after the fabric drop is created for running this test, these xml files overwrite the fabric drop xml files
        /// with the test configurations of these files copied from FabricUnitTests
        /// </summary>
        /// <remarks>
        /// Please also see comments inside these files in %srcroot%\prod\src\Management\fabricsystemapppackage\__fabricsystem_app4294967295
        /// </remarks>
        public const string ISAppManifestFileName = "App.1.0.xml";
        public const string ISPackageManifestFileName = "IS.Package.1.0.xml";

        /// <summary>
        /// Path to overwrite the test version of the InfrastructureService manifests for running this test.
        /// <see cref="ISAppManifestFileName"/>
        /// </summary>
        public const string DestinationPathForISManifests = @"bin\Fabric\Fabric.Code\__FabricSystem_App4294967295";

        public const string LogmanFileName = @"logman";

        // Directories
        public const string DropDirectory = "HttpGatewayTest.Drop";
        public const string BinDirectory = "bin";
        public const string DataDirectory = "Data";
        public const string ScaleMinDirectory = "ScaleMin";
        public const string FabricAppDirectory = @"bin\Fabric";
        public const string FabricCodePattern = "Fabric.Code";

        // Arguments.
        public const string FabricHostArgument = "-c -activateHidden -skipfabricsetup";
        public const string MakeDropArgumentPattern = "/dest:{0} /symbolsDestination:nul";
        public const string FabricDeployerArgumentPattern = @"/operation:Create /fabricBinRoot:{0} /fabricDataRoot:{1} /cm:{2}";
        public const string LogmanStopFabricCounters = @"stop FabricCounters";
        public const string LogmanDeleteFabricCounters = @"delete FabricCounters";

        // ClusterManifest names.
        public const string ScaleMinNoSecurityClusterManifestName = @"%_NTTREE%\FabricUnitTests\DevEnv-FiveNodes-HttpGatewayEnabled.xml";
        public const string ScaleMinCertSecurityClusterManifestName = @"%_NTTREE%\FabricUnitTests\DevEnv-FiveNodes-CertSecurity-HttpGatewayEnabled.xml";
        public const string ScaleMinKerberosSecurityClusterManifestName = @"%_NTTREE%\FabricUnitTests\DevEnv-FiveNodes-Kerberos-HttpGatewayEnabled.xml";
        public const string FabricNamespace = "http://schemas.microsoft.com/2011/01/fabric";

        public const string ServerAllowedCommonName = "WinFabricRing.Rings.WinFabricTestDomain.com";
        public const string SecuritySectionName = "Security";
        public const string ClaimsAuthEnabledParamName = "ClientClaimAuthEnabled";
        public const string HSTSHeaderParamName = "HttpStrictTransportSecurityHeader";
        public const string HttpGatewaySectionName = "HttpGateway";

        // ImageStore related constants.
        public const string ImageStoreDirectory = DropDirectory + @"\Data\ImageStore";
        public const string IncomingDirectory = @"Incoming";
        public const string ManagementSectionName = "Management";
        public const string ImageStoreConnectionStringParameterName = "ImageStoreConnectionString";
        public const string DefaultTag = "_default_";

        // Application related constants.
        public const string TestApplicationName = "MyAppName with space";
        public const string TestApplicationTypeName = "TestApplicationType";
        public const int ExpectedFileCount = 5;

        // Logman exit codes
        public const int LogmanSuccessExitCode = 0;
        public const int LogmanDataCollectorNotRunningExitCode = -2144337660;
        public const int LogmanDataCollectorNotFoundExitCode = -2144337918;

        public const string FabricBinRoot = "FabricBinRoot";
        public const string FabricCodePath = "FabricCodePath";
        public const string FabricDataRoot = "FabricDataRoot";
        public const string FabricRoot = "FabricRoot";
        public const string FabricLogRoot = "FabricLogRoot";

        // Details of the user certificate that is setup by RingCertSetup
        public const string UserCertificateThumbprint = "6f 4a a9 61 8a ea 95 ab 5d ce 8c 77 26 09 38 65 3e e1 5f d7";
        public const string AdminCertificateThumbprint = "d5 ec 42 3b 79 cb e5 07 fd 83 59 3c 56 b9 d5 31 24 25 42 64";
        public const StoreLocation clientCertificateLocation = StoreLocation.LocalMachine;
        public const StoreName clientCertificateStoreName = StoreName.My;

        public static readonly Guid FmPartitionId = new Guid("{00000000-0000-0000-0000-000000000001}");
    }
}
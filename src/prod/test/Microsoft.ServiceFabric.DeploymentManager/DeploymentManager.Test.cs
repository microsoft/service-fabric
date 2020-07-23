// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Test
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Xml;
    using System.Xml.Serialization;
    using System.Fabric.UpgradeOrchestration.Service;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Microsoft.ServiceFabric.DeploymentManager.Model;
    using Newtonsoft.Json;

    /// <summary>
    /// This is the class that tests the validation of the files that we deploy.
    /// </summary>
    [TestClass]
    internal class MicrosoftServiceFabricDeploymentManagerTests
    {
        internal static string BaseDir;

        static readonly string XmlManifestFilename = "Development-Nine-Node-Server.xml";
        static readonly string JsonConfigUnsecureGAFilename = "ClusterConfig.Unsecure.DevCluster.GA.json";
        static readonly string JsonConfigX509SecureGAFilename = "ClusterConfig.X509.DevCluster.GA.json";
        static readonly string JsonConfigWindowsSecureGAFilename = "ClusterConfig.Windows.DevCluster.GA.json";
        static readonly string JsonConfigUnsecureJan2017Filename = "ClusterConfig.Unsecure.DevCluster.Jan2017.json";
        static readonly string JsonConfigX509SecureJan2017Filename = "ClusterConfig.X509.DevCluster.Jan2017.json";
        static readonly string JsonConfigWindowsSecureJan2017Filename = "ClusterConfig.Windows.DevCluster.Jan2017.json";
        static readonly string JsonConfigWindowsSecureMultiBoxJan2017Filename = "ClusterConfig.Windows.MultiMachine.Jan2017.json";
        static readonly string JsonConfigX509SecureIssuerCNOct2017Filename = "ClusterConfig.X509.DevCluster.IssuerCN.Oct2017.json";
        static readonly string JsonConfigWindowsGMSAInvalidOct2017Filename = "ClusterConfig.InvalidWindowsGMSA.Oct2017.json";
        static readonly string JsonConfigX509SecureOptionalIssuerTPOct2017Filename = "ClusterConfig.X509.DevCluster.OptionalIssuerTP.Oct2017.json";
        static readonly string JsonConfigMultiboxFilename = "ClusterConfig.Unsecure.MultiMachine.json";
        static readonly string JsonConfigCodeUpgradeJune2018Filename = "ClusterConfig.Windows.MultiMachine.June2018.V1.json";
        static readonly string JsonConfigCodeUpgradeInvalidJune2018Filename = "ClusterConfig.Windows.MultiMachine.InvalidJune2018.json";
        static readonly string GoalStateFilename = "goalstate.json";
        static readonly string OldVersion = "5.3.301.9590";

        static readonly string[] unsupportedApiVersionJsonConfigFileNames = new string[]
        {
            "UnsupportedApiVersion.noVer.json",
            "UnsupportedApiVersion.unsupportedVer.json",
        };

        static readonly string[] portCollissionJsonConfigFileNames = new string[]
        {
            "NodesSameMachine.SameNodeType.json",
            "NodesSameMachine.PortCollision.json",
            "NodesSameMachine.PortRangeCollision.json",
        };

        [ClassInitialize]
        public static void ClassInitialize(TestContext context)
        {
            MicrosoftServiceFabricDeploymentManagerTests.BaseDir = context.TestDeploymentDir;
            SetTestMode();
        }

        static void SetTestMode()
        {
            Environment.SetEnvironmentVariable(Microsoft.ServiceFabric.DeploymentManager.Constants.FabricUnitTestNoTraceName, "1");
        }

        [TestCleanup]
        public void Cleanup()
        {

        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("maburlik")]
        public void DeploymentManagerTest_GetDevMachinesIsLocalhost()
        {
            string manifestPath = Path.Combine(BaseDir, XmlManifestFilename);
            Assert.IsTrue(File.Exists(manifestPath), string.Format("Manifest file {0} did not exist.", manifestPath));
            List<string> machines = StandaloneUtility.GetMachineNamesFromClusterManifest(manifestPath);
            Assert.IsNotNull(machines);
            Assert.AreEqual(machines.Count, 1);
            Assert.IsTrue("localhost".CompareTo(machines[0]) == 0,
                "Expected to find the machine localhost, but found {0} instead", machines[0]);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("ratando")]
        public void DeploymentManagerTest_ValidateIncorrectJsonConfig()
        {
            string jsonFilePath = Path.Combine(BaseDir, XmlManifestFilename);
            var configModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(jsonFilePath);
            Assert.IsTrue(File.Exists(jsonFilePath), string.Format("Manifest file {0} did not exist.", jsonFilePath));
            Assert.IsNull(configModel, "Cluster manifest should not have been valid JSON.");

            foreach (string fileName in unsupportedApiVersionJsonConfigFileNames)
            {
                jsonFilePath = Path.Combine(BaseDir, fileName);
                Assert.IsTrue(File.Exists(jsonFilePath));
                configModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(jsonFilePath);
                Assert.IsNull(configModel);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("alsantam")]
        public void DeploymentManagerTest_ValidateJsonConfigNodeTypePortCollision()
        {
            string jsonFilePath = Path.Combine(BaseDir, JsonConfigUnsecureGAFilename);
            var configModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(jsonFilePath);
            Assert.IsTrue(File.Exists(jsonFilePath), string.Format("Config file {0} did not exist.", jsonFilePath));
            configModel.ValidateModel();

            foreach (string fileName in portCollissionJsonConfigFileNames)
            {
                jsonFilePath = Path.Combine(BaseDir, fileName);
                Assert.IsTrue(File.Exists(jsonFilePath), string.Format("Config file {0} did not exist.", jsonFilePath));
                configModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(jsonFilePath);

                Utility.ValidateExpectedValidationException(
                    delegate
                    {
                        configModel.ValidateModel();
                    },
                    ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("ratando")]
        public void DeploymentManagerTest_ValidateJsonConfigUnsecureGA()
        {
            string clusterConfigPath = Path.Combine(BaseDir, JsonConfigUnsecureGAFilename);
            var configModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(clusterConfigPath);
            Assert.IsTrue(File.Exists(clusterConfigPath), string.Format("Config file {0} did not exist.", clusterConfigPath));
            Assert.IsNotNull(configModel, "JSON config was invalid.");

            var userConfig = configModel.GetUserConfig();
            Assert.IsNull(userConfig.Security, "Security must be null.");

            var clusterTopology = configModel.GetClusterTopology();
            Assert.IsNotNull(clusterTopology.Nodes);
            Assert.IsNotNull(clusterTopology.Machines);
            Assert.IsTrue(clusterTopology.Nodes.Count > 0, "Cluster JSON config was not written correctly. Nodes missing.");
            Assert.IsTrue(clusterTopology.Machines.Count > 0, "Cluster JSON config was not written correctly. Machines missing.");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("ratando")]
        public void DeploymentManagerTest_ValidateJsonConfigX509SecureGA()
        {
            string clusterConfigPath = Path.Combine(BaseDir, JsonConfigX509SecureGAFilename);
            var configModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(clusterConfigPath);
            Assert.IsTrue(File.Exists(clusterConfigPath), string.Format("Config file {0} did not exist.", clusterConfigPath));
            Assert.IsNotNull(configModel, "JSON config was invalid.");

            var userConfig = configModel.GetUserConfig();
            Assert.IsNotNull(userConfig.Security.CertificateInformation, "X509 Certificate information must not be null");
            Assert.IsNotNull(userConfig.Security.ClusterCredentialType == Microsoft.ServiceFabric.ClusterManagementCommon.CredentialType.X509, "ClusterCredentialType must be X509.");

            var clusterTopology = configModel.GetClusterTopology();
            Assert.IsNotNull(clusterTopology.Nodes);
            Assert.IsNotNull(clusterTopology.Machines);
            Assert.IsTrue(clusterTopology.Nodes.Count > 0, "Cluster JSON config was not written correctly. Nodes missing.");
            Assert.IsTrue(clusterTopology.Machines.Count > 0, "Cluster JSON config was not written correctly. Machines missing.");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("ratando")]
        public void DeploymentManagerTest_ValidateJsonConfigWindowsSecureGA()
        {
            string clusterConfigPath = Path.Combine(BaseDir, JsonConfigWindowsSecureGAFilename);
            var configModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(clusterConfigPath);
            Assert.IsTrue(File.Exists(clusterConfigPath), string.Format("Config file {0} did not exist.", clusterConfigPath));
            Assert.IsNotNull(configModel, "JSON config was invalid.");

            var userConfig = configModel.GetUserConfig();
            Assert.IsNotNull(userConfig.Security.WindowsIdentities, "Windows Identities should be provided.");
            Assert.IsNotNull(userConfig.Security.ClusterCredentialType == Microsoft.ServiceFabric.ClusterManagementCommon.CredentialType.Windows, "ClusterCredentialType must be Windows.");

            var clusterTopology = configModel.GetClusterTopology();
            Assert.IsNotNull(clusterTopology.Nodes);
            Assert.IsNotNull(clusterTopology.Machines);
            Assert.IsTrue(clusterTopology.Nodes.Count > 0, "Cluster JSON config was not written correctly. Nodes missing.");
            Assert.IsTrue(clusterTopology.Machines.Count > 0, "Cluster JSON config was not written correctly. Machines missing.");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("ratando")]
        public void DeploymentManagerTest_ValidateJsonConfigUnsecureJan2017()
        {
            string clusterConfigPath = Path.Combine(BaseDir, JsonConfigUnsecureJan2017Filename);
            var configModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(clusterConfigPath);
            Assert.IsTrue(File.Exists(clusterConfigPath), string.Format("Config file {0} did not exist.", clusterConfigPath));
            Assert.IsNotNull(configModel, "JSON config was invalid.");

            var userConfig = configModel.GetUserConfig();
            Assert.IsNull(userConfig.Security, "Security must be null.");

            var clusterTopology = configModel.GetClusterTopology();
            Assert.IsNotNull(clusterTopology.Nodes);
            Assert.IsNotNull(clusterTopology.Machines);
            Assert.IsTrue(clusterTopology.Nodes.Count > 0, "Cluster JSON config was not written correctly. Nodes missing.");
            Assert.IsTrue(clusterTopology.Machines.Count > 0, "Cluster JSON config was not written correctly. Machines missing.");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("ratando")]
        public void DeploymentManagerTest_ValidateJsonConfigX509SecureJan2017()
        {
            string clusterConfigPath = Path.Combine(BaseDir, JsonConfigX509SecureJan2017Filename);
            var configModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(clusterConfigPath);
            Assert.IsTrue(File.Exists(clusterConfigPath), string.Format("Config file {0} did not exist.", clusterConfigPath));
            Assert.IsNotNull(configModel, "JSON config was invalid.");

            var userConfig = configModel.GetUserConfig();
            Assert.IsNotNull(userConfig.Security.CertificateInformation, "X509 Certificate information must not be null");
            Assert.IsNotNull(userConfig.Security.ClusterCredentialType == Microsoft.ServiceFabric.ClusterManagementCommon.CredentialType.X509, "ClusterCredentialType must be X509.");

            var clusterTopology = configModel.GetClusterTopology();
            Assert.IsNotNull(clusterTopology.Nodes);
            Assert.IsNotNull(clusterTopology.Machines);
            Assert.IsTrue(clusterTopology.Nodes.Count > 0, "Cluster JSON config was not written correctly. Nodes missing.");
            Assert.IsTrue(clusterTopology.Machines.Count > 0, "Cluster JSON config was not written correctly. Machines missing.");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("ratando")]
        public void DeploymentManagerTest_ValidateJsonConfigWindowsSecureJan2017()
        {
            string clusterConfigPath = Path.Combine(BaseDir, JsonConfigWindowsSecureJan2017Filename);
            var configModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(clusterConfigPath);
            Assert.IsTrue(File.Exists(clusterConfigPath), string.Format("Config file {0} did not exist.", clusterConfigPath));
            Assert.IsNotNull(configModel, "JSON config was invalid.");

            var userConfig = configModel.GetUserConfig();
            Assert.IsNotNull(userConfig.Security.WindowsIdentities, "Windows Identities should be provided.");
            Assert.IsNotNull(userConfig.Security.WindowsIdentities.ClustergMSAIdentity, "Cluster gMSA should be provided.");
            Assert.IsNotNull(userConfig.Security.ClusterCredentialType == Microsoft.ServiceFabric.ClusterManagementCommon.CredentialType.Windows, "ClusterCredentialType must be Windows.");

            var clusterTopology = configModel.GetClusterTopology();
            Assert.IsNotNull(clusterTopology.Nodes);
            Assert.IsNotNull(clusterTopology.Machines);
            Assert.IsTrue(clusterTopology.Nodes.Count > 0, "Cluster JSON config was not written correctly. Nodes missing.");
            Assert.IsTrue(clusterTopology.Machines.Count > 0, "Cluster JSON config was not written correctly. Machines missing.");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("ratando")]
        public void DeploymentManagerTest_InvalidJsonConfigWindowsGMSAOct2017()
        {
            string clusterConfigPath = Path.Combine(BaseDir, JsonConfigWindowsGMSAInvalidOct2017Filename);
            var configModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(clusterConfigPath);
            Assert.IsTrue(File.Exists(clusterConfigPath), string.Format("Config file {0} did not exist.", clusterConfigPath));
            Assert.IsNotNull(configModel, "JSON config was invalid.");

            var settingsValidator = new StandaloneSettingsValidator(configModel);
            Utility.ValidateExpectedValidationException(
                delegate
                {
                    settingsValidator.Validate(false);
                },
                ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("ratando")]
        public void DeploymentManagerTest_ValidateJsonConfigMultiBoxWindowsSecureJan2017()
        {
            string clusterConfigPath = Path.Combine(BaseDir, JsonConfigWindowsSecureMultiBoxJan2017Filename);
            var configModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(clusterConfigPath);
            Assert.IsTrue(File.Exists(clusterConfigPath), string.Format("Config file {0} did not exist.", clusterConfigPath));
            Assert.IsNotNull(configModel, "JSON config was invalid.");

            var userConfig = configModel.GetUserConfig();
            Assert.IsNotNull(userConfig.Security.WindowsIdentities, "Windows Identities should be provided.");
            Assert.IsNotNull(userConfig.Security.WindowsIdentities.ClustergMSAIdentity, "Cluster gMSA should be provided.");
            Assert.IsNotNull(userConfig.Security.ClusterCredentialType == Microsoft.ServiceFabric.ClusterManagementCommon.CredentialType.Windows, "ClusterCredentialType must be Windows.");

            var clusterTopology = configModel.GetClusterTopology();
            Assert.IsNotNull(clusterTopology.Nodes);
            Assert.IsNotNull(clusterTopology.Machines);
            Assert.IsTrue(clusterTopology.Nodes.Count > 0, "Cluster JSON config was not written correctly. Nodes missing.");
            Assert.IsTrue(clusterTopology.Machines.Count > 0, "Cluster JSON config was not written correctly. Machines missing.");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("ratando")]
        public void DeploymentManagerTest_ValidateJsonConfigIssuerCertStoreOctober2017()
        {
            string clusterConfigPath = Path.Combine(BaseDir, JsonConfigX509SecureIssuerCNOct2017Filename);
            var configModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(clusterConfigPath);
            Assert.IsTrue(File.Exists(clusterConfigPath), string.Format("Config file {0} did not exist.", clusterConfigPath));
            Assert.IsNotNull(configModel, "JSON config was invalid.");

            var userConfig = configModel.GetUserConfig();
            Assert.IsTrue(userConfig.Security.CertificateInformation.ClusterCertificateIssuerStores.Count()==2, "ClusterCertificateIssuerStore count should be 2.");
            Assert.IsTrue(userConfig.Security.CertificateInformation.ServerCertificateIssuerStores.Count()==1, "ServerCertificateIssuerStore count should be 1.");
            Assert.IsTrue(userConfig.Security.CertificateInformation.ClientCertificateIssuerStores.Count()==1, "ClientCertificateIssuerStore count should be 1.");

            var clusterResource = DeploymentManagerInternal.GetClusterResource(clusterConfigPath, System.Guid.NewGuid().ToString());
            var cm = clusterResource.Current.ExternalState.ClusterManifest;
            var clusterCertificateIssuerStore = cm.FabricSettings.ToList().Where(section => section.Name == StringConstants.SectionName.SecurityClusterCertificateIssuerStores).ToList();
            Assert.IsTrue(clusterCertificateIssuerStore.Count() == 1, "clusterCertificateIssuerStore count should be 1");
            Assert.IsTrue(clusterCertificateIssuerStore[0].Parameter.ToList().Count == 2, "clusterCertificateIssuerStore parameter count should be 2");
            Assert.AreEqual(clusterCertificateIssuerStore[0].Parameter[0].Name, "");
            Assert.AreEqual(clusterCertificateIssuerStore[0].Parameter[0].Value, "My");
            Assert.AreEqual(clusterCertificateIssuerStore[0].Parameter[1].Name, "ClusterIssuer");
            Assert.AreEqual(clusterCertificateIssuerStore[0].Parameter[1].Value, "My, Root");

            var serverCertificateIssuerStore = cm.FabricSettings.ToList().Where(section => section.Name == StringConstants.SectionName.SecurityServerCertificateIssuerStores).ToList();
            Assert.IsTrue(serverCertificateIssuerStore.Count() == 1, "serverCertificateIssuerStore count should be 1");
            Assert.IsTrue(serverCertificateIssuerStore[0].Parameter.ToList().Count == 1, "serverCertificateIssuerStore parameter count should be 1");
            Assert.AreEqual(serverCertificateIssuerStore[0].Parameter[0].Name, "ServerIssuer");
            Assert.AreEqual(serverCertificateIssuerStore[0].Parameter[0].Value, "My");

            var clientCertificateIssuerStore = cm.FabricSettings.ToList().Where(section => section.Name == StringConstants.SectionName.SecurityClientCertificateIssuerStores).ToList();
            Assert.IsTrue(clientCertificateIssuerStore.Count() == 1, "clientCertificateIssuerStore count should be 1");
            Assert.IsTrue(clientCertificateIssuerStore[0].Parameter.ToList().Count == 1, "clientCertificateIssuerStore parameter count should be 1");
            Assert.AreEqual(clientCertificateIssuerStore[0].Parameter[0].Name, "ClientIssuer");
            Assert.AreEqual(clientCertificateIssuerStore[0].Parameter[0].Value, "My");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("jkochhar")]
        public void DeploymentManagerTest_ValidateOptionalCertificateIssuerThumbprint()
        {
            string clusterConfigPath = Path.Combine(BaseDir, JsonConfigX509SecureOptionalIssuerTPOct2017Filename);
            string clusterManifestPath = Path.Combine(Path.GetTempPath(), "cmtest.xml");
            var clusterResource = DeploymentManagerInternal.GetClusterResource(clusterConfigPath, System.Guid.NewGuid().ToString());
            var cm = clusterResource.Current.ExternalState.ClusterManifest;
            Assert.IsNotNull(cm, "Cluster manifest was null.");
            XMLHelper.WriteXmlExclusive<ClusterManifestType>(clusterManifestPath, cm);
            Assert.IsTrue(File.Exists(clusterManifestPath), "Cluster manifest was not written.");

            ClusterManifestType manifest = XMLHelper.ReadClusterManifest(clusterManifestPath);
            Assert.IsNotNull(manifest, "Cluster manifest was null.");
            var fabricSettings = manifest.FabricSettings;
            var securityAdminClientSetting = fabricSettings.Where<SettingsOverridesTypeSection>(p => p.Name == StringConstants.SectionName.SecurityAdminClientX509Names).First();
            foreach (var param in securityAdminClientSetting.Parameter)
            {
                Assert.IsNotNull(param.Value, param.Name + " Value was null");
            }

            var securityClusterSetting = fabricSettings.Where<SettingsOverridesTypeSection>(p => p.Name == StringConstants.SectionName.SecurityClusterX509Names).First();
            foreach (var param in securityClusterSetting.Parameter)
            {
                Assert.IsNotNull(param.Value, param.Name + " Value was null");
            }

            var securityServerSetting = fabricSettings.Where<SettingsOverridesTypeSection>(p => p.Name == StringConstants.SectionName.SecurityServerX509Names).First();
            foreach (var param in securityServerSetting.Parameter)
            {
                Assert.IsNotNull(param.Value, param.Name + " Value was null");
            }

            var securityClientSetting = fabricSettings.Where<SettingsOverridesTypeSection>(p => p.Name == StringConstants.SectionName.SecurityClientX509Names).First();
            foreach (var param in securityClientSetting.Parameter)
            {
                Assert.IsNotNull(param.Value, param.Name + " Value was null");
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("maburlik")]
        public void DeploymentManagerTest_ConvertJsonToXml()
        {
            Console.WriteLine("Current directory: {0}", Environment.CurrentDirectory);
            string clusterConfigPath = Path.Combine(BaseDir, JsonConfigUnsecureJan2017Filename);
            Directory.SetCurrentDirectory(BaseDir);
            Console.WriteLine("New current directory changed to: {0}", Environment.CurrentDirectory);
            Assert.IsTrue(File.Exists(clusterConfigPath), string.Format("Config file {0} did not exist.", clusterConfigPath));
            string clusterManifestPath = Path.Combine(Path.GetTempPath(), "cmtest.xml");
            var clusterResource = DeploymentManagerInternal.GetClusterResource(clusterConfigPath, System.Guid.NewGuid().ToString());
            var cm = clusterResource.Current.ExternalState.ClusterManifest;
            Assert.IsNotNull(cm, "Cluster manifest was null.");
            XMLHelper.WriteXmlExclusive<ClusterManifestType>(clusterManifestPath, cm);
            Assert.IsTrue(File.Exists(clusterManifestPath), "Cluster manifest was not written.");
            List<string> machines = StandaloneUtility.GetMachineNamesFromClusterManifest(clusterManifestPath);
            Assert.IsNotNull(machines);
            Assert.IsTrue(machines.Count > 0, "Cluster manifest was not written correctly. Machines missing.");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("maburlik")]
        public void DeploymentManagerTest_DefaultUosConfigValuesScalemin()
        {
            string clusterConfigPath = Path.Combine(BaseDir, JsonConfigUnsecureJan2017Filename);
            Assert.IsTrue(File.Exists(clusterConfigPath), string.Format("Config file {0} did not exist.", clusterConfigPath));
            var configModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(clusterConfigPath);
            var userConfig = configModel.GetUserConfig();
            Assert.IsTrue(userConfig.AutoupgradeEnabled == false, "Onebox AutoupgradeEnabled should be defaulted to false.");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("maburlik")]
        public void DeploymentManagerTest_DefaultUosConfigValuesMultibox()
        {
            string clusterConfigPath = Path.Combine(BaseDir, JsonConfigMultiboxFilename);
            Assert.IsTrue(File.Exists(clusterConfigPath), string.Format("Config file {0} did not exist.", clusterConfigPath));
            var configModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(clusterConfigPath);
            var userConfig = configModel.GetUserConfig();
            Assert.IsTrue(userConfig.AutoupgradeEnabled == true, "Multibox AutoupgradeEnabled should be defaulted to false.");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("maburlik")]
        public void DeploymentManagerTest_GoalStateDefaults()
        {
            // Create empty package in goal state to test default values
            string emptyPackageDefaults = "{\"packages\": [{}]}";
            GoalStateModel model = JsonConvert.DeserializeObject<GoalStateModel>(emptyPackageDefaults, new JsonSerializerSettings() { DefaultValueHandling = DefaultValueHandling.Populate });
            PackageDetails pd = model.Packages.First();
            Assert.IsTrue(pd.IsUpgradeDisabled == false, "IsUpgradeDisabled should be defaulted to false.");
            Assert.IsTrue(pd.IsDowngradeDisabled == false, "IsDowngradeDisabled should be defaulted to false.");
            Assert.IsTrue(pd.IsGoalPackage == false, "IsGoalPackage should be defaulted to false.");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Ignore]
        [Owner("maburlik")]
        public void DeploymentManagerTest_ProductionGoalState()
        {
            Console.WriteLine("test begins: {0}", DateTime.UtcNow);

            string goalStateFilepath = Path.Combine(BaseDir, GoalStateFilename);
            Assert.IsTrue(File.Exists(goalStateFilepath), string.Format("GoalStateFile {0} did not exist.", goalStateFilepath));
            string goalStateJson = File.ReadAllText(goalStateFilepath);
            GoalStateModel model = GoalStateModel.GetGoalStateModelFromJson(goalStateJson);
            Assert.IsTrue(model != null, "Goal state JSON could not be deserialized.");
            Assert.IsTrue(model.Packages != null, "Goal state model must have a packages element.");

            if (model.Packages.Count() == 0)
            {
                return;
            }

            // There can only be one goal package
            int countGoalpackages = model.Packages.Where(package => package.IsGoalPackage).Count();
            Assert.IsTrue(countGoalpackages == 1, "Exactly one goal package may be marked as IsGoalPackage in the goal state file.");

            // Validate each package link is reachable
            string errorMessage = string.Empty;
            foreach (PackageDetails package in model.Packages)
            {
                Uri packageUri;
                if (!Uri.TryCreate(package.TargetPackageLocation, UriKind.Absolute, out packageUri))
                {
                    errorMessage = string.Format("Cannot parse packageUri: {0}", package.TargetPackageLocation);
                    Assert.Fail(errorMessage);
                }

                if (!StandaloneUtility.IsUriReachable(packageUri))
                {
                    errorMessage = string.Format("Package uri is not reachable: {0}", packageUri.AbsoluteUri);
                }
            }

            if (!string.IsNullOrEmpty(errorMessage))
            {
                Assert.Fail(errorMessage);
            }

            // Validate that goal state downloaded package is the correct version
            string packageDropDir = System.Fabric.Common.Helpers.GetNewTempPath();
            PackageDetails goalPackage = model.Packages.First(package => package.IsGoalPackage);
            Assert.IsTrue(goalPackage != null, "goalPackage null");

            // Validate package downloadable
            Console.WriteLine("download begins for {0}: {1}", goalPackage.Version, DateTime.UtcNow);
            string packageLocalDownloadPath = Path.Combine(packageDropDir, string.Format(Microsoft.ServiceFabric.DeploymentManager.Constants.SFPackageDropNameFormat, goalPackage.Version));
            bool packageDownloaded = StandaloneUtility.DownloadPackageAsync(goalPackage.Version, goalPackage.TargetPackageLocation, packageLocalDownloadPath).Result;
            Assert.IsTrue(packageDownloaded, string.Format("Package {0} failed to download from {1}.", goalPackage.Version, goalPackage.TargetPackageLocation));

            // Validate downloaded package matches version of version in goal state file
            string cabVersion = CabFileOperations.GetCabVersion(packageLocalDownloadPath);
            Assert.IsTrue(goalPackage.Version == cabVersion, string.Format("Goal state package version {0} doesn't match downloaded package version {1}.", goalPackage.Version, cabVersion));

            if (Directory.Exists(packageDropDir))
            {
                try
                {
                    FabricDirectory.Delete(packageDropDir, true, true);
                }
                catch { }
            }

            Console.WriteLine("test ends: {0}", DateTime.UtcNow);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void DeploymentManagerTest_GetRuntimeVersions()
        {
            List<RuntimePackageDetails> latestPackage = DeploymentManager.GetDownloadableRuntimeVersions(DownloadableRuntimeVersionReturnSet.Latest);
            Assert.IsNotNull(latestPackage, "Latest version returned was null.");
            Assert.AreEqual(1, latestPackage.Count());
            Assert.IsNull(latestPackage[0].SupportExpiryDate, "Goal package has a non-null expiry date.");
            Assert.IsNotNull(latestPackage[0].Version, "Goal package does not contain any version");
            Assert.IsNotNull(latestPackage[0].TargetPackageLocation, "Goal package does not contain target location.");

            List<RuntimePackageDetails> supportedPackages = DeploymentManager.GetDownloadableRuntimeVersions(DownloadableRuntimeVersionReturnSet.Supported);
            foreach (RuntimePackageDetails package in supportedPackages)
            {
                Assert.IsNotNull(package, "Supported packages list contains a null package.");
                Assert.IsNotNull(package.Version, "Supported package does not contain any version");
                Assert.IsNotNull(package.TargetPackageLocation, "Supported package does not contain target location.");
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void DeploymentManagerTest_GetRuntimeUpgradeVersions()
        {
            List<RuntimePackageDetails> packages = DeploymentManager.GetUpgradeableRuntimeVersions(OldVersion);
            Assert.IsNotNull(packages, "Latest version returned was null.");
            Assert.IsTrue(packages.Count() > 0);

            foreach (RuntimePackageDetails package in packages)
            {
                Assert.IsNotNull(package, "Supported packages list contains a null package.");
                Assert.IsNotNull(package.Version, "Supported package does not contain any version");
                Assert.IsNotNull(package.TargetPackageLocation, "Supported package does not contain target location.");
            }
        }


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void CreateClusterManifest()
        {
            string jsonConfigPath = Path.Combine(Utility.TestDirectory, "ClusterConfig.Unsecure.DevCluster.json");
            Assert.IsTrue(System.IO.File.Exists(jsonConfigPath), string.Format("JSON config file {0} did not exist.", jsonConfigPath));
            var clusterResource = DeploymentManagerInternal.GetClusterResource(jsonConfigPath, System.Guid.NewGuid().ToString());
            var cm = clusterResource.Current.ExternalState.ClusterManifest;
            Assert.IsNotNull(cm, "Cluster manifest was null.");
            var settings = new XmlWriterSettings { Indent = true };

            using (var fs = new FileStream("ClusterManifest.xml", FileMode.Create))
            {
                using (var writer = XmlWriter.Create(fs, settings))
                {
                    XmlSerializer serializer = new XmlSerializer(typeof(ClusterManifestType));
                    serializer.Serialize(writer, cm);
                }
                fs.Close(); // Explicit close to reduce races.
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void WinCertClusterTest()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.WinCert.DevCluster.json");
            Assert.AreEqual("1.0.0", cluster.Current.CSMConfig.Version.Version);

            var nodeType = cluster.Current.ExternalState.ClusterManifest.NodeTypes.First();
            Assert.IsNotNull(nodeType.Certificates.ClientCertificate);
            Assert.IsNull(nodeType.Certificates.ClusterCertificate);
            Assert.AreEqual(nodeType.Certificates.ServerCertificate, nodeType.Certificates.ClientCertificate);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RuntimeMajorVersionBumpupTest()
        {
            string[] filePaths = new string[]
            {
                Path.Combine(Utility.TestDirectory, "5.7cu2UosState1.json"),
                Path.Combine(Utility.TestDirectory, "5.7cu2UosState2.json"),
            };

            foreach (string file in filePaths)
            {
                string str = File.ReadAllText(file);

                StandAloneCluster cluster = StoreManager.DeserializeClusterResource(str);
                Assert.IsNotNull(cluster);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestStandaloneClusterDeseralization()
        {
            string[] filePaths = new string[]
            {
                Path.Combine(Utility.TestDirectory, "5.7cu2UosState1.json"),
                Path.Combine(Utility.TestDirectory, "5.7cu2UosState2.json"),
            };

            foreach (string file in filePaths)
            {
                string str = File.ReadAllText(file);

                // Till version 5.x is deprecated, we have to keep supporting this i.e. replace 5.0.0.0 with 6.0.0.0 before deserealizing and back to 5.0.0.0 before persisting for 6.x->5.x upgrades to work.
                var normalizedString = StoreManager.NormalizeClusterResource(str, Assembly.GetExecutingAssembly().GetName().Version.ToString());

                // Get defaultDeserealizedCluster for validating purposes
                var defaultDeserealizedCluster = Utility.DeserealizeUsingDefaults(normalizedString);

                // Deserealize using the new JsonConvertor which does not depend on version and assembly info
                var customDeserealizedCluster = Utility.DeserealizeUsingCustomDeserealizer(normalizedString);

                Assert.IsTrue(defaultDeserealizedCluster.Current.NodeConfig.Version.Equals(customDeserealizedCluster.Current.NodeConfig.Version));
                Assert.IsTrue(defaultDeserealizedCluster.Current.CSMConfig.Version.Equals(customDeserealizedCluster.Current.CSMConfig.Version));
                Assert.IsTrue(defaultDeserealizedCluster.Current.WRPConfig.Equals(customDeserealizedCluster.Current.WRPConfig));
                Assert.IsTrue(defaultDeserealizedCluster.Current.ExternalState.Equals(customDeserealizedCluster.Current.ExternalState));

                // Serialize result back with version info and validate
                string defaultSerializedStringForCustomDeserealizedCluster = Utility.SerealizeUsingDefaults(customDeserealizedCluster);
                string defaultSerializedStringForDefaultDeserealizedCluster = Utility.SerealizeUsingDefaults(defaultDeserealizedCluster);
                Assert.IsTrue(string.Equals(defaultSerializedStringForCustomDeserealizedCluster, defaultSerializedStringForDefaultDeserealizedCluster, StringComparison.OrdinalIgnoreCase));

                // Serialize again without version info. This change can be made in UOS once 5.x is deprecated.
                var customSerializedStringForCustomDeserealizedCluster = Utility.SerealizeUsingNewSerializerSettings(customDeserealizedCluster);
                var customSerializedStringForDefaultDeserealizedCluster = Utility.SerealizeUsingNewSerializerSettings(defaultDeserealizedCluster);

                Assert.IsTrue(string.Equals(customSerializedStringForDefaultDeserealizedCluster, customSerializedStringForCustomDeserealizedCluster, StringComparison.OrdinalIgnoreCase));

                // Deserealize it again to ensure it works without version info
                customDeserealizedCluster = Utility.DeserealizeUsingCustomDeserealizer(customSerializedStringForCustomDeserealizedCluster);
                Assert.IsNotNull(customDeserealizedCluster);
                var customSerializedStringForCustomDeserealizedCluster2 = Utility.SerealizeUsingNewSerializerSettings(customDeserealizedCluster);

                Assert.IsTrue(string.Equals(customSerializedStringForCustomDeserealizedCluster, customSerializedStringForCustomDeserealizedCluster2, StringComparison.OrdinalIgnoreCase));
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void VerifySupportedListForCustomDeserealiser()
        {
            var allTypes = new List<Type>();
            List<string> assemblies = new List<string>()
            {
                "Microsoft.ServiceFabric.ClusterManagementCommon.dll",
                "Microsoft.ServiceFabric.DeploymentManager.dll",
                "System.Fabric.Management.ServiceModel.dll"
            };

            foreach (var assembly in assemblies)
            {
                allTypes.AddRange(Assembly.LoadFrom(assembly).GetTypes());
            }

            HashSet<Type> supportedTypes = new HashSet<Type>();
            supportedTypes = GetAllInterfacesAndBaseTypes(typeof(StandAloneCluster), allTypes, new HashSet<Type>());
            Console.WriteLine("SupportedTypes required for Custom Deserealiser:\n {0}", String.Join("\n", supportedTypes));
            Assert.IsFalse(supportedTypes.Except(StandAloneClusterJsonDeserializer.SupportedTypesForCustomDeserealiser.Keys).Any());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneIsSingletonReplicaMoveAllowedDuringUpgradeTestOneNode()
        {
            string jsonFilePath = "ClusterConfig.UnSecure.OneNode.json";
            StandAloneCluster cluster = Utility.PopulateStandAloneClusterWithBaselineJson(jsonFilePath);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneBaselineUpgradeState));
            Assert.AreEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);

            string isSingletonReplicaMoveAllowedDuringUpgradeTest = cluster.Current.ExternalState.ClusterManifest.FabricSettings.First(
                    section => section.Name == StringConstants.SectionName.FailoverManager).Parameter.First(
                    param => param.Name == StringConstants.ParameterName.IsSingletonReplicaMoveAllowedDuringUpgrade).Value;

            Assert.AreEqual("false", isSingletonReplicaMoveAllowedDuringUpgradeTest);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneIsSingletonReplicaMoveAllowedDuringUpgradeTestMultiNode()
        {
            string jsonFilePath = "ClusterConfig.UnSecure.DevCluster.json";
            StandAloneCluster cluster = Utility.PopulateStandAloneClusterWithBaselineJson(jsonFilePath);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneBaselineUpgradeState));
            Assert.AreEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);

            string isSingletonReplicaMoveAllowedDuringUpgradeTest = cluster.Current.ExternalState.ClusterManifest.FabricSettings.First(
                    section => section.Name == StringConstants.SectionName.FailoverManager).Parameter.First(
                    param => param.Name == StringConstants.ParameterName.IsSingletonReplicaMoveAllowedDuringUpgrade).Value;

            Assert.AreEqual("True", isSingletonReplicaMoveAllowedDuringUpgradeTest);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("jkochhar")]
        public void DeploymentManagerTest_ValidateIncorrectCodeVersionJsonConfig()
        {
            var result = BPA.BestPracticesAnalyzer.AnalyzeClusterSetup(JsonConfigCodeUpgradeInvalidJune2018Filename, null, null);
            Assert.IsFalse(result.Passed, "BPA analysis should fail for invalid code version in config");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("jkochhar")]
        public void DeploymentManagerTest_ValidateJsonConfigCodeUpgradeJune2018()
        {
            string clusterConfigPath = Path.Combine(BaseDir, JsonConfigCodeUpgradeJune2018Filename);
            var configModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(clusterConfigPath);
            Assert.IsTrue(File.Exists(clusterConfigPath), string.Format("Config file {0} did not exist.", clusterConfigPath));
            Assert.IsNotNull(configModel, "JSON config was invalid.");

            var userConfig = configModel.GetUserConfig();
            Assert.IsTrue(userConfig.CodeVersion == "5.5.0.0", "Code version should be 5.5.0.0.");
        }

        private HashSet<Type> GetAllInterfacesAndBaseTypes(Type type, List<Type> allTypes, HashSet<Type> alreadySeen)
        {
            var result = new HashSet<Type>();
            if (alreadySeen.Contains(type))
            {
                return result;
            }
            alreadySeen.Add(type);

            /* Current implementation of custom deserealiser does not support Json Properties marked with IsReference=true. For such properties/fields 
               we rely on the default newtonsoft deserealiser, so these properties do not need to be considered while creating the supportedTypes list
               for the custom deserealiser. We can also ignore the properties/fields marked with JsonIgnore flag because these should be ignored by the deserealiser.
            */
            var prop = type.GetRuntimeProperties();
            var reqProp = prop.Where(p => !p.GetCustomAttributes(false).OfType<JsonIgnoreAttribute>().Any() &&
                                          !p.GetCustomAttributes(false).OfType<JsonPropertyAttribute>().Any(item => item.IsReference) &&
                                          !p.GetCustomAttributes(false).OfType<JsonObjectAttribute>().Any(item => item.IsReference))
                               .Select(p => p.PropertyType).ToList(); 
            var fields = type.GetRuntimeFields();
            var reqFields = fields.Where(p => !p.GetCustomAttributes(false).OfType<JsonIgnoreAttribute>().Any() &&
                                          !p.GetCustomAttributes(false).OfType<JsonPropertyAttribute>().Any(item => item.IsReference) &&
                                          !p.GetCustomAttributes(false).OfType<JsonObjectAttribute>().Any(item => item.IsReference))
                               .Select(p => p.FieldType).ToList();

            var members = new List<Type>(reqProp);
            members.AddRange(reqFields);
            foreach (var member in members)
            {
                if (!alreadySeen.Contains(member))
                {
                    alreadySeen.Add(member);
                    if (allTypes.Contains(member) && !member.IsSealed)
                    {
                        var implementedTypes = allTypes.Where(t => t != member && member.IsAssignableFrom(t)).ToList();
                        if (implementedTypes.Count != 0)
                        {
                            result.Add(member);
                            foreach (var implementedType in implementedTypes)
                            {
                                result.UnionWith(GetAllInterfacesAndBaseTypes(implementedType, allTypes, alreadySeen));
                            }
                       }
                       else
                       {
                           result.UnionWith(GetAllInterfacesAndBaseTypes(member, allTypes, alreadySeen));
                       }
                   }
                }
            }

            return result;
        }
    }
}
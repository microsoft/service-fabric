// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Test
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.IO;
    using System.Linq;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Microsoft.ServiceFabric.DeploymentManager.Model;

    using Newtonsoft.Json;

    /// <summary>
    /// test all features of standalone AddonFeature
    /// </summary>
    [TestClass]
    public class AddonFeatureTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConstructorTest()
        {
            StandAloneInstallerJsonModelBase jsonConfig;

            jsonConfig = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(Path.Combine(Utility.TestDirectory, TestFiles.GAv1));
            Assert.IsInstanceOfType(jsonConfig, typeof(StandAloneInstallerJsonModelGA));
            
            jsonConfig = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(Path.Combine(Utility.TestDirectory, TestFiles.Jan2017v1));
            Assert.IsInstanceOfType(jsonConfig, typeof(StandAloneInstallerJSONModelJan2017));

            this.AddonConstructorTest(TestFiles.April2017HasAddon, new AddonFeature[] { AddonFeature.RepairManager, AddonFeature.DnsService }, typeof(StandAloneInstallerJSONModelApril2017));
            this.AddonConstructorTest(TestFiles.April2017NoAddon, new AddonFeature[0], typeof(StandAloneInstallerJSONModelApril2017));
            this.AddonConstructorTest(TestFiles.April2017HasUnsupportedAddon, new AddonFeature[0], typeof(StandAloneInstallerJSONModelApril2017), expectError: true);

            this.AddonConstructorTest(TestFiles.May2017HasAddon, new AddonFeature[] { AddonFeature.RepairManager, AddonFeature.DnsService }, typeof(StandAloneInstallerJSONModelMay2017));
            this.AddonConstructorTest(TestFiles.May2017NoAddon, new AddonFeature[0], typeof(StandAloneInstallerJSONModelMay2017));
            this.AddonConstructorTest(TestFiles.May2017HasUnsupportedAddon, new AddonFeature[0], typeof(StandAloneInstallerJSONModelMay2017), expectError: true);

            this.AddonConstructorTest(TestFiles.Aug2017HasAddon, new AddonFeature[] { AddonFeature.RepairManager, AddonFeature.DnsService, AddonFeature.BackupRestoreService }, typeof(StandAloneInstallerJSONModelAugust2017));
            this.AddonConstructorTest(TestFiles.Aug2017NoAddon, new AddonFeature[0], typeof(StandAloneInstallerJSONModelAugust2017));
            this.AddonConstructorTest(TestFiles.Aug2017HasUnsupportedAddon, new AddonFeature[0], typeof(StandAloneInstallerJSONModelAugust2017), expectError: true);

            this.AddonConstructorTest(TestFiles.Oct2017HasAddon, new AddonFeature[] { AddonFeature.RepairManager, AddonFeature.DnsService, AddonFeature.BackupRestoreService }, typeof(StandAloneInstallerJSONModelOctober2017));
            this.AddonConstructorTest(TestFiles.Oct2017NoAddon, new AddonFeature[0], typeof(StandAloneInstallerJSONModelOctober2017));
            this.AddonConstructorTest(TestFiles.Oct2017HasUnsupportedAddon, new AddonFeature[0], typeof(StandAloneInstallerJSONModelOctober2017), expectError: true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RestartUpgradePolicyTest()
        {
            Console.WriteLine("GA -> GA");
            this.InternalRestartUpgradePolicyTest(TestFiles.GAv1, TestFiles.GAv2, expectedForceRestart: false);

            Console.WriteLine("GA -> Jan2017");
            this.InternalRestartUpgradePolicyTest(TestFiles.GAv1, TestFiles.Jan2017v2, expectedForceRestart: false);

            Console.WriteLine("GA -> April2017: has addon");
            this.InternalRestartUpgradePolicyTest(TestFiles.GAv1, TestFiles.April2017HasAddon, expectedForceRestart: true);

            Console.WriteLine("GA -> April2017: no addon");
            this.InternalRestartUpgradePolicyTest(TestFiles.GAv1, TestFiles.April2017NoAddon, expectedForceRestart: false);

            Console.WriteLine("Jan2017 -> Jan2017");
            this.InternalRestartUpgradePolicyTest(TestFiles.Jan2017v1, TestFiles.Jan2017v2, expectedForceRestart: false);

            Console.WriteLine("Jan2017 -> April2017: has addon");
            this.InternalRestartUpgradePolicyTest(TestFiles.Jan2017v1, TestFiles.April2017HasAddon, expectedForceRestart: true);

            Console.WriteLine("Jan2017 -> April2017: no addon");
            this.InternalRestartUpgradePolicyTest(TestFiles.Jan2017v1, TestFiles.April2017NoAddon, expectedForceRestart: false);

            Console.WriteLine("April2017 -> April2017: no -> has addon");
            this.InternalRestartUpgradePolicyTest(TestFiles.April2017NoAddon, TestFiles.April2017HasAddon, expectedForceRestart: true);

            Console.WriteLine("April2017 -> April2017: has DNS+RM -> has RM");
            this.InternalRestartUpgradePolicyTest(TestFiles.April2017HasAddon, TestFiles.April2017NoAddonDns, expectedForceRestart: true);

            Console.WriteLine("April2017 -> May2017: no -> has addon");
            this.InternalRestartUpgradePolicyTest(TestFiles.April2017NoAddon, TestFiles.May2017HasAddon, expectedForceRestart: true);

            Console.WriteLine("May2017 -> May2017: has DNS+RM -> has RM");
            this.InternalRestartUpgradePolicyTest(TestFiles.May2017HasAddon, TestFiles.May2017NoAddonDns, expectedForceRestart: true);

            Console.WriteLine("May2017 -> Aug2017: has RM+DNS -> has RM+DNS+BRS");
            this.InternalRestartUpgradePolicyTest(TestFiles.May2017HasAddon, TestFiles.Aug2017HasAddon, expectedForceRestart: true);
            
            Console.WriteLine("May2017 -> Aug2017: no -> has");
            this.InternalRestartUpgradePolicyTest(TestFiles.May2017NoAddon, TestFiles.Aug2017HasAddon, expectedForceRestart: true);
            
            Console.WriteLine("Aug2017 -> Aug2017: has DNS+RM+BRS -> has RM");
            this.InternalRestartUpgradePolicyTest(TestFiles.Aug2017HasAddon, TestFiles.Aug2017RMAddon, expectedForceRestart: true);

            Console.WriteLine("Aug2017 -> Oct2017: no -> has addon");
            this.InternalRestartUpgradePolicyTest(TestFiles.Aug2017NoAddon, TestFiles.Oct2017HasAddon, expectedForceRestart: true);

            Console.WriteLine("Oct2017 -> Oct2017: has DNS+RM+BRS -> has RM");
            this.InternalRestartUpgradePolicyTest(TestFiles.Oct2017HasAddon, TestFiles.Oct2017RMAddon, expectedForceRestart: true);

            Console.WriteLine("Oct2017 -> Oct2017: no -> has addon");
            this.InternalRestartUpgradePolicyTest(TestFiles.Oct2017NoAddon, TestFiles.Oct2017HasAddon, expectedForceRestart: true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ClusterManifestBuilderTest()
        {
            StandAloneCluster cluster;
            ClusterManifestType manifest;

            string[] noAddonJsonFiles = new string[]
            {
                TestFiles.GAv1,
                TestFiles.Jan2017v1,
                TestFiles.April2017NoAddon,
                TestFiles.May2017NoAddon,
                TestFiles.Aug2017NoAddon,
                TestFiles.Oct2017NoAddon
            };

            foreach (string jsonFile in noAddonJsonFiles)
            {
                cluster = this.InitializeBaselineUpgrade(jsonFile);
                manifest = cluster.Pending.ExternalState.ClusterManifest;
                Assert.IsFalse(manifest.FabricSettings.Any(
                    section => section.Name.Equals(StringConstants.SectionName.RepairManager, StringComparison.OrdinalIgnoreCase)
                    || section.Name.Equals(StringConstants.SectionName.DnsService, StringComparison.OrdinalIgnoreCase)
                    || section.Name.Equals(StringConstants.SectionName.BackupRestoreService, StringComparison.OrdinalIgnoreCase)));
            }

            // verify RM/ DnsSvc
            Dictionary<string, int> addOnJsonFiles = new Dictionary<string, int>()
            {
                { TestFiles.April2017HasAddon, 1 },
                { TestFiles.April2017HasAddonMultipleBox, -1 },
                { TestFiles.May2017HasAddon, 1 },
                { TestFiles.May2017HasAddonMultipleBox, -1 },
            };

            foreach (KeyValuePair<string, int> addOnJsonFile in addOnJsonFiles)
            {
                cluster = this.InitializeBaselineUpgrade(addOnJsonFile.Key);
                manifest = cluster.Pending.ExternalState.ClusterManifest;
                this.VerifyRepairManagerSection(manifest.FabricSettings.Single(section => section.Name.Equals(StringConstants.SectionName.RepairManager)));
                this.VerifyDnsServiceSection(manifest.FabricSettings.Single(section => section.Name.Equals(StringConstants.SectionName.DnsService)), addOnJsonFile.Value);
            }

            // verify RM/ DnsSvc/ BackupRestoreSvc
            addOnJsonFiles = new Dictionary<string, int>()
            {
                { TestFiles.Aug2017HasAddon, 1 },
                { TestFiles.Aug2017HasAddonMultipleBox, -1 },
                { TestFiles.Oct2017HasAddon, 1 },
                { TestFiles.Oct2017HasAddonMultipleBox, -1 }
            };

            foreach (KeyValuePair<string, int> addOnJsonFile in addOnJsonFiles)
            {
                cluster = this.InitializeBaselineUpgrade(addOnJsonFile.Key);
                manifest = cluster.Pending.ExternalState.ClusterManifest;
                this.VerifyRepairManagerSection(manifest.FabricSettings.Single(section => section.Name.Equals(StringConstants.SectionName.RepairManager)));
                this.VerifyBackupRestoreServiceSection(manifest.FabricSettings.Single(section => section.Name.Equals(StringConstants.SectionName.BackupRestoreService)));
                this.VerifyDnsServiceSection(manifest.FabricSettings.Single(section => section.Name.Equals(StringConstants.SectionName.DnsService)), addOnJsonFile.Value);
            }
            
            // verify upgrade
            cluster = Utility.DoBaselineUpgrade(TestFiles.April2017NoAddon);
            Utility.UpdateStandAloneCluster(TestFiles.April2017NoAddonV2, cluster);
            Assert.IsTrue(Utility.RunStateMachine(cluster));
            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Utility.RollForwardOneIteration(cluster);
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            Assert.IsNull(cluster.Pending);
            manifest = cluster.Current.ExternalState.ClusterManifest;
            this.VerifyRepairManagerSection(manifest.FabricSettings.Single(section => section.Name.Equals(StringConstants.SectionName.RepairManager)));

            // simulate the scenario that customer 'partially' enabled RM due to the regression introduced in 6.0
            var rmSection = manifest.FabricSettings.Single(section => section.Name.Equals(StringConstants.SectionName.RepairManager));
            rmSection.Parameter = new SettingsOverridesTypeSectionParameter[1];
            rmSection.Parameter[0] = new SettingsOverridesTypeSectionParameter()
            {
                Name = StringConstants.ParameterName.EnableHealthChecks,
                Value = "True"
            };

            Utility.UpdateStandAloneCluster(TestFiles.April2017NoAddonV3, cluster);
            Assert.IsTrue(Utility.RunStateMachine(cluster));
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Utility.RollForwardOneIteration(cluster);
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            Assert.IsNull(cluster.Pending);
            manifest = cluster.Current.ExternalState.ClusterManifest;
            this.VerifyRepairManagerSection(manifest.FabricSettings.Single(section => section.Name.Equals(StringConstants.SectionName.RepairManager)));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RepairManagerDefinitionTest()
        {
            string configCsvPath = Path.Combine(Utility.TestDirectory, "Configurations.csv");
            FabricSettingsMetadata metadata = FabricSettingsMetadata.Create(configCsvPath);
            ConfigDescription desc = null;
            metadata.TryGetConfigDescription(StringConstants.SectionName.RepairManager, StringConstants.ParameterName.TargetReplicaSetSize, out desc);
            Assert.AreEqual(desc.DefaultValue, "0");
            Assert.AreEqual(desc.UpgradePolicy, UpgradePolicy.SingleChange);
            metadata.TryGetConfigDescription(StringConstants.SectionName.RepairManager, StringConstants.ParameterName.MinReplicaSetSize, out desc);
            Assert.AreEqual(desc.DefaultValue, "0");
            Assert.AreEqual(desc.UpgradePolicy, UpgradePolicy.SingleChange);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void VerifySingleChangePolicy()
        {
            Environment.SetEnvironmentVariable("StandaloneAutoScaleClusterUpgradeStateSkipQuorumChecksTestHook", "true");
            // Create cluster with no-addon
            var cluster = Utility.DoBaselineUpgrade(TestFiles.Oct2017MultiMachineNoAddon);
            var manifest = cluster.Current.ExternalState.ClusterManifest;
            Assert.IsFalse(manifest.FabricSettings.Any(section => section.Name.Equals(StringConstants.SectionName.RepairManager)));
            Assert.IsFalse(manifest.FabricSettings.Any(section => section.Name.Equals(StringConstants.SectionName.DnsService)));
            Assert.IsFalse(manifest.FabricSettings.Any(section => section.Name.Equals(StringConstants.SectionName.BackupRestoreService)));
            Assert.AreEqual(((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count(), 3);

            // Add RM,DNS and BackupRestore add-ons
            Utility.UpdateStandAloneCluster(TestFiles.Oct2017MultiMachineHasAddon, cluster);
            Assert.IsTrue(Utility.RunStateMachine(cluster));
            var upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Utility.RollForwardOneIteration(cluster);
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            Assert.IsNull(cluster.Pending);
            manifest = cluster.Current.ExternalState.ClusterManifest;
            this.VerifyRepairManagerSection(manifest.FabricSettings.Single(section => section.Name.Equals(StringConstants.SectionName.RepairManager)));
            this.VerifyDnsServiceSection(manifest.FabricSettings.Single(section => section.Name.Equals(StringConstants.SectionName.DnsService)), -1);
            this.VerifyBackupRestoreServiceSection(manifest.FabricSettings.Single(section => section.Name.Equals(StringConstants.SectionName.BackupRestoreService)));
            Assert.AreEqual(((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count(), 3);

            // Scale cluster from 3 to 5 nodes. This should not modify Target/Min Replica set sizes for any system service
            Utility.UpdateStandAloneClusterForAddNode(TestFiles.Oct2017MultiMachineHasAddonScale, cluster);
            Assert.IsTrue(Utility.RunStateMachine(cluster));
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Utility.RollForwardOneIteration(cluster);
            Assert.IsNotNull(cluster.Pending);
            Utility.RollForwardOneIteration(cluster);
            Assert.IsNotNull(cluster.Pending);
            Utility.SetTargetReplicaSetSize(upgradeState);
            Utility.RollForwardOneIteration(cluster);
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            Assert.IsNull(cluster.Pending);
            manifest = cluster.Current.ExternalState.ClusterManifest;
            this.VerifyRepairManagerSection(manifest.FabricSettings.Single(section => section.Name.Equals(StringConstants.SectionName.RepairManager)));
            this.VerifyDnsServiceSection(manifest.FabricSettings.Single(section => section.Name.Equals(StringConstants.SectionName.DnsService)), -1);
            this.VerifyBackupRestoreServiceSection(manifest.FabricSettings.Single(section => section.Name.Equals(StringConstants.SectionName.BackupRestoreService)));
            Assert.AreEqual(((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count(), 5);

            // Remove add-ons
            Utility.UpdateStandAloneCluster(TestFiles.Oct2017MultiMachineNoAddonScale, cluster);
            Assert.IsTrue(Utility.RunStateMachine(cluster));
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Utility.RollForwardOneIteration(cluster);
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            Assert.IsNull(cluster.Pending);
            manifest = cluster.Current.ExternalState.ClusterManifest;
            Assert.IsFalse(manifest.FabricSettings.Any(section => section.Name.Equals(StringConstants.SectionName.RepairManager)));
            Assert.IsFalse(manifest.FabricSettings.Any(section => section.Name.Equals(StringConstants.SectionName.DnsService)));
            Assert.IsFalse(manifest.FabricSettings.Any(section => section.Name.Equals(StringConstants.SectionName.BackupRestoreService)));
            Assert.AreEqual(((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count(), 5);
        }

        private void AddonConstructorTest(string jsonFile, AddonFeature[] expectedAddonFeatures, Type modelType, bool expectError = false)
        {
            StandAloneInstallerJsonModelBase jsonConfig;

            try
            {
                jsonConfig = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(Path.Combine(Utility.TestDirectory, jsonFile));
                if (expectError)
                {
                    if (jsonConfig != null)
                    {
                        throw new Exception("test failure");
                    }
                    else
                    {
                        return;
                    }
                }
            }
            catch (Exception ex)
            {
                if (!expectError)
                {
                    throw new Exception("test failure", ex);
                }

                return;
            }

            bool isAprilModel = jsonConfig.GetType() == typeof(StandAloneInstallerJSONModelApril2017);
            bool isMayModel = jsonConfig.GetType() == typeof(StandAloneInstallerJSONModelMay2017);
            bool isAugModel = jsonConfig.GetType() == typeof(StandAloneInstallerJSONModelAugust2017);
            bool isOctModel = jsonConfig.GetType() == typeof(StandAloneInstallerJSONModelOctober2017);

            PropertyApril2017 aprilProperty = null;
            PropertyMay2017 mayProperty = null;
            PropertyAugust2017 augProperty = null;
            PropertyOctober2017 octProperty = null;

            // deserialization test
            Assert.IsInstanceOfType(jsonConfig, modelType);

            if (isAprilModel)
            {
                aprilProperty = (PropertyApril2017)((StandAloneInstallerJSONModelApril2017)jsonConfig).Properties;
                Assert.AreEqual(expectedAddonFeatures.Length, aprilProperty.AddonFeatures.Count);
                Assert.IsTrue(aprilProperty.AddonFeatures.All(p => expectedAddonFeatures.Contains(p)));
                Assert.IsTrue(expectedAddonFeatures.All(p => aprilProperty.AddonFeatures.Contains(p)));
                Assert.IsNull(aprilProperty.TestHookReliabilityLevel);
            }
            else if (isMayModel)
            {
                mayProperty = (PropertyMay2017)((StandAloneInstallerJSONModelMay2017)jsonConfig).Properties;
                Assert.AreEqual(expectedAddonFeatures.Length, mayProperty.AddonFeatures.Count);
                Assert.IsTrue(mayProperty.AddonFeatures.All(p => expectedAddonFeatures.Contains(p)));
                Assert.IsTrue(expectedAddonFeatures.All(p => mayProperty.AddonFeatures.Contains(p)));
                Assert.IsNull(mayProperty.TestHookReliabilityLevel);
            }
            else if (isAugModel)
            {
                augProperty = (PropertyAugust2017)((StandAloneInstallerJSONModelAugust2017)jsonConfig).Properties;
                Assert.AreEqual(expectedAddonFeatures.Length, augProperty.AddonFeatures.Count);
                Assert.IsTrue(augProperty.AddonFeatures.All(p => expectedAddonFeatures.Contains(p)));
                Assert.IsTrue(expectedAddonFeatures.All(p => augProperty.AddonFeatures.Contains(p)));
                Assert.IsNull(augProperty.TestHookReliabilityLevel);
            }
            else if (isOctModel)
            {
                octProperty = (PropertyOctober2017)((StandAloneInstallerJSONModelOctober2017)jsonConfig).Properties;
                Assert.AreEqual(expectedAddonFeatures.Length, octProperty.AddonFeatures.Count);
                Assert.IsTrue(octProperty.AddonFeatures.All(p => expectedAddonFeatures.Contains(p)));
                Assert.IsTrue(expectedAddonFeatures.All(p => octProperty.AddonFeatures.Contains(p)));
                Assert.IsNull(octProperty.TestHookReliabilityLevel);
            }

            // serialization test
            string newJsonFilePath = Path.Combine(Utility.TestDirectory, "AddonFeatureTestTmpJson.json");
            File.WriteAllText(newJsonFilePath, JsonConvert.SerializeObject(jsonConfig, new JsonSerializerSettings()
            {
                DefaultValueHandling = DefaultValueHandling.Populate,
                PreserveReferencesHandling = PreserveReferencesHandling.None
            }));
            StandAloneInstallerJsonModelBase newJsonConfig = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(newJsonFilePath);
            File.Delete(newJsonFilePath);

            if (isAprilModel)
            {
                PropertyApril2017 newAprilProperty = (PropertyApril2017)((StandAloneInstallerJSONModelApril2017)newJsonConfig).Properties;
                Assert.AreEqual(aprilProperty.AddonFeatures.Count, newAprilProperty.AddonFeatures.Count);
            }
            else if(isMayModel)
            {
                PropertyMay2017 newMayProperty = (PropertyMay2017)((StandAloneInstallerJSONModelMay2017)newJsonConfig).Properties;
                Assert.AreEqual(mayProperty.AddonFeatures.Count, newMayProperty.AddonFeatures.Count);
            }
            else if(isAugModel)
            {
                PropertyAugust2017 newAugProperty = (PropertyAugust2017)((StandAloneInstallerJSONModelAugust2017)newJsonConfig).Properties;
                Assert.AreEqual(augProperty.AddonFeatures.Count, newAugProperty.AddonFeatures.Count);
            }
            else if (isOctModel)
            {
                PropertyOctober2017 newOctProperty = (PropertyOctober2017)((StandAloneInstallerJSONModelOctober2017)newJsonConfig).Properties;
                Assert.AreEqual(octProperty.AddonFeatures.Count, newOctProperty.AddonFeatures.Count);
            }
        }

        private StandAloneCluster InitializeBaselineUpgrade(string jsonFilePath)
        {
            StandAloneCluster result = Utility.PopulateStandAloneClusterWithBaselineJson(jsonFilePath);
            Assert.IsTrue(result.RunStateMachine());
            return result;
        }

        private void InternalRestartUpgradePolicyTest(string v1JsonFile, string v2JsonFile, bool expectedForceRestart)
        {
            StandAloneCluster cluster = Utility.PopulateStandAloneClusterWithBaselineJson(v1JsonFile);
            this.ValidateUpgrade(cluster, 1, false);

            Utility.UpdateStandAloneCluster(v2JsonFile, cluster);
            this.ValidateUpgrade(cluster, 3, expectedForceRestart);
        }

        private void ValidateUpgrade(StandAloneCluster cluster, int iterations, bool expectedForceRestart)
        {
            for (int i = 0; i < iterations; i++)
            {
                cluster.RunStateMachine();

                bool currentExpectedForceRestart = expectedForceRestart && i == 0;
                Assert.AreEqual(currentExpectedForceRestart, cluster.Pending.GetUpgradePolicy().ForceRestart);

                Utility.RollForwardOneIteration(cluster);
                Assert.AreEqual(cluster.Pending == null, i == iterations - 1);
            }

            Assert.IsFalse(cluster.RunStateMachine());
            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
        }

        private void VerifyRepairManagerSection(SettingsOverridesTypeSection rmSection)
        {
            Assert.AreEqual(3, rmSection.Parameter.Length);

            SettingsOverridesTypeSectionParameter enableHealthChecksParameter = rmSection.Parameter.Single(p => p.Name == StringConstants.ParameterName.EnableHealthChecks);
            Assert.IsTrue(enableHealthChecksParameter.Value.Equals("True", StringComparison.OrdinalIgnoreCase));

            SettingsOverridesTypeSectionParameter targetReplicaSetSizeParameter = rmSection.Parameter.Single(p => p.Name == StringConstants.ParameterName.TargetReplicaSetSize);
            Assert.AreEqual("3", targetReplicaSetSizeParameter.Value);

            SettingsOverridesTypeSectionParameter minReplicaSetSizeParameter = rmSection.Parameter.Single(p => p.Name == StringConstants.ParameterName.MinReplicaSetSize);
            Assert.AreEqual("3", minReplicaSetSizeParameter.Value);
        }

        private void VerifyBackupRestoreServiceSection(SettingsOverridesTypeSection brsSection)
        {
            Assert.AreEqual(2, brsSection.Parameter.Length);

            SettingsOverridesTypeSectionParameter targetReplicaSetSizeParameter = brsSection.Parameter.Single(p => p.Name == StringConstants.ParameterName.TargetReplicaSetSize);
            Assert.AreEqual("3", targetReplicaSetSizeParameter.Value);

            SettingsOverridesTypeSectionParameter minReplicaSetSizeParameter = brsSection.Parameter.Single(p => p.Name == StringConstants.ParameterName.MinReplicaSetSize);
            Assert.AreEqual("3", minReplicaSetSizeParameter.Value);
        }

        private void VerifyDnsServiceSection(SettingsOverridesTypeSection dnsSection, int expectedInstanceCount)
        {
            Assert.AreEqual(2, dnsSection.Parameter.Length);

            SettingsOverridesTypeSectionParameter isEnabledParameter = dnsSection.Parameter.Single(p => p.Name == StringConstants.ParameterName.IsEnabled);
            Assert.IsTrue(isEnabledParameter.Value.Equals("True", StringComparison.OrdinalIgnoreCase));

            SettingsOverridesTypeSectionParameter instanceCountParameter = dnsSection.Parameter.Single(p => p.Name == StringConstants.ParameterName.InstanceCount);
            Assert.IsTrue(instanceCountParameter.Value.Equals(expectedInstanceCount.ToString(), StringComparison.OrdinalIgnoreCase));
        }

        private static class TestFiles
        {
            public const string GAv1 = "AddonTest.X509.DevCluster.GA.v1.json";

            public const string GAv2 = "AddonTest.X509.DevCluster.GA.v2.json";

            public const string Jan2017v1 = "AddonTest.X509.DevCluster.Jan2017.v1.json";

            public const string Jan2017v2 = "AddonTest.X509.DevCluster.Jan2017.v2.json";

            public const string April2017NoAddon = "AddonTest.X509.DevCluster.April2017.NoAddon.json";

            public const string April2017NoAddonV2 = "AddonTest.X509.DevCluster.April2017.NoAddonV2.json";

            public const string April2017NoAddonV3 = "AddonTest.X509.DevCluster.April2017.NoAddonV3.json";

            public const string April2017NoAddonDns = "AddonTest.X509.DevCluster.April2017.NoAddonDns.json";

            public const string April2017HasAddon = "AddonTest.X509.DevCluster.April2017.HasAddon.json";

            public const string April2017HasUnsupportedAddon = "AddonTest.X509.DevCluster.April2017.HasUnsupportedAddon.json";

            public const string April2017HasAddonMultipleBox = "AddonTest.X509.DevCluster.April2017.HasAddonMultibox.json";

            public const string May2017NoAddon = "AddonTest.X509.DevCluster.May2017.NoAddon.json";

            public const string May2017NoAddonDns = "AddonTest.X509.DevCluster.May2017.NoAddonDns.json";

            public const string May2017HasAddon = "AddonTest.X509.DevCluster.May2017.HasAddon.json";

            public const string May2017HasUnsupportedAddon = "AddonTest.X509.DevCluster.May2017.HasUnsupportedAddon.json";

            public const string May2017HasAddonMultipleBox = "AddonTest.X509.DevCluster.May2017.HasAddonMultibox.json";

            public const string Aug2017NoAddon = "AddonTest.X509.DevCluster.Aug2017.NoAddon.json";
            
            public const string Aug2017RMAddon = "AddonTest.X509.DevCluster.Aug2017.RMAddon.json";

            public const string Aug2017HasAddon = "AddonTest.X509.DevCluster.Aug2017.HasAddon.json";

            public const string Aug2017HasUnsupportedAddon = "AddonTest.X509.DevCluster.Aug2017.HasUnsupportedAddon.json";

            public const string Aug2017HasAddonMultipleBox = "AddonTest.X509.DevCluster.Aug2017.HasAddonMultibox.json";

            public const string Oct2017HasAddon = "AddonTest.X509.DevCluster.Oct2017.HasAddon.json";

            public const string Oct2017NoAddon = "AddonTest.X509.DevCluster.Oct2017.NoAddon.json";

            public const string Oct2017HasUnsupportedAddon = "AddonTest.X509.DevCluster.Oct2017.HasUnsupportedAddon.json";

            public const string Oct2017HasAddonMultipleBox = "AddonTest.X509.DevCluster.Oct2017.HasAddonMultibox.json";

            public const string Oct2017RMAddon = "AddonTest.X509.DevCluster.Oct2017.RMAddon.json";

            public const string Oct2017MultiMachineNoAddon = "AddonTest.X509.DevCluster.Oct2017.MultiMachine.NoAddon.json";

            public const string Oct2017MultiMachineHasAddon = "AddonTest.X509.DevCluster.Oct2017.MultiMachine.HasAddon.json";

            public const string Oct2017MultiMachineHasAddonScale = "AddonTest.X509.DevCluster.Oct2017.MultiMachine.HasAddonScale.json";

            public const string Oct2017MultiMachineNoAddonScale = "AddonTest.X509.DevCluster.Oct2017.MultiMachine.NoAddonScale.json";
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Collections.Generic;
    using WEX.TestExecution;

    /// <summary>
    /// Tests NativeRunConfigurationSettingValidator
    /// </summary>
    [TestClass]
    public class NativeRunConfigurationSettingValidatorTest
    {
        NativeRunConfigurationSettingValidator Validator;

        [TestInitialize]
        public void TestSetup()
        {
            Validator = new NativeRunConfigurationSettingValidator();
        }

        /// <summary>
        /// Tests to validate the configuration setting.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfiguration_EmptyParameter_Success()
        {
            Dictionary<string, List<SettingsOverridesTypeSectionParameter>> parameters = new Dictionary<string, List<SettingsOverridesTypeSectionParameter>>();
            parameters.Add(FabricValidatorConstants.SectionNames.NativeRunConfiguration, new List<SettingsOverridesTypeSectionParameter>());
            TestValidateConfiguration(parameters);
        }

        /// <summary>
        /// Tests to validate the configuration setting.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfiguration_False_False()
        {
            Dictionary<string, List<SettingsOverridesTypeSectionParameter>> parameters = new Dictionary<string, List<SettingsOverridesTypeSectionParameter>>();
            List<SettingsOverridesTypeSectionParameter> list = new List<SettingsOverridesTypeSectionParameter>
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FabricValidatorConstants.ParameterNames.NativeRunConfiguration.EnableNativeReliableStateManager,
                    Value = "false",
                    IsEncrypted = false
                }
            };

            parameters.Add(FabricValidatorConstants.SectionNames.NativeRunConfiguration, list);
            TestValidateConfiguration(parameters);
        }

        /// <summary>
        /// Tests to validate the configuration setting.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfiguration_True_True()
        {
            Dictionary<string, List<SettingsOverridesTypeSectionParameter>> parameters = new Dictionary<string, List<SettingsOverridesTypeSectionParameter>>();
            List<SettingsOverridesTypeSectionParameter> list = new List<SettingsOverridesTypeSectionParameter>
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FabricValidatorConstants.ParameterNames.NativeRunConfiguration.EnableNativeReliableStateManager,
                    Value = "true",
                    IsEncrypted = false
                }
            };

            parameters.Add(FabricValidatorConstants.SectionNames.NativeRunConfiguration, list);
            TestValidateConfiguration(parameters);
        }

        /// <summary>
        /// Tests when the settings between current and target empty.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfigurationUpgrade_EmptyParameter_Success()
        {
            Dictionary<string, List<SettingsOverridesTypeSectionParameter>> parameters = new Dictionary<string, List<SettingsOverridesTypeSectionParameter>>();
            parameters.Add(FabricValidatorConstants.SectionNames.NativeRunConfiguration, new List<SettingsOverridesTypeSectionParameter>());

            TestValidateConfigurationUpgrade(parameters, parameters);
        }

        /// <summary>
        /// Tests when the settings for EnableNativeReliableStateManager and SerializationVersion match.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfigurationUpgradeWithValidParameterValues()
        {
            Dictionary<string, List<SettingsOverridesTypeSectionParameter>> currentParameters = new Dictionary<string, List<SettingsOverridesTypeSectionParameter>>();
            List<SettingsOverridesTypeSectionParameter> list = new List<SettingsOverridesTypeSectionParameter>
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FabricValidatorConstants.ParameterNames.NativeRunConfiguration.EnableNativeReliableStateManager,
                    Value = "false",
                    IsEncrypted = false
                }
            };

            currentParameters.Add(FabricValidatorConstants.SectionNames.NativeRunConfiguration, list);

            List<SettingsOverridesTypeSectionParameter> list2 = new List<SettingsOverridesTypeSectionParameter>
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FabricValidatorConstants.ParameterNames.TransactionalReplicator2.SerializationVersion,
                    Value = "0",
                    IsEncrypted = false
                }
            };

            currentParameters.Add(FabricValidatorConstants.SectionNames.TransactionalReplicator2.Default, list2);


            Dictionary<string, List<SettingsOverridesTypeSectionParameter>> targetParameters = new Dictionary<string, List<SettingsOverridesTypeSectionParameter>>();
            List<SettingsOverridesTypeSectionParameter> newlist = new List<SettingsOverridesTypeSectionParameter>
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FabricValidatorConstants.ParameterNames.NativeRunConfiguration.EnableNativeReliableStateManager,
                    Value = "true",
                    IsEncrypted = false
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FabricValidatorConstants.ParameterNames.NativeRunConfiguration.Test_LoadEnableNativeReliableStateManager,
                    Value = "true",
                    IsEncrypted = false
                }
            };

            targetParameters.Add(FabricValidatorConstants.SectionNames.NativeRunConfiguration, newlist);

            List<SettingsOverridesTypeSectionParameter> newlist2 = new List<SettingsOverridesTypeSectionParameter>
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FabricValidatorConstants.ParameterNames.TransactionalReplicator2.SerializationVersion,
                    Value = "0",
                    IsEncrypted = false
                }
            };

            targetParameters.Add(FabricValidatorConstants.SectionNames.TransactionalReplicator2.Default, newlist2);

            TestValidateConfigurationUpgrade(currentParameters, targetParameters);
        }

        /// <summary>
        /// Tests when the settings for EnableNativeReliableStateManager and SerializationVersion do not match.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfigurationUpgradeWithInvalidParameterValues()
        {
            ConfigurationUpgradeWithMismatchedTestHelper(
                configName: FabricValidatorConstants.ParameterNames.TransactionalReplicator2.SerializationVersion,
                oldValue: "0",
                newValue: "1");
        }

        /// <summary>
        /// Tests when the settings for MaxStreamSizeInMB do not match.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfigurationUpgradeWithMismatched_MaxStreamSizeInMB()
        {
            ConfigurationUpgradeWithMismatchedTestHelper(
                configName: FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB,
                oldValue: "1024",
                newValue: "512");
        }

        /// <summary>
        /// Tests when the settings for EnableIncrementalBackupsAcrossReplicas do not match.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfigurationUpgradeWithMismatched_EnableIncrementalBackupsAcrossReplicas()
        {
            ConfigurationUpgradeWithMismatchedTestHelper(
                configName: FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.EnableIncrementalBackupsAcrossReplicas,
                oldValue: "false",
                newValue: "true");
        }

        /// <summary>
        /// Tests when the configuration upgrade blocking is expected
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfigurationUpgrade_WhenBlockingUpgradeIsExpected()
        {
            ConfigurationUpgradeHelper(false, true);
        }

        /// <summary>
        /// Tests when the configuration upgrade blocking is not expected
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfigurationUpgrade()
        {
            ConfigurationUpgradeHelper(true);
        }

        private void ConfigurationUpgradeWithMismatchedTestHelper(string configName, string oldValue, string newValue)
        {
            Dictionary<string, List<SettingsOverridesTypeSectionParameter>> currentParameters = new Dictionary<string, List<SettingsOverridesTypeSectionParameter>>();
            List<SettingsOverridesTypeSectionParameter> list = new List<SettingsOverridesTypeSectionParameter>
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FabricValidatorConstants.ParameterNames.NativeRunConfiguration.EnableNativeReliableStateManager,
                    Value = "false",
                    IsEncrypted = false
                }
            };

            List<SettingsOverridesTypeSectionParameter> list2 = new List<SettingsOverridesTypeSectionParameter>
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = configName,
                    Value = oldValue,
                    IsEncrypted = false
                }
            };

            currentParameters.Add(FabricValidatorConstants.SectionNames.NativeRunConfiguration, list);
            currentParameters.Add(FabricValidatorConstants.SectionNames.TransactionalReplicator, list2);

            Dictionary<string, List<SettingsOverridesTypeSectionParameter>> targetParameters = new Dictionary<string, List<SettingsOverridesTypeSectionParameter>>();
            List<SettingsOverridesTypeSectionParameter> list3 = new List<SettingsOverridesTypeSectionParameter>
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FabricValidatorConstants.ParameterNames.NativeRunConfiguration.EnableNativeReliableStateManager,
                    Value = "true",
                    IsEncrypted = false
                }
            };

            List<SettingsOverridesTypeSectionParameter> list4 = new List<SettingsOverridesTypeSectionParameter>
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = configName,
                    Value = newValue,
                    IsEncrypted = false
                }
            };

            targetParameters.Add(FabricValidatorConstants.SectionNames.NativeRunConfiguration, list3);
            targetParameters.Add(FabricValidatorConstants.SectionNames.TransactionalReplicator2.Default, list4);

            Verify.Throws<ArgumentException>(() =>
            {
                TestValidateConfigurationUpgrade(currentParameters, targetParameters);
            },
            string.Format("The change in NativeRunConfiguration is not allowed. The new {0} value is not equal to old value.", configName));
        }

        private void ConfigurationUpgradeHelper(bool loadTestSetting, bool expectedException = false)
        {
            Dictionary<string, List<SettingsOverridesTypeSectionParameter>> currentParameters = new Dictionary<string, List<SettingsOverridesTypeSectionParameter>>();
            List<SettingsOverridesTypeSectionParameter> list = new List<SettingsOverridesTypeSectionParameter>
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FabricValidatorConstants.ParameterNames.NativeRunConfiguration.EnableNativeReliableStateManager,
                    Value = "false",
                    IsEncrypted = false
                }
            };

            currentParameters.Add(FabricValidatorConstants.SectionNames.NativeRunConfiguration, list);

            Dictionary<string, List<SettingsOverridesTypeSectionParameter>> targetParameters = new Dictionary<string, List<SettingsOverridesTypeSectionParameter>>();
            List<SettingsOverridesTypeSectionParameter> list2 = new List<SettingsOverridesTypeSectionParameter>
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FabricValidatorConstants.ParameterNames.NativeRunConfiguration.EnableNativeReliableStateManager,
                    Value = "true",
                    IsEncrypted = false
                }
            };

            if (loadTestSetting)
            {
                List<SettingsOverridesTypeSectionParameter> setting = new List<SettingsOverridesTypeSectionParameter>
                {
                    new SettingsOverridesTypeSectionParameter()
                    {
                        Name = FabricValidatorConstants.ParameterNames.NativeRunConfiguration.Test_LoadEnableNativeReliableStateManager,
                        Value = "true",
                        IsEncrypted = false
                    }
                };

                list2.AddRange(setting);
            }

            targetParameters.Add(FabricValidatorConstants.SectionNames.NativeRunConfiguration, list2);

            if (expectedException)
            {
                Verify.Throws<ArgumentException>(() =>
                {
                    TestValidateConfigurationUpgrade(currentParameters, targetParameters);
                },
                string.Format("The change in NativeRunConfiguration is not allowed"));
            }
            else
            {
                TestValidateConfigurationUpgrade(currentParameters, targetParameters);
            }
        }

        /// <summary>
        /// Helper method to create WindowsFabricSettings object for the provided List of SettingsOverridesTypeSectionParameter.
        /// </summary>
        /// <param name="parameters"></param>
        /// <returns></returns>
        private WindowsFabricSettings GetWindowsFabricSettings(Dictionary<string, List<SettingsOverridesTypeSectionParameter>> parameters)
        {
            ClusterManifestType clusterManifest = (new ClusterManifestHelper(true, false)).ClusterManifest;
            clusterManifest.FabricSettings = new SettingsOverridesTypeSection[parameters.Count];
            int i = 0;

            foreach (var parameter in parameters)
            {
                clusterManifest.FabricSettings[i++] = new SettingsOverridesTypeSection()
                {
                    Name = parameter.Key,
                    Parameter = parameter.Value.ToArray()
                };
            }

            return new WindowsFabricSettings(clusterManifest);
        }

        /// <summary>
        /// Helper method to call ValidateConfiguration method using provided currentParameters.
        /// </summary>
        /// <param name="currentParameters"></param>
        private void TestValidateConfiguration(Dictionary<string, List<SettingsOverridesTypeSectionParameter>> currentParameters)
        {
            Validator.ValidateConfiguration(GetWindowsFabricSettings(currentParameters));
        }

        /// <summary>
        /// Helper method to call ValidateConfigurationUpgrade method using provided currentParameters and targetParameters.
        /// </summary>
        /// <param name="currentParameters"></param>
        /// <param name="targetParameters"></param>
        private void TestValidateConfigurationUpgrade(Dictionary<string, List<SettingsOverridesTypeSectionParameter>> currentParameters, Dictionary<string, List<SettingsOverridesTypeSectionParameter>> targetParameters)
        {
            Validator.ValidateConfigurationUpgrade(GetWindowsFabricSettings(currentParameters), GetWindowsFabricSettings(targetParameters));
        }
    }
}
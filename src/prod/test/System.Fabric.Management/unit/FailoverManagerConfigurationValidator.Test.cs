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
    /// Tests FailoverManagerConfigurationValidator
    /// </summary>
    [TestClass]
    public class FailoverManagerConfigurationValidatorTest
    {
        FailoverManagerConfigurationValidator Validator;
        List<SettingsOverridesTypeSectionParameter> CurrentParameters;
        List<SettingsOverridesTypeSectionParameter> TargetParameters;

        [TestInitialize]
        public void TestSetup()
        {
            Validator = new FailoverManagerConfigurationValidator();
            CurrentParameters = new List<SettingsOverridesTypeSectionParameter>();
            TargetParameters = new List<SettingsOverridesTypeSectionParameter>();
        }

        /// <summary>
        /// Tests when the settings between current and target match.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfigurationUpgradeWithValidParameterValues()
        {
            List<SettingsOverridesTypeSectionParameter> parameters = new List<SettingsOverridesTypeSectionParameter>();
            Validator.ValidateConfigurationUpgrade(GetWindowsFabricSettings(parameters), GetWindowsFabricSettings(parameters));
        }

        /// <summary>
        /// Tests when the settings for TargetReplicaSetSize and MinReplicaSetSize do not match.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfigurationUpgradeWithInvalidParameterValues()
        {
            CurrentParameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.TargetReplicaSetSize, Value = "5", IsEncrypted = false });
            CurrentParameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.MinReplicaSetSize, Value = "3", IsEncrypted = false });

            TargetParameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.TargetReplicaSetSize, Value = "0", IsEncrypted = false });
            TargetParameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.MinReplicaSetSize, Value = "1", IsEncrypted = false });

            Verify.Throws<ArgumentException>(() =>
            {
                TestValidateConfigurationUpgrade(CurrentParameters, TargetParameters);
            },
            "The change in TargetReplicaSetSize is not allowed. The new value: 0 does not match the old one: 5.The change in MinReplicaSetSize is not allowed. The new value: 0 does not match the old one: 3.");
        }

        /// <summary>
        /// Tests when the settings for TargetReplicaSetSize do not match.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfigurationUpgradeWithDifferentTargetReplicaSetSize()
        {
            CurrentParameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.TargetReplicaSetSize, Value = "5", IsEncrypted = false });
            TargetParameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.TargetReplicaSetSize, Value = "0", IsEncrypted = false });

            Verify.Throws<ArgumentException>(() =>
            {
                TestValidateConfigurationUpgrade(CurrentParameters, TargetParameters);
            },
            "The change in TargetReplicaSetSize is not allowed. The new value: 0 does not match the old one: 5.");
        }

        /// <summary>
        /// Tests when the settings for MinReplicaSetSize do not match.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfigurationUpgradeWithDifferentMinReplicaSize()
        {
            CurrentParameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.MinReplicaSetSize, Value = "3", IsEncrypted = false });
            TargetParameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.MinReplicaSetSize, Value = "0", IsEncrypted = false });

            Verify.Throws<ArgumentException>(() =>
            {
                TestValidateConfigurationUpgrade(CurrentParameters, TargetParameters);
            },
            "The change in MinReplicaSetSize is not allowed. The new value: 0 does not match the old one: 3.");
        }

        /// <summary>
        /// Tests when the settings for ReplicaRestartWaitDuration do not match.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfigurationUpgradeWithDifferentReplicaRestartWaitDuration()
        {
            CurrentParameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReplicaRestartWaitDuration, Value = "5", IsEncrypted = false });
            TargetParameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReplicaRestartWaitDuration, Value = "6", IsEncrypted = false });

            Verify.Throws<ArgumentException>(() =>
            {
                TestValidateConfigurationUpgrade(CurrentParameters, TargetParameters);
            },
            "The change in ReplicaRestartWaitDuration is not allowed. The new value: 6 does not match the old one: 5.");
        }

        /// <summary>
        /// Tests when the settings for FullRebuildWaitDuration do not match.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfigurationUpgradeWithDifferentFullRebuildWaitDuration()
        {
            CurrentParameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.FullRebuildWaitDuration, Value = "1.0", IsEncrypted = false });
            TargetParameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.FullRebuildWaitDuration, Value = "2.0", IsEncrypted = false });

            Verify.Throws<ArgumentException>(() =>
            {
                TestValidateConfigurationUpgrade(CurrentParameters, TargetParameters);
            },
            "The change in FullRebuildWaitDuration is not allowed. The new value: 2.0 does not match the old one: 1.0.");
        }

        /// <summary>
        /// Tests when the settings for StandByReplicaKeepDuration do not match.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfigurationUpgradeWithDifferentStandByReplicaKeepDuration()
        {
            CurrentParameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.StandByReplicaKeepDuration, Value = "10", IsEncrypted = false });
            TargetParameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.StandByReplicaKeepDuration, Value = "20", IsEncrypted = false });

            Verify.Throws<ArgumentException>(() =>
            {
                TestValidateConfigurationUpgrade(CurrentParameters, TargetParameters);
            },
            "The change in StandByReplicaKeepDuration is not allowed. The new value: 20 does not match the old one: 10.");
        }

        /// <summary>
        /// Tests when the settings for PlacementConstraints do not match.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfigurationUpgradeWithDifferentPlacementConstraints()
        {
            CurrentParameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.PlacementConstraints, Value = "(NodeType==SystemServices)", IsEncrypted = false });
            TargetParameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.PlacementConstraints, Value = "(NodeType!=SystemServices)", IsEncrypted = false });

            Verify.Throws<ArgumentException>(() =>
            {
                TestValidateConfigurationUpgrade(CurrentParameters, TargetParameters);
            },
            "The change in PlacementConstraints is not allowed. The new value: (NodeType!=SystemServices) does not match the old one: (NodeType==SystemServices).");
        }

        /// <summary>
        /// Tests when current Settings do not have entry for "TargetReplicaSetSize" (missing configuration).
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfigurationUpgradeWithNullCurrentTargetReplicaSize()
        {
            WindowsFabricSettings currentWindowsFabricSettings = GetWindowsFabricSettings(CurrentParameters);
            WindowsFabricSettings targetWindowsFabricSettings = GetWindowsFabricSettings(CurrentParameters);

            // Remove FabricValidatorConstants.ParameterNames.TargetReplicaSetSize setting in currentWindowsFabricSettings
            Dictionary<string, WindowsFabricSettings.SettingsValue> parameterNameToSettingsValue = new Dictionary<string, WindowsFabricSettings.SettingsValue>();
            currentWindowsFabricSettings.SettingsMap.TryGetValue(FabricValidatorConstants.SectionNames.FailoverManager, out parameterNameToSettingsValue);
            parameterNameToSettingsValue.Remove(FabricValidatorConstants.ParameterNames.TargetReplicaSetSize);

            Verify.Throws<ArgumentException>(() =>
            {
                Validator.ValidateConfigurationUpgrade(currentWindowsFabricSettings, targetWindowsFabricSettings);
            },
            "Current value for configuration TargetReplicaSetSize is null.");
        }

        /// <summary>
        /// Helper method to create WindowsFabricSettings object for the provided List of SettingsOverridesTypeSectionParameter.
        /// </summary>
        /// <param name="parameters"></param>
        /// <returns></returns>
        private WindowsFabricSettings GetWindowsFabricSettings(List<SettingsOverridesTypeSectionParameter> parameters)
        {
            ClusterManifestType clusterManifest = (new ClusterManifestHelper(true, false)).ClusterManifest;
            clusterManifest.FabricSettings = new SettingsOverridesTypeSection[1];
            clusterManifest.FabricSettings[0] = new SettingsOverridesTypeSection() { Name = FabricValidatorConstants.SectionNames.FailoverManager, Parameter = parameters.ToArray() };
            return new WindowsFabricSettings(clusterManifest);
        }

        /// <summary>
        /// Helper method to call ValidateConfigurationUpgrade method using provided currentParameters and targetParameters.
        /// </summary>
        /// <param name="currentParameters"></param>
        /// <param name="targetParameters"></param>
        private void TestValidateConfigurationUpgrade(List<SettingsOverridesTypeSectionParameter> currentParameters, List<SettingsOverridesTypeSectionParameter> targetParameters)
        {
            WindowsFabricSettings currentWindowsFabricSettings = GetWindowsFabricSettings(currentParameters);
            WindowsFabricSettings targetWindowsFabricSettings = GetWindowsFabricSettings(targetParameters);

            Validator.ValidateConfigurationUpgrade(GetWindowsFabricSettings(currentParameters), GetWindowsFabricSettings(targetParameters));
        }
    }
}
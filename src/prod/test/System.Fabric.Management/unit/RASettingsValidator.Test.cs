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
    /// Tests RASettingsValidator
    /// </summary>
    [TestClass]
    public class RASettingsValidatorTest
    {
        RASettingsValidator Validator;
        List<SettingsOverridesTypeSectionParameter> parameters;

        [TestInitialize]
        public void TestSetup()
        {
            Validator = new RASettingsValidator();
            parameters = new List<SettingsOverridesTypeSectionParameter>();
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.IsDeactivationInfoEnabled is set to true.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfigurationUpgradeWithIsDeactivationInfoEnabledTrue()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.IsDeactivationInfoEnabled, Value = "true", IsEncrypted = false });
            Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.IsDeactivationInfoEnabled is set to false.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateConfigurationUpgradeWithIsDeactivationInfoEnabledFalse()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.IsDeactivationInfoEnabled, Value = "false", IsEncrypted = false });

            Verify.Throws<ArgumentException>(() =>
            {
                Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
            },
            "IsDeactivationInfoEnabled can not be false.");
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.GracefulReplicaShutdownMaxDuration is set to negative value.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestGracefulReplicaShutdownMaxDurationSetToNegativeValue()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.GracefulReplicaShutdownMaxDuration, Value = "-1", IsEncrypted = false });

            Verify.Throws<ArgumentException>(() =>
            {
                Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
            },
            "Section ReconfigurationAgent parameter GracefulReplicaShutdownMaxDuration value -1 validation failed with error Int value should be greater than or equal to 0");
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.GracefulReplicaShutdownMaxDuration is set to zero.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestGracefulReplicaShutdownMaxDurationSetToZero()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.GracefulReplicaShutdownMaxDuration, Value = "0", IsEncrypted = false });
            Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.GracefulReplicaShutdownMaxDuration is set to positive value.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestGracefulReplicaShutdownMaxDurationSetToPositiveValue()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.GracefulReplicaShutdownMaxDuration, Value = "1", IsEncrypted = false });
            Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ReplicaChangeRoleFailureRestartThreshold is set to negative value.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestReplicaChangeRoleFailureRestartThresholdSetToNegativeValue()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ReplicaChangeRoleFailureRestartThreshold, Value = "-1", IsEncrypted = false });

            Verify.Throws<ArgumentException>(() =>
            {
                Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
            },
            "Section ReconfigurationAgent parameter ReplicaChangeRoleFailureRestartThreshold value -1 validation failed with error Int value should be greater than or equal to 0");
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ReplicaChangeRoleFailureRestartThreshold is set to zero.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestReplicaChangeRoleFailureRestartThresholdSetToZero()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ReplicaChangeRoleFailureRestartThreshold, Value = "0", IsEncrypted = false });
            Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ReplicaChangeRoleFailureRestartThreshold is set to positive value.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestReplicaChangeRoleFailureRestartThresholdSetToPositiveValue()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ReplicaChangeRoleFailureRestartThreshold, Value = "1", IsEncrypted = false });
            Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ReplicaChangeRoleFailureWarningReportThreshold is set to negative value.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestReplicaChangeRoleFailureWarningReportThresholdSetToNegativeValue()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ReplicaChangeRoleFailureWarningReportThreshold, Value = "-1", IsEncrypted = false });

            Verify.Throws<ArgumentException>(() =>
            {
                Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
            },
            "Section ReconfigurationAgent parameter ReplicaChangeRoleFailureWarningReportThreshold value -1 validation failed with error Int value should be greater than or equal to 0");
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ReplicaChangeRoleFailureWarningReportThreshold is set to zero.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestReplicaChangeRoleFailureWarningReportThresholdSetToZero()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ReplicaChangeRoleFailureWarningReportThreshold, Value = "0", IsEncrypted = false });
            Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ReplicaChangeRoleFailureWarningReportThreshold is set to positive value.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestReplicaChangeRoleFailureWarningReportThresholdSetToPositiveValue()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ReplicaChangeRoleFailureWarningReportThreshold, Value = "1", IsEncrypted = false });
            Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ApplicationUpgradeMaxReplicaCloseDuration is set to negative value.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplicationUpgradeMaxReplicaCloseDurationSetToNegativeValue()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ApplicationUpgradeMaxReplicaCloseDuration, Value = "-1", IsEncrypted = false });

            Verify.Throws<ArgumentException>(() =>
            {
                Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
            },
            "Section ReconfigurationAgent parameter ApplicationUpgradeMaxReplicaCloseDuration value -1 validation failed with error Int value should be greater than or equal to 0");
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ApplicationUpgradeMaxReplicaCloseDuration is set to zero.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplicationUpgradeMaxReplicaCloseDurationSetToZero()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ApplicationUpgradeMaxReplicaCloseDuration, Value = "0", IsEncrypted = false });
            Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ApplicationUpgradeMaxReplicaCloseDuration is set to positive value.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplicationUpgradeMaxReplicaCloseDurationSetToPositiveValue()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ApplicationUpgradeMaxReplicaCloseDuration, Value = "1", IsEncrypted = false });
            Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ServiceApiHealthDuration is set to negative value.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServiceApiHealthDurationSetToNegativeValue()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ServiceApiHealthDuration, Value = "-1", IsEncrypted = false });

            Verify.Throws<ArgumentException>(() =>
            {
                Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
            },
            "Section ReconfigurationAgent parameter ServiceApiHealthDuration value -1 validation failed with error Int value should be greater than or equal to 0");
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ServiceApiHealthDuration is set to zero.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServiceApiHealthDurationSetToZero()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ServiceApiHealthDuration, Value = "0", IsEncrypted = false });
            Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ServiceApiHealthDuration is set to positive value.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServiceApiHealthDurationSetToPositiveValue()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ServiceApiHealthDuration, Value = "1", IsEncrypted = false });
            Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.PeriodicApiSlowTraceInterval is set to negative value.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPeriodicApiSlowTraceIntervalSetToNegativeValue()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.PeriodicApiSlowTraceInterval, Value = "-1", IsEncrypted = false });

            Verify.Throws<ArgumentException>(() =>
            {
                Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
            },
            "Section ReconfigurationAgent parameter PeriodicApiSlowTraceInterval value -1 validation failed with error Int value should be greater than or equal to 0");
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.PeriodicApiSlowTraceInterval is set to zero.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPeriodicApiSlowTraceIntervalSetToZero()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.PeriodicApiSlowTraceInterval, Value = "0", IsEncrypted = false });
            Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.PeriodicApiSlowTraceInterval is set to positive value.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPeriodicApiSlowTraceIntervalSetToPositiveValue()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.PeriodicApiSlowTraceInterval, Value = "1", IsEncrypted = false });
            Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.NodeDeactivationMaxReplicaCloseDuration is set to negative value.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestNodeDeactivationMaxReplicaCloseDurationSetToNegativeValue()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.NodeDeactivationMaxReplicaCloseDuration, Value = "-1", IsEncrypted = false });

            Verify.Throws<ArgumentException>(() =>
            {
                Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
            },
            "Section ReconfigurationAgent parameter NodeDeactivationMaxReplicaCloseDuration value -1 validation failed with error Int value should be greater than or equal to 0");
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.NodeDeactivationMaxReplicaCloseDuration is set to zero.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestNodeDeactivationMaxReplicaCloseDurationSetToZero()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.NodeDeactivationMaxReplicaCloseDuration, Value = "0", IsEncrypted = false });
            Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.NodeDeactivationMaxReplicaCloseDuration is set to positive value.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestNodeDeactivationMaxReplicaCloseDurationSetToPositiveValue()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.NodeDeactivationMaxReplicaCloseDuration, Value = "1", IsEncrypted = false });
            Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.FabricUpgradeMaxReplicaCloseDuration is set to negative value.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestFabricUpgradeMaxReplicaCloseDurationSetToNegativeValue()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.FabricUpgradeMaxReplicaCloseDuration, Value = "-1", IsEncrypted = false });

            Verify.Throws<ArgumentException>(() =>
            {
                Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
            },
            "Section ReconfigurationAgent parameter FabricUpgradeMaxReplicaCloseDuration value -1 validation failed with error Int value should be greater than or equal to 0");
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.FabricUpgradeMaxReplicaCloseDuration is set to zero.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestFabricUpgradeMaxReplicaCloseDurationSetToZero()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.FabricUpgradeMaxReplicaCloseDuration, Value = "0", IsEncrypted = false });
            Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
        }

        /// <summary>
        /// Tests when FabricValidatorConstants.ParameterNames.ReconfigurationAgent.FabricUpgradeMaxReplicaCloseDuration is set to positive value.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestFabricUpgradeMaxReplicaCloseDurationSetToPositiveValue()
        {
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ReconfigurationAgent.FabricUpgradeMaxReplicaCloseDuration, Value = "1", IsEncrypted = false });
            Validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
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
            clusterManifest.FabricSettings[0] = new SettingsOverridesTypeSection() { Name = FabricValidatorConstants.SectionNames.ReconfigurationAgent, Parameter = parameters.ToArray() };
            return new WindowsFabricSettings(clusterManifest);
        }
    }
}
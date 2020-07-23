// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// This is the class handles the validation of configuration for the FailoverManager section.
    /// </summary>
    class FailoverManagerConfigurationValidator : BaseFabricConfigurationValidator
    {   
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.FailoverManager; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            
            ValueValidator.VerifyIntValueGreaterThanInput(settingsForThisSection, FabricValidatorConstants.ParameterNames.TargetReplicaSetSize, 0, SectionName);
            ValueValidator.VerifyIntValueGreaterThanInput(settingsForThisSection, FabricValidatorConstants.ParameterNames.MinReplicaSetSize, 0, SectionName);
            ValueValidator.VerifyFirstIntParameterGreaterThanOrEqualToSecondIntParameter(settingsForThisSection, FabricValidatorConstants.ParameterNames.TargetReplicaSetSize, FabricValidatorConstants.ParameterNames.MinReplicaSetSize, SectionName);
            
            FabricValidatorUtility.ValidateExpression(settingsForThisSection, FabricValidatorConstants.ParameterNames.PlacementConstraints);

            ValueValidator.VerifyIntValueGreaterThanEqualToInput(settingsForThisSection, FabricValidatorConstants.ParameterNames.MaxUserStandByReplicaCount, 0, SectionName);
            ValueValidator.VerifyIntValueGreaterThanEqualToInput(settingsForThisSection, FabricValidatorConstants.ParameterNames.MaxSystemStandByReplicaCount, 0, SectionName);
        }

        /// <summary>
        /// Validates the FailoverManager parameters for configuration upgrade.
        /// </summary>
        /// <param name="currentWindowsFabricSettings"></param>
        /// <param name="targetWindowsFabricSettings"></param>
        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {
            StringBuilder errorMessageBuilder = new StringBuilder();
            
            ValidateConfigurationUpgrade(
                currentWindowsFabricSettings,
                targetWindowsFabricSettings,
                FabricValidatorConstants.ParameterNames.TargetReplicaSetSize,
                errorMessageBuilder);
            ValidateConfigurationUpgrade(
                currentWindowsFabricSettings,
                targetWindowsFabricSettings,
                FabricValidatorConstants.ParameterNames.MinReplicaSetSize,
                errorMessageBuilder);
            ValidateConfigurationUpgrade(
                currentWindowsFabricSettings,
                targetWindowsFabricSettings,
                FabricValidatorConstants.ParameterNames.ReplicaRestartWaitDuration,
                errorMessageBuilder);
            ValidateConfigurationUpgrade(
                currentWindowsFabricSettings,
                targetWindowsFabricSettings,
                FabricValidatorConstants.ParameterNames.FullRebuildWaitDuration,
                errorMessageBuilder);
            ValidateConfigurationUpgrade(
                currentWindowsFabricSettings,
                targetWindowsFabricSettings,
                FabricValidatorConstants.ParameterNames.StandByReplicaKeepDuration,
                errorMessageBuilder);
            ValidateConfigurationUpgrade(
                currentWindowsFabricSettings,
                targetWindowsFabricSettings,
                FabricValidatorConstants.ParameterNames.PlacementConstraints,
                errorMessageBuilder);

            if (errorMessageBuilder.Length > 0)
            {
                throw new ArgumentException(errorMessageBuilder.ToString());
            }
        }

        /// <summary>
        /// Validates the provided WindowsFabricSettings for the given configurationName for configuration upgrade.
        /// </summary>
        /// <param name="currentWindowsFabricSettings"></param>
        /// <param name="targetWindowsFabricSettings"></param>
        /// <param name="configurationName"></param>
        /// <param name="errorMessageBuilder"></param>
        private void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings,
            WindowsFabricSettings targetWindowsFabricSettings,
            string configurationName,
            StringBuilder errorMessageBuilder)
        {
            var currentSettingsValue = currentWindowsFabricSettings.GetParameter(
                this.SectionName, configurationName);
            var targetSettingsValue = targetWindowsFabricSettings.GetParameter(
                this.SectionName, configurationName);

            if (!IsConfigurationSettingsValid(currentSettingsValue, targetSettingsValue, configurationName, errorMessageBuilder))
            {
                return; //return if configuration settings is not valid
            }

            string oldValue = currentSettingsValue.Value;
            string newValue = targetSettingsValue.Value;

            if (!oldValue.Equals(newValue))
            {
                errorMessageBuilder.Append(string.Format("The change in {0} is not allowed. The new value: {1} does not match the old one: {2}.",
                    configurationName,
                    newValue,
                    oldValue));
            }
        }

        /// <summary>
        /// Returns true if the provided WindowsFabricSettings.SettingsValue is valid.
        /// </summary>
        /// <param name="currentWindowsFabricSettings"></param>
        /// <param name="targetWindowsFabricSettings"></param>
        /// <param name="configurationName"></param>
        /// <param name="errorMessageBuilder"></param>
        /// <returns></returns>
        private bool IsConfigurationSettingsValid(WindowsFabricSettings.SettingsValue currentSettingsValue,
            WindowsFabricSettings.SettingsValue targetSettingsValue,
            string configurationName,
            StringBuilder errorMessageBuilder)
        {
            bool isSettingsValid = true;

            if (currentSettingsValue == null)
            {
                errorMessageBuilder.Append(string.Format("Current value for configuration {0} is null.", configurationName));
                isSettingsValid = false;
            }

            if (targetSettingsValue == null)
            {
                errorMessageBuilder.Append(string.Format("Target value for configuration {0} is null.", configurationName));
                isSettingsValid = false;
            }

            return isSettingsValid;
        }
    }
}
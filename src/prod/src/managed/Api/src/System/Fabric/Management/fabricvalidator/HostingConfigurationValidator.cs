// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System;
    using System.Globalization;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;

    /// <summary>
    /// This is the class handles the validation of configuration for the hosting subsystem
    /// </summary>
    class HostingConfigurationValidator : BaseFabricConfigurationValidator
    {
        private const int MinEtwTraceLevel = 1;
        private const int MaxEtwTraceLevel = 5;

        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.Hosting; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            ValueValidator.VerifyIntValueGreaterThanEqualToInput(
                settingsForThisSection,
                FabricValidatorConstants.ParameterNames.ApplicationEtwTraceLevel,
                MinEtwTraceLevel,
                this.SectionName);

            ValueValidator.VerifyIntValueLessThanEqualToInput(
                settingsForThisSection,
                FabricValidatorConstants.ParameterNames.ApplicationEtwTraceLevel,
                MaxEtwTraceLevel,
                this.SectionName);

            ValidateApplicationUpgradeConfiguration(windowsFabricSettings);
        }

        //Validate if the ApplicationUpgradeTimeout is more than ActivationTimeout for Switch to happen during ApplicationUpgrade
        public void ValidateApplicationUpgradeConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            string activationTimeout = windowsFabricSettings.SettingsMap[FabricValidatorConstants.SectionNames.Hosting]
                [FabricValidatorConstants.ParameterNames.Hosting.ActivationTimeout].Value;

            string applicationUpgradeTimeout = windowsFabricSettings.SettingsMap[FabricValidatorConstants.SectionNames.Hosting]
                [FabricValidatorConstants.ParameterNames.Hosting.ApplicationUpgradeTimeout].Value;

            var activationTimeSpan = TimeSpan.Parse(activationTimeout);
            var applicationUpgradeTimeSpan = TimeSpan.Parse(applicationUpgradeTimeout);

            if (applicationUpgradeTimeSpan < activationTimeSpan)
            {
                FabricValidator.TraceSource.WriteWarning(
                     FabricValidatorUtility.TraceTag,
                            StringResources.Warning_FabricValidator_ParameterShouldBeGreaterThanAnotherParameter,
                            FabricValidatorConstants.SectionNames.Hosting,
                            FabricValidatorConstants.ParameterNames.Hosting.ApplicationUpgradeTimeout,
                            applicationUpgradeTimeout,
                            FabricValidatorConstants.ParameterNames.Hosting.ActivationTimeout,
                            activationTimeout);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }
}
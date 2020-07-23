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

    /// <summary>
    /// This is the class handles the validation of configuration for performance counter collection section.
    /// </summary>
    class PerformanceCountersConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.PerformanceCounterLocalStore; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            if (settingsForThisSection.ContainsKey(FabricValidatorConstants.ParameterNames.IsEnabled))
            {
                bool isEnabled = bool.Parse(settingsForThisSection[FabricValidatorConstants.ParameterNames.IsEnabled].Value);
                if (!isEnabled)
                {
                    FabricValidatorUtility.WriteWarningInVSOutputFormat(
                        FabricValidatorUtility.TraceTag,
                        "Performance counter collection on node is disabled because section '{0}' parameter '{1}' is set to 'false'.",
                        FabricValidatorConstants.SectionNames.PerformanceCounterLocalStore,
                        FabricValidatorConstants.ParameterNames.IsEnabled);
                }
            }

            if (settingsForThisSection[FabricValidatorConstants.ParameterNames.Counters].IsFromClusterManifest)
            {
                string counterList = settingsForThisSection[FabricValidatorConstants.ParameterNames.Counters].Value;
                string[] counters = counterList.Split(',');
                if (counters.Length == 0)
                {
                    throw new ArgumentException (
                        string.Format(
                        "Section '{0}' parameter '{1}' should specify at least one performance counter.",
                        FabricValidatorConstants.SectionNames.PerformanceCounterLocalStore,
                        FabricValidatorConstants.ParameterNames.Counters));
                }

                bool defaultCountersSpecified = false;
                bool emptyStringPresent = false;
                foreach (string counterAsIs in counters)
                {
                    string counter = counterAsIs.Trim();
                    if (String.IsNullOrEmpty(counter))
                    {
                        emptyStringPresent = true;
                    }
                    else if (counter.Equals(FabricValidatorConstants.DefaultTag, StringComparison.OrdinalIgnoreCase))
                    {
                        defaultCountersSpecified = true;
                    }
                }

                if (emptyStringPresent)
                {
                    throw new ArgumentException(
                        string.Format(
                        "The counter list in section '{0}' parameter '{1}' contains at least one empty string.",
                        FabricValidatorConstants.SectionNames.PerformanceCounterLocalStore,
                        FabricValidatorConstants.ParameterNames.Counters));
                }

                if (false == defaultCountersSpecified)
                {
                    FabricValidatorUtility.WriteWarningInVSOutputFormat(
                        FabricValidatorUtility.TraceTag,
                        "Section '{0}' parameter '{1}' does not include the default performance counters representation ({2}). Therefore, the default performance counters will not be collected.",
                        FabricValidatorConstants.SectionNames.PerformanceCounterLocalStore,
                        FabricValidatorConstants.ParameterNames.Counters,
                        FabricValidatorConstants.DefaultTag);
                }
            }

            if (settingsForThisSection[FabricValidatorConstants.ParameterNames.NewCounterBinaryFileCreationIntervalMinutes].IsFromClusterManifest)
            {
                int minRecommendedNewFileCreationIntervalMinutes = 5;
                int maxNewFileCreationIntervalMinutes = int.MaxValue/60;

                ValueValidator.VerifyIntValueGreaterThanEqualToInput(
                    settingsForThisSection,
                    FabricValidatorConstants.ParameterNames.NewCounterBinaryFileCreationIntervalMinutes,
                    minRecommendedNewFileCreationIntervalMinutes,
                    this.SectionName);

                ValueValidator.VerifyIntValueLessThanEqualToInput(
                              settingsForThisSection,
                              FabricValidatorConstants.ParameterNames.NewCounterBinaryFileCreationIntervalMinutes,
                              maxNewFileCreationIntervalMinutes,
                              this.SectionName);
            }

            if (settingsForThisSection[FabricValidatorConstants.ParameterNames.TestOnlyCounterFilePath].IsFromClusterManifest)
            {
                string testOnlyCounterFilePath = settingsForThisSection[FabricValidatorConstants.ParameterNames.TestOnlyCounterFilePath].Value;
                if (String.IsNullOrEmpty(testOnlyCounterFilePath))
                {
                    throw new ArgumentException(String.Format(
                        "Section '{0}' parameter '{1}' should not be an empty string.",
                        FabricValidatorConstants.SectionNames.PerformanceCounterLocalStore,
                        FabricValidatorConstants.ParameterNames.TestOnlyCounterFilePath));
                }
            }

            if (settingsForThisSection[FabricValidatorConstants.ParameterNames.TestOnlyCounterFileNamePrefix].IsFromClusterManifest)
            {
                string testOnlyCounterFileNamePrefix = settingsForThisSection[FabricValidatorConstants.ParameterNames.TestOnlyCounterFileNamePrefix].Value;
                if (String.IsNullOrEmpty(testOnlyCounterFileNamePrefix))
                {
                    throw new ArgumentException(String.Format(
                        "Section '{0}' parameter '{1}' should not be an empty string.",
                        FabricValidatorConstants.SectionNames.PerformanceCounterLocalStore,
                        FabricValidatorConstants.ParameterNames.TestOnlyCounterFileNamePrefix));
                }
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

}
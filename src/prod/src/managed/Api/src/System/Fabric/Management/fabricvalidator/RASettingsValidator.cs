// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Configuration;

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;

    internal class RASettingsValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.ReconfigurationAgent; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var RASettingsMap = windowsFabricSettings.SettingsMap[SectionName];

            string apiHealthReportWaitDuration = RASettingsMap[FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ServiceApiHealthDuration].Value;
            string reconfigurationHealthReportWaitDuration = RASettingsMap[FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ServiceReconfigurationApiHealthDuration].Value;

            var apiHealthReportTimeSpan = TimeSpan.Parse(apiHealthReportWaitDuration);
            var reconfigurationHealthReportTimeSpan = TimeSpan.Parse(reconfigurationHealthReportWaitDuration);
            if (apiHealthReportTimeSpan < reconfigurationHealthReportTimeSpan)
            {
                FabricValidator.TraceSource.WriteWarning(
                            FabricValidatorUtility.TraceTag,
                            StringResources.Warning_ShouldBeSmaller,
                            SectionName,
                            FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ServiceReconfigurationApiHealthDuration,
                            SectionName,
                            FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ServiceApiHealthDuration);
            }

            // Validate IsDeactivationInfoEnabled should always be true
            var isDeactivationInfoEnabledStr = RASettingsMap[FabricValidatorConstants.ParameterNames.ReconfigurationAgent.IsDeactivationInfoEnabled].Value;

            if (!string.IsNullOrEmpty(isDeactivationInfoEnabledStr) && !bool.Parse(isDeactivationInfoEnabledStr))
            {
                throw new ArgumentException("IsDeactivationInfoEnabled can not be false.");
            }

            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            ValueValidator.VerifyIntValueGreaterThanEqualToInput(settingsForThisSection, FabricValidatorConstants.ParameterNames.ReconfigurationAgent.GracefulReplicaShutdownMaxDuration, 0, SectionName);
            ValueValidator.VerifyIntValueGreaterThanEqualToInput(settingsForThisSection, FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ReplicaChangeRoleFailureRestartThreshold, 0, SectionName);
            ValueValidator.VerifyIntValueGreaterThanEqualToInput(settingsForThisSection, FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ReplicaChangeRoleFailureWarningReportThreshold, 0, SectionName);
            ValueValidator.VerifyIntValueGreaterThanEqualToInput(settingsForThisSection, FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ApplicationUpgradeMaxReplicaCloseDuration, 0, SectionName);
            ValueValidator.VerifyIntValueGreaterThanEqualToInput(settingsForThisSection, FabricValidatorConstants.ParameterNames.ReconfigurationAgent.ServiceApiHealthDuration, 0, SectionName);
            ValueValidator.VerifyIntValueGreaterThanEqualToInput(settingsForThisSection, FabricValidatorConstants.ParameterNames.ReconfigurationAgent.PeriodicApiSlowTraceInterval, 0, SectionName);
            ValueValidator.VerifyIntValueGreaterThanEqualToInput(settingsForThisSection, FabricValidatorConstants.ParameterNames.ReconfigurationAgent.NodeDeactivationMaxReplicaCloseDuration, 0, SectionName);
            ValueValidator.VerifyIntValueGreaterThanEqualToInput(settingsForThisSection, FabricValidatorConstants.ParameterNames.ReconfigurationAgent.FabricUpgradeMaxReplicaCloseDuration, 0, SectionName);
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }
}
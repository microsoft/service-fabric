// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// This is the class handles the validation of configuration for the UpgradeService section.
    /// </summary>
    class UpgradeServiceConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.UpgradeService; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            int warningThreshold = windowsFabricSettings.GetParameter(this.SectionName, FabricValidatorConstants.ParameterNames.UpgradeService.ContinuousFailureWarningThreshold).GetValue<int>();
            int errorThreshold = windowsFabricSettings.GetParameter(this.SectionName, FabricValidatorConstants.ParameterNames.UpgradeService.ContinuousFailureErrorThreshold).GetValue<int>();
            int faultThreshold = windowsFabricSettings.GetParameter(this.SectionName, FabricValidatorConstants.ParameterNames.UpgradeService.ContinuousFailureFaultThreshold).GetValue<int>();

            if (warningThreshold > errorThreshold)
            {
                throw new ArgumentException(
                      string.Format(
                            CultureInfo.InvariantCulture,
                            StringResources.Error_FabricValidator_ParameterShouldBeGreaterThanAnotherParameter,
                            FabricValidatorConstants.SectionNames.UpgradeService,
                            FabricValidatorConstants.ParameterNames.UpgradeService.ContinuousFailureErrorThreshold,
                            errorThreshold,
                            FabricValidatorConstants.ParameterNames.UpgradeService.ContinuousFailureWarningThreshold,
                            warningThreshold));
            }

            if (errorThreshold > faultThreshold)
            {
                throw new ArgumentException(
                      string.Format(
                            CultureInfo.InvariantCulture,
                            StringResources.Error_FabricValidator_ParameterShouldBeGreaterThanAnotherParameter,
                            FabricValidatorConstants.SectionNames.UpgradeService,
                            FabricValidatorConstants.ParameterNames.UpgradeService.ContinuousFailureFaultThreshold,
                            faultThreshold,
                            FabricValidatorConstants.ParameterNames.UpgradeService.ContinuousFailureErrorThreshold,
                            errorThreshold));
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }
}
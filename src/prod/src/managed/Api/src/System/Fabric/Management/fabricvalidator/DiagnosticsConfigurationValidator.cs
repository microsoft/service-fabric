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
    /// This is the class handles the validation of configuration for the Diagnostics section.
    /// </summary>
    class DiagnosticsConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.Diagnostics; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            ValueValidator.VerifyIntValueGreaterThanEqualToInput(settingsForThisSection, FabricValidatorConstants.ParameterNames.ApplicationLogsFormatVersion, 0, SectionName);
            ValueValidator.VerifyIntValueLessThanEqualToInput(settingsForThisSection, FabricValidatorConstants.ParameterNames.ApplicationLogsFormatVersion, 1, SectionName);
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {
        }
    }
}
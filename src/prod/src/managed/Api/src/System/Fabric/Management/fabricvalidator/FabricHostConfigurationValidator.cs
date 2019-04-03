// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Management.ServiceModel;

    /// <summary>
    /// This is the class handles the validation of configuration for fabric host section.
    /// </summary>
    class FabricHostConfigurationValidator : BaseFabricConfigurationValidator
    {

        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.FabricHost; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            ValueValidator.VerifyIntValueGreaterThanInput(settingsForThisSection, FabricValidatorConstants.ParameterNames.ActivationMaxFailureCount, 0, SectionName);
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

}
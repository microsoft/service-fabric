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

    /// <summary>
    /// This is the class handles the validation of configuration for the Infrastructure Service section.
    /// </summary>

    class InfrastructureService:BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.InfrastructureService; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            if (settingsForThisSection.ContainsKey(FabricValidatorConstants.ParameterNames.PlacementConstraints))
            {
                FabricValidatorUtility.ValidateExpression(settingsForThisSection, FabricValidatorConstants.ParameterNames.PlacementConstraints);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }
}
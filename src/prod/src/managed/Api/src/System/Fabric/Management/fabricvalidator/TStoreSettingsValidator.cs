// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System;
    using System.Collections.Generic;

    internal class TStoreSettingValidator : BaseFabricConfigurationValidator
    {
        // Inclusive max sweep threshold
        public const int MinSweepThreshold = -1;

        // Inclusive max sweep threshold : 5 million.
        public const int MaxSweepThreshold = 5000000;

        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.ReliableStateProvider; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var storeSectionSettings = windowsFabricSettings.GetSection(this.SectionName);

            ValueValidator.VerifyIntValueGreaterThanEqualToInput(storeSectionSettings, FabricValidatorConstants.ParameterNames.ReliableStateProvider.SweepThreshold, MinSweepThreshold, this.SectionName);
            ValueValidator.VerifyIntValueLessThanEqualToInput(storeSectionSettings, FabricValidatorConstants.ParameterNames.ReliableStateProvider.SweepThreshold, MaxSweepThreshold, this.SectionName);
        }

        public override void ValidateConfigurationUpgrade(
            WindowsFabricSettings currentWindowsFabricSettings,
            WindowsFabricSettings targetWindowsFabricSettings)
        {
            // No checks required in upgrade for now.
        }
    }
}
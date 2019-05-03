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
    /// This is the class handles the validation of configuration for the ClusterManager section.
    /// </summary>
    class LocalNatConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.Common; }
        }

        public override void ValidateConfiguration(
            WindowsFabricSettings windowsFabricSettings)
        {
            
            var settingsHostingSection = windowsFabricSettings.GetSection(FabricValidatorConstants.SectionNames.Hosting);
            bool isLocalNatIPProviderEnabled = settingsHostingSection[FabricValidatorConstants.ParameterNames.Hosting.LocalNatIPProviderEnabled].GetValue<bool>();

            // If LocalNatIPProviderEnabled is provided, it should be the only network config setting (no open, no isolated settings)
            if (isLocalNatIPProviderEnabled)
            {
                var settingsSetupSection = windowsFabricSettings.GetSection(FabricValidatorConstants.SectionNames.Setup);

                bool ipProviderEnabled = settingsHostingSection[FabricValidatorConstants.ParameterNames.Hosting.IPProviderEnabled].GetValue<bool>();
                bool containerNetworkSetup = settingsHostingSection[FabricValidatorConstants.ParameterNames.Setup.ContainerNetworkSetup].GetValue<bool>();
                bool isolatedNetworkSetup = settingsHostingSection[FabricValidatorConstants.ParameterNames.Setup.IsolatedNetworkSetup].GetValue<bool>();
                
                if (ipProviderEnabled || containerNetworkSetup || isolatedNetworkSetup)
                {
                    throw new InvalidOperationException("Cannot use specify LocalNatIPProviderEnabled with Open or Isolated.");
                }

                var LocalNatIpProviderNetworkName = settingsHostingSection[FabricValidatorConstants.ParameterNames.Hosting.LocalNatIpProviderNetworkName].GetValue<string>();
                var LocalNatIpProviderNetworkRange = settingsHostingSection[FabricValidatorConstants.ParameterNames.Hosting.LocalNatIpProviderNetworkName].GetValue<string>();
                if (string.IsNullOrEmpty(LocalNatIpProviderNetworkName))
                {
                    throw new InvalidOperationException("LocalNatIpProviderNetworkName must be specified.");
                }
                if (string.IsNullOrEmpty(LocalNatIpProviderNetworkName))
                {
                    throw new InvalidOperationException("LocalNatIpProviderNetworkRange must be specified in valid CIDR format.");
                }
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }
}
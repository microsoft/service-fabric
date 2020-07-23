// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;
    using System.Security.Cryptography.X509Certificates;

    class SetupSettings
    {
        public string FabricDataRoot { get; private set; }

        public string FabricLogRoot { get; private set; }

        public bool EnableCircularTraceSession { get; private set; }

        public string ServiceRunAsType { get; private set; }

        public string ServiceRunAsAccountName { get; private set; }

        public string ServiceRunAsPassword { get; private set; }

        public bool SkipFirewallConfiguration { get; private set; }

        public string ServiceStartupType { get; private set; }

        // Settings for preview features that need lightup
        public bool EnableUnsupportedPreviewFeatures { get; private set; }

        public bool IsSFVolumeDiskServiceEnabled { get; private set; }

        /// <summary>
        /// This setting comes from the cluster manifest.
        /// <seealso cref="DeploymentParameters.ContainerDnsSetup"/>.
        /// </summary>
        /// <remarks>
        /// <see cref="DeploymentParameters.ContainerDnsSetup"/> is updated with this value
        /// and passed into the relevant <see cref="DeploymentOperation"/>.
        /// This setting is picked up in Configurations.csv because of an explicit entry made in 
        /// ${srcroot}/prod/tools/ConfigurationValidator/GenerateConfigurationsCSV.pl</remarks>
        public ContainerDnsSetup ContainerDnsSetup { get; private set; }

        public bool ContainerNetworkSetup { get; private set; }

        public string ContainerNetworkName { get; private set; }

#if !DotNetCoreClrLinux
        public bool SkipContainerNetworkResetOnReboot { get; private set; }

        public bool SkipIsolatedNetworkResetOnReboot { get; private set; }
#endif        

        public bool IsolatedNetworkSetup { get; private set; }

        public string IsolatedNetworkName { get; private set; }

        public string IsolatedNetworkInterfaceName { get; private set; }

        public bool UseContainerServiceArguments { get; private set; }

        public string ContainerServiceArguments { get; private set; }

        public bool EnableContainerServiceDebugMode { get; private set; }

        public SetupSettings(ClusterManifestType manifest)
        {
            var setupSection = manifest.FabricSettings.FirstOrDefault(section => section.Name.Equals(Constants.SectionNames.Setup, StringComparison.OrdinalIgnoreCase));
            this.FabricDataRoot = GetValueFromManifest(setupSection, Constants.ParameterNames.FabricDataRoot);
            this.FabricLogRoot = GetValueFromManifest(setupSection, Constants.ParameterNames.FabricLogRoot);
            this.ServiceRunAsAccountName = GetValueFromManifest(setupSection, Constants.ParameterNames.ServiceRunAsAccountName);
            this.ServiceRunAsPassword = GetValueFromManifest(setupSection, Constants.ParameterNames.ServiceRunAsPassword);
            this.SkipFirewallConfiguration = GetBoolFromManifest(setupSection, Constants.ParameterNames.SkipFirewallConfiguration);
            this.ServiceStartupType = GetValueFromManifest(setupSection, Constants.ParameterNames.ServiceStartupType);
            
            // By default this is set to FlatNetworkConstants.NetworkName unless customers override this in fabric settings
            var containerNetworkName = GetValueFromManifest(setupSection, Constants.ParameterNames.ContainerNetworkName);
            this.ContainerNetworkName = (!string.IsNullOrEmpty(containerNetworkName)) ? (containerNetworkName) : (FlatNetworkConstants.NetworkName);

            var containerNetworkSetupStr = GetValueFromManifest(setupSection, Constants.ParameterNames.ContainerNetworkSetup);
            
#if !DotNetCoreClrLinux
            // For windows, this is true by default unless customers override this in fabric settings
            this.ContainerNetworkSetup = (!string.IsNullOrEmpty(containerNetworkSetupStr)) ? GetBoolFromManifest(setupSection, Constants.ParameterNames.ContainerNetworkSetup) : true;

            // For windows container network, this is false by default unless customers override this in fabric settings
            var skipContainerNetworkResetOnRebootStr = GetValueFromManifest(setupSection, Constants.ParameterNames.SkipContainerNetworkResetOnReboot);
            this.SkipContainerNetworkResetOnReboot = (!string.IsNullOrEmpty(skipContainerNetworkResetOnRebootStr)) ? GetBoolFromManifest(setupSection, Constants.ParameterNames.SkipContainerNetworkResetOnReboot) : false;

            // For windows isolated network, this is false by default unless customers override this in fabric settings
            var skipIsolatedNetworkResetOnRebootStr = GetValueFromManifest(setupSection, Constants.ParameterNames.SkipIsolatedNetworkResetOnReboot);
            this.SkipIsolatedNetworkResetOnReboot = (!string.IsNullOrEmpty(skipIsolatedNetworkResetOnRebootStr)) ? GetBoolFromManifest(setupSection, Constants.ParameterNames.SkipIsolatedNetworkResetOnReboot) : false;
#else
            // For linux, this is false by default unless customers override this in fabric settings. This is because NAT and MIP do not work together on linux today.
            this.ContainerNetworkSetup = (!string.IsNullOrEmpty(containerNetworkSetupStr)) ? GetBoolFromManifest(setupSection, Constants.ParameterNames.ContainerNetworkSetup) : false;
#endif

            // By default this is set to IsolatedNetworkConstants.NetworkName unless customers override this in fabric settings
            var isolatedNetworkNameStr = GetValueFromManifest(setupSection, Constants.ParameterNames.IsolatedNetworkName);
            this.IsolatedNetworkName = (!string.IsNullOrEmpty(isolatedNetworkNameStr)) ? (isolatedNetworkNameStr) : (IsolatedNetworkConstants.NetworkName);

            var isolatedNetworkSetupStr = GetValueFromManifest(setupSection, Constants.ParameterNames.IsolatedNetworkSetup);
            this.IsolatedNetworkSetup = (!string.IsNullOrEmpty(isolatedNetworkSetupStr)) ? GetBoolFromManifest(setupSection, Constants.ParameterNames.IsolatedNetworkSetup) : false;

            var isolatedNetworkInterfaceNameStr = GetValueFromManifest(setupSection, Constants.ParameterNames.IsolatedNetworkInterfaceName);
            this.IsolatedNetworkInterfaceName = (!string.IsNullOrEmpty(isolatedNetworkInterfaceNameStr)) ? (isolatedNetworkInterfaceNameStr) : (string.Empty);

            var diagnosticsSection =
                manifest.FabricSettings.FirstOrDefault(section => section.Name.Equals(Constants.SectionNames.Diagnostics, StringComparison.OrdinalIgnoreCase));
            this.EnableCircularTraceSession = GetBoolFromManifest(diagnosticsSection, Constants.ParameterNames.EnableCircularTraceSession);

            this.ContainerDnsSetup = GetValueFromManifest(
                setupSection, 
                ContainerDnsSetup.Allow); // default of Allow to maintain compatibility with 5.6 release which didn't have these options but would always try to setup 

            // Initialize the settings that track state of preview features that need to lightup at runtime
            var commonSection = manifest.FabricSettings.FirstOrDefault(section => section.Name.Equals(Constants.SectionNames.Common, StringComparison.OrdinalIgnoreCase));
            this.EnableUnsupportedPreviewFeatures = GetBoolFromManifest(commonSection, Constants.ParameterNames.EnableUnsupportedPreviewFeatures);     

            var hostingSection = manifest.FabricSettings.FirstOrDefault(section => section.Name.Equals(Constants.SectionNames.Hosting, StringComparison.OrdinalIgnoreCase));
            this.IsSFVolumeDiskServiceEnabled = GetBoolFromManifest(hostingSection, Constants.ParameterNames.IsSFVolumeDiskServiceEnabled);

            // Container service start up params
            var useContainerServiceArgumentsStr = GetValueFromManifest(hostingSection, Constants.ParameterNames.UseContainerServiceArguments);
            this.UseContainerServiceArguments = (!string.IsNullOrEmpty(useContainerServiceArgumentsStr)) 
                ? GetBoolFromManifest(hostingSection, Constants.ParameterNames.UseContainerServiceArguments) : true;

            var containerServiceArgumentsStr = GetValueFromManifest(hostingSection, Constants.ParameterNames.ContainerServiceArguments);
            this.ContainerServiceArguments = (!string.IsNullOrEmpty(containerServiceArgumentsStr)) 
                ? (containerServiceArgumentsStr) : (FlatNetworkConstants.ContainerServiceArguments);

            this.EnableContainerServiceDebugMode = GetBoolFromManifest(hostingSection, Constants.ParameterNames.EnableContainerServiceDebugMode);
        }

        private string GetValueFromManifest(SettingsOverridesTypeSection section, string parameterName)
        {
            var parameter = section == null ? null : section.Parameter.FirstOrDefault(par => par.Name.Equals(parameterName, StringComparison.OrdinalIgnoreCase));
            var value = parameter == null ? null : parameter.Value;
            if (parameter != null && parameter.IsEncrypted && value != null)
            {
                value = NativeConfigStore.DecryptText(value).ToString();
            }
            DeployerTrace.WriteInfo("Setup section, parameter {0}, has value {1}", parameterName, value);
            return value;
        }

        private bool GetBoolFromManifest(SettingsOverridesTypeSection section, string parameterName)
        {
            var value = GetValueFromManifest(section, parameterName);
            try
            {
                return Convert.ToBoolean(value);
            }
            catch (Exception)
            {
                return false;
            }      
        }

        private ContainerDnsSetup GetValueFromManifest(
            SettingsOverridesTypeSection section, 
            ContainerDnsSetup defaultValue)
        {
            var value = GetValueFromManifest(section, Constants.ParameterNames.ContainerDnsSetup);

            ContainerDnsSetup setupValue;

            if (value == null)
            {
                setupValue = defaultValue;
            }
            else
            {
                ContainerDnsSetup parsedValue;
                bool success = Enum.TryParse(value, true, out parsedValue);

                if (success)
                {
                    setupValue = parsedValue;
                }
                else
                {
                    setupValue = defaultValue;

                    DeployerTrace.WriteWarning(
                        "Setup section, parameter: {0}, value: {1} could not be interpreted. Using default value: {2}",
                        Constants.ParameterNames.ContainerDnsSetup, value, defaultValue);
                }
            }

            DeployerTrace.WriteInfo(
                "Setup section, parameter: {0}, value: {1}, interpreted value: {2}",
                Constants.ParameterNames.ContainerDnsSetup, value ?? "<null>", setupValue);
            return setupValue;
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System;
    using System.Globalization;
    using System.Fabric.Strings;

    /// <summary>
    /// This class handles the validation of DnsService configuration settings.
    /// </summary
    internal class DnsServiceConfigurationValidator : BaseFabricConfigurationValidator
    {
        // We cap the length at 5, so that we don't take away much from max DNS Name length allowed by RFC. 
        public const uint MaxLengthAllowedForPrefixAndSuffix = 5;

        // We don't allow "dot" as it intefers with DNS suffix behavior. 
        public const string DisallowedCharactersInPrefixAndSuffix = ".";

        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.DnsService; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var serviceSettings = windowsFabricSettings.GetSection(this.SectionName);

            bool isServiceEnabled = false;
            if ((serviceSettings != null) && serviceSettings.ContainsKey(FabricValidatorConstants.ParameterNames.IsEnabled))
            {
                isServiceEnabled = Boolean.Parse(serviceSettings[FabricValidatorConstants.ParameterNames.IsEnabled].Value);
            }

            FabricValidator.TraceSource.WriteInfo(
                FabricValidatorUtility.TraceTag, "DnsService feature enabled : {0}.", isServiceEnabled.ToString());

            if (isServiceEnabled)
            {
                string prefix = null;
                string suffix = null;

                if (serviceSettings.ContainsKey(FabricValidatorConstants.ParameterNames.DNSService.PartitionPrefix))
                {
                    prefix = serviceSettings[FabricValidatorConstants.ParameterNames.DNSService.PartitionPrefix].Value;
                    FabricValidator.TraceSource.WriteInfo(
                        FabricValidatorUtility.TraceTag,
                        string.Format(
                            CultureInfo.CurrentCulture,
                            "PartitionPrefix setting overriden in DnsService section, Overriden Value: {0}.",
                            prefix));
                }

                if (serviceSettings.ContainsKey(FabricValidatorConstants.ParameterNames.DNSService.PartitionSuffix))
                {
                    suffix = serviceSettings[FabricValidatorConstants.ParameterNames.DNSService.PartitionSuffix].Value;
                    FabricValidator.TraceSource.WriteInfo(
                        FabricValidatorUtility.TraceTag,
                        string.Format(
                            CultureInfo.CurrentCulture,
                            "PartitionSuffix setting overriden in DnsService section, Overriden Value: {0}.",
                            suffix));
                }

                // Validate overriden prefix and suffix settings
                ValidatePrefixAndSuffix(prefix, suffix);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {
            // Currently, we allow all DNS settings to be changed as part of the cluster configuration upgrade.
        }

        public static void ValidatePrefixAndSuffix(string prefix, string suffix)
        {
            if ((prefix == null) && (suffix == null))
            {
                // Nothing to validate. Default values will be used.
                return;
            }

            // If one setting has been overriden, other needs to be overriden also.
            if (prefix == null)
            {
                throw new ArgumentException(string.Format(
                    CultureInfo.CurrentCulture,
                    StringResources.Error_MissingOverrideValue_DnsServiceSettings,
                    FabricValidatorConstants.ParameterNames.DNSService.PartitionPrefix));
            }

            if (suffix == null)
            {
                throw new ArgumentException(string.Format(
                    CultureInfo.CurrentCulture,
                    StringResources.Error_MissingOverrideValue_DnsServiceSettings,
                    FabricValidatorConstants.ParameterNames.DNSService.PartitionSuffix));
            }

            // At this point we know, both partition query related settings have been overriden.
            ValidateOverridenValue(prefix, FabricValidatorConstants.ParameterNames.DNSService.PartitionPrefix);
            ValidateOverridenValue(suffix, FabricValidatorConstants.ParameterNames.DNSService.PartitionSuffix);

            // Construct a test DNS name to verify prefix/suffix strings.
            // Following approach has been taken to avoid having our custom logic
            // of checking for DNS compliance for each character in prefix/suffix strings.
            // For example string "--" is a valid prefix string but by itself is not a valid DNS hostname.
            // Following logic ensures complete DNS compliance in one go.
            string testDnsName = "test" + prefix + "targetpartition" + suffix;

            if (Uri.CheckHostName(testDnsName) != UriHostNameType.Dns)
            {
                throw new ArgumentException(string.Format(
                 CultureInfo.CurrentCulture,
                 StringResources.Error_InvalidChars_DnsServiceSettings,
                 testDnsName));
            }
        }

        public static void ValidateOverridenValue(string settingsValue, string settingsName)
        {
            // We don't allow overriding with empty value for prefix setting.
            if ((string.Compare(settingsName, FabricValidatorConstants.ParameterNames.DNSService.PartitionPrefix) == 0) &&
                (string.Compare(settingsValue, string.Empty) == 0))
            {
                throw new ArgumentException(string.Format(
                    CultureInfo.CurrentCulture,
                    StringResources.Error_Empty_DnsServiceSettings,
                    settingsName));
            }

            if (settingsValue.Length > MaxLengthAllowedForPrefixAndSuffix)
            {
                throw new ArgumentException(string.Format(
                    CultureInfo.CurrentCulture,
                    StringResources.Error_InvalidLength_DnsServiceSettings,
                    settingsName,
                    settingsValue.Length,
                    MaxLengthAllowedForPrefixAndSuffix.ToString()));
            }

            if (settingsValue.Contains(DisallowedCharactersInPrefixAndSuffix))
            {
                throw new ArgumentException(string.Format(
                    CultureInfo.CurrentCulture,
                    StringResources.Error_DisallowedChar_DnsServiceSettings,
                    settingsName,
                    DisallowedCharactersInPrefixAndSuffix));
            }
        }
    }
}
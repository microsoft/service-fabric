// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
using System.Fabric.Management.ServiceModel;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Fabric.Strings;
using System.ComponentModel;
using System.Fabric.Common;
using System.Security;

namespace System.Fabric.Management.WindowsFabricValidator
{
    internal class WindowsFabricSettings
    {
        private HashSet<string> dcaPluginSections = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        private DCASettingsValidator dcaSettingsValidator;

        public WindowsFabricSettings(ClusterManifestType manifest)
            : this(manifest, skipValidation:false)
        {
        }

        // We need an overload to initialize without validation in tests since the 
        // test can connect to clusters which might be in different versions than currently used 
        // management dll hence validations might fail. For example when connecting to linux
        // cluster the DCA settings might not be valid. This class is only used to easily
        // access deployed cluster settings and no other reason
        public WindowsFabricSettings(ClusterManifestType manifest, bool skipValidation)
        {
            this.StoreName = GetStoreName(manifest);
            var configurations = ReadConfigurations();
            var clusterManifest = ConvertSettingsToMap(manifest.FabricSettings);

            if (!skipValidation)
            {
                this.dcaSettingsValidator = new DCASettingsValidator(
                    configurations,
                    clusterManifest,
                    this.StoreName);

                // !!!!! IMPORTANT !!!!!
                //
                // The method call below performs custom validation of sections that
                // are specific to DCA plugins. It is also removes those sections from 
                // the settings map because the standard validation that we perform is
                // not applicable to those sections. Therefore, it is important to make
                // this method call before performing any other validation so that we 
                // can prevent our standard validation logic from being applied to 
                // DCA-plugin-specific sections.
                //
                // !!!!! IMPORTANT !!!!!
                if (!this.DcaSettingsValidator.ValidateDCAPluginSettings())
                {
                    throw new ArgumentException(StringResources.Error_FabricValidator_DCASettingsValidatorFailed);
                }
                this.dcaPluginSections = this.DcaSettingsValidator.DCAPluginSections;
            }

            this.MergeConfigurationsAndClusterManifest(configurations, clusterManifest, skipValidation);

            if (!skipValidation)
            {
                this.ValidateSettingsNamesAndTypes();
            }
        }

        public string StoreName { get; set; }

        public DCASettingsValidator DcaSettingsValidator
        {
            get
            {
#if DotNetCoreClrLinux
                ReleaseAssert.AssertIfNull(this.dcaSettingsValidator, "dcaSettingsValidator");
#else
                System.Fabric.Common.ReleaseAssert.AssertIfNull(this.dcaSettingsValidator, "dcaSettingsValidator");
#endif
                return this.dcaSettingsValidator;
            }
        }

        public Dictionary<string, Dictionary<string, SettingsValue>> SettingsMap
        {
            get { return this.settingsMap; }
        }

        public Dictionary<string, SettingsValue> GetSection(string sectionName)
        {
            if (!this.settingsMap.ContainsKey(sectionName)) return null;
            return this.SettingsMap[sectionName];
        }

        public SettingsValue GetParameter(string sectionName, string paramterName)
        {
            var section = GetSection(sectionName);
            if (section == null) return null;
            if (!section.ContainsKey(paramterName)) return null;
            return this.SettingsMap[sectionName][paramterName];
        }

        private void MergeConfigurationsAndClusterManifest(
            Dictionary<string, Dictionary<string, ConfigDescription>> configurations,
            Dictionary<string, Dictionary<string, SettingsOverridesTypeSectionParameter>> clusterManifest,
            bool skipValidation)
        {
            this.settingsMap = new Dictionary<string, Dictionary<string, SettingsValue>>(StringComparer.OrdinalIgnoreCase);
            var propertyGroupConfigDescriptions = new Dictionary<string, ConfigDescription>(StringComparer.OrdinalIgnoreCase);
            foreach (var section in configurations.Values)
            {
                foreach (var configDesc in section.Values)
                {
                    try
                    {
                        string sectionName = configDesc.SectionName;
                        if (!this.settingsMap.ContainsKey(sectionName))
                        {
                            this.settingsMap.Add(sectionName, new Dictionary<string, SettingsValue>(StringComparer.OrdinalIgnoreCase));
                        }

                        if (!String.Equals(configDesc.SettingName, PropertyGroupParameterName, StringComparison.OrdinalIgnoreCase))
                        {
                            this.settingsMap[sectionName].Add(configDesc.SettingName, new SettingsValue(configDesc.DefaultValue, configDesc.Type, configDesc.UpgradePolicy));
                        }
                        else
                        {
                            propertyGroupConfigDescriptions.Add(sectionName, configDesc);
                        }
                    }
                    catch (Exception e)
                    {
                        throw new ArgumentException(String.Format(StringResources.Warning_ThrowErrorWithParameterName1, configDesc.SectionName, configDesc.SettingName, e));
                    }
                }
            }

            foreach (var sectionName in clusterManifest.Keys)
            {
                if (!this.settingsMap.ContainsKey(sectionName))
                {
                    if (IsValidationDisabled(sectionName) || skipValidation)
                    {
                        this.settingsMap.Add(sectionName, new Dictionary<string, SettingsValue>());
                    }
                    else
                    {
                        throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_SectionNotFound, sectionName));
                    }
                }
                else if (SectionsNotAllowed.Contains(sectionName))
                {
                    throw new ArgumentException(
                        string.Format(
                        StringResources.Error_FabricValidator_InvalidSection,
                        sectionName));
                }

                foreach (var param in clusterManifest[sectionName].Values)
                {
                    try
                    {
                        if (IsValidationDisabled(sectionName) || skipValidation)
                        {
                            if (!this.settingsMap[sectionName].ContainsKey(param.Name))
                            {
                                this.settingsMap[sectionName].Add(param.Name, new SettingsValue(param.Value, StringType, UpgradePolicy.Static));
                            }
                        }
                        else
                        {
                            if (propertyGroupConfigDescriptions.ContainsKey(sectionName))
                            {
                                var configDesc = propertyGroupConfigDescriptions[sectionName];
                                this.settingsMap[sectionName].Add(param.Name, new SettingsValue(
                                    param.Value, ConvertSectionPropertyGroupType(configDesc), configDesc.UpgradePolicy));
                            }
                            else if (!this.settingsMap[sectionName].ContainsKey(param.Name))
                            {
                                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_ParamterNotFound, sectionName, param.Name));
                            }
                        }

                        this.settingsMap[sectionName][param.Name].Value = param.Value;
                        this.settingsMap[sectionName][param.Name].IsEncrypted = param.IsEncrypted;
                        this.settingsMap[sectionName][param.Name].IsFromClusterManifest = true;
                        if (configurations.ContainsKey(sectionName) && configurations[sectionName].ContainsKey(param.Name) && configurations[sectionName][param.Name].SettingType.Equals("DEPRECATED"))
                        {
                            FabricValidator.TraceSource.WriteWarning(
                                FabricValidatorUtility.TraceTag,
                                string.Format(
                                StringResources.Warning_DeprecatedConfigIsUsed,
                                sectionName, param.Name));
                        }
                    }
                    catch (Exception e)
                    {
                        throw new ArgumentException(String.Format(StringResources.Warning_ThrowErrorWithParameterName2, sectionName, param, e));
                    }
                }
            }
        }

        private void ValidateSettingsNamesAndTypes()
        {
            foreach (var section in this.settingsMap)
            {
                var sectionName = section.Key;
                if (IsValidationDisabled(sectionName))
                {
                    continue;
                }
                foreach (var entry in section.Value)
                {
                    try
                    {
                        var parameter = entry.Value;
                        ValidateConfigurationTypes(section.Key, entry.Key, parameter.Type, parameter.Value);
                    }
                    catch (Exception e)
                    {
                        throw new ArgumentException(String.Format(StringResources.Warning_ThrowErrorWithParameterName3, sectionName, entry, e));
                    }
                }
            }
        }

        public bool IsDCASetting(string sectionName)
        {
            return this.dcaPluginSections.Contains(sectionName);
        }

        private bool IsValidationDisabled(String sectionName)
        {
            return IsSectionValidationDisabled(sectionName)
                || this.IsDCASetting(sectionName);
        }

        public static bool IsSectionValidationDisabled(string sectionName)
        {
            return SectionsWithValidationDisabled.Contains(sectionName)
                || SectionPrefixesWithValidationDisabled.Any(prefix => sectionName.StartsWith(prefix, StringComparison.OrdinalIgnoreCase));
        }

        public static bool IsSectionAssumedToBeDynamic(string sectionName)
        {
            return sectionName.Equals(FabricValidatorConstants.SectionNames.InfrastructureService)
                || sectionName.StartsWith(FabricValidatorConstants.SectionNames.InfrastructureService + "/");
        }

        public static int EvaluateExpressionInt(ConfigDescription param)
        {
            if (param.DefaultValue.Contains('*'))
            {
                int res = 1;
                var splited = param.DefaultValue.Split('*');
                foreach (var token in splited)
                {
                    res *= int.Parse(token, CultureInfo.InvariantCulture);
                }
                return res;
            }

            return int.Parse(param.DefaultValue, CultureInfo.InvariantCulture);
        }

        public static uint EvaluateExpressionUint(ConfigDescription param)
        {
            if (param.DefaultValue.Contains('*'))
            {
                uint res = 1;
                var splited = param.DefaultValue.Split('*');
                foreach (var token in splited)
                {
                    if (HasHexPrefix(token))
                    {
                        res *= Convert.ToUInt32(token, 16);
                        continue;
                    }

                    res *= uint.Parse(token, CultureInfo.InvariantCulture);
                }
                return res;
            }

            if (HasHexPrefix(param.DefaultValue))
            {
                return Convert.ToUInt32(param.DefaultValue, 16);
            }

            return uint.Parse(param.DefaultValue, CultureInfo.InvariantCulture);
        }

        public static double EvaluateExpressionDouble(ConfigDescription param)
        {
            if (param.DefaultValue.Contains('*'))
            {
                double res = 1;
                var splited = param.DefaultValue.Split('*');
                foreach (var token in splited)
                {
                    res *= double.Parse(token, CultureInfo.InvariantCulture);
                }
                return res;
            }

            return double.Parse(param.DefaultValue, CultureInfo.InvariantCulture);
        }

        public static string EvaluateExpressionString(ConfigDescription param)
        {
            if (param.DefaultValue.StartsWith("Common::SecureString(L\"") && param.DefaultValue.EndsWith("\")"))
            {
                // Common::SecureString is a native class and default config values are declared using it.
                // Managed code that parses default config values must strip off the native class name.
                //
                return param.DefaultValue.Substring(23, param.DefaultValue.Length - 25);
            }
            else if (param.DefaultValue.StartsWith("L\"") && param.DefaultValue.EndsWith("\""))
            {
                if (param.DefaultValue.Length == 3)
                {
                    return "";
                }
                else
                {
                    return param.DefaultValue.Substring(2, param.DefaultValue.Length - 3);
                }
            }
            return param.DefaultValue;
        }

        public static TimeSpan EvaluateExpressionTimeSpan(ConfigDescription param)
        {
            // The default value can be in the following formats:
            // 1. TimeSpan::Zero or TimeSpan::MinValue or TimeSpan::MaxValue
            // 2. TimeSpan::FromSeconds(x) or FromMinutes(x) or FromTicks(x) or FromMilliseconds(x)

            // 'x' can either be a decimal/integer or an expression consisting of a product of multiple decimals/integers

            // Case 1 - TimeSpan::Zero
            if (
                String.Equals(param.DefaultValue, FabricValidatorConstants.DefaultValues.TimeSpanZero,
                    StringComparison.OrdinalIgnoreCase) ||
                String.Equals(param.DefaultValue, FabricValidatorConstants.DefaultValues.CommonTimeSpanZero,
                    StringComparison.OrdinalIgnoreCase))
            {
                return TimeSpan.Zero;
            }
            // Case 2 - TimeSpan::MinValue
            if (
                String.Equals(param.DefaultValue, FabricValidatorConstants.DefaultValues.TimeSpanMinValue,
                    StringComparison.OrdinalIgnoreCase) ||
                String.Equals(param.DefaultValue, FabricValidatorConstants.DefaultValues.CommonTimeSpanMinValue,
                    StringComparison.OrdinalIgnoreCase))
            {
                return TimeSpan.MinValue;
            }
            // Case 3 - TimeSpan::MaxValue
            if (
                String.Equals(param.DefaultValue, FabricValidatorConstants.DefaultValues.TimeSpanMaxValue,
                    StringComparison.OrdinalIgnoreCase) ||
                String.Equals(param.DefaultValue, FabricValidatorConstants.DefaultValues.CommonTimeSpanMaxValue,
                    StringComparison.OrdinalIgnoreCase))
            {
                return TimeSpan.MaxValue;
            }

            // Other cases involve parsing the double value from the format - TimeSpan::From*(x)
            var expression = param.DefaultValue.Substring(
                param.DefaultValue.IndexOf('(') + 1,
                param.DefaultValue.IndexOf(')') - param.DefaultValue.IndexOf('(') - 1);

            // If the timespan is in Ticks, the only possible value is long
            if (
                param.DefaultValue.StartsWith(FabricValidatorConstants.DefaultValues.CommonTimeSpanFromTicks,
                    StringComparison.OrdinalIgnoreCase) ||
                param.DefaultValue.StartsWith(FabricValidatorConstants.DefaultValues.TimeSpanFromTicks,
                    StringComparison.OrdinalIgnoreCase))
            {
                long result = 1;


                // We allow for the string to be a product of ints/decimals
                foreach (string token in expression.Split('*'))
                {
                    result *= long.Parse(token, CultureInfo.InvariantCulture);
                }

                // unit is ticks 
                return TimeSpan.FromTicks(result);
            }
            else
            {
                // If the timespan is in seconds,milliseconds or minutes, they can be represented as doubles
                double result = 1;

                // We allow for the string to be a product of ints/decimals
                foreach (string token in expression.Split('*'))
                {
                    result *= double.Parse(token, CultureInfo.InvariantCulture);
                }

                if (
                    param.DefaultValue.StartsWith(
                        FabricValidatorConstants.DefaultValues.CommonTimeSpanFromSeconds,
                        StringComparison.OrdinalIgnoreCase) ||
                    param.DefaultValue.StartsWith(FabricValidatorConstants.DefaultValues.TimeSpanFromSeconds,
                        StringComparison.OrdinalIgnoreCase))
                {
                    // unit is seconds
                    return TimeSpan.FromSeconds(result);
                }
                else if (
                    param.DefaultValue.StartsWith(
                        FabricValidatorConstants.DefaultValues.CommonTimeSpanFromMinutes,
                        StringComparison.OrdinalIgnoreCase) ||
                    param.DefaultValue.StartsWith(
                        FabricValidatorConstants.DefaultValues.TimeSpanFromMinutes,
                        StringComparison.OrdinalIgnoreCase))
                {
                    // unit is minutes
                    return TimeSpan.FromMinutes(result);
                }
                else if (
                    param.DefaultValue.StartsWith(
                        FabricValidatorConstants.DefaultValues.CommonTimeSpanFromHours,
                        StringComparison.OrdinalIgnoreCase) ||
                    param.DefaultValue.StartsWith(
                        FabricValidatorConstants.DefaultValues.TimeSpanFromHours,
                        StringComparison.OrdinalIgnoreCase))
                {
                    // unit is hours
                    return TimeSpan.FromHours(result);
                }
                else if (
                    param.DefaultValue.StartsWith(
                        FabricValidatorConstants.DefaultValues.CommonTimeSpanFromMilliseconds,
                        StringComparison.OrdinalIgnoreCase) ||
                    param.DefaultValue.StartsWith(
                        FabricValidatorConstants.DefaultValues.TimeSpanFromMilliseconds,
                        StringComparison.OrdinalIgnoreCase))
                {
                    // unit is milliseconds 
                    return TimeSpan.FromMilliseconds(result);
                }
                else
                {
                    throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_InvalidValue, param.SectionName, param.SettingName, param.Type, param.DefaultValue));
                }
            }
        }

        public static void ValidateConfigurationTypes(string section, string name, string type, string value)
        {
            switch (type)
            {
                case BoolType:
                    VerifyValue<bool>(section, name, type, value);
                    break;
                case CommonTimeSpanType:
                    break;
                case TimeSpanType:
                    VerifyTimeSpan(section, name, type, value);
                    break;
                case DoubleType:
                    VerifyValue<double>(section, name, type, value);
                    break;
                case IntType:
                    VerifyValue<int>(section, name, type, value);
                    break;
                case UIntType:
                    VerifyValue<uint>(section, name, type, value);
                    break;
                default:
                    break;
            }
        }

        public static bool IsPropertyGroupSection(string section)
        {
            return PropertyGroupSectionTypes.ContainsKey(section);
        }

        private static bool HasHexPrefix(string s)
        {
            return s.StartsWith("0x") || s.StartsWith("0X");
        }

        private static void ThrowOnException(string section, string name, string type, string value, Exception exception)
        {
            throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_InvalidValue, section, name, type, value, exception));
        }

        private static void VerifyValue<T>(string section, string name, string type, string value)
        {
            try
            {

                T result = (T)TypeDescriptor.GetConverter(typeof(T)).ConvertFromString(null, CultureInfo.InvariantCulture, value);
            }
            catch (Exception exception)
            {
                ThrowOnException(section, name, type, value, exception);
            }
        }

        private static void VerifyTimeSpan(string section, string name, string type, string value)
        {
            try
            {
                double dVal = Double.Parse(value, CultureInfo.InvariantCulture);
                if (dVal < 0.0)
                {
                    throw new InvalidDataException("value cannot be less than 0");
                }
            }
            catch (Exception exception)
            {
                ThrowOnException(section, name, type, value, exception);
            }
        }

        private static Dictionary<string, Dictionary<string, ConfigDescription>> ReadConfigurations()
        {
            Assembly currentAssembly = typeof(WindowsFabricSettings).GetTypeInfo().Assembly;
            TextReader reader = new StreamReader(currentAssembly.GetManifestResourceStream(FabricValidatorConstants.ConfigurationsFileName));
            var configurations = new Dictionary<string, Dictionary<string, ConfigDescription>>(StringComparer.OrdinalIgnoreCase);
            string line;
            while ((line = reader.ReadLine()) != null)
            {
                if (line.StartsWith("#", StringComparison.Ordinal))
                {
                    continue;
                }

                string[] configurationInfo = line.Split(',');
                ConfigDescription configDesc = new ConfigDescription()
                {
                    SectionName = configurationInfo[0].Trim(),
                    SettingName = configurationInfo[1].Trim(),
                    Type = configurationInfo[2].Trim(),
                    DefaultValue = configurationInfo[3].Trim(),
                    UpgradePolicy = (UpgradePolicy)Enum.Parse(typeof(UpgradePolicy), configurationInfo[4].Trim()),
                    SettingType = configurationInfo[5].Trim()
                };

                try
                {
                    if (String.Equals(configDesc.Type, TimeSpanType, StringComparison.OrdinalIgnoreCase) ||
                        String.Equals(configDesc.Type, CommonTimeSpanType, StringComparison.OrdinalIgnoreCase))
                    {
                        configDesc.DefaultValue = EvaluateExpressionTimeSpan(configDesc).TotalSeconds.ToString(CultureInfo.InvariantCulture);
                    }
                    else if (String.Equals(configDesc.Type, IntType, StringComparison.OrdinalIgnoreCase))
                    {
                        configDesc.DefaultValue = EvaluateExpressionInt(configDesc).ToString(CultureInfo.InvariantCulture);
                    }
                    else if (String.Equals(configDesc.Type, UIntType, StringComparison.OrdinalIgnoreCase))
                    {
                        configDesc.DefaultValue = EvaluateExpressionUint(configDesc).ToString(CultureInfo.InvariantCulture);
                    }
                    else if (String.Equals(configDesc.Type, DoubleType, StringComparison.OrdinalIgnoreCase))
                    {
                        configDesc.DefaultValue = EvaluateExpressionDouble(configDesc).ToString(CultureInfo.InvariantCulture);
                    }
                    else if (String.Equals(configDesc.Type, BoolType, StringComparison.OrdinalIgnoreCase))
                    {
                        // no-op
                    }
                    else if (String.Equals(configDesc.Type, StringType, StringComparison.OrdinalIgnoreCase) || String.Equals(configDesc.Type, SecureStringType, StringComparison.OrdinalIgnoreCase))
                    {
                        configDesc.DefaultValue = EvaluateExpressionString(configDesc);
                    }
                }
                catch (Exception e)
                {
                    throw new ArgumentException(String.Format(StringResources.Warning_ThrowErrorWithParameterName2, configDesc.SectionName, configDesc.SettingName, e));
                }

                if (!configurations.ContainsKey(configDesc.SectionName))
                {
                    configurations.Add(configDesc.SectionName, new Dictionary<string, ConfigDescription>(StringComparer.OrdinalIgnoreCase));
                }

                if (configurations[configDesc.SectionName].ContainsKey(configDesc.SettingName))
                {
                    throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_DuplicateParameterConfiguration,
                        configDesc.SettingName,
                        configDesc.SectionName));
                }

                configurations[configDesc.SectionName].Add(
                    configDesc.SettingName,
                    configDesc);
            }

            return configurations;
        }

        private static Dictionary<string, Dictionary<string, SettingsOverridesTypeSectionParameter>> ConvertSettingsToMap(SettingsOverridesTypeSection[] settingsArray)
        {
            var settingsMap = new Dictionary<string, Dictionary<string, SettingsOverridesTypeSectionParameter>>(StringComparer.OrdinalIgnoreCase);

            foreach (SettingsOverridesTypeSection section in settingsArray)
            {
                if (section == null)
                {
                    continue;
                }
                try
                {
                    settingsMap.Add(section.Name, new Dictionary<string, SettingsOverridesTypeSectionParameter>(StringComparer.OrdinalIgnoreCase));
                }
                catch (ArgumentException)
                {
                    throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_DuplicateSectionManifest, section.Name));
                }
                if (section.Parameter == null)
                {
                    continue;
                }

                foreach (var param in section.Parameter)
                {
                    if (param == null)
                    {
                        continue;
                    }
                    try
                    {
                        settingsMap[section.Name].Add(param.Name, param);
                    }
                    catch (ArgumentException)
                    {
                        throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_DuplicateParameterManifest, param.Name, section.Name));
                    }
                }
            }

            return settingsMap;
        }

        private static string GetStoreName(ClusterManifestType manifest)
        {
            if (manifest.Certificates != null && manifest.Certificates.SecretsCertificate != null)
            {
                return manifest.Certificates.SecretsCertificate.X509StoreName;
            }
            else
            {
                return DefaultX509StoreName;
            }
        }

        private Dictionary<string, Dictionary<string, SettingsValue>> settingsMap;

        public class SettingsValue
        {
            public string Value { get; internal set; }
            public string Type { get; internal set; }
            public bool IsFromClusterManifest { get; internal set; }
            public bool IsEncrypted { get; internal set; }
            public UpgradePolicy UpgradePolicy { get; internal set; }

            public SettingsValue(string value, string type, UpgradePolicy upgradePolicy)
            {
                this.Value = value;
                this.Type = type;
                this.UpgradePolicy = upgradePolicy;
                this.IsFromClusterManifest = false;
                this.IsEncrypted = false;
            }

            public SecureString GetSecureValue(string storeName)
            {
                if (this.IsEncrypted)
                {
                    return FabricValidatorUtility.DecryptValue(storeName, this.Value);
                }
                else
                {
                    if (string.IsNullOrEmpty(this.Value))
                    {
                        return null;
                    }

                    return FabricValidatorUtility.CharArrayToSecureString(this.Value.ToCharArray());
                }
            }

            public T GetValue<T>()
            {
                return (T)TypeDescriptor.GetConverter(typeof(T)).ConvertFromString(null, CultureInfo.InvariantCulture, this.Value);
            }

            public TimeSpan GetTimeSpanValue()
            {
                return TimeSpan.FromSeconds(this.GetValue<double>());
            }
        }

        private const string PropertyGroupParameterName = "PropertyGroup";
        private const string StringType = "wstring";
        private const string SecureStringType = "SecureString";

        // Types to be used for type checking.
        private const string IntType = "int";
        private const string UIntType = "uint";
        private const string TimeSpanType = "TimeSpan";
        private const string CommonTimeSpanType = "Common::TimeSpan";
        private const string BoolType = "bool";
        private const string DoubleType = "double";

        string ConvertSectionPropertyGroupType(ConfigDescription configDescription)
        {
            string sectionName = configDescription.SectionName;
            if(!PropertyGroupSectionTypes.ContainsKey(sectionName))
            {
                throw new ArgumentException(String.Format(
                    StringResources.Error_FabricValidator_InvalidSectionPropertyGroupType, configDescription.Type, sectionName));
            }

            return PropertyGroupSectionTypes[sectionName];
        }

        private static readonly HashSet<string> SectionsWithValidationDisabled =
            new HashSet<string>(StringComparer.OrdinalIgnoreCase)
            {
                FabricValidatorConstants.SectionNames.UnreliableTransport,
                FabricValidatorConstants.SectionNames.Votes,
                FabricValidatorConstants.SectionNames.FabricTestQueryWeights,
                FabricValidatorConstants.SectionNames.RunAs,
                FabricValidatorConstants.SectionNames.RunAs_Fabric,
                FabricValidatorConstants.SectionNames.RunAs_DCA,
                FabricValidatorConstants.SectionNames.RunAs_HttpGateway,
                FabricValidatorConstants.SectionNames.InfrastructureService,
                FabricValidatorConstants.SectionNames.DSTSTokenValidationService,
                FabricValidatorConstants.SectionNames.TokenValidationService,
                FabricValidatorConstants.SectionNames.NodeCapacities,
                FabricValidatorConstants.SectionNames.NodeProperties,
                FabricValidatorConstants.SectionNames.ClusterX509Names,
                FabricValidatorConstants.SectionNames.ServerX509Names,
                FabricValidatorConstants.SectionNames.ClientX509Names,
                FabricValidatorConstants.SectionNames.AdminClientX509Names,
                FabricValidatorConstants.SectionNames.ClusterCertificateIssuerStores,
                FabricValidatorConstants.SectionNames.ServerCertificateIssuerStores,
                FabricValidatorConstants.SectionNames.ClientCertificateIssuerStores,
                FabricValidatorConstants.SectionNames.HealthManager_ClusterHealthPolicy,
                FabricValidatorConstants.SectionNames.ServiceCommonNameAndIssuer,
            };

        // Add other property group types here if you want to validate type of section parameters against Configuration.csv file
        private static Dictionary<string, string> PropertyGroupSectionTypes =
            new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                {
                    FabricValidatorConstants.SectionNames.MetricBalancingThresholds,
                    DoubleType
                },
                {
                    FabricValidatorConstants.SectionNames.MetricActivityThresholds,
                    UIntType
                },
                {
                    FabricValidatorConstants.SectionNames.GlobalMetricWeights,
                    DoubleType
                },
                {
                    FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNode,
                    IntType
                },
                {
                    FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodeBalancingThrottle,
                    IntType
                },
                {
                    FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodeConstraintCheckThrottle,
                    IntType
                },
                {
                    FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodePlacementThrottle,
                    IntType
                },
                {
                    FabricValidatorConstants.SectionNames.DefragmentationMetrics,
                    BoolType
                },
                {
                    FabricValidatorConstants.SectionNames.DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
                    DoubleType
                },
                {
                    FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeDistributionPolicy,
                    UIntType
                },
                {
                    FabricValidatorConstants.SectionNames.PlacementHeuristicIncomingLoadFactor,
                    DoubleType
                },
                {
                    FabricValidatorConstants.SectionNames.PlacementHeuristicEmptySpacePercent,
                    DoubleType
                },
                {
                    FabricValidatorConstants.SectionNames.NodeBufferPercentage,
                    DoubleType
                },
                {
                    FabricValidatorConstants.SectionNames.DefragmentationScopedAlgorithmEnabled,
                    BoolType
                },
                {
                    FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeWeight,
                    DoubleType
                },
                {
                    FabricValidatorConstants.SectionNames.MetricEmptyNodeThresholds,
                    UIntType
                },
                {
                    FabricValidatorConstants.SectionNames.ReservedLoadPerNode,
                    UIntType
                },
                {
                    FabricValidatorConstants.SectionNames.BalancingByPercentage,
                    BoolType
                },
                {
                    FabricValidatorConstants.SectionNames.PlacementStrategy,
                    UIntType
                },
                {
                    FabricValidatorConstants.SectionNames.DefragmentationNonEmptyNodeWeight,
                    DoubleType
                }
            };

        private static readonly IEnumerable<string> SectionPrefixesWithValidationDisabled =
            new[]
            {
                FabricValidatorConstants.SectionNames.InfrastructureService + "/",
                FabricValidatorConstants.SectionNames.DSTSTokenValidationService + "/",
                FabricValidatorConstants.SectionNames.TokenValidationService + "/",
            };

        private static readonly HashSet<string> SectionsNotAllowed =
            new HashSet<string>(StringComparer.OrdinalIgnoreCase)
            {
                FabricValidatorConstants.SectionNames.FabricNode,
                FabricValidatorConstants.SectionNames.NodeDomainIds,
                FabricValidatorConstants.SectionNames.NodeProperties,
                FabricValidatorConstants.SectionNames.NodeCapacities
            };

#if DotNetCoreClrLinux
        private static readonly string DefaultX509StoreName = "";
#else
        private static readonly string DefaultX509StoreName = "My";
#endif
    }
}
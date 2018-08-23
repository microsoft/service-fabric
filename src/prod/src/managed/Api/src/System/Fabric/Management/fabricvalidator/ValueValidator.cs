// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.Tracing;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Management.ImageBuilder;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.Fabric.Strings;

    /// <summary>
    /// 
    /// </summary>
    static class ValueValidator
    {
        public static void VerifyIntValueGreaterThanInput(Dictionary<string, WindowsFabricSettings.SettingsValue> settingsForThisSection, string parameterName, int inputValue, string sectionName)
        {
            if (settingsForThisSection.ContainsKey(parameterName))
            {
                int value = int.Parse(settingsForThisSection[parameterName].Value, CultureInfo.InvariantCulture);
                if (value <= inputValue)
                {
                    ThrowOnFailure(
                        sectionName,
                        parameterName,
#if DotNetCoreClr
                        value.ToString(),
#else
                        value.ToString(CultureInfo.InvariantCulture),
#endif
                        string.Format(CultureInfo.InvariantCulture, StringResources.Error_FabricValidator_IntValueGreaterThanInput, inputValue));
                }
            }
        }

        public static void VerifyIntValueGreaterThanEqualToInput(Dictionary<string, WindowsFabricSettings.SettingsValue> settingsForThisSection, string parameterName, int inputValue, string sectionName)
        {
            if (settingsForThisSection.ContainsKey(parameterName))
            {
                int value = settingsForThisSection[parameterName].GetValue<int>();
                if (value < inputValue)
                {
                    ThrowOnFailure(
                        sectionName,
                        parameterName,
#if DotNetCoreClr
                        value.ToString(),
#else
                        value.ToString(CultureInfo.InvariantCulture),
#endif
                        string.Format(CultureInfo.InvariantCulture, StringResources.Error_FabricValidator_IntValueGreaterThanEqualToInput, inputValue));
                }
            }
        }

        public static void VerifyIntValueLessThanEqualToInput(Dictionary<string, WindowsFabricSettings.SettingsValue> settingsForThisSection, string parameterName, int inputValue, string sectionName)
        {
            if (settingsForThisSection.ContainsKey(parameterName))
            {
                int value = settingsForThisSection[parameterName].GetValue<int>();
                if (value > inputValue)
                {
                    ThrowOnFailure(
                        sectionName,
                        parameterName,
#if DotNetCoreClr
                        value.ToString(),
#else
                        value.ToString(CultureInfo.InvariantCulture),
#endif
                        string.Format(CultureInfo.InvariantCulture, StringResources.Error_FabricValidator_IntValueLessThanEqualToInput, inputValue));
                }
            }
        }

        public static void VerifyIntValueInRangeExcludingRight(Dictionary<string, WindowsFabricSettings.SettingsValue> settingsForThisSection, string parameterName, int lowRange, int highRange, string sectionName)
        {
            if (settingsForThisSection.ContainsKey(parameterName))
            {
                int value = settingsForThisSection[parameterName].GetValue<int>();
                if (value < lowRange || value >= highRange)
                {
                    ThrowOnFailure(
                        sectionName,
                        parameterName,
#if DotNetCoreClr
                        value.ToString(),
#else
                        value.ToString(CultureInfo.InvariantCulture),
#endif
                        string.Format(CultureInfo.InvariantCulture, StringResources.Error_FabricValidator_NamedIntValueInRangeExcludingRight, lowRange, highRange));
                }
            }
        }

        public static void VerifyDoubleValueGreaterThanEqualToInput(Dictionary<string, WindowsFabricSettings.SettingsValue> settingsForThisSection, string parameterName, double inputValue, string sectionName)
        {
            if (settingsForThisSection.ContainsKey(parameterName))
            {
                double value = settingsForThisSection[parameterName].GetValue<double>();
                if (value < inputValue)
                {
                    ThrowOnFailure(
                        sectionName,
                        parameterName,
#if DotNetCoreClr
                        value.ToString(),
#else
                        value.ToString(CultureInfo.InvariantCulture),
#endif
                        string.Format(CultureInfo.InvariantCulture, StringResources.Error_FabricValidator_DoubleValueGreaterThanEqualToInput, inputValue));
                }
            }
        }

        public static void VerifyDoubleValueInRange(Dictionary<string, WindowsFabricSettings.SettingsValue> settingsForThisSection, string parameterName, double lowRange, double highRange, string sectionName)
        {
            if (settingsForThisSection.ContainsKey(parameterName))
            {
                double value = settingsForThisSection[parameterName].GetValue<double>();
                if (value < lowRange || value > highRange)
                {
                    ThrowOnFailure(
                        sectionName,
                        parameterName,
#if DotNetCoreClr
                        value.ToString(),
#else
                        value.ToString(CultureInfo.InvariantCulture),
#endif
                        string.Format(CultureInfo.InvariantCulture, StringResources.Error_FabricValidator_DoubleValueInRange, lowRange, highRange));
                }
            }
        }

        public static void VerifyFirstIntParameterGreaterThanOrEqualToSecondIntParameter(Dictionary<string, WindowsFabricSettings.SettingsValue> settingsForThisSection, string firstParameterName, string secondParameterName, string sectionName)
        {
            int value1 = settingsForThisSection[firstParameterName].GetValue<int>();
            int value2 = settingsForThisSection[secondParameterName].GetValue<int>();
            if (value1 < value2)
            {
                ThrowOnFailure(
                    sectionName,
                    firstParameterName,
#if DotNetCoreClr
                    value1.ToString(),
#else
                    value1.ToString(CultureInfo.InvariantCulture),
#endif
                    string.Format(CultureInfo.InvariantCulture, StringResources.Error_FabricValidator_FirstIntParameterGreaterThanOrEqualToSecondIntParameter, value1, value2));
            }
        }

        public static void ThrowOnFailure(string sectionName, string parameterName, string value, string error)
        {
            throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_ValueValidatorFailed, sectionName, parameterName, value, error));
        }

        // TODO: remove when making DCA validator change
        public static void VerifyIntValueGreaterThanInput(Dictionary<string, SettingsOverridesTypeSectionParameter> settingsForThisSection, string parameterName, int inputValue, string sectionName, EventLevel level)
        {
            if (settingsForThisSection.ContainsKey(parameterName))
            {
                int value = int.Parse(settingsForThisSection[parameterName].Value, CultureInfo.InvariantCulture);
                if (value <= inputValue)
                {
                    TraceOnFailure(
                        level,
                        sectionName,
                        parameterName,
#if DotNetCoreClr
                        value.ToString(),
#else
                        value.ToString(CultureInfo.InvariantCulture),
#endif
                        string.Format(CultureInfo.InvariantCulture, StringResources.Error_ValueShouldBeGreaterThanInputValue, inputValue));
                    throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, StringResources.Error_ValueShouldBeGreaterThanInputValue, inputValue));
                }
            }
        }

        // TODO: remove when making DCA validator change
        public static void VerifyIntValueGreaterThanEqualToInput(Dictionary<string, SettingsOverridesTypeSectionParameter> settingsForThisSection, string parameterName, int inputValue, string sectionName, EventLevel level)
        {
            if (settingsForThisSection.ContainsKey(parameterName))
            {
                int value = int.Parse(settingsForThisSection[parameterName].Value, CultureInfo.InvariantCulture);
                if (value < inputValue)
                {
                    TraceOnFailure(
                        level,
                        sectionName,
                        parameterName,
#if DotNetCoreClr
                        value.ToString(),
#else
                        value.ToString(CultureInfo.InvariantCulture),
#endif
                        string.Format(CultureInfo.InvariantCulture, StringResources.Error_ValueGreaterThanOrEqualToZero, inputValue));
                    throw new ArgumentException(string.Format(StringResources.Error_ValueGreaterThanOrEqualToZero, inputValue));
                }
            }
        }

        // TODO: remove when making DCA validator change
        public static void VerifyIntValueLessThanEqualToInput(Dictionary<string, SettingsOverridesTypeSectionParameter> settingsForThisSection, string parameterName, int inputValue, string sectionName, EventLevel level)
        {
            if (settingsForThisSection.ContainsKey(parameterName))
            {
                int value = int.Parse(settingsForThisSection[parameterName].Value, CultureInfo.InvariantCulture);
                if (value > inputValue)
                {
                    TraceOnFailure(
                        level,
                        sectionName,
                        parameterName,
#if DotNetCoreClr
                        value.ToString(),
#else
                        value.ToString(CultureInfo.InvariantCulture),
#endif
                        string.Format(CultureInfo.InvariantCulture, StringResources.Error_ValueShouldBeLessThanOrEqualToZero, inputValue));
                    throw new ArgumentException(string.Format(StringResources.Error_ValueShouldBeLessThanOrEqualToZero, inputValue));
                }
            }
        }


        // TODO: remove when making DCA validator change
        public static void TraceOnFailure(EventLevel level, string sectionName, string parameterName, string value, string error)
        {
            if (level == EventLevel.Warning)
            {
                FabricValidator.TraceSource.WriteWarning(
                    FabricValidatorUtility.TraceTag,
                    FabricValidatorUtility.TraceTag,
                    "Section '{0}', parameter '{1}' has value {2}. {3}",
                    sectionName,
                    parameterName,
                    value,
                    error);
            }
            else if (level == EventLevel.Error || level == EventLevel.Critical)
            {
                FabricValidator.TraceSource.WriteError(
                    FabricValidatorUtility.TraceTag,
                    "Section '{0}', parameter '{1}' has value {2}. {3}",
                    sectionName,
                    parameterName,
                    value,
                    error);
            }
        }

    }
}
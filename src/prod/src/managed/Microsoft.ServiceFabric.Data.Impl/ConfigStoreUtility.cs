// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Fabric.Common;
    using System.Globalization;

    /// <summary>
    /// Utility class to load settings
    /// </summary>
    internal class ConfigStoreUtility
    {
        // Config store
        private static IConfigStore configStore;

        internal static T GetUnencryptedConfigValue<T>(string sectionName, string valueName, T defaultValue)
        {
            var newValue = configStore.ReadUnencryptedString(sectionName, valueName);
            if (string.IsNullOrEmpty(newValue))
            {
                return defaultValue;
            }
            return (T) Convert.ChangeType(newValue, typeof(T), CultureInfo.InvariantCulture);
        }

        internal static uint? GetUnencryptedUIntConfigValue(string sectionName, string valueName, uint? defaultValue)
        {
            var newValue = configStore.ReadUnencryptedString(sectionName, valueName);
            return string.IsNullOrEmpty(newValue) ? defaultValue : (uint.Parse(newValue));
        }

        internal static int? GetUnencryptedIntConfigValue(string sectionName, string valueName, int? defaultValue)
        {
            var newValue = configStore.ReadUnencryptedString(sectionName, valueName);
            return string.IsNullOrEmpty(newValue) ? defaultValue : (int.Parse(newValue));
        }

        internal static TimeSpan? GetUnencryptedTimespanConfigValue(string sectionName, string valueName, TimeSpan? defaultValue)
        {
            var newValue = configStore.ReadUnencryptedString(sectionName, valueName);
            return string.IsNullOrEmpty(newValue) ? defaultValue : (TimeSpan.FromSeconds(double.Parse(newValue)));
        }

        internal static long? GetUnencryptedLongConfigValue(string sectionName, string valueName, long? defaultValue)
        {
            var newValue = configStore.ReadUnencryptedString(sectionName, valueName);
            return string.IsNullOrEmpty(newValue) ? defaultValue : (long.Parse(newValue));
        }

        internal static bool? GetUnencryptedBoolConfigValue(string sectionName, string valueName, bool? defaultValue)
        {
            var newValue = configStore.ReadUnencryptedString(sectionName, valueName);
            return string.IsNullOrEmpty(newValue) ? defaultValue : (bool.Parse(newValue));
        }

        internal static string GetUnencryptedStringConfigValue(string sectionName, string valueName, string defaultValue)
        {
            var newValue = configStore.ReadUnencryptedString(sectionName, valueName);
            return string.IsNullOrEmpty(newValue) ? defaultValue : newValue;
        }

        internal static double? GetUnencryptedDoubleConfigValue(string sectionName, string valueName, double defaultValue)
        {
            var newValue = configStore.ReadUnencryptedString(sectionName, valueName);
            double parsedDouble;
            return double.TryParse(newValue, out parsedDouble) ? parsedDouble : defaultValue;
        }

        internal static void InitializeConfigStore()
        {
            configStore = NativeConfigStore.FabricGetConfigStore();
            return;
        }
    }
}
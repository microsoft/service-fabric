// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Fabric.Data.Common;
    using System.Fabric.Store;

    internal static class StoreSettingsUtil
    {
        public static void LoadFromClusterManifest(string traceType, ref StoreSettings storeSettings)
        {
            Diagnostics.Assert(storeSettings != null, traceType, "storeSettings");
            ConfigStoreUtility.InitializeConfigStore();
            var sectionName = StoreSettingsConfigurationNames.SectionName;

            SetSweepThreshold(traceType, ref storeSettings, sectionName);
            SetEnableStrict2PL(traceType, ref storeSettings, sectionName);

            ValidateSettings(traceType, storeSettings);
        }

        public static void ValidateSettings(string traceType, StoreSettings storeSettings)
        {
            Diagnostics.Assert(storeSettings != null, traceType, "storeSettings");

            ValidateSweepThreshold(storeSettings);
            ValidateEnableString2PL(storeSettings);
        }

        private static void SetSweepThreshold(string traceType, ref StoreSettings storeSettings, string sectionName)
        {
            Diagnostics.Assert(string.IsNullOrEmpty(sectionName) ==false, traceType, "section name cannot be null or empty");

            if (!storeSettings.SweepThreshold.HasValue)
            {
                storeSettings.SweepThreshold = ConfigStoreUtility.GetUnencryptedIntConfigValue(
                    sectionName,
                    StoreSettingsConfigurationNames.SweepThreshold.Item1,
                    StoreSettingsConfigurationNames.SweepThreshold.Item2);
            }

            Diagnostics.Assert(storeSettings.SweepThreshold.HasValue, traceType, "SweepThreshold must have a value");
        }

        private static void SetEnableStrict2PL(string traceType, ref StoreSettings storeSettings, string sectionName)
        {
            Diagnostics.Assert(string.IsNullOrEmpty(sectionName) == false, traceType, "section name cannot be null or empty");

            if (!storeSettings.EnableStrict2PL.HasValue)
            {
                storeSettings.EnableStrict2PL = ConfigStoreUtility.GetUnencryptedBoolConfigValue(
                    sectionName,
                    StoreSettingsConfigurationNames.EnableStrict2PL.Item1,
                    StoreSettingsConfigurationNames.EnableStrict2PL.Item2);
            }

            Diagnostics.Assert(storeSettings.EnableStrict2PL.HasValue, traceType, "EnableStrict2PL must have a value");
        }

        private static void ValidateSweepThreshold(StoreSettings storeSettings)
        {
            if (storeSettings.SweepThreshold.HasValue == false)
            {
                throw new ArgumentNullException(StoreSettingsConfigurationNames.SweepThreshold.Item1);
            }

            if (storeSettings.SweepThreshold < TStoreConstants.SweepThresholdDisabledFlag)
            {
                throw new ArgumentOutOfRangeException(StoreSettingsConfigurationNames.SweepThreshold.Item1, "less than zero");
            }

            if (storeSettings.SweepThreshold > TStoreConstants.MaxSweepThreshold)
            {
                throw new ArgumentOutOfRangeException(StoreSettingsConfigurationNames.SweepThreshold.Item1, "is greater than max");
            }
        }

        private static void ValidateEnableString2PL(StoreSettings storeSettings)
        {
            if (storeSettings.EnableStrict2PL.HasValue == false)
            {
                throw new ArgumentNullException(StoreSettingsConfigurationNames.SweepThreshold.Item1);
            }
        }
    }
}
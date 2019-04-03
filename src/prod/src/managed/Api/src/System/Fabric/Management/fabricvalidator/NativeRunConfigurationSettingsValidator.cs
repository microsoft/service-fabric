// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System;
    using System.Text;

    internal class NativeRunConfigurationSettingValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.NativeRunConfiguration; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
           var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);

           if (settingsForThisSection != null && settingsForThisSection.ContainsKey(FabricValidatorConstants.ParameterNames.NativeRunConfiguration.EnableNativeReliableStateManager))
           {
                settingsForThisSection[FabricValidatorConstants.ParameterNames.NativeRunConfiguration.EnableNativeReliableStateManager].GetValue<bool>();
           }

           if (settingsForThisSection != null && settingsForThisSection.ContainsKey(FabricValidatorConstants.ParameterNames.NativeRunConfiguration.Test_LoadEnableNativeReliableStateManager))
           {
                settingsForThisSection[FabricValidatorConstants.ParameterNames.NativeRunConfiguration.Test_LoadEnableNativeReliableStateManager].GetValue<bool>();
           }
        }

        public override void ValidateConfigurationUpgrade(
            WindowsFabricSettings currentWindowsFabricSettings,
            WindowsFabricSettings targetWindowsFabricSettings)
        {
            string configurationParametername = FabricValidatorConstants.ParameterNames.NativeRunConfiguration.EnableNativeReliableStateManager;
            string loadEnableNativeReliableStateManager = FabricValidatorConstants.ParameterNames.NativeRunConfiguration.Test_LoadEnableNativeReliableStateManager;

            bool currentSettingsValue = currentWindowsFabricSettings.GetParameter(this.SectionName, configurationParametername).GetValue<bool>();
            bool targetSettingsValue = targetWindowsFabricSettings.GetParameter(this.SectionName, configurationParametername).GetValue<bool>();

            bool loadEnableNativeReliableStateManagerValue = targetWindowsFabricSettings.GetParameter(this.SectionName, loadEnableNativeReliableStateManager).GetValue<bool>();

            int targetSerializationVersionValue = targetWindowsFabricSettings.GetParameter(
                FabricValidatorConstants.SectionNames.TransactionalReplicator2.Default,
                FabricValidatorConstants.ParameterNames.TransactionalReplicator2.SerializationVersion).GetValue<int>();

            // upgrading from managed to native stack
            if (currentSettingsValue == false && targetSettingsValue == true) 
            {
                // Fail the upgrade if loadEnableNativeReliableStateManagerValue is false
                if (!loadEnableNativeReliableStateManagerValue)
                {
                    throw new ArgumentException(
                       string.Format(
                       "EnableNativeReliableStateManager = {0}. Upgrade is not supported.",
                       targetSettingsValue));
                }

                // Validate the SerializationVersion constraint
                // if current SerializationVersion is 0 then managed to native reliable statemanager is only allowed when target SerializationVersion is also 0
                if (targetSerializationVersionValue != 0)
                {
                    throw new ArgumentException(
                       string.Format(
                       "SerializationVersion = {0}. New SerializationVersion must be 0 when {1} is true.",
                       targetSerializationVersionValue,
                       configurationParametername));
                }

                // Validate the common settings values during upgrade. Throw exception in case the new and old values don't match
                this.ValidateSettingValue<int>(currentWindowsFabricSettings, targetWindowsFabricSettings, FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB);
                this.ValidateSettingValue<int>(currentWindowsFabricSettings, targetWindowsFabricSettings, FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxRecordSizeInKB);
                this.ValidateSettingValue<int>(currentWindowsFabricSettings, targetWindowsFabricSettings, FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.CheckpointThresholdInMB);
                this.ValidateSettingValue<int>(currentWindowsFabricSettings, targetWindowsFabricSettings, FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxAccumulatedBackupLogSizeInMB);
                this.ValidateSettingValue<int>(currentWindowsFabricSettings, targetWindowsFabricSettings, FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB);
                this.ValidateSettingValue<int>(currentWindowsFabricSettings, targetWindowsFabricSettings, FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.TruncationThresholdFactor);
                this.ValidateSettingValue<bool>(currentWindowsFabricSettings, targetWindowsFabricSettings, FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.OptimizeForLocalSSD);
                this.ValidateSettingValue<bool>(currentWindowsFabricSettings, targetWindowsFabricSettings, FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.OptimizeLogForLowerDiskUsage);
                this.ValidateSettingValue<TimeSpan>(currentWindowsFabricSettings, targetWindowsFabricSettings, FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.SlowApiMonitoringDuration);
                this.ValidateSettingValue<bool>(currentWindowsFabricSettings, targetWindowsFabricSettings, FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.EnableIncrementalBackupsAcrossReplicas);

                // TODO : RDBug 11792270
                // this.ValidateSettingValue<int>(currentWindowsFabricSettings, targetWindowsFabricSettings, FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxMetadataSizeInKB);
                // this.ValidateSettingValue<int>(currentWindowsFabricSettings, targetWindowsFabricSettings, FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxWriteQueueDepthInKB);
                // this.ValidateSettingValue<int>(currentWindowsFabricSettings, targetWindowsFabricSettings, FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.ThrottlingThresholdFactor);
            }
        }

        private void ValidateSettingValue<T>(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings, string settingName)
        {
            T oldValue = currentWindowsFabricSettings.GetParameter(
                FabricValidatorConstants.SectionNames.TransactionalReplicator,
                settingName).GetValue<T>();

            T newValue = targetWindowsFabricSettings.GetParameter(
                FabricValidatorConstants.SectionNames.TransactionalReplicator2.Default,
                settingName).GetValue<T>();

            if (!newValue.Equals(oldValue))
            {
                throw new ArgumentException(
                   string.Format(
                   "Setting - {0} - new value {1} should match old value {2}.",
                   settingName,
                   newValue,
                   oldValue));
            }
        }
    }
}
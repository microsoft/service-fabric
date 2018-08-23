// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents the optional settings for a local ESE store.</para>
    /// </summary>
    /// <remarks>
    /// <para>Please see  HYPERLINK "http://msdn.microsoft.com/library/gg294139(v=exchg.10).aspx" http://msdn.microsoft.com/library/gg294139(v=exchg.10).aspx for documentation on ESE parameters.</para>
    /// </remarks>
    public sealed class LocalEseStoreSettings : LocalStoreSettings
    {
        /// <summary>
        /// <para>Creates and initializes a new instance of the <see cref="System.Fabric.LocalEseStoreSettings" /> class.</para>
        /// </summary>
        public LocalEseStoreSettings()
            : base(LocalStoreKind.Ese)
        {
            // Using an overloaded constructor instead of a constructor with a default parameter of false
            // to not break existing user's who may be using reflection
            //
            // 0 values cause native code to load defaults from config when converting
            // from public API structure
            //
            this.LogFileSizeInKB = 0;
            this.LogBufferSizeInKB = 0;
            this.MaxCursors = 0;
            this.MaxVerPages = 0;
            this.MaxAsyncCommitDelay = TimeSpan.FromMilliseconds(0);
            this.EnableIncrementalBackup = false;
            this.MaxCacheSizeInMB = 0;
            this.MaxDefragFrequencyInMinutes = 0;
            this.DefragThresholdInMB = 0;
            this.CompactionThresholdInMB = 0;
            this.DatabasePageSizeInKB = 0;
            this.IntrinsicValueThresholdInBytes = 0;
        }

        /// <summary>
        /// <para>Gets or sets the file path that contains the local store files.</para>
        /// </summary>
        /// <value>
        /// <para>The file path that contains the local store files.</para>
        /// </value>
        public string DbFolderPath { get; set; }

        /// <summary>
        /// <para>Maps directly to JET_paramLogFileSize on the local ESE store.
        /// </para>
        /// </summary>
        /// <remarks>
        /// The specified log file size is only applied to newly created databases. Existing databases will continue to use the log file sizes they were created with if different from this setting.
        /// </remarks>
        /// <value>
        /// <para>The log file size in KB.</para>
        /// </value>
        public int LogFileSizeInKB { get; set; }

        /// <summary>
        /// <para>Maps to JET_paramLogBuffers on the local ESE store. There is a conversion from KB to 512 bytes (volume sector size) in the mapping.</para>
        /// </summary>
        /// <value>
        /// <para>The log buffer size in KB.</para>
        /// </value>
        public int LogBufferSizeInKB { get; set; }

        /// <summary>
        /// <para>Maps directly to JET_paramMaxCursors on the local ESE store.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The maximum number of allowed database cursors.</para>
        /// </value>
        public int MaxCursors { get; set; }

        /// <summary>
        /// <para>Maps directly to JET_paramMaxVerPages on the local ESE store.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The maximum number of allowed version pages.</para>
        /// </value>
        public int MaxVerPages { get; set; }

        /// <summary>
        /// Maps directly to JET_paramCacheSizeMax on the local ESE store.
        /// </summary>
        /// <value>
        /// Returns the maximum database cache size in MB.
        /// </value>
        public int MaxCacheSizeInMB { get; set; }

        /// <summary>
        /// Gets or sets a value indicating the frequency of periodic online defragmentation.
        /// </summary>
        /// <value>
        /// Returns the frequency in minutes.
        /// </value>
        public int MaxDefragFrequencyInMinutes { get; set; }

        /// <summary>
        /// Gets or sets a value indicating the minimum logical size of a database for online defragmentation to occur the background.
        /// </summary>
        /// <value>
        /// Returns the threshold in MB.
        /// </value>
        public int DefragThresholdInMB { get; set; }

        /// <summary>
        /// Gets or sets a value indicating the minimum file size of a database for offline compaction to occur during open.
        /// </summary>
        /// <value>
        /// Returns the threshold in MB.
        /// </value>
        public int CompactionThresholdInMB { get; set; }

        /// <summary>
        /// Gets or sets a value indicating the maximum value size at which updates will occur with the JET_bitSetIntrinsicLV flag. Setting this to a non-positive value will use the ESE default of 1024 bytes.
        /// </summary>
        /// <value>
        /// Returns the threshold in bytes.
        /// </value>
        public int IntrinsicValueThresholdInBytes { get; set; }

        /// <summary>
        /// Maps directly to JET_paramDatabasePageSize on the local ESE store.
        /// </summary>
        /// <remarks>
        /// The specified database page size is only applied to newly created databases. Existing databases will continue to use the database page sizes they were created with if different from this setting.
        /// </remarks>
        /// <value>
        /// Returns the database page size in KB.
        /// </value>
        public int DatabasePageSizeInKB { get; set; }

        /// <summary>
        /// <para>Maps directly to the cmsecDurableCommit parameter on the JetCommitTransaction2() ESE API calls when local commits are performed.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The durable lazy commit duration.</para>
        /// </value>
        public TimeSpan MaxAsyncCommitDelay { get; set; }

        /// <summary>
        /// <para>Gets or sets a value indicating whether the incremental backup feature is enabled for use.</para>
        /// </summary>
        /// <value>
        /// Returns <languageKeyword>true</languageKeyword> if incremental backup is enabled; otherwise, <languageKeywork>false</languageKeywork>.
        /// </value>
        public bool EnableIncrementalBackup { get; set; }

        /// <summary>
        /// Enables in-place replace of value (versus insert/delete) during update operation. Maps to JET_bitSetOverwriteLV on the local ESE store.
        /// </summary>
        /// <remarks>
        /// Enabling this setting is useful in cases where certain access patterns can cause the ESE database file to grow on disk even though the
        /// logical data size remains same. For example, a large number of transactions getting rolled back or a series of updates made by some
        /// transactions while other transactions remain open as updates are happening. 
        /// 
        /// Note that enabling this setting may increase version store usage and the value of <see cref="System.Fabric.LocalEseStoreSettings.MaxVerPages"/>
        /// may need to be increased.
        /// </remarks>
        /// <value>
        /// Returns <languageKeyword>true</languageKeyword> if overwrite on update is enabled; otherwise, <languageKeywork>false</languageKeywork>.
        /// </value>
        public bool EnableOverwriteOnUpdate { get; set; }

        /// <summary>
        /// Convenience method to create an instance of this class initialized with property values loaded from the application configuration package.
        /// </summary>
        /// <param name="codePackageActivationContext">The activation context under which this code is running. Retrieved from <see cref="System.Fabric.FabricRuntime"/>.</param>
        /// <param name="configPackageName">The name of the configuration package (specified in the Service Manifest) containing the settings to load.</param>
        /// <param name="sectionName">The name of the section in Settings.xml within the specified configuration package containing the settings to load.</param>
        /// <returns>The initialized settings object.</returns>
        public static LocalEseStoreSettings LoadFrom(
            CodePackageActivationContext codePackageActivationContext, 
            string configPackageName, 
            string sectionName)
        {
            return Utility.WrapNativeSyncInvokeInMTA<LocalEseStoreSettings>(
                () => InternalLoadFrom(codePackageActivationContext, configPackageName, sectionName), "LocalEseStoreSettings::LoadFrom");
        }

        private static LocalEseStoreSettings InternalLoadFrom(
            CodePackageActivationContext codePackageActivationContext, 
            string configPackageName, 
            string sectionName)
        {
            using (var pin = new PinCollection())
            {
                NativeRuntime.IFabricEseLocalStoreSettingsResult nativeResult = NativeRuntime.FabricLoadEseLocalStoreSettings(
                    codePackageActivationContext.NativeActivationContext,
                    pin.AddBlittable(configPackageName),
                    pin.AddBlittable(sectionName));

                return LocalEseStoreSettings.CreateFromNative(nativeResult);
            }
        }

        internal override IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_LOCAL_STORE_KIND kind)
        {
            var nativeSettings = new NativeTypes.FABRIC_ESE_LOCAL_STORE_SETTINGS[1];

            nativeSettings[0].DbFolderPath = pin.AddBlittable(this.DbFolderPath);
            nativeSettings[0].LogFileSizeInKB = this.LogFileSizeInKB;
            nativeSettings[0].LogBufferSizeInKB = this.LogBufferSizeInKB;
            nativeSettings[0].MaxCursors = this.MaxCursors;
            nativeSettings[0].MaxVerPages = this.MaxVerPages;
            nativeSettings[0].MaxAsyncCommitDelayInMilliseconds = (Int32)this.MaxAsyncCommitDelay.TotalMilliseconds;

            var ex1Settings = new NativeTypes.FABRIC_ESE_LOCAL_STORE_SETTINGS_EX1[1];
            ex1Settings[0].EnableIncrementalBackup = NativeTypes.ToBOOLEAN(this.EnableIncrementalBackup);

            var ex2Settings = new NativeTypes.FABRIC_ESE_LOCAL_STORE_SETTINGS_EX2[1];
            ex2Settings[0].MaxCacheSizeInMB = this.MaxCacheSizeInMB;

            var ex3Settings = new NativeTypes.FABRIC_ESE_LOCAL_STORE_SETTINGS_EX3[1];
            ex3Settings[0].MaxDefragFrequencyInMinutes = this.MaxDefragFrequencyInMinutes;
            ex3Settings[0].DefragThresholdInMB = this.DefragThresholdInMB;
            ex3Settings[0].DatabasePageSizeInKB = this.DatabasePageSizeInKB;

            var ex4Settings = new NativeTypes.FABRIC_ESE_LOCAL_STORE_SETTINGS_EX4[1];
            ex4Settings[0].CompactionThresholdInMB = this.CompactionThresholdInMB;
            
            var ex5Settings = new NativeTypes.FABRIC_ESE_LOCAL_STORE_SETTINGS_EX5[1];
            ex5Settings[0].IntrinsicValueThresholdInBytes = this.IntrinsicValueThresholdInBytes;

            var ex6Settings = new NativeTypes.FABRIC_ESE_LOCAL_STORE_SETTINGS_EX6[1];
            ex6Settings[0].EnableOverwriteOnUpdate = NativeTypes.ToBOOLEAN(this.EnableOverwriteOnUpdate);

            nativeSettings[0].Reserved = pin.AddBlittable(ex1Settings);
            ex1Settings[0].Reserved = pin.AddBlittable(ex2Settings);
            ex2Settings[0].Reserved = pin.AddBlittable(ex3Settings);
            ex3Settings[0].Reserved = pin.AddBlittable(ex4Settings);
            ex4Settings[0].Reserved = pin.AddBlittable(ex5Settings);
            ex5Settings[0].Reserved = pin.AddBlittable(ex6Settings);

            kind = NativeTypes.FABRIC_LOCAL_STORE_KIND.FABRIC_LOCAL_STORE_KIND_ESE;

            return pin.AddBlittable(nativeSettings);
        }

        private static unsafe LocalEseStoreSettings CreateFromNative(NativeRuntime.IFabricEseLocalStoreSettingsResult nativeResult)
        {
            var managedSettings = new LocalEseStoreSettings();

            var uncastedSettings = nativeResult.get_Settings();
            if (uncastedSettings == IntPtr.Zero) { return managedSettings; }

            var nativeSettings = (NativeTypes.FABRIC_ESE_LOCAL_STORE_SETTINGS *)uncastedSettings;

            managedSettings.DbFolderPath = NativeTypes.FromNativeString(nativeSettings->DbFolderPath);
            managedSettings.LogFileSizeInKB = nativeSettings->LogFileSizeInKB;
            managedSettings.LogBufferSizeInKB = nativeSettings->LogBufferSizeInKB;
            managedSettings.MaxCursors = nativeSettings->MaxCursors;
            managedSettings.MaxVerPages = nativeSettings->MaxVerPages;
            managedSettings.MaxAsyncCommitDelay = TimeSpan.FromMilliseconds(nativeSettings->MaxAsyncCommitDelayInMilliseconds);

            if (nativeSettings->Reserved == IntPtr.Zero)
            {
                return managedSettings;
            }

            var ex1Settings = (NativeTypes.FABRIC_ESE_LOCAL_STORE_SETTINGS_EX1 *)nativeSettings->Reserved;

            managedSettings.EnableIncrementalBackup = NativeTypes.FromBOOLEAN(ex1Settings->EnableIncrementalBackup);

            if (ex1Settings->Reserved == IntPtr.Zero)
            {
                return managedSettings;
            }

            var ex2Settings = (NativeTypes.FABRIC_ESE_LOCAL_STORE_SETTINGS_EX2*)ex1Settings->Reserved;

            managedSettings.MaxCacheSizeInMB = ex2Settings->MaxCacheSizeInMB;

            if (ex2Settings->Reserved == IntPtr.Zero)
            {
                return managedSettings;
            }

            var ex3Settings = (NativeTypes.FABRIC_ESE_LOCAL_STORE_SETTINGS_EX3*)ex2Settings->Reserved;

            managedSettings.MaxDefragFrequencyInMinutes = ex3Settings->MaxDefragFrequencyInMinutes;
            managedSettings.DefragThresholdInMB = ex3Settings->DefragThresholdInMB;
            managedSettings.DatabasePageSizeInKB = ex3Settings->DatabasePageSizeInKB;

            if (ex3Settings->Reserved == IntPtr.Zero)
            {
                return managedSettings;
            }

            var ex4Settings = (NativeTypes.FABRIC_ESE_LOCAL_STORE_SETTINGS_EX4*)ex3Settings->Reserved;
            managedSettings.CompactionThresholdInMB = ex4Settings->CompactionThresholdInMB;

            if (ex4Settings->Reserved == IntPtr.Zero)
            {
                return managedSettings;
            }

            var ex5Settings = (NativeTypes.FABRIC_ESE_LOCAL_STORE_SETTINGS_EX5*)ex4Settings->Reserved;
            managedSettings.IntrinsicValueThresholdInBytes = ex5Settings->IntrinsicValueThresholdInBytes;

            if (ex5Settings->Reserved == IntPtr.Zero)
            {
                return managedSettings;
            }

            var ex6Settings = (NativeTypes.FABRIC_ESE_LOCAL_STORE_SETTINGS_EX6*)ex5Settings->Reserved;
            managedSettings.EnableOverwriteOnUpdate = NativeTypes.FromBOOLEAN(ex6Settings->EnableOverwriteOnUpdate);

            GC.KeepAlive(nativeResult);

            return managedSettings;
        }
    }
}
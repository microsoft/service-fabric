// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// Represents the settings for a key/value store replica.
    /// </summary>
    public class KeyValueStoreReplicaSettings
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.KeyValueStoreReplicaSettings"/> class.
        /// </summary>
        public KeyValueStoreReplicaSettings()
        {
            Utility.WrapNativeSyncInvokeInMTA(() => LoadFromConfigHelper(), "KeyValueStoreReplicaSettings.LoadFromConfigHelper");
        }

        /// <summary>
        /// Gets or sets a value indicating the amount of time outstanding transactions have to drain during demotion from primary role before the host process is forcefully terminated.
        /// </summary>
        /// <value>
        /// Returns the drain timeout.
        /// </value>
        public TimeSpan TransactionDrainTimeout  { get; set; }

        /// <summary>
        /// Gets or sets a value indicating the secondary notification mode to enable. <see cref="System.Fabric.KeyValueStoreReplica.SecondaryNotificationMode" />
        /// </summary>
        /// <value>
        /// Returns the secondary notification mode.
        /// </value>
        public KeyValueStoreReplica.SecondaryNotificationMode SecondaryNotificationMode  { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether or not to prefetch the database during open.
        /// </summary>
        /// <value>
        /// Returns true if enabled, false otherwise.
        /// </value>
        public bool EnableCopyNotificationPrefetch { get; set; }

        /// <summary>
        /// Gets or sets a value indicating the full copy mode to use when building secondary replicas. <see cref="System.Fabric.KeyValueStoreReplica.FullCopyMode" />
        /// </summary>
        /// <value>
        /// Returns the full copy mode.
        /// </value>
        public KeyValueStoreReplica.FullCopyMode FullCopyMode { get; set; }

        /// <summary>
        /// Gets or sets the interval after which <see cref="System.Fabric.KeyValueStoreReplica"/> tries to truncate local store logs
        /// if local store has <see cref="System.Fabric.LocalEseStoreSettings.EnableIncrementalBackup"/> enabled and no backup has been initiated by user during this interval.
        /// </summary>
        public int LogTruncationIntervalInMinutes { get; set; }

        private unsafe void LoadFromConfigHelper()
        {
            var nativeResult = NativeRuntimeInternal.GetFabricKeyValueStoreReplicaDefaultSettings();
            var nativeSettingsPtr = nativeResult.get_Settings();

            this.FromNative(nativeSettingsPtr);

            GC.KeepAlive(nativeResult);
        }

        internal IntPtr ToNative(PinCollection pin)
        {
            var nativeSettings = new NativeTypes.FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS[1];
            nativeSettings[0].TransactionDrainTimeoutInSeconds = (UInt32)this.TransactionDrainTimeout.TotalSeconds;
            nativeSettings[0].SecondaryNotificationMode = (NativeTypes.FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE)this.SecondaryNotificationMode;

            var ex1Settings = new NativeTypes.FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_EX1[1];
            ex1Settings[0].EnableCopyNotificationPrefetch = NativeTypes.ToBOOLEAN(this.EnableCopyNotificationPrefetch);

            var ex2Settings = new NativeTypes.FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_EX2[1];
            ex2Settings[0].FullCopyMode = (NativeTypes.FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE)this.FullCopyMode;

            var ex3Settings = new NativeTypes.FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_EX3[1];
            ex3Settings[0].LogTruncationIntervalInMinutes = this.LogTruncationIntervalInMinutes;

            nativeSettings[0].Reserved = pin.AddBlittable(ex1Settings);
            ex1Settings[0].Reserved = pin.AddBlittable(ex2Settings);
            ex2Settings[0].Reserved = pin.AddBlittable(ex3Settings);

            return pin.AddBlittable(nativeSettings);
        }

        private unsafe void FromNative(IntPtr nativeSettingsPtr)
        {
            ReleaseAssert.AssertIf(nativeSettingsPtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeSettingsPtr"));

            var nativeSettings = (NativeTypes.FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS*)nativeSettingsPtr;

            this.TransactionDrainTimeout = TimeSpan.FromSeconds(nativeSettings->TransactionDrainTimeoutInSeconds);
            this.SecondaryNotificationMode = (KeyValueStoreReplica.SecondaryNotificationMode)nativeSettings->SecondaryNotificationMode;

            if (nativeSettings->Reserved == IntPtr.Zero)
            {
                return;
            }

            var ex1Settings = (NativeTypes.FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_EX1*)nativeSettings->Reserved;

            this.EnableCopyNotificationPrefetch = NativeTypes.FromBOOLEAN(ex1Settings->EnableCopyNotificationPrefetch);

            if (ex1Settings->Reserved == IntPtr.Zero)
            {
                return;
            }

            var ex2Settings = (NativeTypes.FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_EX2*)ex1Settings->Reserved;

            this.FullCopyMode = (KeyValueStoreReplica.FullCopyMode)ex2Settings->FullCopyMode;

            if (ex2Settings->Reserved == IntPtr.Zero)
            {
                return;
            }

            var ex3Settings = (NativeTypes.FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_EX3*)ex2Settings->Reserved;

            this.LogTruncationIntervalInMinutes = ex3Settings->LogTruncationIntervalInMinutes;
        }
    }
}
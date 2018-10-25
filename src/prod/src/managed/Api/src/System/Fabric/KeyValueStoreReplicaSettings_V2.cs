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

    internal class KeyValueStoreReplicaSettings_V2
    {
        public KeyValueStoreReplicaSettings_V2(string workingDirectory)
            : this(workingDirectory, string.Empty, string.Empty, Guid.Empty)
        {
        }

        public KeyValueStoreReplicaSettings_V2(
            string workingDirectory, 
            string sharedLogDirectory, 
            string sharedLogFileName, 
            Guid sharedLogGuid)
        {
            Requires.Argument<string>("workingDirectory", workingDirectory).NotNullOrEmpty();

            Utility.WrapNativeSyncInvokeInMTA(() => LoadFromConfigHelper(
                workingDirectory, 
                sharedLogDirectory,
                sharedLogFileName,
                sharedLogGuid), 
            "KeyValueStoreReplicaSettings_V2.LoadFromConfigHelper");
        }

        public string WorkingDirectory { get; private set; }

        public SharedLogSettings SharedLogSettings  { get; private set; }

        public KeyValueStoreReplica.SecondaryNotificationMode SecondaryNotificationMode  { get; set; }

        private unsafe void LoadFromConfigHelper(
            string workingDirectory, 
            string sharedLogDirectory, 
            string sharedLogFileName, 
            Guid sharedLogGuid)
        {
            using (var pin = new PinCollection())
            {
                var nativeResult = NativeRuntimeInternal.GetFabricKeyValueStoreReplicaDefaultSettings_V2(
                    pin.AddObject(workingDirectory),
                    pin.AddObject(sharedLogDirectory),
                    pin.AddObject(sharedLogFileName),
                    sharedLogGuid);
                var nativeSettingsPtr = nativeResult.get_Settings();

                this.FromNative(nativeSettingsPtr);

                GC.KeepAlive(nativeResult);
            }
        }

        internal IntPtr ToNative(PinCollection pin)
        {
            var nativeSettings = new NativeTypes.FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2[1];
            nativeSettings[0].WorkingDirectory = pin.AddObject(this.WorkingDirectory);
            nativeSettings[0].SharedLogSettings = this.SharedLogSettings.ToNative(pin);
            nativeSettings[0].SecondaryNotificationMode = (NativeTypes.FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE)this.SecondaryNotificationMode;

            return pin.AddBlittable(nativeSettings);
        }

        private unsafe void FromNative(IntPtr nativeSettingsPtr)
        {
            ReleaseAssert.AssertIf(nativeSettingsPtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeSettingsPtr"));

            var nativeSettings = (NativeTypes.FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2*)nativeSettingsPtr;

            this.WorkingDirectory = NativeTypes.FromNativeString(nativeSettings->WorkingDirectory);
            this.SharedLogSettings = new SharedLogSettings(nativeSettings->SharedLogSettings);
            this.SecondaryNotificationMode = (KeyValueStoreReplica.SecondaryNotificationMode)nativeSettings->SecondaryNotificationMode;
        }
    }
}
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

    internal class SharedLogSettings
    {
        public SharedLogSettings(string workingDirectory) 
            : this(workingDirectory, string.Empty, string.Empty, Guid.Empty)
        {
        }

        public SharedLogSettings(
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
            "SharedLogSettings.LoadFromConfigHelper");
        }

        internal SharedLogSettings(IntPtr nativeSettingsPtr)
        {
            this.FromNative(nativeSettingsPtr);
        }

        public string ContainerPath { get; set; }

        public Guid ContainerId { get; set; }

        public long LogSizeInBytes  { get; set; }

        public int MaximumNumberStreams  { get; set; }

        public int MaximumRecordSize  { get; set; }

        public int CreateFlags  { get; set; }

        private unsafe void LoadFromConfigHelper(
            string workingDirectory, 
            string sharedLogDirectory, 
            string sharedLogFileName, 
            Guid sharedLogGuid)
        {
            using (var pin = new PinCollection())
            {
                var nativeResult = NativeRuntimeInternal.GetFabricSharedLogDefaultSettings(
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
            var nativeSettings = new NativeTypes.KTLLOGGER_SHARED_LOG_SETTINGS[1];
            nativeSettings[0].ContainerPath = pin.AddObject(this.ContainerPath);
            nativeSettings[0].ContainerId = pin.AddObject(this.ContainerId.ToString());
            nativeSettings[0].LogSize = this.LogSizeInBytes;
            nativeSettings[0].MaximumNumberStreams = this.MaximumNumberStreams;
            nativeSettings[0].MaximumRecordSize = this.MaximumRecordSize;
            nativeSettings[0].CreateFlags = this.CreateFlags;

            return pin.AddBlittable(nativeSettings);
        }

        private unsafe void FromNative(IntPtr nativeSettingsPtr)
        {
            ReleaseAssert.AssertIf(nativeSettingsPtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeSettingsPtr"));

            var nativeSettings = (NativeTypes.KTLLOGGER_SHARED_LOG_SETTINGS*)nativeSettingsPtr;

            this.ContainerPath = NativeTypes.FromNativeString(nativeSettings->ContainerPath);
            this.ContainerId = new Guid(NativeTypes.FromNativeString(nativeSettings->ContainerId));
            this.LogSizeInBytes = nativeSettings->LogSize;
            this.MaximumNumberStreams = nativeSettings->MaximumNumberStreams;
            this.MaximumRecordSize = nativeSettings->MaximumRecordSize;
            this.CreateFlags = nativeSettings->CreateFlags;
        }
    }
}
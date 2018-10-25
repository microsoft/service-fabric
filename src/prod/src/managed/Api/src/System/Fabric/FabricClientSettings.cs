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
    /// <para>Represents the configuration settings for the <see cref="System.Fabric.FabricClient" /> class.</para>
    /// </summary>
    public sealed class FabricClientSettings
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.FabricClientSettings" /> class.</para>
        /// </summary>
        public FabricClientSettings()
        {
            this.ClientFriendlyName = null;

            this.PartitionLocationCacheBucketCount = 0;

            this.HealthReportRetrySendInterval = TimeSpan.Zero;

            Utility.WrapNativeSyncInvokeInMTA(() => LoadFromConfigHelper(), "FabricClientSettings.LoadFromConfigHelper");
        }

        internal FabricClientSettings(bool skipLoadFromDefaultConfig)
        {
            this.ClientFriendlyName = null;

            this.PartitionLocationCacheBucketCount = 0;

            this.HealthReportRetrySendInterval = TimeSpan.Zero;

            if (!skipLoadFromDefaultConfig)
            {
                Utility.WrapNativeSyncInvokeInMTA(() => LoadFromConfigHelper(), "FabricClientSettings.LoadFromConfigHelper");
            }
        }

        /// <summary>
        /// <para>Gets or sets the client friendly name that will appear in Service Fabric traces for debugging.</para>
        /// </summary>
        /// <value>
        /// <para>The client friendly name that will appear in Service Fabric traces for debugging.</para>
        /// </value>
        /// <remarks>
        /// <para>The default value is null and the client friendly name will automatically be generated as a GUID internally.</para>
        /// <para>If multiple clients can be created from the same process or on the same node, it is recommended to append a unique identifier to the name.
        /// For example, MyProcessIdentifier-{guid}.
        /// This ensures that traces can track different actions to the clients that generated them.
        /// </para>
        /// </remarks>
        public string ClientFriendlyName { get; set; }

        /// <summary>
        /// <para>Gets the maximum number of cached location entries on the client.</para>
        /// </summary>
        /// <value>
        /// <para>The maximum number of cached location entries on the client.</para>
        /// </value>
        /// <remarks>
        /// <para>The default value of the <see cref="System.Fabric.FabricClientSettings.PartitionLocationCacheLimit" /> property is 1000.</para>
        /// <para>The <see cref="System.Fabric.FabricClientSettings.PartitionLocationCacheLimit" /> property is not updatable. Setting this property will throw a <see cref="System.ArgumentException" /> exception.</para>
        /// <para>When the cache limit is reached the oldest entries are discarded first. The default value is 100.</para>
        /// </remarks>
        public long PartitionLocationCacheLimit { get; set; }

        /// <summary>
        /// <para>Gets or sets the bucket count used by the client’s service resolution cache.</para>
        /// </summary>
        /// <value>
        /// <para>The bucket count used by the client’s service resolution cache.</para>
        /// </value>
        /// <remarks>
        /// <para>The default value is 1024.</para>
        /// </remarks>
        public long PartitionLocationCacheBucketCount { get; set; }

        /// <summary>
        /// <para>Gets or sets the timeout on service change notification requests from the client to the gateway for all registered callbacks.</para>
        /// </summary>
        /// <value>
        /// <para>The timeout on service change notification requests from the client to the gateway for all registered callbacks.</para>
        /// </value>
        /// <remarks>
        /// <para>The default value of the <see cref="System.Fabric.FabricClientSettings.ServiceChangePollInterval" /> property is 120 seconds.</para>
        /// </remarks>
        public TimeSpan ServiceChangePollInterval { get; set; }

        /// <summary>
        /// <para>Gets or sets the timeout after which, if the current gateway address does not respond with a valid connection, another different address is randomly selected from the gateway addresses collection.</para>
        /// </summary>
        /// <value>
        /// <para>The timeout after the current gateway address does not respond with a valid connection.</para>
        /// </value>
        /// <remarks>
        /// <para>The default value of the <see cref="System.Fabric.FabricClientSettings.ConnectionInitializationTimeout" /> property is 2 seconds.</para>
        /// <para>The <see cref="System.Fabric.FabricClientSettings.ConnectionInitializationTimeout" /> property must be less than the value of the <see cref="System.Fabric.FabricClientSettings.ServiceChangePollInterval" /> property.</para>
        /// </remarks>
        public TimeSpan ConnectionInitializationTimeout { get; set; }

        /// <summary>
        /// <para>Gets the interval at which the <see cref="System.Fabric.FabricClient" /> will ping the connected endpoint.</para>
        /// </summary>
        /// <value>
        /// <para>The interval at which the <see cref="System.Fabric.FabricClient" /> will ping the connected endpoint.</para>
        /// </value>
        /// <remarks>
        /// <para>The default value of the <see cref="System.Fabric.FabricClientSettings.KeepAliveInterval" /> property is 0 seconds.</para>
        /// <para>This property can't be updated after the <see cref="System.Fabric.FabricClient" /> is opened.
        /// Setting this property will throw a <see cref="System.ArgumentException" /> exception.</para>
        /// <para>
        ///     <see cref="System.Fabric.FabricClient" /> will continue pinging as long as it has pending operations.</para>
        /// </remarks>
        public TimeSpan KeepAliveInterval { get; set; }

        /// <summary>
        /// This parameter has been deprecated. This will be removed in our next release.
        /// </summary>
        //[Obsolete("This parameter has been deprecated.")]
        public TimeSpan ConnectionIdleTimeout { get; set; }

        /// <summary>
        /// <para>Gets or sets the timeout on health operation requests from the client to the gateway.</para>
        /// </summary>
        /// <value>
        /// <para>The timeout on health operation requests from the client to the gateway. This setting applies only to report health operations, and is not
        /// used by the query operations.</para>
        /// </value>
        /// <remarks>
        /// <para>The default value of the <see cref="System.Fabric.FabricClientSettings.HealthOperationTimeout" /> property is 120 seconds.</para>
        /// </remarks>
        public TimeSpan HealthOperationTimeout { get; set; }

        /// <summary>
        /// <para>Gets or sets the interval at which health reports are sent to Health Manager.</para>
        /// </summary>
        /// <value>
        /// <para>The interval at which health reports are sent to Health Manager.</para>
        /// </value>
        /// <remarks>
        /// <para>The default value of the <see cref="System.Fabric.FabricClientSettings.HealthReportSendInterval" /> property is 30 seconds.</para>
        /// </remarks>
        public TimeSpan HealthReportSendInterval { get; set; }

        /// <summary>
        /// <para>Gets or sets the retry interval at which health reports that have not yet been acknowledged by the Health Manager are resent.</para>
        /// </summary>
        /// <value>
        /// <para>The retry interval at which health reports that have not yet been acknowledged by the Health Manager are resent.</para>
        /// </value>
        /// <remarks>
        /// <para>The default value of the <see cref="System.Fabric.FabricClientSettings.HealthReportRetrySendInterval" /> property is 30 seconds.</para>
        /// </remarks>
        public TimeSpan HealthReportRetrySendInterval { get; set; }

        /// <summary>
        /// <para>Gets or sets the timeout for running a re-connection protocol if the client has registered for service notifications. The default value is 30 seconds. </para>
        /// </summary>
        /// <value>
        /// <para>The timeout for running a re-connection protocol if the client has registered for service notifications.</para>
        /// </value>
        public TimeSpan NotificationGatewayConnectionTimeout { get; set; }

        /// <summary>
        /// <para>Gets or sets the timeout for updating the local cache in response to service notifications. The default value is 30 seconds.</para>
        /// </summary>
        /// <value>
        /// <para>The timeout for updating the local cache in response to service notifications.</para>
        /// </value>
        public TimeSpan NotificationCacheUpdateTimeout { get; set; }

        /// <summary>
        /// Gets or sets a value indicating the buffer size to use when retrieving an authentication token from Azure Active Directory.
        /// </summary>
        /// <value>
        /// Returns the buffer size in bytes.
        /// </value>
        public long AuthTokenBufferSize { get; set; }


        internal void Validate()
        {
            if (this.PartitionLocationCacheLimit <= 0)
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidFabricClientPartitionLocationCacheLimit, this.PartitionLocationCacheLimit), "FabricClientSettings.PartitionLocationCacheLimit");
            }

            if (this.PartitionLocationCacheBucketCount < 0)
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_NonNegative_Value_Required), "FabricClientSettings.PartitionLocationCacheBucketCount");
            }

            if (this.ServiceChangePollInterval.TotalSeconds <= 0)
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidFabricClientSettingsTimeout, this.ServiceChangePollInterval), "FabricClientSettings.ServiceChangePollInterval");
            }

            if (this.ConnectionInitializationTimeout.TotalSeconds <= 0)
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidFabricClientSettingsTimeout, this.ConnectionInitializationTimeout), "FabricClientSettings.ConnectionInitializationTimeout");
            }

            if (this.KeepAliveInterval.TotalSeconds < 0)
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidFabricClientSettingsTimeout, this.KeepAliveInterval), "FabricClientSettings.KeepAliveInterval");
            }

            if (this.ConnectionIdleTimeout.TotalSeconds < 0)
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidFabricClientSettingsTimeout, this.ConnectionIdleTimeout), "FabricClientSettings.ConnectionIdleTimeout");
            }

            if (this.HealthOperationTimeout.TotalSeconds <= 0)
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidFabricClientSettingsTimeout, this.HealthOperationTimeout), "FabricClientSettings.HealthOperationTimeout");
            }

            if (this.HealthReportSendInterval.TotalSeconds < 0)
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidFabricClientSettingsTimeout, this.HealthReportSendInterval), "FabricClientSettings.HealthReportSendInterval");
            }

            if (this.HealthReportRetrySendInterval.TotalSeconds < 0)
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidFabricClientSettingsTimeout, this.HealthReportRetrySendInterval), "FabricClientSettings.HealthReportRetrySendInterval");
            }

            if (this.NotificationGatewayConnectionTimeout.TotalSeconds < 0)
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidFabricClientSettingsTimeout, this.NotificationGatewayConnectionTimeout), "FabricClientSettings.NotificationGatewayConnectionTimeout");
            }

            if (this.NotificationCacheUpdateTimeout.TotalSeconds < 0)
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidFabricClientSettingsTimeout, this.NotificationCacheUpdateTimeout), "FabricClientSettings.NotificationCacheUpdateTimeout");
            }

            if (this.AuthTokenBufferSize < 0)
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_NonNegative_Value_Required), "FabricClientSettings.AuthTokenBufferSize");
            }

        }

        internal IntPtr ToNative(PinCollection pin)
        {
            var nativeSettings = new NativeTypes.FABRIC_CLIENT_SETTINGS[1];

            nativeSettings[0].PartitionLocationCacheLimit = (uint)this.PartitionLocationCacheLimit;
            nativeSettings[0].ServiceChangePollIntervalInSeconds = (uint)this.ServiceChangePollInterval.TotalSeconds;
            nativeSettings[0].ConnectionInitializationTimeoutInSeconds = (uint)this.ConnectionInitializationTimeout.TotalSeconds;
            nativeSettings[0].KeepAliveIntervalInSeconds = (uint)this.KeepAliveInterval.TotalSeconds;
            nativeSettings[0].HealthOperationTimeoutInSeconds = (uint)this.HealthOperationTimeout.TotalSeconds;
            nativeSettings[0].HealthReportSendIntervalInSeconds = (uint)this.HealthReportSendInterval.TotalSeconds;

            var nativeSettingsEx1 = new NativeTypes.FABRIC_CLIENT_SETTINGS_EX1[1];

            if (this.ClientFriendlyName != null)
            {
                nativeSettingsEx1[0].ClientFriendlyName = pin.AddObject(this.ClientFriendlyName);
            }

            if ((uint)this.PartitionLocationCacheBucketCount > 0)
            {
                nativeSettingsEx1[0].PartitionLocationCacheBucketCount = (uint)this.PartitionLocationCacheBucketCount;
            }

            nativeSettingsEx1[0].HealthReportRetrySendIntervalInSeconds = (uint)this.HealthReportRetrySendInterval.TotalSeconds;

            var nativeSettingsEx2 = new NativeTypes.FABRIC_CLIENT_SETTINGS_EX2[1];

            nativeSettingsEx2[0].NotificationGatewayConnectionTimeoutInSeconds = (uint)this.NotificationGatewayConnectionTimeout.TotalSeconds;
            nativeSettingsEx2[0].NotificationCacheUpdateTimeoutInSeconds = (uint)this.NotificationCacheUpdateTimeout.TotalSeconds;

            var nativeSettingsEx3 = new NativeTypes.FABRIC_CLIENT_SETTINGS_EX3[1];

            nativeSettingsEx3[0].AuthTokenBufferSize = (uint)this.AuthTokenBufferSize;

            var nativeSettingsEx4 = new NativeTypes.FABRIC_CLIENT_SETTINGS_EX4[1];

            nativeSettingsEx4[0].ConnectionIdleTimeoutInSeconds = (uint)this.ConnectionIdleTimeout.TotalSeconds;

            nativeSettings[0].Reserved = pin.AddBlittable(nativeSettingsEx1);
            nativeSettingsEx1[0].Reserved = pin.AddBlittable(nativeSettingsEx2);
            nativeSettingsEx2[0].Reserved = pin.AddBlittable(nativeSettingsEx3);
            nativeSettingsEx3[0].Reserved = pin.AddBlittable(nativeSettingsEx4);

            return pin.AddBlittable(nativeSettings);
        }

        internal static unsafe FabricClientSettings FromNative(NativeClient.IFabricClientSettingsResult nativeSettings)
        {
            var nativeSettingsPtr = nativeSettings.get_Settings();

            FabricClientSettings settings = new FabricClientSettings(true /*skipLoadFromConfigHelper*/);
            settings.FromNative(nativeSettingsPtr);

            GC.KeepAlive(nativeSettings);

            return settings;
        }

        private unsafe void LoadFromConfigHelper()
        {
            var nativeResult = NativeClientInternal.GetFabricClientDefaultSettings();
            var nativeSettingsPtr = nativeResult.get_Settings();

            this.FromNative(nativeSettingsPtr);

            GC.KeepAlive(nativeResult);
        }

        private unsafe void FromNative(IntPtr nativeSettingsPtr)
        {
            ReleaseAssert.AssertIf(nativeSettingsPtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeSettingsPtr"));

            var nativeSettings = (NativeTypes.FABRIC_CLIENT_SETTINGS*)nativeSettingsPtr;

            this.PartitionLocationCacheLimit = (long)nativeSettings->PartitionLocationCacheLimit;
            this.ServiceChangePollInterval = TimeSpan.FromSeconds((double)nativeSettings->ServiceChangePollIntervalInSeconds);
            this.ConnectionInitializationTimeout = TimeSpan.FromSeconds((double)nativeSettings->ConnectionInitializationTimeoutInSeconds);
            this.KeepAliveInterval = TimeSpan.FromSeconds((double)nativeSettings->KeepAliveIntervalInSeconds);
            this.HealthOperationTimeout = TimeSpan.FromSeconds((double)nativeSettings->HealthOperationTimeoutInSeconds);
            this.HealthReportSendInterval = TimeSpan.FromSeconds((double)nativeSettings->HealthReportSendIntervalInSeconds);

            if (nativeSettings->Reserved != IntPtr.Zero)
            {
                var castedEx1 = (NativeTypes.FABRIC_CLIENT_SETTINGS_EX1*)nativeSettings->Reserved;

                if (castedEx1->ClientFriendlyName != IntPtr.Zero)
                {
                    this.ClientFriendlyName = NativeTypes.FromNativeString(castedEx1->ClientFriendlyName);
                }

                this.PartitionLocationCacheBucketCount = castedEx1->PartitionLocationCacheBucketCount;
                this.HealthReportRetrySendInterval = TimeSpan.FromSeconds((double)castedEx1->HealthReportRetrySendIntervalInSeconds);

                if (castedEx1->Reserved != IntPtr.Zero)
                {
                    var castedEx2 = (NativeTypes.FABRIC_CLIENT_SETTINGS_EX2*)castedEx1->Reserved;

                    this.NotificationGatewayConnectionTimeout = TimeSpan.FromSeconds((double)castedEx2->NotificationGatewayConnectionTimeoutInSeconds);
                    this.NotificationCacheUpdateTimeout = TimeSpan.FromSeconds((double)castedEx2->NotificationCacheUpdateTimeoutInSeconds);

                    if (castedEx2->Reserved != IntPtr.Zero)
                    {
                        var castedEx3 = (NativeTypes.FABRIC_CLIENT_SETTINGS_EX3*)castedEx2->Reserved;

                        this.AuthTokenBufferSize = castedEx3->AuthTokenBufferSize;

                        if (castedEx3->Reserved != IntPtr.Zero)
                        {
                            var castedEx4 = (NativeTypes.FABRIC_CLIENT_SETTINGS_EX4*)castedEx3->Reserved;

                            this.ConnectionIdleTimeout = TimeSpan.FromSeconds((double)castedEx4->ConnectionIdleTimeoutInSeconds);

                        }
                    }
                }
            }
        }
    }
}
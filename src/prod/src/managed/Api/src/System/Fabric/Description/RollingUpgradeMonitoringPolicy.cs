// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// <para>Represents a class to encapsulate a rolling upgrade monitoring policy.</para>
    /// </summary>
    public class RollingUpgradeMonitoringPolicy
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy" /> class.</para>
        /// </summary>
        /// <remarks>
        /// <para>The initialization sets the properties of the <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy" /> class with the following defaults.</para>
        /// <para>Property</para>
        /// <para>Default value</para>
        ///   <list type="table">
        ///     <item>
        ///       <term>
        ///         <para>
        ///           <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.FailureAction" />
        ///         </para>
        ///       </term>
        ///       <description>
        ///         <para>
        ///           <see cref="System.Fabric.UpgradeFailureAction.Invalid" />
        ///         </para>
        ///         <para>This value must be changed or a <see cref="System.ArgumentException" /> will be thrown before the upgrade begins.</para>
        ///       </description>
        ///     </item>
        ///     <item>
        ///       <term>
        ///         <para>
        ///           <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.HealthCheckWaitDuration" />
        ///         </para>
        ///       </term>
        ///       <description>
        ///         <para>
        ///           <see cref="System.TimeSpan.Zero" />
        ///         </para>
        ///       </description>
        ///     </item>
        ///     <item>
        ///       <term>
        ///         <para>
        ///           <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.HealthCheckStableDuration" />
        ///         </para>
        ///       </term>
        ///       <description>
        ///         <para>
        ///           <see cref="System.TimeSpan" /> value that defaults to 120 seconds.
        ///         </para>
        ///       </description>
        ///     </item>
        ///     <item>
        ///       <term>
        ///         <para>
        ///           <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.HealthCheckRetryTimeout" />
        ///         </para>
        ///       </term>
        ///       <description>
        ///         <para>600 seconds</para>
        ///       </description>
        ///     </item>
        ///     <item>
        ///       <term>
        ///         <para>
        ///           <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.UpgradeDomainTimeout" />
        ///         </para>
        ///       </term>
        ///       <description>
        ///         <para>TimeSpan.FromSeconds(uint.MaxValue)</para>
        ///       </description>
        ///     </item>
        ///     <item>
        ///       <term>
        ///         <para>
        ///           <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.UpgradeTimeout" />
        ///         </para>
        ///       </term>
        ///       <description>
        ///         <para>TimeSpan.FromSeconds(uint.MaxValue)</para>
        ///       </description>
        ///     </item>
        ///   </list>
        /// </remarks>
        public RollingUpgradeMonitoringPolicy()
        {
            // Create default policy using the default values specified in native
            Utility.WrapNativeSyncInvokeInMTA(() => LoadFromConfigHelper(), "RollingUpgradeMonitoringPolicy.LoadFromConfigHelper");
        }

        /// <summary>
        /// <para>Gets or sets the action to take if an upgrade fails. The default is <see cref="System.Fabric.UpgradeFailureAction.Invalid" />.</para>
        /// </summary>
        /// <value>
        /// <para>The action to take if an upgrade fails.</para>
        /// </value>
        /// <exception cref="System.ArgumentException">
        /// <para>The <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.FailureAction" /> property is set to 
        /// <see cref="System.Fabric.UpgradeFailureAction.Invalid" />.</para>
        /// </exception>
        /// <remarks>
        /// <para>The <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.FailureAction" /> property must be changed from the default 
        /// of <see cref="System.Fabric.UpgradeFailureAction.Invalid" /> or a <see cref="System.ArgumentException" /> will be thrown before the upgrade begins.</para>
        /// </remarks>
        public UpgradeFailureAction FailureAction { get; set; }

        /// <summary>
        /// <para>Gets or sets the length of time to wait after completing an upgrade domain before performing health checks.</para>
        /// </summary>
        /// <value>
        /// <para>The length of time to wait after completing an upgrade domain before performing health checks.</para>
        /// </value>
        /// <remarks>
        /// <para>The default is <see cref="System.TimeSpan.Zero" />.</para>
        /// </remarks>
        [JsonCustomization(PropertyName = JsonPropertyNames.HealthCheckWaitDurationInMilliseconds)]
        public TimeSpan HealthCheckWaitDuration { get; set; }

        /// <summary>
        /// <para>Gets or sets the length of time that health checks must pass continuously before the upgrade proceeds 
        /// to the next upgrade domain.</para>
        /// </summary>
        /// <value>
        /// <para>The length of time that health checks must pass continuously.</para>
        /// </value>
        /// <remarks>
        /// <para>The default is 120 seconds.</para>
        /// </remarks>
        [JsonCustomization(PropertyName = JsonPropertyNames.HealthCheckStableDurationInMilliseconds)]
        public TimeSpan HealthCheckStableDuration { get; set; }

        /// <summary>
        /// <para>Gets or sets the length of time that health checks can fail continuously before the upgrade fails and the action specified by <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.FailureAction" /> occurs.</para>
        /// </summary>
        /// <value>
        /// <para>The length of time that health checks can fail continuously.</para>
        /// </value>
        /// <remarks>
        /// <para>The default is 600 seconds. Setting <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.HealthCheckRetryTimeout" /> 
        /// to <see cref="System.TimeSpan.Zero" /> will result in only a single health check.</para>
        /// </remarks>
        [JsonCustomization(PropertyName = JsonPropertyNames.HealthCheckRetryTimeoutInMilliseconds)]
        public TimeSpan HealthCheckRetryTimeout { get; set; }

        /// <summary>
        /// <para>Gets or sets the length of time that the overall upgrade can take before the upgrade fails and the action specified by <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.FailureAction" /> occurs.</para>
        /// </summary>
        /// <value>
        /// <para>The upgrade timeout.</para>
        /// </value>
        /// <remarks>
        /// <para>The default is TimeSpan.FromSeconds(uint.MaxValue).</para>
        /// </remarks>
        [JsonCustomization(PropertyName = JsonPropertyNames.UpgradeTimeoutInMilliseconds)]
        public TimeSpan UpgradeTimeout { get; set; }

        /// <summary>
        /// <para>Gets or sets the length of time that the processing of any upgrade domain can take before the upgrade fails and the action specified by <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.FailureAction" /> occurs.</para>
        /// </summary>
        /// <value>
        /// <para>The timeout for any upgrade domain.</para>
        /// </value>
        /// <remarks>
        /// <para>The default is TimeSpan.FromSeconds(uint.MaxValue).</para>
        /// </remarks>
        [JsonCustomization(PropertyName = JsonPropertyNames.UpgradeDomainTimeoutInMilliseconds)]
        public TimeSpan UpgradeDomainTimeout { get; set; }

        internal void Validate()
        {
            if (this.FailureAction == UpgradeFailureAction.Invalid)
            {
                throw new ArgumentException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_ArgumentInvalid_Formatted,
                        "RollingUpgradeMonitoringPolicy.FailureAction",
                        "Validate"));
            }
        }

        // For unit tests
        internal static bool AreEqual(RollingUpgradeMonitoringPolicy left, RollingUpgradeMonitoringPolicy right)
        {
            if (left != null && right != null)
            {
                return
                    left.FailureAction == right.FailureAction &&
                    left.HealthCheckWaitDuration == right.HealthCheckWaitDuration &&
                    left.HealthCheckStableDuration == right.HealthCheckStableDuration &&
                    left.HealthCheckRetryTimeout == right.HealthCheckRetryTimeout &&
                    left.UpgradeTimeout == right.UpgradeTimeout &&
                    left.UpgradeDomainTimeout == right.UpgradeDomainTimeout;
            }
            else
            {
                return (left == null && right == null);
            }
        }

        internal static unsafe RollingUpgradeMonitoringPolicy FromNative(IntPtr nativePolicyPtr)
        {
            if (nativePolicyPtr == IntPtr.Zero) { return null; }

            var managedPolicy = new RollingUpgradeMonitoringPolicy();
            var nativePolicy = (NativeTypes.FABRIC_ROLLING_UPGRADE_MONITORING_POLICY*)nativePolicyPtr;

            managedPolicy.FailureAction = (UpgradeFailureAction)nativePolicy->FailureAction;
            managedPolicy.HealthCheckWaitDuration = FromNativeTimeInSeconds(nativePolicy->HealthCheckWaitDurationInSeconds);
            managedPolicy.HealthCheckRetryTimeout = FromNativeTimeInSeconds(nativePolicy->HealthCheckRetryTimeoutInSeconds);
            managedPolicy.UpgradeTimeout = FromNativeTimeInSeconds(nativePolicy->UpgradeTimeoutInSeconds);
            managedPolicy.UpgradeDomainTimeout = FromNativeTimeInSeconds(nativePolicy->UpgradeDomainTimeoutInSeconds);

            if (nativePolicy->Reserved != IntPtr.Zero)
            {
                var ex1Ptr = (NativeTypes.FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_EX1*)nativePolicy->Reserved;
                managedPolicy.HealthCheckStableDuration = FromNativeTimeInSeconds(ex1Ptr->HealthCheckStableDurationInSeconds);
            }

            return managedPolicy;
        }

        internal virtual IntPtr ToNative(PinCollection pinCollection)
        {
            var nativePolicy = new NativeTypes.FABRIC_ROLLING_UPGRADE_MONITORING_POLICY();

            switch (this.FailureAction)
            {
                case UpgradeFailureAction.Rollback:
                case UpgradeFailureAction.Manual:
                    nativePolicy.FailureAction = (NativeTypes.FABRIC_MONITORED_UPGRADE_FAILURE_ACTION)this.FailureAction;
                    break;

                default:
                    // Allow this for modify upgrade
                    //
                    nativePolicy.FailureAction = (NativeTypes.FABRIC_MONITORED_UPGRADE_FAILURE_ACTION.FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_INVALID);
                    break;
            }

            nativePolicy.HealthCheckWaitDurationInSeconds = ToNativeTimeInSeconds(this.HealthCheckWaitDuration);
            nativePolicy.HealthCheckRetryTimeoutInSeconds = ToNativeTimeInSeconds(this.HealthCheckRetryTimeout);
            nativePolicy.UpgradeTimeoutInSeconds = ToNativeTimeInSeconds(this.UpgradeTimeout);
            nativePolicy.UpgradeDomainTimeoutInSeconds = ToNativeTimeInSeconds(this.UpgradeDomainTimeout);

            var ex1 = new NativeTypes.FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_EX1();
            ex1.HealthCheckStableDurationInSeconds = ToNativeTimeInSeconds(this.HealthCheckStableDuration);

            nativePolicy.Reserved = pinCollection.AddBlittable(ex1);

            return pinCollection.AddBlittable(nativePolicy);
        }

        private unsafe void LoadFromConfigHelper()
        {
            var nativeResult = NativeClient.FabricGetDefaultRollingUpgradeMonitoringPolicy();
            var nativePolicyPtr = nativeResult.get_Policy();

            ReleaseAssert.AssertIf(nativePolicyPtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativePolicyPtr"));

            var nativePolicy = (NativeTypes.FABRIC_ROLLING_UPGRADE_MONITORING_POLICY*)nativePolicyPtr;

            this.FailureAction = (UpgradeFailureAction)nativePolicy->FailureAction;
            this.HealthCheckWaitDuration = FromNativeTimeInSeconds(nativePolicy->HealthCheckWaitDurationInSeconds);
            this.HealthCheckRetryTimeout = FromNativeTimeInSeconds(nativePolicy->HealthCheckRetryTimeoutInSeconds);
            this.UpgradeTimeout = FromNativeTimeInSeconds(nativePolicy->UpgradeTimeoutInSeconds);
            this.UpgradeDomainTimeout = FromNativeTimeInSeconds(nativePolicy->UpgradeDomainTimeoutInSeconds);

            if (nativePolicy->Reserved != IntPtr.Zero)
            {
                var ex1Ptr = (NativeTypes.FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_EX1*)nativePolicy->Reserved;
                this.HealthCheckStableDuration = FromNativeTimeInSeconds(ex1Ptr->HealthCheckStableDurationInSeconds);
            }

            GC.KeepAlive(nativeResult);
        }

        private static uint ToNativeTimeInSeconds(TimeSpan time)
        {
            if (time == TimeSpan.MaxValue)
            {
                return NativeTypes.FABRIC_INFINITE_DURATION;
            }

            return (uint)time.TotalSeconds;
        }

        private static TimeSpan FromNativeTimeInSeconds(uint time)
        {
            if (time == NativeTypes.FABRIC_INFINITE_DURATION)
            {
                return TimeSpan.MaxValue;
            }

            return TimeSpan.FromSeconds(time);
        }
    }
}
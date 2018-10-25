// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Common.Serialization;
    using System.Fabric.Health;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Describes the behavior to use when performing an application upgrade.</para>
    /// </summary>
    public sealed class MonitoredRollingApplicationUpgradePolicyDescription : MonitoredRollingUpgradePolicyDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.MonitoredRollingApplicationUpgradePolicyDescription" /> class.</para>
        /// </summary>
        public MonitoredRollingApplicationUpgradePolicyDescription()
            : base()
        {
            this.HealthPolicy = null;
        }

        /// <summary>
        /// <para>Gets or sets the health policy to use when performing health checks against an upgrading application.</para>
        /// </summary>
        /// <value>
        /// <para>The health policy to use when performing health checks against an upgrading application.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.ApplicationHealthPolicy)]
        public ApplicationHealthPolicy HealthPolicy { get; set; }

        new internal static unsafe MonitoredRollingApplicationUpgradePolicyDescription FromNative(IntPtr policyPtr)
        {
            if (policyPtr == IntPtr.Zero) { return null; }

            var castedPtr = (NativeTypes.FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION*)policyPtr;

            var monitoringPolicy = new MonitoredRollingApplicationUpgradePolicyDescription();

            if (castedPtr->Reserved != IntPtr.Zero)
            {
                var castedEx1Ptr = (NativeTypes.FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1*)castedPtr->Reserved;
                if (castedEx1Ptr->MonitoringPolicy != IntPtr.Zero)
                {
                    monitoringPolicy.MonitoringPolicy = RollingUpgradeMonitoringPolicy.FromNative(castedEx1Ptr->MonitoringPolicy);
                }

                if (castedEx1Ptr->HealthPolicy != IntPtr.Zero)
                {
                    monitoringPolicy.HealthPolicy = ApplicationHealthPolicy.FromNative(castedEx1Ptr->HealthPolicy);
                }
            }

            monitoringPolicy.FromNativeHelper(policyPtr);

            return monitoringPolicy;
        }

        internal override IntPtr HealthPolicy_ToNative(PinCollection pinCollection)
        {
            if (this.HealthPolicy != null)
            {
                return this.HealthPolicy.ToNative(pinCollection);
            }
            else
            {
                return IntPtr.Zero;
            }
        }
    }

}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// <para>Description of the rolling upgrade policy.</para>
    /// </summary>
    public class RollingUpgradePolicyDescription : UpgradePolicyDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.RollingUpgradePolicyDescription" /> class.</para>
        /// </summary>
        public RollingUpgradePolicyDescription()
            : base(UpgradeKind.Rolling)
        {
            this.UpgradeMode = RollingUpgradeMode.Invalid;
            this.ForceRestart = false;
            this.UpgradeReplicaSetCheckTimeout = GetMaxReplicaSetCheckTimeoutForNative();
        }

        /// <summary>
        /// <para>Type: <see cref="System.Fabric.RollingUpgradeMode" />Specifies the types of upgrade (<see cref="System.Fabric.RollingUpgradeMode" />) to be 
        /// used for upgrading the application instance. </para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.RollingUpgradeMode" />.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.RollingUpgradeMode)]
        public RollingUpgradeMode UpgradeMode { get; set; }

        /// <summary>
        /// <para>Specifies if the service host should be restarted even when there are no code package changes as part of the upgrade. Set this flag to 
        /// true if the service cannot dynamically accept config or data package changes.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Boolean" />.</para>
        /// </value>
        public bool ForceRestart { get; set; }

        /// <summary>
        /// <para>Specifies the duration Service Fabric should wait before upgrading the services of an application instance
        /// in an upgrade domain if the services do not have quorum.</para>
        /// </summary>
        /// <value>
        /// <para>The duration Service Fabric should wait before upgrading the services of an application instance
        /// in an upgrade domain if the services do not have quorum.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.UpgradeReplicaSetCheckTimeoutInSeconds)]
        public TimeSpan UpgradeReplicaSetCheckTimeout { get; set; }

        internal override void Validate()
        {
            if (this.UpgradeMode == RollingUpgradeMode.Invalid)
            {
                throw new ArgumentException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_ArgumentInvalid_Formatted,
                        "RollingUpgradePolicyDescription.UpgradeMode",
                        "Validate"));
            }

            if (this.UpgradeMode == RollingUpgradeMode.Monitored && !(this is MonitoredRollingUpgradePolicyDescription))
            {
                throw new ArgumentException("Must use MonitoredRollingUpgradePolicyDescription object for monitored upgrades");
            }
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativePolicyDescription = new NativeTypes.FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION();

            switch (this.UpgradeMode)
            {
                case RollingUpgradeMode.UnmonitoredAuto:
                case RollingUpgradeMode.UnmonitoredManual:
                case RollingUpgradeMode.Monitored:
                    nativePolicyDescription.RollingUpgradeMode = (NativeTypes.FABRIC_ROLLING_UPGRADE_MODE)this.UpgradeMode;
                    break;

                default:
                    throw new ArgumentException("description.UpgradeMode");
            }

            nativePolicyDescription.ForceRestart = NativeTypes.ToBOOLEAN(this.ForceRestart);
            nativePolicyDescription.UpgradeReplicaSetCheckTimeoutInSeconds = (uint)(this.UpgradeReplicaSetCheckTimeout == TimeSpan.MaxValue
                ? GetMaxReplicaSetCheckTimeoutForNative()
                : this.UpgradeReplicaSetCheckTimeout).TotalSeconds;
            nativePolicyDescription.Reserved = this.Reserved_ToNative(pinCollection);

            return pinCollection.AddBlittable(nativePolicyDescription);
        }

        internal static unsafe RollingUpgradePolicyDescription FromNative(IntPtr policyPtr)
        {
            if (policyPtr == IntPtr.Zero) { return null; }

            var description = new RollingUpgradePolicyDescription();
            description.FromNativeHelper(policyPtr);

            return description;
        }

        internal unsafe void FromNativeHelper(IntPtr policyPtr)
        {
            if (policyPtr == IntPtr.Zero) { return; }

            var castedPtr = (NativeTypes.FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION*)policyPtr;

            switch (castedPtr->RollingUpgradeMode)
            {
                case NativeTypes.FABRIC_ROLLING_UPGRADE_MODE.FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO:
                case NativeTypes.FABRIC_ROLLING_UPGRADE_MODE.FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL:
                case NativeTypes.FABRIC_ROLLING_UPGRADE_MODE.FABRIC_ROLLING_UPGRADE_MODE_MONITORED:
                    this.UpgradeMode = (RollingUpgradeMode)castedPtr->RollingUpgradeMode;
                    break;

                default:
                    this.UpgradeMode = RollingUpgradeMode.Invalid;
                    break;
            }

            this.ForceRestart = NativeTypes.FromBOOLEAN(castedPtr->ForceRestart);
            this.UpgradeReplicaSetCheckTimeout = TimeSpan.FromSeconds(castedPtr->UpgradeReplicaSetCheckTimeoutInSeconds);
        }

        internal virtual IntPtr Reserved_ToNative(PinCollection pinCollection)
        {
            return IntPtr.Zero;
        }

        static private TimeSpan GetMaxReplicaSetCheckTimeoutForNative()
        {
            return TimeSpan.FromSeconds(uint.MaxValue);
        }
    }
}
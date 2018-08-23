// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Specifies the behavior to use when performing a monitored application or cluster upgrade.</para>
    /// </summary>
    [KnownType(typeof(MonitoredRollingApplicationUpgradePolicyDescription))]
    [KnownType(typeof(MonitoredRollingFabricUpgradePolicyDescription))]
    public abstract class MonitoredRollingUpgradePolicyDescription : RollingUpgradePolicyDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.MonitoredRollingUpgradePolicyDescription" /> class.</para>
        /// </summary>
        protected MonitoredRollingUpgradePolicyDescription()
            : base()
        {
            this.UpgradeMode = RollingUpgradeMode.Monitored;
            this.MonitoringPolicy = null;
        }

        /// <summary>
        /// <para>Gets or sets the monitoring policy to use when performing an upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The monitoring policy to use when performing an upgrade.</para>
        /// </value>
        public RollingUpgradeMonitoringPolicy MonitoringPolicy { get; set; }

        internal override void Validate()
        {
            Requires.Argument<RollingUpgradeMonitoringPolicy>("MonitoringPolicy", this.MonitoringPolicy).NotNull();

            this.MonitoringPolicy.Validate();

            base.Validate();
        }

        internal UInt32 FromUpdateDescription(UpgradeUpdateDescriptionBase updateDescription)
        {
            UInt32 flags = 0;

            var monitoringPolicy = new RollingUpgradeMonitoringPolicy();

            if (updateDescription.UpgradeMode.HasValue)
            {
                flags |= (UInt32)NativeTypes.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_MODE;
                this.UpgradeMode = updateDescription.UpgradeMode.Value;
            }

            if (updateDescription.ForceRestart.HasValue)
            {
                flags |= (UInt32)NativeTypes.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_FORCE_RESTART;
                this.ForceRestart = updateDescription.ForceRestart.Value;
            }

            if (updateDescription.UpgradeReplicaSetCheckTimeout.HasValue)
            {
                flags |= (UInt32)NativeTypes.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_REPLICA_SET_CHECK_TIMEOUT;
                this.UpgradeReplicaSetCheckTimeout = updateDescription.UpgradeReplicaSetCheckTimeout.Value;
            }

            if (updateDescription.FailureAction.HasValue)
            {
                flags |= (UInt32)NativeTypes.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_FAILURE_ACTION;
                monitoringPolicy.FailureAction = updateDescription.FailureAction.Value;
            }

            if (updateDescription.HealthCheckWaitDuration.HasValue)
            {
                flags |= (UInt32)NativeTypes.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_CHECK_WAIT;
                monitoringPolicy.HealthCheckWaitDuration = updateDescription.HealthCheckWaitDuration.Value;
            }

            if (updateDescription.HealthCheckStableDuration.HasValue)
            {
                flags |= (UInt32)NativeTypes.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_CHECK_STABLE;
                monitoringPolicy.HealthCheckStableDuration = updateDescription.HealthCheckStableDuration.Value;
            }

            if (updateDescription.HealthCheckRetryTimeout.HasValue)
            {
                flags |= (UInt32)NativeTypes.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_CHECK_RETRY;
                monitoringPolicy.HealthCheckRetryTimeout = updateDescription.HealthCheckRetryTimeout.Value;
            }

            if (updateDescription.UpgradeTimeout.HasValue)
            {
                flags |= (UInt32)NativeTypes.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_TIMEOUT;
                monitoringPolicy.UpgradeTimeout = updateDescription.UpgradeTimeout.Value;
            }

            if (updateDescription.UpgradeDomainTimeout.HasValue)
            {
                flags |= (UInt32)NativeTypes.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_DOMAIN_TIMEOUT;
                monitoringPolicy.UpgradeDomainTimeout = updateDescription.UpgradeDomainTimeout.Value;
            }

            // Flags will determine whether any values are actually used
            //
            this.MonitoringPolicy = monitoringPolicy;

            return flags;
        }

        internal override IntPtr Reserved_ToNative(PinCollection pinCollection)
        {
            var nativeEx1 = new NativeTypes.FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1();

            if (this.MonitoringPolicy != null)
            {
                nativeEx1.MonitoringPolicy = this.MonitoringPolicy.ToNative(pinCollection);
            }

            nativeEx1.HealthPolicy = this.HealthPolicy_ToNative(pinCollection);

            nativeEx1.Reserved = this.ReservedEx_ToNative(pinCollection);
            
            return pinCollection.AddBlittable(nativeEx1);
        }

        internal abstract IntPtr HealthPolicy_ToNative(PinCollection pinCollection);

        internal virtual IntPtr ReservedEx_ToNative(PinCollection pinCollection)
        {
            return IntPtr.Zero;
        }
    }
}
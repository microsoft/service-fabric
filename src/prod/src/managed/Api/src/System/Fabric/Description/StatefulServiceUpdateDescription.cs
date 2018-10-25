// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>
    /// Describes changes to the <see cref="System.Fabric.Description.StatefulServiceDescription" /> of a running service performed via 
    /// <see cref="System.Fabric.FabricClient.ServiceManagementClient.UpdateServiceAsync(System.Uri,System.Fabric.Description.ServiceUpdateDescription)" />.
    /// </para>
    /// </summary>
    public sealed class StatefulServiceUpdateDescription : ServiceUpdateDescription
    {
        private TimeSpan? replicaRestartWaitDuration;
        private TimeSpan? quorumLossWaitDuration;
        private TimeSpan? standByReplicaKeepDuration;

        /// <summary>
        /// <para>
        /// Initializes a new instance of the <see cref="System.Fabric.Description.StatefulServiceUpdateDescription" /> class with no changes specified.
        /// The relevant properties must be explicitly set to specify changes.
        /// </para>
        /// </summary>
        public StatefulServiceUpdateDescription()
            : base(ServiceDescriptionKind.Stateful)
        {
        }

        /// <summary>
        /// <para>Gets or sets the target replica set size.</para>
        /// </summary>
        /// <value>
        /// <para>The target replica set size.</para>
        /// </value>
        /// <remarks>
        /// <para>See <see cref="System.Fabric.Description.StatefulServiceDescription.TargetReplicaSetSize" />.</para>
        /// </remarks>
        public int? TargetReplicaSetSize { get; set; }

        /// <summary>
        /// <para>Gets or sets the minimum replica set size.</para>
        /// </summary>
        /// <value>
        /// <para>The minimum replica set size.</para>
        /// </value>
        /// <remarks>
        /// <para>See <see cref="System.Fabric.Description.StatefulServiceDescription.MinReplicaSetSize" /></para>
        /// </remarks>
        public int? MinReplicaSetSize { get; set; }

        /// <summary>
        /// <para>Gets or sets the replica restart wait duration.</para>
        /// </summary>
        /// <value>
        /// <para>The replica restart wait duration.</para>
        /// </value>
        /// <remarks>
        /// <para>See <see cref="System.Fabric.Description.StatefulServiceDescription.ReplicaRestartWaitDuration" /></para>
        /// </remarks>
        public TimeSpan? ReplicaRestartWaitDuration
        {
            get { return this.replicaRestartWaitDuration; }
            
            set
            {
                if (value < TimeSpan.Zero)
                {
                    throw new ArgumentOutOfRangeException();
                }

                this.replicaRestartWaitDuration = value;
            }
        }

        /// <summary>
        /// <para>Gets or sets the quorum loss wait duration.</para>
        /// </summary>
        /// <value>
        /// <para>The quorum loss wait duration.</para>
        /// </value>
        /// <remarks>
        /// <para>See <see cref="System.Fabric.Description.StatefulServiceDescription.QuorumLossWaitDuration" /></para>
        /// </remarks>
        public TimeSpan? QuorumLossWaitDuration
        {
            get { return this.quorumLossWaitDuration; }

            set
            {
                if (value < TimeSpan.Zero)
                {
                    throw new ArgumentOutOfRangeException();
                }

                this.quorumLossWaitDuration = value;
            }
        }

        /// <summary>
        /// <para>Gets or sets the standby replica keep duration.</para>
        /// </summary>
        /// <value>
        /// <para>The standby replica keep duration.</para>
        /// </value>
        /// <remarks>
        /// <para>See <see cref="System.Fabric.Description.StatefulServiceUpdateDescription.StandByReplicaKeepDuration" /></para>
        /// </remarks>
        public TimeSpan? StandByReplicaKeepDuration
        {
            get { return this.standByReplicaKeepDuration; }

            set
            {
                if (value < TimeSpan.Zero)
                {
                    throw new ArgumentOutOfRangeException();
                }

                this.standByReplicaKeepDuration = value;
            }
        }

        internal override IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND kind)
        {
            var nativeUpdateDescription = new NativeTypes.FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION[1];
            var nativeUpdateDescriptionEx1 = new NativeTypes.FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_EX1[1];
            var nativeUpdateDescriptionEx2 = new NativeTypes.FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_EX2[1];
            var nativeUpdateDescriptionEx3 = new NativeTypes.FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_EX3[1];
            var nativeUpdateDescriptionEx4 = new NativeTypes.FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_EX4[1];
            var nativeUpdateDescriptionEx5 = new NativeTypes.FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_EX5[1];

            var flags = NativeTypes.FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATEFUL_SERVICE_NONE;

            if (TargetReplicaSetSize.HasValue)
            {
                flags |= NativeTypes.FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATEFUL_SERVICE_TARGET_REPLICA_SET_SIZE;
                nativeUpdateDescription[0].TargetReplicaSetSize = this.TargetReplicaSetSize.Value;
            }

            if (ReplicaRestartWaitDuration.HasValue)
            {
                flags |= NativeTypes.FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATEFUL_SERVICE_REPLICA_RESTART_WAIT_DURATION;
                nativeUpdateDescription[0].ReplicaRestartWaitDurationSeconds = (uint)this.ReplicaRestartWaitDuration.Value.TotalSeconds;
            }

            if (QuorumLossWaitDuration.HasValue)
            {
                flags |= NativeTypes.FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATEFUL_SERVICE_QUORUM_LOSS_WAIT_DURATION;
                nativeUpdateDescription[0].QuorumLossWaitDurationSeconds = (uint)this.QuorumLossWaitDuration.Value.TotalSeconds;
            }

            if (StandByReplicaKeepDuration.HasValue)
            {
                flags |= NativeTypes.FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATEFUL_SERVICE_STANDBY_REPLICA_KEEP_DURATION;
                nativeUpdateDescriptionEx1[0].StandByReplicaKeepDurationSeconds = (uint)this.StandByReplicaKeepDuration.Value.TotalSeconds;
            }

            if (MinReplicaSetSize.HasValue)
            {
                flags |= NativeTypes.FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATEFUL_SERVICE_MIN_REPLICA_SET_SIZE;
                nativeUpdateDescriptionEx2[0].MinReplicaSetSize = this.MinReplicaSetSize.Value;
            }

            if (this.PlacementConstraints != null)
            {
                flags |= NativeTypes.FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATEFUL_SERVICE_PLACEMENT_CONSTRAINTS;
                nativeUpdateDescriptionEx3[0].PlacementConstraints = pin.AddBlittable(this.PlacementConstraints);
            }

            if (this.Correlations != null)
            {
                var correlations = this.ToNativeCorrelations(pin);

                if (correlations != null)
                {
                    flags |= NativeTypes.FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATEFUL_SERVICE_CORRELATIONS;
                    nativeUpdateDescriptionEx3[0].CorrelationCount = correlations.Item1;
                    nativeUpdateDescriptionEx3[0].Correlations = correlations.Item2;
                }
            }

            if (this.Metrics != null)
            {
                var metrics = NativeTypes.ToNativeLoadMetrics(pin, this.Metrics);
                if (metrics != null)
                {
                    flags |= NativeTypes.FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATEFUL_SERVICE_METRICS;
                    nativeUpdateDescriptionEx3[0].MetricCount = metrics.Item1;
                    nativeUpdateDescriptionEx3[0].Metrics = metrics.Item2;
                }
            }

            if (this.PlacementPolicies != null)
            {
                flags |= NativeTypes.FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATEFUL_SERVICE_POLICY_LIST;
                var policies = this.ToNativePolicies(pin);
                var pList = new NativeTypes.FABRIC_SERVICE_PLACEMENT_POLICY_LIST[1];

                pList[0].PolicyCount = policies.Item1;
                pList[0].Policies = policies.Item2;

                nativeUpdateDescriptionEx3[0].PolicyList = pin.AddBlittable(pList);
            }

            if (this.DefaultMoveCost.HasValue)
            {
                flags |= NativeTypes.FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATEFUL_SERVICE_MOVE_COST;
                nativeUpdateDescriptionEx4[0].DefaultMoveCost = (NativeTypes.FABRIC_MOVE_COST)DefaultMoveCost.Value;
            }

            if (this.RepartitionDescription != null)
            {
                NativeTypes.FABRIC_SERVICE_PARTITION_KIND partitionKind;
                nativeUpdateDescriptionEx5[0].RepartitionDescription = this.RepartitionDescription.ToNative(pin, out partitionKind);
                nativeUpdateDescriptionEx5[0].RepartitionKind = partitionKind;
            }

            if (this.ScalingPolicies != null)
            {
                flags |= NativeTypes.FABRIC_STATEFUL_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATEFUL_SERVICE_SCALING_POLICY;
                var scalingPolicies = this.ToNativeScalingPolicies(pin);
                nativeUpdateDescriptionEx5[0].ScalingPolicyCount = scalingPolicies.Item1;
                nativeUpdateDescriptionEx5[0].ServiceScalingPolicies = scalingPolicies.Item2;
            }

            nativeUpdateDescription[0].Flags = (uint)flags;
            nativeUpdateDescriptionEx4[0].Reserved = pin.AddBlittable(nativeUpdateDescriptionEx5);
            nativeUpdateDescriptionEx3[0].Reserved = pin.AddBlittable(nativeUpdateDescriptionEx4);
            nativeUpdateDescriptionEx2[0].Reserved = pin.AddBlittable(nativeUpdateDescriptionEx3);
            nativeUpdateDescriptionEx1[0].Reserved = pin.AddBlittable(nativeUpdateDescriptionEx2);
            nativeUpdateDescription[0].Reserved = pin.AddBlittable(nativeUpdateDescriptionEx1);

            kind = NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND.FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
            return pin.AddBlittable(nativeUpdateDescription);
        }
    }
}
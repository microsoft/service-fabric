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
    /// Describes changes to the <see cref="System.Fabric.Description.StatelessServiceDescription" /> of a running service performed via 
    /// <see cref="System.Fabric.FabricClient.ServiceManagementClient.UpdateServiceAsync(System.Uri,System.Fabric.Description.ServiceUpdateDescription)" />.
    /// </para>
    /// </summary>
    public sealed class StatelessServiceUpdateDescription : ServiceUpdateDescription
    {
        /// <summary>
        /// <para>
        /// Initializes a new instance of the <see cref="System.Fabric.Description.StatelessServiceUpdateDescription" /> class with no changes specified.
        /// The relevant properties must be explicitly set to specify changes.
        /// </para>
        /// </summary>
        public StatelessServiceUpdateDescription()
            : base(ServiceDescriptionKind.Stateless)
        {
        }

        /// <summary>
        /// <para>Gets or sets the instance count</para>
        /// </summary>
        /// <value>
        /// <para>The instance count</para>
        /// </value>
        /// <remarks>
        /// <para>See <see cref="System.Fabric.Description.StatelessServiceDescription.InstanceCount" /></para>
        /// </remarks>
        public int? InstanceCount
        {
            get;
            set;
        }

        internal override IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND kind)
        {
            var nativeUpdateDescription = new NativeTypes.FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION[1];
            var nativeUpdateDescriptionEx1 = new NativeTypes.FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION_EX1[1];
            var nativeUpdateDescriptionEx2 = new NativeTypes.FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION_EX2[1];
            var nativeUpdateDescriptionEx3 = new NativeTypes.FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION_EX3[1];

            var flags = NativeTypes.FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATELESS_SERVICE_NONE;

            if (InstanceCount.HasValue)
            {
                flags |= NativeTypes.FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATELESS_SERVICE_INSTANCE_COUNT;
                nativeUpdateDescription[0].InstanceCount = this.InstanceCount.Value;
            }

            if (this.PlacementConstraints != null)
            {
                flags |= NativeTypes.FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATELESS_SERVICE_PLACEMENT_CONSTRAINTS;
                nativeUpdateDescriptionEx1[0].PlacementConstraints = pin.AddBlittable(this.PlacementConstraints);
            }

            if (this.Correlations != null)
            {
                var correlations = this.ToNativeCorrelations(pin);
                if (correlations != null)
                {
                    flags |= NativeTypes.FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATELESS_SERVICE_CORRELATIONS;
                    nativeUpdateDescriptionEx1[0].CorrelationCount = correlations.Item1;
                    nativeUpdateDescriptionEx1[0].Correlations = correlations.Item2;
                }
            }

            if (this.Metrics != null)
            {
                var metrics = NativeTypes.ToNativeLoadMetrics(pin, this.Metrics);
                if (metrics != null)
                {
                    flags |= NativeTypes.FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATELESS_SERVICE_METRICS;
                    nativeUpdateDescriptionEx1[0].MetricCount = metrics.Item1;
                    nativeUpdateDescriptionEx1[0].Metrics = metrics.Item2;
                }
            }

            if (this.PlacementPolicies != null)
            {
                flags |= NativeTypes.FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATELESS_SERVICE_POLICY_LIST;
                var policies = this.ToNativePolicies(pin);
                var pList = new NativeTypes.FABRIC_SERVICE_PLACEMENT_POLICY_LIST[1];

                pList[0].PolicyCount = policies.Item1;
                pList[0].Policies = policies.Item2;

                nativeUpdateDescriptionEx1[0].PolicyList = pin.AddBlittable(pList);
            }

            if (this.DefaultMoveCost.HasValue)
            {
                flags |= NativeTypes.FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATELESS_SERVICE_MOVE_COST;
                nativeUpdateDescriptionEx2[0].DefaultMoveCost = (NativeTypes.FABRIC_MOVE_COST)DefaultMoveCost.Value;
            }

            if (this.RepartitionDescription != null)
            {
                NativeTypes.FABRIC_SERVICE_PARTITION_KIND partitionKind;
                nativeUpdateDescriptionEx3[0].RepartitionDescription = this.RepartitionDescription.ToNative(pin, out partitionKind);
                nativeUpdateDescriptionEx3[0].RepartitionKind = partitionKind;
            }

            if (this.ScalingPolicies != null)
            {
                flags |= NativeTypes.FABRIC_STATELESS_SERVICE_UPDATE_DESCRIPTION_FLAGS.FABRIC_STATELESS_SERVICE_SCALING_POLICY;
                var scalingPolicies = this.ToNativeScalingPolicies(pin);
                nativeUpdateDescriptionEx3[0].ScalingPolicyCount = scalingPolicies.Item1;
                nativeUpdateDescriptionEx3[0].ServiceScalingPolicies = scalingPolicies.Item2;
            }

            nativeUpdateDescription[0].Flags = (uint)flags;

            nativeUpdateDescriptionEx2[0].Reserved = pin.AddBlittable(nativeUpdateDescriptionEx3);
            nativeUpdateDescriptionEx1[0].Reserved = pin.AddBlittable(nativeUpdateDescriptionEx2);
            nativeUpdateDescription[0].Reserved = pin.AddBlittable(nativeUpdateDescriptionEx1);

            kind = NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND.FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS;
            return pin.AddBlittable(nativeUpdateDescription);
        }
    }
}
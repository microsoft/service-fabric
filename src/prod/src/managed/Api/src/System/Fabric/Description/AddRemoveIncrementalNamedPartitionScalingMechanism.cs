// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using Common.Serialization;
    using System;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>
    /// Represents a scaling mechanism for adding or removing named partitions of a service. 
    /// When this mechanism is used there will be new named partitions added or removed from this service.
    /// The expected schema of named partitions is "0","1",..."N-1" when N partitions are needed.
    /// Should be used with <see cref="System.Fabric.Description.AverageServiceLoadScalingTrigger" />.
    /// </para>
    /// </summary>
    public class AddRemoveIncrementalNamedPartitionScalingMechanism: ScalingMechanismDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.AddRemoveIncrementalNamedPartitionScalingMechanism" /> class of a particular kind.</para>
        /// </summary>
        public AddRemoveIncrementalNamedPartitionScalingMechanism()
            :base(ScalingMechanismKind.AddRemoveIncrementalNamedPartition)
        {
        }

        internal AddRemoveIncrementalNamedPartitionScalingMechanism(AddRemoveIncrementalNamedPartitionScalingMechanism other)
            :base(ScalingMechanismKind.AddRemoveIncrementalNamedPartition)
        {
            MinPartitionCount = other.MinPartitionCount;
            MaxPartitionCount = other.MaxPartitionCount;
            ScaleIncrement = other.ScaleIncrement;
        }
 
        /// <summary>
        /// <para>Gets or sets the minimum number of partitions which the service should have according to this scaling policy. </para>
        /// </summary>
        /// <value>
        /// <para>The minimum number of instances. </para>
        /// </value>
        public int MinPartitionCount { get; set; }

        /// <summary>
        /// <para>Gets or sets the maximum number of partitions which the service should have according to this scaling policy.</para>
        /// </summary>
        /// <value>
        /// <para>The maximum number of instances. </para>
        /// </value>
        public int MaxPartitionCount { get; set; }

        /// <summary>
        /// <para>Gets or sets the amount of partitions that should be added/removed when scaling is performed. </para>
        /// </summary>
        /// <value>
        /// <para>The number of instances by which to perform the upshift/downshift whenever a scaling operation is triggered. </para>
        /// </value>
        public int ScaleIncrement { get; set; }


        internal new static unsafe AddRemoveIncrementalNamedPartitionScalingMechanism CreateFromNative(NativeTypes.FABRIC_SCALING_MECHANISM policy)
        {
            if (policy.ScalingMechanismDescription != IntPtr.Zero)
            {
                var AddRemoveIncrementalNamedPartition = new AddRemoveIncrementalNamedPartitionScalingMechanism();
                var nativeDescription = (NativeTypes.FABRIC_SCALING_MECHANISM_ADD_REMOVE_INCREMENTAL_NAMED_PARTITION*)policy.ScalingMechanismDescription;

                AddRemoveIncrementalNamedPartition.ScaleIncrement = nativeDescription->ScaleIncrement;
                AddRemoveIncrementalNamedPartition.MinPartitionCount = nativeDescription->MinimumPartitionCount;
                AddRemoveIncrementalNamedPartition.MaxPartitionCount = nativeDescription->MaximumPartitionCount;

                return AddRemoveIncrementalNamedPartition;
            }

            return null;
        }

        /// <summary>
        /// <para> 
        /// Returns a string representation of the AddRemoveIncrementalNamedPartitionScalingMechanism.
        /// </para>
        /// </summary>
        /// <returns>
        /// <para>A string representing the AddRemoveIncrementalNamedPartitionScalingMechanism object.</para>
        /// </returns>
        public override string ToString()
        {
            Text.StringBuilder sb = new Text.StringBuilder();

            sb.AppendFormat("Kind:{0} MinPartitionCount:{1},MaxPartitionCount:{2},ScaleIncrement:{3}", 
                Kind, MinPartitionCount, MaxPartitionCount, ScaleIncrement);

            return sb.ToString();
        }

        internal override void ToNative(PinCollection pinCollection, ref NativeTypes.FABRIC_SCALING_MECHANISM scalingMechanism)
        {
            scalingMechanism.ScalingMechanismKind = NativeTypes.FABRIC_SCALING_MECHANISM_KIND.FABRIC_SCALING_MECHANISM_KIND_ADD_REMOVE_INCREMENTAL_NAMED_PARTITION;
            var AddRemoveIncrementalNamedPartition = new NativeTypes.FABRIC_SCALING_MECHANISM_ADD_REMOVE_INCREMENTAL_NAMED_PARTITION();

            AddRemoveIncrementalNamedPartition.MinimumPartitionCount = MinPartitionCount;
            AddRemoveIncrementalNamedPartition.MaximumPartitionCount = MaxPartitionCount;
            AddRemoveIncrementalNamedPartition.ScaleIncrement = ScaleIncrement;

            scalingMechanism.ScalingMechanismDescription = pinCollection.AddBlittable(AddRemoveIncrementalNamedPartition);
        }
    }

}
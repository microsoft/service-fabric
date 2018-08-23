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
    /// Represents a scaling mechanism for adding or removing instances of stateless service partition. 
    /// When this mechanism is used this will affect all partitions of a service and do independent scaling of each of them.
    /// Should be used with <see cref="System.Fabric.Description.AveragePartitionLoadScalingTrigger" />.
    /// </para>
    /// </summary>
    public class PartitionInstanceCountScaleMechanism: ScalingMechanismDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.PartitionInstanceCountScaleMechanism" /> class of a particular kind.</para>
        /// </summary>
        public PartitionInstanceCountScaleMechanism()
            :base(ScalingMechanismKind.ScalePartitionInstanceCount)
        {
        }

        internal PartitionInstanceCountScaleMechanism(PartitionInstanceCountScaleMechanism other)
            :base(ScalingMechanismKind.ScalePartitionInstanceCount)
        {
            MinInstanceCount = other.MinInstanceCount;
            MaxInstanceCount = other.MaxInstanceCount;
            ScaleIncrement = other.ScaleIncrement;
        }
 
        /// <summary>
        /// <para>Gets or sets the minimum number of instances which the service should have according to this scaling policy. </para>
        /// </summary>
        /// <value>
        /// <para>The minimum number of instances. </para>
        /// </value>
        public int MinInstanceCount { get; set; }

        /// <summary>
        /// <para>Gets or sets the maximum number of instances which the service should have according to this scaling policy. </para>
        /// </summary>
        /// <value>
        /// <para>The maximum number of instances. </para>
        /// </value>
        public int MaxInstanceCount { get; set; }

        /// <summary>
        /// <para>Gets or sets the amount of instances that should be added/removed when scaling is performed. </para>
        /// </summary>
        /// <value>
        /// <para>The number of instances by wich to perform the upshift/downshift. </para>
        /// </value>
        public int ScaleIncrement { get; set; }


        internal new static unsafe PartitionInstanceCountScaleMechanism CreateFromNative(NativeTypes.FABRIC_SCALING_MECHANISM policy)
        {
            if (policy.ScalingMechanismDescription != IntPtr.Zero)
            {
                var partitionInstanceCount = new PartitionInstanceCountScaleMechanism();
                var nativeDescription = (NativeTypes.FABRIC_SCALING_MECHANISM_PARTITION_INSTANCE_COUNT*)policy.ScalingMechanismDescription;

                partitionInstanceCount.ScaleIncrement = nativeDescription->ScaleIncrement;
                partitionInstanceCount.MinInstanceCount = nativeDescription->MinimumInstanceCount;
                partitionInstanceCount.MaxInstanceCount = nativeDescription->MaximumInstanceCount;

                return partitionInstanceCount;
            }

            return null;
        }

        /// <summary>
        /// <para> 
        /// Returns a string representation of the PartitionInstanceCountScaleMechanism.
        /// </para>
        /// </summary>
        /// <returns>
        /// <para>A string representing the PartitionInstanceCountScaleMechanism object.</para>
        /// </returns>
        public override string ToString()
        {
            Text.StringBuilder sb = new Text.StringBuilder();

            sb.AppendFormat("Kind:{0} MinInstanceCount:{1},MaxInstanceCount:{2},ScaleIncrement:{3}", 
                Kind, MinInstanceCount, MaxInstanceCount, ScaleIncrement);

            return sb.ToString();
        }

        internal override void ToNative(PinCollection pinCollection, ref NativeTypes.FABRIC_SCALING_MECHANISM scalingMechanism)
        {
            scalingMechanism.ScalingMechanismKind = NativeTypes.FABRIC_SCALING_MECHANISM_KIND.FABRIC_SCALING_MECHANISM_KIND_SCALE_PARTITION_INSTANCE_COUNT;
            var partitionInstanceCount = new NativeTypes.FABRIC_SCALING_MECHANISM_PARTITION_INSTANCE_COUNT();

            partitionInstanceCount.MinimumInstanceCount = MinInstanceCount;
            partitionInstanceCount.MaximumInstanceCount = MaxInstanceCount;
            partitionInstanceCount.ScaleIncrement = ScaleIncrement;

            scalingMechanism.ScalingMechanismDescription = pinCollection.AddBlittable(partitionInstanceCount);
        }
    }

}
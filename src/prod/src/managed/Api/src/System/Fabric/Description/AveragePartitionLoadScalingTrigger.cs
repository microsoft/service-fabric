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
    /// Represents a scaling policy related to an average load of a metric/resource of a partition.
    /// When this policy is used the service fabric platform will trigger scaling if the average load of a partition does not fit inside the limits specified for a particular metric.
    /// Should be used with <see cref="System.Fabric.Description.PartitionInstanceCountScaleMechanism" />.
    /// </para>
    /// </summary>
    public class AveragePartitionLoadScalingTrigger : ScalingTriggerDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.AveragePartitionLoadScalingTrigger" /> class of a particular kind.</para>
        /// </summary>
        public AveragePartitionLoadScalingTrigger()
            :base(ScalingTriggerKind.AveragePartitionLoadTrigger)
        {
        }

        internal AveragePartitionLoadScalingTrigger(AveragePartitionLoadScalingTrigger other)
            :base(ScalingTriggerKind.AveragePartitionLoadTrigger)
        {
            MetricName = other.MetricName;
            LowerLoadThreshold = other.LowerLoadThreshold;
            UpperLoadThreshold = other.UpperLoadThreshold;
            ScaleInterval = other.ScaleInterval;
        }

        /// <summary>
        /// <para>Gets or sets the name of the metric based on which scaling should be performed. </para>
        /// </summary>
        /// <value>
        /// <para>The name of metric which should be used for scaling operations. </para>
        /// </value>
        public string MetricName { get; set; }

        /// <summary>
        /// <para>Gets or sets the lower limit of the load value of a particular metric. If the average load over a time period is less than this value scale in is performed by the service fabric platform. </para>
        /// </summary>
        /// <value>
        /// <para>The lower limit of the load value of a metric. </para>
        /// </value>
        public Double LowerLoadThreshold { get; set; }

        /// <summary>
        /// <para>Gets or sets the upper limit of the load value of a particular metric. If the average load over a time period is greater than this value scale out is performed by the service fabric platform. </para>
        /// </summary>
        /// <value>
        /// <para>The upper limit of the load value of a metric. </para>
        /// </value>
        public Double UpperLoadThreshold { get; set; }

        /// <summary>
        /// <para>
        /// Gets or sets the time interval which should be considered when checking whether scaling should be performed.
        /// Every ScaleInterval there will be a check of average load based on which scaling will be triggered if necessary.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The time interval to be considered for scaling. </para>
        /// </value>
        [JsonCustomization(IsIgnored = true)]
        public TimeSpan? ScaleInterval
        {
            get;
            set;
        }

        [JsonCustomization(IsDefaultValueIgnored = true)]
        private int ScaleIntervalInSeconds
        {
            get { return (int)(this.ScaleInterval ?? TimeSpan.Zero).TotalSeconds; }
            set { this.ScaleInterval = TimeSpan.FromTicks(value * TimeSpan.TicksPerSecond); }
        }

        /// <summary>
        /// <para> 
        /// Returns a string representation of the AverageLoadScalingPolicyDescription.
        /// </para>
        /// </summary>
        /// <returns>
        /// <para>A string representing the AverageLoadScalingPolicyDescription object.</para>
        /// </returns>
        public override string ToString()
        {
            Text.StringBuilder sb = new Text.StringBuilder();

            sb.AppendFormat("Kind:{0},MetricName:{1},LowerLoadThreshold:{2},UpperLoadThreshold:{3},ScaleInterval:{4}", 
                Kind, MetricName, LowerLoadThreshold, UpperLoadThreshold, ScaleInterval);

            return sb.ToString();
        }

        internal override void ToNative(PinCollection pinCollection, ref NativeTypes.FABRIC_SCALING_TRIGGER scalingTrigger)
        {
            scalingTrigger.ScalingTriggerKind = NativeTypes.FABRIC_SCALING_TRIGGER_KIND.FABRIC_SCALING_TRIGGER_KIND_AVERAGE_PARTITION_LOAD;
            var partitionAverageLoad = new NativeTypes.FABRIC_SCALING_TRIGGER_AVERAGE_PARTITION_LOAD();

            partitionAverageLoad.MetricName = pinCollection.AddBlittable(MetricName);
            partitionAverageLoad.UpperLoadThreshold = UpperLoadThreshold;
            partitionAverageLoad.LowerLoadThreshold = LowerLoadThreshold;
            partitionAverageLoad.ScaleIntervalInSeconds = (uint)ScaleIntervalInSeconds;

            scalingTrigger.ScalingTriggerDescription = pinCollection.AddBlittable(partitionAverageLoad);
        }

        internal new static unsafe AveragePartitionLoadScalingTrigger CreateFromNative(NativeTypes.FABRIC_SCALING_TRIGGER policy)
        {
            if (policy.ScalingTriggerDescription != IntPtr.Zero)
            {
                var partitionAverageLoad = new AveragePartitionLoadScalingTrigger();
                var nativeDescription = (NativeTypes.FABRIC_SCALING_TRIGGER_AVERAGE_PARTITION_LOAD*)policy.ScalingTriggerDescription;

                partitionAverageLoad.MetricName = NativeTypes.FromNativeString(nativeDescription->MetricName);
                partitionAverageLoad.UpperLoadThreshold = nativeDescription->UpperLoadThreshold;
                partitionAverageLoad.LowerLoadThreshold = nativeDescription->LowerLoadThreshold;
                partitionAverageLoad.ScaleIntervalInSeconds = (int)nativeDescription->ScaleIntervalInSeconds;

                return partitionAverageLoad;
            }

            return null;
        }
    }

}
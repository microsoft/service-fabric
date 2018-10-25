// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using Common.Serialization;
    using System;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>
    /// Represents a scaling policy related to an average load of a metric/resource of a service.
    /// When this policy is used the service fabric platform will trigger scaling if the average load of a service does not fit inside the limits specified for a particular metric.
    /// Should be used with <see cref="System.Fabric.Description.AddRemoveIncrementalNamedPartitionScalingMechanism" />.
    /// </para>
    /// </summary>
    public class AverageServiceLoadScalingTrigger : ScalingTriggerDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.AverageServiceLoadScalingTrigger" /> class of a particular kind.</para>
        /// </summary>
        public AverageServiceLoadScalingTrigger()
            :base(ScalingTriggerKind.AverageServiceLoadTrigger)
        {
            UseOnlyPrimaryLoad = false;
        }

        internal AverageServiceLoadScalingTrigger(AverageServiceLoadScalingTrigger other)
            :base(ScalingTriggerKind.AverageServiceLoadTrigger)
        {
            MetricName = other.MetricName;
            LowerLoadThreshold = other.LowerLoadThreshold;
            UpperLoadThreshold = other.UpperLoadThreshold;
            ScaleInterval = other.ScaleInterval;
            UseOnlyPrimaryLoad = other.UseOnlyPrimaryLoad;
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
        /// <para>Gets or sets whether only the load of primary replica should be considered for scaling. </para>
        /// </summary>
        /// <value>
        /// <para>If set to true, then trigger will only consider the load of primary replicas of stateful service. If set to false, trigger will consider load of all replicas. This parameter cannot be set to true for stateless service. </para>
        /// </value>
        public Boolean UseOnlyPrimaryLoad { get; set; }

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

            sb.AppendFormat("Kind:{0},MetricName:{1},LowerLoadThreshold:{2},UpperLoadThreshold:{3},ScaleInterval:{4},UseOnlyPrimaryLoad:{5}", 
                Kind, MetricName, LowerLoadThreshold, UpperLoadThreshold, ScaleInterval, UseOnlyPrimaryLoad);

            return sb.ToString();
        }

        internal override void ToNative(PinCollection pinCollection, ref NativeTypes.FABRIC_SCALING_TRIGGER scalingTrigger)
        {
            scalingTrigger.ScalingTriggerKind = NativeTypes.FABRIC_SCALING_TRIGGER_KIND.FABRIC_SCALING_TRIGGER_KIND_AVERAGE_SERVICE_LOAD;
            var serviceAverageLoad = new NativeTypes.FABRIC_SCALING_TRIGGER_AVERAGE_SERVICE_LOAD();
            var serviceAverageLoadEx1 = new NativeTypes.FABRIC_SCALING_TRIGGER_AVERAGE_SERVICE_LOAD_EX1();

            serviceAverageLoad.MetricName = pinCollection.AddBlittable(MetricName);
            serviceAverageLoad.UpperLoadThreshold = UpperLoadThreshold;
            serviceAverageLoad.LowerLoadThreshold = LowerLoadThreshold;
            serviceAverageLoad.ScaleIntervalInSeconds = (uint)ScaleIntervalInSeconds;
            serviceAverageLoadEx1.UseOnlyPrimaryLoad = NativeTypes.ToBOOLEAN(UseOnlyPrimaryLoad);

            serviceAverageLoad.Reserved = pinCollection.AddBlittable(serviceAverageLoadEx1);

            scalingTrigger.ScalingTriggerDescription = pinCollection.AddBlittable(serviceAverageLoad);
        }

        internal new static unsafe AverageServiceLoadScalingTrigger CreateFromNative(NativeTypes.FABRIC_SCALING_TRIGGER policy)
        {
            if (policy.ScalingTriggerDescription != IntPtr.Zero)
            {
                var serviceAverageLoad = new AverageServiceLoadScalingTrigger();
                var nativeDescription = (NativeTypes.FABRIC_SCALING_TRIGGER_AVERAGE_SERVICE_LOAD*)policy.ScalingTriggerDescription;

                serviceAverageLoad.MetricName = NativeTypes.FromNativeString(nativeDescription->MetricName);
                serviceAverageLoad.UpperLoadThreshold = nativeDescription->UpperLoadThreshold;
                serviceAverageLoad.LowerLoadThreshold = nativeDescription->LowerLoadThreshold;
                serviceAverageLoad.ScaleIntervalInSeconds = (int)nativeDescription->ScaleIntervalInSeconds;

                if (nativeDescription->Reserved != IntPtr.Zero)
                {
                    NativeTypes.FABRIC_SCALING_TRIGGER_AVERAGE_SERVICE_LOAD_EX1* ex1 = (NativeTypes.FABRIC_SCALING_TRIGGER_AVERAGE_SERVICE_LOAD_EX1*)nativeDescription->Reserved;
                    if (ex1 == null)
                    {
                        throw new ArgumentException(StringResources.Error_UnknownReservedType);
                    }

                    serviceAverageLoad.UseOnlyPrimaryLoad = NativeTypes.FromBOOLEAN(ex1->UseOnlyPrimaryLoad);
                }

                return serviceAverageLoad;
            }

            return null;
        }
    }

}
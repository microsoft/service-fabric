// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// <para>Represents the filter for <see cref="System.Fabric.Health.ReplicaHealthState" /> objects.</para>
    /// </summary>
    /// <remarks>The filter can be used in <see cref="System.Fabric.Description.PartitionHealthQueryDescription"/>
    /// to specify which replica children should be returned as part of <see cref="System.Fabric.Health.PartitionHealth"/>.</remarks>
    public sealed class ReplicaHealthStatesFilter
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.ReplicaHealthStatesFilter" /> class.</para>
        /// </summary>
        public ReplicaHealthStatesFilter()
        {
        }

        /// <summary>
        /// Gets or sets the filter for the aggregated health state of the <see cref="System.Fabric.Health.ReplicaHealthState"/> entries in the collection. 
        /// </summary>
        /// <value>The filter for the aggregated health state of the <see cref="System.Fabric.Health.ReplicaHealthState"/> entries.</value>
        /// <remarks>The input collection is filtered to return only entries that respect the desired health state. 
        /// The filter represents a value obtained from members or bitwise combinations of members of <see cref="System.Fabric.Health.HealthStateFilter"/>.</remarks>
        public HealthStateFilter HealthStateFilterValue
        {
            get;
            set;
        }

        /// <summary>
        /// <para>DEPRECATED. Gets or sets the filter for the aggregated health state of the <see cref="System.Fabric.Health.ReplicaHealthState" /> entries in 
        /// the collection. Represents a value obtained from members or bitwise combinations of members of <see cref="System.Fabric.Health.HealthStateFilter" />.</para>
        /// </summary>
        /// <value>
        /// <para> The filter for the aggregated health state of the <see cref="System.Fabric.Health.ReplicaHealthState" /> entries in the 
        /// collection. The value comes from members or bitwise combination of members of <see cref="System.Fabric.Health.HealthStateFilter" />.</para>
        /// </value>
        /// <remarks>This property is obsolete. Use <see cref="System.Fabric.Health.ReplicaHealthStatesFilter.HealthStateFilterValue"/> instead.</remarks>
        [Obsolete("This property is obsolete. Use HealthStateFilterValue instead.")]
        public long HealthStateFilter
        {
            get { return (long)this.HealthStateFilterValue; }
            set { this.HealthStateFilterValue = (HealthStateFilter)value; }
        }

        /// <summary>
        /// Returns a string representation of the filter.
        /// </summary>
        /// <returns>A string representation of the filter.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "ReplicasFilter: {0}", this.HealthStateFilterValue);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeReplicaHealthStatesFilter = new NativeTypes.FABRIC_REPLICA_HEALTH_STATES_FILTER();

            nativeReplicaHealthStatesFilter.HealthStateFilter = (UInt32)this.HealthStateFilterValue;

            return pinCollection.AddBlittable(nativeReplicaHealthStatesFilter);
        }
    }
}

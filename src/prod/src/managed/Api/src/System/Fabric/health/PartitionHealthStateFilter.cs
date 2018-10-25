// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// Filter for <see cref="System.Fabric.Health.PartitionHealthState"/> objects.
    /// </summary>
    /// <remarks>The health state chunk queries can specify a list of partition filters to fine-grain
    /// select the partitions that should be included in the query result.
    /// Note that all the partitions are used to evaluate parents' aggregated health state, regardless of the filter passed in.</remarks>
    public sealed class PartitionHealthStateFilter
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.Health.PartitionHealthStateFilter"/> class.
        /// </summary>
        public PartitionHealthStateFilter()
        {
            this.PartitionIdFilter = Guid.Empty;
            this.ReplicaFilters = new List<ReplicaHealthStateFilter>();
        }

        /// <summary>
        /// Gets or sets filter for the aggregated health state of the <see cref="System.Fabric.Health.PartitionHealthState"/> entries in the collection. 
        /// </summary>
        /// <value>The filter for the aggregated health state of the <see cref="System.Fabric.Health.PartitionHealthState"/> entries in the collection.</value>
        /// <remarks>The health state filter value comes from members or bitwise combination of members of <see cref="System.Fabric.Health.HealthStateFilter"/>. 
        /// For a partition to match the filter, its aggregated health state must match the specified health state filter.</remarks>
        public HealthStateFilter HealthStateFilter
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the partition id filter.
        /// </summary>
        /// <value>The partition id filter.</value>
        /// <remarks>If not specified, all partitions that match the parent filters (if any) and the specified health state filter match the filter.
        /// Otherwise, the filter only applies to the partition identified by the partition id. All the other filter members, like health state filter and replicas filter, are applied to this partition.</remarks>
        public Guid PartitionIdFilter
        {
            get;
            set;
        }

        /// <summary>
        /// Gets the list of <see cref="System.Fabric.Health.ReplicaHealthStateFilter"/> to be applied to the replica children health states.
        /// </summary>
        /// <value>The list of <see cref="System.Fabric.Health.ReplicaHealthStateFilter"/> to be applied to the replica children health states.</value>
        /// <remarks>All replica children that match the filter will be returned as children of the partition.
        /// If empty, the children are not returned by default.</remarks>
        public IList<ReplicaHealthStateFilter> ReplicaFilters
        {
            get;
            internal set;
        }

        /// <summary>
        /// Returns a string representation of the filter.
        /// </summary>
        /// <returns>A string representation of the filter.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("PartitionHealthStateFilter: ");
            if (this.PartitionIdFilter != Guid.Empty)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}, ", this.PartitionIdFilter));
            }
            
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}", this.HealthStateFilter));
            return sb.ToString();
        }
        
        internal static IntPtr ToNativeList(PinCollection pinCollection, IList<PartitionHealthStateFilter> partitionList)
        {
            if (partitionList == null || partitionList.Count == 0)
            {
                return IntPtr.Zero;
            }

            var nativeArray = new NativeTypes.FABRIC_PARTITION_HEALTH_STATE_FILTER[partitionList.Count];

            for (int i = 0; i < partitionList.Count; i++)
            {
                nativeArray[i].HealthStateFilter = (UInt32)partitionList[i].HealthStateFilter;
                nativeArray[i].PartitionIdFilter = partitionList[i].PartitionIdFilter;
                nativeArray[i].ReplicaFilters = ReplicaHealthStateFilter.ToNativeList(pinCollection, partitionList[i].ReplicaFilters);
            }

            var nativeList = new NativeTypes.FABRIC_PARTITION_HEALTH_STATE_FILTER_LIST();
            nativeList.Count = (uint)partitionList.Count;
            nativeList.Items = pinCollection.AddBlittable(nativeArray);

            return pinCollection.AddBlittable(nativeList);
        }
    }
}

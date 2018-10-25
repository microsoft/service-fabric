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
    /// Filter for <see cref="System.Fabric.Health.ReplicaHealthState"/> objects.
    /// </summary>
    /// <remarks>The entity health state chunk queries can specify a list of replica filters to fine-grain
    /// select the replicas that should be included in the query result.
    /// Note that all the replicas are used to evaluate parents aggregated health state, regardless of the filter passed in.</remarks>
    public sealed class ReplicaHealthStateFilter
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.Health.ReplicaHealthStateFilter"/> class.
        /// </summary>
        public ReplicaHealthStateFilter()
        {
            this.ReplicaOrInstanceIdFilter = NativeTypes.FABRIC_INVALID_INSTANCE_ID;
        }

        /// <summary>
        /// Gets or sets filter for the aggregated health state of the <see cref="System.Fabric.Health.ReplicaHealthState"/> entries in the collection. 
        /// </summary>
        /// <value>The filter for the aggregated health state of the <see cref="System.Fabric.Health.ReplicaHealthState"/> entries in the collection.</value>
        /// <remarks>The health state filter value comes from members or bitwise combination of members of <see cref="System.Fabric.Health.HealthStateFilter"/>. 
        /// For a replica to match the filter, its aggregated health state must match the specified health state filter.</remarks>
        public HealthStateFilter HealthStateFilter
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the stateful service replica id or the stateless instance id filter.
        /// </summary>
        /// <value>The stateful service replica id or the stateless instance id filter.</value>
        /// <remarks>If not specified, all replicas that match the parent filters (if any) and the specified health state filter match the filter.
        /// Otherwise, the filter only applies to the replica identified by the given id.</remarks>
        public long ReplicaOrInstanceIdFilter
        {
            get;
            set;
        }

        /// <summary>
        /// Returns a string representation of the filter.
        /// </summary>
        /// <returns>A string representation of the filter.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("ReplicaHealthStateFilter: ");
            if (this.ReplicaOrInstanceIdFilter != NativeTypes.FABRIC_INVALID_INSTANCE_ID)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}, ", this.ReplicaOrInstanceIdFilter));
            } 
            
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}", this.HealthStateFilter));
            return sb.ToString();
        }

        internal static IntPtr ToNativeList(PinCollection pinCollection, IList<ReplicaHealthStateFilter> replicaList)
        {
            if (replicaList == null || replicaList.Count == 0)
            {
                return IntPtr.Zero;
            }

            var nativeArray = new NativeTypes.FABRIC_REPLICA_HEALTH_STATE_FILTER[replicaList.Count];

            for (int i = 0; i < replicaList.Count; i++)
            {
                nativeArray[i].HealthStateFilter = (UInt32)replicaList[i].HealthStateFilter;
                nativeArray[i].ReplicaOrInstanceIdFilter = replicaList[i].ReplicaOrInstanceIdFilter;
            }
                        
            var nativeList = new NativeTypes.FABRIC_REPLICA_HEALTH_STATE_FILTER_LIST();
            nativeList.Count = (uint)replicaList.Count;
            nativeList.Items = pinCollection.AddBlittable(nativeArray);

            return pinCollection.AddBlittable(nativeList);
        }
    }
}

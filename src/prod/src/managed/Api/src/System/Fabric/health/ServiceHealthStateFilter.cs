// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// Filter for <see cref="System.Fabric.Health.ServiceHealthState"/> objects.
    /// </summary>
    /// <remarks>The entity health state chunk queries can specify a list of service filters to fine-grain
    /// select the services that should be included in the query result.
    /// Note that all the services are used to evaluate parents aggregated health state, regardless of the filter passed in.</remarks>
    public sealed class ServiceHealthStateFilter
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.Health.ServiceHealthStateFilter"/> class.
        /// </summary>
        public ServiceHealthStateFilter()
        {
            this.PartitionFilters = new List<PartitionHealthStateFilter>();
        }

        /// <summary>
        /// Gets or sets filter for the aggregated health state of the <see cref="System.Fabric.Health.ServiceHealthState"/> entries in the collection. 
        /// </summary>
        /// <value>The filter for the aggregated health state of the <see cref="System.Fabric.Health.ServiceHealthState"/> entries in the collection.</value>
        /// <remarks>The health state filter value comes from members or bitwise combination of members of <see cref="System.Fabric.Health.HealthStateFilter"/>. 
        /// For a service to match the filter, its aggregated health state must match the specified health state filter.</remarks>
        public HealthStateFilter HealthStateFilter
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the service name filter.
        /// </summary>
        /// <value>The service name filter.</value>
        /// <remarks>If not specified, all services that match the parent filters (if any) and the specified health state filter match the filter.
        /// Otherwise, the filter only applies to the specified service. All the other filter members, like health state filter, are applied to this service.</remarks>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public Uri ServiceNameFilter
        {
            get;
            set;
        }

        /// <summary>
        /// Gets the list of <see cref="System.Fabric.Health.PartitionHealthStateFilter"/> to be applied to the partition children health states.
        /// </summary>
        /// <value>The list of <see cref="System.Fabric.Health.PartitionHealthStateFilter"/> to be applied to the partition children health states.</value>
        /// <remarks>The list can contain one default partition filter and/or partition filters for specific partitions to fine-grain entities returned by the query.
        /// All partition children that match the filter will be returned as children of the service.
        /// If empty, the children are not returned by default.</remarks>
        public IList<PartitionHealthStateFilter> PartitionFilters
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
            StringBuilder sb = new StringBuilder("ServiceHealthStateFilter: ");
            if (this.ServiceNameFilter != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}, ", this.ServiceNameFilter));
            }
            
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}", this.HealthStateFilter));
         
            return sb.ToString();
        }

        internal static IntPtr ToNativeList(PinCollection pinCollection, IList<ServiceHealthStateFilter> serviceList)
        {
            if (serviceList == null || serviceList.Count == 0)
            {
                return IntPtr.Zero;
            }

            var nativeArray = new NativeTypes.FABRIC_SERVICE_HEALTH_STATE_FILTER[serviceList.Count];

            for (int i = 0; i < serviceList.Count; i++)
            {
                nativeArray[i].HealthStateFilter = (UInt32)serviceList[i].HealthStateFilter;
                nativeArray[i].ServiceNameFilter = pinCollection.AddObject(serviceList[i].ServiceNameFilter);
                nativeArray[i].PartitionFilters = PartitionHealthStateFilter.ToNativeList(pinCollection, serviceList[i].PartitionFilters);
            }

            var nativeList = new NativeTypes.FABRIC_SERVICE_HEALTH_STATE_FILTER_LIST();
            nativeList.Count = (uint)serviceList.Count;
            nativeList.Items = pinCollection.AddBlittable(nativeArray);

            return pinCollection.AddBlittable(nativeList);
        }
    }
}

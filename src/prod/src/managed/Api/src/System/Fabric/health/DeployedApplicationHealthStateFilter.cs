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
    /// Filter for <see cref="System.Fabric.Health.DeployedApplicationHealthState"/> objects.
    /// </summary>
    /// <remarks>The health state chunk queries can specify a list of deployed application filters to fine-grain
    /// select the deployed applications that should be included in the query result.
    /// Note that all the deployed applications are used to evaluate parents' aggregated health state, regardless of the filter passed in.</remarks>
    public sealed class DeployedApplicationHealthStateFilter
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.Health.DeployedApplicationHealthStateFilter"/> class.
        /// </summary>
        public DeployedApplicationHealthStateFilter()
        {
            this.DeployedServicePackageFilters = new List<DeployedServicePackageHealthStateFilter>();
        }

        /// <summary>
        /// Gets or sets filter for the aggregated health state of the <see cref="System.Fabric.Health.DeployedApplicationHealthState"/> entries in the collection. 
        /// </summary>
        /// <value>The filter for the aggregated health state of the <see cref="System.Fabric.Health.DeployedApplicationHealthState"/> entries in the collection.</value>
        /// <remarks>The health state filter value comes from members or bitwise combination of members of <see cref="System.Fabric.Health.HealthStateFilter"/>. 
        /// For a deployed application to match the filter, its aggregated health state must match the specified health state filter.</remarks>
        public HealthStateFilter HealthStateFilter
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the node name filter.
        /// </summary>
        /// <value>The node name filter.</value>
        /// <remarks>If not specified, all deployed applications that match the parent filters (if any) are taken into consideration and matched against the other filter members, like health state filter.
        /// Otherwise, the filter only applies to applications deployed on the specified node.</remarks>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public string NodeNameFilter
        {
            get;
            set;
        }

        /// <summary>
        /// Gets the list of <see cref="System.Fabric.Health.DeployedServicePackageHealthStateFilter"/> to be applied to the deployed service package health states.
        /// </summary>
        /// <value>The list of <see cref="System.Fabric.Health.DeployedServicePackageHealthStateFilter"/> to be applied to the deployed service package health states.</value>
        /// <remarks>All deployed service packages that match the filter will be returned as children of the deployed application.
        /// If empty, the children are not returned by default.</remarks>
        public IList<DeployedServicePackageHealthStateFilter> DeployedServicePackageFilters
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
            StringBuilder sb = new StringBuilder("DeployedApplicationHealthStateFilter: ");
            
            if (!string.IsNullOrWhiteSpace(this.NodeNameFilter))
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, "NodeNameFilter={0}, ", this.NodeNameFilter));
            }

            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}", this.HealthStateFilter));
            return sb.ToString();
        }

        internal static IntPtr ToNativeList(PinCollection pinCollection, IList<DeployedApplicationHealthStateFilter> deployedApplicationList)
        {
            if (deployedApplicationList == null || deployedApplicationList.Count == 0)
            {
                return IntPtr.Zero;
            }

            var nativeArray = new NativeTypes.FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_FILTER[deployedApplicationList.Count];

            for (int i = 0; i < deployedApplicationList.Count; i++)
            {
                nativeArray[i].HealthStateFilter = (UInt32)deployedApplicationList[i].HealthStateFilter;
                nativeArray[i].NodeNameFilter = pinCollection.AddObject(deployedApplicationList[i].NodeNameFilter);
                nativeArray[i].DeployedServicePackageFilters = DeployedServicePackageHealthStateFilter.ToNativeList(pinCollection, deployedApplicationList[i].DeployedServicePackageFilters);
            }

            var nativeList = new NativeTypes.FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_FILTER_LIST();
            nativeList.Count = (uint)deployedApplicationList.Count;
            nativeList.Items = pinCollection.AddBlittable(nativeArray);

            return pinCollection.AddBlittable(nativeList);
        }
    }
}
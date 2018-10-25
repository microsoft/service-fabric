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
    /// Filter for <see cref="System.Fabric.Health.NodeHealthState"/> objects.
    /// </summary>
    /// <remarks>The cluster health state chunk queries can specify a list of node filters to fine-grain
    /// select the nodes that should be included in the query result.
    /// Note that all the nodes are used to evaluate cluster aggregated health state, regardless of the filter passed in.</remarks>
    public sealed class NodeHealthStateFilter
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.Health.NodeHealthStateFilter"/> class.
        /// </summary>
        public NodeHealthStateFilter()
        {
        }

        /// <summary>
        /// Gets or sets filter for the aggregated health state of the <see cref="System.Fabric.Health.NodeHealthState"/> entries in the collection. 
        /// </summary>
        /// <value>The filter for the aggregated health state of the <see cref="System.Fabric.Health.NodeHealthState"/> entries in the collection.</value>
        /// <remarks>The health state filter value comes from members or bitwise combination of members of <see cref="System.Fabric.Health.HealthStateFilter"/>. 
        /// For a node to match the filter, its aggregated health state must match the specified health state filter.</remarks>
        public HealthStateFilter HealthStateFilter
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the node name filter.
        /// </summary>
        /// <value>The node name filter.</value>
        /// <remarks>If not specified, all nodes that match the parent filters (if any) are taken into consideration and matched against the other filter members, like health state filter.
        /// Otherwise, the filter only applies to the specfied node.</remarks>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public string NodeNameFilter
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
            StringBuilder sb = new StringBuilder("NodeHealthStateFilter: ");
            if (!string.IsNullOrWhiteSpace(this.NodeNameFilter))
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}, ", this.NodeNameFilter));
            }

            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}", this.HealthStateFilter));
            return sb.ToString();
        }
        
        internal static IntPtr ToNativeList(PinCollection pinCollection, IList<NodeHealthStateFilter> nodeList)
        {
            if (nodeList == null || nodeList.Count == 0)
            {
                return IntPtr.Zero;
            }

            var nativeArray = new NativeTypes.FABRIC_NODE_HEALTH_STATE_FILTER[nodeList.Count];

            for (int i = 0; i < nodeList.Count; i++)
            {
                nativeArray[i].HealthStateFilter = (UInt32)nodeList[i].HealthStateFilter;
                nativeArray[i].NodeNameFilter = pinCollection.AddObject(nodeList[i].NodeNameFilter);
            }
            
            var nativeList = new NativeTypes.FABRIC_NODE_HEALTH_STATE_FILTER_LIST();
            nativeList.Count = (uint)nativeArray.Length;
            nativeList.Items = pinCollection.AddBlittable(nativeArray);

            return pinCollection.AddBlittable(nativeList);
        }
    }
}

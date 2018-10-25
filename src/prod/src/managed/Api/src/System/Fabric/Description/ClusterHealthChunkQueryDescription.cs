// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Health;
    using System.Fabric.Interop;

    /// <summary>
    /// Describes the cluster health chunk query input.
    /// </summary>
    public sealed class ClusterHealthChunkQueryDescription
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.Description.ClusterHealthChunkQueryDescription"/> class.
        /// </summary>
        public ClusterHealthChunkQueryDescription()
        {
            this.ApplicationHealthPolicies = new ApplicationHealthPolicyMap();
            this.ApplicationFilters = new List<ApplicationHealthStateFilter>();
            this.NodeFilters = new List<NodeHealthStateFilter>();
        }

        /// <summary>
        /// Gets or sets the <see cref="System.Fabric.Health.ClusterHealthPolicy"/> used to evaluate the cluster health. 
        /// </summary>
        /// <value>the <see cref="System.Fabric.Health.ClusterHealthPolicy"/> used to evaluate the cluster health.</value>
        /// <remarks>The policy will be used to evaluate the aggregated health state of the events reported on cluster and the aggregated health state of the nodes.
        /// If not specified, the cluster health policy described in the manifest or the default cluster health policy are used.</remarks>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public ClusterHealthPolicy ClusterHealthPolicy
        {
            get;
            set;
        }

        /// <summary>
        /// Gets the application health policies used to evaluate the health of the applications from the cluster. 
        /// </summary>
        /// <value>The application health policies used to evaluate the health of the specified applications.</value>
        /// <remarks>Each entry specifies as key the application name and as value an
        /// <see cref="System.Fabric.Health.ApplicationHealthPolicy"/> used to evaluate the application health.
        /// If an application is not specified in the map, the ApplicationHealthPolicy found in the application manifest will be used for evaluation. 
        /// The map is empty by default.
        /// </remarks>
        public ApplicationHealthPolicyMap ApplicationHealthPolicies
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the list of <see cref="System.Fabric.Health.ApplicationHealthStateFilter"/> to be applied to the application children health states.
        /// </summary>
        /// <value>The list of <see cref="System.Fabric.Health.ApplicationHealthStateFilter"/> to be applied to the application children health states.</value>
        /// <remarks>The list can contain one default application filter and/or application filters for specific applications or application types to fine-grain entities returned by the query.
        /// All application children that match the filter will be returned as children of the cluster.
        /// If empty, no application is returned.</remarks>
        public IList<ApplicationHealthStateFilter> ApplicationFilters
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the list of <see cref="System.Fabric.Health.NodeHealthStateFilter"/> to be applied to the node children health states.
        /// </summary>
        /// <value>The list of <see cref="System.Fabric.Health.NodeHealthStateFilter"/> to be applied to the node children health states.</value>
        /// <remarks>
        /// All node children that match the filter will be returned as children of the cluster.
        /// If empty, no node is returned.</remarks>
        public IList<NodeHealthStateFilter> NodeFilters
        {
            get;
            internal set;
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeClusterHealthChunkQueryDescription = new NativeTypes.FABRIC_CLUSTER_HEALTH_CHUNK_QUERY_DESCRIPTION();

            if (this.ClusterHealthPolicy != null)
            {
                nativeClusterHealthChunkQueryDescription.ClusterHealthPolicy = this.ClusterHealthPolicy.ToNative(pinCollection);
            }

            nativeClusterHealthChunkQueryDescription.ApplicationHealthPolicyMap = this.ApplicationHealthPolicies.ToNative(pinCollection);

            nativeClusterHealthChunkQueryDescription.ApplicationFilters = ApplicationHealthStateFilter.ToNativeList(pinCollection, this.ApplicationFilters);
            nativeClusterHealthChunkQueryDescription.NodeFilters = NodeHealthStateFilter.ToNativeList(pinCollection, this.NodeFilters);

            return pinCollection.AddBlittable(nativeClusterHealthChunkQueryDescription);
        }
    }
}
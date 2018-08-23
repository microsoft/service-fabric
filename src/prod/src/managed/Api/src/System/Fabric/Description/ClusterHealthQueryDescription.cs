// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// <para>Provides query input for getting <see cref="System.Fabric.Health.ClusterHealth" />. Used by 
    /// <see cref="System.Fabric.FabricClient.HealthClient.GetClusterHealthAsync(System.Fabric.Description.ClusterHealthQueryDescription)" />.</para>
    /// </summary>
    public sealed class ClusterHealthQueryDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ClusterHealthQueryDescription" /> class.</para>
        /// </summary>
        public ClusterHealthQueryDescription()
        {
            this.ApplicationHealthPolicyMap = new ApplicationHealthPolicyMap();
        }

        /// <summary>
        /// <para>Gets or sets the <see cref="System.Fabric.Health.ClusterHealthPolicy" /> used to evaluate the cluster health. The policy 
        /// will be used to evaluate the aggregated health state of the events reported on cluster as well as the aggregated health states of the nodes.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Health.ClusterHealthPolicy" /> used to evaluate the cluster health.</para>
        /// </value>
        public ClusterHealthPolicy HealthPolicy
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the filter for the collection of <see cref="System.Fabric.Health.HealthEvent" /> reported on the cluster.</para>
        /// </summary>
        /// <value>
        /// <para>The filter for the collection of <see cref="System.Fabric.Health.HealthEvent" /> reported on the cluster.</para>
        /// </value>
        /// <remarks><para> Only events that match 
        /// the filter will be returned. All events will be used to evaluate the cluster aggregated health state.
        /// If the filter is not specified, all events are returned.</para></remarks>
        public HealthEventsFilter EventsFilter
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the filter for <see cref="System.Fabric.Health.NodeHealthState" /> children. Only children that match the filter 
        /// will be returned.</para>
        /// </summary>
        /// <value>
        /// <para>The filter for <see cref="System.Fabric.Health.NodeHealthState" /> children.</para>
        /// </value>
        /// <remarks><para> Only nodes that match the filter will be returned. 
        /// All nodes will be used to evaluate the cluster aggregated health state.
        /// If the filter is not specified, all cluster nodes are returned.</para></remarks>
        public NodeHealthStatesFilter NodesFilter
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the filter for <see cref="System.Fabric.Health.ApplicationHealthState" /> children. Only children that match the 
        /// filter will be returned.</para>
        /// </summary>
        /// <value>
        /// <para>The filter for <see cref="System.Fabric.Health.ApplicationHealthState" /> children.</para>
        /// </value>
        /// <remarks><para> Only applications that match the filter will be returned. 
        /// All applications will be used to evaluate the cluster aggregated health state.
        /// If the filter is not specified, all cluster applications are returned.</para></remarks>
        public ApplicationHealthStatesFilter ApplicationsFilter
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets the application health policies used to evaluate the health of the applications from the cluster. Each entry specifies 
        /// as key the application name and as value an <see cref="System.Fabric.Health.ApplicationHealthPolicy" /> used to evaluate the 
        /// application health.</para>
        /// </summary>
        /// <value>
        /// <para>The application health policies used to evaluate the health of the applications from the cluster.</para>
        /// </value>
        /// <remarks><para>If an application is not specified in the map, the <see cref="System.Fabric.Health.ApplicationHealthPolicy" /> found 
        /// in its application manifest will be used for evaluation. The map is empty by default.</para></remarks>
        public ApplicationHealthPolicyMap ApplicationHealthPolicyMap
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets or sets the health statistics filter.
        /// </summary>
        /// <value>The health statistics filter.</value>
        /// <remarks>
        /// <para>
        /// The health statistics filter controls whether the <see cref="System.Fabric.Health.ClusterHealth"/> returned by the query
        /// contains the cluster health statistics. If not specified, the statistics are included.
        /// </para>
        /// </remarks>
        public ClusterHealthStatisticsFilter HealthStatisticsFilter
        {
            get;
            set;
        }

        /// <summary>
        /// Gets a string representation of the health query description.
        /// </summary>
        /// <returns>A string representation of the health query description.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(string.Format(CultureInfo.CurrentCulture, "Cluster query"));
            if (this.EventsFilter != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.EventsFilter));
            }

            if (this.NodesFilter != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.NodesFilter));
            }

            if (this.ApplicationsFilter != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.ApplicationsFilter));
            }

            if (this.HealthPolicy != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.HealthPolicy));
            }

            if (this.HealthStatisticsFilter != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.HealthStatisticsFilter));
            }

            return sb.ToString();
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeClusterHealthQueryDescription = new NativeTypes.FABRIC_CLUSTER_HEALTH_QUERY_DESCRIPTION();

            if (this.HealthPolicy != null)
            {
                nativeClusterHealthQueryDescription.HealthPolicy = this.HealthPolicy.ToNative(pinCollection);
            }

            if (this.EventsFilter != null)
            {
                nativeClusterHealthQueryDescription.EventsFilter = this.EventsFilter.ToNative(pinCollection);
            }

            if (this.NodesFilter != null)
            {
                nativeClusterHealthQueryDescription.NodesFilter = this.NodesFilter.ToNative(pinCollection);
            }

            if (this.ApplicationsFilter != null)
            {
                nativeClusterHealthQueryDescription.ApplicationsFilter = this.ApplicationsFilter.ToNative(pinCollection);
            }

            if (this.ApplicationHealthPolicyMap.Count > 0)
            {
                nativeClusterHealthQueryDescription.ApplicationHealthPolicyMap = this.ApplicationHealthPolicyMap.ToNative(pinCollection);
            }

            if (this.HealthStatisticsFilter != null)
            {
                var ex1 = new NativeTypes.FABRIC_CLUSTER_HEALTH_QUERY_DESCRIPTION_EX1();
                ex1.HealthStatisticsFilter = this.HealthStatisticsFilter.ToNative(pinCollection);
                nativeClusterHealthQueryDescription.Reserved = pinCollection.AddBlittable(ex1);
            }

            return pinCollection.AddBlittable(nativeClusterHealthQueryDescription);
        }
    }
}

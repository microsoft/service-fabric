// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// <para>Describes query input for getting <see cref="System.Fabric.Health.NodeHealth" />. Used by 
    /// <see cref="System.Fabric.FabricClient.HealthClient.GetNodeHealthAsync(System.Fabric.Description.NodeHealthQueryDescription)" />.</para>
    /// </summary>
    public sealed class NodeHealthQueryDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.NodeHealthQueryDescription" /> class.</para>
        /// </summary>
        /// <param name="nodeName">
        /// <para>The node name. Cannot be null or empty.</para>
        /// </param>
        /// <exception cref="System.ArgumentException">
        /// <para>A required parameter can't be null or empty.</para>
        /// </exception>
        public NodeHealthQueryDescription(string nodeName)
        {
            Requires.Argument<string>("nodeName", nodeName).NotNullOrEmpty();
            this.NodeName = nodeName;
        }

        /// <summary>
        /// <para>Gets the node name.</para>
        /// </summary>
        /// <value>
        /// <para>The node name.</para>
        /// </value>
        public string NodeName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets or sets the <see cref="System.Fabric.Health.ClusterHealthPolicy" /> used to evaluate the node health.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Health.ClusterHealthPolicy" /> used to evaluate the node health.</para>
        /// </value>
        /// <remarks><para>If not specified, the health store uses the cluster health policy from the manifest (if exists) or the default, strict health policy.</para></remarks>
        public ClusterHealthPolicy HealthPolicy
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the filter for the collection of <see cref="System.Fabric.Health.HealthEvent" /> reported on the node. Only events 
        /// that match the filter will be returned.</para>
        /// </summary>
        /// <value>
        /// <para>The filter for the health event.</para>
        /// </value>
        /// <remarks><para> Only events that match 
        /// the filter will be returned. All events will be used to evaluate the node aggregated health state.
        /// If the filter is not specified, all events are returned.</para></remarks>
        public HealthEventsFilter EventsFilter
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
            sb.Append(string.Format(CultureInfo.CurrentCulture, "Node {0}", this.NodeName));
            if (this.EventsFilter != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.EventsFilter));
            }

            if (this.HealthPolicy != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.HealthPolicy));
            }

            return sb.ToString();
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeNodeHealthQueryDescription = new NativeTypes.FABRIC_NODE_HEALTH_QUERY_DESCRIPTION();

            nativeNodeHealthQueryDescription.NodeName = pinCollection.AddBlittable(this.NodeName);
            if (this.HealthPolicy != null)
            {
                nativeNodeHealthQueryDescription.HealthPolicy = this.HealthPolicy.ToNative(pinCollection);
            }
            if (this.EventsFilter != null)
            {
                nativeNodeHealthQueryDescription.EventsFilter = this.EventsFilter.ToNative(pinCollection);
            }

            return pinCollection.AddBlittable(nativeNodeHealthQueryDescription);
        }
    }
}

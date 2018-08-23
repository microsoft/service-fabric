// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;
    using System.Fabric.Query;
    using System.Text;

    /// <summary>
    /// <para>Describes a set of filters used when running the query
    /// <see cref="System.Fabric.FabricClient.QueryClient.GetNodePagedListAsync(NodeQueryDescription, TimeSpan, Threading.CancellationToken)"/>.</para>
    /// </summary>
    /// <remarks>
    /// <para>This query description can be customized by setting individual properties.</para>
    /// </remarks>
    public sealed class NodeQueryDescription : PagedQueryDescriptionBase
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.NodeQueryDescription" /> class.</para>
        /// </summary>
        public NodeQueryDescription()
        {
            this.NodeNameFilter = null;
            this.NodeStatusFilter = NodeStatusFilter.Default;
        }

        /// <summary>
        /// <para>Gets or sets the name of the node to query for.</para>
        /// </summary>
        /// <value>
        /// <para>The full name of node to query for.</para>
        /// </value>
        /// <remarks>
        /// <para>The node name is a case-sensitive exact match. For example, "Test" will not match "TestNode".
        /// Gets all nodes if the node name filter is not set.
        /// If the node name does not match any node on the cluster, an empty list is returned.</para>
        /// </remarks>
        public string NodeNameFilter { get; set; }

        /// <summary>
        /// <para>Gets or sets the status of the node to filter by.</para>
        /// </summary>
        /// <value>
        /// <para>The node status of the nodes to get details for.</para>
        /// </value>
        /// <remarks>
        /// <para>This describes the state of the node, such as up or down.</para>
        /// </remarks>
        public NodeStatusFilter NodeStatusFilter { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_NODE_QUERY_DESCRIPTION();

            nativeDescription.NodeNameFilter = pinCollection.AddObject(this.NodeNameFilter);

            var ex1 = new NativeTypes.FABRIC_NODE_QUERY_DESCRIPTION_EX1();
            ex1.ContinuationToken = pinCollection.AddObject(this.ContinuationToken);

            var ex2 = new NativeTypes.FABRIC_NODE_QUERY_DESCRIPTION_EX2();
            ex2.NodeStatusFilter = (UInt32)this.NodeStatusFilter;

            var ex3 = new NativeTypes.FABRIC_NODE_QUERY_DESCRIPTION_EX3();
            if (this.MaxResults.HasValue)
            {
                ex3.MaxResults = this.MaxResults.Value;
            }

            ex3.Reserved = IntPtr.Zero;
            ex2.Reserved = pinCollection.AddBlittable(ex3);

            ex1.Reserved = pinCollection.AddBlittable(ex2);

            nativeDescription.Reserved = pinCollection.AddBlittable(ex1);
            return pinCollection.AddBlittable(nativeDescription);
        }

        /// <summary>
        /// Overrides ToString() method to print all content of the query description.
        /// </summary>
        /// <returns>
        /// Returns a string containing all the properties of the query description.
        /// </returns>
        public override string ToString()
        {
            var sb = new StringBuilder();

            sb.AppendFormat("NodeQueryDescription:{0}", Environment.NewLine);
            sb.AppendFormat("NodeNameFilter = {0}{1}", this.NodeNameFilter, Environment.NewLine);
            sb.AppendFormat("NodeStatusFilter = {0}{1}", this.NodeStatusFilter, Environment.NewLine);
            sb.AppendFormat("Paging = {0}", base.ToString());

            return sb.ToString();
        }
    }
}

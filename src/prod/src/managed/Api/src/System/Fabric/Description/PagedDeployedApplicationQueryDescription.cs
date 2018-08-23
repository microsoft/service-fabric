// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;
    using Text;

    /// <summary>
    /// <para>Describes a set of filters used when running the query
    /// <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedApplicationPagedListAsync(PagedDeployedApplicationQueryDescription)"/>.</para>
    /// </summary>
    /// <remarks>
    /// <para>This query description can be customized by setting individual properties.</para>
    /// </remarks>
    public sealed class PagedDeployedApplicationQueryDescription : PagedQueryDescriptionBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.Description.PagedDeployedApplicationQueryDescription"/> class.
        /// </summary>
        public PagedDeployedApplicationQueryDescription(string nodeName)
        {
            this.NodeName = nodeName;
        }

        /// <summary>
        /// <para>Gets or sets the node on which to get a list of deployed applications.</para>
        /// </summary>
        /// <remarks>
        /// <para>
        /// This is a required parameter.
        /// </para>
        /// <para>
        /// This parameter matches against a case sensitive exact node name.
        /// For example, the value "Node" does not match "Node1" because it is only a partial match.
        /// An exception is thrown when the given node name does not match any node on the cluster.
        /// </para>
        /// </remarks>
        public string NodeName { get; set; }

        /// <summary>
        /// <para>Gets or sets the application name to get details for.</para>
        /// </summary>
        /// <remarks>
        /// <para>
        /// Defaults to null, which matches all the application names on the given node.
        /// If an application name is provided, a <see cref="System.Fabric.Query.DeployedApplicationPagedList" /> containing one item will be returned.
        /// </para>
        /// <para>
        /// This parameter matches against the case sensitive exact application names.
        /// For example, the value "Test" does not match "TestApp" because it is only a partial match.
        /// To request all the applications on the given node, do not set this value.
        /// An empty <see cref="System.Fabric.Query.DeployedApplicationPagedList" /> will be returned if the name does not match any applications on the node.
        /// </para>
        /// </remarks>
        public Uri ApplicationNameFilter { get; set; }

        /// <summary>
        /// <para>Gets or sets whether to include the health state of the deployed application from the query result.</para>
        /// </summary>
        /// <remarks>
        /// <para>
        /// Defaults to false.
        /// </para>
        /// <para>
        /// Setting this to true fills in the <see cref="System.Fabric.Query.DeployedApplication.HealthState" /> state. Otherwise, the value is 'Unknown'.
        /// </para>
        /// </remarks>
        public bool IncludeHealthState { get; set; }

        /// <summary>
        /// Overrides ToString() method to print all content of the query description.
        /// </summary>
        /// <returns>
        /// Returns a string containing all the properties of the query description.
        /// </returns>
        public override string ToString()
        {
            var sb = new StringBuilder();

            sb.AppendFormat("System.Fabric.Description.PagedDeployedApplicationQueryDescription object:{0}", Environment.NewLine);
            sb.AppendFormat("NodeName = {0}{1}", this.NodeName, Environment.NewLine);
            sb.AppendFormat("ApplicationNameFilter = {0}{1}", this.ApplicationNameFilter, Environment.NewLine);
            sb.AppendFormat("IncludeHealthState = {0}{1}", this.IncludeHealthState, Environment.NewLine);
            sb.AppendFormat("ContinuationToken = {0}{1}", this.ContinuationToken, Environment.NewLine);
            sb.AppendFormat("MaxResults = {0}", this.MaxResults);

            return sb.ToString();
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_PAGED_DEPLOYED_APPLICATION_QUERY_DESCRIPTION();

            if (this.ApplicationNameFilter != null)
            {
                nativeDescription.ApplicationNameFilter = pinCollection.AddObject(this.ApplicationNameFilter);
            }

            nativeDescription.PagingDescription = this.ToNativePagingDescription(pinCollection);
            nativeDescription.NodeName = pinCollection.AddObject(this.NodeName); // Required parameter
            nativeDescription.IncludeHealthState = NativeTypes.ToBOOLEAN(this.IncludeHealthState);

            nativeDescription.Reserved = IntPtr.Zero;
            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}
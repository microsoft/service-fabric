// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// Represents the cluster state chunk, which includes health information about the cluster.
    /// </summary>
    public sealed class ClusterHealthChunk
    {
        /// <summary>
        /// Instantiates a <see cref="System.Fabric.Health.ClusterHealthChunk"/>.
        /// </summary>
        internal ClusterHealthChunk()
        {
        }

        /// <summary>
        /// Gets the cluster aggregated health state.
        /// </summary>
        /// <value>The cluster aggregated health state.</value>
        public HealthState HealthState
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the list of the node health state chunks that respect the input filters.
        /// </summary>
        /// <value>The list of the node health state chunks that respect the input filters.</value>
        /// <remarks>
        /// <para>By default, no children are included in results. Users can request to include
        /// some children based on desired health or other information.</para></remarks>
        public NodeHealthStateChunkList NodeHealthStateChunks
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the list of the application health state chunks that respect the input filters.
        /// </summary>
        /// <value>The list of the application health state chunks that respect the input filters.</value>
        /// <remarks>
        /// <para>By default, no children are included in results. Users can request to include
        /// some children based on desired health or other information.</para></remarks>
        public ApplicationHealthStateChunkList ApplicationHealthStateChunks
        {
            get;
            internal set;
        }

        /// <summary>
        /// Creates a string description of the cluster chunk.
        /// </summary>
        /// <returns>String description of the cluster chunk.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(string.Format(CultureInfo.CurrentCulture, "Cluster: {0}", this.HealthState));

            if (this.NodeHealthStateChunks.Count > 0)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", Nodes: {0}/{1}", this.NodeHealthStateChunks.Count, this.NodeHealthStateChunks.TotalCount));
            }

            if (this.ApplicationHealthStateChunks.Count > 0)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", Applications {0}/{1}", this.ApplicationHealthStateChunks.Count, this.ApplicationHealthStateChunks.TotalCount));
            }

            return sb.ToString();
        }

        internal static unsafe ClusterHealthChunk FromNativeResult(NativeClient.IFabricGetClusterHealthChunkResult nativeResult)
        {
            var nativePtr = nativeResult.get_ClusterHealthChunk();
            ReleaseAssert.AssertIf(nativePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "ClusterHealthChunk"));

            var retval = FromNative(*(NativeTypes.FABRIC_CLUSTER_HEALTH_CHUNK*)nativePtr);

            GC.KeepAlive(nativeResult);
            return retval;
        }

        internal static unsafe ClusterHealthChunk FromNative(NativeTypes.FABRIC_CLUSTER_HEALTH_CHUNK nativeClusterHealthChunk)
        {
            var managedClusterHealthChunk = new ClusterHealthChunk();

            managedClusterHealthChunk.HealthState = (HealthState)nativeClusterHealthChunk.HealthState;
            managedClusterHealthChunk.NodeHealthStateChunks = NodeHealthStateChunkList.CreateFromNativeList(nativeClusterHealthChunk.NodeHealthStateChunks);
            managedClusterHealthChunk.ApplicationHealthStateChunks = ApplicationHealthStateChunkList.CreateFromNativeList(nativeClusterHealthChunk.ApplicationHealthStateChunks);

           return managedClusterHealthChunk;
        }
    }
}

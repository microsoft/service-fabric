// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// Represents a node health state chunk, which contains basic health information about the node.
    /// </summary>
    public sealed class NodeHealthStateChunk
    {
        internal NodeHealthStateChunk()
        {
        }

        /// <summary>
        /// Gets the node name.
        /// </summary>
        /// <value>The node name.</value>
        public string NodeName
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the aggregated health state of the node, computed based on all reported health events and the cluster health policy.
        /// </summary>
        /// <value>The aggregated health state of the node.</value>
        public HealthState HealthState
        {
            get;
            internal set;
        }

        /// <summary>
        /// Creates a string description of the node health state chunk.
        /// </summary>
        /// <returns>String description of the <see cref="System.Fabric.Health.NodeHealthStateChunk"/>.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "{0}: {1}",
                this.NodeName,
                this.HealthState);
        }

        internal static unsafe NodeHealthStateChunk FromNative(NativeTypes.FABRIC_NODE_HEALTH_STATE_CHUNK nativeNodeHealthStateChunk)
        {
            var managedNodeHealthStateChunk = new NodeHealthStateChunk();

            managedNodeHealthStateChunk.NodeName = NativeTypes.FromNativeString(nativeNodeHealthStateChunk.NodeName);
            managedNodeHealthStateChunk.HealthState = (HealthState)nativeNodeHealthStateChunk.HealthState;

           return managedNodeHealthStateChunk;
        }
    }
}

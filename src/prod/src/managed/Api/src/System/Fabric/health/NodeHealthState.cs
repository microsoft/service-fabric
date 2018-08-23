// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Collections.Generic;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// <para>Represents the health state of a node, which contains the node identifier and its aggregated health state.</para>
    /// </summary>
    public sealed class NodeHealthState : EntityHealthState
    {
        internal NodeHealthState()
        {
        }

        /// <summary>
        /// <para>Gets the name of the node.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the node.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Name)]
        public string NodeName
        {
            get;
            internal set;
        }

        /// <summary>
        /// Creates a string description of the node health state.
        /// </summary>
        /// <returns>String description of the <see cref="System.Fabric.Health.NodeHealthState"/>.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "{0}: {1}",
                this.NodeName,
                this.AggregatedHealthState);
        }

        internal static unsafe IList<NodeHealthState> FromNativeList(IntPtr nativeListPtr)
        {
            if (nativeListPtr != IntPtr.Zero)
            {
                var nativeList = (NativeTypes.FABRIC_NODE_HEALTH_STATE_LIST*)nativeListPtr;
                return NodeHealthStateList.FromNativeList(nativeList);
            }
            else
            {
                return new NodeHealthStateList();
            }
        }

        internal static unsafe NodeHealthState FromNative(NativeTypes.FABRIC_NODE_HEALTH_STATE nativeState)
        {
            var nodeHealthState = new NodeHealthState();
            nodeHealthState.NodeName = NativeTypes.FromNativeString(nativeState.NodeName);
            nodeHealthState.AggregatedHealthState = (HealthState)nativeState.AggregatedHealthState;

            return nodeHealthState;
        }
    }
}
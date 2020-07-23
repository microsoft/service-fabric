// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System;    
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;    

    /// <summary>
    /// <para>Describes a node in a container network.</para>
    /// </summary>    
    public sealed class NetworkNode
    {
        internal NetworkNode()
        {            
        }

        /// <summary>
        /// <para>Gets the name of the node.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the node.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Name)]
        public string NodeName { get; internal set; }

        internal static unsafe NetworkNode CreateFromNative(NativeTypes.FABRIC_NETWORK_NODE_QUERY_RESULT_ITEM nativeNetworkNode)
        {
            NetworkNode networkNode = new NetworkNode();

            networkNode.NodeName = NativeTypes.FromNativeString(nativeNetworkNode.NodeName);

            return networkNode;
        }
    }
}

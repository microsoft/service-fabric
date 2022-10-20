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
    /// <para>Describes a container network deployed to a node.</para>
    /// </summary>    
    public sealed class DeployedNetwork
    {
        internal DeployedNetwork()
        {            
        }

        /// <summary>
        /// <para>Gets the name of the network.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the network.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.NetworkName)]
        public string NetworkName { get; internal set; }

        internal static unsafe DeployedNetwork CreateFromNative(NativeTypes.FABRIC_DEPLOYED_NETWORK_QUERY_RESULT_ITEM nativeDeployedNetwork)
        {
            DeployedNetwork deployedNetwork = new DeployedNetwork();

            deployedNetwork.NetworkName = NativeTypes.FromNativeString(nativeDeployedNetwork.NetworkName);

            return deployedNetwork;
        }
    }
}

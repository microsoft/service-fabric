// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{    
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents the query input used by <see cref="System.Fabric.FabricClient.NetworkManagementClient.GetNetworkNodeListAsync(NetworkNodeQueryDescription)" />.</para>
    /// </summary> 
    public sealed class NetworkNodeQueryDescription : PagedQueryDescriptionBase
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.NetworkNodeQueryDescription" /> class.</para>
        /// </summary>
        public NetworkNodeQueryDescription()
        {
            this.NetworkName = null;
            this.NodeNameFilter = null;
            this.ContinuationToken = null;
        }

        /// <summary>
        /// <para>Gets or sets name of the network to query for.</para>
        /// </summary>
        public string NetworkName { get; set; }

        /// <summary>
        /// Gets or sets the node name used to filter the nodes to query for.
        /// The node that is of this node name will be returned.
        /// </summary>
        public string NodeNameFilter { get; set; }

        internal static void Validate(NetworkNodeQueryDescription queryDescription)
        {
            Requires.Argument<string>("NetworkName", queryDescription.NetworkName).NotNullOrWhiteSpace();

            ValidatePaging(queryDescription);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_NETWORK_NODE_QUERY_DESCRIPTION();
            nativeDescription.NetworkName = pinCollection.AddObject(this.NetworkName);
            nativeDescription.NodeNameFilter = pinCollection.AddObject(this.NodeNameFilter);
            nativeDescription.PagingDescription = this.ToNativePagingDescription(pinCollection);
            nativeDescription.Reserved = IntPtr.Zero;

            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}

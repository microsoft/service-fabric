// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents the query input used by <see cref="System.Fabric.FabricClient.NetworkManagementClient.GetNetworkListAsync(NetworkQueryDescription, TimeSpan, Threading.CancellationToken)" />.</para>
    /// </summary> 
    public sealed class NetworkQueryDescription : PagedQueryDescriptionBase
    {
        /// <summary>
        /// <para>Base class constructor used by specific network query description classes.</para>
        /// </summary>
        public NetworkQueryDescription()
        {
            this.NetworkNameFilter = null;
            this.NetworkStatusFilter = NetworkStatusFilter.Default;
            this.ContinuationToken = null;
        }

        /// <summary>
        /// <para>Gets or sets the network name used to filter the container networks to query for.
        /// The network with this network name will be returned.</para>
        /// </summary>
        public string NetworkNameFilter { get; set; }

        /// <summary>
        /// <para>Gets or sets the network status used to filter the container networks to query for.
        /// The networks with this network status will be returned.</para>
        /// </summary>
        public NetworkStatusFilter NetworkStatusFilter { get; set; }

        internal static void Validate(NetworkQueryDescription queryDescription)
        {         
            if (!string.IsNullOrEmpty(queryDescription.NetworkNameFilter) && queryDescription.NetworkStatusFilter != NetworkStatusFilter.Default)
            {
                throw new ArgumentException("Incompatible network filters");
            }

            ValidatePaging(queryDescription);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_NETWORK_QUERY_DESCRIPTION();
            nativeDescription.NetworkNameFilter = pinCollection.AddObject(this.NetworkNameFilter);
            nativeDescription.NetworkStatusFilter = (UInt32)this.NetworkStatusFilter;
            nativeDescription.PagingDescription = this.ToNativePagingDescription(pinCollection);
            nativeDescription.Reserved = IntPtr.Zero;

            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}

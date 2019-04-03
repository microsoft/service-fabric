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
    /// <para>Represents the query input used by <see cref="System.Fabric.FabricClient.NetworkManagementClient.GetNetworkApplicationListAsync(NetworkApplicationQueryDescription)" />.</para>
    /// </summary> 
    public sealed class NetworkApplicationQueryDescription : PagedQueryDescriptionBase
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.NetworkApplicationQueryDescription" /> class.</para>
        /// </summary>
        public NetworkApplicationQueryDescription()
        {
            this.NetworkName = null;
            this.ApplicationNameFilter = null;
            this.ContinuationToken = null;
        }

        /// <summary>
        /// <para>Gets or sets name of the network to query for.</para>
        /// </summary>
        public string NetworkName { get; set; }

        /// <summary>
        /// Gets or sets the application name used to filter the applications to query for.
        /// The application that is of this application name will be returned.
        /// </summary>
        public Uri ApplicationNameFilter { get; set; }

        internal static void Validate(NetworkApplicationQueryDescription queryDescription)
        {
            Requires.Argument<string>("NetworkName", queryDescription.NetworkName).NotNullOrWhiteSpace();

            ValidatePaging(queryDescription);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_NETWORK_APPLICATION_QUERY_DESCRIPTION();
            nativeDescription.NetworkName = pinCollection.AddObject(this.NetworkName);
            nativeDescription.ApplicationNameFilter = pinCollection.AddObject(this.ApplicationNameFilter);
            nativeDescription.PagingDescription = this.ToNativePagingDescription(pinCollection);
            nativeDescription.Reserved = IntPtr.Zero;

            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}

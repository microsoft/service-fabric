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
    /// <para>Represents the query input used by <see cref="System.Fabric.FabricClient.NetworkManagementClient.GetDeployedNetworkListAsync(DeployedNetworkQueryDescription)" />.</para>
    /// </summary> 
    public sealed class DeployedNetworkQueryDescription : PagedQueryDescriptionBase
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.DeployedNetworkQueryDescription" /> class.</para>
        /// </summary>
        public DeployedNetworkQueryDescription()
        {            
            this.NodeName = null;
            this.ContinuationToken = null;            
        }

        /// <summary>
        /// Gets or sets the node name to query for.        
        /// </summary>
        public string NodeName { get; set; }

        internal static void Validate(DeployedNetworkQueryDescription queryDescription)
        {
            Requires.Argument<string>("NodeName", queryDescription.NodeName).NotNullOrWhiteSpace();
            ValidatePaging(queryDescription);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_DEPLOYED_NETWORK_QUERY_DESCRIPTION();
            nativeDescription.NodeName = pinCollection.AddObject(this.NodeName);            
            nativeDescription.PagingDescription = this.ToNativePagingDescription(pinCollection);
            nativeDescription.Reserved = IntPtr.Zero;

            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}

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
    /// <para>Represents the query input used by <see cref="System.Fabric.FabricClient.NetworkManagementClient.GetApplicationNetworkListAsync(ApplicationNetworkQueryDescription)" />.</para>
    /// </summary> 
    public sealed class ApplicationNetworkQueryDescription : PagedQueryDescriptionBase
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ApplicationNetworkQueryDescription" /> class.</para>
        /// </summary>
        public ApplicationNetworkQueryDescription()
        {            
            this.ApplicationName = null;
            this.ContinuationToken = null;
        }

        /// <summary>
        /// Gets or sets the application name to query for.        
        /// </summary>
        public Uri ApplicationName { get; set; }

        internal static void Validate(ApplicationNetworkQueryDescription queryDescription)
        {
            Requires.Argument<Uri>("ApplicationName", queryDescription.ApplicationName).NotNull();

            ValidatePaging(queryDescription);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_APPLICATION_NETWORK_QUERY_DESCRIPTION();
            nativeDescription.ApplicationName = pinCollection.AddObject(this.ApplicationName);            
            nativeDescription.PagingDescription = this.ToNativePagingDescription(pinCollection);
            nativeDescription.Reserved = IntPtr.Zero;

            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}

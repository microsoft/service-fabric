// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Describes a container network to be deleted by using 
    /// <see cref="System.Fabric.FabricClient.NetworkManagementClient.DeleteNetworkAsync(DeleteNetworkDescription, TimeSpan, Threading.CancellationToken)" />.</para>
    /// </summary>    
    public class DeleteNetworkDescription
    {
        /// <summary>
        /// <para>Instantiates an instance of <see cref="System.Fabric.Description.DeleteNetworkDescription" />. </para>
        /// </summary>
        /// <param name="networkName">
        /// <para>Name of the container network.</para>
        /// </param>
        public DeleteNetworkDescription(string networkName)
        {
            this.NetworkName = networkName;
        }

        /// <summary>
        /// <para>Gets or sets name of the container network.</para>
        /// </summary>
        /// <value>
        /// <para>Name of the container network.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.NetworkName)]
        public string NetworkName { get; set; }

        internal static void Validate(DeleteNetworkDescription deleteDescription)
        {
            Requires.Argument<string>("NetworkName", deleteDescription.NetworkName).NotNullOrWhiteSpace();
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {            
            var nativeDescription = new NativeTypes.FABRIC_DELETE_NETWORK_DESCRIPTION();
            nativeDescription.NetworkName = pinCollection.AddObject(this.NetworkName);
            nativeDescription.Reserved = IntPtr.Zero;

            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}

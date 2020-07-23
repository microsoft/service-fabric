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
    /// <para>Represents the query input used by <see cref="System.Fabric.FabricClient.NetworkManagementClient.GetDeployedNetworkCodePackageListAsync(DeployedNetworkCodePackageQueryDescription)" />.</para>
    /// </summary> 
    public sealed class DeployedNetworkCodePackageQueryDescription : PagedQueryDescriptionBase
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.DeployedNetworkCodePackageQueryDescription" /> class.</para>
        /// </summary>
        public DeployedNetworkCodePackageQueryDescription()
        {
            this.NodeName = null;
            this.NetworkName = null;
            this.ApplicationNameFilter = null;
            this.ServiceManifestNameFilter = null;
            this.CodePackageNameFilter = null;
            this.ContinuationToken = null;
        }

        /// <summary>
        /// Gets or sets the node name to query for.
        /// </summary>
        public string NodeName { get; set; }

        /// <summary>
        /// Gets or sets the network name to query for.
        /// </summary>
        public string NetworkName { get; set; }

        /// <summary>
        /// <para>Gets or sets the URI name of the application used to filter the code packages to query for.</para>
        /// </summary>
        public Uri ApplicationNameFilter { get; set; }

        /// <summary>
        /// <para>Gets or sets the service manifest name used to filter the code packages to query for.</para>
        /// </summary>
        public string ServiceManifestNameFilter { get; set; }

        /// <summary>
        /// <para>Gets or sets the code package name used to filter the code packages to query for.</para>
        /// </summary>
        public string CodePackageNameFilter { get; set; }

        internal static void Validate(DeployedNetworkCodePackageQueryDescription queryDescription)
        {
            Requires.Argument<string>("NodeName", queryDescription.NodeName).NotNullOrWhiteSpace();
            Requires.Argument<string>("NetworkName", queryDescription.NetworkName).NotNullOrWhiteSpace();
            ValidatePaging(queryDescription);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_DESCRIPTION();
            nativeDescription.NodeName = pinCollection.AddObject(this.NodeName);
            nativeDescription.NetworkName = pinCollection.AddObject(this.NetworkName);
            nativeDescription.ApplicationNameFilter = pinCollection.AddObject(this.ApplicationNameFilter);
            nativeDescription.ServiceManifestNameFilter = pinCollection.AddObject(this.ServiceManifestNameFilter);
            nativeDescription.CodePackageNameFilter = pinCollection.AddObject(this.CodePackageNameFilter);
            nativeDescription.PagingDescription = this.ToNativePagingDescription(pinCollection);
            nativeDescription.Reserved = IntPtr.Zero;

            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}

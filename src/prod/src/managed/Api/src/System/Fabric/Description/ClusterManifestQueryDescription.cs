// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// <para>
    /// Describes the query parameters for use with <see cref="System.Fabric.FabricClient.ClusterManagementClient.GetClusterManifestAsync(ClusterManifestQueryDescription, TimeSpan, CancellationToken)" />.
    /// </para>
    /// </summary>
    public sealed class ClusterManifestQueryDescription
    {
        /// <summary>
        /// <para>
        /// Constructs query parameters for retrieving the contents of a cluster manifest.
        /// </para>
        /// </summary>
        /// <remarks>
        /// <para>
        /// By default, the current running cluster manifest will be retrieved. To query
        /// for another cluster manifest version that was previously provisioned using
        /// <see cref="System.Fabric.FabricClient.ClusterManagementClient.ProvisionFabricAsync(string, string)" />, specify the desired query version using
        /// <see cref="System.Fabric.Description.ClusterManifestQueryDescription.ClusterManifestVersion" />.
        /// </para>
        /// </remarks>
        public ClusterManifestQueryDescription()
        {
        }

        /// <summary>
        /// <para>
        /// Gets or sets the version of the cluster manifest to retrieve.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The version of the cluster manifest.</para>
        /// </value>
        /// <remarks>
        /// <para>
        /// When set to null (default) or an empty string, the current running cluster manifest will be retrieved.
        /// </para>
        /// </remarks>
        public string ClusterManifestVersion { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_CLUSTER_MANIFEST_QUERY_DESCRIPTION[1];

            nativeDescription[0].ClusterManifestVersion = pinCollection.AddObject(this.ClusterManifestVersion);

            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}
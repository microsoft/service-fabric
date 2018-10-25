// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    /// <summary>
    /// <para>Represents a resolved service endpoint, which contains information
    /// about service partition replica role and the address it listens to.</para>
    /// </summary>
    /// <remarks>
    /// <para>
    /// The resolved service endpoints are returned from
    /// <see cref="System.Fabric.FabricClient.ServiceManagementClient.ResolveServicePartitionAsync(System.Uri)"/>,
    /// as part of <see cref="System.Fabric.ResolvedServicePartition"/>.
    /// </para>
    /// </remarks>
    public sealed class ResolvedServiceEndpoint
    {
        // To prevent auto generated default public constructor from being public
        internal ResolvedServiceEndpoint()
        {
        }

        /// <summary>
        /// <para>Gets the role of a service endpoint.</para>
        /// </summary>
        /// <value>
        /// <para>The role of a service endpoint.</para>
        /// </value>
        /// <remarks>
        /// <para>The <see cref="System.Fabric.ServiceEndpointRole"/> is used by a client to determine which service instance
        /// or replica to select and connect to.</para>
        /// </remarks>
        public ServiceEndpointRole Role { get; internal set; }

        /// <summary>
        /// <para>Gets the address for the endpoint.
        /// The service replica gives this string to Service Fabric to let users know where it can be reached.</para>
        /// </summary>
        /// <value>
        /// <para>The address for the endpoint where the service replica or instance can be reached.</para>
        /// </value>
        public string Address { get; internal set; }
    }
}
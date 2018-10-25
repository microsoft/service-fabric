// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    /// <summary>
    /// <para>A delegate type for client-side callbacks that are made to user code when the endpoints of a service change or an exception is 
    /// encountered during the process of updating endpoint information at runtime.</para>
    /// </summary>
    /// <param name="source">
    /// <para>A reference to the <see cref="System.Fabric.FabricClient" /> instance receiving the endpoint change event.</para>
    /// </param>
    /// <param name="handlerId">
    /// <para>The ID returned from <see cref="System.Fabric.FabricClient.ServiceManagementClient.RegisterServicePartitionResolutionChangeHandler(Uri, ServicePartitionResolutionChangeHandler)" /> when the handler was registered.</para>
    /// </param>
    /// <param name="args">
    /// <para>Event arguments containing details for the event. <seealso cref="System.Fabric.ServicePartitionResolutionChange" />.</para>
    /// </param>
    public delegate void ServicePartitionResolutionChangeHandler(FabricClient source, long handlerId, ServicePartitionResolutionChange args);
}
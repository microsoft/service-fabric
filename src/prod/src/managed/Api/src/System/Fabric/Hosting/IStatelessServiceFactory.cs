// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    /// <summary>
    /// <para>Represents a stateless service factory that is responsible for creating instances of a specific type of stateless service. </para>
    /// </summary>
    /// <remarks>
    /// <para>Stateless service factories are registered with the <see cref="System.Fabric.FabricRuntime" /> by service hosts via 
    /// <see cref="System.Fabric.FabricRuntime.RegisterStatelessServiceFactory(System.String,System.Fabric.IStatelessServiceFactory)" /> or 
    /// <see cref="System.Fabric.FabricRuntime.RegisterStatelessServiceFactoryAsync(System.String,System.Fabric.IStatelessServiceFactory,System.TimeSpan,System.Threading.CancellationToken)" />.</para>
    /// </remarks>
    public interface IStatelessServiceFactory
    {
        /// <summary>
        /// <para>Creates a stateless service instance for a particular service. This method is called by Service Fabric.</para>
        /// </summary>
        /// <param name="serviceTypeName">
        /// <para>The service type that Service Fabric requests to be created.</para>
        /// </param>
        /// <param name="serviceName">
        /// <para>The <c>fabric:/ name</c> (Uri) of the service with which this replica is associated. </para>
        /// </param>
        /// <param name="initializationData">
        /// <para>A byte array that contains the initialization data which was originally passed as a part of this service’s <see cref="System.Fabric.Description.ServiceDescription" />.</para>
        /// </param>
        /// <param name="partitionId">
        /// <para>The partition ID (GUID) with which this replica is associated. </para>
        /// </param>
        /// <param name="instanceId">
        /// <para>The replica ID for this replica of type <see cref="System.Int64" />.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Fabric.IStatelessServiceInstance" />.</para>
        /// </returns>
        IStatelessServiceInstance CreateInstance(string serviceTypeName, Uri serviceName, byte[] initializationData, Guid partitionId, long instanceId);
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    /// <summary>
    /// <para>Represents a stateful service factory that is responsible for creating replicas of a specific type of stateful service. 
    /// Stateful service factories are registered with the <see cref="System.Fabric.FabricRuntime" /> by service hosts via 
    /// <see cref="System.Fabric.FabricRuntime.RegisterStatefulServiceFactory(System.String,System.Fabric.IStatefulServiceFactory)" /> or 
    /// <see cref="System.Fabric.FabricRuntime.RegisterStatefulServiceFactoryAsync(System.String,System.Fabric.IStatefulServiceFactory,System.TimeSpan,System.Threading.CancellationToken)" />.</para>
    /// </summary>
    public interface IStatefulServiceFactory
    {
        /// <summary>
        /// <para>Called by Service Fabric to create a stateful service replica for a particular service.</para>
        /// </summary>
        /// <param name="serviceTypeName">
        /// <para>The service type that Service Fabric requests to be created.</para>
        /// </param>
        /// <param name="serviceName">
        /// <para>The fabric:/ name (Uri) of the service with which this replica is associated.</para>
        /// </param>
        /// <param name="initializationData">
        /// <para>A byte array that contains the initialization data which was originally passed as a part of this 
        /// service’s <see cref="System.Fabric.Description.ServiceDescription" />.</para>
        /// </param>
        /// <param name="partitionId">
        /// <para>The partition ID of type, a GUID, with which this replica is associated.</para>
        /// </param>
        /// <param name="replicaId">
        /// <para>The replica ID of type long for this replica. </para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Fabric.IStatefulServiceReplica" />.</para>
        /// </returns>
        IStatefulServiceReplica CreateReplica(string serviceTypeName, Uri serviceName, byte[] initializationData, Guid partitionId, long replicaId);
    }
}
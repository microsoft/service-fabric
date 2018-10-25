// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// <para>Defines behavior that governs the lifecycle of a replica, such as startup, initialization, role changes, and shutdown. </para>
    /// </summary>
    /// <remarks>
    /// <para>
    ///     Stateful service types must implement this interface. The <see href="https://docs.microsoft.com/dotnet/api/microsoft.servicefabric.services.runtime.statefulservice">Reliable Stateful service</see> implements this interface and handles replica lifecycle internally. </para> 
    /// <para>
    ///     The logic of a stateful service type includes behavior that is invoked on primary replicas and behavior that is invoked on secondary replicas.</para>
    /// <para>
    ///     If the service author wants to make use of the provided <see cref="System.Fabric.FabricReplicator" />, then the service must also implement <see cref="System.Fabric.IStateProvider" /> to use the implementation of <see cref="System.Fabric.IStateReplicator" /> that is provided by <see cref="System.Fabric.FabricReplicator" />.</para>
    /// </remarks>
    public interface IStatefulServiceReplica 
    {
        /// <summary>
        /// <para>Initializes a newly created service replica.</para>
        /// </summary>
        /// <param name="initializationParameters">
        /// <para>The <see cref="System.Fabric.StatefulServiceInitializationParameters" /> for this replica.</para>
        /// </param>
        void Initialize(StatefulServiceInitializationParameters initializationParameters);

        /// <summary>
        /// <para>Opens an initialized service replica so that additional actions can be taken.</para>
        /// </summary>
        /// <param name="openMode">
        /// <para>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</para>
        /// </param>
        /// <param name="partition">
        /// <para>The <see cref="System.Fabric.IStatefulServicePartition" /> information for this replica.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Threading.Tasks.Task{T}" />&lt;<see cref="System.Fabric.IReplicator" />&gt;, 
        /// the <see cref="System.Fabric.IReplicator" /> that is used by the stateful service. To use the Service Fabric implementation, 
        /// in <see cref="System.Fabric.IStatefulServiceReplica.OpenAsync(System.Fabric.ReplicaOpenMode,System.Fabric.IStatefulServicePartition,System.Threading.CancellationToken)" />, 
        /// the replica should return a <see cref="System.Fabric.FabricReplicator" /> that is obtained 
        /// from  <see cref="System.Fabric.IStatefulServicePartition.CreateReplicator(System.Fabric.IStateProvider,System.Fabric.ReplicatorSettings)" />.</para>
        /// </returns>
        Task<IReplicator> OpenAsync(ReplicaOpenMode openMode, IStatefulServicePartition partition, CancellationToken cancellationToken);

        /// <summary>
        /// <para>Changes the role of the service replica to one of the <see cref="System.Fabric.ReplicaRole"/>. </para>
        /// </summary>
        /// <param name="newRole">
        /// <para>The updated <see cref="System.Fabric.ReplicaRole" /> that this replica should transition to.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. It can be used to send a notification that the operation should be canceled.
        /// Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Threading.Tasks.Task{T}" /> of type <see cref="System.String" />, the service’s new connection address that is to be associated
        /// with the replica via Service Fabric Naming.</para>
        /// </returns>
        /// <remarks>
        /// <para>The new role is indicated as a parameter. When the service transitions to the new role, the service has a chance to update its current listening address.
        /// The listening address is the address where clients connect to it and the one returned via the 
        /// <see href="https://docs.microsoft.com/dotnet/api/microsoft.servicefabric.services.client.servicepartitionresolver#Microsoft_ServiceFabric_Services_Client_ServicePartitionResolver_ResolveAsync_System_Fabric_ResolvedServicePartition_System_Threading_CancellationToken_">ResolveAsync</see> API. 
        /// This enables the service when it is a primary replica to only claim some resources such as ports when communication from clients is expected.</para>
        /// <seealso href="https://docs.microsoft.com/azure/service-fabric/service-fabric-reliable-services-communication"/>
        /// </remarks>
        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task<string> ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken);

        /// <summary>
        /// <para>Closes the service replica gracefully when it is being shut down.</para>
        /// </summary>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. It can be used to send a notification that the operation should be canceled. 
        /// Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Threading.Tasks.Task" />.</para>
        /// </returns>
        Task CloseAsync(CancellationToken cancellationToken);

        /// <summary>
        /// <para>Ungracefully terminates the service replica.</para>
        /// </summary>
        /// <remarks>
        /// <para>Network issues resulting in Service Fabric process shutdown and the use of <see cref="System.Fabric.IServicePartition.ReportFault(System.Fabric.FaultType)" /> 
        /// to report a <see cref="System.Fabric.FaultType.Permanent" /> fault are examples of ungraceful termination. When this method is invoked, 
        /// the service replica should immediately release and clean up all references and return.</para>
        /// </remarks>
        void Abort();
    }
}
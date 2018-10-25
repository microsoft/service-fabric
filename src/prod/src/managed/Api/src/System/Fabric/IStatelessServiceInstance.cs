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
    /// <para>Defines behavior that governs the lifecycle of a stateless service instance, such as startup, initialization, and shutdown. </para>
    /// <remarks>
    /// Stateless service types must implement this interface. The <see href="https://docs.microsoft.com/dotnet/api/microsoft.servicefabric.services.runtime.statelessservice">Reliable Stateless service</see> implements this interface and handles instance lifecycle internally.
    /// </remarks>
    /// </summary>
    public interface IStatelessServiceInstance 
    {
        /// <summary>
        /// <para> Initializes a newly created service instance.</para>
        /// </summary>
        /// <param name="initializationParameters">
        /// <para>The <see cref="System.Fabric.StatelessServiceInitializationParameters" /> for this service.</para>
        /// </param>
        void Initialize(StatelessServiceInitializationParameters initializationParameters);

        /// <summary>
        /// <para>Opens an initialized service instance so that it can be contacted by clients.</para>
        /// </summary>
        /// <param name="partition">
        /// <para>
        ///     The <see cref="System.Fabric.IStatelessServicePartition" /> that this instance is associated with</para>        
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. It can be used to send a notification that the operation
        /// should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Threading.Tasks.Task{T}" /> of type <see cref="System.String" />.</para>
        /// </returns>
        /// <remarks>
        /// <para>Opening an instance stateless service indicates that the service is now resolvable and discoverable by service clients. The string that is returned
        /// is the address of this service instance. The address is associated with the service name via Service Fabric naming and returned to clients 
        /// that resolve the service via <see cref="System.Fabric.FabricClient.ServiceManagementClient.ResolveServicePartitionAsync(Uri)" />.</para>
        /// </remarks>
        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task<string> OpenAsync(IStatelessServicePartition partition, CancellationToken cancellationToken);

        /// <summary>
        /// <para>Closes this service instance gracefully when the service instance is being shut down.</para>
        /// </summary>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. It can be used to send a notification
        /// that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Threading.Tasks.Task" />.</para>
        /// </returns>
        Task CloseAsync(CancellationToken cancellationToken);

        /// <summary>
        /// <para> Terminates this instance ungracefully with this synchronous method call. </para>
        /// </summary>
        /// <remarks>
        /// <para>Examples of ungraceful termination are network issues resulting in Service Fabric process shutdown and the use of 
        /// <see cref="System.Fabric.IServicePartition.ReportFault(System.Fabric.FaultType)" /> to report a <see cref="System.Fabric.FaultType.Permanent" /> fault. 
        /// When the service instance receives this method, it should immediately release and clean up all references and return.</para>
        /// </remarks>
        void Abort();
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Services
{
    using System.Fabric;
    using System.Fabric.Data.Replicator;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Extensions to the replica interface. It contains commonly used methods 
    /// that are to be implemented by a service replica class.
    /// </summary>
    public abstract class IReplicatedService
    {
        /// <summary>
        /// Create the state provider broker in this method. Typically this is the top state provider.
        /// It is guaranteed that the InitializationParameters are available.
        /// </summary>
        /// <returns></returns>
        protected abstract IStateProviderBroker CreateStateProviderBroker();

        /// <summary>
        /// Return ReplicatorSettings in this method.
        /// It is guaranteed that when this is called InitializationParameters and the Partition are available.
        /// </summary>
        /// <returns></returns>
        protected abstract ReplicatorSettings ReplicatorSettings { get; }

        /// <summary>
        /// Return ReplicatorSettings in this method.
        /// It is guaranteed that when this is called InitializationParameters and the Partition are available.
        /// </summary>
        /// <returns></returns>
        protected abstract ReplicatorLogSettings ReplicatorLogSettings { get; }

        /// <summary>
        /// Return the endpoint for the fabric client to resolve. Depeding on the replica role
        /// the endpoint may or may not be empty.
        /// </summary>
        /// <param name="role">Replica role used to determine if an endpoint should be opened.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        protected abstract Task<string> GetEndpointAsync(ReplicaRole role, CancellationToken cancellationToken);

        /// <summary>
        /// Called during replica initialization.
        /// </summary>
        /// <param name="initializationParameters">Service intialization parameters.</param>
        protected abstract void OnInitialize(StatefulServiceInitializationParameters initializationParameters);

        /// <summary>
        /// Called during replica OpenAsync.
        /// The derived class should perform its initialization in this method.
        /// </summary>
        /// <param name="openMode">Replica open mode.</param>
        /// <param name="partition">Stateful partition object.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        protected abstract Task OnOpenAsync(ReplicaOpenMode openMode, CancellationToken cancellationToken);

        /// <summary>
        /// Called during replica ChangeRoleAsync.
        /// </summary>
        /// <param name="newRole">New replica role.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        protected abstract Task OnChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken);

        /// <summary>
        /// Called during replica CloseAsync.
        /// The derived class should gracefully cleanup its resources.
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        protected abstract Task OnCloseAsync(CancellationToken cancellationToken);

        /// <summary>
        /// Called during replica Abort.
        /// The derived class should attempt to immediately cleanup its resources.
        /// </summary>
        protected abstract void OnAbort();
    }
}
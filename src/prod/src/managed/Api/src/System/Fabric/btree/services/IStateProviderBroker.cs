// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Services
{
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric;
    using System.Fabric.Data.Replicator;

    /// <summary>
    /// Interface usd to manipulate the state provider by the hosting stateful replica.
    /// </summary>
    public interface IStateProviderBroker
    {
        /// <summary>
        /// Initialization of the state provider broker.
        /// </summary>
        /// <param name="initializationParameters">State provider initialization data.</param>
        void Initialize(StatefulServiceInitializationParameters initializationParameters);

        /// <summary>
        /// Opens the state provider when the replica opens.
        /// </summary>
        /// <param name="openMode">Replica is either new or existent.</param>
        /// <param name="statefulPartition">Partition hosting this state provider.</param>
        /// <param name="stateReplicator">Replicator used by this state provider.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        Task OpenAsync(
            ReplicaOpenMode openMode,
            IStatefulServicePartitionEx statefulPartition,
            IAtomicGroupStateReplicatorEx stateReplicator,
            CancellationToken cancellationToken);

        /// <summary>
        /// Called on the state provider when the replica is changing role.
        /// </summary>
        /// <param name="newRole">New role for the state provider.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        Task ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken);

        /// <summary>
        /// Called on the state provider when the replica is being closed.
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        Task CloseAsync(CancellationToken cancellationToken);

        /// <summary>
        /// Called on the state provider when the replica is being aborted.
        /// </summary>
        void Abort();
    }
}
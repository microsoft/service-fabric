// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// This is for internal use only.
    /// Interface for state providers.
    /// </summary>
    /// <devnote>The interface name needs to change.</devnote>
    internal interface IStateProvider2 : IReliableState
    {
        /// <summary>
        /// Gets Initialization context.
        /// </summary>
        byte[] InitializationContext { get; }

        /// <summary>
        /// Called when replica is being aborted.
        /// </summary>
        void Abort();

        /// <summary>
        /// Applies operation.
        /// </summary>
        /// <param name="lsn">replication sequence number.</param>
        /// <param name="transactionBase">transaction that this operation is a part of.</param>
        /// <param name="metaData">metadata bytes pertaining to the operation.</param>
        /// <param name="data">undo or redo bytes pertaining to the operation.</param>
        /// <param name="applyContext">apply context.</param>
        /// <returns>Operation context that unlock of this operation needs.</returns>
        Task<object> ApplyAsync(
            long lsn,
            TransactionBase transactionBase,
            OperationData metaData,
            OperationData data,
            ApplyContext applyContext);

        /// <summary>
        /// Backup the existing checkpoint state on local disk (if any) to the given directory.
        /// </summary>
        /// <param name="backupDirectory">The directory where the checkpoint backup is to be stored.</param>
        /// <param name="cancellationToken">Request cancellation of the checkpoint backup.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task BackupCheckpointAsync(string backupDirectory, CancellationToken cancellationToken);

        /// <summary>
        /// Called at the start of set current state.
        /// </summary>
        void BeginSettingCurrentState();

        /// <summary>
        /// ChangeRole is called whenever the replica role is being changed.
        /// </summary>
        /// <param name="newRole">new role to be transition to.</param>
        /// <param name="cancellationToken">cancellation token.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken);

        /// <summary>
        /// Called when replica is being closed.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task CloseAsync();

        /// <summary>
        /// Completes checkpoint.
        /// </summary>
        /// <param name="cancellationToken">cancellation token.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task CompleteCheckpointAsync(CancellationToken cancellationToken);

        /// <summary>
        /// Called at the end of set state. This API will not get called if copy process gets aborted in the middle.
        /// </summary>
        Task EndSettingCurrentStateAsync();

        /// <summary>
        /// Get all the state providers contained within this state provider instance.
        /// </summary>
        /// <param name="name">Name of the parent state provider</param>
        /// <returns>Collection of state providers.</returns>
        IEnumerable<IStateProvider2> GetChildren(Uri name);

        /// <summary>
        /// Gets current copy state from primary.
        /// </summary>
        /// <returns>Operation data stream that contains the current stream.</returns>
        IOperationDataStream GetCurrentState();

        /// <summary>
        /// Initializes state provider.
        /// </summary>
        /// <param name="transactionalReplicator">Transactional Replicator instance.</param>
        /// <param name="name">name of the state provider.</param>
        /// <param name="initializationContext">initialization parameters</param>
        /// <param name="stateProviderId">id to uniquely identify state providers.</param>
        /// <param name="stateProviderWorkDirectory">Work directory for state provider checkpoint files.</param>
        /// <param name="childrenStateProviders">Children state providers.</param>
        void Initialize(
            TransactionalReplicator transactionalReplicator,
            Uri name,
            byte[] initializationContext,
            long stateProviderId,
            string stateProviderWorkDirectory,
            IEnumerable<IStateProvider2> childrenStateProviders);

        /// <summary>
        /// Called when data loss is encountered.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task OnDataLossAsync();

        /// <summary>
        /// Called when recovery is completed for the replica.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task OnRecoveryCompletedAsync();

        /// <summary>
        /// Called when replica is being opened.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task OpenAsync();

        /// <summary>
        /// Checkpoints state to local disk.
        /// </summary>
        /// <param name="performMode">Represents different mode to perform checkpoint.</param>
        /// <param name="cancellationToken">cancellation token.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task PerformCheckpointAsync(PerformCheckpointMode performMode, CancellationToken cancellationToken);

        /// <summary>
        /// Prepares for checkpoint by taking a snapshot of the state to be checkpointed.
        /// </summary>
        /// <param name="checkpointLSN">Checkpoint logical sequence number.</param>
        /// <param name="cancellationToken">cancellation token.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task PrepareCheckpointAsync(long checkpointLSN, CancellationToken cancellationToken);

        /// <summary>
        /// Prepares for removing state provider.
        /// </summary>
        /// <param name="transaction">transaction that this operation is a part of.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task PrepareForRemoveAsync(Transaction transaction, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Recovers state from checkpointed state.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task RecoverCheckpointAsync();

        /// <summary>
        /// Remove state.
        /// </summary>
        /// <param name="stateProviderId"> Id that uniquely identifies a state provider.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task RemoveStateAsync(long stateProviderId);

        /// <summary>
        /// Restore the checkpoint state from the given directory.
        /// </summary>
        /// <param name="backupDirectory">The directory where the checkpoint backup is stored.</param>
        /// <param name="cancellationToken">Request cancellation of the checkpoint restore.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        /// <remarks>
        /// The previously backed up checkpoint state becomes the current checkpoint state.  The
        /// checkpoint is not recovered from.
        /// </remarks>
        Task RestoreCheckpointAsync(string backupDirectory, CancellationToken cancellationToken);

        /// <summary>
        /// Sets the state on secondary.
        /// </summary>
        /// <param name="stateRecordNumber">record number.</param>
        /// <param name="data">data that needs to be copied to the secondary.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task SetCurrentStateAsync(long stateRecordNumber, OperationData data);

        /// <summary>
        /// Releases all the locks acquired as a part of a transaction.
        /// </summary>
        /// <param name="state">state that represents lock context.</param>
        void Unlock(object state);
    }
}
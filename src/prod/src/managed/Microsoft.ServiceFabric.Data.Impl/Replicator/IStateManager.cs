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
    using Microsoft.ServiceFabric.Data.Notifications;
    using ReplicatorTransaction = Microsoft.ServiceFabric.Replicator.Transaction;
    using ReplicatorTransactionBase = Microsoft.ServiceFabric.Replicator.TransactionBase;

    /// <summary>
    /// Component that performs state replication of transactions and supports state provider management
    /// </summary>
    internal interface IStateManager : ISerializationManager
    {
        /// <summary>
        /// EventHandler for publishing changes in transactions.
        /// For example, notifying that the transaction committed.
        /// </summary>
        event EventHandler<NotifyStateManagerChangedEventArgs> StateManagerChanged;

        /// <summary>
        /// Gets a value indicating whether the service is currently in the OnDataLoss call.
        /// </summary>
        bool IsServiceOnDataLossInFlight { get; }

        /// <summary>
        /// Gets the logging replicator.
        /// </summary>
        ILoggingReplicator LoggingReplicator { get; }

        /// <summary>
        /// Gets replica role.
        /// </summary>
        ReplicaRole Role { get; }

        /// <summary>
        /// Gets the partition for service that has state.
        /// </summary>
        IStatefulServicePartition StatefulPartition { get; }

        /// <summary>
        /// Gets initialization parameters.
        /// </summary>
        StatefulServiceContext StatefulServiceContext { get; }

        /// <summary>
        /// Gets or sets the delegate that creates state provider.
        /// </summary>
        Func<Uri, Type, IStateProvider2> StateProviderFactory { get; set; }

        /// <summary>
        /// Cleans up resources.
        /// </summary>
        void Abort();

        /// <summary>
        /// Called when state providers need to replicate their operation and when local state needs to be replicated.
        /// </summary>
        void AddOperation(
            Transaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext,
            long stateProviderId);

        /// <summary>
        /// Called when state providers need to replicate their operation and when local state needs to be replicated.
        /// </summary>
        Task<long> AddOperationAsync(
            AtomicOperation atomicOperation,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext,
            long stateProviderId);

        /// <summary>
        /// Called when state providers need to replicate their redo only operation and when local state needs to be replicated.
        /// </summary>
        Task<long> AddOperationAsync(
            AtomicRedoOperation atomicRedoOperation,
            OperationData metaData,
            OperationData redo,
            object operationContext,
            long stateProviderId);

        /// <summary>
        /// Registers state provider with the state manager.
        /// </summary>
        /// <param name="transaction">transaction that this operation is a part of</param>
        /// <param name="stateProviderName">name of the state provider that needs to be registered</param>
        /// <param name="stateProvider">state provider that needs to be registered</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task AddStateProviderAsync(
            Transaction transaction,
            Uri stateProviderName,
            IStateProvider2 stateProvider,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task BackupAsync(string backupDirectory, BackupOption backupOption, CancellationToken cancellationToken);

        /// <summary>
        /// Calls state providers to indicate the end of sequence of a set current state providers. 
        /// </summary>
        void BeginSettingCurrentState();

        void BeginTransaction(
            Transaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext,
            long stateProviderId);

        Task<long> BeginTransactionAsync(
            Transaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext,
            long stateProviderId);

        /// <summary>
        /// ChangeRole is called whenever the replica role is being changed.
        /// </summary>
        Task ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken);

        /// <summary>
        /// Does clean up of replicator and calls close on all the registered state providers.
        /// </summary>
        Task CloseAsync();

        /// <summary>
        /// Complete checkpoint on all state providers
        /// </summary>
        Task CompleteCheckpointAsync();

        /// <summary>
        /// Factory method to create an atomic operation
        /// </summary>
        /// <returns>
        /// An object representing an atomic operation
        /// </returns>
        AtomicOperation CreateAtomicOperation();

        /// <summary>
        /// Factory method to create an atomic redo only operation
        /// </summary>
        /// <returns>
        /// An object representing an atomic redo only operation
        /// </returns>
        AtomicRedoOperation CreateAtomicRedoOperation();

        /// <summary>
        /// Gets the list of state providers.
        /// </summary>
        IEnumerable<IStateProvider2> CreateEnumerable(bool parentsOnly, bool includeSystemStateProviders);

        /// <summary>
        /// Factory method to create transaction
        /// </summary>
        /// <returns>
        /// An object representing a transaction 
        /// </returns>
        Transaction CreateTransaction();

        /// <summary>
        /// Calls state providers to indicate the end of sequence of a set current state providers. This API will not get called if copy process gets aborted in the middle.
        /// </summary>
        Task EndSettingCurrentStateAsync();

        /// <summary>
        /// Gets copy context.
        /// </summary>
        IOperationDataStream GetCopyContext();

        /// <summary>
        /// Gets copy state.
        /// </summary>
        IOperationDataStream GetCopyState(long upToSequenceNumber, IOperationDataStream copyContext);

        /// <summary>
        /// Gets current data loss number.
        /// </summary>
        long GetCurrentDataLossNumber();

        /// <summary>
        /// Gets copy stream from state providers.
        /// </summary>
        IOperationDataStream GetCurrentState();

        /// <summary>
        /// Gets last committed lsn.
        /// </summary>
        long GetLastCommittedSequenceNumber();

        /// <summary>
        /// Gets or Adds state provider.
        /// </summary>
        /// <param name="transaction">transaction that this operation is a part of.</param>
        /// <param name="stateProviderName">name of the state provider that needs to be registered.</param>
        /// <param name="stateProviderFactory">state provider factory that creates the state provider that needs to be registered.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>Registered state provider.</returns>
        Task<ConditionalValue<T>> GetOrAddStateProviderAsync<T>(
            Transaction transaction, 
            Uri stateProviderName, 
            Func<Uri, T> stateProviderFactory, 
            TimeSpan timeout, 
            CancellationToken cancellationToken) where T : IStateProvider2;

        /// <summary>
        /// Initializes the state manager
        /// </summary>
        /// <returns></returns>
        void Initialize(StatefulServiceContext statefulServiceContext, IStatefulServicePartition partition);

        /// <summary>
        /// Commits transaction.
        /// </summary>
        Task<object> OnApplyAsync(
            long lsn,
            TransactionBase transaction,
            OperationData metaData,
            OperationData data,
            ApplyContext applyContext);

        /// <summary>
        /// Called when dataloss is encountered to recover from backup or proceed with current data.
        /// </summary>
        Task OnDataLossAsync(CancellationToken cancellationToken, bool shouldRestore);

        /// <summary>
        /// Calls recovery completed on all the registered state providers.
        /// </summary>
        Task OnRecoveryCompletedAsync();

        /// <summary>
        /// Initializes resources necessary for the replicator and for all the state providers.
        /// </summary>
        Task OpenAsync(ReplicaOpenMode openMode, TransactionalReplicatorSettings replicatorSettings);

        /// <summary>
        /// Perform checkpoint on all state providers
        /// </summary>
        Task PerformCheckpointAsync(PerformCheckpointMode performMode);

        /// <summary>
        /// Prepare checkpoint on all state providers
        /// </summary>
        /// <param name="checkpointLSN">Checkpoint logical sequence number.</param>
        Task PrepareCheckpointAsync(long checkpointLSN);

        /// <summary>
        /// Unregisters state provider.
        /// </summary>
        /// <param name="transaction">transaction that this operation is a part of.</param>
        /// <param name="stateProviderName">name of the state provider that needs to be unregistered.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task RemoveStateProviderAsync(
            Transaction transaction,
            Uri stateProviderName,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task RestoreAsync(string backupDirectory, CancellationToken cancellationToken);

        /// <summary>
        /// Send current state to the state providers.
        /// </summary>
        Task SetCurrentStateAsync(long stateRecordNumber, OperationData data);

        /// <summary>
        /// Throws if state provider is not registered
        /// </summary>
        /// <param name="stateProviderId"> Id of the state provider to check</param>
        /// <param name="transactionId"> transaction id of the transaction</param>
        void ThrowIfStateProviderIsNotRegistered(long stateProviderId, long transactionId);
        
        /// <summary>
        /// Gets the given state provider registered with the state manager.
        /// </summary>
        /// <returns>State provider, if the given state provider is registered, else null.</returns>
        ConditionalValue<IStateProvider2> TryGetStateProvider(Uri stateProviderName);

        /// <summary>
        /// Unlocks resources after transaction.
        /// </summary>
        void Unlock(object state);

        /// <summary>
        /// Unlocks operation context when single operation transaction is aborted
        /// </summary>
        /// <param name="stateProviderId"> Name of the state provider to check</param>
        /// <param name="state"> context</param>
        void SingleOperationTransactionAbortUnlock(long stateProviderId, object state);

        /// <summary>
        /// Called upon reconfiguration.
        /// </summary>
        Task UpdateEpochAsync(Epoch epoch, long previousEpochLastSequenceNumber, CancellationToken cancellationToken);
    }
}
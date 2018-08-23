// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Replicator.Diagnostics;

    /// <summary>
    /// Interface that performs state replication of transactions and supports state provider management.
    /// </summary>
    internal interface ITransactionalReplicator : ITransactionVersionManager, IVersionManager
    {
        /// <summary>
        /// Gets the service context.
        /// </summary>
        StatefulServiceContext ServiceContext { get; }

        /// <summary>
        /// Gets a value indicating whether the replicator is snapshot readable.
        /// </summary>
        bool IsReadable { get; }

        /// <summary>
        /// Gets replica role.
        /// </summary>
        ReplicaRole Role { get; }

        /// <summary>
        /// Gets the partition for service that has state.
        /// </summary>
        IStatefulServicePartition StatefulPartition { get; }

        /// <summary>
        /// Gets the metric manager.
        /// </summary>
        IMetricManager MetricManager { get; }

        /// <summary>
        /// Aborts transaction.
        /// </summary>
        Task<long> AbortTransactionAsync(Transaction transaction);

        /// <summary>
        /// Called when state providers need to replicate their operation and when local state needs to be replicated.
        /// </summary>
        /// <devnote>Change the parameter form StateProviderName to id to fix the bug that AddOperation(takes in Name) 
        /// call on a state provider which has been removed but added again with same name, in this case the state provider 
        /// object has changed, but AddOperation call can not differential them. In the user point of view, the state provider 
        /// exists, but it is not the state provider seen before.</devnote>
        void AddOperation(
            Transaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext,
            long stateProviderId);

        /// <summary>
        /// Called when state providers need to replicate their atomic operation and when local state needs to be replicated.
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
        /// Gets or registers state provider with the transactional replicator.
        /// </summary>
        /// <param name="transaction">Transaction that this operation is a part of.</param>
        /// <param name="stateProviderName">Name of the state provider that needs to be registered.</param>
        /// <param name="stateProvider">State provider that needs to be registered.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that the operation should be canceled.</param>
        /// <exception cref="System.ArgumentException">Not supported state provider type or state provider is already present or state provider name is a reserved name.</exception>
        /// <exception cref="System.ArgumentNullException">Key is null or value is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">Specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Replica was transitioned out of primary role.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was canceled.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Replica was closed.</exception>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task AddStateProviderAsync(
            Transaction transaction,
            Uri stateProviderName,
            IStateProvider2 stateProvider,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Called when state providers indicate they need to begin a new transactional operation that will need to be replicated.
        /// </summary>
        void BeginTransaction(
            Transaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext,
            long stateProviderId);

        /// <summary>
        /// Called when state providers indicate they need to begin a new transactional operation that will need to be replicated.
        /// </summary>
        Task<long> BeginTransactionAsync(
            Transaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext,
            long stateProviderId);

        /// <summary>
        /// Commits transaction.
        /// </summary>
        Task<long> CommitTransactionAsync(Transaction transaction);

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
        /// Factory method to create a transaction
        /// </summary>
        /// <returns>
        /// An object representing a transaction 
        /// </returns>
        Transaction CreateTransaction();

        /// <summary>
        /// Gets current data loss number.
        /// </summary>
        /// <returns>Current data loss number.</returns>
        long GetCurrentDataLossNumber();

        /// <summary>
        /// Gets last committed logical sequence number.
        /// </summary>
        /// <returns>Last committed logical sequence number.</returns>
        long GetLastCommittedSequenceNumber();

        /// <summary>
        /// Gets last stable logical sequence number.  Any commit LSN less than or equal to this stable lsn will not be false-progressed.
        /// </summary>
        /// <returns>Last stable logical sequence number.</returns>
        long GetLastStableSequenceNumber();

        /// <summary>
        /// Retrieves or registers state provider with the transactional replicator.
        /// </summary>
        /// <param name="transaction">Transaction that this operation is a part of.</param>
        /// <param name="stateProviderName">Name of the state provider that needs to be registered.</param>
        /// <param name="stateProviderFactory">state provider factory that creates the state provider that needs to be registered.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that the operation should be canceled.</param>
        /// <exception cref="System.ArgumentException">Not supported state provider type or state provider is already present or state provider name is a reserved name.</exception>
        /// <exception cref="System.ArgumentNullException">Key is null or value is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">Specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Replica was transitioned out of primary role.</exception>
        /// <exception cref="System.Fabric.FabricNotReadableException">Replica is not in a readable state.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was canceled.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Replica was closed.</exception>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task<ConditionalValue<T>> GetOrAddStateProviderAsync<T>(
            Transaction transaction,
            Uri stateProviderName,
            Func<Uri, T> stateProviderFactory,
            TimeSpan timeout,
            CancellationToken cancellationToken) where T : IStateProvider2;

        /// <summary>
        /// Get state serializer.
        /// </summary>
        /// <typeparam name="T">Type for the serialization.</typeparam>
        /// <param name="name">Name of the state provider.</param>
        /// <returns>IStateSerializer of T.</returns>
        IStateSerializer<T> GetStateSerializer<T>(Uri name);

        /// <summary>
        /// Unregisters state provider.
        /// </summary>
        /// <param name="transaction">Transaction that this operation is a part of.</param>
        /// <param name="stateProviderName">State provider that needs to be unregistered.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that the operation should be canceled.</param>
        /// <exception cref="System.ArgumentException">StateProviderName does not exist.</exception>
        /// <exception cref="System.ArgumentNullException">StateProviderName is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">Specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Replica was transitioned out of primary role.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was canceled.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Replica was closed.</exception>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task RemoveStateProviderAsync(
            Transaction transaction,
            Uri stateProviderName,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Remove state serializer.
        /// </summary>
        /// <typeparam name="T">Type of the serializer.</typeparam>
        void RemoveStateSerializer<T>();

        /// <summary>
        /// Remove state serializer.
        /// </summary>
        /// <typeparam name="T">Type of the serializer.</typeparam>
        /// <param name="name">Name of the state provider.</param>
        void RemoveStateSerializer<T>(Uri name);

        /// <summary>
        /// Throws if state provider is not registered
        /// </summary>
        /// <param name="stateProviderId"></param>
        /// <param name="transactionId"></param>
        void ThrowIfStateProviderIsNotRegistered(long stateProviderId, long transactionId);

        /// <summary>
        /// Transaction unlocks the single operation context that was never passed into replicator
        /// </summary>
        /// <param name="stateProviderId"></param>
        /// <param name="state"></param>
        void SingleOperationTransactionAbortUnlock(long stateProviderId, object state);

        /// <summary>
        /// Add state serializer.
        /// </summary>
        /// <typeparam name="T">Type of the serializer.</typeparam>
        /// <param name="stateSerializer">The state serializer.</param>
        /// <returns>Boolean indicating whether state serializer was added.</returns>
        bool TryAddStateSerializer<T>(IStateSerializer<T> stateSerializer);

        /// <summary>
        /// Add state serializer.
        /// </summary>
        /// <typeparam name="T">Type of the serializer.</typeparam>
        /// <param name="name">Name of the state provider.</param>
        /// <param name="stateSerializer">The state serializer.</param>
        bool TryAddStateSerializer<T>(Uri name, IStateSerializer<T> stateSerializer);

        /// <summary>
        /// Gets the given state provider registered with the transactional replicator.
        /// </summary>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Replica was closed.</exception>
        /// <exception cref="System.ArgumentNullException">StateProviderName is null.</exception>
        /// <exception cref="System.Fabric.FabricNotReadableException">Replica is not in a readable state.</exception>
        /// <returns>state provider, if the given state provider is registered.</returns>
        ConditionalValue<IStateProvider2> TryGetStateProvider(Uri stateProviderName);
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Fabric;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Collections;
    using Microsoft.ServiceFabric.Data.Notifications;
    using Microsoft.ServiceFabric.Replicator.Diagnostics;
    using ReplicatorTransaction = Microsoft.ServiceFabric.Replicator.Transaction;
    using ReplicatorTransactionBase = Microsoft.ServiceFabric.Replicator.TransactionBase;

    /// <summary>
    /// This is for internal use only.
    /// Component that performs state replication of transactions and supports state provider management
    /// </summary>
    internal class TransactionalReplicator 
        : IStateProvider
        , ITransactionalReplicator
        , System.Fabric.Interop.NativeRuntimeInternal.IFabricStateProviderSupportsCopyUntilLatestLsn
    {
        /// <summary>
        /// Default cancellation token.
        /// </summary>
        internal readonly CancellationToken DefaultCancellationToken = CancellationToken.None;

        /// <summary>
        /// Default timeout.
        /// </summary>
        internal readonly TimeSpan DefaultTimeout = TimeSpan.FromSeconds(4);

        /// <summary>
        /// Service Initialization parameters
        /// </summary>
        private StatefulServiceContext statefulServiceContext;

        /// <summary>
        /// Metric manager instance.
        /// </summary>
        internal readonly IMetricManager metricManager;

        /// <summary>
        /// Is OnDataLoss call in flight in the service.
        /// </summary>
        private bool isServiceOnDataLossInFlight = false;

        /// <summary>
        /// Callback that is called in the service replica.
        /// </summary>
        private Func<CancellationToken, Task<bool>> onDataLossCallback;

        /// <summary>
        /// State manager instance.
        /// </summary>
        private IStateManager stateManager;

        /// <summary>
        ///  Initializes a new instance of the <see cref="TransactionalReplicator"/> class. 
        /// </summary>
        /// <param name="onDataLossCallback">On data loss callback. Called when a data loss is suspected.</param>
        internal TransactionalReplicator(Func<CancellationToken, Task<bool>> onDataLossCallback)
        {
            this.metricManager = new MetricManager(this);
            this.stateManager = new DynamicStateManager(this);
            this.onDataLossCallback = onDataLossCallback;
        }

        /// <summary>
        ///  Initializes a new instance of the <see cref="TransactionalReplicator"/> class for unit tests
        /// </summary>
        /// <param name="loggingReplicator">logging replicator</param>
        /// <param name="onDataLossCallback">On data loss callback. Called when a data loss is suspected.</param>
        internal TransactionalReplicator(ILoggingReplicator loggingReplicator, Func<CancellationToken, Task<bool>> onDataLossCallback)
        {
            this.metricManager = new MetricManager(this);
            this.stateManager = new DynamicStateManager(this, loggingReplicator);
            this.onDataLossCallback = onDataLossCallback;
        }

        public event EventHandler<NotifyStateManagerChangedEventArgs> StateManagerChanged
        {
            add { this.StateManager.StateManagerChanged += value; }

            remove { this.StateManager.StateManagerChanged -= value; }
        }

        public event EventHandler<NotifyTransactionChangedEventArgs> TransactionChanged
        {
            add { this.TransactionsManager.TransactionChanged += value; }

            remove { this.TransactionsManager.TransactionChanged -= value; }
        }

        /// <summary>
        /// Gets replica role.
        /// </summary>
        public bool IsReadable
        {
            get { return this.stateManager.LoggingReplicator.IsReadable; }
        }

        /// <summary>
        /// Gets replica role.
        /// </summary>
        public ReplicaRole Role
        {
            get { return this.stateManager.Role; }
        }

        /// <summary>
        /// Gets initialization parameters.
        /// </summary>
        public StatefulServiceContext ServiceContext
        {
            get { return this.statefulServiceContext; }
        }

        /// <summary>
        /// Gets the partition for service that has state.
        /// </summary>
        public IStatefulServicePartition StatefulPartition
        {
            get { return this.stateManager.StatefulPartition; }
        }

        /// <summary>
        /// Gets a value indicating whether OnDataLoss call is in flight in the service.
        /// </summary>
        internal bool IsServiceOnDataLossInFlight
        {
            get { return this.isServiceOnDataLossInFlight; }
        }

        /// <summary>
        /// Gets the instance of logging replicator.
        /// </summary>
        internal ILoggingReplicator LoggingReplicator
        {
            get { return this.stateManager.LoggingReplicator; }
        }

        /// <summary>
        /// Gets the instance of metric manager
        /// </summary>
        public IMetricManager MetricManager
        {
            get
            {
                return this.metricManager;
            }
        }

        /// <summary>
        /// Gets the instance of transactions manager.
        /// </summary>
        internal ITransactionManager TransactionsManager 
        {
            get { return this.stateManager.LoggingReplicator as ITransactionManager; }
        }

        /// <summary>
        /// Gets the instance of transactional replicator.
        /// </summary>
        internal IStateManager StateManager
        {
            get { return this.stateManager; }
        }

        /// <summary>
        /// Gets or sets the delegate that creates state provider.
        /// </summary>
        internal Func<Uri, Type, IStateProvider2> StateProviderFactory
        {
            get { return this.stateManager.StateProviderFactory; }

            set { this.stateManager.StateProviderFactory = value; }
        }

        /// <summary>
        /// Gets or registers state provider with the state manager.
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
        public async Task AddStateProviderAsync(
            Transaction transaction,
            Uri stateProviderName,
            IStateProvider2 stateProvider,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            await
                this.stateManager.AddStateProviderAsync(
                    transaction,
                    stateProviderName,
                    stateProvider,
                    timeout,
                    cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Registers state provider with the state manager.
        /// </summary>
        /// <param name="transaction">Transaction that this operation is a part of.</param>
        /// <param name="stateProviderName">Name of the state provider that needs to be registered.</param>
        /// <param name="stateProvider">State provider that needs to be registered.</param>
        /// <exception cref="System.ArgumentException">Not supported state provider type or state provider is already present or state provider name is a reserved name.</exception>
        /// <exception cref="System.ArgumentNullException">StateProviderName is null or StateProvider is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">Specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Replica was transitioned out of primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Replica was closed.</exception>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public async Task AddStateProviderAsync(
            Transaction transaction,
            Uri stateProviderName,
            IStateProvider2 stateProvider)
        {
            await
                this.stateManager.AddStateProviderAsync(
                    transaction,
                    stateProviderName,
                    stateProvider,
                    this.DefaultTimeout,
                    this.DefaultCancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Factory method to create an atomic operation
        /// </summary>
        /// <returns>
        /// An object representing an atomic operation
        /// </returns>
        public AtomicOperation CreateAtomicOperation()
        {
            return this.stateManager.CreateAtomicOperation();
        }

        /// <summary>
        /// Factory method to create an atomic redo only operation
        /// </summary>
        /// <returns>
        /// An object representing an atomic redo only operation
        /// </returns>
        public AtomicRedoOperation CreateAtomicRedoOperation()
        {
            return this.stateManager.CreateAtomicRedoOperation();
        }

        /// <summary>
        /// Gets state providers registered with the state manager.
        /// </summary>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Replica was closed.</exception>
        /// <exception cref="System.Fabric.FabricNotReadableException">Replica is not in a readable state.</exception>
        /// <returns>Collection of state providers.</returns>
        public IAsyncEnumerable<IStateProvider2> CreateAsyncEnumerable(bool parentsOnly, bool includeSystemStateProviders)
        {
            return this.stateManager.CreateEnumerable(parentsOnly, includeSystemStateProviders).ToAsyncEnumerable();
        }

        /// <summary>
        /// Factory method to create a transaction
        /// </summary>
        /// <returns>
        /// An object representing a transaction 
        /// </returns>
        public Transaction CreateTransaction()
        {
            return this.stateManager.CreateTransaction();
        }

        /// <summary>
        /// Gets current data loss number.
        /// </summary>
        /// <returns>Current data loss number.</returns>
        public long GetCurrentDataLossNumber()
        {
            return this.stateManager.GetCurrentDataLossNumber();
        }

        /// <summary>
        /// Gets last committed logical sequence number..
        /// </summary>
        /// <returns>Last committed logical sequence number.</returns>
        public long GetLastCommittedSequenceNumber()
        {
            return this.stateManager.GetLastCommittedSequenceNumber();
        }

        /// <summary>
        /// Gets last stable logical sequence number.
        /// </summary>
        /// <returns>Last stable logical sequence number.</returns>
        public long GetLastStableSequenceNumber()
        {
            return this.LoggingReplicator.GetLastStableSequenceNumber();
        }

        /// <summary>
        /// Retrieves or registers state provider with the state manager.
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
        public Task<ConditionalValue<T>> GetOrAddStateProviderAsync<T>(
            Transaction transaction, 
            Uri stateProviderName,
            Func<Uri, T> stateProviderFactory,
            TimeSpan timeout,
            CancellationToken cancellationToken) where T : IStateProvider2
        {
            return this.stateManager.GetOrAddStateProviderAsync<T>(
                transaction,
                stateProviderName,
                stateProviderFactory,
                timeout,
                cancellationToken);
        }

        /// <summary>
        /// Get state serializer.
        /// </summary>
        /// <typeparam name="T">Type for the serialization.</typeparam>
        /// <param name="name">Name of the state provider.</param>
        /// <returns>IStateSerializer of T.</returns>
        public IStateSerializer<T> GetStateSerializer<T>(Uri name)
        {
            return this.stateManager.GetStateSerializer<T>(name);
        }

        public Task<long> RegisterAsync()
        {
            return this.LoggingReplicator.RegisterAsync();
        }

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
        public async Task RemoveStateProviderAsync(
            Transaction transaction,
            Uri stateProviderName,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            await this.stateManager.RemoveStateProviderAsync(transaction, stateProviderName, timeout, cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Unregisters state provider.
        /// </summary>
        /// <param name="transaction">Transaction that this operation is a part of.</param>
        /// <param name="stateProviderName">State provider that needs to be unregistered.</param>
        /// <exception cref="System.ArgumentException">StateProviderName does not exist.</exception>
        /// <exception cref="System.ArgumentNullException">StateProviderName is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">Specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Replica was transitioned out of primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Replica was closed.</exception>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public async Task RemoveStateProviderAsync(Transaction transaction, Uri stateProviderName)
        {
            await
                this.stateManager.RemoveStateProviderAsync(
                    transaction,
                    stateProviderName,
                    this.DefaultTimeout,
                    this.DefaultCancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Remove state serializer.
        /// </summary>
        /// <typeparam name="T">Type of the serializer.</typeparam>
        public void RemoveStateSerializer<T>()
        {
            this.stateManager.RemoveStateSerializer<T>();
        }

        /// <summary>
        /// Remove state serializer.
        /// </summary>
        /// <typeparam name="T">Type of the serializer.</typeparam>
        /// <param name="name">Name of the state provider.</param>
        public void RemoveStateSerializer<T>(Uri name)
        {
            this.stateManager.RemoveStateSerializer<T>(name);
        }

        /// <summary>
        /// Add state serializer.
        /// </summary>
        /// <typeparam name="T">Type of the serializer.</typeparam>
        /// <param name="stateSerializer">The state serializer.</param>
        /// <returns>Boolean indicating whether state serializer was added.</returns>
        public bool TryAddStateSerializer<T>(IStateSerializer<T> stateSerializer)
        {
            return this.stateManager.TryAddStateSerializer(stateSerializer);
        }

        /// <summary>
        /// Add state serializer.
        /// </summary>
        /// <typeparam name="T">Type of the serializer.</typeparam>
        /// <param name="name">Name of the state provider.</param>
        /// <param name="stateSerializer">The state serializer.</param>
        public bool TryAddStateSerializer<T>(Uri name, IStateSerializer<T> stateSerializer)
        {
            return this.stateManager.TryAddStateSerializer(name, stateSerializer);
        }

        /// <summary>
        /// Gets the given state provider registered with the state manager.
        /// </summary>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Replica was closed.</exception>
        /// <exception cref="System.ArgumentNullException">StateProviderName is null.</exception>
        /// <exception cref="System.Fabric.FabricNotReadableException">Replica is not in a readable state.</exception>
        /// <returns>sate provider, if the given state provider is registered.</returns>
        public ConditionalValue<IStateProvider2> TryGetStateProvider(Uri stateProviderName)
        {
            return this.stateManager.TryGetStateProvider(stateProviderName);
        }

        /// <summary>
        /// Called to determine whether relevant checkpoint file can be removed
        /// </summary>
        /// <param name="checkpointLsnToBeRemoved">Version of the checkpoint in question.</param>
        /// <param name="nextcheckpointLsn">Lsn of the next checkpoint after the checkpoint in question.</param>
        /// <returns>
        /// Task that represents when it can be removed.
        /// If immediately, null will be returned.
        /// Otherwise, the task will fire when the checkpoint can be removed.
        /// </returns>
        public Task TryRemoveCheckpointAsync(long checkpointLsnToBeRemoved, long nextcheckpointLsn)
        {
            return this.LoggingReplicator.TryRemoveCheckpointAsync(checkpointLsnToBeRemoved, nextcheckpointLsn);
        }

        /// <summary>
        /// Called to determine whether relevant version is currently being referenced by a snapshot transaction.
        /// If so, notification tasks will be returned for visibility sequence number completions that state provider has not registered yet.
        /// </summary>
        /// <param name="stateProviderId">State Provider Id of the caller.</param>
        /// <param name="commitSequenceNumber">Version in question.</param>
        /// <param name="nextCommitSequenceNumber">Next Version after the version in question.</param>
        /// <returns>TryRemoveVersionResult that indicates whether version can be removed, which snapshot visibility numbers are referencing it and relevant notifications.</returns>
        public TryRemoveVersionResult TryRemoveVersion(
            long stateProviderId,
            long commitSequenceNumber,
            long nextCommitSequenceNumber)
        {
            return this.LoggingReplicator.TryRemoveVersion(
                stateProviderId,
                commitSequenceNumber,
                nextCommitSequenceNumber);
        }

        public void UnRegister(long visibilityVersionNumber)
        {
            this.LoggingReplicator.UnRegister(visibilityVersionNumber);
        }

        /// <summary>
        /// Gets copy context.
        /// </summary>
        IOperationDataStream IStateProvider.GetCopyContext()
        {
            return this.stateManager.GetCopyContext();
        }

        /// <summary>
        /// Gets copy state.
        /// </summary>
        IOperationDataStream IStateProvider.GetCopyState(long upToSequenceNumber, IOperationDataStream copyContext)
        {
            return this.stateManager.GetCopyState(upToSequenceNumber, copyContext);
        }

        /// <summary>
        /// Called when a suspected data loss is encountered.
        /// </summary>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// Task that contains boolean. Boolean represents whether any of the state providers recovered their state externally.
        /// In such cases, GetSequenceNumber should be called on the state providers that returned true.
        /// </returns>
        async Task<bool> IStateProvider.OnDataLossAsync(CancellationToken cancellationToken)
        {
            var shouldRestore = false;
            this.isServiceOnDataLossInFlight = true;

            try
            {
                // TODO: Monitor onDataLossCallback duration
                // Do not handle the exception. If the service throws let that bubble up.
                shouldRestore = await this.onDataLossCallback.Invoke(cancellationToken).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                Utility.Assert(null != this.LoggingReplicator, "OnDataLossAsync: LoggingReplicator is null");
                Utility.Assert(
                    null != this.LoggingReplicator.Tracer,
                    "OnDataLossAsync: LoggingReplicator.Tracer is null");
                Utility.Assert(null != this.statefulServiceContext, "OnDataLossAsync: StatefulServiceContext is null");

                var type = string.Format("StateManager.ApiError@{0}", this.statefulServiceContext.PartitionId);
                var message = string.Format(
                    CultureInfo.InvariantCulture,
                    "Replica {0} threw following exception at OnDataLoss: {1}",
                    this.statefulServiceContext.ReplicaId,
                    e);

                // TODO: Structured Tracing. Trace TransactionalReplicator.ApiError@<pid>, t.Exception.InnerException.
                this.LoggingReplicator.Tracer.WriteInfo(type, message);

                throw;
            }
            finally
            {
                this.isServiceOnDataLossInFlight = false;
            }

            await this.stateManager.OnDataLossAsync(cancellationToken, shouldRestore).ConfigureAwait(false);

            return shouldRestore;
        }

        /// <summary>
        /// Called upon reconfiguration.
        /// </summary>
        Task IStateProvider.UpdateEpochAsync(
            Epoch epoch,
            long previousEpochLastSequenceNumber,
            CancellationToken cancellationToken)
        {
            return this.stateManager.UpdateEpochAsync(epoch, previousEpochLastSequenceNumber, cancellationToken);
        }

        /// <summary>
        /// Aborts transaction.
        /// </summary>
        Task<long> ITransactionalReplicator.AbortTransactionAsync(Transaction transaction)
        {
            return this.TransactionsManager.AbortTransactionAsync(transaction);
        }

        /// <summary>
        /// Called when state providers need to replicate their operation and when local state needs to be replicated.
        /// </summary>
        void ITransactionalReplicator.AddOperation(
            Transaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext,
            long stateProviderId)
        {
            this.stateManager.AddOperation(transaction, metaData, undo, redo, operationContext, stateProviderId);
        }

        /// <summary>
        /// Called when state providers need to replicate their atomic operation and when local state needs to be replicated.
        /// </summary>
        Task<long> ITransactionalReplicator.AddOperationAsync(
            AtomicOperation atomicOperation,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext,
            long stateProviderId)
        {
            return this.stateManager.AddOperationAsync(
                atomicOperation,
                metaData,
                undo,
                redo,
                operationContext,
                stateProviderId);
        }

        /// <summary>
        /// Called when state providers need to replicate their redo only operation and when local state needs to be replicated.
        /// </summary>
        Task<long> ITransactionalReplicator.AddOperationAsync(
            AtomicRedoOperation atomicRedoOperation,
            OperationData metaData,
            OperationData redo,
            object operationContext,
            long stateProviderId)
        {
            return this.stateManager.AddOperationAsync(
                atomicRedoOperation,
                metaData,
                redo,
                operationContext,
                stateProviderId);
        }

        /// <summary>
        /// Called when state providers indicate they need to begin a new transactional operation that will need to be replicated.
        /// </summary>
        void ITransactionalReplicator.BeginTransaction(
            Transaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext,
            long stateProviderId)
        {
            this.stateManager.BeginTransaction(transaction, metaData, undo, redo, operationContext, stateProviderId);
        }

        Task<long> ITransactionalReplicator.BeginTransactionAsync(
            Transaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext,
            long stateProviderId)
        {
            return this.stateManager.BeginTransactionAsync(
                transaction,
                metaData,
                undo,
                redo,
                operationContext,
                stateProviderId);
        }

        /// <summary>
        /// Commits transaction.
        /// </summary>
        Task<long> ITransactionalReplicator.CommitTransactionAsync(Transaction transaction)
        {
            return this.TransactionsManager.CommitTransactionAsync(transaction);
        }

        void ITransactionalReplicator.ThrowIfStateProviderIsNotRegistered(long stateProviderId, long transactionId)
        {
            this.stateManager.ThrowIfStateProviderIsNotRegistered(stateProviderId, transactionId);
        }

        void ITransactionalReplicator.SingleOperationTransactionAbortUnlock(long stateProviderId, object state)
        {
            this.stateManager.SingleOperationTransactionAbortUnlock(stateProviderId, state);
        }

        /// <summary>
        /// Cleans up resources.
        /// </summary>
        internal void Abort()
        {
            this.stateManager.Abort();
        }

        /// <summary>
        /// Creates a backup of the transactional replicator and all registered state providers.
        /// The default backupOption is FULL.
        /// The default timeout is 1 hour.
        /// The default cancellationToken is None.
        /// </summary>
        /// <param name="backupCallback">Method that will be called once the local backup folder has been created.</param>
        /// <exception cref="FabricBackupInProgressException">Backup is in progress.</exception>
        /// <exception cref="FabricNotPrimaryException">Replicator role is not primary.</exception>
        /// <exception cref="ArgumentException">backup folder path is null.</exception>
        /// <exception cref="ArgumentNullException">backupCallbackAsync delegate is null.</exception>
        /// <exception cref="OperationCanceledException">Operation has been canceled.</exception>
        /// <exception cref="FabricTransientException">Retriable exception.</exception>
        /// <exception cref="TimeoutException">Operation has timed out.</exception>
        /// <returns>Task that represents the asynchronous backup.</returns>
        internal Task<BackupInfo> BackupAsync(Func<BackupInfo, CancellationToken, Task<bool>> backupCallback)
        {
            return this.stateManager.LoggingReplicator.BackupAsync(backupCallback);
        }

        /// <summary>
        /// Creates a backup of the transactional replicator and all registered state providers.
        /// </summary>
        /// <param name="backupCallback">Method that will be called once the local backup folder has been created.</param>
        /// <param name="backupOption">The backup option. </param>
        /// <param name="timeout">The timeout for the backup.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. </param>
        /// <exception cref="FabricBackupInProgressException">Backup is in progress.</exception>
        /// <exception cref="FabricNotPrimaryException">Replicator role is not primary.</exception>
        /// <exception cref="ArgumentException">backup folder path is null.</exception>
        /// <exception cref="ArgumentNullException">backupCallbackAsync delegate is null.</exception>
        /// <exception cref="OperationCanceledException">Operation has been canceled.</exception>
        /// <exception cref="FabricTransientException">Retriable exception.</exception>
        /// <exception cref="TimeoutException">Operation has timed out.</exception>
        /// <returns>Task that represents the asynchronous backup.</returns>
        internal Task<BackupInfo> BackupAsync(
            Func<BackupInfo, CancellationToken, Task<bool>> backupCallback,
            BackupOption backupOption, 
            TimeSpan timeout, 
            CancellationToken cancellationToken)
        {
            return this.stateManager.LoggingReplicator.BackupAsync(
                backupCallback,
                backupOption,
                timeout,
                cancellationToken);
        }

        /// <summary>
        /// Calls state providers to indicate the end of sequence of a set current state providers. 
        /// </summary>
        internal void BeginSettingCurrentState()
        {
            this.stateManager.BeginSettingCurrentState();
        }

        /// <summary>
        /// ChangeRole is called whenever the replica role is being changed.
        /// </summary>
        internal async Task ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            await this.stateManager.ChangeRoleAsync(newRole, cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Calls  clean on replicator and all the registered state providers.
        /// </summary>
        internal async Task CloseAsync()
        {
            await this.stateManager.CloseAsync().ConfigureAwait(false);
            await this.metricManager.StopTaskAsync().ConfigureAwait(false);
        }

        /// <summary>
        /// Complete checkpoint on all state providers
        /// </summary>
        internal async Task CompleteCheckpointAsync()
        {
            await this.stateManager.CompleteCheckpointAsync().ConfigureAwait(false);
        }

        /// <summary>
        /// Calls state providers to indicate the end of sequence of a set current state providers. 
        /// </summary>
        internal async Task EndSettingCurrentStateAsync()
        {
            await this.stateManager.EndSettingCurrentStateAsync().ConfigureAwait(false);
        }

        /// <summary>
        /// Gets copy stream from state providers.
        /// </summary>
        internal IOperationDataStream GetCurrentState()
        {
            return this.stateManager.GetCurrentState();
        }

        /// <summary>
        /// Initializes the Transactional Replicator.
        /// </summary>
        /// <param name="statefulServiceContext"></param>
        /// <param name="partition">service partition</param>
        /// <returns></returns>
        internal void Initialize(StatefulServiceContext statefulServiceContext, IStatefulServicePartition partition)
        {
            this.statefulServiceContext = statefulServiceContext;
            this.metricManager.Initialize(this.statefulServiceContext);
            this.stateManager.Initialize(this.statefulServiceContext, partition);
        }

        /// <summary>
        /// Calls recovery completed on all the registered state providers.
        /// </summary>
        internal async Task OnRecoveryCompletedAsync()
        {
            await this.stateManager.OnRecoveryCompletedAsync().ConfigureAwait(false);
        }

        /// <summary>
        /// Initializes resources necessary for the replicator and for all the state providers.
        /// </summary>
        internal async Task OpenAsync(
            ReplicaOpenMode openMode,
            TransactionalReplicatorSettings transactionalReplicatorSettings)
        {
            this.metricManager.StartTask();
            await this.stateManager.OpenAsync(openMode, transactionalReplicatorSettings).ConfigureAwait(false);
        }

        /// <summary>
        /// Perform checkpoint on all state providers
        /// </summary>
        internal async Task PerformCheckpointAsync()
        {
            await this.stateManager.PerformCheckpointAsync(PerformCheckpointMode.Default).ConfigureAwait(false);
        }

        /// <summary>
        /// Prepare checkpoint on all state providers
        /// </summary>
        /// <param name="checkpointLSN">Checkpoint logical sequence number.</param>
        internal async Task PrepareCheckpointAsync(long checkpointLSN)
        {
            await this.stateManager.PrepareCheckpointAsync(checkpointLSN).ConfigureAwait(false);
        }

        /// <summary>
        /// Restores the transactional replicator from the backup folder.
        /// The default restorePolicy is RestorePolicy.Safe.
        /// The default cancellationToken is None.
        /// </summary>
        /// <param name="backupFolder">
        /// The directory where the replica is to be restored from.
        /// This parameter cannot be null, empty or contain just whitespace. 
        /// UNC paths may also be provided.
        /// </param>
        /// <returns>Task that represents the asynchronous restore.</returns>
        internal Task RestoreAsync(string backupFolder)
        {
            return this.stateManager.LoggingReplicator.RestoreAsync(backupFolder);
        }

        /// <summary>
        /// Restores the transactional replicator from the backup folder.
        /// </summary>
        /// <param name="backupFolder">
        /// The directory where the replica is to be restored from.
        /// This parameter cannot be null, empty or contain just whitespace. 
        /// UNC paths may also be provided.
        /// </param>
        /// <param name="restorePolicy">The restore policy.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous restore.</returns>
        internal Task RestoreAsync(
            string backupFolder,
            RestorePolicy restorePolicy,
            CancellationToken cancellationToken)
        {
            return this.stateManager.LoggingReplicator.RestoreAsync(backupFolder, restorePolicy, cancellationToken);
        }

        /// <summary>
        /// Send current state to the state providers.
        /// </summary>
        internal async Task SetCurrentStateAsync(long stateRecordNumber, OperationData data)
        {
            await this.stateManager.SetCurrentStateAsync(stateRecordNumber, data).ConfigureAwait(false);
        }
    }
}
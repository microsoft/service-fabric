// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Collections;
    using Microsoft.ServiceFabric.Data.Notifications;
    using ReplicatorTransaction = Microsoft.ServiceFabric.Replicator.Transaction;
    using ReplicatorTransactionBase = Microsoft.ServiceFabric.Replicator.TransactionBase;

    /// <summary>
    /// Dynamic component of the state manager
    /// </summary>
    internal class DynamicStateManager : IStateManager, IDisposable
    {
        /// <summary>
        /// State manager id that is used for consistency during replication.
        /// </summary>
        public const long EmptyStateProviderId = 0;

        /// <summary>
        /// Name prefix which is reserved for system state providers
        /// </summary>
        public readonly Uri SystemStateProviderNamePrefix = new Uri("urn:c0e554a9-5936-4655-b175-46b6f969549f");

        /// <summary>
        /// State manager id that is used for consistency during replication.
        /// </summary>
        /// <devnote>Making it a min to avoid colliding with FileTimeUTC, it would take a 1000+ years before it rolls over. </devnote>
        internal const long StateManagerId = long.MinValue;

        /// <summary>
        /// Empty list.
        /// </summary>
        internal static readonly IReadOnlyList<Metadata> EmptyList = new List<Metadata>();

        /// <summary>
        /// State manager name
        /// </summary>
        internal static readonly Uri StateManagerName = new Uri("fabric:/StateManager");

        internal static readonly string NativeCurrentCheckpointPrefix = "StateManager.cpt";
        internal static readonly string NativeTempCheckpointPrefix = "StateManager.tmp";
        internal static readonly string NativeBackupCheckpointPrefix = "StateManager.bkp";

        internal static readonly string CurrentCheckpointSuffix = "StateManager";
        internal static readonly string TempCheckpointSuffix = "StateManager_tmp";
        internal static readonly string BackupCheckpointSuffix = "StateManager_bkp";

        /// <summary>
        /// Version that represents on disk format and wire format.
        /// </summary>
        private const int CurrentVersion = 1;

        /// <summary>
        /// Max used for exponential back-off for replication operations.
        /// </summary>
        private const int ExponentialBackoffMax = 4*1024;

        /// <summary>
        /// Serializer dictionary.
        /// </summary>
        private readonly SerializationManager serializationManager = new SerializationManager();

        /// <summary>
        /// Transactional Replicator instance that wraps the dynamic transactional replicator.
        /// </summary>
        private readonly TransactionalReplicator transactionalReplicator;

        /// <summary>
        /// The logging replicator.
        /// </summary>
        private readonly ILoggingReplicator loggingReplicator;

        /// <summary>
        /// The transactions manager 
        /// </summary>
        private readonly ITransactionManager transactionsManager;

        /// <summary>
        /// Dispatches APIs to the State Provider.
        /// </summary>
        private readonly ApiDispatcher apiDispatcher;

        /// <summary>
        /// Lock that synchronizes change role calls that is dispatched to state providers on create and during its life time.
        /// </summary>
        private SemaphoreSlim changeRoleLock;

        /// <summary>
        /// Set to track state providers for whom full copy is in progress.
        /// </summary>
        private HashSet<IStateProvider2> copyProgressSet;

        /// <summary>
        /// Replica role of state providers.
        /// </summary>
        private ReplicaRole currentRoleOfStateProviders;

        /// <summary>
        /// Flag that indicates if dispose has been called already.
        /// </summary>
        private bool disposed = false;

        /// <summary>
        /// The latest state provider identifier
        /// </summary>
        private long lastStateProviderId = DateTime.UtcNow.ToFileTimeUtc();

        /// <summary>
        /// Gets or sets lock that protects the copy or checkpoint snapshot of metadata.
        /// </summary>
        private Dictionary<long, Metadata> prepareCheckpointMetadataSnapshot;

        /// <summary>
        /// Lock used to synchronize state provider id creation.
        /// </summary>
        private object stateProviderIdLock = new object();

        /// <summary>
        /// PrepareCheckpointLSN is a sequence number passsed to PrepareCheckpoint, which indicates
        /// all the operation before are stable since its a barrier assigned by TransactionalReplicator.
        /// We use this property to do idempotency check. For consistency, we initialize it with InvalidLSN(-1).
        /// </summary>
        private long prepareCheckpointLSN = StateManagerConstants.InvalidLSN;

        /// <summary>
        /// In memory state that stores local state information
        /// </summary>
        /// <devnote>Make it a concurrent dictionary.</devnote>
        private StateProviderMetadataManager stateProviderMetadataManager;

        /// <summary>
        /// Used for tracing. Computed once.
        /// </summary>
        private string traceType;

        /// <summary>
        /// Work folder.
        /// </summary>
        private string workDirectory;

        /// <summary>
        /// Flag that indicates whether change role has been completed by replicator and state providers.
        /// </summary>
        private bool changeRoleCompleted;

        /// <summary>
        ///  Initializes a new instance of the <see cref="DynamicStateManager"/> class.  
        ///  This constructor is meant for tests only and is not intended to be exposed.
        /// </summary>
        public DynamicStateManager(
            ILoggingReplicator loggingReplicator,
            StateProviderMetadataManager stateProviderMetadataManager,
            string workDirectory)
        {
            this.changeRoleLock = new SemaphoreSlim(1);
            this.loggingReplicator = loggingReplicator;
            this.transactionsManager = loggingReplicator as ITransactionManager;
            Utility.Assert(
                this.transactionsManager != null,
                "{0}: ctor. ILoggingReplicator object must also implement ITransactionsManager. WorkDirectory: {1}",
                this.traceType,
                workDirectory);
            this.stateProviderMetadataManager = stateProviderMetadataManager;
            this.apiDispatcher = new ApiDispatcher(this.traceType);
            this.copyProgressSet = new HashSet<IStateProvider2>();
            this.Version = CurrentVersion;
            this.workDirectory = workDirectory;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="DynamicStateManager"/> class. 
        /// </summary>
        /// <param name="transactionalReplicator">transactional replicator</param>
        public DynamicStateManager(TransactionalReplicator transactionalReplicator)
        {
            transactionalReplicator.ThrowIfNull("transactionalReplicator");

            this.transactionalReplicator = transactionalReplicator;
            this.loggingReplicator = new LoggingReplicator(this);
            this.transactionsManager = this.loggingReplicator as ITransactionManager;
            this.apiDispatcher = new ApiDispatcher(this.traceType);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="DynamicStateManager"/> class. 
        /// </summary>
        /// <param name="transactionalReplicator">transactional replicator</param>
        /// <param name="loggingReplicator">logging replicator</param>
        public DynamicStateManager(
            TransactionalReplicator transactionalReplicator,
            ILoggingReplicator loggingReplicator)
        {
            this.transactionalReplicator = transactionalReplicator;
            this.loggingReplicator = loggingReplicator;
            this.transactionsManager = this.loggingReplicator as ITransactionManager;
            Utility.Assert(
                this.transactionsManager != null,
                "{0}: ctor. ILoggingReplicator object must also implement ITransactionsManager",
                this.traceType);
            this.apiDispatcher = new ApiDispatcher(this.traceType);
        }

        /// <summary>
        /// EventHandler for publishing changes in State Manager's state.
        /// For example, notifying a state provider is added or removed.
        /// </summary>
        public event EventHandler<NotifyStateManagerChangedEventArgs> StateManagerChanged;

        /// <summary>
        /// Gets a value indicating whether the service is currently in the onDataLoss call.
        /// </summary>
        public bool IsServiceOnDataLossInFlight
        {
            get { return this.transactionalReplicator.IsServiceOnDataLossInFlight; }
        }

        /// <summary>
        /// Gets the logging replicator.
        /// </summary>
        public ILoggingReplicator LoggingReplicator
        {
            get
            {
                return this.loggingReplicator;
            }
        }

        /// <summary>
        /// Gets the transactions manager.
        /// </summary>
        public ITransactionManager TransactionsManager 
        {
            get
            {
                return this.transactionsManager;
            }
        }

        /// <summary>
        /// Gets replica role.
        /// </summary>
        public ReplicaRole Role
        {
            get
            {
                return this.LoggingReplicator.Role;
            }
        }

        /// <summary>
        /// Gets state provider metadata manager.
        /// </summary>
        public StateProviderMetadataManager StateProviderMetadataManager
        {
            get
            {
                return this.stateProviderMetadataManager;
            }
        }

        /// <summary>
        /// Gets the partition for service that has state.
        /// </summary>
        public IStatefulServicePartition StatefulPartition { get; private set; }

        /// <summary>
        /// Stateful service context
        /// </summary>
        public StatefulServiceContext StatefulServiceContext { get; private set; }

        /// <summary>
        /// Gets or sets the delegate that creates state provider.
        /// </summary>
        public Func<Uri, Type, IStateProvider2> StateProviderFactory { get; set; }

        /// <summary>
        /// Gets or sets the backup checkpoint file name.
        /// </summary>
        /// <devnote>Exposed for unit tests only.</devnote>
        internal string BackupCheckpointFileName { get; set; }

        /// <summary>
        /// Gets or sets the checkpoint file name.
        /// </summary>
        /// <devnote>Exposed for unit tests only.</devnote>
        internal string CheckpointFileName { get; set; }

        /// <summary>
        /// Gets or sets the value indicating that the replica has been closed or aborted.
        /// </summary>
        /// <devnote>Exposed for unit tests only.</devnote>
        internal bool IsClosing { get; set; }

        /// <summary>
        /// Gets or sets partition Id.
        /// </summary>
        /// <devnote>Exposed for unit tests only.</devnote>
        internal Guid PartitionId { get; set; }

        /// <summary>
        /// Gets or sets replica id.
        /// </summary>
        /// <devnote>Exposed for unit tests only.</devnote>
        internal long ReplicaId { get; set; }

        /// <summary>
        /// Gets or sets the next checkpoint file name.
        /// </summary>
        /// <devnote>Exposed for unit tests only.</devnote>
        internal string TemporaryCheckpointFileName { get; set; }

        /// <summary>
        /// Gets or sets on disk format and wire format version.
        /// </summary>
        /// <devnote>Exposed for unit tests only.</devnote>
        internal int Version { get; set; }

        /// <summary>
        /// Gets the work directory.
        /// </summary>
         /// <devnote>Exposed for unit tests only.</devnote>
        internal string WorkDirectory 
        {
            get
            {
                return this.workDirectory;
            }
        }

        public void Abort()
        {
            // If Abort throws, the exception should not be eaten.
            var abortTask = Task.Run(new Func<Task>(this.AbortAsync));
            abortTask.ContinueWith((task) => { throw task.Exception; }, TaskContinuationOptions.OnlyOnFaulted);
        }

        /// <summary>
        /// Called when state providers need to replicate their operation and when local state needs to be replicated.
        /// </summary>
        public void AddOperation(
            ReplicatorTransaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext,
            long stateProviderId)
        {
            if (transaction == null)
            {
                throw new ArgumentNullException(SR.Error_Transaction);
            }

            NamedOperationContext namedOperationContext = null;
            ThrowIfStateProviderIsNotRegistered(this, stateProviderId, true, transaction.Id);

            if (operationContext != null)
            {
                namedOperationContext = new NamedOperationContext(operationContext, stateProviderId);
            }

            // NamedOperationData used for dispatching purpose, get the user OperationData, then append the 
            // StateProviderId to it. Then remove the infomartion added before when we apply, recover ...
            NamedOperationData namedOperationData = new NamedOperationData(metaData, stateProviderId, this.Version, this.traceType);

            this.TransactionsManager.AddOperation(transaction, namedOperationData, undo, redo, namedOperationContext);
        }

        /// <summary>
        /// Called when state providers need to replicate their operation and when local state needs to be replicated.
        /// </summary>
        public async Task<long> AddOperationAsync(
            AtomicOperation atomicOperation,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext,
            long stateProviderId)
        {
            if (atomicOperation == null)
            {
                throw new ArgumentNullException(SR.Error_AtomicOperation);
            }

            ThrowIfStateProviderIsNotRegistered(
                this,
                stateProviderId,
                false,
                atomicOperation.Id);
            var markedOperationContext = new NamedOperationContext(operationContext, stateProviderId);

            NamedOperationData namedOperationData = new NamedOperationData(metaData, stateProviderId, this.Version, this.traceType);

            return await this.TransactionsManager.AddOperationAsync(
                        atomicOperation,
                        namedOperationData,
                        undo,
                        redo,
                        markedOperationContext).ConfigureAwait(false);
        }

        /// <summary>
        /// Called when state providers need to replicate their redo only operation and when local state needs to be replicated.
        /// </summary>
        public async Task<long> AddOperationAsync(
            AtomicRedoOperation atomicRedoOperation,
            OperationData metaData,
            OperationData redo,
            object operationContext,
            long stateProviderId)
        {
            if (atomicRedoOperation == null)
            {
                throw new ArgumentNullException(SR.Error_AtomicRedoOperation);
            }

            ThrowIfStateProviderIsNotRegistered(
                this,
                stateProviderId,
                false,
                atomicRedoOperation.Id);
            var markedOperationContext = new NamedOperationContext(operationContext, stateProviderId);

            NamedOperationData namedOperationData = new NamedOperationData(metaData, stateProviderId, this.Version, this.traceType);

            return await this.TransactionsManager.AddOperationAsync(
                        atomicRedoOperation,
                        namedOperationData,
                        redo,
                        markedOperationContext).ConfigureAwait(false);
        }

        /// <summary>
        /// Registers state provider with the state manager.
        /// </summary>
        /// <param name="transaction">transaction that this operation is a part of</param>
        /// <param name="stateProviderName">name of the state provider that needs to be registered</param>
        /// <param name="stateProvider">state provider that needs to be registered</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public async Task AddStateProviderAsync(
            ReplicatorTransaction transaction,
            Uri stateProviderName,
            IStateProvider2 stateProvider,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            if (transaction == null)
            {
                throw new ArgumentNullException(SR.Error_Transaction);
            }

            if (stateProviderName == null)
            {
                throw new ArgumentNullException(SR.Error_StateProviderName);
            }

            if (stateProvider == null)
            {
                throw new ArgumentNullException(SR.Error_Transaction);
            }

            var stateProviderType = stateProvider.GetType();

            if (this.StateProviderFactory == null)
            {
                var constructor = stateProviderType.GetConstructor(Type.EmptyTypes);

                if (constructor == null)
                {
                    throw new ArgumentException(
                        SR.Error_SP_ParameterlessConstructor,
                        "stateProviderType");
                }
            }

            this.ThrowIfNotWritable();

            if (stateProviderName == StateManagerName)
            {
                throw new ArgumentException(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        SR.Error_SP_Name_Reserved,
                        stateProviderName.OriginalString));
            }

            // Query for children.
            var stateProviders = GetChildren(
                stateProvider,
                stateProviderName,
                this.traceType);

            cancellationToken.ThrowIfCancellationRequested();

            // Add parent first.

            // Acquire lock
            var parentStateProviderId = this.CreateStateProviderId();
            var lockContext =
                await
                    this.stateProviderMetadataManager.LockForWriteAsync(
                        stateProviderName,
                        parentStateProviderId,
                        transaction,
                        timeout,
                        cancellationToken).ConfigureAwait(false);

            // Register for unlock call.
            transaction.AddLockContext(
                new StateManagerTransactionContext(transaction.Id, lockContext, OperationType.Add));

            // If the parentStateProviderId does not match the lock context state provider id, update it.
            if (parentStateProviderId != lockContext.StateProviderId)
            {
                // Trace and update it.
                FabricEvents.Events.DynamicStateManagerAddStateProvider(
                    this.traceType,
                    lockContext.StateProviderId);

                parentStateProviderId = lockContext.StateProviderId;
            }

            foreach (Uri key in stateProviders.Keys)
            {
                var stateProviderId = this.CreateStateProviderId();
                await
                    this.AddSingleStateProviderAsync(
                        transaction,
                        key,
                        stateProviders[key],
                        stateProviderId,
                        true,
                        parentStateProviderId,
                        timeout,
                        cancellationToken).ConfigureAwait(false);
            }

            // Call AddSP for parent last
            await
                this.AddSingleStateProviderAsync(
                    transaction,
                    stateProviderName,
                    stateProvider,
                    parentStateProviderId,
                    false,
                    EmptyStateProviderId,
                    timeout,
                    cancellationToken).ConfigureAwait(false);

            this.ThrowIfClosed();
        }

        public async Task BackupAsync(
            string backupDirectory,
            BackupOption backupOption,
            CancellationToken cancellationToken)
        {
            FabricEvents.Events.ApiBackupBegin(this.traceType);

            // Run all backup operations in parallel
            var taskList = new List<Task>();

            // Backup the state manager's checkpoint
            taskList.Add(this.BackupCheckpointAsync(backupDirectory));

            // CopyOrCheckpointMetadataSnapshot gets updated only during perform checkpoint and replicator  guarntees that perform and backup cannot happen at the same time,
            // so it is safe to use it here.
            // Backup all active state providers checkpoints
            foreach (var kvp in this.stateProviderMetadataManager.CopyOrCheckpointMetadataSnapshot)
            {
                var metadata = kvp.Value;
                if (metadata.MetadataMode == MetadataMode.Active)
                {
                    var stateProvider = metadata.StateProvider;
                    var stateProviderDirectory = Path.Combine(backupDirectory, metadata.StateProviderId.ToString());

                    FabricDirectory.CreateDirectory(stateProviderDirectory);
                    taskList.Add(stateProvider.BackupCheckpointAsync(stateProviderDirectory, cancellationToken));
                }
            }

            try
            {
                // Wait for all backup operations to finish
                await Task.WhenAll(taskList).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                this.TraceException("BackupAsync", e);
                throw;
            }

            FabricEvents.Events.ApiBackupEnd(this.traceType);
        }

        /// <summary>
        /// Calls state providers to indicate the end of sequence of a set current state providers. 
        /// </summary>
        public void BeginSettingCurrentState()
        {
            FabricEvents.Events.ApiBeginSettingCurrentState(this.traceType);
            this.BeginSetCurrentLocalState();
        }

        public void BeginTransaction(
            ReplicatorTransaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext,
            long stateProviderId)
        {
            if (transaction == null)
            {
                throw new ArgumentNullException(SR.Error_Transaction);
            }

            ThrowIfStateProviderIsNotRegistered(this, stateProviderId, true, transaction.Id);
            NamedOperationContext namedOperationContext = null;

            if (operationContext != null)
            {
                namedOperationContext = new NamedOperationContext(operationContext, stateProviderId);
            }

            NamedOperationData namedOperationData = new NamedOperationData(metaData, stateProviderId, this.Version, this.traceType);

            this.TransactionsManager.BeginTransaction(transaction, namedOperationData, undo, redo, namedOperationContext);
        }

        /// <summary>
        /// Called when state providers need to replicate their operation and when local state needs to be replicated.
        /// </summary>
        public async Task<long> BeginTransactionAsync(
            ReplicatorTransaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext,
            long stateProviderId)
        {
            if (transaction == null)
            {
                throw new ArgumentNullException(SR.Error_Transaction);
            }

            NamedOperationContext namedOperationContext = null;
            ThrowIfStateProviderIsNotRegistered(this, stateProviderId, false, transaction.Id);

            if (operationContext != null)
            {
                namedOperationContext = new NamedOperationContext(operationContext, stateProviderId);
            }

            NamedOperationData namedOperationData = new NamedOperationData(metaData, stateProviderId, this.Version, this.traceType);

            return await this.TransactionsManager.BeginTransactionAsync(
                        transaction,
                        namedOperationData,
                        undo,
                        redo,
                        namedOperationContext).ConfigureAwait(false);
        }

        /// <summary>
        /// ChangeRole is called whenever the replica role is being changed.
        /// </summary>
        public async Task ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            FabricEvents.Events.ApiChangeRole(
                this.traceType,
                this.Role.ToString(),
                newRole.ToString());

            try
            {
                var currentRole = this.Role;

                // TODO: Once we support custom state providers, add retry for failed state providers.
                if (currentRole == newRole)
                {
                    if (this.changeRoleCompleted)
                    {
                        // Service failed the change role so just no-op transactional replicator's change role.
                        FabricEvents.Events.ServiceChangeRoleFailed(this.traceType);

                        return;
                    }
                    else
                    {
                        Utility.Assert(
                            currentRole != newRole,
                            "{0}: ChangeRoleAsync: current role is the same as the new role. CurrentRole: {1} NewRole: {2}",
                            this.traceType,
                            this.Role,
                            newRole);
                    }
                }

                this.changeRoleCompleted = false;

                switch (newRole)
                {
                    case ReplicaRole.Primary:
                        await this.LoggingReplicator.BecomePrimaryAsync().ConfigureAwait(false);
                        break;

                    case ReplicaRole.IdleSecondary:
                        await this.LoggingReplicator.BecomeIdleSecondaryAsync().ConfigureAwait(false);
                        break;

                    case ReplicaRole.ActiveSecondary:
                        await this.LoggingReplicator.BecomeActiveSecondaryAsync().ConfigureAwait(false);
                        break;

                    case ReplicaRole.None:
                        // SumukhS: The order of these statements are significant
                        await this.LoggingReplicator.BecomeNoneAsync().ConfigureAwait(false);
                        await this.ChangeRoleOnStateProvidersAsync(ReplicaRole.None, cancellationToken).ConfigureAwait(false);
                        await this.RemoveStateOnStateProvidersAsync().ConfigureAwait(false);
                        this.RemoveLocalState();
                        await this.LoggingReplicator.DeleteLogAsync().ConfigureAwait(false);
                        break;

                    case ReplicaRole.Unknown:
                    default:
                        Utility.CodingError("Invalid change role: {0} -> {1}", currentRole, newRole);
                        break;
                }

                FabricEvents.Events.ChangeRole_TStateManager(this.traceType);
                if (newRole != ReplicaRole.None)
                {
                    await this.ChangeRoleOnStateProvidersAsync(newRole, cancellationToken).ConfigureAwait(false);
                    FabricEvents.Events.ApiChangeRoleEnd(this.traceType);
                }

                this.changeRoleCompleted = true;
            }
            catch (Exception e)
            {
                this.TraceException("ChangeRoleAsync", e);
                throw;
            }
        }

        /// <summary>
        /// Calls clean on replicator and all the registered state providers.
        /// </summary>
        public async Task CloseAsync()
        {
            FabricEvents.Events.ApiClose(this.traceType, "started");
            this.IsClosing = true;
            try
            {
                // GopalK: The order of the following statements is significant
                await this.LoggingReplicator.PrepareToCloseAsync().ConfigureAwait(false);

                await this.CloseStateProvidersAsync().ConfigureAwait(false);
                this.LoggingReplicator.Dispose();

                // state manager dispose here
                this.Dispose();
                FabricEvents.Events.ApiClose(this.traceType, "completed");
            }
            catch (Exception e)
            {
                this.TraceException("CloseAsync", e);
                throw;
            }
        }

        /// <summary>
        /// Complete checkpoint on all state providers
        /// </summary>
        /// <returns></returns>
        public async Task CompleteCheckpointAsync()
        {
            FabricEvents.Events.ApiCompleteCheckpointBegin(this.traceType);

            // MCoskun: Order of the following operations is significant.
            // #9291020: We depend on CompleteCheckpointLogRecord only being written after replace file is guaranteed to happen by the time
            // current file was to be re-openned even in case of abrupt failur like power loss.
            // CompleteCheckpointOnLocalStateAsync (SafeFileReplaceAsync) calls MoveFileEx(WriteThrough) which flushes the NTFS metadata log.
            // This ensures that replacing the currentfile is not lost in case of power failure after they have returned TRUE.
            // Note that we are taking a dependency on the fact that calling MoveFileEx with WriteThrough will flush the entire metadata log for the drive.
            // By taking this dependency, we do not have to use write-through on the state provider's replace file in CompleteCheckpoint.
            await this.CompleteCheckpointOnStateProvidersAsync().ConfigureAwait(false);
            await this.CompleteCheckpointOnLocalStateAsync().ConfigureAwait(false);

            await this.CleanUpMetadataAsync().ConfigureAwait(false);

            FabricEvents.Events.ApiCompleteCheckpointEnd(this.traceType);
        }

        /// <summary>
        /// Factory method to create an atomic operation
        /// </summary>
        /// <returns>
        /// Object representing an atomic operation
        /// </returns>
        public AtomicOperation CreateAtomicOperation()
        {
            return AtomicOperation.CreateAtomicOperationInternal(this.transactionalReplicator);
        }

        /// <summary>
        /// Factory method to create an atomic redo only operation
        /// </summary>
        /// <returns>
        /// Object representing an atomic redo only operation
        /// </returns>
        public AtomicRedoOperation CreateAtomicRedoOperation()
        {
            return AtomicRedoOperation.CreateAtomicRedoOperationInternal(this.transactionalReplicator);
        }

        /// <summary>
        /// Gets state providers registered with the state manager.
        /// </summary>
        public IEnumerable<IStateProvider2> CreateEnumerable(bool parentsOnly, bool includeSystemStateProviders)
        {
            this.ThrowIfNotReadable();

            // No lock is taken. Must ToList since the state can change.
            if (parentsOnly)
            {
                if (includeSystemStateProviders)
                {
                    return
                        this.stateProviderMetadataManager.GetMetadataCollection()
                            .Where(metadata => metadata.ParentStateProviderId == EmptyStateProviderId)
                            .Select(metadata => metadata.StateProvider)
                            .ToList();
                }
                else
                {
                    return
                        this.stateProviderMetadataManager.GetMetadataCollection()
                            .Where(metadata => metadata.ParentStateProviderId == EmptyStateProviderId
                                    && !metadata.StateProvider.Name.AbsolutePath.StartsWith(this.SystemStateProviderNamePrefix.AbsolutePath))
                            .Select(metadata => metadata.StateProvider)
                            .ToList();
                }
            }
            else
            {
                if (includeSystemStateProviders)
                {
                    return
                        this.stateProviderMetadataManager.GetMetadataCollection()
                            .Select(metadata => metadata.StateProvider)
                            .ToList();
                }
                else
                {
                    return
                        this.stateProviderMetadataManager.GetMetadataCollection()
                            .Where(metadata => !metadata.StateProvider.Name.AbsolutePath.StartsWith(this.SystemStateProviderNamePrefix.AbsolutePath))
                            .Select(metadata => metadata.StateProvider)
                            .ToList();
                }
            }
        }

        /// <summary>
        /// Factory method to create transaction
        /// </summary>
        /// <returns>
        /// Object representing a transaction 
        /// </returns>
        public ReplicatorTransaction CreateTransaction()
        {
            return Transaction.CreateTransactionInternal(this.transactionalReplicator);
        }

        /// <summary>
        /// Disposes state manager.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Calls state providers to indicate the end of sequence of a set current state providers. 
        /// </summary>
        public async Task EndSettingCurrentStateAsync()
        {
            this.EndSetCurrentLocalState();

            // EndSetCurrentState needs to be called on all state providers where BeginSetCurrentState was called. 
            // It cannot be called on all state providers present in in-memory state since there can be 
            // state providers created after SetCurrentState and BeginSetCurrentState has not be caled for them.
            List<Task> tasks = new List<Task>();
            foreach (var stateProvider in this.copyProgressSet)
            {
                tasks.Add(stateProvider.EndSettingCurrentStateAsync());
            }

            try
            {
                await Task.WhenAll(tasks).ConfigureAwait(false);
            }
            catch(Exception e)
            {
                this.TraceException("EndSettingCurrentStateAsync", e);
                throw;
            }

            this.copyProgressSet.Clear();
            FabricEvents.Events.ApiEndSettingCurrentState(this.traceType);
        }

        /// <summary>
        /// Gets copy context.
        /// </summary>
        public IOperationDataStream GetCopyContext()
        {
            return this.LoggingReplicator.GetCopyContext();
        }

        /// <summary>
        /// Gets copy state.
        /// </summary>
        public IOperationDataStream GetCopyState(long upToSequenceNumber, IOperationDataStream copyContext)
        {
            return this.LoggingReplicator.GetCopyState(upToSequenceNumber, copyContext);
        }

        /// <summary>
        /// Gets current data loss number.
        /// </summary>
        public long GetCurrentDataLossNumber()
        {
            return this.LoggingReplicator.GetCurrentDataLossNumber();
        }

        /// <summary>
        /// Gets copy stream from state providers.
        /// </summary>
        public IOperationDataStream GetCurrentState()
        {
            FabricEvents.Events.ApiGetCurrentStateBegin(this.traceType);
            NamedOperationDataCollection namedOperationDataCollection = null;

            try
            {
                this.stateProviderMetadataManager.CopyOrCheckpointLock.EnterReadLock();
                namedOperationDataCollection = new NamedOperationDataCollection(this.traceType, this.Version);

                // 11073193: Using QuickSort instead of c# enumerable OrderByDescending call.
                IOrderedEnumerable<Metadata> sortedMetadataList =
                    this.stateProviderMetadataManager.CopyOrCheckpointMetadataSnapshot.Values.OrderByDescending(
                        o => o.Name.ToString());

                Dictionary<long, SerializableMetadata> serializableMetadataCollection = new Dictionary<long, SerializableMetadata>();

                // copy active state before delete list because if a delete happens after 
                // the delete list is snapped, it will not be captured in the active list or delete list.
                foreach (Metadata metadata in sortedMetadataList)
                {
                    serializableMetadataCollection.Add(
                        metadata.StateProviderId,
                        new SerializableMetadata(
                            metadata.Name,
                            metadata.Type,
                            metadata.InitializationContext,
                            metadata.StateProviderId,
                            metadata.ParentStateProviderId,
                            metadata.MetadataMode,
                            metadata.CreateLsn,
                            metadata.DeleteLsn));
                }

                // call GetCurrentState for local state
                StateProviderIdentifier stateManagerIdentifier = new StateProviderIdentifier(
                    StateManagerName,
                    StateManagerId);

                namedOperationDataCollection.Add(
                    stateManagerIdentifier,
                    new StateManagerCopyStream(serializableMetadataCollection.Values, this.traceType));

                // Get state for all the registered state providers.
                int activeStateprovidersCount = 0;
                foreach (Metadata metadata in sortedMetadataList)
                {
                    // while taking a snapshot, there is a chance that the state provider has been deleted.
                    if (metadata.MetadataMode.Equals(MetadataMode.Active))
                    {
                        IOperationDataStream operationDataStream = metadata.StateProvider.GetCurrentState();
                        if (operationDataStream != null)
                        {
                            activeStateprovidersCount++;
                            StateProviderIdentifier stateProviderIdentifier = new StateProviderIdentifier(
                                metadata.Name,
                                metadata.StateProviderId);
                            namedOperationDataCollection.Add(stateProviderIdentifier, operationDataStream);
                            FabricEvents.Events.GetCurrentState_TStateManager(
                                this.traceType,
                                metadata.StateProviderId,
                                (int) metadata.MetadataMode);
                        }
                        else
                        {
                            FabricEvents.Events.GetCurrentStateNull(
                                this.traceType,
                                metadata.StateProviderId);
                        }
                    }
                }

                FabricEvents.Events.ApiGetCurrentStateEnd(this.traceType, activeStateprovidersCount);
                return namedOperationDataCollection;
            }
            catch (Exception e)
            {
                this.TraceException("GetCurrentState", e);

                if (namedOperationDataCollection != null)
                {
                    namedOperationDataCollection.Dispose();
                }
                
                throw;
            }
            finally
            {
                if (this.stateProviderMetadataManager.CopyOrCheckpointLock.IsReadLockHeld)
                {
                    this.stateProviderMetadataManager.CopyOrCheckpointLock.ExitReadLock();
                }
            }
        }

        /// <summary>
        /// Gets last committed logical sequence number.
        /// </summary>
        public long GetLastCommittedSequenceNumber()
        {
            return this.LoggingReplicator.GetLastCommittedSequenceNumber();
        }

        /// <summary>
        /// Gets or Adds state provider.
        /// </summary>
        /// <param name="transaction">transaction that this operation is a part of</param>
        /// <param name="stateProviderName">name of the state provider that needs to be registered</param>
        /// <param name="stateProviderFactory">state provider factory that creates the state provider that needs to be registered.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>Registered state provider.</returns>
        public async Task<ConditionalValue<T>> GetOrAddStateProviderAsync<T>(
            ReplicatorTransaction transaction,
            Uri stateProviderName,
            Func<Uri, T> stateProviderFactory,
            TimeSpan timeout,
            CancellationToken cancellationToken) where T : IStateProvider2
        {
            if (transaction == null)
            {
                throw new ArgumentNullException(SR.Error_Transaction);
            }

            if (stateProviderName == null)
            {
                throw new ArgumentNullException(SR.Error_StateProviderName);
            }

            if (stateProviderFactory == null)
            {
                throw new ArgumentNullException(SR.Error_StateProviderFactory);
            }

            int position = 0;
            long parentStateProviderId = 0;
            StateManagerLockContext lockContext = null;

            // GetOrAdd - Get section.
            // Step 1: Create StateProviderId
            // Step 2: Acquire the ReadLock
            // Step 3: Try Get StateProvider
            // Step 4: Register the ReadLock if Get success
            // Generate Id for the parent.
            parentStateProviderId = this.CreateStateProviderId();

            try
            {
                // Acquire lock
                lockContext =  await  this.stateProviderMetadataManager.LockForReadAsync(
                    stateProviderName,
                    parentStateProviderId,
                    transaction,
                    timeout,
                    cancellationToken).ConfigureAwait(false);
                ++position;

                ConditionalValue<IStateProvider2> result;

                // todo: Preethas, use remove lock context instead of releasing the lock on catch as it is consistent with the rest of the code
                // since remove lock context on LR's ILockcontext has never been used/tested, do it in the next realease, this is the safer option for now.
                try
                {
                    result = this.TryGetStateProvider(stateProviderName);
                    ++position;
                }
                catch (Exception)
                {
                    lockContext.Release(transaction.Id);
                    throw;
                }

                if (result.HasValue == true)
                {
                    FabricEvents.Events.GetOrAddStateProvider(this.traceType, stateProviderName.ToString());

                    // Register for unlock call.
                    transaction.AddLockContext(
                        new StateManagerTransactionContext(transaction.Id, lockContext, OperationType.Read));
                    return new ConditionalValue<T>(true, (T)result.Value);
                }
            }
            catch (OutOfMemoryException exception)
            {
                string message = string.Format(
                    CultureInfo.InvariantCulture,
                    "Exception thrown in Get section, position: {0} TxnId: {1} SPName: {2} LockContextCount: {3} MemoryUsage: {4} Stack: {5}",
                    position,
                    transaction.Id,
                    stateProviderName,
                    transaction.LockContexts.Count,
                    GC.GetTotalMemory(false),
                    exception.StackTrace);
                this.TraceExceptionWarning(
                    "GetOrAddStateProviderAsync",
                    message,
                    exception);
                throw;
            }

            // GetOrAdd - Add section.
            // Release read lock and acquire write lock
            lockContext.Release(transaction.Id);
            var writeLockContext = await this.stateProviderMetadataManager.LockForWriteAsync(
                stateProviderName,
                parentStateProviderId,
                transaction,
                timeout,
                cancellationToken).ConfigureAwait(false);

            transaction.AddLockContext(
                new StateManagerTransactionContext(transaction.Id, writeLockContext, OperationType.Add));

            // If the parentStateProviderId does not match the lock context state provider id, update it.
            // TODO: Post GA, move this logic into the lock class.
            if (parentStateProviderId != writeLockContext.StateProviderId)
            {
                // Trace and update it.
                FabricEvents.Events.DynamicStateManagerGetOrAddStateProvider(
                    this.traceType,
                    writeLockContext.StateProviderId);

                parentStateProviderId = writeLockContext.StateProviderId;
            }

            var isStateProviderPresent = this.TryGetStateProvider(stateProviderName);
            if (isStateProviderPresent.HasValue)
            {
                FabricEvents.Events.GetOrAddStateProvider(this.traceType, stateProviderName.ToString());
                return new ConditionalValue<T>(true, (T)isStateProviderPresent.Value);
            }

            var stateProvider = stateProviderFactory(stateProviderName);
            if (stateProvider == null)
            {
                throw new ArgumentException(SR.Error_StateProviderFactory);
            }

            var stateProviderType = stateProvider.GetType();

            if (this.StateProviderFactory == null)
            {
                var constructor = stateProviderType.GetConstructor(Type.EmptyTypes);

                if (constructor == null)
                {
                    throw new ArgumentException(
                        SR.Error_SP_ParameterlessConstructor,
                        "stateProviderType");
                }
            }

            // Query for children
            var stateProviders = GetChildren(
                stateProvider,
                stateProviderName,
                this.traceType);

            cancellationToken.ThrowIfCancellationRequested();

            foreach (var key in stateProviders.Keys)
            {
                var stateProviderId = this.CreateStateProviderId();
                await this.AddSingleStateProviderAsync(
                    transaction,
                    key,
                    stateProviders[key],
                    stateProviderId,
                    true,
                    parentStateProviderId,
                    timeout,
                    cancellationToken).ConfigureAwait(false);
            }

            // Call AddSP for parent last.
            await this.AddSingleStateProviderAsync(
                transaction,
                stateProviderName,
                stateProvider,
                parentStateProviderId,
                false,
                EmptyStateProviderId,
                timeout,
                cancellationToken).ConfigureAwait(false);

            this.ThrowIfClosed();

            return new ConditionalValue<T>(false, (T)stateProvider);
        }

        public IStateSerializer<T> GetStateSerializer<T>(Uri name)
        {
            return this.serializationManager.GetStateSerializer<T>(name);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="DynamicStateManager"/> class. 
        /// </summary>
        /// <param name="statefulServiceContext">initialization parameters</param>
        /// <param name="partition">partition object.</param>
        public void Initialize(StatefulServiceContext statefulServiceContext, IStatefulServicePartition partition)
        {
            this.ThrowIfArgumentIsNull(statefulServiceContext, "statefulServiceContext");
            this.StatefulServiceContext = statefulServiceContext;

            this.PartitionId = this.StatefulServiceContext.PartitionId;
            this.ReplicaId = this.StatefulServiceContext.ReplicaId;
            this.workDirectory = this.StatefulServiceContext.CodePackageActivationContext.WorkDirectory;

            this.traceType = string.Format(
                CultureInfo.InvariantCulture,
                "{0}@{1}@{2}",
                this.PartitionId,
                this.ReplicaId,
                StateManagerName);

            partition.ThrowIfNull("partition");
            this.StatefulPartition = partition;

            this.LoggingReplicator.Initialize(this.StatefulServiceContext, this.StatefulPartition);
            this.copyProgressSet = new HashSet<IStateProvider2>();

            this.stateProviderMetadataManager = new StateProviderMetadataManager(this.LoggingReplicator, this.traceType);

            var serviceInstanceName = this.StatefulServiceContext.ServiceName;
            this.CheckpointFileName = this.GetCheckpointFilePath(CurrentCheckpointSuffix, serviceInstanceName);
            this.TemporaryCheckpointFileName = this.GetCheckpointFilePath(TempCheckpointSuffix, serviceInstanceName);
            this.BackupCheckpointFileName = this.GetCheckpointFilePath(BackupCheckpointSuffix, serviceInstanceName);

            this.MoveNativeStateManagerCheckpointFilesIfNecessary();

            this.changeRoleLock = new SemaphoreSlim(1);
            this.Version = CurrentVersion;
            this.prepareCheckpointMetadataSnapshot = new Dictionary<long, Metadata>();
        }

        /// <summary>
        /// Commits transaction.
        /// </summary>
        public async Task<object> OnApplyAsync(
            long applyLSN,
            ReplicatorTransactionBase transaction,
            OperationData metaData,
            OperationData data,
            ApplyContext applyContext)
        {
            Utility.Assert(
                transaction.IsPrimaryTransaction,
                "{0): OnApplyAsync: only primary originated transactions can be applied. ApplyLSN: {1} TxnId: {2} CommitSequenceNumber: {3}",
                this.traceType,
                applyLSN,
                transaction.Id,
                transaction.CommitSequenceNumber);

            NamedOperationData metadataOperationData = null;
            var roleType = applyContext & ApplyContext.ROLE_MASK;

            // If it is primary, we can avoid the deserialization here, we already have the 
            // deserialized data. If not, create the NamedOperationData by deserializing from the OperationData.
            if (ApplyContext.PRIMARY == roleType)
            {
                metadataOperationData = metaData as NamedOperationData;
            }
            else
            {
                metadataOperationData = new NamedOperationData(
                    metaData, 
                    this.Version, 
                    this.traceType);
            }

            Utility.Assert(
                metadataOperationData != null,
                "{0}: OnApplyAsync: metadataOperationData can not be null here. ApplyLSN: {1} TxnId: {2} CommitSequenceNumber: {3}", 
                this.traceType,
                applyLSN,
                transaction.Id,
                transaction.CommitSequenceNumber);

            return await this.OnApplyAsync(
                applyLSN, 
                transaction, 
                metadataOperationData.OperationData, 
                data, 
                applyContext, 
                metadataOperationData.StateProviderId).ConfigureAwait(false);
        }

        /// <summary>
        /// Applies operation.
        /// </summary>
        public async Task<object> OnApplyAsync(
            long applyLSN,
            ReplicatorTransactionBase transaction,
            OperationData metaData,
            OperationData actualData,
            ApplyContext applyContext,
            long stateProviderId)
        {
            object context = null;
            if (stateProviderId == EmptyStateProviderId)
            {
                FabricEvents.Events.OnApplyNameError(this.traceType, transaction.Id);
            }

            Utility.Assert(
                stateProviderId != EmptyStateProviderId,
                "{0}: OnApplyAsync: state Manager deserialization error on apply. ApplyLSN: {1} TxnId: {2} CommitSequenceNumber: {3} SPID: {4}",
                this.traceType,
                applyLSN,
                transaction.Id,
                transaction.CommitSequenceNumber,
                stateProviderId);

            if (stateProviderId == StateManagerId)
            {
                // local state apply
                context = await this.OnApplyOnLocalStateAsync(applyLSN, transaction, metaData, actualData, applyContext).ConfigureAwait(false);
            }
            else
            {
                // if it is present in delete list, ignore it.
                if (this.stateProviderMetadataManager.IsStateProviderDeleted(stateProviderId))
                {
                    FabricEvents.Events.OnApplyInsertStateProviderSkipped(
                        this.traceType,
                        stateProviderId);
                }
                else
                {
                    // The method below asserts that the state provider is present.
                    var stateProvider =
                        this.stateProviderMetadataManager.GetActiveStateProvider(stateProviderId);
                    context = await stateProvider.ApplyAsync(applyLSN, transaction, metaData, actualData, applyContext).ConfigureAwait(false);

                    // todo: preethas : set checkpoint flag to indicate that checkpoint needs to be aclled on the state provider.
                    // metadata.SetCheckpointFlag();
                    // FabricEvents.Events.OnApply(this.traceType, metadata.ToString(), transaction.Id);
                }
            }

            if (context == null)
            {
                return null;
            }

            var markedOperationContext = new NamedOperationContext(context, stateProviderId);
            return markedOperationContext;
        }

        /// <summary>
        /// Called when data loss is encountered to recover from backup or proceed with current data.
        /// </summary>
        public async Task OnDataLossAsync(CancellationToken cancellationToken, bool shouldRestore)
        {
            // This function could not have been called if the restore was not requested by the service.
            var taskList = new List<Task>();

            foreach (var metadata in this.stateProviderMetadataManager.GetMetadataCollection())
            {
                Utility.Assert(
                    metadata != null,
                    "{0}: OnDataLossAsync: metadata != null. ShouldRestore: {1}",
                    this.traceType,
                    shouldRestore);
                taskList.Add(metadata.StateProvider.OnDataLossAsync());
            }

            try
            {
                await Task.WhenAll(taskList).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                this.TraceException("OnDataLossAsync", e);
                throw;
            }

            FabricEvents.Events.OnDataLoss(this.traceType, shouldRestore);
            await this.LoggingReplicator.OnDataLossAsync(cancellationToken, shouldRestore).ConfigureAwait(false);
        }

        /// <summary>
        /// Calls recovery completed on all the registered state providers.
        /// </summary>
        public async Task OnRecoveryCompletedAsync()
        {
            FabricEvents.Events.ApiOnRecoveryCompletedBegin(this.traceType);

            this.OnLocalRecoveryCompleted();

            var taskList = new List<Task>();
            foreach (var metadata in this.stateProviderMetadataManager.GetMetadataCollection())
            {
                Utility.Assert(
                    metadata != null,
                    "{0}: OnRecoveryCompletedAsync: metadata != null",
                    this.traceType);
                taskList.Add(metadata.StateProvider.OnRecoveryCompletedAsync());
            }

            try
            {
                await Task.WhenAll(taskList).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                this.TraceException("OnRecoveryCompletedAsync", e);
                throw;
            }

            FabricEvents.Events.ApiOnRecoveryCompletedEnd(this.traceType);
        }

        /// <summary>
        /// Initializes resources necessary for the replicator and for all the state providers.
        /// </summary>
        public async Task OpenAsync(
            ReplicaOpenMode openMode,
            TransactionalReplicatorSettings transactionalReplicatorSettings)
        {
            FabricEvents.Events.ApiOpen(this.traceType, "started");

            var isOpenDoneOnStateProviders = false;

            try
            {
                var recoveryLogRecordInformation =
                    await this.LoggingReplicator.OpenAsync(openMode, transactionalReplicatorSettings).ConfigureAwait(false);

                // If recoveryLogRecordInformation.ShouldRemoveLocalStateDueToIncompleteRestore is true, log has been deleted. 
                // So recoveryLogRecordInformation.LogCompleteCheckpointAfterRecovery cannot be true.
                Utility.Assert(
                    recoveryLogRecordInformation.ShouldRemoveLocalStateDueToIncompleteRestore == false
                    || recoveryLogRecordInformation.LogCompleteCheckpointAfterRecovery == false,
                    "{0}: OpenAsync: ShouldRemoveLocalStateDueToIncompleteRestore: {1} LogCompleteCheckpointAfterRecovery: {2}",
                    this.traceType,
                    recoveryLogRecordInformation.ShouldRemoveLocalStateDueToIncompleteRestore,
                    recoveryLogRecordInformation.LogCompleteCheckpointAfterRecovery);

                if (recoveryLogRecordInformation.LogCompleteCheckpointAfterRecovery)
                {
                    await this.CompleteCheckpointOnLocalStateAsync().ConfigureAwait(false);
                }

                if (recoveryLogRecordInformation.ShouldRemoveLocalStateDueToIncompleteRestore)
                {
                    await this.CleanUpIncompleteRestoreAsync(CancellationToken.None).ConfigureAwait(false);
                }

                await this.RecoverStateManagerCheckpointAsync(CancellationToken.None).ConfigureAwait(false);

                await this.OpenStateProvidersAsync().ConfigureAwait(false);
                isOpenDoneOnStateProviders = true;

                if (recoveryLogRecordInformation.LogCompleteCheckpointAfterRecovery)
                {
                    await this.CompleteCheckpointOnStateProvidersAsync().ConfigureAwait(false);
                }

                await this.RecoverStateProvidersAsync().ConfigureAwait(false);
                FabricEvents.Events.Open(this.traceType, "local recovery completed");

                if (recoveryLogRecordInformation.ShouldSkipRecoveryDueToIncompleteChangeRoleNone)
                {
                    FabricEvents.Events.Open(this.traceType, "Skipping recovery");
                }
                else
                {
                    // recover replicator
                    await this.LoggingReplicator.PerformRecoveryAsync(recoveryLogRecordInformation).ConfigureAwait(false);

                    FabricEvents.Events.ApiOpen(this.traceType, "completed");
                }
            }
            catch (Exception e)
            {
                this.TraceException("OpenAsync", e);

                if (isOpenDoneOnStateProviders)
                {
                    // abort all the state providers.
                    this.AbortStateProviders();
                }

                // If open fails, cleanup the objects as "Close" or "Abort" will not be invoked
                this.LoggingReplicator.Dispose();
                this.Dispose();

                throw;
            }
        }

        /// <summary>
        /// Perform checkpoint on all state providers
        /// </summary>
        /// <param name="performMode">Represents different mode to perform checkpoint.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public async Task PerformCheckpointAsync(PerformCheckpointMode performMode)
        {
            FabricEvents.Events.ApiPerformCheckpointBegin(this.traceType);

            try
            {
                await this.PerformCheckpointStateProvidersAsync(performMode).ConfigureAwait(false);
                await this.LocalStateCheckpointAsync().ConfigureAwait(false);
            }
            catch (Exception e)
            {
                this.TraceException("PerformCheckpointAsync", e);
                throw;
            }

            FabricEvents.Events.ApiPerformCheckpointEnd(this.traceType);
        }

        /// <summary>
        /// Prepare checkpoint on all state providers
        /// </summary>
        /// <param name="checkpointLSN">Checkpoint logical sequence number.</param>
        /// <returns></returns>
        public async Task PrepareCheckpointAsync(long checkpointLSN)
        {
            Utility.Assert(
                checkpointLSN >= 0,
                "{0}: PrepareCheckpointAsync: checkpoint lsn must >= 0. checkpointLSN: {1}",
                this.traceType,
                checkpointLSN);

            FabricEvents.Events.ApiPrepareCheckpointBegin(this.traceType);

            this.prepareCheckpointMetadataSnapshot.Clear();

            // copy can be done in any order, as there can be no active applies during this time.
            foreach (var metadata in this.stateProviderMetadataManager.GetMetadataCollection())
            {
                var copiedMetadata = new Metadata(
                    metadata.Name,
                    metadata.Type,
                    metadata.StateProvider,
                    metadata.InitializationContext,
                    metadata.StateProviderId,
                    metadata.ParentStateProviderId,
                    metadata.CreateLsn,
                    metadata.DeleteLsn,
                    MetadataMode.Active,
                    false);

                this.prepareCheckpointMetadataSnapshot.Add(copiedMetadata.StateProviderId, copiedMetadata);
            }

            // Preparecheckpoint has to snap deleted state providers because copy uses this snap and copy needs to know
            // deletes stated providers to be able to handle duplicates that can be applied on copy.
            foreach (var metadata in this.stateProviderMetadataManager.GetDeletedStateProviders())
            {
                if (metadata.MetadataMode == MetadataMode.DelayDelete)
                {
                    var copiedMetadata = new Metadata(
                        metadata.Name,
                        metadata.Type,
                        metadata.StateProvider,
                        metadata.InitializationContext,
                        metadata.StateProviderId,
                        metadata.ParentStateProviderId,
                        metadata.CreateLsn,
                        metadata.DeleteLsn,
                        metadata.MetadataMode,
                        false);

                    this.prepareCheckpointMetadataSnapshot.Add(copiedMetadata.StateProviderId, copiedMetadata);
                }
            }

            // Save the checkpointLSN, which will be written to the file and used for idempotency check.
            this.prepareCheckpointLSN = checkpointLSN;

            await this.PrepareCheckpointStateProvidersAsync(checkpointLSN).ConfigureAwait(false);

            FabricEvents.Events.ApiPrepareCheckpointEnd(this.traceType);
        }

        /// <summary>
        /// // Remove all local state as this replica is no longer needed
        /// </summary>
        public async Task RemoveStateOnStateProvidersAsync()
        {
            List<Task> taskList = new List<Task>();
            List<long> stateProviderIdList = new List<long>();

            foreach (var metadata in this.stateProviderMetadataManager.GetMetadataCollection())
            {
                Utility.Assert(
                    metadata != null,
                    "{0}: RemoveStateOnStateProvidersAsync: metadata != null",
                    this.traceType);
                taskList.Add(metadata.StateProvider.RemoveStateAsync(metadata.StateProviderId));
                stateProviderIdList.Add(metadata.StateProviderId);

                FabricEvents.Events.RemoveStateOnStateProvider(this.traceType, metadata.StateProviderId);
            }

            Utility.Assert(
                this.stateProviderMetadataManager.DeletedStateProviders != null,
                "{0}: RemoveStateOnStateProvidersAsync: this.stateProviderMetadataManager.DeletedStateProviders!=null",
                this.traceType);

            foreach (var kvp in this.stateProviderMetadataManager.DeletedStateProviders)
            {
                var metadata = kvp.Value;
                Utility.Assert(
                    metadata != null,
                    "{0}: RemoveStateOnStateProvidersAsync: metadata != null",
                    this.traceType);
                taskList.Add(metadata.StateProvider.RemoveStateAsync(metadata.StateProviderId));
                stateProviderIdList.Add(metadata.StateProviderId);

                FabricEvents.Events.RemoveStateOnStateProvider(this.traceType, metadata.StateProviderId);
            }

            try
            {
                await Task.WhenAll(taskList).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                this.TraceException("RemoveStateOnStateProvidersAsync", e);
                throw;
            }

            // #10129507: Clean up empty folders (issue was with RQ and RD)
            foreach (long stateProviderId in stateProviderIdList)
            {
                DeleteStateProviderWorkDirectory(stateProviderId);
            }
        }

        /// <summary>
        /// Unregisters state provider.
        /// </summary>
        /// <param name="transaction">transaction that the unregister operation is a part of.</param>
        /// <param name="stateProviderName">state provider that needs to be unregistered.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public async Task RemoveStateProviderAsync(
            ReplicatorTransaction transaction,
            Uri stateProviderName,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            if (transaction == null)
            {
                throw new ArgumentNullException(SR.Error_Transaction);
            }

            if (stateProviderName == null)
            {
                throw new ArgumentNullException(SR.Error_StateProviderName);
            }

            var metadata = this.stateProviderMetadataManager.GetMetadata(stateProviderName);
            if (metadata == null)
            {
                throw new ArgumentException(
                    string.Format(CultureInfo.CurrentCulture,"State provider {0} does not exist", stateProviderName.OriginalString));
            }

            // TODO: Preethas This is a workaround until state manager supports hierarchy. Once hierarchy is supported,
            // hierarchical information should be available in the state manager and should not be queried from the state provider.

            // Query for children
            var stateProviders = GetChildren(
                metadata.StateProvider,
                stateProviderName,
                this.traceType);

            cancellationToken.ThrowIfCancellationRequested();

            // Call parent first.
            await this.RemoveSingleStateProviderAsync(transaction, stateProviderName, timeout, cancellationToken).ConfigureAwait(false);

            foreach (var name in stateProviders.Keys)
            {
                await this.RemoveSingleStateProviderAsync(transaction, name, timeout, cancellationToken).ConfigureAwait(false);
            }

            this.ThrowIfClosed();
        }

        public void RemoveStateSerializer<T>()
        {
            this.serializationManager.RemoveStateSerializer<T>();
        }

        public void RemoveStateSerializer<T>(Uri name)
        {
            this.serializationManager.RemoveStateSerializer<T>(name);
        }

        /// <summary>
        /// Re-initializes the SM and the state providers from the backed-up state.
        /// </summary>
        /// <param name="backupDirectory">Backup directory.</param>
        /// <param name="cancellationToken">The cancellation token.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public async Task RestoreAsync(string backupDirectory, CancellationToken cancellationToken)
        {
            FabricEvents.Events.ApiRestoreBegin(this.traceType);

            var isOpenDoneOnStateProviders = false;
            try
            {
                // todo: During restore change role to none, close and remove state should be called on active and deleted state providers.
                // Remove the state of all current state providers.
                await this.ChangeRoleOnStateProvidersAsync(ReplicaRole.None, cancellationToken).ConfigureAwait(false);

                // Remove state for all the state providers that have been recovered so far.
                await this.RemoveStateOnStateProvidersAsync().ConfigureAwait(false);

                // Clear the current in-memory state
                await this.CloseStateProvidersAsync().ConfigureAwait(false);

                this.RemoveLocalState();

                this.stateProviderMetadataManager.Dispose();
                this.stateProviderMetadataManager = new StateProviderMetadataManager(
                    this.LoggingReplicator,
                    this.traceType);

                // Start the restore process
                await this.RestoreCheckpointAsync(backupDirectory).ConfigureAwait(false);
                await this.LocalStateRecoverCheckpointAsync().ConfigureAwait(false);

                var collection = this.stateProviderMetadataManager.GetMetadataCollection()
                    .Where(metadata => metadata.ParentStateProviderId == EmptyStateProviderId)
                    .Select(metadata => metadata.StateProvider)
                    .ToList();

                // Notifies on the changes on parent state providers.
                this.NotifyStateManagerStateChanged(collection.ToAsyncEnumerable());

                await this.OpenStateProvidersAsync().ConfigureAwait(false);
                isOpenDoneOnStateProviders = true;

                await this.RestoreStateProvidersAsync(backupDirectory).ConfigureAwait(false);
                await this.RecoverStateProvidersAsync().ConfigureAwait(false);

                FabricEvents.Events.ApiRestoreEnd(this.traceType);
            }
            catch (Exception e)
            {
                this.TraceException("RestoreAsync", e);

                if (isOpenDoneOnStateProviders)
                {
                    // abort all the state providers.
                    this.AbortStateProviders();
                }

                throw;
            }
        }

        /// <summary>
        /// Send current state to the state providers.
        /// </summary>
        public async Task SetCurrentStateAsync(long stateRecordNumber, OperationData data)
        {
            var namedOperationData = NamedOperationData.Deserialize(data, this.traceType, this.Version);

            if (namedOperationData.StateProviderId == EmptyStateProviderId)
            {
                FabricEvents.Events.SetCurrentStateDeserializationError(this.traceType);
            }

            Utility.Assert(
                namedOperationData.StateProviderId != 0,
                "{0}: SetCurrentStateAsync: state manager deserialization error on set current state. StateRecordNumber: {1}",
                this.traceType,
                stateRecordNumber);

            // Set local state.
            if (namedOperationData.StateProviderId == StateManagerId)
            {
                await this.SetCurrentLocalStateAsync(namedOperationData.OperationData).ConfigureAwait(false);
            }
            else
            {
                // check if it is in the delete list
                if (this.stateProviderMetadataManager.IsStateProviderDeleted(namedOperationData.StateProviderId))
                {
                    var metadata =
                        this.stateProviderMetadataManager.DeletedStateProviders[namedOperationData.StateProviderId];
                    FabricEvents.Events.SetCurrentStateSkipped(this.traceType, metadata.StateProviderId);
                }
                else
                {
                    var stateProvider =
                        this.stateProviderMetadataManager.GetActiveStateProvider(namedOperationData.StateProviderId);

                    Utility.Assert(
                        this.copyProgressSet.Contains(stateProvider),
                        "{0}: SetCurrentStateAsync: begin set current state has not been called on state provider {2}",
                        this.traceType, namedOperationData.StateProviderId);
                    await stateProvider.SetCurrentStateAsync(stateRecordNumber, namedOperationData.OperationData).ConfigureAwait(false);

                    FabricEvents.Events.SetCurrentState(
                        this.traceType,
                        namedOperationData.StateProviderId);
                }
            }
        }

        public bool TryAddStateSerializer<T>(IStateSerializer<T> stateSerializer)
        {
            return this.serializationManager.TryAddStateSerializer<T>(stateSerializer);
        }

        public bool TryAddStateSerializer<T>(Uri name, IStateSerializer<T> stateSerializer)
        {
            return this.serializationManager.TryAddStateSerializer<T>(name, stateSerializer);
        }

        /// <summary>
        /// Gets the given state provider registered with the state manager.
        /// </summary>
        /// <returns>state provider, if the given state provider is registered.</returns>
        public ConditionalValue<IStateProvider2> TryGetStateProvider(Uri stateProviderName)
        {
            this.ThrowIfNotReadable();

            if (stateProviderName == null)
            {
                throw new ArgumentNullException(SR.Error_StateProviderName);
            }

            // No lock is taken.
            var metadata = this.stateProviderMetadataManager.GetMetadata(stateProviderName);
            this.ThrowIfClosed();

            if (metadata != null)
            {
                return new ConditionalValue<IStateProvider2>(true, metadata.StateProvider);
            }
            else
            {
                return new ConditionalValue<IStateProvider2>(false, null);
            }
        }

        /// <summary>
        /// Unlocks resources after transaction.
        /// </summary>
        public void Unlock(object state)
        {
            var context = (NamedOperationContext) state;
            var id = context.StateProviderId;

            if (id == StateManagerId)
            {
                Utility.Assert(
                    this.Role != ReplicaRole.Primary,
                    "{0}: Unlock: this.Role != ReplicaRole.Primary. Role: {1}",
                    this.traceType, 
                    this.Role);
                this.UnlockOnLocalState(context);
            }
            else
            {
                if (context.OperationContext != null)
                {
                    var stateProvider = this.stateProviderMetadataManager.GetActiveStateProvider(id);

                    stateProvider.Unlock(context.OperationContext);

                    // FabricEvents.Events.Unlock(this.traceType, metadata.ToString());
                }
            }
        }

        /// <summary>
        /// Called upon reconfiguration.
        /// </summary>
        public Task UpdateEpochAsync(
            Epoch epoch,
            long previousEpochLastSequenceNumber,
            CancellationToken cancellationToken)
        {
            return this.LoggingReplicator.UpdateEpochAsync(epoch, previousEpochLastSequenceNumber, cancellationToken);
        }

        /// <summary>
        /// Throws if there is an operation request for a state provider that is not yet registered with the state manager.
        /// </summary>
        /// <devnote>Exposed for testability only.</devnote>
        void IStateManager.ThrowIfStateProviderIsNotRegistered(long stateProviderId, long transactionId)
        {
            ThrowIfStateProviderIsNotRegistered(this, stateProviderId, true, 0);
        }

        void IStateManager.SingleOperationTransactionAbortUnlock(long stateProviderId, object state)
        {
            try 
            {
                if (stateProviderId == StateManagerId)
                {
                    this.Unlock(state);
                }
                else
                {
                    var metaData = this.stateProviderMetadataManager.GetMetadata(stateProviderId);
                    Utility.Assert(
                        metaData != null, "{0}: SingleOperationTransactionAbortUnlock: Active state provider {1} not expected to be null", 
                        this.traceType,
                        stateProviderId);
                    metaData.StateProvider.Unlock(state);
                }
            }
            catch(Exception e)
            {
                this.TraceExceptionWarning(
                    "SingleOperationTransactionAbortUnlock",
                    string.Format(CultureInfo.InvariantCulture, "Unlock call for state provider {0}.", stateProviderId),
                    e);
                
                this.StatefulPartition.ReportFault(FaultType.Transient);
            }
        }

        /// <summary>
        /// Gets all the state provider under the given parent.
        /// </summary>
        /// <param name="root">root state provider</param>
        /// <param name="rootName">name of the root state provider</param>
        /// <param name="traceType">trace information</param>
        /// <returns>collection of state providers</returns>
        /// <devnote>Exposed for unit tests only.</devnote>
        internal static Dictionary<Uri, IStateProvider2> GetChildren(
            IStateProvider2 root,
            Uri rootName,
            string traceType)
        {
            Utility.Assert(root != null, "{0}: GetChildren: root cannot be null", traceType);
            Utility.Assert(rootName != null, "{0}: GetChildren: root name cannot be null", traceType);

            var stateProviders =
                new Dictionary<Uri, IStateProvider2>(AbsoluteUriEqualityComparer.Comparer);
            FabricEvents.Events.GetChildrenBegin(traceType);

            try
            {
                var treeEntries = new Queue<IStateProvider2>();
                treeEntries.Enqueue(root);

                while (treeEntries.Count > 0)
                {
                    var stateProvider = treeEntries.Dequeue();
                    var children = stateProvider.GetChildren(rootName);
                    if (children != null)
                    {
                        foreach (var entry in children)
                        {
                            treeEntries.Enqueue(entry);
                            try
                            {
                                stateProviders.Add(entry.Name, entry);
                            }
                            catch (ArgumentException)
                            {
                                FabricEvents.Events.GetChildrenDuplicateFound(
                                    traceType,
                                    stateProvider.Name.ToString());
                                throw;
                            }
                        }
                    }
                }
            }
            catch (Exception e)
            {
                StateManagerStructuredEvents.TraceException(traceType, "GetChildren", string.Empty, e);
                throw;
            }

            FabricEvents.Events.GetChildrenEnd(traceType);
            return stateProviders;
        }

        internal static void ThrowIfStateProviderIsNotRegistered(
            DynamicStateManager dynamicStateManager,
            long stateProviderId,
            bool checkforDeleting,
            long transactionId)
        {
            if (dynamicStateManager.stateProviderMetadataManager.GetMetadata(stateProviderId) == null)
            {
                if (stateProviderId != StateManagerId)
                {
                    if (dynamicStateManager.IsClosing)
                    {
                        // Throw fabric object closed exception if state provider does not exist as
                        // the in-memory state has been cleared on close.
                        throw new FabricObjectClosedException();
                    }

                    FabricEvents.Events.ThrowIfStateProviderIdIsNotRegistered(
                        dynamicStateManager.traceType,
                        stateProviderId,
                        StateManagerName.OriginalString);
                    throw new InvalidOperationException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            "State provider {0} is not registered with the state manager",
                            stateProviderId));
                }
            }

            if (stateProviderId != StateManagerId)
            {
                var metadata = dynamicStateManager.stateProviderMetadataManager.GetMetadata(stateProviderId);

                if (metadata == null && dynamicStateManager.IsClosing)
                {
                    // Throw fabric object closed exception if state provider does not exist as
                    // the in-memory state has been cleared on close.
                    throw new FabricObjectClosedException();
                }

                stateProviderId = metadata.StateProviderId;

                if (checkforDeleting && metadata.TransientDelete && transactionId == metadata.TransactionId)
                {
                    // MCoskun: Keeping the metadata.ToString() since this is an error trace.
                    FabricEvents.Events.AddOperation(
                        dynamicStateManager.traceType,
                        metadata.ToString(),
                        transactionId);

                    throw new FabricObjectClosedException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            "State provider {0} is being deleted in this transaction",
                            stateProviderId));
                }
            }
        }

        internal async Task<StateManagerTransactionContext> ApplyOnFalseProgressAsync(
            long applyLSN,
            ReplicatorTransactionBase transactionBase,
            long stateproviderId,
            Uri stateProviderName,
            StateManagerApplyType applyType,
            ApplyContext operationType)
        {
            Utility.Assert(
                operationType == ApplyContext.FALSE_PROGRESS,
                "{0}: ApplyOnFalseProgressAsync: operation type should be false progress. ApplyLSN: {1} TxnId: {2} CommitLSN: {3} SPID: {4} SPName: {5} ApplyType: {6} OperationType: {7}",
                this.traceType,
                applyLSN,
                transactionBase.Id,
                transactionBase.CommitSequenceNumber,
                stateproviderId,
                stateProviderName,
                applyType,
                operationType);
            if (applyType == StateManagerApplyType.Delete)
            {
                FabricEvents.Events.ApplyOnSecondaryFalseProgressUndoInsertStateProvider(
                    this.traceType,
                    stateproviderId,
                    transactionBase.Id);

                var metadata = this.stateProviderMetadataManager.GetMetadata(stateProviderName);

                // assert that the ids match.
                Utility.Assert(
                    metadata.StateProviderId == stateproviderId,
                    "{0}: ApplyOnFalseProgressAsync: state provider id mismatch between the one presnet {1} and one received on false progress {2}. ApplyLSN: {3} TxnId: {4} CommitLSN: {5} SPName: {6} ApplyType: {7} OperationType: {8}",
                    this.traceType,
                    metadata.StateProviderId,
                    stateproviderId,
                    applyLSN,
                    transactionBase.Id,
                    transactionBase.CommitSequenceNumber,
                    stateProviderName,
                    applyType,
                    operationType);

                this.stateProviderMetadataManager.SoftDelete(stateProviderName, MetadataMode.FalseProgress);
                this.NotifyStateManagerStateChanged(
                    (ITransaction) transactionBase,
                    metadata.StateProvider,
                    metadata.ParentStateProviderId,
                    NotifyStateManagerChangedAction.Remove);

                FabricEvents.Events.ApplyOnSecondarySoftDeleteStateProvider(
                    this.traceType,
                    stateproviderId);
            }
            else
            {
                FabricEvents.Events.ApplyOnSecondaryFalseProgressUndoDeleteStateProvider(
                    this.traceType,
                    stateproviderId,
                    transactionBase.Id);

                var metadata = await this.stateProviderMetadataManager.ResurrectStateProviderAsync(stateproviderId).ConfigureAwait(false);

                this.NotifyStateManagerStateChanged(
                    (ITransaction) transactionBase,
                    metadata.StateProvider,
                    metadata.ParentStateProviderId,
                    NotifyStateManagerChangedAction.Add);

                // call unpause api on the state provider
            }

            return null;
        }

        /// <summary>
        /// Applies operations to the primary replica.
        /// </summary>
        internal async Task ApplyOnPrimaryAsync(
            long applyLSN,
            ReplicatorTransactionBase transactionBase,
            StateManagerApplyType applyType,
            Uri name,
            long stateProviderId)
        {
            this.ThrowIfNotValid(
                applyType == StateManagerApplyType.Insert || applyType == StateManagerApplyType.Delete,
                "{0}: ApplyOnPrimaryAsync: Unexpecetd apply type. ApplyLSN: {1} TxnID: {2} CommitLSN: {3} ApplyType: {4} Name: {5} SPID: {6}",
                this.traceType,
                applyLSN,
                transactionBase.Id,
                transactionBase.CommitSequenceNumber,
                applyType,
                name,
                stateProviderId);

            Metadata metadata = this.stateProviderMetadataManager.GetMetadata(name, true);
            Utility.Assert(
                metadata != null,
                "{0}: ApplyOnPrimaryAsync: metadata is not present for the name. ApplyLSN: {1} TxnID: {2} CommitLSN: {3} ApplyType: {4} Name: {5} SPID: {6}",
                this.traceType,
                applyLSN,
                transactionBase.Id,
                transactionBase.CommitSequenceNumber,
                applyType,
                name,
                stateProviderId);
            Utility.Assert(
                metadata.StateProviderId == stateProviderId,
                "{0}: ApplyOnPrimaryAsync: expected state provider id is {1}, but id present in active state is {2}. ApplyLSN: {3} TxnID: {4} CommitLSN: {5} ApplyType: {6} Name: {7}",
                this.traceType,
                stateProviderId, 
                metadata.StateProviderId,
                applyLSN,
                transactionBase.Id,
                transactionBase.CommitSequenceNumber,
                applyType,
                name);

            if (applyType == StateManagerApplyType.Insert)
            {
                metadata.CreateLsn = transactionBase.CommitSequenceNumber;
                await this.InitializeStateProvidersAsync(metadata, false).ConfigureAwait(false);

                this.NotifyStateManagerStateChanged(
                    (ITransaction) transactionBase,
                    metadata.StateProvider,
                    metadata.ParentStateProviderId,
                    NotifyStateManagerChangedAction.Add);

                FabricEvents.Events.ApplyOnPrimaryInsertStateProvider(
                    this.traceType,
                    metadata.StateProviderId,
                    transactionBase.Id);
            }
            else if (applyType == StateManagerApplyType.Delete)
            {
                // close state provider. skipping it for now.
                // take the register lock along with the close lock, register lock should be taken first and then the close lock
                // register lock is needed as change role currently does not acquire lock for every state provider.

                // soft delete 
                metadata.DeleteLsn = transactionBase.CommitSequenceNumber;
                this.stateProviderMetadataManager.SoftDelete(metadata.Name, MetadataMode.DelayDelete);

                this.NotifyStateManagerStateChanged(
                    (ITransaction) transactionBase,
                    metadata.StateProvider,
                    metadata.ParentStateProviderId,
                    NotifyStateManagerChangedAction.Remove);

                FabricEvents.Events.ApplyOnPrimaryDeleteStateProvider(
                    this.traceType,
                    metadata.StateProviderId,
                    transactionBase.Id);
            }

            // lock context is not returned here as it is already registered with the transaction
        }

        /// <summary>
        /// Applies operations during recovery.
        /// </summary>
        internal async Task<StateManagerTransactionContext> ApplyOnRecoveryAsync(
            long applyLSN,
            ReplicatorTransactionBase transactionBase,
            ReplicationMetadata replicationMetadata)
        {
            try
            {
                var applyType = replicationMetadata.StateManagerApplyType;
                StateManagerTransactionContext stateManagerTransactionContext = null;

                if (applyType == StateManagerApplyType.Insert)
                {
                    // if it is already present in delete list, ignore it.
                    if (this.stateProviderMetadataManager.IsStateProviderDeleted(replicationMetadata.StateProviderId))
                    {
                        // TODO: RDBug 10631157:Add idempotent operation assert for CreateLSN/DeleteLSN check
                        FabricEvents.Events.ApplyOnRecoveryInsertStateProviderSkipped(
                            this.traceType,
                            replicationMetadata.StateProviderId);
                    }
                    else if (this.stateProviderMetadataManager.ContainsKey(replicationMetadata.Name))
                    {
                        // duplicate keys can come in during recovery.
                        var metadata = this.stateProviderMetadataManager.GetMetadata(replicationMetadata.Name);
                        Utility.Assert(
                            metadata != null,
                            "{0}: ApplyOnRecoveryAsync: metadata {1} is not present in active state. ApplyLSN: {2} TxnID:{3} CommitLSN: {4}",
                            this.traceType,
                            replicationMetadata,
                            applyLSN,
                            transactionBase.Id,
                            transactionBase.CommitSequenceNumber);
                        Utility.Assert(
                            replicationMetadata.StateProviderId == metadata.StateProviderId,
                            "{0}: ApplyOnRecoveryAsync: stateprovider id present in active state is {1}, obtained through replication is {2}. ApplyLSN: {3} TxnID:{4} CommitLSN: {5}",
                            this.traceType,
                            metadata,
                            replicationMetadata,
                            applyLSN,
                            transactionBase.Id,
                            transactionBase.CommitSequenceNumber);
                        this.CheckIdempotency(
                            replicationMetadata.Name,
                            applyLSN,
                            transactionBase.CommitSequenceNumber);
                    }
                    else
                    {
                        var lockContext =
                            await
                                this.stateProviderMetadataManager.LockForWriteAsync(
                                    replicationMetadata.Name,
                                    replicationMetadata.StateProviderId,
                                    transactionBase,
                                    Timeout.InfiniteTimeSpan,
                                    CancellationToken.None).ConfigureAwait(false);
                        stateManagerTransactionContext = new StateManagerTransactionContext(
                            transactionBase.Id,
                            lockContext,
                            OperationType.Add);

                        var stateProvider = this.CreateStateProvider(
                            replicationMetadata.Name,
                            replicationMetadata.Type);

                        var metadata = new Metadata(
                            replicationMetadata.Name,
                            replicationMetadata.Type,
                            stateProvider,
                            replicationMetadata.InitializationContext,
                            replicationMetadata.StateProviderId,
                            replicationMetadata.ParentStateProviderId,
                            transactionBase.CommitSequenceNumber,
                            -1,
                            MetadataMode.Active,
                            false);

                        var stateProviderList = this.InitializeStateProvidersInOrder(metadata);

                        var result = this.stateProviderMetadataManager.Add(metadata.StateProvider.Name, metadata);
                        Utility.Assert(
                            result,
                            "{0}: ApplyOnRecoveryAsync: Add has failed because the key {1} is already present. ApplyLSN: {2} TxnID:{3} CommitLSN: {4}",
                            this.traceType,
                            metadata,
                            applyLSN,
                            transactionBase.Id,
                            transactionBase.CommitSequenceNumber);

                        this.NotifyStateManagerStateChanged(
                            (ITransaction) transactionBase,
                            metadata.StateProvider,
                            metadata.ParentStateProviderId,
                            NotifyStateManagerChangedAction.Add);

                        foreach (var stateProviderMetadata in stateProviderList)
                        {
                            // state provider initialize methods
                            await stateProviderMetadata.StateProvider.OpenAsync().ConfigureAwait(false);
                            await stateProviderMetadata.StateProvider.RecoverCheckpointAsync().ConfigureAwait(false);
                            await stateProviderMetadata.StateProvider.OnRecoveryCompletedAsync().ConfigureAwait(false);

                            this.UpdateLastStateProviderId(metadata.StateProviderId);

                            FabricEvents.Events.ApplyOnRecoveryInsertStateProvider(
                                this.traceType,
                                stateProviderMetadata.StateProviderId,
                                transactionBase.Id);
                        }
                    }
                }
                else
                {
                    this.ThrowIfNotValid(
                        applyType == StateManagerApplyType.Delete,
                        "StateManager.ApplyOnRecoverAsync. expected apply type is delete but the apply type is {0}",
                        applyType);

                    // duplicate keys can come in during recovery.
                    if (this.stateProviderMetadataManager.IsStateProviderDeleted(replicationMetadata.StateProviderId))
                    {
                        var metadata =
                            this.stateProviderMetadataManager.DeletedStateProviders[replicationMetadata.StateProviderId];

                        // Managed sets commit lsn as delete lsn. Native sets apply lsn as delete lsn.
                        // Make assert weaker here to make sure it doesn't fail during upgrade where
                        // managed can apply operations generated from native and vice versa.
                        Utility.Assert(
                            metadata.DeleteLsn == applyLSN || metadata.DeleteLsn == transactionBase.CommitSequenceNumber,
                            "{0}: ApplyOnRecoveryAsync: Lsn obtained through replication is {1}, metadata present in deleted state is {2} and commit sequence number is {3}",
                            this.traceType,
                            applyLSN,
                            metadata,
                            transactionBase.CommitSequenceNumber);
                    }
                    else
                    {
                        var metadata = this.stateProviderMetadataManager.GetMetadata(
                            replicationMetadata.Name,
                            false);
                        Utility.Assert(
                            metadata != null,
                            "{0}: ApplyOnRecoveryAsync: state provider {1} is not present in the active state. ApplyLSN: {2} TxnID:{3} CommitLSN: {4}",
                            this.traceType,
                            replicationMetadata,
                            applyLSN,
                            transactionBase.Id,
                            transactionBase.CommitSequenceNumber);
                        StateManagerLockContext lockContext = await this.stateProviderMetadataManager.LockForWriteAsync(
                            metadata.Name,
                            metadata.StateProviderId,
                            transactionBase,
                            Timeout.InfiniteTimeSpan,
                            CancellationToken.None).ConfigureAwait(false);
                        stateManagerTransactionContext = new StateManagerTransactionContext(
                            transactionBase.Id,
                            lockContext,
                            OperationType.Add);
                        metadata.DeleteLsn = transactionBase.CommitSequenceNumber;
                        this.stateProviderMetadataManager.SoftDelete(metadata.Name, MetadataMode.DelayDelete);

                        this.NotifyStateManagerStateChanged(
                            (ITransaction) transactionBase,
                            metadata.StateProvider,
                            metadata.ParentStateProviderId,
                            NotifyStateManagerChangedAction.Remove);

                        FabricEvents.Events.ApplyOnRecoveryDeleteStateProvider(
                            this.traceType,
                            metadata.StateProviderId,
                            transactionBase.Id);
                    }
                }

                return stateManagerTransactionContext;
            }
            catch (Exception e)
            {
                this.TraceException("ApplyOnRecoveryAsync", e);
                throw;
            }
        }

        /// <summary>
        /// Applies operations to the secondary replica.
        /// </summary>
        internal async Task<StateManagerTransactionContext> ApplyOnSecondaryAsync(
            long applyLSN,
            ReplicatorTransactionBase transactionBase,
            ReplicationMetadata replicationMetadata,
            ApplyContext operationType,
            ApplyContext roleType)
        {
            StateManagerTransactionContext stateManagerTransactionContext = null;
            StateManagerLockContext lockContext = null;
            var applyType = replicationMetadata.StateManagerApplyType;

            Utility.Assert(ApplyContext.REDO == operationType, "operation type should be redo");
            if (applyType == StateManagerApplyType.Insert)
            {
                // check if it has been deleted.
                if (this.stateProviderMetadataManager.IsStateProviderDeleted(replicationMetadata.StateProviderId))
                {
                    FabricEvents.Events.ApplyOnSecondaryInsertStateProviderSkipped(
                        this.traceType,
                        replicationMetadata.StateProviderId);
                }
                else if (this.stateProviderMetadataManager.ContainsKey(replicationMetadata.Name))
                {
                    // duplicate keys can come in during copy.
                    var metadata = this.stateProviderMetadataManager.GetMetadata(replicationMetadata.Name);
                    Utility.Assert(
                        metadata.StateProviderId == replicationMetadata.StateProviderId,
                        "state provider id obtained through replication is {0} and id present in active state is {1}",
                        replicationMetadata,
                        metadata);
                    this.CheckIdempotency(
                        replicationMetadata.Name,
                        applyLSN,
                        transactionBase.CommitSequenceNumber);
                }
                else
                {
                    // Check if it is in the stale state. This check is needed only during build.
                    if (this.stateProviderMetadataManager.IsStateProviderStale(replicationMetadata.StateProviderId))
                    {
                        Metadata metadataToBeRemoved = null;
                        var result =
                            this.stateProviderMetadataManager.DeletedStateProviders.TryRemove(
                                replicationMetadata.StateProviderId,
                                out metadataToBeRemoved);
                        Utility.Assert(
                            result,
                            "remove of stale state provider failed for {0}",
                            replicationMetadata.StateProviderId);
                    }

                    // Insert
                    lockContext =
                        await
                            this.stateProviderMetadataManager.LockForWriteAsync(
                                replicationMetadata.Name,
                                replicationMetadata.StateProviderId,
                                transactionBase,
                                Timeout.InfiniteTimeSpan,
                                CancellationToken.None).ConfigureAwait(false);
                    stateManagerTransactionContext = new StateManagerTransactionContext(
                        transactionBase.Id,
                        lockContext,
                        OperationType.Add);
                    var stateProvider = this.CreateStateProvider(
                        replicationMetadata.Name,
                        replicationMetadata.Type);

                    var metadata = new Metadata(
                        replicationMetadata.Name,
                        replicationMetadata.Type,
                        stateProvider,
                        replicationMetadata.InitializationContext,
                        replicationMetadata.StateProviderId,
                        replicationMetadata.ParentStateProviderId,
                        transactionBase.CommitSequenceNumber,
                        -1,
                        MetadataMode.Active,
                        false);

                    await this.InitializeStateProvidersAsync(metadata, true).ConfigureAwait(false);

                    this.UpdateLastStateProviderId(replicationMetadata.StateProviderId);

                    this.NotifyStateManagerStateChanged(
                        (ITransaction) transactionBase,
                        metadata.StateProvider,
                        metadata.ParentStateProviderId,
                        NotifyStateManagerChangedAction.Add);

                    FabricEvents.Events.ApplyOnSecondaryInsertStateProvider(
                        this.traceType,
                        metadata.StateProviderId,
                        transactionBase.Id);
                }
            }
            else
            {
                // Must be delete.
                Utility.Assert(
                    applyType == StateManagerApplyType.Delete,
                    "expected apply type is delete but the apply type is {0}",
                    applyType);

                // duplicate keys can come in during copy.
                if (this.stateProviderMetadataManager.IsStateProviderDeleted(replicationMetadata.StateProviderId))
                {
                    var metadata =
                        this.stateProviderMetadataManager.DeletedStateProviders[replicationMetadata.StateProviderId];

                    // Managed sets commit lsn as delete lsn. Native sets apply lsn as delete lsn.
                    // Make assert weaker here to make sure it doesn't fail during upgrade where
                    // managed can apply operations generated from native and vice versa.
                    Utility.Assert(
                        metadata.DeleteLsn == applyLSN || metadata.DeleteLsn == transactionBase.CommitSequenceNumber,
                        "Lsn obtained through replication is {0}, metadata present in deleted state is {1} and commit sequence number is {2}",
                        applyLSN,
                        metadata,
                        transactionBase.CommitSequenceNumber);
                }
                else
                {
                    if (this.stateProviderMetadataManager.IsStateProviderStale(replicationMetadata.StateProviderId))
                    {
                        Metadata metadataToBeRemoved = null;
                        var result =
                            this.stateProviderMetadataManager.DeletedStateProviders.TryRemove(
                                replicationMetadata.StateProviderId,
                                out metadataToBeRemoved);
                        Utility.Assert(
                            result,
                            "remove of stale state provider failed",
                            replicationMetadata.StateProviderId);
                    }

                    var metadata = this.stateProviderMetadataManager.GetMetadata(replicationMetadata.Name, false);
                    Utility.Assert(
                        metadata != null,
                        "metadata {0} is not present in active state",
                        replicationMetadata);
                    Utility.Assert(
                        metadata != null,
                        "state provider {0} not present during secondary apply",
                        replicationMetadata.Name.OriginalString);
                    lockContext =
                        await
                            this.stateProviderMetadataManager.LockForWriteAsync(
                                metadata.Name,
                                metadata.StateProviderId,
                                transactionBase,
                                Timeout.InfiniteTimeSpan,
                                CancellationToken.None).ConfigureAwait(false);
                    stateManagerTransactionContext = new StateManagerTransactionContext(
                        transactionBase.Id,
                        lockContext,
                        OperationType.Remove);
                    metadata.DeleteLsn = transactionBase.CommitSequenceNumber;
                    this.stateProviderMetadataManager.SoftDelete(metadata.Name, MetadataMode.DelayDelete);

                    this.NotifyStateManagerStateChanged(
                        (ITransaction) transactionBase,
                        metadata.StateProvider,
                        metadata.ParentStateProviderId,
                        NotifyStateManagerChangedAction.Remove);

                    FabricEvents.Events.ApplyOnSecondaryDeleteStateProvider(
                        this.traceType,
                        metadata.StateProviderId,
                        transactionBase.Id);
                }
            }

            return stateManagerTransactionContext;
        }

        /// <summary>
        /// Begin copy on local state.
        /// </summary>
        internal void BeginSetCurrentLocalState()
        {
            FabricEvents.Events.BeginSetCurrentLocalState(this.traceType);

            // Change mode of all deleted state providers to FalseProgress
            foreach (var kvp in this.stateProviderMetadataManager.DeletedStateProviders)
            {
                var metadata = kvp.Value;
                metadata.MetadataMode = MetadataMode.FalseProgress;
            }

            // Save state to avoid creating state providers in case the same data is obtained thro copy and track them to avoid sp state file leaks.
            this.stateProviderMetadataManager.MoveStateProvidersToDeletedList();

            var copyOfDeletedStateProviders = this.stateProviderMetadataManager.DeletedStateProviders;
            this.stateProviderMetadataManager.Dispose();

            // re-initialize
            this.stateProviderMetadataManager = new StateProviderMetadataManager(this.LoggingReplicator, this.traceType);

            // Track these state providers for file clean up and avoid recreating them, if they are present in secondary.
            this.stateProviderMetadataManager.DeletedStateProviders = copyOfDeletedStateProviders;
        }

        /// <summary>
        /// Checks if the key is already present in the dictionary and the LSNs match.
        /// </summary>
        /// <param name="stateProviderName">state provider name.</param>
        /// <param name="applyLSN"></param>
        /// <param name="commitSequenceNumber"></param>
        /// <returns></returns>
        internal void CheckIdempotency(Uri stateProviderName, long applyLSN, long commitSequenceNumber)
        {
            var metadata = this.stateProviderMetadataManager.GetMetadata(stateProviderName, false);
            Utility.Assert(
                metadata != null, 
                "{0}: CheckIdempotency: metadata != null. Name: {1} ApplyLSN: {2} CommitLSN: {3}",
                this.traceType,
                stateProviderName,
                applyLSN,
                commitSequenceNumber);

            // Managed sets commit lsn as create lsn. Native sets apply lsn as create lsn.
            // Make assert weaker here to make sure it doesn't fail during upgrade where
            // managed can apply operations generated from native and vice versa.
            Utility.Assert(
                metadata.CreateLsn == applyLSN || metadata.CreateLsn == commitSequenceNumber,
                "{0}: CheckIdempotency: Idempotency check failed: lsn in active state is {1}, lsn from replicator is: {2} and commit sequence number is {3}. Name: {4}",
                this.traceType,
                metadata,
                applyLSN,
                commitSequenceNumber,
                stateProviderName);
        }

        /// <summary>
        /// Delete state providers that are no more in the log.
        /// </summary>
        internal async Task CleanUpMetadataAsync()
        {
            try
            {
                var stateProvidersToBeDeleted = new List<long>();
                var cleanUpLsn = this.LoggingReplicator.GetSafeLsnToRemoveStateProvider();

                FabricEvents.Events.CleanupMetadataLsn(this.traceType, cleanUpLsn);

                // Remove state for false progressed state providers and deleted state providers.
                foreach (var metadata in this.stateProviderMetadataManager.GetDeletedStateProviders())
                {
                    var shouldBeDeleted = false;
                    Utility.Assert(
                        metadata.MetadataMode == MetadataMode.FalseProgress
                        || metadata.MetadataMode == MetadataMode.DelayDelete,
                        "{0}: CleanUpMetadataAsync: Metadata mode cannot be active. SPID: {1} MetadataMode: {2}",
                        this.traceType,
                        metadata.StateProviderId,
                        metadata.MetadataMode);

                    if (metadata.MetadataMode == MetadataMode.DelayDelete)
                    {
                        Utility.Assert(
                            metadata.DeleteLsn != -1,
                            "{0}: CleanUpMetadataAsync: Delete lsn has not been set. SPID: {1} DeleteLsn: {2}",
                            this.traceType,
                            metadata.StateProviderId,
                            metadata.DeleteLsn);

                        if (metadata.DeleteLsn < cleanUpLsn)
                        {
                            shouldBeDeleted = true;
                        }
                    }
                    else
                    {
                        // For stale state providers, delete lsn might or might not be present.
                        // It is important to check the lsn before calling remove state as copy replay 
                        // can happen during checkpoint.
                        var removeLsn = metadata.DeleteLsn;
                        if (removeLsn == -1)
                        {
                            removeLsn = metadata.CreateLsn;
                        }

                        Utility.Assert(
                            metadata.CreateLsn != -1,
                            "{0}: CleanUpMetadataAsync: Create lsn has not been set. SPID: {1} CreateLsn: {2}",
                            this.traceType,
                            metadata.StateProviderId,
                            metadata.CreateLsn);

                        if (removeLsn < cleanUpLsn)
                        {
                            shouldBeDeleted = true;
                        }
                    }

                    if (shouldBeDeleted)
                    {
                        FabricEvents.Events.CleanupMetadataRemoveStateProvider(
                            this.traceType,
                            metadata.StateProviderId);

                        await metadata.StateProvider.RemoveStateAsync(metadata.StateProviderId).ConfigureAwait(false);

                        // #10129507: Clean up empty folders (issue was with RQ and RD)
                        DeleteStateProviderWorkDirectory(metadata.StateProviderId);

                        metadata.StateProvider.Abort();

                        stateProvidersToBeDeleted.Add(metadata.StateProviderId);
                    }
                }

                // remove from delete list.
                foreach (var stateproviderId in stateProvidersToBeDeleted)
                {
                    Metadata metadataToBeRemoved = null;
                    var isRemoved = this.stateProviderMetadataManager.DeletedStateProviders.TryRemove(
                        stateproviderId,
                        out metadataToBeRemoved);
                    Utility.Assert(
                        isRemoved,
                        "{0}: CleanUpMetadataAsync: failed to remove state provider {1} from the delete list",
                        this.traceType,
                        stateproviderId);

                    if (this.stateProviderMetadataManager.CopyOrCheckpointMetadataSnapshot.ContainsKey(stateproviderId))
                    {
                        this.stateProviderMetadataManager.CopyOrCheckpointMetadataSnapshot.Remove(stateproviderId);
                    }
                }
            }
            catch (Exception e)
            {
                this.TraceException("CleanUpMetadataAsync", e);
                throw;
            }
        }

        /// <summary>
        /// Close all State Providers: (Active and TransientDeleted) +  Deleted state providers.
        /// </summary>
        internal async Task CloseStateProvidersAsync()
        {
            List<Task> taskList = new List<Task>(2);

            // 1. Add all Active and TransientDeleted state providers to the close task.
            taskList.Add(this.apiDispatcher.CloseAsync(
                this.stateProviderMetadataManager.GetMetadataCollection(),
                ApiDispatcher.FailureAction.AbortStateProvider));

            // 2. Add all Deleted state providers to the close task.
            taskList.Add(this.apiDispatcher.CloseAsync(
                this.stateProviderMetadataManager.DeletedStateProviders.Values,
                ApiDispatcher.FailureAction.AbortStateProvider));

            // The Task for waiting all CloseAsync finish is exception free, since for every CloseAsync
            // failure, the state provider will call Abort which will never throw exception.
            await Task.WhenAll(taskList).ConfigureAwait(false);
        }

        /// <summary>
        /// Complete checkpoint on local state.
        /// </summary>
        /// <returns></returns>
        internal async Task CompleteCheckpointOnLocalStateAsync()
        {
            FabricEvents.Events.CompleteCheckpointOnLocalStateBegin(this.traceType);

            if (FabricFile.Exists(this.TemporaryCheckpointFileName))
            {
                await StateManagerFile.SafeFileReplaceAsync(
                    this.CheckpointFileName,
                    this.TemporaryCheckpointFileName,
                    this.BackupCheckpointFileName,
                    this.traceType).ConfigureAwait(false);
            }
            else if (FabricFile.Exists(this.BackupCheckpointFileName))
            {
                Utility.Assert(
                    FabricFile.Exists(this.CheckpointFileName),
                    "{0}: CompleteCheckpointOnLocalStateAsync: Checkpoint file does not exist when it is needed. CheckpointFileName: {1}",
                    this.traceType,
                    this.CheckpointFileName);
                FabricFile.Delete(this.BackupCheckpointFileName);
            }
            else
            {
                FabricEvents.Events.CompleteCheckpointOnLocalStateNoRenameNeeded(this.traceType);
            }

            FabricEvents.Events.CompleteCheckpointOnLocalStateEnd(this.traceType);
        }

        /// <summary>
        /// Add copy data to in-memory state.
        /// </summary>
        internal async Task CopyToLocalStateAsync(SerializableMetadata serializableMetadata)
        {
            // set current state cannot obtain data such that it is present both in active and deleted list.
            if (serializableMetadata.MetadataMode == MetadataMode.Active)
            {
                var shouldStateProviderBeCreated = true;

                // check if the state provider is already present in the recovered state.
                if (this.stateProviderMetadataManager.IsStateProviderStale(serializableMetadata.StateProviderId))
                {
                    var metadata =
                        this.stateProviderMetadataManager.DeletedStateProviders[serializableMetadata.StateProviderId];

                    this.ThrowIfNotValid(
                        metadata.MetadataMode == MetadataMode.FalseProgress,
                        "StateManager.CopyToLocalStateAsync. expected metadata mode is false progress, but obtained is {0}",
                        metadata);

                    await
                        this.stateProviderMetadataManager.AcquireLockAndAddAsync(
                            metadata.Name,
                            metadata,
                            Timeout.InfiniteTimeSpan,
                            CancellationToken.None).ConfigureAwait(false);

                    FabricEvents.Events.CopyToLocalStateAddStateProvider(
                        this.traceType,
                        metadata.StateProviderId);

                    Metadata metadataToBeRemoved = null;
                    var isRemoved =
                        this.stateProviderMetadataManager.DeletedStateProviders.TryRemove(
                            metadata.StateProviderId,
                            out metadataToBeRemoved);
                    Utility.Assert(
                        isRemoved,
                        "{0}: CopyToLocalStateAsync: failed to remove data for key {1}", 
                        this.traceType,
                        metadata);

                    metadata.MetadataMode = MetadataMode.Active;
                    metadata.StateProvider.BeginSettingCurrentState();
                    this.copyProgressSet.Add(metadata.StateProvider);

                    shouldStateProviderBeCreated = false;
                }

                if (shouldStateProviderBeCreated)
                {
                    var stateProvider = this.CreateStateProvider(
                        serializableMetadata.Name,
                        serializableMetadata.Type);

                    var metadata = new Metadata(
                        serializableMetadata.Name,
                        serializableMetadata.Type,
                        stateProvider,
                        serializableMetadata.InitializationContext,
                        serializableMetadata.StateProviderId,
                        serializableMetadata.ParentStateProviderId,
                        serializableMetadata.CreateLsn,
                        serializableMetadata.DeleteLsn,
                        serializableMetadata.MetadataMode,
                        false);

                    var stateProviderMetadataList = this.InitializeStateProvidersInOrder(metadata);

                    await
                        this.stateProviderMetadataManager.AcquireLockAndAddAsync(
                            metadata.Name,
                            metadata,
                            Timeout.InfiniteTimeSpan,
                            CancellationToken.None).ConfigureAwait(false);

                    FabricEvents.Events.CopyToLocalStateCreateStateProvider(
                        this.traceType,
                        metadata.StateProviderId);

                    foreach (var stateProviderMetadata in stateProviderMetadataList)
                    {
                        // state provider initialize methods
                        await stateProviderMetadata.StateProvider.OpenAsync().ConfigureAwait(false);
                        await stateProviderMetadata.StateProvider.RecoverCheckpointAsync().ConfigureAwait(false);
                        await stateProviderMetadata.StateProvider.OnRecoveryCompletedAsync().ConfigureAwait(false);
                        stateProviderMetadata.StateProvider.BeginSettingCurrentState();
                        this.copyProgressSet.Add(stateProviderMetadata.StateProvider);
                    }
                }
            }
            else
            {
                // Check if it is already present in the stale state, it cannot be present in active and stale state
                if (this.stateProviderMetadataManager.IsStateProviderStale(serializableMetadata.StateProviderId))
                {
                    var metadata =
                        this.stateProviderMetadataManager.DeletedStateProviders[serializableMetadata.StateProviderId];

                    // This state provider must have been a part of the recovered data and its metadata mode needs to updated.
                    metadata.MetadataMode = serializableMetadata.MetadataMode;
                    metadata.DeleteLsn = serializableMetadata.DeleteLsn;
                }
                else
                {
                    var stateProvider = this.CreateStateProvider(
                        serializableMetadata.Name,
                        serializableMetadata.Type);

                    var metadata = new Metadata(
                        serializableMetadata.Name,
                        serializableMetadata.Type,
                        stateProvider,
                        serializableMetadata.InitializationContext,
                        serializableMetadata.StateProviderId,
                        serializableMetadata.ParentStateProviderId,
                        serializableMetadata.CreateLsn,
                        serializableMetadata.DeleteLsn,
                        serializableMetadata.MetadataMode,
                        false);

                    var addToDeleteList =
                        this.stateProviderMetadataManager.DeletedStateProviders.TryAdd(
                            metadata.StateProviderId,
                            metadata);
                    Utility.Assert(
                        addToDeleteList,
                        "{0}: CopyToLocalStateAsync: failed to add state provider {1} to delete list",
                        this.traceType,
                        metadata);
                    metadata.MetadataMode = serializableMetadata.MetadataMode;

                    FabricEvents.Events.CopyToLocalStateDeleteStateProvider(
                        this.traceType,
                        metadata.StateProviderId);

                    var stateProviderMetadataList = this.InitializeStateProvidersInOrder(metadata);

                    foreach (var stateProviderMetadata in stateProviderMetadataList)
                    {
                        // state provider initialize methods
                        // open is called for now to be consistent with delete during replication.
                        await stateProviderMetadata.StateProvider.OpenAsync().ConfigureAwait(false);
                    }
                }
            }
        }

        internal long CreateStateProviderId()
        {
            var stateProviderId = DateTime.UtcNow.ToFileTimeUtc();
            lock (this.stateProviderIdLock)
            {
                if (stateProviderId > this.lastStateProviderId)
                {
                    this.lastStateProviderId = stateProviderId;
                }
                else
                {
                    ++this.lastStateProviderId;
                    stateProviderId = this.lastStateProviderId;
                }
            }

            return stateProviderId;
        }

        /// <summary>
        /// Completes copy on local state.
        /// </summary>
        internal void EndSetCurrentLocalState()
        {
            var items = this.stateProviderMetadataManager.GetMetadataCollection()
                .Where(metadata => metadata.ParentStateProviderId == EmptyStateProviderId)
                .Select(metadata => metadata.StateProvider)
                .ToList();

            this.NotifyStateManagerStateChanged(items.ToAsyncEnumerable());
        }

        /// <summary>
        /// Initializes state providers.
        /// </summary>
        internal async Task InitializeStateProvidersAsync(Metadata metadata, bool shouldMetadataBeAdded)
        {
            IReadOnlyList<Metadata> stateProviderList = null;

            Utility.Assert(
                metadata.StateProvider != null,
                "{0}: InitializeStateProvidersAsync: Null state provider during initialization. SecondaryInsertCase: {1}",
                this.traceType,
                shouldMetadataBeAdded);

            try
            {
                stateProviderList = this.InitializeStateProvidersInOrder(metadata);
            }
            catch (Exception e)
            {
                TraceException(
                    this.traceType,
                    "InitializeStateProvidersAsync",
                    string.Format(CultureInfo.InvariantCulture, "Exception while initializing state provider {0}.",
                        metadata.ToString()),
                    e);
                throw;
            }

            foreach (Metadata stateProviderMetadata in stateProviderList)
            {
                IStateProvider2 stateProvider = stateProviderMetadata.StateProvider;
                Utility.Assert(
                    stateProvider != null,
                    "{0}: InitializeStateProvidersAsync: Null state provider during initialization. SecondaryInsertCase: {1}",
                    this.traceType,
                    stateProviderMetadata.ToString());

                await stateProvider.OpenAsync().ConfigureAwait(false);

                // Note: At this point, the state provider is opened but it is still in transient state, if exception 
                // thrown here, the state provider will not be closed since close state provider only take action on
                // active and deleted. So handle close the transient opened state provider on failure.
                try
                {
                    await stateProvider.RecoverCheckpointAsync().ConfigureAwait(false);
                    await stateProvider.OnRecoveryCompletedAsync().ConfigureAwait(false);

                    FabricEvents.Events.InitializeStateProvidersAcquireLock(
                        this.traceType,
                        stateProviderMetadata.StateProviderId,
                        (int)this.Role);

                    await this.AcquireChangeRoleLockAsync().ConfigureAwait(false);
                    try
                    {
                        ReplicaRole role = this.currentRoleOfStateProviders;
                        FabricEvents.Events.InitializeStateProvider(
                            this.traceType,
                            stateProviderMetadata.ToString(),
                            role.ToString());

                        switch (role)
                        {
                            case ReplicaRole.Primary:
                                await stateProvider.ChangeRoleAsync(ReplicaRole.Primary, CancellationToken.None).ConfigureAwait(false);
                                break;

                            case ReplicaRole.IdleSecondary:
                                await stateProvider.ChangeRoleAsync(ReplicaRole.IdleSecondary, CancellationToken.None).ConfigureAwait(false);
                                break;

                            case ReplicaRole.ActiveSecondary:
                                await stateProvider.ChangeRoleAsync(ReplicaRole.IdleSecondary, CancellationToken.None).ConfigureAwait(false);
                                await stateProvider.ChangeRoleAsync(ReplicaRole.ActiveSecondary, CancellationToken.None).ConfigureAwait(false);
                                break;

                            case ReplicaRole.None:
                            case ReplicaRole.Unknown:
                            default:
                                Utility.CodingError(
                                    "{0}: InitializeStateProvidersAsync: state providers cannot be applied(excluding recovery) when the state manager is in role {1}.",
                                    this.traceType,
                                    this.Role);
                                break;
                        }

                        if (shouldMetadataBeAdded)
                        {
                            bool result = this.stateProviderMetadataManager.Add(
                                stateProvider.Name,
                                stateProviderMetadata);
                            Utility.Assert(
                                result,
                                "{0}: InitializeStateProvidersAsync: Add has failed because the key {1} is already present.",
                                this.traceType,
                                stateProvider.Name);
                        }
                        else
                        {
                            this.stateProviderMetadataManager.ResetTransientState(stateProvider.Name);
                        }
                    }
                    finally
                    {
                        this.changeRoleLock.Release();
                    }
                }
                catch (Exception e)
                {
                    // If Close state provider failed we abort it, and abort state provider will not throw.
                    await this.apiDispatcher.CloseAsync(
                        stateProviderMetadata,
                        ApiDispatcher.FailureAction.AbortStateProvider);
                    TraceException(
                        this.traceType,
                        "InitializeStateProvidersAsync",
                        string.Format(CultureInfo.InvariantCulture, "Exception while initializing state provider {0}.",
                            metadata.ToString()),
                        e);
                    throw;
                }
            }
        }

        /// <summary>
        /// Local checkpoint.
        /// </summary>
        internal async Task LocalStateCheckpointAsync()
        {
            try
            {
                FabricEvents.Events.LocalStateCheckpointBegin(this.traceType);

                var serializableMetadataCollection = new List<SerializableMetadata>();
                foreach (var kvp in this.stateProviderMetadataManager.CopyOrCheckpointMetadataSnapshot)
                {
                    var metadata = kvp.Value;
                    Utility.Assert(
                        metadata != null,
                        "{0}: LocalStateCheckpointAsync: metadata != null",
                        this.traceType);

                    serializableMetadataCollection.Add(
                        new SerializableMetadata(
                            metadata.Name,
                            metadata.Type,
                            metadata.InitializationContext,
                            metadata.StateProviderId,
                            metadata.ParentStateProviderId,
                            metadata.MetadataMode,
                            metadata.CreateLsn,
                            metadata.DeleteLsn));
                }

                FabricEvents.Events.MetadataSerializerSerializeCount(
                    this.traceType,
                    serializableMetadataCollection.Count);

                // Note: For the checkpoint file write path, we directly pass prepareCheckpointLSN into the 
                // CreateAsync and write to the file.
                StateManagerFile stateManagerFile = await StateManagerFile.CreateAsync(
                    this.TemporaryCheckpointFileName,
                    this.PartitionId,
                    this.ReplicaId,
                    serializableMetadataCollection,
                    allowPrepareCheckpointLSNToBeInvalid: false,
                    prepareCheckpointLSN: this.prepareCheckpointLSN,
                    cancellationToken: CancellationToken.None).ConfigureAwait(false);

                FabricEvents.Events.LocalStateCheckpoint(this.traceType, stateManagerFile.FileSize);
            }
            catch (Exception e)
            {
                this.TraceException("LocalStateCheckpointAsync", e);
                throw;
            }
        }

        /// <summary>
        /// Recover local checkpoint.
        /// </summary>
        internal async Task LocalStateRecoverCheckpointAsync()
        {
            Utility.Assert(
                !this.stateProviderMetadataManager.Any(),
                "{0}: LocalStateRecoverCheckpointAsync. In memory state is empty",
                this.traceType);

            FabricEvents.Events.LocalStateRecoverCheckpointBegin(this.traceType);

            try
            {
                StateManagerFile stateManagerFile = await StateManagerFile.OpenAsync(
                    this.CheckpointFileName, 
                    this.traceType, 
                    CancellationToken.None).ConfigureAwait(false);
                this.prepareCheckpointLSN = stateManagerFile.Properties.PrepareCheckpointLSN;

                var sortedState = stateManagerFile.StateProviderMetadata.OrderByDescending(o => o.Name.ToString());
                var count = 0;
                var activeCount = 0;
                var deleteCount = 0;

                // populate in-memory state
                foreach (var serializableMeatadata in sortedState)
                {
                    count++;

                    var stateProvider = this.CreateStateProvider(
                        serializableMeatadata.Name,
                        serializableMeatadata.Type);

                    var metadata = new Metadata(
                        serializableMeatadata.Name,
                        serializableMeatadata.Type,
                        stateProvider,
                        serializableMeatadata.InitializationContext,
                        serializableMeatadata.StateProviderId,
                        serializableMeatadata.ParentStateProviderId,
                        serializableMeatadata.CreateLsn,
                        serializableMeatadata.DeleteLsn,
                        serializableMeatadata.MetadataMode,
                        false);

                    this.InitializeStateProvidersInOrder(metadata);

                    // MCoskun: Update the last state provider id irrespective of Active or Soft deleted.
                    // Even if the state provider is soft deleted, we do not want the state provider ids to conflict
                    // in case of a undo.
                    this.UpdateLastStateProviderId(metadata.StateProviderId);

                    if (metadata.MetadataMode == MetadataMode.Active)
                    {
                        await
                            this.stateProviderMetadataManager.AcquireLockAndAddAsync(
                                metadata.Name,
                                metadata,
                                Timeout.InfiniteTimeSpan,
                                CancellationToken.None).ConfigureAwait(false);

                        activeCount++;
                        FabricEvents.Events.LocalStateRecoverCheckpointAddStateProvider(
                            this.traceType,
                            metadata.StateProviderId);
                    }
                    else
                    {
                        this.ThrowIfNotValid(
                            metadata.MetadataMode == MetadataMode.DelayDelete
                            || metadata.MetadataMode == MetadataMode.FalseProgress,
                            "StateManager.LocalStateRecoverCheckpointAsync. Invalid metadata mode {0}",
                            metadata.MetadataMode);

                        var isAdded =
                            this.stateProviderMetadataManager.DeletedStateProviders.TryAdd(
                                metadata.StateProviderId,
                                metadata);
                        Utility.Assert(
                            isAdded,
                            "{0}: LocalStateRecoverCheckpointAsync: Failed to add state provider {1} to the delete list",
                            this.traceType,
                            metadata);

                        deleteCount++;
                        FabricEvents.Events.LocalStateRecoverCheckpointDeleteStateProvider(
                            this.traceType,
                            metadata.StateProviderId);
                    }
                }

                FabricEvents.Events.MetadataSerializerDeserializeCount(this.traceType, count, activeCount, deleteCount);

#if !DotNetCoreClr
                // These are new events defined in System.Fabric, existing CoreCLR apps would break 
                // if these events are refernced as it wont be found. As CoreCLR apps carry System.Fabric
                // along with application
                // This is just a mitigation for now. Actual fix being tracked via bug# 11614507

                FabricEvents.Events.LocalStateRecoverCheckpointEnd(this.traceType, this.prepareCheckpointLSN);
#endif

                // populate snapshot for copy in case there is no PrepareCheckpoint call before copy.
                foreach (var metadata in this.stateProviderMetadataManager.GetMetadataCollection())
                {
                    var copiedMetadata = new Metadata(
                        metadata.Name,
                        metadata.Type,
                        metadata.StateProvider,
                        metadata.InitializationContext,
                        metadata.StateProviderId,
                        metadata.ParentStateProviderId,
                        metadata.CreateLsn,
                        metadata.DeleteLsn,
                        metadata.MetadataMode,
                        false);

                    this.stateProviderMetadataManager.CopyOrCheckpointMetadataSnapshot.Add(
                        copiedMetadata.StateProviderId,
                        copiedMetadata);
                }

                foreach (var metadata in this.stateProviderMetadataManager.GetDeletedStateProviders())
                {
                    Utility.Assert(
                        metadata.MetadataMode != MetadataMode.Active,
                        "{0}: LocalStateRecoverCheckpointAsync: invalid metadata mode {1}",
                        this.traceType,
                        metadata);
                    var copiedMetadata = new Metadata(
                        metadata.Name,
                        metadata.Type,
                        metadata.StateProvider,
                        metadata.InitializationContext,
                        metadata.StateProviderId,
                        metadata.ParentStateProviderId,
                        metadata.CreateLsn,
                        metadata.DeleteLsn,
                        metadata.MetadataMode,
                        false);
                    this.stateProviderMetadataManager.CopyOrCheckpointMetadataSnapshot.Add(
                        copiedMetadata.StateProviderId,
                        copiedMetadata);
                }

                this.RemoveUnreferencedStateProviders();
            }
            catch (Exception e)
            {
                this.TraceException("LocalStateRecoverCheckpointAsync", e);
                throw;
            }
        }

        /// <summary>
        /// Remove any unreferences stateproviders.
        /// </summary>
        internal void RemoveUnreferencedStateProviders()
        {
            FabricEvents.Events.RemoveUnreferencedStateProvidersStart(this.traceType, this.workDirectory);

            double elapsedTime;
            Stopwatch stopwatch = Stopwatch.StartNew();

            // Construct a hash set from the currently valid state providers.
            var referencedDirectories = new HashSet<string>(this.GetCurrentStateProviderDirectories());
            var directoriesToBeDeleted = new List<string>();
            
            foreach (string directory in this.GetDirectoriesInWorkDirectory())
            {
                // If this table file does not exist in the hash set, it must not be referenced - so delete it.
                if (!referencedDirectories.Remove(directory))
                {
                    FabricEvents.Events.UnreferencedStateProvidersToBeDeleted(this.traceType, directory);
                    directoriesToBeDeleted.Add(directory);
                }
            }


            // Safety verification - if we didn't find all the currently referenced files, something went
            // wrong and we should not delete any files (case mismatch? drive\network path mismatch?).
            Utility.Assert(
                referencedDirectories.Count == 0,
                "{0}: RemoveUnreferencedStateProviders: failed to find all referenced directories when trimming unreferenced directories.  NOT deleting any directory.",
                this.traceType);

            // We can now assume it's safe to delete this unreferenced sorted table file.
            foreach (var directory in directoriesToBeDeleted)
            {
                try
                {
                    FabricEvents.Events.UnreferencedStateProvidersBeingDeleted(this.traceType, directory);
                    FabricDirectory.Delete(directory, recursive: true);
                }
                catch (Exception e)
                {
                    // Ignore the exception if we failed to delete the folder.
                    this.TraceExceptionWarning(
                        "RemoveUnreferencedStateProviders",
                        string.Format(CultureInfo.InvariantCulture, "Failed to clean up directory {0}.", directory),
                        e);
                }
            }

            stopwatch.Stop();
            elapsedTime = stopwatch.Elapsed.TotalSeconds;

#if DEBUG
            Utility.Assert(
                elapsedTime < StateManagerConstants.MaxRemoveUnreferencedStateProvidersTime,
                "{0}: RemoveUnreferencedStateProviders: Complete must take less than 30 seconds. Total: {1} ",
                this.traceType,
                elapsedTime);
#else
            if (elapsedTime >= StateManagerConstants.MaxRemoveUnreferencedStateProvidersTime)
            {
                FabricEvents.Events.UnreferencedStateProvidersTimer(this.traceType, elapsedTime);
            }
#endif
            FabricEvents.Events.RemoveUnreferencedStateProvidersEnd(this.traceType, this.workDirectory);
        }

        /// <summary>
        /// Gets directory of all state providers present in SM.
        /// </summary>
        /// <returns>Set of directories used by state provioders.</returns>
        internal IEnumerable<string> GetCurrentStateProviderDirectories()
        {
            foreach (var metadata in this.stateProviderMetadataManager.GetMetadataCollection())
            {
                yield return this.GetStateProviderWorkDirectory(metadata.StateProviderId);
            }

            foreach (var metadata in this.stateProviderMetadataManager.GetDeletedStateProviders())
            {
                yield return this.GetStateProviderWorkDirectory(metadata.StateProviderId);
            }
        }

        /// <summary>
        /// Gets an enumeration over all files in the current metadata table.
        /// </summary>
        /// <returns>Set of files references by the metadata table.</returns>
        public string[] GetDirectoriesInWorkDirectory()
        {
            var sb = new StringBuilder();
            sb.Append(this.PartitionId.ToString("N"));
            sb.Append("_");
            sb.Append(this.ReplicaId);

            // Don't include partitionId_ReplicaId/ foler where user may keep private files.
            sb.Append("_");
            string currentDirectoryPrefix = sb.ToString();
            return FabricDirectory.GetDirectories(this.workDirectory, currentDirectoryPrefix + "*", SearchOption.TopDirectoryOnly);
        }

        /// <summary>
        /// Apply on the state manager's local state
        /// </summary>
        internal async Task<object> OnApplyOnLocalStateAsync(
            long applyLSN,
            ReplicatorTransactionBase transactionBase,
            OperationData metaData,
            OperationData data,
            ApplyContext applyContext)
        {
            try
            {
                var roleType = applyContext & ApplyContext.ROLE_MASK;
                var operationType = applyContext & ApplyContext.OPERATION_MASK;
                StateManagerTransactionContext stateManagerTransactionContext = null;

                Utility.Assert(
                    metaData != null,
                    "{0}: OnApplyOnLocalStateAsync: metadata cannot be null",
                    this.traceType);
                Utility.Assert(
                    metaData.Count > 0,
                    "{0}: OnApplyOnLocalStateAsync: metadata count should be greater than zero",
                    this.traceType);
                Utility.Assert(
                    metaData[0].Array != null,
                    "{0}: OnApplyOnLocalStateAsync: metadata array cannot be null",
                    this.traceType);

                MemoryStream memoryStreamForMetaDataReader =
                    new MemoryStream(metaData[0].Array, metaData[0].Offset, metaData[0].Count);
                if (operationType == ApplyContext.FALSE_PROGRESS)
                {
                    // Assert data is null
                    Utility.Assert(
                        data == null,
                        "{0}: OnApplyOnLocalStateAsync: undo data must be null",
                        this.traceType);

                    long stateProviderid;
                    Uri name;
                    StateManagerApplyType applyType;

                    using (var metaDataReader = new InMemoryBinaryReader(memoryStreamForMetaDataReader))
                    {
                        ReplicationMetadata.DeserializeMetadataForUndoOperation(
                            metaDataReader,
                            this.traceType,
                            this.Version,
                            out stateProviderid,
                            out name,
                            out applyType);
                    }

                    await
                        this.ApplyOnFalseProgressAsync(
                            applyLSN,
                            transactionBase,
                            stateProviderid,
                            name,
                            applyType,
                            operationType).ConfigureAwait(false);
                }
                else
                {
                    ReplicationMetadata replicationMetadata = null;

                    // TODO #10279752: Separate redo out of ReplicationMetadata. Making
                    // sure remove set null redo data in both native and managed.
                    // In managed code, delete redo operation is not null.
                    // In native code, delete redo operation is null.
                    // This inconsistency would potentially result in a break in managed
                    // to native migration where primary native replica replicates
                    // serialization mode 0 data to secondary managed replica.
                    if (data != null)
                    {
                        Utility.Assert(
                            data.Count > 0,
                            "{0}: OnApplyOnLocalStateAsync: data count should be greater than zero",
                            this.traceType);
                        Utility.Assert(
                            data[0].Array != null,
                            "{0}: OnApplyOnLocalStateAsync: data array cannot be null",
                            this.traceType);
                        MemoryStream memoryStreamForRedoDataReader =
                            new MemoryStream(data[0].Array, data[0].Offset, data[0].Count);
                        using (var metaDataReader = new InMemoryBinaryReader(memoryStreamForMetaDataReader))
                        {
                            using (var redoDataReader = new InMemoryBinaryReader(memoryStreamForRedoDataReader))
                            {
                                replicationMetadata = ReplicationMetadata.Deserialize(
                                    metaDataReader,
                                    redoDataReader,
                                    this.traceType,
                                    this.Version);
                            }
                        }
                    }
                    else
                    {
                        // In managed to native upgrade scenario, native remove redo
                        // operation is null.
                        using (var metaDataReader = new InMemoryBinaryReader(memoryStreamForMetaDataReader))
                        {
                            replicationMetadata = ReplicationMetadata.Deserialize(
                                metaDataReader,
                                null,
                                this.traceType,
                                this.Version);
                        }
                    }

                    this.ThrowIfNotValid(
                        replicationMetadata.StateManagerApplyType != StateManagerApplyType.Invalid,
                        "StateManager.OnApplyOnLocalStateAsync. StateManager apply type : {0}: is not a valid apply type",
                        replicationMetadata.StateManagerApplyType);

                    if (ApplyContext.PRIMARY == roleType)
                    {
                        Utility.Assert(
                            ApplyContext.REDO == operationType,
                            "{0}: OnApplyOnLocalStateAsync: StateManager: Only redo operations are supported on primary. OperationType: {1}",
                            this.traceType,
                            operationType);

                        await
                            this.ApplyOnPrimaryAsync(
                                applyLSN,
                                transactionBase,
                                replicationMetadata.StateManagerApplyType,
                                replicationMetadata.Name,
                                replicationMetadata.StateProviderId).ConfigureAwait(false);
                    }
                    else if (roleType == ApplyContext.SECONDARY)
                    {
                        Utility.Assert(
                            operationType == ApplyContext.REDO || operationType == ApplyContext.FALSE_PROGRESS,
                            "{0}: OnApplyOnLocalStateAsync: Invalid operation type {1}",
                            this.traceType,
                            operationType);

                        stateManagerTransactionContext =
                            await
                                this.ApplyOnSecondaryAsync(
                                    applyLSN,
                                    transactionBase,
                                    replicationMetadata,
                                    operationType,
                                    roleType).ConfigureAwait(false);
                    }
                    else
                    {
                        Utility.Assert(
                            ApplyContext.RECOVERY == roleType,
                            "{0}: OnApplyOnLocalStateAsync: ApplyContext.RECOVERY == roleType.Role type is: {1}",
                            this.traceType,
                            roleType);
                        Utility.Assert(
                            operationType == ApplyContext.REDO,
                            "{0}: OnApplyOnLocalStateAsync: Operation Type should be redo only. But Operation type is: {1}",
                            this.traceType,
                            operationType);

                        stateManagerTransactionContext =
                            await this.ApplyOnRecoveryAsync(applyLSN, transactionBase, replicationMetadata).ConfigureAwait(false);
                    }
                }

                if (stateManagerTransactionContext != null)
                {
                    return stateManagerTransactionContext;
                }

                return null;
            }
            catch (Exception e)
            {
                this.TraceException("OnApplyOnLocalStateAsync", e);
                throw;
            }
        }

        /// <summary>
        /// Called on local recover completion.
        /// </summary>
        internal void OnLocalRecoveryCompleted()
        {
        }

        /// <summary>
        /// Calls open on all the registered state providers.
        /// </summary>
        /// <returns></returns>
        internal async Task OpenStateProvidersAsync()
        {
            var taskList = new List<Task>();
            var stateProvidersThatCompletedOpen = new ConcurrentBag<IStateProvider2>();

            foreach (var metadata in this.stateProviderMetadataManager.GetMetadataCollection())
            {
                taskList.Add(
                    Task.Factory.StartNew(
                        async () =>
                        {
                            try
                            {
                                Utility.Assert(
                                    metadata != null,
                                    "{0}: OpenStateProvidersAsync: metadata != null",
                                    this.traceType);
                                await metadata.StateProvider.OpenAsync().ConfigureAwait(false);
                                stateProvidersThatCompletedOpen.Add(metadata.StateProvider);
                            }
                            catch (Exception e)
                            {
                                TraceException(
                                    this.traceType,
                                    "OpenStateProvidersAsync",
                                    string.Format(CultureInfo.InvariantCulture, "Exception while opening state provider {0}.", metadata.StateProvider.Name),
                                    e);
                                throw;
                            }
                        }).Unwrap());
            }

            // Opening deleted state providers as well so that remove state can be called on them.
            foreach (var metadata in this.stateProviderMetadataManager.DeletedStateProviders.Values)
            {
                taskList.Add(
                    Task.Factory.StartNew(
                        async () =>
                        {
                            try
                            {
                                Utility.Assert(
                                    metadata != null,
                                    "{0}: OpenStateProvidersAsync: metadata != null",
                                    this.traceType);
                                await metadata.StateProvider.OpenAsync().ConfigureAwait(false);
                                stateProvidersThatCompletedOpen.Add(metadata.StateProvider);
                            }
                            catch (Exception e)
                            {
                                TraceException(
                                    this.traceType,
                                    "OpenStateProvidersAsync",
                                    string.Format(
                                        CultureInfo.InvariantCulture,
                                        "Exception while opening deleted state provider {0}.",
                                        metadata.StateProvider.Name.ToString()),
                                    e);
                                throw;
                            }
                        }).Unwrap());
            }

            try
            {
                await Task.WhenAll(taskList).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                this.TraceException("OpenStateProvidersAsync", e);

                // abort all state providers that have successfully completed open before throwing the exception
                this.AbortStateProviders(stateProvidersThatCompletedOpen);

                throw;
            }
        }

        /// <summary>
        /// Calls checkpoint on all the registered state providers.
        /// </summary>
        /// <param name="performMode">Represents different mode to perform checkpoint.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        internal async Task PerformCheckpointStateProvidersAsync(PerformCheckpointMode performMode)
        {
            try
            {
                this.stateProviderMetadataManager.CopyOrCheckpointLock.EnterWriteLock();
                this.stateProviderMetadataManager.CopyOrCheckpointMetadataSnapshot.Clear();

                // PrepareCheckpointSnap is stable, it can become the copyorcheckpointSnap
                foreach (var metadata in this.prepareCheckpointMetadataSnapshot.Values)
                {
                    FabricEvents.Events.PerformCheckpointAddStateProvider(
                        this.traceType,
                        metadata.StateProviderId);
                    this.stateProviderMetadataManager.CopyOrCheckpointMetadataSnapshot.Add(
                        metadata.StateProviderId,
                        metadata);
                }

                this.stateProviderMetadataManager.CopyOrCheckpointLock.ExitWriteLock();

                var taskList = new List<Task>();
                int activeCount = 0;

                foreach (var metadata in this.stateProviderMetadataManager.CopyOrCheckpointMetadataSnapshot.Values)
                {
                    Utility.Assert(
                        metadata != null, 
                        "{0}: PerformCheckpointStateProvidersAsync: metadata != null",
                        this.traceType);

                    // since the state in the snap has become stable, it is safe to not call checkpoint on the deleted state providers.
                    if (metadata.MetadataMode == MetadataMode.Active)
                    {
                        taskList.Add(metadata.StateProvider.PerformCheckpointAsync(performMode, CancellationToken.None));
                        activeCount++;
                    }
                }   

                try
                {
                    await Task.WhenAll(taskList).ConfigureAwait(false);
                    FabricEvents.Events.StateProviderCount(this.traceType, activeCount);
                }
                catch (Exception e)
                {
                    TraceException(this.traceType, "PerformCheckpointStateProvidersAsync", string.Empty, e);
                    throw;
                }
            }
            finally
            {
                if (this.stateProviderMetadataManager.CopyOrCheckpointLock.IsWriteLockHeld)
                {
                    this.stateProviderMetadataManager.CopyOrCheckpointLock.ExitWriteLock();
                }
            }
        }

        /// <summary>
        /// Calls checkpoint on all the registered state providers.
        /// </summary>
        /// <returns></returns>
        internal async Task PrepareCheckpointStateProvidersAsync(long checkpointLSN)
        {
            var taskList = new List<Task>();

            foreach (var metadata in this.prepareCheckpointMetadataSnapshot.Values)
            {
                Utility.Assert(
                    metadata != null,
                    "{0}: PrepareCheckpointStateProvidersAsync: metadata != null. CheckpointLSN: {1}",
                    this.traceType,
                    checkpointLSN);

                // metadata.ShouldCheckpoint should be called here and esnured taht performm is not called on it.
                if (metadata.MetadataMode == MetadataMode.Active)
                {
                    taskList.Add(metadata.StateProvider.PrepareCheckpointAsync(checkpointLSN, CancellationToken.None));
                }
            }

            try
            {
                await Task.WhenAll(taskList).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                this.TraceException("PrepareCheckpointStateProvidersAsync", e);
                throw;
            }
        }

        /// <summary>
        /// Removes local state.
        /// </summary>
        internal void RemoveLocalState()
        {
            if (FabricFile.Exists(this.CheckpointFileName))
            {
                FabricFile.Delete(this.CheckpointFileName);
            }

            if (FabricFile.Exists(this.TemporaryCheckpointFileName))
            {
                FabricFile.Delete(this.TemporaryCheckpointFileName);
            }

            if (FabricFile.Exists(this.BackupCheckpointFileName))
            {
                FabricFile.Delete(this.BackupCheckpointFileName);
            }
        }

        /// <summary>
        /// Gets copy data for local state.
        /// </summary>
        /// <param name="operationData"></param>
        /// <returns></returns>
        internal async Task SetCurrentLocalStateAsync(OperationData operationData)
        {
            foreach (var metadata in StateManagerFile.ReadCopyData(operationData, this.traceType))
            {
                await this.CopyToLocalStateAsync(metadata).ConfigureAwait(false);
            }
        }

        /// <summary>
        /// Unlock local state.
        /// </summary>
        internal void UnlockOnLocalState(NamedOperationContext context)
        {
            Utility.Assert(
                context != null,
                "{0}: UnlockOnLocalState: context != null",
                this.traceType);
            try
            {
                var transactionContext =
                    context.OperationContext as StateManagerTransactionContext;

                if (transactionContext == null)
                {
                    FabricEvents.Events.UnlockOnLocalStateInvalidContext(this.traceType);
                }

                Utility.Assert(
                    transactionContext.TransactionId != StateManagerConstants.InvalidTransactionId,
                    "{0}: UnlockOnLocalState: owner transaction cannot be null",
                    this.traceType);

                FabricEvents.Events.UnlockOnLocalState(this.traceType, transactionContext.TransactionId);

                this.stateProviderMetadataManager.Unlock(transactionContext);
            }
            catch (Exception e)
            {
                this.TraceException("UnlockOnLocalState", e);
                throw;
            }
        }

        internal void UpdateLastStateProviderId(long stateProviderId)
        {
            lock (this.stateProviderIdLock)
            {
                if (stateProviderId > this.lastStateProviderId)
                {
                    this.lastStateProviderId = stateProviderId;
                }
            }
        }

        /// <summary>
        /// Creates and Gets work directory for state providers.
        /// </summary>
        /// <param name="stateProviderId">The State Provider id.</param>
        /// <devnote>
        /// Exposed for testability only.
        /// </devnote>
        internal string CreateAndGetStateProviderWorkDirectory(long stateProviderId)
        {
            string stateProviderWorkDirectory = this.GetStateProviderWorkDirectory(stateProviderId);
            this.MoveNativeStateProvidersFoldersIfNecessary(stateProviderId, stateProviderWorkDirectory);
            FabricDirectory.CreateDirectory(stateProviderWorkDirectory);
            return stateProviderWorkDirectory;
        }

        /// <summary>
        /// Deletes the work directory for the relevant state provider.
        /// </summary>
        /// <param name="stateProviderId">The State Provider id.</param>
        internal void DeleteStateProviderWorkDirectory(long stateProviderId)
        {
            string stateProviderWorkDirectory = this.GetStateProviderWorkDirectory(stateProviderId);

            bool directoryExists = FabricDirectory.Exists(stateProviderWorkDirectory);
            if (directoryExists == false)
            {
                // Note that preferably, since SM creates the folder, SM should be the one deleting it.
                // However, custom state providers (and RCQ) may choose to clean up their state by deleting the folder instead of 
                // one file at a time (which can be more efficient). Hence SM allows the state provider to delete the folder.
                return;
            }

            try
            {
                FabricDirectory.Delete(stateProviderWorkDirectory, recursive: true);
            }
            catch (Exception e)
            {
                // Ignore the exception. This causes folder leak but it is not worth asserting or bringing the process down.
                TraceException(this.traceType, e);
            }
        }

        /// <summary>
        /// Disposes managed resources used by the state manager.
        /// </summary>
        protected virtual void Dispose(bool disposing)
        {
            if (this.disposed)
            {
                return;
            }

            if (disposing)
            {
                // dispose state
                if (this.prepareCheckpointMetadataSnapshot != null)
                {
                    this.prepareCheckpointMetadataSnapshot.Clear();
                }

                if (this.stateProviderMetadataManager != null)
                {
                    this.stateProviderMetadataManager.Dispose();
                }

                if (this.changeRoleLock != null)
                {
                    this.changeRoleLock.Dispose();
                }
            }

            // Free any unmanaged objects here. 
            this.disposed = true;
        }

        /// <summary>
        /// Calculates the serialized size of the Uri
        /// </summary>
        /// <param name="input"></param>
        /// <returns>Serialized size</returns>
        /// <remarks>
        /// BinaryWriter uses 7bit length encoding and UTFEncoding for the letters.
        /// Following implementation balances between CPU and memory allocation.
        /// 3 block 7 bit integer can encode length up to 2097151 (2097152 characters).
        /// Largest Uri can be 65518.
        /// </remarks>
        private static int GetByteCount(Uri input)
        {
            // Max number of bytes required for encoing length of Uri string is 3.
            return 3 + input.OriginalString.Length;
        }

        private static void TraceException(string type, string method, string message, Exception exception)
        {
            StateManagerStructuredEvents.TraceException(type, method, message, exception);
        }

        /// <summary>
        /// Aborts state.
        /// </summary>
        private async Task AbortAsync()
        {
            FabricEvents.Events.ApiAbort(this.traceType, "started");
            this.IsClosing = true;

            try
            {
                await this.LoggingReplicator.PrepareToCloseAsync().ConfigureAwait(false);
                FabricEvents.Events.Abort_TStateManager(this.traceType, "Queisce completed");

                this.AbortStateProviders();
            }
            catch (Exception e)
            {
                this.TraceException("AbortAsync", e);
                throw;
            }
            finally
            {
                this.LoggingReplicator.Dispose();

                // dispose state manager
                this.Dispose();
            }

            FabricEvents.Events.ApiAbort(this.traceType, "completed");
        }

        /// <summary>
        /// Aborts all registers state providers.
        /// </summary>
        private void AbortStateProviders()
        {
            // Stage 1: Abort active state provider.
            apiDispatcher.Abort(this.stateProviderMetadataManager.GetMetadataCollection());

            // Stage 2: Abort deleted state providers.
            apiDispatcher.Abort(this.stateProviderMetadataManager.DeletedStateProviders.Values);
        }

        /// <summary>
        /// Aborts state providers.
        /// </summary>
        private void AbortStateProviders(ConcurrentBag<IStateProvider2> stateProviders)
        {
            foreach (var stateProvider in stateProviders)
            {
                stateProvider.Abort();
            }
        }

        private void AddChildrenToList(Metadata metadata, IList<Metadata> children)
        {
            children.Add(metadata);

            IList<Metadata> myChildren = null;
            var removeSuccess =
                this.stateProviderMetadataManager.ParentToChildernStateProviderMap.TryRemove(
                    metadata.StateProviderId,
                    out myChildren);

            if (removeSuccess == false)
            {
                return;
            }

            foreach (var child in myChildren)
            {
                this.AddChildrenToList(child, children);
            }
        }

        /// <summary>
        /// Registers state provider with the state manager.
        /// </summary>
        /// <param name="transaction">transaction that this operation is a part of</param>
        /// <param name="stateProviderName">name of the state provider that needs to be registered</param>
        /// <param name="stateProvider">state provider that needs to be registered</param>
        /// <param name="parentId"></param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out</param>
        /// <param name="stateProviderId"></param>
        /// <param name="shouldAcquireLock">Specifies if lock should be acquired or not</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        private async Task AddSingleStateProviderAsync(
            ReplicatorTransaction transaction,
            Uri stateProviderName,
            IStateProvider2 stateProvider,
            long stateProviderId,
            bool shouldAcquireLock,
            long parentId,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            if (transaction == null)
            {
                throw new ArgumentNullException(SR.Error_Transaction);
            }

            if (stateProvider == null)
            {
                throw new ArgumentNullException(SR.Error_Transaction);
            }

            FabricEvents.Events.AddSingleStateProviderBegin(this.traceType, stateProviderName.ToString());

            var stateProviderType = stateProvider.GetType();

            if (this.StateProviderFactory == null)
            {
                var constructor = stateProviderType.GetConstructor(Type.EmptyTypes);

                if (constructor == null)
                {
                    throw new ArgumentException(
                        SR.Error_SP_ParameterlessConstructor,
                        "stateProviderType");
                }
            }

            if (stateProviderName == null)
            {
                throw new ArgumentException(SR.Error_SP_Name_Required);
            }

            this.ThrowIfNotWritable();

            if (stateProviderName == StateManagerName)
            {
                throw new ArgumentException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        SR.Error_SP_Name_Reserved,
                        stateProviderName.OriginalString));
            }

            if (shouldAcquireLock)
            {
                // Acquire lock.
                var lockContext =
                    await
                        this.stateProviderMetadataManager.LockForWriteAsync(
                            stateProviderName,
                            stateProviderId,
                            transaction,
                            timeout,
                            cancellationToken).ConfigureAwait(false);

                // Register for unlock call.
                transaction.AddLockContext(
                    new StateManagerTransactionContext(transaction.Id, lockContext, OperationType.Add));
            }

            var replicationMetadata = new ReplicationMetadata(
                stateProviderName,
                stateProviderType,
                stateProvider.InitializationContext,
                stateProviderId,
                parentId,
                this.Version,
                StateManagerApplyType.Insert);
            var metadata = new Metadata(
                stateProviderName,
                stateProviderType,
                stateProvider,
                stateProvider.InitializationContext,
                stateProviderId,
                parentId,
                MetadataMode.Active,
                true);

            if (this.stateProviderMetadataManager.Add(stateProviderName, metadata))
            {
                FabricEvents.Events.AddSingleStateProvider(
                    this.traceType,
                    metadata.StateProviderId,
                    transaction.Id);
            }
            else
            {
                // Key already exists. Wrap inner exception.
                FabricEvents.Events.AddSingleStateProviderKeyExists(
                    this.traceType,
                    metadata.StateProviderId,
                    transaction.Id);
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, SR.Error_SP_Already_Exists, metadata.Name));
            }

            await this.ReplicateAsync(transaction, replicationMetadata, timeout, cancellationToken).ConfigureAwait(false);

            FabricEvents.Events.AddSingleStateProviderEnd(this.traceType, metadata.StateProviderId);
        }

        private async Task BackupCheckpointAsync(string backupDirectory)
        {
            var backupCheckpointFileName = Path.Combine(backupDirectory, "backup.chkpt");

            // Delete, if it already exists.
            if (FabricFile.Exists(backupCheckpointFileName))
            {
                FabricFile.Delete(backupCheckpointFileName);
            }

            var serializableMetadataCollection = new List<SerializableMetadata>();

            // CopyOrCheckpointMetadataSnapshot gets updated only during perform checkpoint and replicator guarntees that perform 
            // and backup cannot happen at the same time, so it is safe to use it here.
            foreach (var kvp in this.stateProviderMetadataManager.CopyOrCheckpointMetadataSnapshot)
            {
                var metadata = kvp.Value;
                Utility.Assert(
                    metadata != null,
                    "{0}: BackupCheckpointAsync: metadata != null. BackupDirectory: {1}",
                    this.traceType,
                    backupDirectory);

                serializableMetadataCollection.Add(
                    new SerializableMetadata(
                        metadata.Name,
                        metadata.Type,
                        metadata.InitializationContext,
                        metadata.StateProviderId,
                        metadata.ParentStateProviderId,
                        metadata.MetadataMode,
                        metadata.CreateLsn,
                        metadata.DeleteLsn));
            }

            // Note: For the checkpoint file write path, we directly pass prepareCheckpointLSN into the 
            // CreateAsync and write to the file.
            StateManagerFile stateManagerFile = await StateManagerFile.CreateAsync(
                backupCheckpointFileName,
                this.PartitionId,
                this.ReplicaId,
                serializableMetadataCollection,
                allowPrepareCheckpointLSNToBeInvalid: true,
                prepareCheckpointLSN: this.prepareCheckpointLSN,
                cancellationToken: CancellationToken.None).ConfigureAwait(false);
        }

        /// <summary>
        /// Change role is called on all registers state providers.
        /// </summary>
        private async Task ChangeRoleOnStateProvidersAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            try
            {
                FabricEvents.Events.ChangeRoleOnStateProvidersAcquireLock(
                    this.traceType,
                    newRole.ToString());

                // do not acquire state provider locks here as it can deadlock with a create state provider transaction when the change role lock is acquired
                // as a part of state provider initialize.
                await this.AcquireChangeRoleLockAsync().ConfigureAwait(false);

                FabricEvents.Events.ChangeRoleOnStateProviders(this.traceType, newRole.ToString());

                var taskList = new List<Task>();

                foreach (var metadata in this.stateProviderMetadataManager.GetMetadataCollection())
                {
                    taskList.Add(metadata.StateProvider.ChangeRoleAsync(newRole, cancellationToken));
                }

                await Task.WhenAll(taskList).ConfigureAwait(false);
                this.currentRoleOfStateProviders = this.Role;
            }
            catch (Exception e)
            {
                this.TraceException("ChangeRoleOnStateProvidersAsync", e);
                throw;
            }
            finally
            {
                this.changeRoleLock.Release();
            }
        }

        private async Task CleanUpIncompleteRestoreAsync(CancellationToken cancellationToken)
        {
            // Recover StateManager's checkpoint. 
            // This also creates and initializes the state providers.
            await this.RecoverStateManagerCheckpointAsync(cancellationToken).ConfigureAwait(false);

            // Now that we have recovered all state providers, we need to remote their state and close them.
            await this.OpenStateProvidersAsync().ConfigureAwait(false);

            await this.ChangeRoleOnStateProvidersAsync(ReplicaRole.None, cancellationToken).ConfigureAwait(false);
            await this.RemoveStateOnStateProvidersAsync().ConfigureAwait(false);

            await this.CloseStateProvidersAsync().ConfigureAwait(false);

            // Now that state provider's have been cleaned up, we need to clean up the State Manager's state.
            this.RemoveLocalState();

            this.stateProviderMetadataManager.Dispose();
            this.stateProviderMetadataManager = new StateProviderMetadataManager(this.LoggingReplicator, this.traceType);
        }

        /// <summary>
        /// Calls complete checkpoint on all  state providers.
        /// </summary>
        private async Task CompleteCheckpointOnStateProvidersAsync()
        {
            var taskList = new List<Task>();

            foreach (var metadata in this.stateProviderMetadataManager.CopyOrCheckpointMetadataSnapshot.Values)
            {
                Utility.Assert(
                    metadata != null,
                    "{0}: CompleteCheckpointOnStateProvidersAsync: metadata != null",
                    this.traceType);

                // since the state in the snap has become stable, it is safe to not call checkpoint on the deleted state providers.
                if (metadata.MetadataMode == MetadataMode.Active)
                {
                    taskList.Add(metadata.StateProvider.CompleteCheckpointAsync(CancellationToken.None));
                }
            }

            try
            {
                await Task.WhenAll(taskList).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                this.TraceException("CompleteCheckpointOnStateProvidersAsync", e);
                throw;
            }
        }

        /// <summary>
        /// Constructs the path for the new checkpoint file.
        /// </summary>
        private string GetCheckpointFilePath(string name, Uri serviceInstanceName)
        {
            var sb = new StringBuilder();
            sb.Append(this.PartitionId.ToString("N"));
            sb.Append("_");
            sb.Append(this.ReplicaId);
            sb.Append("_");
            sb.Append(CRC64.ToCRC64String(Encoding.Unicode.GetBytes(serviceInstanceName.OriginalString)));
            sb.Append("_");
            sb.Append(CRC64.ToCRC64String(Encoding.Unicode.GetBytes(name.ToString())));
            return Path.Combine(this.workDirectory, sb.ToString());
        }

        /// <summary>
        /// Move V2 (native generated) state manager checkpoint files if they exist.
        /// This must happen before recovering state manager checkpoint files.
        /// For existing customers' upgrade, native code will move existing V1 state manager
        /// checkpoint files to use native name path which managed code doesn't recognize.
        /// If upgrading from managed code to native fails and it rollbacks to use V1 managed 
        /// code, managed code would read from the wrong place, hence V1 recovery fails. 
        /// Function MoveNativeStateManagerCheckpointFilesIfNecessary will put native-generated 
        /// state manager checkpoint files back to the right place where V2 reads from.
        /// Version 1: Native work directory structure
        /// workDirectory/
        ///  |__PartitionID_ReplicaID/
        ///       |__StateManager.cpt (managed doesn't recognize this file name after rollback)
        ///       |__StateManager.tmp (managed doesn't recognize this file name after rollback)
        ///       |__StateManager.bkp (managed doesn't recognize this file name after rollback)
        ///       |__StateProviderID1/
        ///       |    |__State provider files
        ///       |__StateProviderID2/
        ///       |    |__State provider files
        ///       |  ...
        ///       |__StateProviderID2/
        ///            |__State provider files
        /// Version 2: Managed work directory structure:
        /// workDirectory/
        ///  |__PartitionID_ReplicaID_CRC(serviceName)_CRC("StateManager")
        ///  |__PartitionID_ReplicaID_CRC(serviceName)_CRC("StateManager_tmp")
        ///  |__PartitionID_ReplicaID_CRC(serviceName)_CRC("StateManager_bkp")
        ///  |__PartitionID_ReplicaID_StateProviderID1/
        ///  |    |__State provider files
        ///  |__PartitionID_ReplicaID_StateProviderID2/
        ///  |    |__State provider files
        ///  |  ...
        ///  |__PartitionID_ReplicaID_StateProviderIDn/
        ///       |__State provider files
        /// Algorithm:
        ///     1. Search native-generated state provider folder PartitionID_ReplicaID/. If that exists,
        ///     we don't need to do folder restructure.
        ///     2. Move native state manager checkpoint files from folder PartitionID_ReplicaID/ to
        ///     workDirectory/ if they exist.
        /// </summary>
        private void MoveNativeStateManagerCheckpointFilesIfNecessary()
        {
            string pIdRIdFolder = Path.Combine(
                this.workDirectory,
                this.PartitionId.ToString("N") + "_" + this.ReplicaId);
            if (FabricDirectory.Exists(pIdRIdFolder) == false)
            {
                return;
            }

            // Move state manager checkpoint files
            // The following order of file moves can make sure it is
            // impossible that only backup checkpoint file exists.
            string nativeTempCheckpointFileName = Path.Combine(
                pIdRIdFolder,
                NativeTempCheckpointPrefix);
            if (FabricFile.Exists(nativeTempCheckpointFileName))
            {
                FabricFile.Move(
                    nativeTempCheckpointFileName,
                    this.TemporaryCheckpointFileName);
            }

            string nativeCurrentCheckpointFileName = Path.Combine(
                pIdRIdFolder,
                NativeCurrentCheckpointPrefix);
            if (FabricFile.Exists(nativeCurrentCheckpointFileName))
            {
                FabricFile.Move(
                    nativeCurrentCheckpointFileName,
                    this.CheckpointFileName);
            }
            
            string nativeBackupCheckpointFileName = Path.Combine(
                pIdRIdFolder,
                NativeBackupCheckpointPrefix);
            if (FabricFile.Exists(nativeBackupCheckpointFileName))
            {
                FabricFile.Move(
                    nativeBackupCheckpointFileName,
                    this.BackupCheckpointFileName);
            }
        }

        /// <summary>
        /// Move V2 (native generated) state providers' files if they exist.
        /// This must happen after recovering state manager checkpoint files (so we get all
        /// metadata) and before initializing and recovering state providers (so we can recover
        /// state providers).
        /// For existing customers' upgrade, native code will move existing V1 state providers'
        /// files to use native name path which managed code doesn't recognize.
        /// If upgrading from managed code to native fails and it rollbacks to use V1 managed 
        /// code, managed code would read from the wrong place, hence V1 recovery fails. 
        /// Function MoveNativeStateProvidersFoldersIfNecessary will put native-generated 
        /// state providers' files back to the right place where V2 reads from.
        /// Version 1: Native work directory structure
        /// workDirectory/
        ///  |__PartitionID_ReplicaID/
        ///       |__StateManager.cpt (managed doesn't recognize this file name after rollback)
        ///       |__StateManager.tmp (managed doesn't recognize this file name after rollback)
        ///       |__StateManager.bkp (managed doesn't recognize this file name after rollback)
        ///       |__StateProviderID1/
        ///       |    |__State provider files
        ///       |__StateProviderID2/
        ///       |    |__State provider files
        ///       |  ...
        ///       |__StateProviderID2/
        ///            |__State provider files
        /// Version 2: Managed work directory structure:
        /// workDirectory/
        ///  |__PartitionID_ReplicaID_CRC(serviceName)_CRC("StateManager")
        ///  |__PartitionID_ReplicaID_CRC(serviceName)_CRC("StateManager_tmp")
        ///  |__PartitionID_ReplicaID_CRC(serviceName)_CRC("StateManager_bkp")
        ///  |__PartitionID_ReplicaID_StateProviderID1/
        ///  |    |__State provider files
        ///  |__PartitionID_ReplicaID_StateProviderID2/
        ///  |    |__State provider files
        ///  |  ...
        ///  |__PartitionID_ReplicaID_StateProviderIDn/
        ///       |__State provider files
        /// Algorithm:
        ///     1. Search native-generated state provider folder PartitionID_ReplicaID/. If that exists,
        ///     we don't need to do folder restructure.
        ///     2. Move native state providers' files from folder PartitionID_ReplicaID/ to
        ///     workDirectory/ if they exist.
        /// </summary>
        private void MoveNativeStateProvidersFoldersIfNecessary(
            long stateProviderId,
            string stateProviderWorkDirectory)
        {
            string pIdRIdFolder = Path.Combine(
                this.workDirectory,
                this.PartitionId.ToString("N") + "_" + this.ReplicaId);
            if (FabricDirectory.Exists(pIdRIdFolder) == false)
            {
                return;
            }

            string nativeStateProviderFolder = Path.Combine(
                this.workDirectory,
                this.PartitionId.ToString("N") + "_" + this.ReplicaId,
                stateProviderId.ToString());
            if (FabricDirectory.Exists(nativeStateProviderFolder))
            {
                FabricDirectory.Rename(
                    nativeStateProviderFolder,
                    stateProviderWorkDirectory,
                    true);
            }
        }

        /// <summary>
        /// Creates state provider based on type and name.
        /// </summary>
        private IStateProvider2 CreateStateProvider(Uri name, Type type)
        {
            // Allow the service to override creation of the state provider
            if (this.StateProviderFactory != null)
            {
                return this.StateProviderFactory(name, type);
            }
            else
            {
                if (type == null)
                {
                    var errorMessage =
                        string.Format(
                            "Service Fabric failed to find the Type. This can be caused if the Type is not present in all the replicas. See this link for more details:  {0}",
                            "https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-application-upgrade-data-serialization");
                    FabricEvents.Events.DiagnosticError(this.traceType, errorMessage);
                    Utility.Assert(
                        false,
                        "{0}: CreateStateProvider: type is null for state provider name {1}. " + errorMessage,
                        this.traceType,
                        name.OriginalString);
                }

                return (IStateProvider2) Activator.CreateInstance(type);
            }
        }

        /// <summary>
        /// Creates and Gets work directory for state providers.
        /// </summary>
        private string GetStateProviderWorkDirectory(long stateProviderId)
        {
            var sb = new StringBuilder();
            sb.Append(this.PartitionId.ToString("N"));
            sb.Append("_");
            sb.Append(this.ReplicaId);
            sb.Append("_");
            sb.Append(stateProviderId);
            var stateProviderWorkDirectory = Path.Combine(this.workDirectory, sb.ToString());

            return stateProviderWorkDirectory;
        }

        private IReadOnlyList<Metadata> InitializeStateProvidersInOrder(Metadata metadata)
        {
            IList<Metadata> myChildrenWithMetadata = null;
            var isParentStateProvider =
                this.stateProviderMetadataManager.ParentToChildernStateProviderMap.TryGetValue(
                    metadata.StateProviderId,
                    out myChildrenWithMetadata);

            if (isParentStateProvider == false)
            {
                metadata.StateProvider.Initialize(
                    this.transactionalReplicator,
                    metadata.Name,
                    metadata.InitializationContext,
                    metadata.StateProviderId,
                    this.CreateAndGetStateProviderWorkDirectory(metadata.StateProviderId),
                    null);

                // We can not remove it from parentToChildren because it not opened yet.
            }
            else
            {
                var children = myChildrenWithMetadata.Select(o => o.StateProvider);

                metadata.StateProvider.Initialize(
                    this.transactionalReplicator,
                    metadata.Name,
                    metadata.InitializationContext,
                    metadata.StateProviderId,
                    this.CreateAndGetStateProviderWorkDirectory(metadata.StateProviderId),
                    children);
            }

            // If this is a child, add it to the parent's childrent
            if (metadata.ParentStateProviderId != EmptyStateProviderId)
            {
                var parentsChildren =
                    this.stateProviderMetadataManager.ParentToChildernStateProviderMap.GetOrAdd(
                        metadata.ParentStateProviderId,
                        new List<Metadata>());
                parentsChildren.Add(metadata);

                return EmptyList;
            }

            // If parent return all state Providers.
            var output = new List<Metadata>();

            this.AddChildrenToList(metadata, output);

            return output.AsReadOnly();
        }

        /// <summary>
        /// Checks if a replication exception indicates that operation can be retried.
        /// </summary>
        /// <param name="e">Exception to be to be checked if it can be retried or not.</param>
        /// <returns></returns>
        private bool IsRetryableReplicationException(Exception e)
        {
            return e is FabricTransientException;
        }

        private void NotifyStateManagerStateChanged(
            ITransaction transaction,
            IReliableState reliableState,
            long parentStateProviderId,
            NotifyStateManagerChangedAction action)
        {
            var eventHandler = this.StateManagerChanged;

            if (eventHandler == null || parentStateProviderId != EmptyStateProviderId)
            {
                return;
            }

            // Don't give change notifications for system state providers
            if (reliableState.Name.AbsolutePath.StartsWith(this.SystemStateProviderNamePrefix.AbsolutePath))
            {
                return;
            }

            try
            {
                var newEvent = new NotifyStateManagerSingleEntityChangedEventArgs(transaction, reliableState, action);

                eventHandler(this, newEvent);
            }
            catch (Exception e)
            {
                this.TraceException("Single Entity Notification", e);
                throw;
            }
        }

        private void NotifyStateManagerStateChanged(IAsyncEnumerable<IReliableState> stateProviders)
        {
            var eventHandler = this.StateManagerChanged;

            if (eventHandler == null)
            {
                return;
            }

            try
            {
                eventHandler(this, new NotifyStateManagerRebuildEventArgs(stateProviders));
            }
            catch (Exception e)
            {
                this.TraceException("Rebuild Notification", e);
                throw;
            }
        }

        private async Task RecoverStateManagerCheckpointAsync(CancellationToken cancellationToken)
        {
            if (!FabricFile.Exists(this.CheckpointFileName))
            {
                // Note: For the checkpoint file has PrepareCheckpointLSN, the value must be
                // larger than or equal to 0. For the file does not have the property, the value 
                // must be InvalidLSN(-1).
                this.prepareCheckpointLSN = StateManagerConstants.ZeroLSN;

                // Write the first (empty) checkpoint file.
                await this.LocalStateCheckpointAsync().ConfigureAwait(false);
                await this.CompleteCheckpointOnLocalStateAsync().ConfigureAwait(false);
            }
            else
            {
                // Creates and initializes the recovered state providers.
                await this.LocalStateRecoverCheckpointAsync().ConfigureAwait(false);
            }

            this.NotifyStateManagerStateChanged(
                this.stateProviderMetadataManager.GetMetadataCollection()
                    .Where(metadata => metadata.ParentStateProviderId == EmptyStateProviderId)
                    .Select(metadata => metadata.StateProvider)
                    .ToList().ToAsyncEnumerable());
        }

        /// <summary>
        /// Recovers state provider.
        /// </summary>
        private async Task RecoverStateProvidersAsync()
        {
            var taskList = new List<Task>();

            // Recovering only active state providers.
            foreach (var metadata in this.stateProviderMetadataManager.GetMetadataCollection())
            {
                Utility.Assert(
                    metadata != null,
                    "{0}: RecoverStateProvidersAsync: metadata != null",
                    this.traceType);
                FabricEvents.Events.RecoverStateProvider(this.traceType, metadata.StateProviderId);
                taskList.Add(metadata.StateProvider.RecoverCheckpointAsync());
            }

            try
            {
                await Task.WhenAll(taskList).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                this.TraceException("RecoverStateProvidersAsync", e);
                throw;
            }

            FabricEvents.Events.RecoverStateProviders(this.traceType);
        }

        /// <summary>
        /// Unregisters state provider.
        /// </summary>
        /// <param name="transaction">transaction that the unregister operation is a part of</param>
        /// <param name="stateProviderName">state provider that needs to be unregistered</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        /// <devnote>Locks are always acquired in this method unlike AddSingleStateProviderAsync</devnote>
        private async Task RemoveSingleStateProviderAsync(
            ReplicatorTransaction transaction,
            Uri stateProviderName,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            if (transaction == null)
            {
                throw new ArgumentNullException(SR.Error_Transaction);
            }

            if (stateProviderName == null)
            {
                throw new ArgumentNullException(SR.Error_StateProviderName);
            }

            FabricEvents.Events.RemoveSingleStateProviderBegin(this.traceType, stateProviderName.ToString());

            var metadata = this.stateProviderMetadataManager.GetMetadata(stateProviderName);
            if (metadata == null)
            {
                throw new ArgumentException(
                    string.Format(CultureInfo.CurrentCulture, SR.Error_SP_Null, stateProviderName.OriginalString));
            }

            // Acquire lock.
            var lockContext =
                await
                    this.stateProviderMetadataManager.LockForWriteAsync(
                        stateProviderName,
                        metadata.StateProviderId,
                        transaction,
                        timeout,
                        cancellationToken).ConfigureAwait(false);

            // Register for unlock call.
            transaction.AddLockContext(
                new StateManagerTransactionContext(transaction.Id, lockContext, OperationType.Remove));

            // Check if state provider exists after the lock is acquired.
            if (!this.stateProviderMetadataManager.ContainsKey(stateProviderName))
            {
                FabricEvents.Events.RemoveSingleStateProviderKeyNotFound(
                    this.traceType,
                    metadata.StateProviderId,
                    transaction.Id);
                throw new ArgumentException(
                    string.Format(CultureInfo.CurrentCulture, SR.Error_SP_Null, stateProviderName.OriginalString));
            }

            // Validation checks.
            if (metadata.TransientCreate)
            {
                throw new ArgumentException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        SR.Error_SP_CreatedDestroyed_Tx,
                        metadata.Name));
            }

            if (metadata.TransientDelete)
            {
                throw new ArgumentException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        SR.Error_SP_CreatedDestroyed_Tx, 
                        metadata.Name));
            }

            // Mark transient delete
            metadata.TransientDelete = true;
            metadata.TransactionId = transaction.Id;

            await metadata.StateProvider.PrepareForRemoveAsync(transaction, timeout, cancellationToken).ConfigureAwait(false);

            // Replicate
            var replicationMetadata = new ReplicationMetadata(
                stateProviderName,
                metadata.StateProvider.GetType(),
                metadata.InitializationContext,
                metadata.StateProviderId,
                metadata.ParentStateProviderId,
                this.Version,
                StateManagerApplyType.Delete);

            FabricEvents.Events.RemoveSingleStateProviderBeforeReplication(this.traceType);

            await this.ReplicateAsync(transaction, replicationMetadata, timeout, cancellationToken).ConfigureAwait(false);

            FabricEvents.Events.RemoveSingleStateProviderEnd(this.traceType, metadata.StateProviderId);
        }

        /// <summary>
        /// Replicates state manager operation.
        /// </summary>
        private async Task ReplicateAsync(
            ReplicatorTransaction transaction,
            ReplicationMetadata replicationMetaData,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            try
            {
                using (var writer = new InMemoryBinaryWriter(new MemoryStream()))
                {
                    var posBeforeWrite = checked((int) writer.BaseStream.Position);
                    ReplicationMetadata.SerializeMetaData(replicationMetaData, this.traceType, writer);
                    var metaDataBuffer = new ArraySegment<byte>(
                        writer.BaseStream.GetBuffer(),
                        posBeforeWrite,
                        checked((int) writer.BaseStream.Position - posBeforeWrite));

                    posBeforeWrite = checked((int) writer.BaseStream.Position);
                    ReplicationMetadata.SerializeRedoData(replicationMetaData, this.traceType, writer);
                    var redo = new ArraySegment<byte>(
                        writer.BaseStream.GetBuffer(),
                        posBeforeWrite,
                        checked((int) writer.BaseStream.Position - posBeforeWrite));

                    // Replicate.
                    await this.ReplicateOperationAsync(transaction, metaDataBuffer, redo, timeout, cancellationToken).ConfigureAwait(false);
                }
            }
            catch (Exception e)
            {
                // unlock would release the locks so there is no need to release the locks here.
                this.TraceExceptionWarning(
                    "ReplicateAsync",
                    string.Format(CultureInfo.InvariantCulture, "Replicate exception for transaction {0}.", transaction.Id),
                    e);
                throw;
            }
        }

        /// <summary>
        /// Replicates state manager operation.
        /// </summary>
        private async Task ReplicateOperationAsync(
            ReplicatorTransaction transaction,
            ArraySegment<byte> metaData,
            ArraySegment<byte> redo,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            Utility.Assert(
                transaction != null,
                "{0}: ReplicateOperationAsync: transaction cannt be null",
                this.traceType);
            Utility.Assert(
                metaData.Array != null,
                "{0}: ReplicateOperationAsync: metadata bytes cannot be null. TxnID: {1}",
                this.traceType,
                transaction.Id);
            Utility.Assert(
                redo.Array != null,
                "{0}: ReplicateOperationAsync: redo bytes cannot be null. TxnID: {1}",
                this.traceType,
                transaction.Id);

            // Exponential backoff starts at 16ms and stops at 4s.
            var exponentialBackoff = 8;
            var isDone = false;
            var countRetry = 0;
            var metaDataOperationData = new OperationData(metaData);
            var redoOperationData = new OperationData(redo);
            var start = DateTime.UtcNow;

            while (!isDone)
            {
                try
                {
                    transaction.AddOperation(metaDataOperationData, null, redoOperationData, null, StateManagerId);

                    isDone = true;
                }
                catch (Exception e)
                {
                    // Retry replication if possible.
                    if (this.IsRetryableReplicationException(e))
                    {
                        if (exponentialBackoff*2 < ExponentialBackoffMax)
                        {
                            exponentialBackoff *= 2;
                        }
                        else
                        {
                            exponentialBackoff = ExponentialBackoffMax;
                        }

                        isDone = false;
                    }
                    else
                    {
                        // Trace the failure.
                        this.TraceException("ReplicateOperationAsync", e);

                        // Rethrow inner exception.
                        throw;
                    }
                }

                // Postpone the completion of this task in case of backoff.
                if (!isDone)
                {
                    if (timeout != Timeout.InfiniteTimeSpan)
                    {
                        var end = DateTime.UtcNow;
                        var duration = end.Subtract(start);

                        if (duration.TotalMilliseconds > timeout.TotalMilliseconds)
                        {
                            throw new TimeoutException(
                                string.Format(
                                    CultureInfo.CurrentCulture,
                                    SR.ReplicationTimeout_StateManager,
                                    this.traceType,
                                    timeout.TotalMilliseconds));
                        }
                    }

                    // Trace only after a few retries have failed.
                    countRetry++;
                    if (countRetry == 16)
                    {
                        FabricEvents.Events.ReplicateOperationRetry(
                            this.traceType,
                            transaction.Id,
                            exponentialBackoff);
                        countRetry = 0;
                    }

                    // Back off.
                    await Task.Delay(exponentialBackoff, cancellationToken).ConfigureAwait(false);

                    // Make sure writes are allowed.
                    this.ThrowIfNotWritable();

                    // Stop processing if cancellation was requested.
                    cancellationToken.ThrowIfCancellationRequested();
                }
            }
        }

        private async Task RestoreCheckpointAsync(string backupDirectory)
        {
            FabricEvents.Events.StateManagerRestoreCheckpointAsync(
                this.traceType,
                string.Format(CultureInfo.InvariantCulture,
                "start. backup directory: {0}", 
                backupDirectory));

            // Validate the backup.
            var backupCheckpointFileName = Path.Combine(backupDirectory, "backup.chkpt");
            if (!FabricFile.Exists(backupCheckpointFileName))
            {
                throw new InvalidDataException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        SR.Error_InvalidBackup,
                        backupCheckpointFileName));
            }

            try
            {
                // Validate the backup file is not corrupt.
                StateManagerFile backupFile = await StateManagerFile.OpenAsync(
                    backupCheckpointFileName, 
                    this.traceType, 
                    CancellationToken.None).ConfigureAwait(false);

                // Set PrepareCheckpointLSN to the value reading from the file.
                this.prepareCheckpointLSN = backupFile.Properties.PrepareCheckpointLSN;

                // Consistency checks.
                Utility.Assert(
                    !FabricFile.Exists(this.CheckpointFileName),
                    "{0}: RestoreCheckpointAsync: Checkpoint file must not exist. CheckpointFileName: {1} BackupDirectory: {2}",
                    this.traceType,
                    this.CheckpointFileName,
                    backupDirectory);
                Utility.Assert(
                    !FabricFile.Exists(this.TemporaryCheckpointFileName),
                    "{0}: RestoreCheckpointAsync: Temporary checkpoint file must not exist. TemporaryCheckpointFileName: {1} BackupDirectory: {2}",
                    this.traceType,
                    this.TemporaryCheckpointFileName,
                    backupDirectory);

                // First copy the backup checkpoint file to a temporary location, in case we fail during the copy.
                FabricFile.Copy(src: backupCheckpointFileName, des: this.TemporaryCheckpointFileName, overwrite: false);

                // Now that it's safely copied, we can atomically move it to be the current checkpoint.
                FabricFile.Move(src: this.TemporaryCheckpointFileName, des: this.CheckpointFileName);
            }
            catch (Exception e)
            {
                this.TraceException("RestoreCheckpointAsync", e);
                throw;
            }

            FabricEvents.Events.StateManagerRestoreCheckpointAsync(this.traceType, "complete.");
        }

        /// <summary>
        /// Restores state providers from the given backup.
        /// </summary>
        private async Task RestoreStateProvidersAsync(string backupDirectory)
        {
            var taskList = new List<Task>();

            foreach (var metadata in this.stateProviderMetadataManager.GetMetadataCollection())
            {
                Utility.Assert(
                    metadata != null,
                    "{0}: RestoreStateProvidersAsync: metadata != null",
                    this.traceType);
                FabricEvents.Events.RestoreStateProvider(this.traceType, metadata.StateProviderId);

                // Restore the checkpoint file, then recover from that checkpoint file
                var stateProviderDirectory = Path.Combine(backupDirectory, metadata.StateProviderId.ToString());

                taskList.Add(
                    metadata.StateProvider.RestoreCheckpointAsync(stateProviderDirectory, CancellationToken.None));
            }

            try
            {
                await Task.WhenAll(taskList).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                this.TraceException("RestoreStateProvidersAsync", e);
                throw;
            }

            FabricEvents.Events.RestoreStateProviders(this.traceType, backupDirectory);
        }

        private void ThrowIfArgumentIsNull(object arg, string name)
        {
            if (arg == null)
            {
                throw new ArgumentNullException(name);
            }
        }

        /// <summary>
        /// Throws exception if the partition is not readable.
        /// </summary>
        private void ThrowIfNotReadable()
        {
            var status = this.StatefulPartition.ReadStatus;
            Utility.Assert(
                PartitionAccessStatus.Invalid != status,
                "{0}: ThrowIfNotReadable: invalid partition status. status: {1}",
                this.traceType,
                status);

            // If the replica is readable, no need to throw.
            // Note that this takes dependency on RA not making a replica readable
            // during Unknown and Idle.
            if (PartitionAccessStatus.Granted == status)
            {
                return;
            }

            // If Primary, ReadStatus must be adhered to.
            // Not doing so will cause the replica to be readable during dataloss.
            if (this.Role == ReplicaRole.Primary)
            {
                throw new FabricNotReadableException("Primary State Manager is currently not readable.");
            }

            // If role is Active Secondary, we need to ignore ReadStatus since it is never granted on Active Secondary.
            // Instead we will use IsReadable which indicates that logging replicator has ensured the state is locally consistent.
            if (this.Role == ReplicaRole.ActiveSecondary && this.LoggingReplicator.IsReadable)
            {
                return;
            }

            throw new FabricNotReadableException("State Manager is currently not readable.");
        }

        /// <summary>
        /// Throws invalid operation exception if there is an unexpected operation type.
        /// </summary>
        private void ThrowIfNotValid(bool condition, string format, params object[] list)
        {
            if (!condition)
            {
                var message = string.Format(CultureInfo.InvariantCulture, format, list);
                FabricEvents.Events.ThrowIfNotValid(this.traceType, message);
                message = string.Format(CultureInfo.CurrentCulture, format, list);
                throw new InvalidOperationException(message);
            }
        }

        /// <summary>
        /// Throws exception if the state provider is not writable.
        /// </summary>
        private void ThrowIfNotWritable()
        {
            var status = this.StatefulPartition.WriteStatus;
            Utility.Assert(
                PartitionAccessStatus.Invalid != status,
                "{0}: ThrowIfNotWritable: invalid partition status. status: {1}",
                this.traceType,
                status);

            if (PartitionAccessStatus.NotPrimary == status)
            {
                throw new FabricNotPrimaryException();
            }
        }

        private void TraceException(string method, Exception exception)
        {
            TraceException(this.traceType, method, string.Empty, exception);
        }

        private void TraceExceptionWarning(string method, string message, Exception exception)
        {
            StateManagerStructuredEvents.TraceExceptionWarning(this.traceType, method, message, exception);
        }

        private void ThrowIfClosed()
        {
            if(this.IsClosing)
            {
                throw new FabricObjectClosedException();
            }
        }

        private async Task AcquireChangeRoleLockAsync()
        {
            try
            {
                await this.changeRoleLock.WaitAsync().ConfigureAwait(false);
            }
            catch (Exception e)
            {
                this.TraceExceptionWarning("AcquireChangeRoleLockAsync", "Failed to acquire change role lock", e);
                if (this.IsClosing)
                {
                    throw new FabricObjectClosedException();
                }

                throw;
            }
        }
    }
}

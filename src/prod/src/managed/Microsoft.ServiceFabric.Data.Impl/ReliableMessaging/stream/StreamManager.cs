// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    #region using directives

    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.ReliableMessaging;
    using System.Fabric.ReliableMessaging.Session;
    using System.Globalization;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Replicator;
    using StreamConsolidatedStore =
        System.Fabric.Store.TStore
            <StreamStoreConsolidatedKey, StreamStoreConsolidatedBody, StreamStoreConsolidatedKeyComparer,
                StreamStoreConsolidatedKeyComparer, StreamStoreConsolidatedKeyPartitioner>;

    #endregion

    #region enums StreamManagerState, TransactionCompletionMode

    internal enum StreamManagerState
    {
        PrimaryRestoring = 1002,
        PrimaryOperating = 1003,
        NotPrimary = 1004,
        Closed = 1005,
        Unknown = -10000
    }

    internal enum TransactionCompletionMode
    {
        Commit = 2001,
        Abort = 2002
    }

    #endregion

    /// <summary>
    /// The Stream Manager class is the primary entry point to Reliable messaging.
    /// The stream manager(singleton) is registered as a State Provider in State Manager and manages states(streams, sessions) using the 
    /// the power of WinFab to provide the reliability and consistency guarantees for reliable messaging.
    /// </summary>
    public class StreamManager : IMessageStreamManager, IStateProvider2
    {
        #region fields

        /// <summary>
        /// Pointer to the consolidated TStore state provider that contains both message and metadata state
        /// </summary>
        private StreamConsolidatedStore consolidatedStore;

        /// <summary>
        /// Refers to the current service partition
        /// </summary>
        private IStatefulServicePartition partition;

        /// <summary>
        /// Refers to the replica init parameters (Partition ID, Name ex..)
        /// </summary>
        private StatefulServiceContext initializationParameters;

        /// <summary>
        /// Not used.
        /// </summary>
        private long stateProviderId;

        /// <summary>
        /// Sync point used during change role activities
        /// </summary>
        private readonly SingularSyncPoint changeRoleSyncPoint;

        /// <summary>
        /// Refers to the role state of the current replica.
        /// This is initially defaulted to unknown
        /// </summary>
        private ReplicaRole role = ReplicaRole.Unknown;

        /// <summary>
        /// TaskCompletionSource, when replication role callback signals a change to the primary.
        /// </summary>
        private readonly ChangeRoleSynchronizer roleSynchronizer;

        /// <summary>
        /// Did we get or create the singleton stream manager.
        /// The singleton stream manager when created is stored in SM, thereby ensuring a
        /// distributed singleton instance. We use this to detect partner not ready case.
        /// </summary>
        private bool getSingletonStreamManagerCalled;

        #endregion

        #region Properties

        /// <summary>
        /// Stream Manager instance maintains streams for this Partition
        /// </summary>
        public PartitionKey PartitionIdentity { get; private set; }

        /// <summary>
        /// The stream manager is registered as a state provider with this name
        /// </summary>
        public Uri Name { get; private set; }

        /// <summary>
        /// Runtime resources is a collection of stream manager state (in/out bound collections, drivers, sync points)
        /// </summary>
        internal ActiveResources RuntimeResources { get; private set; }

        /// <summary>
        /// An instance of Session Manager to Send and Receive messages
        /// </summary>
        internal IReliableSessionManager SessionManager { get; private set; }

        /// <summary>
        /// An instance of SessionConnectionManager to manage the Sessions and corresponding session drivers
        /// </summary>
        internal SessionConnectionManager SessionConnectionManager { get; private set; }

        /// <summary>
        /// An reference to this instance of replica State Manager
        /// </summary>
        internal TransactionalReplicator TReplicator { get; private set; }

        /// <summary>
        /// The era is empty when the object is constructed.  It changes to a new Guid on ChangeRole to Primary.  
        /// and set to empty when role changes to non-primary.
        /// </summary>
        internal Guid Era { get; private set; }

        /// <summary>
        /// Trace Desc.
        /// </summary>
        internal string TraceType { get; private set; }

        internal string Tracer { get; private set; }

        byte[] IStateProvider2.InitializationContext
        {
            get { return null; }
        }

        #endregion

        #region cstor

        /// <summary>
        /// Initialize and creates the stream manager. 
        /// </summary>
        public StreamManager()
        {
            // Denote a starting role
            this.Era = Guid.Empty;
            // To enforce a singleton object creation
            this.getSingletonStreamManagerCalled = false;
            // An instance of TaskCompletionSource, when replication role callback signals a change to the primary.
            // Manages the role change Async.
            this.roleSynchronizer = new ChangeRoleSynchronizer(this);
            // Assign the needed resources
            this.RuntimeResources = new ActiveResources();
            // Start a sync point to manage role changes.
            this.changeRoleSyncPoint = new SingularSyncPoint("CommunicationStackLifecycle");
            // Stream Manager name.
            this.Name = new Uri(StreamConstants.StreamManagerName);
        }

        #endregion

        #region IStateProvider2

        private class EmptyDataStream : IOperationDataStream
        {
            /// <summary>
            /// Get the next piece of data from the stream
            /// </summary>
            /// <param name="cancellationToken">Cancellation token for the operation</param>
            /// <returns>
            /// A Task.
            /// </returns>
            public Task<OperationData> GetNextAsync(CancellationToken cancellationToken)
            {
                OperationData result = null;
                return Task.FromResult(result);
            }
        }

        /// <summary>
        /// When the role is set to ShutDown(i.e when its no longer a primary) -- we deactivate the stream manager! 
        /// (by unsubscribing from callbacks, disposing runtime resources and aborting sessions ex,) as
        /// We only process streams and related activity only if its a primary.
        /// </summary>
        /// <param name="newRole">New Role</param>
        /// <returns></returns>
        private async Task ShutDown(ReplicaRole newRole)
        {
            var oldRole = this.role;
            this.role = newRole;
            this.Era = Guid.Empty;

            // If current role is not a primary, there is nothing to do.
            if (oldRole != ReplicaRole.Primary)
            {
                //nothing to shut down
                return;
            }

            // Since the role is a primary, change it to not primary.
            this.roleSynchronizer.ChangeToNotPrimary();

            try
            {
                // Unsubscribe from the consolidated store as we no longer need to react to state changes.
                // as the replica is no longer a primary.
                if (this.consolidatedStore != null)
                {
                    this.consolidatedStore.DictionaryChanged -= this.OnConsolidatedStoreChanged;
                }

                // Abort all sessions and close session manager
                await this.ShutdownCommunicationAsync(newRole);

                // drop all in memory state
                this.RuntimeResources.Clear();
            }
            catch (Exception e)
            {
                Tracing.WriteExceptionAsError("StreamManager.ShutDown", e, "{0} Unexpected Failure", this.Tracer);
            }
        }

        /// <summary>
        /// Continuation handler in lieu of await for StartAsPrimaryAsync
        /// </summary>
        /// <param name="task">Task</param>
        private void ChangeRoleToPrimaryAsyncContinuation(Task task)
        {
            // TODO: exception handling
            if (task.Exception != null)
            {
                if (task.Exception.InnerException is FabricNotPrimaryException)
                {
                    Tracing.WriteExceptionAsWarning(
                        "ChangeRoleToPrimaryAsyncContinuation.Failure",
                        task.Exception,
                        "{0} ChangeRoleToPrimaryAsync failure",
                        this.Tracer);
                }
                else
                {
                    Tracing.WriteExceptionAsError(
                        "ChangeRoleToPrimaryAsyncContinuation.Failure",
                        task.Exception,
                        "{0} ChangeRoleToPrimaryAsync failure",
                        this.Tracer);
                }
            }
        }

        /// <summary>
        /// Set the init(activate) stream manager  when role changes to primary.
        /// 
        /// </summary>
        /// <param name="oldRole">The previous role state</param>
        /// <param name="cancellationToken">Cancellation token</param>
        /// <returns></returns>
        private Task ChangeRoleToPrimaryAsync(ReplicaRole oldRole, CancellationToken cancellationToken)
        {
            FabricEvents.Events.ChangeRole_ReliableStream("Start@" + this.TraceType, oldRole.ToString(), ReplicaRole.Primary.ToString(), this.Era.ToString());

            // Init stream manager for new primary
            this.role = ReplicaRole.Primary;

            // Create a new Era to signify new primary role
            this.Era = Guid.NewGuid();

            // Set role sync. to denote primary state
            this.roleSynchronizer.ChangeToPrimary();
            // Reset all sync points
            this.RuntimeResources.ResetSyncPointCollections();

            // Now activate(ref to StartAsPrimaryAsnc desc) stream manager with the corresponding continuation as we do not want to 
            // hold ChangeRoleAsync() completion for extended time.
            var task = this.StartAsPrimaryAsync().ContinueWith(this.StartAsPrimaryAsyncContinuation);

            FabricEvents.Events.ChangeRole_ReliableStream(
                "FinishWithContinuation@" + this.TraceType,
                oldRole.ToString(),
                ReplicaRole.Primary.ToString(),
                this.Era.ToString());

            return task;
        }

        /// <summary>
        /// Subscribe to Change Role Event to manage the role change to primary and to non-primary.
        /// When role changes to primary, stream manager is initialized and setup for operations, when
        /// role is set to non-primary(active seconday, idleSeconday, none, unknown ex) the role(stream manager) shutdown is initiated.
        /// In the shutdown mode, the sessions, endpoints and active runtime resources are released.
        /// </summary>
        /// <param name="newRole">New Role</param>
        /// <param name="cancellationToken">Cancellation Token if any</param>
        /// <returns></returns>
        async Task IStateProvider2.ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            var oldRole = this.role;

            // if primary, initialize and setup the role stream manager to primary status
            switch (newRole)
            {
                case ReplicaRole.Primary:
                    FabricEvents.Events.ChangeRole_ReliableStream(
                        "StartToPrimary@" + this.TraceType,
                        oldRole.ToString(),
                        ReplicaRole.Primary.ToString(),
                        this.Era.ToString());

                    var task = this.ChangeRoleToPrimaryAsync(oldRole, cancellationToken).ContinueWith(this.ChangeRoleToPrimaryAsyncContinuation);

                    FabricEvents.Events.ChangeRole_ReliableStream(
                        "FinishToPrimary@" + this.TraceType,
                        oldRole.ToString(),
                        ReplicaRole.Primary.ToString(),
                        this.Era.ToString());
                    break;
                // Shutdown, role stream manager if not-primary.
                default:
                    FabricEvents.Events.ChangeRole_ReliableStream("StartWithShutdown@" + this.TraceType, oldRole.ToString(), newRole.ToString(), this.Era.ToString());

                    await this.ShutDown(newRole);

                    FabricEvents.Events.ChangeRole_ReliableStream("FinishWithShutdown@" + this.TraceType, oldRole.ToString(), newRole.ToString(), this.Era.ToString());
                    break;
            }
        }

        /// <summary>
        /// Called when replica is being opened. This is a no-op for stream manager.
        /// </summary>
        /// <returns>
        /// Task that represents the asynchronous operation.
        /// </returns>
        Task IStateProvider2.OpenAsync()
        {
            FabricEvents.Events.OpenStreamManager("Start@" + this.TraceType, this.role.ToString(), this.Era.ToString());
            FabricEvents.Events.OpenStreamManager("Finish@" + this.TraceType, this.role.ToString(), this.Era.ToString());

            object result = null;
            return Task.FromResult(result);
        }

        /// <summary>
        /// Called when replica is being closed.
        /// Initiates the shutdown of stream manager.
        /// </summary>
        /// <returns>
        /// Task that represents the asynchronous operation.
        /// </returns>
        async Task IStateProvider2.CloseAsync()
        {
            FabricEvents.Events.CloseStreamManager("Start@" + this.TraceType, this.role.ToString(), this.Era.ToString());

            await this.ShutDown(ReplicaRole.None);

            FabricEvents.Events.CloseStreamManager("Finish@" + this.TraceType, this.role.ToString(), this.Era.ToString());
        }

        /// <summary>
        /// Called when replica is being aborted.
        /// This shuts down stream manager.
        /// </summary>
        void IStateProvider2.Abort()
        {
            FabricEvents.Events.AbortStreamManager("Start@" + this.TraceType, this.role.ToString(), this.Era.ToString());

            var shutdownTask = this.ShutDown(ReplicaRole.None);

            //We wait as as Abort semantics is synchronous in all Fabric patterns
            shutdownTask.Wait();

            FabricEvents.Events.AbortStreamManager("Finish@" + this.TraceType, this.role.ToString(), this.Era.ToString());
        }

        /// <summary>
        /// Initializes state provider.
        /// </summary>
        /// <param name="tReplicator">state manager instance.</param>
        /// <param name="name">name of the state provider.</param>
        /// <param name="initializationContext">initialization parameters</param>
        /// <param name="stateProviderId">id to uniquely identify state providers.</param>
        /// <param name="workDirectory"></param>
        /// <param name="children"></param>
        void IStateProvider2.Initialize(
            TransactionalReplicator tReplicator, Uri name, byte[] initializationContext, long stateProviderId, string workDirectory,
            IEnumerable<IStateProvider2> children)
        {
            if (!UriEqualityComparer.AreEqual(name, new Uri(StreamConstants.StreamManagerName)))
            {
                throw new ArgumentException("StreamManager.Initialize called with incorrect stream manager name");
            }

            // transactional replicator cannot be null.
            tReplicator.ThrowIfNull("tReplicator");

            this.InitializeReplicaParameters(tReplicator);
            this.stateProviderId = stateProviderId;

            // Ignore the return. If it already there we do not need to register it.
            tReplicator.TryAddStateSerializer<StreamStoreConsolidatedKey>(new StreamStoreConsolidatedKeyConverter());
            tReplicator.TryAddStateSerializer<StreamStoreConsolidatedBody>(new StreamStoreConsolidatedBodyConverter());
        }

        /// <summary>
        /// Prepares for removing state provider.
        /// </summary>
        /// <param name="transaction">transaction that this operation is a part of.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task PrepareForRemoveAsync(Transaction transaction, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var tcs = new TaskCompletionSource<object>();
            tcs.SetResult(null);
            return tcs.Task;
        }

        /// <summary>
        /// Prepares for checkpoint by snapping the state to be checkpointed.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task PrepareCheckpointAsync(long checkpointLSN, CancellationToken cancellationToken)
        {
            var tcs = new TaskCompletionSource<object>();
            tcs.SetResult(null);
            return tcs.Task;
        }

        /// <summary>
        /// Checkpoints state to local disk.
        /// </summary>
        /// <param name="performMode">Represents different mode to perform checkpoint.</param>
        /// <param name="cancellationToken">cancellation token.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task PerformCheckpointAsync(PerformCheckpointMode performMode, CancellationToken cancellationToken)
        {
            var tcs = new TaskCompletionSource<object>();
            tcs.SetResult(null);
            return tcs.Task;
        }

        /// <summary>
        /// Completes checkpoint.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task CompleteCheckpointAsync(CancellationToken cancellationToken)
        {
            var tcs = new TaskCompletionSource<object>();
            tcs.SetResult(null);
            return tcs.Task;
        }

        /// <summary>
        /// Called when data loss is encountered. No-Op for stream manager for now.
        /// We will revisit this later when we design for data loss.
        /// </summary>
        /// <returns>
        /// Task that represents the asynchronous operation.
        /// </returns>
        Task IStateProvider2.OnDataLossAsync()
        {
            var tcs = new TaskCompletionSource<object>();
            tcs.SetResult(null);
            return tcs.Task;
        }

        /// <summary>
        /// Called when recovery is completed for the replica.
        /// No-op for stream manager.
        /// </summary>
        /// <returns>
        /// Task that represents the asynchronous operation.
        /// </returns>
        Task IStateProvider2.OnRecoveryCompletedAsync()
        {
            object result = null;
            return Task.FromResult(result);
        }

        /// <summary>
        /// Recovers state from checkpointed state.
        /// No-Op for stream manager.
        /// </summary>
        /// /// 
        /// <returns>
        /// Task that represents the asynchronous operation.
        /// </returns>
        Task IStateProvider2.RecoverCheckpointAsync()
        {
            object result = null;
            return Task.FromResult(result);
        }

        /// <summary>
        /// Backup the existing checkpoint state on local disk (if any) to the given directory.
        /// </summary>
        /// <param name="backupDirectory">The directory where the checkpoint backup is to be stored.</param>
        /// <param name="cancellationToken">Request cancellation of the checkpoint backup.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.BackupCheckpointAsync(string backupDirectory, CancellationToken cancellationToken)
        {
            return Task.FromResult(true);
        }

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
        Task IStateProvider2.RestoreCheckpointAsync(string backupDirectory, CancellationToken cancellationToken)
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// Remove state.
        /// </summary>
        /// <param name="stateProviderId"> Id that uniquely identifies a state provider.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.RemoveStateAsync(long stateProviderId)
        {
            object result = null;
            return Task.FromResult(result);
        }

        /// <summary>
        /// Called at the start of set current state.
        /// </summary>
        void IStateProvider2.BeginSettingCurrentState()
        {
        }

        /// <summary>
        /// Sets the state on secondary.
        /// </summary>
        /// <param name="stateRecordNumber">record number.</param><param name="data">data that needs to be copied to the secondary.</param>
        Task IStateProvider2.SetCurrentStateAsync(long stateRecordNumber, OperationData data)
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// Called at the end of set state.
        /// </summary>
        Task IStateProvider2.EndSettingCurrentStateAsync()
        {
            return Task.FromResult(0);
        }

        /// <summary>
        /// Gets current copy state from primary.
        /// </summary>
        /// <returns>
        /// Operation data stream that contains the current stream.
        /// In this case, since we do not replicate data, we send an empty data stream.
        /// </returns>
        IOperationDataStream IStateProvider2.GetCurrentState()
        {
            return new EmptyDataStream();
        }

        /// <summary>
        /// Applies operation.
        /// </summary>
        /// <param name="lsn">replication sequence number.</param>
        /// <param name="transaction">transaction that this operation is  apart of.</param>
        /// <param name="metadata"></param>
        /// <param name="data">undo or redo bytes pertaining to the operation.</param>
        /// <param name="applyContext">apply context.</param>
        /// <returns>
        /// Operation context that unlock of this operation needs.
        /// </returns>
        Task<object> IStateProvider2.ApplyAsync(long lsn, TransactionBase transaction, OperationData metadata, OperationData data, ApplyContext applyContext)
        {
            throw new NotImplementedException("Stream manager does not support ApplyAsync");
        }

        /// <summary>
        /// Releases all the locks acquired as a part of a transaction.
        /// </summary>
        /// <param name="state">state that represents lock context.</param>
        void IStateProvider2.Unlock(object state)
        {
            throw new NotImplementedException("Stream manager does not support Unlock");
        }

        /// <summary>
        /// Get all the state providers contained within this state provider instance.
        /// </summary>
        /// <returns>Collection of state providers</returns>
        IEnumerable<IStateProvider2> IStateProvider2.GetChildren(Uri name)
        {
            return null;
        }

        #endregion

        #region InitializationHelpers

        /// <summary>
        /// Ensure that the stream manager is in an operating state(== role.IsPrimary and RoleChangeSync to primary is complete)
        /// </summary>
        /// <returns></returns>
        internal async Task<bool> WaitForOperatingStateAsync()
        {
            if (this.TReplicator == null)
            {
                throw new ArgumentException("Incorrectly constructed StreamManager object");
            }

            // Am I primary?
            if (this.NotWritable())
            {
                return false;
            }

            var result = false;

            try
            {
                // The role is primary, but wait for the role(Stream manager state) to completely recovered to primary state.
                await this.roleSynchronizer.WaitForRecovery();
                result = true;
            }
            catch (Exception e)
            {
                e = Diagnostics.ProcessException(
                    string.Format(CultureInfo.InvariantCulture, "{0}@{1}", "StreamManager.WaitForOperatingStateAsync", this.TraceType),
                    e,
                    "{0} roleSynchronizer.WaitForRecovery threw unexpected exception",
                    this.Tracer);

                if (!(e is FabricNotPrimaryException))
                {
                    Tracing.WriteExceptionAsError(
                        "StreamManager.WaitForOperatingStateAsync",
                        e,
                        "{0} roleSynchronizer.WaitForRecovery threw unexpected exception",
                        this.Tracer);
                }
            }

            return result;
        }

        /// <summary>
        /// Set the stream manager replica parameters.
        /// </summary>
        /// <param name="tReplicator"></param>
        private void InitializeReplicaParameters(TransactionalReplicator tReplicator)
        {
            tReplicator.ThrowIfNull("tReplicator");

            this.TReplicator = tReplicator;
            this.partition = this.TReplicator.StatefulPartition;
            this.initializationParameters = this.TReplicator.ServiceContext;
            var serviceName = this.initializationParameters.ServiceName;
            var partitionInfo = this.partition.PartitionInfo;
            var partitionKind = partitionInfo.Kind;

            switch (partitionKind)
            {
                case ServicePartitionKind.Singleton:
                    var singletonInfo = (SingletonPartitionInformation) partitionInfo;
                    this.PartitionIdentity = new PartitionKey(serviceName);
                    break;

                case ServicePartitionKind.Named:
                    var namedInfo = (NamedPartitionInformation) partitionInfo;
                    this.PartitionIdentity = new PartitionKey(serviceName, namedInfo.Name);
                    break;

                case ServicePartitionKind.Int64Range:
                    var rangeInfo = (Int64RangePartitionInformation) partitionInfo;
                    this.PartitionIdentity = new PartitionKey(serviceName, rangeInfo.LowKey, rangeInfo.HighKey);
                    break;
            }

            this.TraceType = string.Format(
                CultureInfo.InvariantCulture,
                "{0}::{1}",
                this.initializationParameters.PartitionId,
                this.initializationParameters.ReplicaId);

            this.Tracer = string.Format(
                CultureInfo.InvariantCulture,
                "{0}. PartitionId: {1}. ReplicaId: {2}",
                this.PartitionIdentity,
                this.initializationParameters.PartitionId,
                this.initializationParameters.ReplicaId);
        }

        /// <summary>
        /// Gets the underlying consolidated distributed state store.
        /// </summary>
        private void GetUnderlyingStores()
        {
            var storeName = new Uri(this.Name + "/ConsolidatedStore");
            var result = this.TReplicator.TryGetStateProvider(storeName);

            Diagnostics.Assert(result.HasValue, "Failed to initialize message and metadata stores");

            this.consolidatedStore =
                result.Value as
                    Store.TStore
                        <StreamStoreConsolidatedKey, StreamStoreConsolidatedBody, StreamStoreConsolidatedKeyComparer, StreamStoreConsolidatedKeyComparer,
                            StreamStoreConsolidatedKeyPartitioner>;
            Diagnostics.Assert(
                this.consolidatedStore != null,
                "{0} StreamStoreFactory.CreateAsync failed to cast registered state provider to consolidated store",
                this.Tracer);
        }

        /// <summary>
        /// Init Reliable Sessions and subscribe to Inbound callback
        /// </summary>
        /// <returns></returns>
        private async Task InitializeCommunicationAsync()
        {
            FabricEvents.Events.InitializeCommunication(
                "Start@" +
                this.TraceType,
                this.Era.ToString());

            try
            {
                await this.changeRoleSyncPoint.EnterAsync();

                switch (this.PartitionIdentity.Kind)
                {
                    case PartitionKind.Singleton:
                        this.SessionManager = new ReliableSessionManager(
                            this.initializationParameters.PartitionId,
                            this.initializationParameters.ReplicaId,
                            this.PartitionIdentity.ServiceInstanceName);
                        break;
                    case PartitionKind.Named:
                        this.SessionManager = new ReliableSessionManager(
                            this.initializationParameters.PartitionId,
                            this.initializationParameters.ReplicaId,
                            this.PartitionIdentity.ServiceInstanceName,
                            this.PartitionIdentity.PartitionName);
                        break;
                    case PartitionKind.Numbered:
                        this.SessionManager = new ReliableSessionManager(
                            this.initializationParameters.PartitionId,
                            this.initializationParameters.ReplicaId,
                            this.PartitionIdentity.ServiceInstanceName,
                            this.PartitionIdentity.PartitionRange);
                        break;
                    default:
                        Diagnostics.Assert(
                            false,
                            "{0} Unknown partition Kind while creating session manager",
                            this.Tracer);
                        break;
                }

                // Create and open Session Connection Manager
                this.SessionConnectionManager = new SessionConnectionManager(this, this.SessionManager, this.Tracer);
                await this.SessionManager.OpenAsync(this.SessionConnectionManager, CancellationToken.None);

                // Reg. for inbound session callback
                // TODO: ReliableSessionManagerAlreadyListening must not happen; Assert?
                var myEndpoint = await this.SessionManager.RegisterInboundSessionCallbackAsync(this.SessionConnectionManager, CancellationToken.None);

                // Reg. the partition (end point and era) to property manager, this is usefull to check if the partner got moved 
                // during partner first contact session connect operation
                await this.SessionConnectionManager.RegisterPartitionAsync(this.PartitionIdentity, myEndpoint, this.Era);
            }
            catch (Exception e)
            {
                // TODO: FabricObjectClosedException may occur if this races with shutdown; catch and ..?
                Tracing.WriteExceptionAsError("InitializeCommunicationAsync", e, "{0} Failed", this.Tracer);
                throw;
            }
            finally
            {
                this.changeRoleSyncPoint.Leave();
            }

            FabricEvents.Events.InitializeCommunication(
                "Finish@" +
                this.TraceType,
                this.Era.ToString());
        }

        /// <summary>
        /// This is called during role shutdown, to revoke the partition end point and close and release all communication sessions.
        /// </summary>
        /// <param name="newRole">New Role</param>
        /// <returns></returns>
        private async Task ShutdownCommunicationAsync(ReplicaRole newRole)
        {
            FabricEvents.Events.ShutdownCommunication("Start@" + this.TraceType, newRole.ToString());
            try
            {
                await this.changeRoleSyncPoint.EnterAsync();

                // Revoke Session End point for this partition.
                if (this.SessionConnectionManager != null)
                {
                    try
                    {
                        await this.SessionConnectionManager.RevokeEnpointAsync(this.PartitionIdentity);
                    }
                    catch (Exception e)
                    {
                        Tracing.WriteExceptionAsWarning(
                            "StreamManager.SessionConnectionManager.RevokeEnpointAsync",
                            e,
                            "{0} Failed with exception",
                            this.Tracer);
                    }

                    this.SessionConnectionManager = null;
                }

                // Close Session Manager.
                if (this.SessionManager != null)
                {
                    try
                    {
                        await this.SessionManager.CloseAsync(CancellationToken.None);
                        this.SessionManager = null;
                    }
                    catch (Exception e)
                    {
                        Tracing.WriteExceptionAsWarning("StreamManager.SessionManager.CloseAsync", e, "{0} Failed with exception", this.Tracer);
                    }
                }
            }
            finally
            {
                this.changeRoleSyncPoint.Leave();
            }

            FabricEvents.Events.ShutdownCommunication("Finish@" + this.TraceType, newRole.ToString());
        }

        /// <summary>
        /// Delete the given inbound stream and related metadata from the consolidated store.
        /// </summary>
        internal async Task DeleteInboundStreamFromConsolidatedStore(
            Transaction txn, Guid streamId, Uri streamName, PartitionKey targetPartition, TimeSpan timeout)
        {
            FabricEvents.Events.DeleteInboundStreamFromConsolidatedStore("Start@" + this.TraceType, streamId.ToString());

            var txnLookupResult = this.consolidatedStore.CreateOrFindTransaction(txn);
            var storeTransaction = txnLookupResult.Value;
            storeTransaction.Isolation = Store.StoreTransactionReadIsolationLevel.ReadCommitted;
            storeTransaction.LockingHints = Store.LockingHints.UpdateLock;

            try
            {
                var startChange = DateTime.UtcNow;
                var remainingTimeout = timeout;

                await this.consolidatedStore.TryRemoveAsync(
                    storeTransaction,
                    new StreamStoreConsolidatedKey(new StreamMetadataKey(streamId, StreamMetadataKind.ReceiveSequenceNumber)),
                    remainingTimeout,
                    CancellationToken.None);

                FabricEvents.Events.DeleteInboundStreamFromConsolidatedStore("ReceiveSequenceNumber@" + this.TraceType, streamId.ToString());

                // Now remove stream metadata info (InboundStable).
                var endChange = DateTime.UtcNow;
                remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, startChange, endChange);

                await this.consolidatedStore.TryRemoveAsync(
                    storeTransaction,
                    new StreamStoreConsolidatedKey(new StreamMetadataKey(streamId, StreamMetadataKind.InboundStableParameters)),
                    remainingTimeout,
                    CancellationToken.None);

                FabricEvents.Events.DeleteInboundStreamFromConsolidatedStore("Stable@" + this.TraceType, streamId.ToString());
            }
            catch (ArgumentException e)
            {
                Tracing.WriteExceptionAsError("DeleteInboundStreamFromConsolidatedStore", e, "{0} failed for StreamId: {1}", this.Tracer, streamId);
                Diagnostics.Assert(false, "{0} Unexpected Argument exception {1} in DeleteInboundStreamFromConsolidatedStore", this.Tracer, e);
            }
            catch (FabricNotPrimaryException)
            {
                FabricEvents.Events.DeleteInboundStreamFromConsolidatedStore("NotPrimary@" + this.TraceType, streamId + "::" + txn);
                throw;
            }
            catch (FabricObjectClosedException)
            {
                FabricEvents.Events.DeleteInboundStreamFromConsolidatedStore("ObjectClosed@" + this.TraceType, streamId + "::" + txn);
                throw;
            }
            catch (Exception e)
            {
                Tracing.WriteExceptionAsError("DeleteInboundStreamFromConsolidatedStore", e, "{0} failed for StreamId: {1}", this.Tracer, streamId);
                Diagnostics.Assert(false, "{0} Unexpected exception {1} in DeleteInboundStreamFromConsolidatedStore", this.Tracer, e);
            }

            FabricEvents.Events.DeleteInboundStreamFromConsolidatedStore("Finish@" + this.TraceType, streamId.ToString());
        }

        /// <summary>
        /// Get outbound stream from consolidated store.
        /// </summary>
        /// <param name="txn">Transaction to be used for this.</param>
        /// <param name="streamName">Check if the stream exists for this Name and Partition Key</param>
        /// <param name="partitionKey">Check if the stream exists for this Name and Partition Key</param>
        /// <returns>True if stream exists in consolidated store, false otherwise</returns>
        internal async Task<Tuple<bool, IMessageStream>> GetOutboundStreamFromConsolidatedStore(Transaction txn, Uri streamName, PartitionKey partitionKey)
        {
            // Args check
            txn.ThrowIfNull("Transaction");
            streamName.ThrowIfNull("StreamName");
            partitionKey.ThrowIfNull("partitionKey");

            FabricEvents.Events.GetOutboundStream("Start@" + this.TraceType, this.Era.ToString(), streamName + ":" + partitionKey);

            var txnLookupResult = this.consolidatedStore.CreateOrFindTransaction(txn);
            var metadataStoreTransaction = txnLookupResult.Value;

            // Store original isolation level and replace once ops is done 
            var tempIsolationLevel = metadataStoreTransaction.Isolation;
            metadataStoreTransaction.Isolation = Store.StoreTransactionReadIsolationLevel.ReadRepeatable;

            try
            {
                // Check if the stream has already been created previously.
                var lookupStream = await this.consolidatedStore.GetAsync(
                    metadataStoreTransaction,
                    new StreamStoreConsolidatedKey(new StreamNameKey(streamName, partitionKey)),
                    Timeout.InfiniteTimeSpan,
                    CancellationToken.None);

                // return as stream does not exist.
                if (lookupStream.HasValue == false)
                {
                    return new Tuple<bool, IMessageStream>(false, null);
                }

                // Stream found.
                var streamId = lookupStream.Value.NameBody.StreamId;

                // Now get relevant stream info.
                var outboundParamsResult = await this.consolidatedStore.GetAsync(
                    metadataStoreTransaction,
                    new StreamStoreConsolidatedKey(
                        new StreamMetadataKey(
                            streamId,
                            StreamMetadataKind.OutboundStableParameters)),
                    Timeout.InfiniteTimeSpan,
                    CancellationToken.None);

                var parameters = new OutboundStreamStableParameters((OutboundStreamStableParameters) outboundParamsResult.Value.MetadataBody);

                // Init a new stream object.
                var result =
                    new MessageStream(
                        StreamKind.Outbound,
                        partitionKey,
                        streamId,
                        streamName,
                        parameters.MessageQuota,
                        this.consolidatedStore,
                        null,
                        this,
                        parameters.CurrentState);

                return new Tuple<bool, IMessageStream>(true, result);
            }
            finally
            {
                // Revert back to the original isolation level
                metadataStoreTransaction.Isolation = tempIsolationLevel;
                FabricEvents.Events.GetOutboundStream("Finish@" + this.TraceType, this.Era.ToString(), streamName + ":" + partitionKey);
            }
        }

        /// <summary>
        /// check if outbound stream exists in consolidated store.
        /// </summary>
        /// <param name="txn">Transaction to be used for this.</param>
        /// <param name="streamId">Stream Id</param>
        /// <returns>True if stream exists in consolidated store, false otherwise</returns>
        internal async Task<bool> DoesOutboundStreamExistsInConsolidatedStore(Transaction txn, Guid streamId)
        {
            // Args check
            txn.ThrowIfNull("Transaction");
            streamId.ThrowIfNull("StreamId");

            FabricEvents.Events.DoesOutboundStreamExistsInConsolidatedStore("Start@" + this.TraceType, this.Era.ToString(), streamId.ToString());

            var txnLookupResult = this.consolidatedStore.CreateOrFindTransaction(txn);
            var metadataStoreTransaction = txnLookupResult.Value;

            // Store original isolation level and replace once ops is done 
            var tempIsolationLevel = metadataStoreTransaction.Isolation;
            metadataStoreTransaction.Isolation = Store.StoreTransactionReadIsolationLevel.ReadRepeatable;

            try
            {
                // Now check if stream exists
                var outboundParamsResult = await this.consolidatedStore.GetAsync(
                    metadataStoreTransaction,
                    new StreamStoreConsolidatedKey(
                        new StreamMetadataKey(
                            streamId,
                            StreamMetadataKind.OutboundStableParameters)),
                    Timeout.InfiniteTimeSpan,
                    CancellationToken.None);

                return outboundParamsResult.HasValue;
            }
            finally
            {
                // Revert back to the original isolation level
                metadataStoreTransaction.Isolation = tempIsolationLevel;
                FabricEvents.Events.DoesOutboundStreamExistsInConsolidatedStore("Finish@" + this.TraceType, this.Era.ToString(), streamId.ToString());
            }
        }

        /// <summary>
        /// Get inbound stream from consolidated store.
        /// </summary>
        /// <param name="txn">Transaction to be used for this.</param>
        /// <param name="streamId">Check if the stream with this streamId exists</param>
        /// <returns>True if stream exists in consolidated store, false otherwise</returns>
        internal async Task<Tuple<bool, IMessageStream>> GetInboundStreamFromConsolidatedStore(Transaction txn, Guid streamId)
        {
            // Args check
            txn.ThrowIfNull("Transaction");
            streamId.ThrowIfNull("StreamId");

            FabricEvents.Events.GetInboundStream("Start@" + this.TraceType, this.Era.ToString(), streamId.ToString());
            var txnLookupResult = this.consolidatedStore.CreateOrFindTransaction(txn);
            var metadataStoreTransaction = txnLookupResult.Value;

            // Store original isolation level and replace once ops is done 
            var tempIsolationLevel = metadataStoreTransaction.Isolation;
            metadataStoreTransaction.Isolation = Store.StoreTransactionReadIsolationLevel.ReadRepeatable;

            try
            {
                // Now get relevant stream info.
                var inboundParamsResult = await this.consolidatedStore.GetAsync(
                    metadataStoreTransaction,
                    new StreamStoreConsolidatedKey(
                        new StreamMetadataKey(
                            streamId,
                            StreamMetadataKind.InboundStableParameters)),
                    Timeout.InfiniteTimeSpan,
                    CancellationToken.None);

                // return as stream does not exist.
                if (inboundParamsResult.HasValue == false)
                {
                    return new Tuple<bool, IMessageStream>(false, null);
                }

                var stableParameters = new InboundStreamStableParameters((InboundStreamStableParameters) inboundParamsResult.Value.MetadataBody);
                var outboundSessionDriver =
                    await
                        OutboundSessionDriver.FindOrCreateDriverAsync(stableParameters.PartnerId, this, this.SessionConnectionManager, Timeout.InfiniteTimeSpan);
                Diagnostics.Assert(
                    outboundSessionDriver != null,
                    "{0} Unexpected failure to find OutboundSessionDriver in StreamManager.GetInboundStreamAsync, StreamID: {1}",
                    this.Tracer,
                    streamId);

                var result = new MessageStream(
                    StreamKind.Inbound,
                    stableParameters.PartnerId,
                    streamId,
                    stableParameters.StreamName,
                    0,
                    this.consolidatedStore,
                    outboundSessionDriver,
                    this,
                    stableParameters.CurrentState,
                    stableParameters.CloseMessageSequenceNumber);

                return new Tuple<bool, IMessageStream>(true, result);
            }
            finally
            {
                // Revert back to the original isolation level
                metadataStoreTransaction.Isolation = tempIsolationLevel;
                FabricEvents.Events.GetInboundStream("Finish@" + this.TraceType, this.Era.ToString(), streamId.ToString());
            }
        }

        #endregion

        #region Stream manager API handlers

        /// <summary>
        /// Creates an enumerable of currently active outbound streams
        /// </summary>
        /// <returns>an enumerable collection of stream objects</returns>
        public async Task<IEnumerable<IMessageStream>> CreateOutboundStreamsEnumerableAsync()
        {
            var isOperating = await this.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state
                throw new FabricNotPrimaryException();
            }

            return this.RuntimeResources.OutboundStreams.Values;
        }

        /// <summary>
        /// Creates an enumerable of currently active inbound streams
        /// </summary>
        /// <returns>an enumerable collection of stream objects</returns>
        public async Task<IEnumerable<IMessageStream>> CreateInboundStreamsEnumerableAsync()
        {
            var isOperating = await this.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state
                throw new FabricNotPrimaryException();
            }

            return this.RuntimeResources.InboundStreams.Values;
        }

        /// <summary>
        /// Get a specific inbound stream given its StreamId
        /// </summary>
        /// <returns>Tuple(true,stream) if the stream exists, or Tuple(false,null) if it does not </returns>
        public async Task<Tuple<bool, IMessageStream>> GetInboundStreamAsync(Guid streamId)
        {
            var isOperating = await this.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state
                throw new FabricNotPrimaryException();
            }

            MessageStream result = null;
            var exists = this.RuntimeResources.InboundStreams.TryGetValue(streamId, out result);
            return new Tuple<bool, IMessageStream>(exists, result);
        }

        /// <summary>
        /// Get a specific inbound stream given  its Stream Name and Partition Key
        /// </summary>
        /// <returns>Tuple(true,stream) if the stream exists, or Tuple(false,null) if it does not </returns>
        public async Task<Tuple<bool, IMessageStream>> GetInboundStreamAsync(Uri streamName, PartitionKey partitionKey)
        {
            var isOperating = await this.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state
                throw new FabricNotPrimaryException();
            }
            MessageStream result = null;
            result =
                this.RuntimeResources.InboundStreams.Values.FirstOrDefault(
                    x => x.PartnerIdentity.Equals(partitionKey) && UriEqualityComparer.AreEqual(x.StreamName, streamName));
            return result != null ? new Tuple<bool, IMessageStream>(true, result) : new Tuple<bool, IMessageStream>(false, result);
        }

        /// <summary>
        /// Get a specific outbound stream given its StreamId
        /// </summary>
        /// <returns>Tuple(true,stream) if the stream exists, or Tuple(false,null) if it does not </returns>
        public async Task<Tuple<bool, IMessageStream>> GetOutboundStreamAsync(Guid streamId)
        {
            var isOperating = await this.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state
                throw new FabricNotPrimaryException();
            }

            MessageStream result = null;
            var exists = this.RuntimeResources.OutboundStreams.TryGetValue(streamId, out result);
            return new Tuple<bool, IMessageStream>(exists, result);
        }

        /// <summary>
        /// Get a specific inbound stream given its StreamId, This also verifies if the stream actually exists in the statefull consolidated store 
        /// by ensuring the stream is fetched under a store transaction with isolation level of read repeatable.
        /// </summary>
        /// <returns>Tuple(true,stream) if the stream exists, or Tuple(false,null) if it does not </returns>
        public async Task<Tuple<bool, IMessageStream>> GetInboundStreamAsync(Transaction txn, Guid streamId)
        {
            var isOperating = await this.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state
                throw new FabricNotPrimaryException();
            }

            return await this.GetInboundStreamFromConsolidatedStore(txn, streamId);
        }

        /// <summary>
        /// Get a specific outbound stream given its Stream Name and Partition Key
        /// </summary>
        /// <returns>Tuple(true,stream) if the stream exists, or Tuple(false,null) if it does not </returns>tes
        /// 
        public async Task<Tuple<bool, IMessageStream>> GetOutboundStreamAsync(Uri streamName, PartitionKey partitionKey)
        {
            var isOperating = await this.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state
                throw new FabricNotPrimaryException();
            }

            MessageStream result = null;
            result =
                this.RuntimeResources.OutboundStreams.Values.FirstOrDefault(
                    x => (x.PartnerIdentity.Equals(partitionKey) && UriEqualityComparer.AreEqual(x.StreamName, streamName)));
            return result != null ? new Tuple<bool, IMessageStream>(true, result) : new Tuple<bool, IMessageStream>(false, result);
        }

        /// <summary>
        /// Get a specific outbound stream given its Stream Name and Partition Key, This also verifies if the stream actually exists in the statefull consolidated store 
        /// by ensuring the stream is fetched under a store transaction with isolation level of repeatable read.
        /// The reason to confirm for stream existence in the statefull store is due to a quirk in the create outbound stream () process, where in the stream
        /// is created in memory during the create stream process and it does not get removed if the application transaction during create stream is aborted. 
        /// </summary>
        /// <returns>Tuple(true,stream) if the stream exists, or Tuple(false,null) if it does not </returns>tes
        /// 
        public async Task<Tuple<bool, IMessageStream>> GetOutboundStreamAsync(Transaction txn, Uri streamName, PartitionKey partitionKey)
        {
            var isOperating = await this.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state
                throw new FabricNotPrimaryException();
            }

            return await this.GetOutboundStreamFromConsolidatedStore(txn, streamName, partitionKey);
        }

        /// <summary>
        /// Get the handle (singleton instance) of stream manager from state manager, or create one.
        /// 
        /// TODO: Why not make this a overloaded constructor, or precreate one that they can use right away.
        /// </summary>
        /// <param name="tReplicator">Replicator</param>
        /// <param name="inboundStreamCallback">Inbound Stream CallBack, null if outbound only</param>
        /// <param name="timeout">Timeout for the operation to complete</param>
        /// <param name="cancellationToken">Cancellation Token</param>
        /// <returns>Stream Manager</returns>
        internal static async Task<IMessageStreamManager> GetSingletonStreamManagerAsync(TransactionalReplicator tReplicator, IInboundStreamCallback inboundStreamCallback, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var traceType = string.Format(
                CultureInfo.InvariantCulture,
                "{0}::{1}",
                tReplicator.ServiceContext.PartitionId,
                tReplicator.ServiceContext.ReplicaId);
            FabricEvents.Events.GetSingletonStreamManager("Start@" + traceType, "");

            tReplicator.ThrowIfNull("tReplicator");
            var notReady = true;

            // Wait for Partition to be Primary ready state.
            while (notReady)
            {
                var currentPartition = tReplicator.StatefulPartition;
                var writeStatus = currentPartition.WriteStatus;
                var readStatus = currentPartition.ReadStatus;

                // Verify for Write and ReadStatus
                if ((writeStatus != PartitionAccessStatus.Granted) && (readStatus != PartitionAccessStatus.Granted))
                {
                    await Task.Delay(StreamConstants.BaseDelayForReplicatorRetry, cancellationToken);
                    cancellationToken.ThrowIfCancellationRequested();
                }
                else
                {
                    notReady = false;
                }
            }

            var streamManagerName = new Uri(StreamConstants.StreamManagerName);
            var notDone = true;
            StreamManager streamManager = null;

            FabricEvents.Events.GetSingletonStreamManager("DoingTryGetStateProvider@" + traceType, "");

            while (notDone)
            {
                var getNotDone = true;
                var result = default(ConditionalValue<IStateProvider2>);

                // Check if already registered in StateManager
                while (getNotDone)
                {
                    try
                    {
                        FabricEvents.Events.GetSingletonStreamManager("StartTryGetStateProvider@" + traceType, "");
                        result = tReplicator.TryGetStateProvider(streamManagerName);
                        FabricEvents.Events.GetSingletonStreamManager("FinishedTryGetStateProvider@" + traceType, "");
                        getNotDone = false;
                    }
                    catch (Exception e)
                    {
                        // Check inner exception.
                        e = Diagnostics.ProcessException(
                            "StreamManager.GetSingletonStreamManagerAsync",
                            e,
                            "{0} Get State Provider Failed",
                            traceType);

                        if (!(e is FabricTransientException))
                        {
                            Tracing.WriteExceptionAsWarning("GetSingletonStreamManager", e, "{0} Failed", traceType);
                            throw;
                        }

                        FabricEvents.Events.GetSingletonStreamManager("TransientException@" + traceType, "TryGetStateProvider");
                    }

                    if (getNotDone)
                    {
                        await Task.Delay(StreamConstants.BaseDelayForReplicatorRetry);
                    }
                }

                // If Stream Manager is found, i.e we have registered this earlier.
                // update inbound callback reference and return
                if (result.HasValue)
                {
                    streamManager = result.Value as StreamManager;

                    if (streamManager == null)
                    {
                        Exception e = new ArgumentException("State provider with standard stream manager name is an object not of type StreamManager");
                        Tracing.WriteExceptionAsError(
                            "GetSingletonStreamManagerAsync",
                            e,
                            "{0} State provider with standard stream manager name is an object not of type StreamManager",
                            traceType);
                        throw e;
                    }

                    FabricEvents.Events.GetSingletonStreamManager("FoundExisting@" + traceType, "");

                    if (null != inboundStreamCallback)
                    {
                        if (null == streamManager.RuntimeResources.InboundCallbacks.DefaultCallback)
                        {
                            streamManager.RuntimeResources.InboundCallbacks.DefaultCallback = inboundStreamCallback;
                        }
                        else if (inboundStreamCallback != streamManager.RuntimeResources.InboundCallbacks.DefaultCallback)
                        {
                            Exception e = new ArgumentException("An unexpected update has been attempted to reset the default InboundStreamCallback.");
                            Tracing.WriteExceptionAsError(
                                "GetSingletonStreamManagerAsync",
                                e,
                                "{0} An unexpected update has been attempted to reset the default InboundStreamCallback.",
                                traceType);
                            throw e;
                        }
                    }

                    notDone = false;
                }
                // If stream manager is not found, we did not register it earlier.
                // Create one and register in StateManager for later use.
                else
                {
                    FabricEvents.Events.GetSingletonStreamManager("CreatingNew@" + traceType, "");
                    streamManager = new StreamManager();
                    streamManager.InitializeReplicaParameters(tReplicator);
                    streamManager.RuntimeResources.InboundCallbacks.DefaultCallback = inboundStreamCallback;

                    try
                    {
                        using (
                            var replicatorTransaction =
                                await CreateReplicatorTransactionAsync(tReplicator, streamManager.TraceType, streamManager.Tracer, timeout))
                        {
                            await streamManager.RegisterAsync(replicatorTransaction, tReplicator, timeout, cancellationToken);
                            await
                                CompleteReplicatorTransactionAsync(
                                    replicatorTransaction,
                                    TransactionCompletionMode.Commit,
                                    streamManager.TraceType,
                                    streamManager.Tracer);
                        }

                        /*
                         * The tReplicator will not commit the replicator transaction until ChangeRole has been called and finished, hence we can return safely at this point
                         */
                        notDone = false;
                        FabricEvents.Events.GetSingletonStreamManager("FinishedCreatingNew@" + traceType, "");
                    }
                    catch (Exception e)
                    {
                        // Check inner exception.
                        e = Diagnostics.ProcessException(
                            "StreamManager.GetSingletonStreamManagerAsync",
                            e,
                            "{0} Register Async Failed",
                            traceType);

                        if (!(e is DuplicateStateProviderException))
                        {
                            throw;
                        }
                    }
                }
            }
            // Return stream manager
            streamManager.getSingletonStreamManagerCalled = true;
            FabricEvents.Events.GetSingletonStreamManager("Finish@" + traceType, streamManager.Era.ToString());
            return streamManager;
        }

        /// <summary>
        /// Register the consolidated TStore, that is used to store(consistent and replicated) messages and metadata. 
        /// </summary>
        /// <param name="replicatorTransaction">Transaction for this operation</param>
        /// <param name="timeout">Timeout for this opertion to complete</param>
        /// <param name="token">Cancellation Token</param>
        /// <returns></returns>
        private async Task RegisterStoresAsync(Transaction replicatorTransaction, TimeSpan timeout, CancellationToken token)
        {
            FabricEvents.Events.RegisterStores("Start@" + this.TraceType, this.Era.ToString());

            // The name of the consolidated store.
            var consolidatedStoreNameString = StreamConstants.StreamManagerName + "/ConsolidatedStore";
            var consolidatedStoreName = new Uri(consolidatedStoreNameString);

            // Create a TStore instance and ref to Consolidated StreamStore data structures, Compareres and Partitioners.
            var store = new Store.TStore
                <StreamStoreConsolidatedKey, StreamStoreConsolidatedBody, StreamStoreConsolidatedKeyComparer, StreamStoreConsolidatedKeyComparer,
                    StreamStoreConsolidatedKeyPartitioner>(
                this.TReplicator,
                consolidatedStoreName,
                Store.StoreBehavior.SingleVersion,
                false);

            var registerStoresNotDone = true;

            // Register TStore to StateManager with retry
            // TODO: Implement Max delay
            while (registerStoresNotDone)
            {
                try
                {
                    await this.TReplicator.AddStateProviderAsync(replicatorTransaction, store.Name, store, timeout, token);
                    registerStoresNotDone = false;
                }
                catch (Exception e)
                {
                    // Check inner exception.
                    e = Diagnostics.ProcessException(
                        "StreamManager.RegisterStoresAsync",
                        e,
                        "{0} Add State Provider Failed",
                        this.TraceType);

                    if (!(e is FabricTransientException))
                    {
                        if (e is ArgumentException)
                        {
                            // NB:  We are dependent on the tReplicator using this exact exception message -- and not throwing any other argument exception in our context
                            Diagnostics.Assert(
                                e.Message == string.Format(CultureInfo.InvariantCulture, "State provider {0} already exists", consolidatedStoreName),
                                "{0} Unexpected ArgumentException from tReplicator.RegisterAsync for stream manager replicated store {1}",
                                this.Tracer,
                                e.Message);

                            Tracing.WriteExceptionAsWarning(
                                "StreamManager.RegisterStateProvider",
                                e,
                                "{0} StreamManagerReplicatedStore duplicate -- probable race",
                                this.Tracer);
                            throw new DuplicateStateProviderException("StreamManagerReplicatedStore");
                        }

                        Tracing.WriteExceptionAsWarning("StreamManager.Register", e, "{0} RegisterStoresAsync failed", this.Tracer);
                        throw;
                    }
                }

                if (registerStoresNotDone)
                {
                    await Task.Delay(StreamConstants.BaseDelayForReplicatorRetry);
                }
            }

            FabricEvents.Events.RegisterStores("Finish@" + this.TraceType, this.Era.ToString());
        }

        /// <summary>
        /// Register Stream Manager to statemanager. The successfull completion of this operation makes
        /// the stream manager recoverable through replication.
        /// </summary>
        /// <param name="transaction">Transaction for this operation</param>
        /// <param name="tReplicator">Instance of tReplicator</param>
        /// <param name="timeout">Timeout for the operation to close.</param>
        /// <param name="cancellationToken">Cancellation Token</param>
        /// <returns></returns>
        private async Task RegisterAsync(Transaction transaction, TransactionalReplicator tReplicator, TimeSpan timeout, CancellationToken cancellationToken)
        {
            FabricEvents.Events.Register("Start@" + this.TraceType, this.Era.ToString());

            // Don't call ThrowIfNotWritable here because we are dependent there on a ChangeRole to Primary having been called 
            // -- which call will be triggered by the Registration itself, so let any failures of Replicator functionality flow out

            this.InitializeReplicaParameters(tReplicator);

            // First register the stores.
            await this.RegisterStoresAsync(transaction, timeout, cancellationToken);

            var registerStreamManagerNotDone = true;
            var initializationContext = new byte[0];

            while (registerStreamManagerNotDone)
            {
                try
                {
                    // Register the stream manager
                    await this.TReplicator.AddStateProviderAsync(
                        transaction,
                        this.Name,
                        this,
                        timeout,
                        cancellationToken);

                    registerStreamManagerNotDone = false;
                }
                catch (Exception e)
                {
                    // Check inner exception.
                    e = Diagnostics.ProcessException(
                        "StreamManager.RegisterAsync",
                        e,
                        "{0} Add State Provider Failed",
                        this.TraceType);

                    if (!(e is FabricTransientException))
                    {
                        if (e is ArgumentException)
                        {
                            // NB:  We are dependent on the tReplicator using this exact exception message -- and not throwing any other argument exception in our context
                            Diagnostics.Assert(
                                e.Message == string.Format(CultureInfo.InvariantCulture, "State provider {0} already exists", this.Name),
                                "{0} Unexpected ArgumentException from tReplicator.RegisterAsync for singleton stream manager {1}",
                                this.Tracer,
                                e.Message);

                            Tracing.WriteExceptionAsWarning(
                                "StreamManager.RegisterStateProvider",
                                e,
                                "{0} StreamManager duplicate -- probable race",
                                this.Tracer);
                            throw new DuplicateStateProviderException("StreamManager");
                        }

                        Tracing.WriteExceptionAsWarning("StreamManager.Register", e, "{0} RegisterStateProviderAsync failed", this.Tracer);
                        throw;
                    }
                }
                // retry if needed.
                if (registerStreamManagerNotDone)
                {
                    await Task.Delay(StreamConstants.BaseDelayForReplicatorRetry);
                }
            }

            FabricEvents.Events.Register("Finish@" + this.TraceType, this.Era.ToString());
        }

        /// <summary>
        /// Handler in lieu of await for StartAsPrimaryAsync
        /// </summary>
        /// <param name="task"></param>
        private void StartAsPrimaryAsyncContinuation(Task task)
        {
            // TODO: exception handling
            if (task.Exception != null)
            {
                if (task.Exception.InnerException is FabricNotPrimaryException)
                {
                    Tracing.WriteExceptionAsWarning("StartAsPrimaryAsyncContinuation.Failure", task.Exception, "{0} StartAsPrimaryAsync failure", this.Tracer);
                }
                else
                {
                    Tracing.WriteExceptionAsError("StartAsPrimaryAsyncContinuation.Failure", task.Exception, "{0} StartAsPrimaryAsync failure", this.Tracer);
                }
            }
        }

        /// <summary>
        /// Essentially we are activating stream manager by
        /// Waiting for role to turn to primary, and
        /// 1. Subscribe to reliable session callbacks.
        /// 2. Get an handle to Consolidated stores
        /// 3. Subscribe to StoreChanges callbacks
        /// 4. Recover Streams
        /// 5. Set role sync to primary (communicate to rest of tasks of ready for operation state)
        /// 6. Reset, Outbound Partner streams 
        /// </summary>
        /// <returns></returns>
        private async Task StartAsPrimaryAsync()
        {
            this.ThrowIfNotWritable();

            FabricEvents.Events.StartAsPrimary("Start@" + this.TraceType, this.Era.ToString());

            try
            {
                // Wait to change to Primary
                await this.WaitForPrimaryRoleAsync();

                try
                {
                    // TODO: what exceptions is this allowed to throw?
                    // Subscribe to reliable sessions
                    await this.InitializeCommunicationAsync();
                }
                catch (Exception e)
                {
                    Tracing.WriteExceptionAsError("InitializeCommunicationAsync.Failure", e, "{0}", this.Tracer);
                    Diagnostics.Assert(false, "{0} StreamManager.InitializeCommunicationAsync failed with unexpected exception: {1}", this.Tracer, e);
                }

                // Get handle to Consolidated Store and Subscribe to TStore callbacks.
                this.GetUnderlyingStores();
                this.consolidatedStore.DictionaryChanged += this.OnConsolidatedStoreChanged;

                // assume this is a recovery or change of role to primary  -- if fresh start as primary this will be a no-op
                await this.RecoverStreamsOnBecomingPrimaryAsync();

                var setSuccess = this.roleSynchronizer.SetSuccess();

                if (setSuccess)
                {
                    // assume this is a recovery or change of role to primary  -- if fresh start as primary this will be a no-op
                    foreach (var pair in this.RuntimeResources.OutboundSessionDrivers)
                    {
                        FabricEvents.Events.SendingResetPartnerStreams(this.TraceType, pair.Key.ToString());

                        pair.Value.Send(new OutboundResetPartnerStreamsWireMessage(this.Era));
                    }
                }
            }
            catch (Exception e)
            {
                e = Diagnostics.ProcessException(
                    "StreamManager.StartAsPrimaryAsync",
                    e,
                    "{0} Failed",
                    this.TraceType);

                Tracing.WriteExceptionAsWarning("StartAsPrimaryAsync.Failure", e, "{0}", this.Tracer);

                if (e is FabricNotPrimaryException || e is FabricNotReadableException || e is FabricObjectClosedException)
                {
                    this.roleSynchronizer.SetFailure(new FabricNotPrimaryException());
                    throw;
                }

                Diagnostics.Assert(false, "{0} StreamManager.StartAsPrimaryAsync failed with unexpected exception: {1}", this.Tracer, e);
            }

            FabricEvents.Events.StartAsPrimary("Finish@" + this.TraceType, this.Era.ToString());
        }

        /// <summary>
        /// Create the stream: singleton target partition; will be collapsed to a single method instead of three, using IPartitionKey
        /// </summary>
        /// <returns> The created stream</returns>
        public Task<IMessageStream> CreateStreamAsync(Transaction streamTransaction, Uri serviceInstanceName, Uri streamName, int sendQuota)
        {
            var key = new PartitionKey(serviceInstanceName);
            return this.CreateStreamAsync(streamTransaction, key, streamName, sendQuota);
        }

        /// <summary>
        /// Create the stream: named target partition
        /// </summary>
        /// <returns> The created stream</returns>
        public Task<IMessageStream> CreateStreamAsync(
            Transaction streamTransaction, Uri serviceInstanceName, string partitionName, Uri streamName, int sendQuota)
        {
            var key = new PartitionKey(serviceInstanceName, partitionName);
            return this.CreateStreamAsync(streamTransaction, key, streamName, sendQuota);
        }

        /// <summary>
        /// Create the stream: numbered target partition
        /// </summary>
        /// <returns> The created stream</returns>
        public Task<IMessageStream> CreateStreamAsync(
            Transaction streamTransaction, Uri serviceInstanceName, long partitionNumber, Uri streamName, int sendQuota)
        {
            var key = new PartitionKey(serviceInstanceName, partitionNumber);
            return this.CreateStreamAsync(streamTransaction, key, streamName, sendQuota);
        }

        /// <summary>
        /// Register the call back for a stream name prefix. 
        /// Inbound stream callbacks are routed to this callback when streams matches the prefix.
        /// In case of conflicts, the longest prefix match is selected first. 
        /// </summary>
        /// <param name="streamNamePrefix">Stream Name Prefix - Absolute Uri</param>
        /// <param name="streamCallback">Callback when stream matches the prefix</param>
        /// <returns>void</returns>
        /// <exception cref="ArgumentException">When stream name prefix is not a valid Absolute Uri, or
        /// when an attempt is made to register a duplicate prefix.</exception>
        public async Task RegisterCallbackByPrefixAsync(Uri streamNamePrefix, IInboundStreamCallback streamCallback)
        {
            streamNamePrefix.ThrowIfNull("Stream Name Prefix");
            streamCallback.ThrowIfNull("StreamCallback");

            var isOperating = await this.WaitForOperatingStateAsync();
            if (!isOperating)
            {
                // if we are not in operating state
                throw new FabricNotPrimaryException();
            }

            // Register callback.
            this.RuntimeResources.InboundCallbacks.AddCallbackByPrefix(streamNamePrefix, streamCallback);
        }

        /// <summary>
        /// Create a durable message stream: target partition encapsulated as PartitionKey object
        /// A durable message stream consists of storage of the following state info
        /// 1. StreamName
        /// 2. StreamMetaData (Quota)
        /// 3. StreamMetaData (Send Seq. Number)
        /// 4. StreamMetaData (Delete (Receive) Seq. Number).
        /// 
        /// and creation of an instance of the inmemory message stream.
        /// </summary>
        /// <returns> The created stream</returns>
        public async Task<IMessageStream> CreateStreamAsync(Transaction txn, PartitionKey targetPartition, Uri streamName, int sendQuota)
        {
            txn.ThrowIfNull("Transaction");
            streamName.ThrowIfNull("Stream Name");

            if (!streamName.IsAbsoluteUri)
            {
                throw new ArgumentException("Stream name must be an absolute Uri", "streamName");
            }

            if (sendQuota <= 0)
            {
                throw new ArgumentOutOfRangeException("sendQuota");
            }

            var isOperating = await this.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state
                throw new FabricNotPrimaryException();
            }

            if (targetPartition.Kind == PartitionKind.Numbered)
            {
                Diagnostics.Assert(
                    targetPartition.PartitionRange.IntegerKeyLow == targetPartition.PartitionRange.IntegerKeyHigh,
                    "{0} Unexpected non-trivial partition range for target partition {1} in CreateStreamAsync",
                    this.Tracer,
                    targetPartition);
            }

            var streamId = Guid.NewGuid();

            FabricEvents.Events.CreateStream(
                "Start@" + this.TraceType,
                streamName.OriginalString,
                sendQuota.ToString(CultureInfo.InvariantCulture),
                targetPartition.ToString(),
                streamId + "::" + txn);

            var txnLookupResult = this.consolidatedStore.CreateOrFindTransaction(txn);
            var storeTransaction = txnLookupResult.Value;
            storeTransaction.Isolation = Store.StoreTransactionReadIsolationLevel.ReadCommitted;

            try
            {
                // Check if the stream has already been created previously.
                var uniqueNameSetResult =
                    await this.consolidatedStore.GetOrAddAsync(
                        storeTransaction,
                        new StreamStoreConsolidatedKey(new StreamNameKey(streamName, targetPartition)),
                        new StreamStoreConsolidatedBody(new StreamNameBody(streamId)),
                        Timeout.InfiniteTimeSpan,
                        CancellationToken.None);

                var streamIdForName = uniqueNameSetResult.NameBody.StreamId;
                // If a different Guid is returned than the stream was already created previously, In this case throw an exception.
                if (streamIdForName != streamId)
                {
                    FabricEvents.Events.DuplicateStreamName(this.TraceType, streamName.OriginalString, targetPartition.ToString(), streamId.ToString());
                    throw new ArgumentException("duplicate stream name for the same target partition");
                }

                // Now that we added the Stream, add the metadata info (Outbound, Send Seq #, and DeleteSeq.#).
                await this.consolidatedStore.AddAsync(
                    storeTransaction,
                    new StreamStoreConsolidatedKey(new StreamMetadataKey(streamId, StreamMetadataKind.OutboundStableParameters)),
                    new StreamStoreConsolidatedBody(new OutboundStreamStableParameters(streamName, targetPartition, sendQuota)),
                    Timeout.InfiniteTimeSpan,
                    CancellationToken.None);

                await this.consolidatedStore.AddAsync(
                    storeTransaction,
                    new StreamStoreConsolidatedKey(new StreamMetadataKey(streamId, StreamMetadataKind.SendSequenceNumber)),
                    new StreamStoreConsolidatedBody(new SendStreamSequenceNumber(StreamConstants.FirstMessageSequenceNumber)),
                    Timeout.InfiniteTimeSpan,
                    CancellationToken.None);

                await this.consolidatedStore.AddAsync(
                    storeTransaction,
                    new StreamStoreConsolidatedKey(new StreamMetadataKey(streamId, StreamMetadataKind.DeleteSequenceNumber)),
                    new StreamStoreConsolidatedBody(new DeleteStreamSequenceNumber(StreamConstants.FirstMessageSequenceNumber)),
                    Timeout.InfiniteTimeSpan,
                    CancellationToken.None);
            }
            catch (ArgumentException)
            {
                throw;
            }
            catch (FabricNotPrimaryException)
            {
                FabricEvents.Events.CreateStream(
                    "NotPrimary@" + this.TraceType,
                    streamName.OriginalString,
                    sendQuota.ToString(CultureInfo.InvariantCulture),
                    targetPartition.ToString(),
                    streamId + "::" + txn);

                throw;
            }
            catch (FabricObjectClosedException)
            {
                FabricEvents.Events.CreateStream(
                    "ObjectClosed@" + this.TraceType,
                    streamName.OriginalString,
                    sendQuota.ToString(CultureInfo.InvariantCulture),
                    targetPartition.ToString(),
                    streamId + "::" + txn);

                throw;
            }
            catch (Exception e)
            {
                Tracing.WriteExceptionAsError("CreateStreamAsync", e, "{0} failed for Target: {1}", this.Tracer, targetPartition);
                Diagnostics.Assert(false, "{0} Unexpected exception {1} in CreateStreamAsync", this.Tracer, e);
            }


            // Init a new stream object.
            var result =
                new MessageStream(
                    StreamKind.Outbound,
                    targetPartition,
                    streamId,
                    streamName,
                    sendQuota,
                    this.consolidatedStore,
                    null,
                    this,
                    PersistentStreamState.Initialized);

            // Add to the runtime outbound stream collection
            var addSuccess = this.RuntimeResources.OutboundStreams.TryAdd(streamId, result);
            Diagnostics.Assert(addSuccess, "{0} Unexpected failure to add OutboundStream in CreateStreamAsync", this.Tracer);

            FabricEvents.Events.CreateStream(
                "Finish@" + this.TraceType,
                streamName.OriginalString,
                sendQuota.ToString(CultureInfo.InvariantCulture),
                targetPartition.ToString(),
                streamId + "::" + txn);

            return result;
        }

        #endregion

        #region Stream lifecycle handlers

        /// <summary>
        /// Inbound Stream Requested Message request 
        /// Check if stream exists already in the consolidated store if exists, i.e this can only happen if service accepted the inbound create stream request earlier.
        /// if it does not exists, call and await user callback response, if accepted, store to consolidated store 
        /// Respond, acceptance or rejection as a outbound message.
        /// </summary>
        /// <param name="partnerKey">Partner Key</param>
        /// <param name="openMessage">Inbound Stream Request Message</param>
        /// <returns></returns>
        internal async Task InboundStreamRequested(PartitionKey partnerKey, InboundBaseStreamWireMessage openMessage)
        {
            var streamName = (new InboundOpenStreamName(openMessage)).StreamName;
            var streamId = openMessage.StreamIdentity;

            var isOperating = await this.WaitForOperatingStateAsync();
            if (!isOperating)
            {
                // if we are not in operating state drop the message
                return;
            }

            FabricEvents.Events.InboundStreamRequested("Start@" + this.TraceType, streamName.OriginalString, streamId.ToString(), partnerKey.ToString());

            Diagnostics.Assert(
                openMessage.MessageSequenceNumber == 0,
                string.Format(
                    CultureInfo.InvariantCulture,
                    "Unexpected non-standard sequence number: {0} in OpenStream message, StreamID: {1}",
                    openMessage.MessageSequenceNumber,
                    openMessage.StreamIdentity));


            var response = StreamWireProtocolResponse.StreamRejected;

            // TODO: need simple exception handling structure/pattern
            var streamExists = false;
            var accepted = false;

            var syncPoint = new SyncPoint<Guid>(this, streamId, this.RuntimeResources.PauseStreamSyncpoints);

            try
            {
                await syncPoint.EnterAsync(Timeout.InfiniteTimeSpan);

                // Check if stream already exists in consolidated store.
                using (var replicatorTransaction = this.TReplicator.CreateTransaction())
                {
                    var checkTxn = this.consolidatedStore.CreateStoreReadWriteTransaction(replicatorTransaction);
                    var inboundParametersKey = new StreamStoreConsolidatedKey(new StreamMetadataKey(streamId, StreamMetadataKind.InboundStableParameters));

                    var getResult =
                        await this.consolidatedStore.GetAsync(checkTxn, inboundParametersKey, Timeout.InfiniteTimeSpan, CancellationToken.None);

                    streamExists = getResult.HasValue;
                }

                // if it does not exist, process user callback event
                if (!streamExists)
                {
                    var inboundStreamCallback = this.RuntimeResources.InboundCallbacks.GetCallback(streamName);

                    if (inboundStreamCallback == null)
                    {
                        response = this.getSingletonStreamManagerCalled
                            ? StreamWireProtocolResponse.TargetNotAcceptingStreams
                            : StreamWireProtocolResponse.TargetNotReady;
                    }
                    else
                    {
                        FabricEvents.Events.InboundStreamRequested(
                            "AcceptCallback.Start@" + this.TraceType,
                            streamName.OriginalString,
                            streamId.ToString(),
                            partnerKey.ToString());
                        var preCallEra = this.Era;

                        try
                        {
                            accepted = await inboundStreamCallback.InboundStreamRequested(partnerKey, streamName, streamId);
                        }
                        catch (Exception e)
                        {
                            e = Diagnostics.ProcessException(
                                "IInboundStreamCallback.InboundStreamRequested",
                                e,
                                "{0} Failed",
                                this.TraceType);

                            if ((e is FabricNotPrimaryException) || (this.NotWritable()) || (this.Era != preCallEra))
                            {
                                // we assume that the callee ran into a fabric role change and hence the OpenStream message should be dropped; it will be retried
                                return;
                            }
                            else
                            {
                                Tracing.WriteExceptionAsError("IInboundStreamCallback.InboundStreamRequested", e, "{0}", this.Tracer);
                                response = StreamWireProtocolResponse.StreamPartnerFaulted;
                            }
                        }

                        FabricEvents.Events.InboundStreamRequested(
                            "AcceptCallback.Finish@" + this.TraceType,
                            streamName.OriginalString,
                            streamId.ToString(),
                            partnerKey.ToString());
                    }
                }

                // if user callback accepted. Store to durable consolidated store.
                // register Inbound stream and Complete Tx
                if (accepted)
                {
                    // accepted and streamExists are mutually exclusive cases
                    Diagnostics.Assert(!streamExists, "streamExists and accepted cannot both be true in StreamManager.InboundStreamRequested");

                    using (var replicatorTransaction = await this.CreateReplicatorTransactionAsync(Timeout.InfiniteTimeSpan))
                    {
                        var txnLookupResult = this.consolidatedStore.CreateOrFindTransaction(replicatorTransaction);
                        Diagnostics.Assert(
                            !txnLookupResult.HasValue,
                            "{0} found existing store transaction for new replicator transaction; TransactionId: {1}",
                            this.Tracer,
                            replicatorTransaction);
                        var metadataStoreTransaction = txnLookupResult.Value;

                        metadataStoreTransaction.Isolation = Store.StoreTransactionReadIsolationLevel.ReadCommitted;

                        await this.RegisterInboundStream(metadataStoreTransaction, partnerKey, streamName, streamId);

                        await this.CompleteReplicatorTransactionAsync(replicatorTransaction, TransactionCompletionMode.Commit);
                    }

                    response = StreamWireProtocolResponse.StreamAccepted;
                }
            }
            catch (Exception e)
            {
                e = Diagnostics.ProcessException(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "{0}@{1}",
                        "IInboundStreamCallback.InboundStreamRequested",
                        this.TraceType),
                    e,
                    "{0} failed for Target: {1} StreamName: {2} StreamID: {3}",
                    this.Tracer,
                    partnerKey,
                    streamName,
                    streamId);

                Diagnostics.Assert(
                    e is FabricNotPrimaryException || e is FabricObjectClosedException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));


                // we assume that the callee ran into a fabric role change and hence the OpenStream message should be dropped; it will be retried
                return;
            }
            finally
            {
                syncPoint.Dispose();
            }

            if (streamExists)
            {
                // accepted and streamExists are mutually exclusive cases
                Diagnostics.Assert(!accepted, "streamExists and accepted cannot both be true in StreamManager.InboundStreamRequested");

                // this case occurs during recovery when the stream was accepted and registered but the Open response was lost due to some failure
                // In this case we must send the response immediately since no transaction will occur and therefore no post transaction notification to cause completion
                response = StreamWireProtocolResponse.StreamAccepted;
            }

            // Respond Stream accepted wire protocol response.
            if ((response != StreamWireProtocolResponse.StreamAccepted) || streamExists)
            {
                FabricEvents.Events.InboundStreamRequested(
                    response + "@" + this.TraceType,
                    streamName.OriginalString,
                    streamId.ToString(),
                    partnerKey.ToString());
                var outboundSessionDriver =
                    await OutboundSessionDriver.FindOrCreateDriverAsync(partnerKey, this, this.SessionConnectionManager, Timeout.InfiniteTimeSpan);
                var responseWireMessage = new OutboundOpenStreamResponseWireMessage(streamId, response);
                outboundSessionDriver.Send(responseWireMessage);
            }
        }

        /// <summary>
        /// This is called, once store communicates the commit of InBound Stream Acceptance stream registration completion.
        /// This creates the outbound session driver to communicate subsequent messages to the partner, and add the inbound stream to 
        /// stream manager collection. Finally responds with a Stream acceptance wire message.
        /// </summary>
        /// <param name="partnerKey">Partner Key</param>
        /// <param name="streamName">Stream Name</param>
        /// <param name="streamId">Stream Id</param>
        /// <returns></returns>
        internal async Task InboundStreamRequestedCompletionAsync(PartitionKey partnerKey, Uri streamName, Guid streamId)
        {
            var outboundSessionDriver =
                await OutboundSessionDriver.FindOrCreateDriverAsync(partnerKey, this, this.SessionConnectionManager, Timeout.InfiniteTimeSpan);

            var stream =
                new MessageStream(
                    StreamKind.Inbound,
                    partnerKey,
                    streamId,
                    streamName,
                    0,
                    this.consolidatedStore,
                    outboundSessionDriver,
                    this,
                    PersistentStreamState.Open);

            // Init Stream. and add to collection
            var addSuccess = this.RuntimeResources.NextSequenceNumberToDelete.TryAdd(streamId, StreamConstants.FirstMessageSequenceNumber);

            Diagnostics.Assert(
                addSuccess,
                "{0} Unexpected failure to add NextSequenceNumberToDelete in StreamManager.InboundStreamRequestedCompletionAsync",
                this.Tracer);
            addSuccess = this.RuntimeResources.InboundStreams.TryAdd(streamId, stream);
            Diagnostics.Assert(addSuccess, "{0} Unexpected failure to add inbound stream in StreamManager.InboundStreamRequestedCompletionAsync", this.Tracer);

            FabricEvents.Events.InboundStreamRequested(
                "CreatedCallback.Start@" + this.TraceType,
                streamName.OriginalString,
                streamId.ToString(),
                partnerKey.ToString());

            var inboundStreamCallback = this.RuntimeResources.InboundCallbacks.GetCallback(streamName);
            Diagnostics.Assert(
                inboundStreamCallback != null,
                "{0} Unexpected failure to fetch inboundStreamCallback in StreamManager.InboundStreamRequestedCompletionAsync",
                this.Tracer);
            // Raise InboundStream Created Callback.
            await Task.Run(
                () => inboundStreamCallback.InboundStreamCreated(stream)
                    .ContinueWith(
                        antecedent =>
                            this.InboundStreamCreatedContinuation(antecedent, streamName, streamId, partnerKey)));

            var responseWireMessage = new OutboundOpenStreamResponseWireMessage(streamId, StreamWireProtocolResponse.StreamAccepted);
            outboundSessionDriver.Send(responseWireMessage);

            FabricEvents.Events.InboundStreamRequested("Accepted@" + this.TraceType, streamName.OriginalString, streamId.ToString(), partnerKey.ToString());
        }

        /// <summary>
        /// Handler in lieu of await for IInboundStreamCallback.InboundStreamCreated
        /// </summary>
        /// <param name="task"></param>
        private void InboundStreamRequestedCompletionContinuation(Task task)
        {
            // TODO: exception handling
            if (task.Exception != null)
            {
                Tracing.WriteExceptionAsError("InboundStreamRequestedCompletion", task.Exception, "{0}", this.Tracer);
            }
        }

        /// <summary>
        /// Handler in lieu of await for IInboundStreamCallback.InboundStreamCreated
        /// </summary>
        /// <param name="task"></param>
        /// <param name="streamName"></param>
        /// <param name="streamId"></param>
        /// <param name="partnerKey"></param>
        private void InboundStreamCreatedContinuation(Task task, Uri streamName, Guid streamId, PartitionKey partnerKey)
        {
            // TODO: exception handling: should we do ReportFaultTransient in this case?
            if (task.Exception != null)
            {
                Tracing.WriteExceptionAsError("IInboundStreamCallback.InboundStreamCreated", task.Exception, "{0}", this.Tracer);
            }
            else
            {
                FabricEvents.Events.InboundStreamRequested(
                    "CreatedCallback.Finish@" + this.TraceType,
                    streamName.OriginalString,
                    streamId.ToString(),
                    partnerKey.ToString());
            }
        }

        /// <summary>
        /// Handler in lieu of await for IInboundStreamCallback.InboundStreamDeleted
        /// </summary>
        /// <param name="task"></param>
        /// <param name="streamId"></param>
        /// <param name="partnerKey"></param>
        private void InboundStreamDeletedContinuation(Task task, Guid streamId, PartitionKey partnerKey)
        {
            // TODO: exception handling: should we do ReportFaultTransient in this case?
            if (task.Exception != null)
            {
                Tracing.WriteExceptionAsError("IInboundStreamCallback.InboundStreamDeleted", task.Exception, "{0}", this.Tracer);
            }
            else
            {
                FabricEvents.Events.DeleteInboundStreamRequested("DeletedCallback.Finish@" + this.TraceType, streamId.ToString(), partnerKey.ToString());
            }
        }

        /// <summary>
        /// Delete Inbound Stream Requested Message request: 
        /// Check if stream was deleted before, i.e this can only happen if delete inbound stream request was processed earlier.
        /// if the stream is not in a deleted state, callback service, and store this fact to the consolidated store and respond to the source service.
        /// </summary>
        /// <param name="partnerKey">Partner Key</param>
        /// <param name="deleteMessage">Inbound Stream Request Message</param>
        /// <returns></returns>
        internal async Task DeleteInboundStreamRequested(PartitionKey partnerKey, InboundBaseStreamWireMessage deleteMessage)
        {
            var streamId = deleteMessage.StreamIdentity;
            var response = StreamWireProtocolResponse.Unknown;

            var isOperating = await this.WaitForOperatingStateAsync();
            if (!isOperating)
            {
                // if we are not in operating state drop the message
                return;
            }

            FabricEvents.Events.DeleteInboundStreamRequested("Start@" + this.TraceType, streamId.ToString(), partnerKey.ToString());

            MessageStream stream = null;
            var getSuccess = this.RuntimeResources.InboundStreams.TryGetValue(streamId, out stream);

            // This might happen in the rare case when response ack was lost and source retried after
            // lazy deletion of inbound stream already took place.
            if (!getSuccess)
            {
                response = StreamWireProtocolResponse.StreamNotFound;
            }

            // Check if we notified previously. Hint, a successfull notification would have set the state of the stream to deleted.
            if (response == StreamWireProtocolResponse.Unknown)
            {
                // This might happen if the stream delete responce ack was lost and retried.
                if (stream.CurrentState == PersistentStreamState.Deleted)
                {
                    response = StreamWireProtocolResponse.TargetNotified;
                }
            }

            // Now notify callback
            if (response == StreamWireProtocolResponse.Unknown)
            {
                try
                {
                    // Since we are notify only Ack, we will first set the inbound stream state to 'Deleted' before 
                    // service notification
                    // Update store to set state to deleted.
                    await stream.DeleteInboundStream();
                }
                catch (Exception e)
                {
                    e = Diagnostics.ProcessException(
                        "DeleteInboundStreamRequested.InboundStreamDeleted",
                        e,
                        "{0} Failed",
                        this.TraceType);

                    Tracing.WriteExceptionAsError("DeleteInboundStreamRequested.InboundStreamDeleted", e, "{0}", this.Tracer);

                    if ((e is FabricNotPrimaryException) || (this.NotWritable()))
                    {
                        // we assume that the callee ran into a fabric role change and hence the Delete Stream message should be dropped; it will be retried
                        return;
                    }
                }

                try
                {
                    FabricEvents.Events.DeleteInboundStreamRequested("NotifyCallback.Start@" + this.TraceType, streamId.ToString(), partnerKey.ToString());
                    var inboundStreamCallback = this.RuntimeResources.InboundCallbacks.GetCallback(stream.StreamName);

                    // We  need to delete the stream regardless of the service accepting streams or not.
                    if (inboundStreamCallback != null)
                    {
                        await Task.Run(
                            () => inboundStreamCallback.InboundStreamDeleted(stream)
                                .ContinueWith(
                                    antecedent =>
                                        this.InboundStreamDeletedContinuation(antecedent, streamId, partnerKey)));
                    }
                    response = StreamWireProtocolResponse.TargetNotified;

                    FabricEvents.Events.DeleteInboundStreamRequested("NotifyCallback.Finish@" + this.TraceType, streamId.ToString(), partnerKey.ToString());
                }
                catch (Exception)
                {
                    Diagnostics.Assert(
                        false,
                        "{0} unexpected failure during inboundStreamCallback.InboundStreamDeleted for StreamId: {1}",
                        this.Tracer,
                        streamId);

                    // We will go ahead with response as Target Notified regardless as the stream has already been deleted at this point.
                    response = StreamWireProtocolResponse.TargetNotified;
                }
                FabricEvents.Events.DeleteInboundStreamRequested("NotifyCallback.Finish@" + this.TraceType, streamId.ToString(), partnerKey.ToString());
            }

            Diagnostics.Assert(
                response != StreamWireProtocolResponse.Unknown,
                "{0} unexpected failure during DeleteInboundStreamRequested for StreamId: {1}",
                this.Tracer,
                streamId);

            // Now send Delete Protocol Response
            FabricEvents.Events.DeleteInboundStreamRequested(response + "@" + this.TraceType, streamId.ToString(), partnerKey.ToString());
            var outboundSessionDriver =
                await OutboundSessionDriver.FindOrCreateDriverAsync(partnerKey, this, this.SessionConnectionManager, Timeout.InfiniteTimeSpan);
            var responseWireMessage = new OutboundDeleteStreamResponseWireMessage(streamId, response);
            outboundSessionDriver.Send(responseWireMessage);
        }

        /// <summary>
        /// This process(at the source) the open stream response(accepted or rejected) from the target service.
        /// Based on the response, the outbound stream is either opened(and can proceed with send messages ops)
        /// or the stream is set to 'closed' state.
        /// </summary>
        /// <param name="openResponseMessage">Open Message</param>
        /// <returns></returns>
        internal async Task CompleteOpenStreamProtocol(InboundBaseStreamWireMessage openResponseMessage)
        {
            Diagnostics.Assert(
                openResponseMessage.MessageSequenceNumber == StreamConstants.StreamProtocolResponseMessageSequenceNumber,
                "{0} Unexpected non-standard sequence number in StreamProtocolResponseMessage",
                this.Tracer);

            var response = (new InboundOpenStreamResponseValue(openResponseMessage)).Response;

            var isOperating = await this.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state drop the message
                return;
            }

            var streamId = openResponseMessage.StreamIdentity;

            FabricEvents.Events.CompleteOpenStreamProtocol("Start@" + this.TraceType, streamId.ToString(), response.ToString());

            MessageStream stream = null;
            var getSuccess = this.RuntimeResources.OutboundStreams.TryGetValue(streamId, out stream);

            // There is a chance that the Open Stream Ack was received for a stale stream(deleted).
            // In this case, check existence of stream in TStore before asserting.
            if (getSuccess == false)
            {
                // Check if stream already exists in consolidated store.
                using (var replicatorTransaction = await this.CreateReplicatorTransactionAsync(Timeout.InfiniteTimeSpan))
                {
                    // Assert if found -- clearly there is a mismatch in in-memory cache vs the store.
                    getSuccess = await this.DoesOutboundStreamExistsInConsolidatedStore(replicatorTransaction, streamId);
                    Diagnostics.Assert(
                        !getSuccess,
                        "{0} unexpected failure to find outbound stream in CompleteOpenStreamProtocol StreamId: {1}",
                        this.Tracer,
                        streamId);
                }
                Tracing.WriteWarning(
                    "StreamManager.CompleteOpenStreamProtocol.Failure to find outbound stream",
                    "Target: {0} StreamID: {1}",
                    this.Tracer,
                    streamId);
                return;
            }
            // Check and Ack Open Stream Request.
            await stream.CompleteOpenAsync(response);
            FabricEvents.Events.CompleteOpenStreamProtocol("Finish@" + this.TraceType, streamId.ToString(), response.ToString());
        }

        /// <summary>
        /// This process(at the source) the close stream response(Ack only) from the target service.
        /// by setting the state of the stream to 'closed' state.
        /// </summary>
        /// <param name="closeResponseMessage">Close Message</param>
        /// <returns></returns>
        internal async Task CompleteCloseStreamProtocol(InboundBaseStreamWireMessage closeResponseMessage)
        {
            Diagnostics.Assert(
                closeResponseMessage.MessageSequenceNumber == StreamConstants.StreamProtocolResponseMessageSequenceNumber,
                "{0} Unexpected non-standard sequence number in StreamProtocolResponseMessage",
                this.Tracer);

            var isOperating = await this.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state drop the message
                return;
            }

            var response = (new InboundCloseStreamResponseValue(closeResponseMessage)).Response;
            var streamId = closeResponseMessage.StreamIdentity;

            FabricEvents.Events.CompleteCloseStreamProtocol("Start@" + this.TraceType, streamId.ToString(), response.ToString());

            Diagnostics.Assert(
                response == StreamWireProtocolResponse.CloseStreamCompleted,
                "{0} Unexpected StreamWireProtocolResponse in StreamManager.CompleteCloseStreamProtocol",
                this.Tracer);

            MessageStream stream = null;
            var getSuccess = this.RuntimeResources.OutboundStreams.TryGetValue(streamId, out stream);
            // There is a chance that the Close Stream Ack was received for a stale stream(deleted).
            // In this case, check existence of stream in TStore before asserting.
            if (getSuccess == false)
            {
                // Check if stream already exists in consolidated store.
                using (var replicatorTransaction = await this.CreateReplicatorTransactionAsync(Timeout.InfiniteTimeSpan))
                {
                    // Assert if found -- clearly there is a mismatch in in-memory cache vs the store.
                    getSuccess = await this.DoesOutboundStreamExistsInConsolidatedStore(replicatorTransaction, streamId);
                    Diagnostics.Assert(
                        !getSuccess,
                        "{0} unexpected failure to find outbound stream in CompleteCloseStreamProtocol StreamId: {1}",
                        this.Tracer,
                        streamId);
                }
                Tracing.WriteWarning(
                    "StreamManager.CompleteCloseStreamProtocol.Failure to find outbound stream",
                    "Target: {0} StreamID: {1}",
                    this.Tracer,
                    streamId);
                return;
            }
            await stream.CompleteCloseAsync(response);
            FabricEvents.Events.CompleteCloseStreamProtocol("Finish@" + this.TraceType, streamId.ToString(), response.ToString());
        }

        /// <summary>
        /// This process(at the source) the delete stream response(Ack only) from the target service.
        /// by setting the state of the stream to 'deleted' state.
        /// </summary>
        /// <param name="deleteResponseMessage"></param>
        /// <returns></returns>
        internal async Task CompleteDeleteStreamProtocol(InboundBaseStreamWireMessage deleteResponseMessage)
        {
            var isOperating = await this.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state drop the message
                return;
            }

            var response = (new InboundDeleteStreamResponseValue(deleteResponseMessage)).Response;
            var streamId = deleteResponseMessage.StreamIdentity;

            FabricEvents.Events.CompleteDeleteStreamProtocol("Start@" + this.TraceType, streamId.ToString(), response.ToString());

            Diagnostics.Assert(
                (response == StreamWireProtocolResponse.TargetNotified) ||
                (response == StreamWireProtocolResponse.StreamNotFound),
                "{0} Unexpected StreamWireProtocolResponse in StreamManager.CompleteDeleteStreamProtocol",
                this.Tracer);

            MessageStream stream = null;
            var getSuccess = this.RuntimeResources.OutboundStreams.TryGetValue(streamId, out stream);

            // There is a chance that the Delete Stream Ack was received for a stale stream(deleted).
            // In this case, check existence of stream in TStore before asserting.
            if (getSuccess == false)
            {
                // Check if stream already exists in consolidated store.
                using (var replicatorTransaction = await this.CreateReplicatorTransactionAsync(Timeout.InfiniteTimeSpan))
                {
                    // Assert if found -- clearly there is a mismatch in in-memory cache vs the store.
                    getSuccess = await this.DoesOutboundStreamExistsInConsolidatedStore(replicatorTransaction, streamId);
                    Diagnostics.Assert(
                        !getSuccess,
                        "{0} unexpected failure to find outbound stream in CompleteDeleteStreamProtocol StreamId: {1}",
                        this.Tracer,
                        streamId);
                }
                Tracing.WriteWarning(
                    "StreamManager.CompleteDeleteStreamProtocol.Failure to find outbound stream",
                    "Target: {0} StreamID: {1}",
                    this.Tracer,
                    streamId);
                return;
            }
            await stream.CompleteDeleteAsync(response);
            FabricEvents.Events.CompleteDeleteStreamProtocol("Finish@" + this.TraceType, streamId.ToString(), response.ToString());
        }

        /// <summary>
        /// Register the inbound stream only if the target service accepts the inbound stream request by
        /// storing the required metadata of the stream to consolidated store.
        /// </summary>
        /// <param name="metadataStoreTransaction">RW Transaction</param>
        /// <param name="partnerKey">Partner Key</param>
        /// <param name="streamName">Stream Name to register</param>
        /// <param name="streamId">Stream Id as Guid</param>
        /// <returns></returns>
        internal async Task RegisterInboundStream(Store.IStoreReadWriteTransaction metadataStoreTransaction, PartitionKey partnerKey, Uri streamName, Guid streamId)
        {
            FabricEvents.Events.RegisterInboundStream("Start@" + this.TraceType, streamName.OriginalString, streamId.ToString(), partnerKey.ToString());
            try
            {
                await this.consolidatedStore.AddAsync(
                    metadataStoreTransaction,
                    new StreamStoreConsolidatedKey(new StreamMetadataKey(streamId, StreamMetadataKind.ReceiveSequenceNumber)),
                    new StreamStoreConsolidatedBody(new ReceiveStreamSequenceNumber(StreamConstants.FirstMessageSequenceNumber)),
                    Timeout.InfiniteTimeSpan,
                    CancellationToken.None);

                await this.consolidatedStore.AddAsync(
                    metadataStoreTransaction,
                    new StreamStoreConsolidatedKey(new StreamMetadataKey(streamId, StreamMetadataKind.InboundStableParameters)),
                    new StreamStoreConsolidatedBody(new InboundStreamStableParameters(streamName, partnerKey, PersistentStreamState.Open)),
                    Timeout.InfiniteTimeSpan,
                    CancellationToken.None);
            }
            catch (Exception e)
            {
                // TODO: handle exceptions
                Tracing.WriteExceptionAsWarning(
                    "StreamManager.RegisterInboundStream.Failure",
                    e,
                    "Target: {0} Source: {1} StreamName: {2} StreamID: {3}",
                    this.Tracer,
                    partnerKey,
                    streamName,
                    streamId);
            }

            FabricEvents.Events.RegisterInboundStream("Finish@" + this.TraceType, streamName.OriginalString, streamId.ToString(), partnerKey.ToString());
        }

        #endregion

        #region Stream Inbound Message Handlers

        /// <summary>
        /// When an inbound message is retrieved, pick the right inbound stream in the queue and callback the corresponding 
        /// message Received operation to consume this message.
        /// </summary>
        /// <param name="message">Inbound Message</param>
        /// <returns></returns>
        internal async Task MessageReceived(InboundBaseStreamWireMessage message)
        {
            MessageStream inboundStream = null;
            Diagnostics.Assert(
                (message.Kind == StreamWireProtocolMessageKind.CloseStream) || (message.Payload != null),
                "{0} Unexpected null payload in data content message",
                this.Tracer);
            Diagnostics.Assert(
                message.MessageSequenceNumber > 0,
                "{0} Unexpected non-positive sequence number in stream message, Kind: {1}",
                this.Tracer,
                message.Kind);

            var isOperating = await this.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state drop the message
                return;
            }

            var inboundStreamFound = this.RuntimeResources.InboundStreams.TryGetValue(message.StreamIdentity, out inboundStream);

            if (inboundStreamFound)
            {
                inboundStream.MessageReceived(message);
            }
            else
            {
                FabricEvents.Events.StreamNotFoundForInboundWireMessage(
                    this.TraceType,
                    message.StreamIdentity.ToString(),
                    message.MessageSequenceNumber,
                    message.Kind.ToString());
            }
        }

        /// <summary>
        /// Message Ack. Delete from Senders durable storage and update to new delete sequence number.
        /// </summary>
        /// <param name="ackMessage">Message Ack.</param>
        /// <returns></returns>
        internal async Task AckReceived(InboundBaseStreamWireMessage ackMessage)
        {
            Diagnostics.Assert(ackMessage.Payload == null, "{0} Unexpected non-null payload in sequence ack message", this.Tracer);
            Diagnostics.Assert(ackMessage.MessageSequenceNumber > 0, "{0} Unexpected non-positive sequence number in sequence ack message", this.Tracer);
            MessageStream stream = null;

            var isOperating = await this.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state drop the message
                return;
            }

            var outboundStreamFound = this.RuntimeResources.OutboundStreams.TryGetValue(ackMessage.StreamIdentity, out stream);

            if (outboundStreamFound)
            {
                await stream.AckReceived(ackMessage);
            }
            else
            {
                FabricEvents.Events.StreamNotFoundForInboundWireMessage(
                    this.TraceType,
                    ackMessage.StreamIdentity.ToString(),
                    ackMessage.MessageSequenceNumber,
                    ackMessage.Kind.ToString());
            }
        }

        #endregion

        #region Recovery handlers

        /// <summary>
        /// Reset Partner Stream Requested. This needs to be an idempotent operation.
        /// As with any distributed service in the windows fabric world the primary is bound to be moved from
        /// replica to replica for various reasons, when this happens the stream manager is deactivated from the old-primary 
        /// and re-incarnated in the new primary. 
        /// 
        /// Due to this,  the endpoint of the service may change thereby we send out ResetPartnerStream request anytime 
        /// the primary changes to all outbound sessions(properties that have a stream open with this service) requesting they reset there outbound session to the 'this' changed primary service.
        /// 
        /// This operation is called as a response to ResetPartnerStream request and during first contact situation during Open stream call. 
        /// Please refer to OpenStreamAsync() for details.
        /// </summary>
        /// <param name="requesterEra">What is the requested Era.</param>
        /// <param name="partnerKey">Partner Key to be reset</param>
        /// <param name="sendResponse">Send Response</param>
        /// <returns></returns>
        internal async Task ResetPartnerStreamsAsync(Guid requesterEra, PartitionKey partnerKey, bool sendResponse = true)
        {
            var isOperating = await this.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state drop the message
                return;
            }

            FabricEvents.Events.ResetPartnerStreams("Start@" + this.TraceType, partnerKey.ToString());

            var syncPoint = new SyncPoint<PartitionKey>(this, partnerKey, this.RuntimeResources.PartnerResetSyncpoints);

            try
            {
                // this prevents any new streams from being opened while we are in the middle of reset -- we may end up clearing the driver for a new stream that was created 
                // and opened between the PausePartnerStreamsAsync statement and the ClearDriverAsync statement -- unlikely but possible
                // A new stream may be created in between but it will not have a driver and it will not be restarted since we restart only the paused streams
                await syncPoint.EnterAsync(Timeout.InfiniteTimeSpan);

                // We pause all streams, then reconstruct the outbound driver with an empty session queue
                var pausedStreams = await this.PausePartnerStreamsAsync(partnerKey);

                await OutboundSessionDriver.ClearDriverAsync(partnerKey, this);
                var driver = await OutboundSessionDriver.FindOrCreateDriverAsync(partnerKey, this, this.SessionConnectionManager, Timeout.InfiniteTimeSpan);

                if (sendResponse)
                {
                    // not an internal reset -- send response to partner who asked for the reset
                    var responseMessage = new OutboundResetPartnerStreamsResponseWireMessage(
                        requesterEra,
                        StreamWireProtocolResponse.ResetPartnerStreamsCompleted);
                    driver.Send(responseMessage);
                }

                await this.RestartPartnerStreamsAsync(partnerKey, pausedStreams, true);
            }
            finally
            {
                syncPoint.Dispose();
            }

            FabricEvents.Events.ResetPartnerStreams("Finish@" + this.TraceType, partnerKey.ToString());
        }

        /// <summary>
        /// Pause all relevant streams for this parter.
        /// </summary>
        /// <param name="partnerKey">Partner Key</param>
        /// <returns></returns>
        internal async Task<ConcurrentDictionary<Guid, MessageStream>> PausePartnerStreamsAsync(PartitionKey partnerKey)
        {
            FabricEvents.Events.PausePartnerStreams("Start@" + this.TraceType, partnerKey.ToString());

            // We remember and return the paused streams, since new outbound streams may be created in the middle of the reset process for a partner 
            var pausedStreams = new ConcurrentDictionary<Guid, MessageStream>();

            foreach (var pair in this.RuntimeResources.OutboundStreams)
            {
                var stream = pair.Value;
                var streamState = stream.CurrentState;

                if ((streamState != PersistentStreamState.Initialized) && (streamState != PersistentStreamState.Closed))
                {
                    // need to normalize because the outbound stream partner may be numbered and in that case a single number in the range was used to identify target partition 
                    var normalizedKey = this.SessionConnectionManager.GetNormalizedPartitionKey(stream.PartnerIdentity);

                    if (normalizedKey.Equals(partnerKey))
                    {
                        await stream.PauseAsync();
                        var addSuccess = pausedStreams.TryAdd(pair.Key, stream);
                        Diagnostics.Assert(addSuccess, "{0} Unexpected failure to add stream with Id: {1} to paused streams collection", this.Tracer, pair.Key);
                    }
                }
            }

            /*
             * We cannot pause inbound streams nor do we need to; the service may have outstanding ReceiveAsync calls which are holding a lock on
               the receive sequence number so pause may deadlock, but in any case ReceiveAsync will pause naturally after the inbound buffer becomes empty
             */

            FabricEvents.Events.PausePartnerStreams("Finish@" + this.TraceType, partnerKey.ToString());

            return pausedStreams;
        }

        ///<summary>
        /// In a restartAfterPause scenario We restart only the paused streams as given by the outboundStreamsToRestart parameter, 
        /// new outbound streams may be created 
        ///  in the middle of the reset process for a partner after the PausePartnerStreamsAsync is completed, 
        /// they do not need to be and will not be restarted
        ///</summary>
        /// <param name="partnerKey">Partner Key to restart</param>
        /// <param name="outboundStreamsToRestart">Outbound Streams to restart</param>
        /// <param name="restartAfterPause">Restart after pause</param>
        /// <returns></returns>
        internal async Task RestartPartnerStreamsAsync(
            PartitionKey partnerKey, ConcurrentDictionary<Guid, MessageStream> outboundStreamsToRestart, bool restartAfterPause)
        {
            var isOperating = await this.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state do nothing
                return;
            }

            FabricEvents.Events.RestartPartnerStreams("Start@" + this.TraceType, partnerKey.ToString());

            try
            {
                using (var replicatorTransaction = this.TReplicator.CreateTransaction())
                {
                    var myTransaction = this.consolidatedStore.CreateStoreReadWriteTransaction(replicatorTransaction);
                    foreach (var pair in outboundStreamsToRestart)
                    {
                        var stream = pair.Value;
                        var streamState = stream.CurrentState;

                        if ((streamState != PersistentStreamState.Initialized) && (streamState != PersistentStreamState.Closed))
                        {
                            // need to normalize because the outbound stream partner may be numbered and in that case a single number in the range was used to identify target partition 
                            var normalizedKey = this.SessionConnectionManager.GetNormalizedPartitionKey(stream.PartnerIdentity);

                            if (normalizedKey.Equals(partnerKey))
                            {
                                await stream.RestartOutboundAsync(myTransaction, restartAfterPause);
                            }
                        }
                    }

                    foreach (var pair in this.RuntimeResources.InboundStreams)
                    {
                        var stream = pair.Value;
                        if (stream.PartnerIdentity.Equals(partnerKey))
                        {
                            await stream.RestartInboundAsync();
                        }
                    }
                }
            }
            catch (Exception e)
            {
                e = Diagnostics.ProcessException(
                    "Stream.RestartPartnerStreamsAsync",
                    e,
                    "{0} UnexpectedException",
                    this.TraceType);

                if (e is FabricNotPrimaryException)
                {
                    FabricEvents.Events.RestartPartnerStreams("NotPrimary@" + this.TraceType, partnerKey.ToString());
                    throw;
                }

                if (e is FabricObjectClosedException)
                {
                    FabricEvents.Events.RestartPartnerStreams("ObjectClosed@" + this.TraceType, partnerKey.ToString());
                    throw;
                }

                if (e is FabricNotReadableException)
                {
                    if (this.NotWritable())
                    {
                        FabricEvents.Events.RestartPartnerStreams("NotPrimary@" + this.TraceType, partnerKey.ToString());
                        throw new FabricNotPrimaryException();
                    }
                }

                Tracing.WriteExceptionAsError("Stream.RestartPartnerStreamsAsync.UnexpectedException", e, "{0}", this.Tracer);
                Diagnostics.Assert(false, "{0} Stream RestartPartnerStreamsAsync UnexpectedException {1}", this.Tracer, e.GetType());
            }

            FabricEvents.Events.RestartPartnerStreams("Finish@" + this.TraceType, partnerKey.ToString());
        }

        #endregion

        #region Restore streams at primary startup

        /// <summary>
        /// Restore streams (stream manager active resources collections) from durable consolidated store when role becomes primary. 
        /// </summary>
        /// <returns></returns>
        private async Task RecoverStreamsOnBecomingPrimaryAsync()
        {
            FabricEvents.Events.RecoverPartnerStreams("Start@" + this.TraceType, this.PartitionIdentity.ToString());

            Diagnostics.Assert(
                this.consolidatedStore != null,
                "StreamManager.RecoverStreamsOnBecomingPrimaryAsync called before replicated store was initialized");
            Diagnostics.Assert(
                this.RuntimeResources.OutboundSessionDrivers.IsEmpty,
                "StreamManager.RecoverStreamsOnBecomingPrimaryAsync found non-empty OutboundSessionDrivers collection");

            Func<StreamStoreConsolidatedKey, bool> filter = (key) => (key.StoreKind == StreamStoreKind.MetadataStore) &&
                                                                     ((key.MetadataKey.MetadataKind == StreamMetadataKind.InboundStableParameters) ||
                                                                      (key.MetadataKey.MetadataKind == StreamMetadataKind.ReceiveSequenceNumber) ||
                                                                      (key.MetadataKey.MetadataKind == StreamMetadataKind.OutboundStableParameters));

            using (var replicatorTransaction = this.TReplicator.CreateTransaction())
            {
                // TODO: there is a store GetAsync at the bottom of this enumeration so think about the exceptions
                var readTransaction = this.consolidatedStore.CreateStoreReadWriteTransaction(replicatorTransaction);
                var storeEnumerable
                    = await this.consolidatedStore.CreateEnumerableAsync(readTransaction, filter, false, Store.ReadMode.CacheResult);

                var syncEnumerable = storeEnumerable.ToEnumerable();

                foreach (var nextPair in syncEnumerable)
                {
                    var key = nextPair.Key;
                    var body = nextPair.Value;

                    var validKey = (key.StoreKind == StreamStoreKind.MetadataStore) &&
                                   ((key.MetadataKey.MetadataKind == StreamMetadataKind.InboundStableParameters) ||
                                    (key.MetadataKey.MetadataKind == StreamMetadataKind.ReceiveSequenceNumber) ||
                                    (key.MetadataKey.MetadataKind == StreamMetadataKind.OutboundStableParameters));

                    Diagnostics.Assert(
                        validKey,
                        string.Format(
                            CultureInfo.InvariantCulture,
                            "Invalid key: {0} encountered in StreamManager.RecoverStreamsOnBecomingPrimaryAsync",
                            key));

                    if (key.MetadataKey.MetadataKind == StreamMetadataKind.InboundStableParameters)
                    {
                        var inboundParams = (InboundStreamStableParameters) body.MetadataBody;

                        // check for expired deleted inbound streams and hard delete them from the consolidated store.
                        if (inboundParams.CurrentState == PersistentStreamState.Deleted)
                        {
                            var exp = DateTime.UtcNow.Date - inboundParams.CurrentStateDate.Date;
                            if (exp.TotalDays > StreamConstants.ExpirationForDeletedInboundStream)
                            {
                                // preethas: re-visit this.
                                //using (Transaction replicatorTransaction = await this.CreateReplicatorTransactionAsync(Timeout.InfiniteTimeSpan))
                                //{
                                // remove inbound stream from store.
                                await this.DeleteInboundStreamFromConsolidatedStore(
                                    replicatorTransaction,
                                    key.StreamId,
                                    inboundParams.StreamName,
                                    inboundParams.PartnerId,
                                    Timeout.InfiniteTimeSpan);

                                await this.CompleteReplicatorTransactionAsync(replicatorTransaction, TransactionCompletionMode.Commit);
                                continue;
                                //}
                            }
                        }

                        // now recover all remaining inbound streams including deleted ones that have not expired.
                        // we need the non-expired deleted streams to replay lost acks for source delete stream requests.
                        await this.RecoverInboundStreamAsync(key.StreamId, inboundParams);
                    }

                    if (key.MetadataKey.MetadataKind == StreamMetadataKind.ReceiveSequenceNumber)
                    {
                        var receiveSeqNum = (ReceiveStreamSequenceNumber) body.MetadataBody;
                        var addSuccess = this.RuntimeResources.NextSequenceNumberToDelete.TryAdd(key.StreamId, receiveSeqNum.NextSequenceNumber);
                        Diagnostics.Assert(
                            addSuccess,
                            "{0} Unexpected failure to add NextSequenceNumberToDelete in StreamManager.RestoreInboundStream",
                            this.Tracer);
                    }

                    if (key.MetadataKey.MetadataKind == StreamMetadataKind.OutboundStableParameters)
                    {
                        var outboundParams = (OutboundStreamStableParameters) body.MetadataBody;
                        await this.RecoverOutboundStreamAsync(key.StreamId, outboundParams);
                    }
                }
            }

            FabricEvents.Events.RecoverPartnerStreams("Finish@" + this.TraceType, this.PartitionIdentity.ToString());
        }

        /// <summary>
        /// Recover(add to the stream manager active resources collection cache) Inbound Stream during role recovery.
        /// </summary>
        /// <param name="streamId">Stream Id</param>
        /// <param name="stableParameters">Inbound Stable Parameters</param>
        /// <returns></returns>
        private async Task RecoverInboundStreamAsync(Guid streamId, InboundStreamStableParameters stableParameters)
        {
            FabricEvents.Events.RecoverInboundStream(
                "Start@" + this.TraceType,
                stableParameters.StreamName.OriginalString,
                streamId.ToString(),
                stableParameters.CurrentState.ToString(),
                stableParameters.PartnerId.ToString());

            var outboundSessionDriver =
                await OutboundSessionDriver.FindOrCreateDriverAsync(stableParameters.PartnerId, this, this.SessionConnectionManager, Timeout.InfiniteTimeSpan);
            Diagnostics.Assert(
                outboundSessionDriver != null,
                "{0} Unexpected failure to find OutboundSessionDriver in StreamManager.RecoverInboundStreamAsync, StreamID: {1}",
                this.Tracer,
                streamId);

            var stream =
                new MessageStream(
                    StreamKind.Inbound,
                    stableParameters.PartnerId,
                    streamId,
                    stableParameters.StreamName,
                    0,
                    this.consolidatedStore,
                    outboundSessionDriver,
                    this,
                    stableParameters.CurrentState,
                    stableParameters.CloseMessageSequenceNumber);

            var addSuccess = this.RuntimeResources.InboundStreams.TryAdd(streamId, stream);
            Diagnostics.Assert(addSuccess, "{0} Unexpected failure to add inbound stream in StreamManager.InboundStreamRequested", this.Tracer);

            FabricEvents.Events.RecoverInboundStream(
                "Finish@" + this.TraceType,
                stableParameters.StreamName.OriginalString,
                streamId.ToString(),
                stableParameters.CurrentState.ToString(),
                stableParameters.PartnerId.ToString());
        }

        /// <summary>
        /// Recover(add to the stream manager active resources collection cache) Outbound Stream during role recovery.
        /// </summary>
        /// <param name="streamId">Stream Id</param>
        /// <param name="stableParameters">Outbound Stable Parameters</param>
        /// <returns></returns>
        private async Task RecoverOutboundStreamAsync(Guid streamId, OutboundStreamStableParameters stableParameters)
        {
            FabricEvents.Events.RecoverOutboundStream(
                "Start@" + this.TraceType,
                stableParameters.StreamName.OriginalString,
                streamId.ToString(),
                stableParameters.CurrentState.ToString(),
                stableParameters.PartnerId.ToString());

            OutboundSessionDriver outboundSessionDriver = null;

            if (stableParameters.CurrentState != PersistentStreamState.Initialized && stableParameters.CurrentState != PersistentStreamState.Closed)
            {
                outboundSessionDriver =
                    await
                        OutboundSessionDriver.FindOrCreateDriverAsync(stableParameters.PartnerId, this, this.SessionConnectionManager, Timeout.InfiniteTimeSpan);
                Diagnostics.Assert(
                    outboundSessionDriver != null,
                    "{0} Unexpected failure to get OutboundSessionDriver in StreamManager.RecoverOutboundStreamAsync",
                    this.Tracer);
            }

            var stream =
                new MessageStream(
                    StreamKind.Outbound,
                    stableParameters.PartnerId,
                    streamId,
                    stableParameters.StreamName,
                    stableParameters.MessageQuota,
                    this.consolidatedStore,
                    outboundSessionDriver,
                    this,
                    stableParameters.CurrentState,
                    stableParameters.CloseMessageSequenceNumber);

            var addSuccess = this.RuntimeResources.OutboundStreams.TryAdd(streamId, stream);
            Diagnostics.Assert(addSuccess, "{0} Unexpected failure to add outbound stream in StreamManager.RecoverOutboundStreamAsync", this.Tracer);

            FabricEvents.Events.RecoverOutboundStream(
                "Finish@" + this.TraceType,
                stableParameters.StreamName.OriginalString,
                streamId.ToString(),
                stableParameters.CurrentState.ToString(),
                stableParameters.PartnerId.ToString());
        }

        #endregion

        #region Store transaction notification handler

        /// <summary>
        ///  We use the store transaction notification handler, to handle operations after a service owned transaction is completed(Commited/Aborted) 
        /// by the service. 
        ///  For example we send out payload messages only if the service send transaction is commited or
        /// Send an Inbound Create Stream Acceptance only if the stream is created ex.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e">Event Args</param>
        private void OnConsolidatedStoreChanged(object sender, EventArgs e)
        {
            /*
            if (this.NotWritable())
            {
                return;
            }

            IStoreReadWriteTransaction eventSender = sender as IStoreReadWriteTransaction;
            INotifyStoreChangedEventArgs<StreamStoreConsolidatedKey, StreamStoreConsolidatedBody> eventArgsBase = e as INotifyStoreChangedEventArgs<StreamStoreConsolidatedKey, StreamStoreConsolidatedBody>;

            Diagnostics.Assert(eventArgsBase != null, "{0} Null event args base after cast in OnConsolidatedStoreChanged, actual type: {1}", this.Tracer, e.GetType());

            StoreChangedEventType eventType = eventArgsBase.StoreChangeType;

            if (eventType == StoreChangedEventType.Changed)
            {
                INotifyStoreChangeCollectionEventArgs<StreamStoreConsolidatedKey, StreamStoreConsolidatedBody> eventArgs 
                    = eventArgsBase as INotifyStoreChangeCollectionEventArgs<StreamStoreConsolidatedKey, StreamStoreConsolidatedBody>;

                Diagnostics.Assert(eventArgs != null, "{0} Null event args after cast in OnConsolidatedStoreChanged, actual type: {1}", this.Tracer, e.GetType());

                using (IEnumerator<INotifyStoreChangedEvent> enumerate = eventArgs.ChangeList.GetEnumerator())
                {
                    long lastSendSeqNum = -1;

                    while (enumerate.MoveNext())
                    {
                        if (enumerate.Current.ChangeType == StoreChangeEventType.Item)
                        {
                            INotifyStoreItemChangedEventArgs<StreamStoreConsolidatedKey, StreamStoreConsolidatedBody> storeEvent = enumerate.Current as INotifyStoreItemChangedEventArgs<StreamStoreConsolidatedKey, StreamStoreConsolidatedBody>;
                            
                            if (storeEvent.ItemChangeType == StoreItemChangedEventType.Added)
                            {
                                INotifyStoreAddedEventArgs<StreamStoreConsolidatedKey, StreamStoreConsolidatedBody> storeInsertEvent = storeEvent as INotifyStoreAddedEventArgs<StreamStoreConsolidatedKey, StreamStoreConsolidatedBody>;
                                if (storeInsertEvent.Item.Key.StoreKind == StreamStoreKind.MessageStore)
                                {
                                    StreamStoreMessageKey messageKey = storeInsertEvent.Item.Key.MessageKey;
                                    lastSendSeqNum = messageKey.StreamSequenceNumber;
                                    MessageStream outboundStream = null;
                                    bool streamFound = this.RuntimeResources.OutboundStreams.TryGetValue(messageKey.StreamId, out outboundStream);
                                    Diagnostics.Assert(streamFound, "{0} Unexpected failure to find Outbound Stream while sending payload message", this.Tracer);
                                    outboundStream.SendPayloadMessage(messageKey, storeInsertEvent.Item.Value.MessageBody);
                                }

                                if (storeInsertEvent.Item.Key.StoreKind == StreamStoreKind.MetadataStore)
                                {
                                    StreamMetadataKey metadataKey = storeInsertEvent.Item.Key.MetadataKey;

                                    if (metadataKey.MetadataKind== StreamMetadataKind.InboundStableParameters)
                                    {
                                        StreamMetadataBody metadataBody = storeInsertEvent.Item.Value.MetadataBody;
                                        InboundStreamStableParameters parameters = (InboundStreamStableParameters)metadataBody;

                                        Task.Run(() => this.InboundStreamRequestedCompletionAsync(parameters.PartnerId, parameters.StreamName, metadataKey.StreamId)
                                            .ContinueWith(this.InboundStreamRequestedCompletionContinuation));
                                    }
                                }
                            }

                            if (storeEvent.ItemChangeType == StoreItemChangedEventType.Removed)
                            {
                                INotifyStoreRemovedEventArgs<StreamStoreConsolidatedKey, StreamStoreConsolidatedBody> storeDeleteEvent = storeEvent as INotifyStoreRemovedEventArgs<StreamStoreConsolidatedKey, StreamStoreConsolidatedBody>;
                                if (storeDeleteEvent.Key.StoreKind == StreamStoreKind.MetadataStore)
                                {
                                    // This is called when a outbound stream is deleted.
                                    if (storeDeleteEvent.Key.MetadataKey.MetadataKind == StreamMetadataKind.OutboundStableParameters)
                                    {
                                        StreamMetadataKey metadataKey = storeDeleteEvent.Key.MetadataKey;
                                        MessageStream outboundStream = null;
                                        bool streamFound = this.RuntimeResources.OutboundStreams.TryGetValue(metadataKey.StreamId, out outboundStream);
                                        Diagnostics.Assert(streamFound, "{0} Unexpected failure to find Outbound Stream while deleting stream.", this.Tracer);
                                        // Remove stream from runtime resources dict. and update in memory stream state to deleted.
                                        outboundStream.OutboundStreamDeleted();
                                    }
                                    // We do not need this update for inbound streams as, as streams are deleted from store via lazy delete process  
                                    // during restart of the roles. Since we do not hydrate inbound streams that are hard deleted from consolidated store
                                    // This update is not required.
                                }
                            }

                            if (storeEvent.ItemChangeType == StoreItemChangedEventType.Updated)
                            {
                                INotifyStoreUpdateEventArgs<StreamStoreConsolidatedKey, StreamStoreConsolidatedBody> storeUpdateEvent = storeEvent as INotifyStoreUpdateEventArgs<StreamStoreConsolidatedKey, StreamStoreConsolidatedBody>;

                                if (storeUpdateEvent.Item.Key.StoreKind == StreamStoreKind.MetadataStore)
                                {
                                    if (storeUpdateEvent.Item.Key.MetadataKey.MetadataKind== StreamMetadataKind.ReceiveSequenceNumber)
                                    {
                                        // TODO: look at sequence number management end-to-end including what happens to Open and Close messages
                                        ReceiveStreamSequenceNumber sequenceNumberMetadata = (ReceiveStreamSequenceNumber)storeUpdateEvent.Item.Value.MetadataBody;
                                        long nextReceiveSSN = sequenceNumberMetadata.NextSequenceNumber;
                                        Guid streamId = storeUpdateEvent.Item.Key.StreamId;

                                        MessageStream inboundStream = null;
                                        bool streamFound = this.RuntimeResources.InboundStreams.TryGetValue(streamId, out inboundStream);
                                        Diagnostics.Assert(streamFound, "Missing inbound stream during message ack");

                                        inboundStream.RemoveMessages(nextReceiveSSN);

                                        // check if the close message was transactionally received, and also send receive ack
                                        long closeMessageSequenceNumber = inboundStream.CloseMessageSequenceNumber;
                                        bool streamClosed = (closeMessageSequenceNumber > 0) && (nextReceiveSSN > closeMessageSequenceNumber);
                                        if (streamClosed)
                                        {
                                            // don't send ack if the stream was empty
                                            if (closeMessageSequenceNumber > StreamConstants.FirstMessageSequenceNumber)
                                            {
                                                inboundStream.SendAck(closeMessageSequenceNumber - 1);
                                            }
                                        }
                                        else
                                        {
                                            // Don't send ack if the nextReceiveSSN did not actually go beyond 1 -- can happen if the first receive timed out
                                            if (nextReceiveSSN > StreamConstants.FirstMessageSequenceNumber)
                                            {
                                                inboundStream.SendAck(nextReceiveSSN - 1);
                                            }
                                        }
                                    }

                                    if (storeUpdateEvent.Item.Key.MetadataKey.MetadataKind== StreamMetadataKind.InboundStableParameters)
                                    {
                                        StreamMetadataKey metadataKey = storeUpdateEvent.Item.Key.MetadataKey;
                                        StreamMetadataBody metadataBody = storeUpdateEvent.Item.Value.MetadataBody;
                                        InboundStreamStableParameters parameters = (InboundStreamStableParameters)metadataBody;
                                        if (parameters.CurrentState == PersistentStreamState.Closed)
                                        {
                                            MessageStream inboundStream = null;
                                            bool streamFound = this.RuntimeResources.InboundStreams.TryGetValue(metadataKey.StreamId, out inboundStream);
                                            Diagnostics.Assert(streamFound, "Missing inbound stream while executing InboundStreamClosed commit notification");
                                            inboundStream.InboundStreamClosed();
                                        }
                                    }

                                    // This is called during the commit of DeleteStream() transaction.
                                    if (storeUpdateEvent.Item.Key.MetadataKey.MetadataKind == StreamMetadataKind.OutboundStableParameters)
                                    {
                                        StreamMetadataKey metadataKey = storeUpdateEvent.Item.Key.MetadataKey;
                                        StreamMetadataBody metadataBody = storeUpdateEvent.Item.Value.MetadataBody;
                                        OutboundStreamStableParameters parameters = (OutboundStreamStableParameters)metadataBody;

                                        if (parameters.CurrentState == PersistentStreamState.Deleting)
                                        {
                                            MessageStream outboundStream = null;
                                            bool streamFound = this.RuntimeResources.OutboundStreams.TryGetValue(metadataKey.StreamId, out outboundStream);
                                            Diagnostics.Assert(streamFound, "{0} Unexpected failure to find Outbound Stream while sending OutboundStream delete request message", this.Tracer);

                                            // Initiate the delete protocol.
                                            outboundStream.FinishDeleteAsync(Timeout.InfiniteTimeSpan).ContinueWith(outboundStream.FinishDeleteAsyncContinuation);
                                        }
                                       }
                                }
                            }
                        }
                    }
                }
            }

            else if (eventType == StoreChangedEventType.Clear)
            {
                INotifyStoreClearEventArgs<StreamStoreConsolidatedKey, StreamStoreConsolidatedBody> storeEvent = eventArgsBase as INotifyStoreClearEventArgs<StreamStoreConsolidatedKey, StreamStoreConsolidatedBody>;
                
                {
                    // TODO: what is this Clear all about?  Should we use this for anything?
                }
            }

            // check if transaction aborted
            else if (eventType == StoreChangedEventType.None)
            {
                // we are going to discharge waiters only if the transaction aborted -- if the transaction committed any waiters that got in 
                // and are still waiting are the only waiters that will ever have a chance to receive that message so we will keep them alive; 
                // no guarantee they will ever complete in a transactional sense since the replica could crash before they are fulfilled but  
                // the guarantee of delivery only applies if the service waits for the receive to complete before committing the enclosing transaction
                ConcurrentDictionary<Guid, MessageStream> registeredInboundStreams = null;

                bool registeredStreamsExist = this.RuntimeResources.ActiveInboundStreamsByTransaction.TryRemove(eventSender.Id, out registeredInboundStreams);

                if (registeredStreamsExist)
                {
                    foreach (KeyValuePair<Guid, MessageStream> pair in registeredInboundStreams)
                    {
                        pair.Value.CloseWaiter(eventSender.Id);
                    }
                }

                // TODO: this is where we would take back an outbound stream if the transaction aborted
            }
            */
        }

        #endregion

        #region ReplicatorTransactionHandling

        /// <summary>
        /// Creates a replicator transaction and retries transient errors.
        /// </summary>
        /// <returns></returns>
        internal async Task<Transaction> CreateReplicatorTransactionAsync(TimeSpan timeout)
        {
            return await CreateReplicatorTransactionAsync(this.TReplicator, this.TraceType, this.Tracer, timeout);
        }

        /// <summary>
        /// Creates a replicator transaction and retries transient errors during create.
        /// </summary>
        /// <param name="tReplicator">Replicator</param>
        /// <param name="traceType">Trace Type</param>
        /// <param name="tracer">Tracer</param>
        /// <param name="timeout">Timeout for the operation to complete</param>
        /// <returns>Transaction</returns>
        private static async Task<Transaction> CreateReplicatorTransactionAsync(
            TransactionalReplicator tReplicator, string traceType, string tracer, TimeSpan timeout)
        {
            Transaction replicatorTransaction = null;
            var transactionCreated = false;

            Diagnostics.Assert(
                (timeout == Timeout.InfiniteTimeSpan || timeout.TotalMilliseconds >= 0),
                "{0} Invalid timeout in CreateReplicatorTransactionAsync",
                tracer);

            var remainingTimeout = timeout;

            // Create replicator transaction.
            while (!transactionCreated)
            {
                var startCreate = DateTime.UtcNow;

                try
                {
                    replicatorTransaction = tReplicator.CreateTransaction();
                    transactionCreated = true;
                }
                catch (FabricTransientException)
                {
                    FabricEvents.Events.ReplicatorTransactionTransientException(
                        "Create@" + traceType,
                        "Timeout: " + ((long) timeout.TotalMilliseconds),
                        replicatorTransaction.ToString());
                }
                catch (Exception e)
                {
                    Tracing.WriteExceptionAsError("CreateReplicatorTransaction.FabricPermanentException", e, "{0}", tracer);
                    // TODO what kind of exception should this be?  Should we wrap it in something?
                    throw;
                }

                if (!transactionCreated)
                {
                    var pastCreate = DateTime.UtcNow;

                    remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, startCreate, pastCreate);

                    int delay;

                    if (remainingTimeout == Timeout.InfiniteTimeSpan || remainingTimeout.TotalMilliseconds > StreamConstants.BaseDelayForReplicatorRetry)
                    {
                        delay = StreamConstants.BaseDelayForReplicatorRetry;
                    }
                    else
                    {
                        delay = (int) remainingTimeout.TotalMilliseconds;
                    }

                    // Back off it will likely pass
                    await Task.Delay(delay);
                }
            }

            return replicatorTransaction;
        }

        /// <summary>
        /// Creates a replicator transaction and retries transient errors.
        /// Not using this since the V2 replicator retries on transient errors
        /// </summary>
        /// <returns></returns>
        internal async Task CompleteReplicatorTransactionAsync(Transaction replicatorTransaction, TransactionCompletionMode mode)
        {
            await CompleteReplicatorTransactionAsync(replicatorTransaction, mode, this.TraceType, this.Tracer);
        }

        /// <summary>
        /// Completes the Transaction and retries transient errors.
        /// </summary>
        /// <param name="replicatorTransaction">Transaction</param>
        /// <param name="mode">Commit/Abort</param>
        /// <param name="traceType">Trace Type</param>
        /// <param name="tracer">Tracer</param>
        /// <returns></returns>
        private static async Task CompleteReplicatorTransactionAsync(
            Transaction replicatorTransaction, TransactionCompletionMode mode, string traceType, string tracer)
        {
            var transactionCompleted = false;

            while (!transactionCompleted)
            {
                try
                {
                    if (mode == TransactionCompletionMode.Commit)
                    {
                        await replicatorTransaction.CommitAsync();
                    }
                    else
                    {
                        await replicatorTransaction.AbortAsync();
                    }

                    transactionCompleted = true;
                }
                catch (FabricTransientException)
                {
                    FabricEvents.Events.ReplicatorTransactionTransientException("Complete@" + traceType, "Mode: " + mode, replicatorTransaction.ToString());
                }
                catch (Exception e)
                {
                    Tracing.WriteExceptionAsError(
                        "CompleteReplicatorTransactionAsync.FabricPermanentException",
                        e,
                        "{0} CompletionType: {1}",
                        tracer,
                        mode);
                    // TODO what kind of exception should this be?
                    throw;
                }

                if (!transactionCompleted)
                {
                    // Back off it will likely pass
                    await Task.Delay(StreamConstants.BaseDelayForReplicatorRetry);
                }
            }
        }

        #endregion

        #region PartitionChecks

        /// <summary>
        /// Wait for the role to change to Primary.
        /// </summary>
        /// <returns></returns>
        internal async Task WaitForPrimaryRoleAsync()
        {
            while (true)
            {
                var writeStatus = this.partition.WriteStatus;
                var readStatus = this.partition.ReadStatus;

                Diagnostics.Assert(
                    (writeStatus != PartitionAccessStatus.Invalid) && (readStatus != PartitionAccessStatus.Invalid),
                    "{0} Invalid partition status, ReadStatus: {1} WriteStatus: {2}",
                    this.Tracer,
                    readStatus,
                    writeStatus);

                if (writeStatus != PartitionAccessStatus.Granted || readStatus != PartitionAccessStatus.Granted)
                {
                    await Task.Delay(StreamConstants.BaseDelayForReplicatorRetry);
                }
                else
                {
                    break;
                }
            }
        }

        /// <summary>
        /// Am I primary?
        /// </summary>
        /// <returns></returns>
        internal bool NotWritable()
        {
            var status = this.TReplicator.StatefulPartition.WriteStatus;
            Diagnostics.Assert(PartitionAccessStatus.Invalid != status, "invalid partition status");

            return (PartitionAccessStatus.NotPrimary == status) || (this.roleSynchronizer.WaitHandle == null);
        }

        /// <summary>
        /// Throws exception if the state provider is not writable.
        /// </summary>
        /// 
        internal void ThrowIfNotWritable()
        {
            if (this.NotWritable())
            {
                var notPrimaryException = new FabricNotPrimaryException();
                Tracing.WriteExceptionAsWarning(
                    "FabricNotPrimary",
                    notPrimaryException,
                    "{0} Status: {1}",
                    this.Tracer,
                    this.TReplicator.StatefulPartition.WriteStatus);
                throw notPrimaryException;
            }
        }

        #endregion
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.StateProviders
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;    
    using System.Globalization;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Data.Services;
    using System.Fabric.Data.Replicator;

    /// <summary>
    /// Base implementation of a state provider.
    /// </summary>
    public abstract class ReplicatedStateProvider : IReplicatedStateProvider, IAtomicGroupStateProviderEx, IStateProviderBroker, IDisposable
    {
        #region Instance Members

        /// <summary>
        /// Replicator.
        /// </summary>
        IAtomicGroupStateReplicatorEx atomicGroupStateReplicator;

        /// <summary>
        /// Partition.
        /// </summary>
        IStatefulServicePartitionEx statefulPartition;

        /// <summary>
        /// Service instance name.
        /// </summary>
        Uri serviceInstanceName;

        /// <summary>
        /// Replica role.
        /// </summary>
        ReplicaRole role = ReplicaRole.Unknown;

        /// <summary>
        /// Replica id.
        /// </summary>
        long replicaId;

        /// <summary>
        /// Partition id.
        /// </summary>
        Guid partitionId;

        /// <summary>
        /// Health status for the state provider.
        /// </summary>
        private const long StatusFaulted = 0;
        private const long StatusHealthy = 1;
        long status = ReplicatedStateProvider.StatusFaulted;
        long faultType = (long)FaultType.Invalid;

        /// <summary>
        /// Max used for exponential backoff of replication operations.
        /// </summary>
        private const int ExponentialBackoffMax = 1024;

        /// <summary>
        /// Concurrency control.
        /// </summary>
        ReaderWriterLockSlim inFlightReplicationTasksLock = new ReaderWriterLockSlim(LockRecursionPolicy.SupportsRecursion);

        /// <summary>
        /// Populated on the primary with replication operations.
        /// </summary>
        SortedList<long, StateProviderReplicationContext> inFlightReplicationTasks = new SortedList<long, StateProviderReplicationContext>();

        /// <summary>
        /// Represents the highest replication sequence number that was completed.
        /// </summary>
        long lastCompletedSequenceNumberOnPrimary = FabricReplicatorEx.InvalidSequenceNumber;

        /// <summary>
        /// Progress vector used by a state provider that wants to implement partial copy.
        /// </summary>
        ProgressVector progressVector = new ProgressVector();

        /// <summary>
        /// Maximum number on entires in the progress vector.
        /// </summary>
        const int MaxProgressVectorEntryCount = 128;

        /// <summary>
        /// Activation context;
        /// </summary>
        CodePackageActivationContext codePackageActivationContext;

        
        #endregion

        #region Properties

        [SuppressMessage("Microsoft.Naming", "CA1711:IdentifiersShouldNotHaveIncorrectSuffix")]
        /// <summary>
        /// State replicator.
        /// </summary>
        public IAtomicGroupStateReplicatorEx AtomicGroupStateReplicatorEx
        {
            get
            {
                return this.atomicGroupStateReplicator;
            }
        }

        /// <summary>
        /// Stateful partition.
        /// </summary>
        public IStatefulServicePartitionEx StatefulPartition 
        {
            get
            {
                return this.statefulPartition;
            }
        }

        /// <summary>
        /// Service instance name hosting this state provider.
        /// </summary>
        public Uri ServiceInstanceName 
        {
            get
            {
                return this.serviceInstanceName;
            }
        }

        /// <summary>
        /// Role of this state provider. Kept in sync with the replica role hosting this state provider.
        /// </summary>
        public ReplicaRole ReplicaRole 
        {
            get
            {
                return this.role;
            }
            internal set 
            {
                this.role = value;
            }
        }

        /// <summary>
        /// Replica id.
        /// </summary>
        public long ReplicaId
        {
            get
            {
                return this.replicaId;
            }
        }

        /// <summary>
        /// Partition id.
        /// </summary>
        public Guid PartitionId
        {
            get
            {
                return this.partitionId;
            }
        }

        /// <summary>
        /// Progress vector.
        /// </summary>
        public ProgressVector StateVersionHistory
        {
            get
            {
                return this.progressVector;
            }
        }

        /// <summary>
        /// Volailte flag.
        /// </summary>
        public bool IsVolatile
        {
            get
            {
                return !this.statefulPartition.IsPersisted;
            }
        }

        /// <summary>
        /// Activation context.
        /// </summary>
        public CodePackageActivationContext CodePackageActivationContext
        {
            get
            {
                return this.codePackageActivationContext;
            }
        }

        #endregion

        #region IStateProviderBroker Members

        /// <summary>
        /// Initialization of the state provider broker.
        /// </summary>
        /// <param name="initializationParameters">State provider initialization data.</param>
        public virtual void Initialize(StatefulServiceInitializationParameters initializationParameters)
        {
            //
            // Check arguments.
            //
            if (null == initializationParameters)
            {
                throw new ArgumentNullException("initializationParameters");
            }
            //
            // Initialize members.
            //
            this.serviceInstanceName = initializationParameters.ServiceName;
            this.replicaId = initializationParameters.ReplicaId;
            this.partitionId = initializationParameters.PartitionId;
            this.codePackageActivationContext = initializationParameters.CodePackageActivationContext;
            AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.Initialize", "{0}", this.ToString());
        }

        /// <summary>
        /// Opens the state provider when the replica opens.
        /// </summary>
        /// <param name="openMode">Replica is either new or existent.</param>
        /// <param name="replicaId">Replica id hosting this state provider.</param>
        /// <param name="serviceInstanceName">Service instance name hosting this state provider.</param>
        /// <param name="statefulPartition">Partition hosting this state provider.</param>
        /// <param name="stateReplicator">Replicator used by this state provider.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        public virtual async Task OpenAsync(
            ReplicaOpenMode openMode,
            IStatefulServicePartitionEx statefulPartition,
            IAtomicGroupStateReplicatorEx stateReplicator,
            CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == statefulPartition || null == stateReplicator)
            {
                throw new ArgumentNullException();
            }
            AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.Open", "{0}", this.ToString());
            //
            // Populate fields.
            //
            this.statefulPartition = statefulPartition;
            this.atomicGroupStateReplicator = stateReplicator;
            this.status = ReplicatedStateProvider.StatusHealthy;
            //
            // Call the perform specialized open functionality.
            //
            await this.OnOpenAsync(openMode, cancellationToken);
            //
            // If the state provider is volatile, there is no recovery needed.
            //
            if (!this.IsVolatile)
            {
                AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.Open", "{0} start recovery processing", this.ToString());
                Task startRecovery = Task.Factory.StartNew(() => { this.OnRecovery(); });
            }
        }

        /// <summary>
        /// Called on the state provider when the replica is changing role.
        /// </summary>
        /// <param name="newRole">New role for the state provider.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        public virtual async Task ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            ReplicatedStateProvider.Assert(ReplicaRole.Unknown != newRole, "invalid new replica role");
            if (this.IsFaulted())
            {
                if (ReplicaRole.None != newRole)
                {
                    AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.ChangeRole", "{0} is faulted", this.ToString());
                    throw new StateProviderFaultedException();
                }
            }
            AppTrace.TraceSource.WriteNoise(
                "Data.ReplicatedStateProvider.ChangeRole",
                "{0} ({1}->{2})",
                this.ToString(),
                this.ReplicaRole,
                newRole);
            //
            // Check if the role change has been performed already.
            //
            if (newRole == this.ReplicaRole)
            {
                AppTrace.TraceSource.WriteNoise(
                    "Data.ReplicatedStateProvider.ChangeRole",
                    "{0} ({1}->{2}) already performed",
                    this.ToString(),
                    this.ReplicaRole,
                    newRole);
                return;
            }
            //
            // Perform role change.
            //
            if (ReplicaRole.Unknown == this.ReplicaRole)
            {
                if (ReplicaRole.ActiveSecondary == newRole)
                {
                    AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.ChangeRole", "{0} bug 868411", this.ToString());
                    Environment.Exit(-1); /* Bug 868411. */
                }
                if (ReplicaRole.None != newRole)
                {
                    //
                    // Report initial load metrics.
                    //
                    this.CatchAllReportLoadMetrics();
                }
                //
                // Call the perform specialized change role functionality.
                //
                await this.OnChangeRoleAsync(newRole, cancellationToken);
                //
                // Perform common change role functionality.
                //
                if (ReplicaRole.IdleSecondary == newRole)
                {
                    //
                    // Start copy stream processing.
                    //
                    AppTrace.TraceSource.WriteNoise(
                        "Data.ReplicatedStateProvider.ChangeRole",
                        "{0} ({1}->{2}) start copy processing",
                        this.ToString(),
                        this.ReplicaRole,
                        newRole);
                    Task startCopy = Task.Factory.StartNew(() => { this.OnCopy(); });
                }
            }
            if (ReplicaRole.IdleSecondary == this.ReplicaRole)
            {
                //
                // Call the perform specialized change role functionality.
                //
                await this.OnChangeRoleAsync(newRole, cancellationToken);
                //
                // Perform common change role functionality.
                //
                if (ReplicaRole.ActiveSecondary == newRole)
                {
                    //
                    // Start replication stream processing.
                    //
                    AppTrace.TraceSource.WriteNoise(
                        "Data.ReplicatedStateProvider.ChangeRole",
                        "{0} ({1}->{2}) start replication processing",
                        this.ToString(),
                        this.ReplicaRole,
                        newRole);
                    Task startReplication = Task.Factory.StartNew(() => { this.OnReplication(); });
                }
            }
            if (ReplicaRole.ActiveSecondary == this.ReplicaRole)
            {
                //
                // Call the perform specialized change role functionality.
                //
                await this.OnChangeRoleAsync(newRole, cancellationToken);
            }
            if (ReplicaRole.Primary == this.ReplicaRole)
            {
                //
                // Call the perform specialized change role functionality.
                //
                await this.OnChangeRoleAsync(newRole, cancellationToken);
                //
                // Perform common change role functionality.
                //
                if (ReplicaRole.ActiveSecondary == newRole)
                {
                    //
                    // Start replication stream processing.
                    //
                    AppTrace.TraceSource.WriteNoise(
                        "Data.ReplicatedStateProvider.ChangeRole",
                        "{0} ({1}->{2}) start replication processing",
                        this.ToString(),
                        this.ReplicaRole,
                        newRole);
                    Task startReplication = Task.Factory.StartNew(() => { this.OnReplication(); });
                }
            }
            //
            // Update current role.
            //
            this.ReplicaRole = newRole;
        }

        /// <summary>
        /// Closes the state provider gracefully.
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        public virtual async Task CloseAsync(CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.Close", "{0}", this.ToString());
            //
            // Call the perform specialized close functionality.
            //
            await this.OnCloseAsync(cancellationToken);
        }

        /// <summary>
        /// Called on the state provider when the replica is being aborted.
        /// </summary>
        public virtual void Abort()
        {
            AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.Abort", "{0}", this.ToString());
            //
            // Call the perform specialized abort functionality.
            //
            this.OnAbort();
        }

        #endregion

        #region IReplicatedStateProvider Members

        /// <summary>
        /// Opens the state provider.
        /// </summary>
        /// <param name="openMode">State provider is either new or existent.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Partition, replicator, or instance name are null.</exception>
        /// <returns></returns>
        protected override Task OnOpenAsync(ReplicaOpenMode openMode, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.OnOpenAsync", "{0}", this.ToString());

            //
            // Do nothing.
            //
            var tcs = new TaskCompletionSource<bool>();
            tcs.SetResult(true);
            return tcs.Task;
        }

        /// <summary>
        /// Signals the state provider that the replica role is changing.
        /// </summary>
        /// <param name="newRole">New role for the state provider.</param>
        /// <param name="cancellationToken">Cancellation token used to cancel this operation.</param>
        /// <returns></returns>
        protected override Task OnChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.OnChangeRoleAsync", "{0}", this.ToString());

            //
            // Do nothing.
            //
            var tcs = new TaskCompletionSource<bool>();
            tcs.SetResult(true);
            return tcs.Task;
        }

        /// <summary>
        /// Closes the state provider gracefully.
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        protected override Task OnCloseAsync(CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.OnCloseAsync", "{0}", this.ToString());

            //
            // Do nothing.
            //
            var tcs = new TaskCompletionSource<bool>();
            tcs.SetResult(true);
            return tcs.Task;
        }

        /// <summary>
        /// Aborts this state provider.
        /// </summary>
        protected override void OnAbort()
        {
            AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.OnAbort", "{0}", this.ToString());
        }

        /// <summary>
        /// Override in order to process the progres vector further.
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        protected override Task OnUpdateEpochAsync(CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.OnUpdateEpochAsync", "{0}", this.ToString());
            //
            // Complete this call synchronously.
            //
            TaskCompletionSource<bool> synchronousCompletion = new TaskCompletionSource<bool>();
            synchronousCompletion.SetResult(false);
            return synchronousCompletion.Task;
        }

        /// <summary>
        // Retrieves the replication stream and drains it.
        /// </summary>
        protected override async void OnReplication()
        {
            AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.OnReplication", "{0}", this.ToString());

            bool replicationSuccess = false;
            try
            {
                IOperationStreamEx stream = this.AtomicGroupStateReplicatorEx.GetReplicationStream();
                ReplicatedStateProvider.Assert(null != stream, "unexpected null replication operation stream");
                IOperationEx operation = await stream.GetOperationAsync(CancellationToken.None);
                while (null != operation)
                {
                    ReplicatedStateProvider.Assert(
                        OperationTypeEx.Redo == operation.OperationType ||
                        OperationTypeEx.Undo == operation.OperationType ||
                        OperationTypeEx.CreateAtomicGroup == operation.OperationType ||
                        OperationTypeEx.CommitAtomicGroup == operation.OperationType ||
                        OperationTypeEx.AbortAtomicGroup == operation.OperationType ||
                        OperationTypeEx.RollbackAtomicGroup == operation.OperationType ||
                        OperationTypeEx.SingleOperation == operation.OperationType ||
                        OperationTypeEx.EndOfStream == operation.OperationType ||
                        OperationTypeEx.RedoPassComplete == operation.OperationType /* Work item 1189766. */,
                        "unexpected replication operation type");
                    if (OperationTypeEx.EndOfStream == operation.OperationType ||
                        OperationTypeEx.RedoPassComplete == operation.OperationType /* Work item 1189766. */)
                    {
                        ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidAtomicGroupId == operation.AtomicGroupId, "unexpected atomic group id");
                        ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidSequenceNumber == operation.SequenceNumber, "unexpected sequence number");
                        ReplicatedStateProvider.Assert(0 == operation.Data.Count(), "unexpected operation data");
                        ReplicatedStateProvider.Assert(0 == operation.ServiceMetadata.Count(), "unexpected operation service metadata");
                    }
                    else
                    {
                        ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidSequenceNumber != operation.SequenceNumber, "unexpected sequence number");
                        if (OperationTypeEx.CreateAtomicGroup == operation.OperationType ||
                            OperationTypeEx.CommitAtomicGroup == operation.OperationType ||
                            OperationTypeEx.Redo == operation.OperationType ||
                            OperationTypeEx.Undo == operation.OperationType ||
                            OperationTypeEx.AbortAtomicGroup == operation.OperationType ||
                            OperationTypeEx.RollbackAtomicGroup == operation.OperationType)
                        {
                            ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidAtomicGroupId != operation.AtomicGroupId, "unexpected atomic group id");
                        }
                    }
                    //
                    // Check status.
                    //
                    if (this.IsFaulted())
                    {
                        AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.OnReplication", "{0} is faulted", this.ToString());
                        break;
                    }
                    try
                    {
                        AppTrace.TraceSource.WriteNoise(
                            "Data.ReplicatedStateProvider.OnReplication",
                            "{0} operation {1} atomic group {2} type {3}",
                            this.ToString(),
                            operation.SequenceNumber,
                            operation.AtomicGroupId,
                            operation.OperationType);

                        if (this.IsOperationCompletionOrdered)
                        {
                            //
                            // Apply replication operation.
                            //
                            await this.OnReplicationOperationAsync(operation, CancellationToken.None);
                            //
                            // Check end of stream.
                            //
                            replicationSuccess = (OperationTypeEx.EndOfStream == operation.OperationType);
                            //
                            // Ack current operation.
                            //
                            operation.Acknowledge();
                        }
                        else
                        {
                            //
                            // Start applying the replication operation.
                            //
                            Task apply = this.OnReplicationOperationAsync(operation, CancellationToken.None).ContinueWith((x) =>
                            {
                                Exception ex = null;
                                if (x.IsFaulted)
                                {
                                    ex = ReplicatedStateProvider.ProcessException("Data.ReplicatedStateProvider.OnReplication", x.Exception, "{0}", this.ToString());
                                }
                                if (ex is StateProviderNonRetryableException)
                                {
                                    //
                                    // The operation could not be applied. State provider is now faulted.
                                    //
                                    this.SetFaulted(FaultType.Permanent);
                                }
                                else
                                {
                                    if (null != ex)
                                    {
                                        //
                                        // The operation could not be applied. State provider is now faulted.
                                        //
                                        this.SetFaulted(FaultType.Transient);
                                    }
                                    //
                                    // Check end of stream.
                                    //
                                    replicationSuccess = (OperationTypeEx.EndOfStream == operation.OperationType);
                                    //
                                    // Ack current operation.
                                    //
                                    operation.Acknowledge();
                                }
                            },
                            TaskContinuationOptions.ExecuteSynchronously);
                        }
                    }
                    catch (Exception e)
                    {
                        e = ReplicatedStateProvider.ProcessException("Data.ReplicatedStateProvider.OnReplication", e, "{0}", this.ToString());
                        //
                        // The operation could not be performed. State provider is now faulted.
                        //
                        this.SetFaulted(e is StateProviderNonRetryableException ? FaultType.Permanent : FaultType.Transient);
                    }
                    //
                    // Check status.
                    //
                    if (this.IsFaulted())
                    {
                        AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.OnReplication", "{0} is faulted", this.ToString());
                        break;
                    }
                    //
                    // Move to next operation in the stream.
                    //
                    operation = await stream.GetOperationAsync(CancellationToken.None);
                }
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.ReplicatedStateProvider.OnReplication", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || e is FabricObjectClosedException, 
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Stream encountered out-of-memory or it was closed already.
                //
                if (e is OutOfMemoryException)
                {
                    this.SetFaulted(FaultType.Transient);
                }
            }
            if (!replicationSuccess)
            {
                AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.OnReplication", "{0} replication drain failed", this.ToString());
            }
            else
            {
                AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.OnReplication", "{0} replication drain succeeded", this.ToString());
            }
        }

        /// <summary>
        // Retrieves the copy stream and drains it.
        /// </summary>
        protected override async void OnCopy()
        {
            AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.OnCopy", "{0}", this.ToString());

            bool copySuccess = false;
            try
            {
                IOperationStreamEx stream = this.AtomicGroupStateReplicatorEx.GetCopyStream();
                ReplicatedStateProvider.Assert(null != stream, "unexpected null copy operation stream");
                IOperationEx operation = await stream.GetOperationAsync(CancellationToken.None);
                while (null != operation)
                {
                    ReplicatedStateProvider.Assert(OperationTypeEx.Copy == operation.OperationType || OperationTypeEx.EndOfStream == operation.OperationType, "unexpected copy operation type");
                    ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidAtomicGroupId == operation.AtomicGroupId, "unexpected atomic group id");
                    ReplicatedStateProvider.Assert(0 == operation.ServiceMetadata.Count(), "unexpected operation service metadata");
                    if (OperationTypeEx.EndOfStream == operation.OperationType)
                    {
                        ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidSequenceNumber == operation.SequenceNumber, "unexpected sequence number");
                        ReplicatedStateProvider.Assert(0 == operation.Data.Count(), "unexpected operation data");
                    }
                    else
                    {
                        ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidSequenceNumber != operation.SequenceNumber, "unexpected sequence number");
                    }
                    //
                    // Check status.
                    //
                    if (this.IsFaulted())
                    {
                        AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.OnCopy", "{0} is faulted", this.ToString());
                        break;
                    }
                    try
                    {
                        AppTrace.TraceSource.WriteNoise(
                            "Data.ReplicatedStateProvider.OnCopy",
                            "{0} operation {1} type {2}",
                            this.ToString(),
                            operation.SequenceNumber,
                            operation.OperationType);
                        //
                        // Apply copy operation.
                        //
                        await this.OnCopyOperationAsync(operation, CancellationToken.None);
                    }
                    catch (Exception e)
                    {
                        e = ReplicatedStateProvider.ProcessException("Data.ReplicatedStateProvider.OnCopy", e, "{0}", this.ToString());
                        //
                        // The operation could not be performed. State provider is now faulted.
                        //
                        this.SetFaulted(e is StateProviderNonRetryableException ? FaultType.Permanent : FaultType.Transient);
                    }
                    //
                    // Check status.
                    //
                    if (this.IsFaulted())
                    {
                        AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.OnCopy", "{0} is faulted", this.ToString());
                        break;
                    }
                    //
                    // Set copy success code.
                    //
                    copySuccess = OperationTypeEx.EndOfStream == operation.OperationType;
                    //
                    // Ack current operation.
                    //
                    operation.Acknowledge();
                    //
                    // Move to next operation in the stream.
                    //
                    operation = await stream.GetOperationAsync(CancellationToken.None);
                }
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.ReplicatedStateProvider.OnCopy", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || e is FabricObjectClosedException, 
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Stream encountered out-of-memory or it was closed already.
                //
                if (e is OutOfMemoryException)
                {
                    this.SetFaulted(FaultType.Transient);
                }
            }
            if (!copySuccess)
            {
                AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.OnCopy", "{0} copy drain failed", this.ToString());
            }
            else
            {
                AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.OnCopy", "{0} copy drain succeeded", this.ToString());
            }
        }

        /// <summary>
        // Retrieves the recovery stream and drains it.
        /// </summary>
        protected override async void OnRecovery()
        {
            AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.OnRecovery", "{0}", this.ToString());

            Exception exp = null;
            bool recoverySuccess = false;
            try
            {
                IOperationStreamEx stream = this.AtomicGroupStateReplicatorEx.GetRecoveryStream();
                ReplicatedStateProvider.Assert(null != stream, "unexpected null recovery operation stream");
                IOperationEx operation = await stream.GetOperationAsync(CancellationToken.None);
                while (null != operation)
                {
                    ReplicatedStateProvider.Assert(
                        OperationTypeEx.Redo == operation.OperationType ||
                        OperationTypeEx.Undo == operation.OperationType ||
                        OperationTypeEx.CreateAtomicGroup == operation.OperationType ||
                        OperationTypeEx.CommitAtomicGroup == operation.OperationType ||
                        OperationTypeEx.RollbackAtomicGroup == operation.OperationType ||
                        OperationTypeEx.SingleOperation == operation.OperationType ||
                        OperationTypeEx.EndOfStream == operation.OperationType ||
                        OperationTypeEx.RedoPassComplete == operation.OperationType ||
                        OperationTypeEx.UndoPassComplete == operation.OperationType,
                        "unexpected recovery operation type");
                    if (OperationTypeEx.EndOfStream == operation.OperationType ||
                        OperationTypeEx.RedoPassComplete == operation.OperationType ||
                        OperationTypeEx.UndoPassComplete == operation.OperationType)
                    {
                        ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidAtomicGroupId == operation.AtomicGroupId, "unexpected atomic group id");
                        ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidSequenceNumber == operation.SequenceNumber, "unexpected sequence number");
                        ReplicatedStateProvider.Assert(0 == operation.Data.Count(), "unexpected operation data");
                        ReplicatedStateProvider.Assert(0 == operation.ServiceMetadata.Count(), "unexpected operation service metadata");
                    }
                    else
                    {
                        ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidSequenceNumber != operation.SequenceNumber, "unexpected sequence number");
                        if (OperationTypeEx.CreateAtomicGroup == operation.OperationType ||
                            OperationTypeEx.Redo == operation.OperationType ||
                            OperationTypeEx.Undo == operation.OperationType ||
                            OperationTypeEx.CommitAtomicGroup == operation.OperationType ||
                            OperationTypeEx.RollbackAtomicGroup == operation.OperationType)
                        {
                            ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidAtomicGroupId != operation.AtomicGroupId, "unexpected atomic group id");
                        }
                    }
                    //
                    // Check status.
                    //
                    if (this.IsFaulted())
                    {
                        AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.OnRecovery", "{0} is faulted", this.ToString());
                        break;
                    }
                    try
                    {
                        AppTrace.TraceSource.WriteNoise(
                            "Data.ReplicatedStateProvider.OnRecovery",
                            "{0} operation {1} atomic group {2} type {3}",
                            this.ToString(),
                            operation.SequenceNumber,
                            operation.AtomicGroupId,
                            operation.OperationType);

                        if (this.IsOperationCompletionOrdered)
                        {
                            //
                            // Apply recovery operation.
                            //
                            await this.OnRecoveryOperationAsync(operation, CancellationToken.None);
                            //
                            // Check end of stream.
                            //
                            recoverySuccess = (OperationTypeEx.EndOfStream == operation.OperationType);
                            //
                            // Ack current operation.
                            //
                            operation.Acknowledge();
                        }
                        else
                        {
                            //
                            // Start applying the recovery operation.
                            //
                            Task apply = this.OnRecoveryOperationAsync(operation, CancellationToken.None).ContinueWith((x) =>
                            {
                                Exception ex = null;
                                if (x.IsFaulted)
                                {
                                    ex = ReplicatedStateProvider.ProcessException("Data.ReplicatedStateProvider.OnRecovery", x.Exception, "{0}", this.ToString());
                                }
                                if (ex is StateProviderNonRetryableException)
                                {
                                    //
                                    // The operation could not be applied. State provider is now faulted.
                                    //
                                    this.SetFaulted(FaultType.Permanent);
                                }
                                else
                                {
                                    if (null != ex)
                                    {
                                        //
                                        // The operation could not be applied. State provider is now faulted.
                                        //
                                        this.SetFaulted(FaultType.Transient);
                                    }
                                    //
                                    // Check end of stream.
                                    //
                                    recoverySuccess = (OperationTypeEx.EndOfStream == operation.OperationType);
                                    //
                                    // Ack current operation.
                                    //
                                    operation.Acknowledge();
                                }
                            },
                            TaskContinuationOptions.ExecuteSynchronously);
                        }
                    }
                    catch (Exception e)
                    {
                        e = ReplicatedStateProvider.ProcessException("Data.eplicatedStateProvider.OnRecovery", e, "{0}", this.ToString());
                        //
                        // The operation could not be performed. State provider is now faulted.
                        //
                        this.SetFaulted(exp is StateProviderNonRetryableException ? FaultType.Permanent : FaultType.Transient);
                    }
                    //
                    // Check status.
                    //
                    if (this.IsFaulted())
                    {
                        AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.OnRecovery", "{0} is faulted", this.ToString());
                        break;
                    }
                    //
                    // Move to next operation in the stream.
                    //
                    operation = await stream.GetOperationAsync(CancellationToken.None);
                }
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.ReplicatedStateProvider.OnRecovery", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || e is FabricObjectClosedException, 
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Stream encountered out-of-memory or it was closed already.
                //
                if (e is OutOfMemoryException)
                {
                    this.SetFaulted(FaultType.Transient);
                }
            }
            if (!recoverySuccess)
            {
                AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.OnRecovery", "{0} recovery drain failed", this.ToString());
            }
            else
            {
                AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.OnRecovery", "{0} recovery drain succeeded", this.ToString());
            }
        }

        /// <summary>
        /// Reports state provider load metrics in case they have changed.
        /// </summary>
        protected override void OnReportLoadMetrics()
        {
            //
            // Report no metrics by default.
            //
        }

        #endregion

        #region IAtomicGroupStateProviderEx Members

        /// <summary>
        /// Volatile state providers do not provide this information.
        /// </summary>
        /// <returns></returns>
        public abstract long GetLastCommittedSequenceNumber();

        /// <summary>
        /// By default, state providers are either volatile or do not support partial copy.
        /// </summary>
        /// <param name="epoch">New epoch.</param>
        /// <param name="previousEpochLastSequenceNumber">Last sequence number in the old epoch.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        public virtual async Task UpdateEpochAsync(Epoch epoch, long previousEpochLastSequenceNumber, CancellationToken cancellationToken)
        {
            if (this.IsFaulted())
            {
                AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.UpdateEpoch", "{0} is faulted", this.ToString());
                throw new StateProviderFaultedException();
            }
            AppTrace.TraceSource.WriteNoise(
                "Data.ReplicatedStateProvider.UpdateEpoch",
                "{0} received new epoch ({1}, {2})",
                this.ToString(),
                epoch.DataLossNumber,
                epoch.ConfigurationNumber);
            //
            // Update progress vector.
            //
            this.StateVersionHistory.Progress.Add(new StateVersion(epoch, previousEpochLastSequenceNumber));
            if (this.StateVersionHistory.Progress.Count > ReplicatedStateProvider.MaxProgressVectorEntryCount)
            {
                //
                // Truncate early progress vector entries.
                //
                this.StateVersionHistory.Progress.RemoveAt(0);
            }
            //
            // Allow for custom progress vector functionality.
            //
            await this.OnUpdateEpochAsync(cancellationToken);
        }

        /// <summary>
        /// By default, state providers do not support data loss processing. The state after data loss
        /// is the state that the service continues its execution with.
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        public virtual Task<bool> OnDataLossAsync(CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.OnDataLoss", "{0}", this.ToString());
            //
            // Complete this call synchronously.
            //
            TaskCompletionSource<bool> synchronousCompletion = new TaskCompletionSource<bool>();
            synchronousCompletion.SetResult(false);
            return synchronousCompletion.Task;
        }

        /// <summary>
        /// Asks the state provider to copy its partial copy context.
        /// </summary>
        /// <returns></returns>
        public abstract IOperationDataStreamEx GetCopyContext();

        /// <summary>
        /// Asks the state provider to copy its state.
        /// </summary>
        /// <param name="upToSequenceNumber">Highest sequence number not to be included in the copy state.</param>
        /// <param name="copyContext">Copy context for the copy state operation.</param>
        /// <returns></returns>
        public abstract IOperationDataStreamEx GetCopyState(long upToSequenceNumber, IOperationDataStreamEx copyContext);

        /// <summary>
        /// Commits an in-flight atomic group.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group to commit.</param>
        /// <param name="commitSequenceNumber">Sequence number of the commit operation.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        public virtual async Task AtomicGroupCommitAsync(long atomicGroupId, long commitSequenceNumber, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise(
                "Data.ReplicatedStateProvider.AtomicGroupCommit", 
                "{0} atomic group ({1}, {2})", 
                this.ToString(),
                atomicGroupId,
                commitSequenceNumber);
            //
            // Check status.
            //
            ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidAtomicGroupId != atomicGroupId, "unexpected atomic group id");
            if (this.IsFaulted())
            {
                AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.AtomicGroupCommit", "{0} is faulted", this.ToString());
            }
            else
            {
                AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.AtomicGroupCommit", "{0} succeeded", this.ToString());
                //
                // Allow for custom commit functionality.
                //
                await this.OnAtomicGroupCommitAsync(atomicGroupId, commitSequenceNumber, cancellationToken);
                //
                // Report load metrics.
                //
                this.CatchAllReportLoadMetrics();
            }
        }

        /// <summary>
        /// Rolls back an in-flight atomic group.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group that is rolled back.</param>
        /// <param name="undoOperationStream">Operation stream containing the undo operations part of the rollback.</param>
        /// <param name="cancellationToken">Cancellation token.</param>
        /// <returns></returns>
        public async virtual Task AtomicGroupRollbackAsync(long atomicGroupId, IOperationStreamEx undoOperationStream, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.AtomicGroupRollback", "{0}", this.ToString());

            bool rollbackSuccess = false;
            long rollbackSequenceNumber = FabricReplicatorEx.InvalidSequenceNumber;
            ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidAtomicGroupId != atomicGroupId, "unexpected atomic group id");
            //
            // Retrieve first undo operation.
            //
            IOperationEx operation = await undoOperationStream.GetOperationAsync(CancellationToken.None);
            ReplicatedStateProvider.Assert(null != operation, "unexpected null undo stream operation");
            while (null != operation)
            {
                ReplicatedStateProvider.Assert(
                    OperationTypeEx.Undo == operation.OperationType ||
                    OperationTypeEx.RollbackAtomicGroup == operation.OperationType ||
                    OperationTypeEx.EndOfStream == operation.OperationType,
                    "unexpected undo stream operation type");
                ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidAtomicGroupId != operation.AtomicGroupId, "unexpected atomic group id");
                if (OperationTypeEx.EndOfStream == operation.OperationType)
                {
                    ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidSequenceNumber == operation.SequenceNumber, "unexpected sequence number");
                    ReplicatedStateProvider.Assert(0 == operation.Data.Count(), "unexpected operation data");
                    ReplicatedStateProvider.Assert(0 == operation.ServiceMetadata.Count(), "unexpected operation service metadata");
                }
                else
                {
                    ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidSequenceNumber != operation.SequenceNumber, "unexpected sequence number");
                }
                //
                // Check status.
                //
                if (this.IsFaulted())
                {
                    AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.AtomicGroupRollback", "{0} is faulted", this.ToString());
                    break;
                }
                try
                {
                    AppTrace.TraceSource.WriteNoise(
                        "Data.ReplicatedStateProvider.AtomicGroupRollback", 
                        "{0} atomic group ({1}, {2})", 
                        this.ToString(),
                        atomicGroupId,
                        operation.SequenceNumber);
                    //
                    // Apply replication operation.
                    //
                    await this.OnReplicationOperationAsync(operation, CancellationToken.None);
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.ReplicatedStateProvider.AtomicGroupRollback", e, "{0}", this.ToString());
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(e is StateProviderNonRetryableException ? FaultType.Permanent : FaultType.Transient);
                }
                //
                // Check status.
                //
                if (this.IsFaulted())
                {
                    AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.AtomicGroupRollback", "{0} is faulted", this.ToString());
                    break;
                }
                //
                // Set rollback sequence number.
                //
                if (OperationTypeEx.RollbackAtomicGroup == operation.OperationType)
                {
                    rollbackSequenceNumber = operation.SequenceNumber;
                }
                //
                // Set rollback success code.
                //
                rollbackSuccess = OperationTypeEx.EndOfStream == operation.OperationType;
                //
                // Ack current operation.
                //
                operation.Acknowledge();
                //
                // Move to next operation in the stream.
                //
                operation = await undoOperationStream.GetOperationAsync(CancellationToken.None);
            }
            if (rollbackSuccess)
            {
                AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.AtomicGroupRollback", "{0} undo drain succeeded", this.ToString());
                //
                // Allow for custom rollback functionality to be performed.
                //
                await this.OnAtomicGroupRollbackAsync(atomicGroupId, rollbackSequenceNumber, cancellationToken);
                //
                // Report load metrics.
                //
                this.CatchAllReportLoadMetrics();
            }
            else
            {
                AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.AtomicGroupRollback", "{0} undo drain failed", this.ToString());
            }
        }

        /// <summary>
        /// Aborts an in-flight atomic group.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group to abort.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        public virtual async Task AtomicGroupAbortAsync(long atomicGroupId, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise(
                "Data.ReplicatedStateProvider.AtomicGroupAbort", 
                "{0} atomic group ({1}", 
                this.ToString(),
                atomicGroupId);
            //
            // Check status.
            //
            ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidAtomicGroupId != atomicGroupId, "unexpected atomic group id");
            if (this.IsFaulted())
            {
                AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.AtomicGroupAbort", "{0} is faulted", this.ToString());
            }
            else
            {
                AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.AtomicGroupAbort", "{0} succeeded", this.ToString());
                //
                // Allow for custom commit functionality.
                //
                await this.OnAtomicGroupAbortAsync(atomicGroupId, cancellationToken);
            }
        }

        /// <summary>
        /// By defaut, state providers do not support undoing committed data.
        /// </summary>
        /// <param name="fromCommitSequenceNumber">Lowest sequence number from which the committed state has to be undone.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        public virtual Task UndoProgressAsync(long fromCommitSequenceNumber, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.UndoProgress", "{0}", this.ToString());
            //
            // Complete this call synchronously.
            //
            TaskCompletionSource<bool> synchronousCompletion = new TaskCompletionSource<bool>();
            synchronousCompletion.SetResult(true);
            return synchronousCompletion.Task;
        }

        /// <summary>
        /// Checkpoints the state provider.
        /// </summary>
        /// <param name="operationSequenceNumber">State mutations up to this sequence number must be made stable.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        public abstract Task CheckpointAsync(long operationSequenceNumber, CancellationToken cancellationToken);

        /// <summary>
        /// Informs the state provider of the progress in the operation stream.
        /// </summary>
        /// <param name="operationSequenceNumber">State mutations up to this sequence number could be made stable.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        public abstract Task<long> OnOperationStableAsync(long operationSequenceNumber, CancellationToken cancellationToken);

        #endregion

        #region Helper Methods

        /// <summary>
        /// Reports load metrics and catches any exception.
        /// </summary>
        void CatchAllReportLoadMetrics()
        {
            try
            {
                AppTrace.TraceSource.WriteNoise(
                    "Data.ReplicatedStateProvider.CatchAllReportLoadMetrics", 
                    "{0} reports load metrics", 
                    this.ToString());
                //
                // Report load metrics.
                //
                this.OnReportLoadMetrics();
            }
            catch (Exception e)
            {
                //
                // Load reporting is best effort.
                //
                ReplicatedStateProvider.ProcessException("Data.ReplicatedStateProvider.CatchAllReportLoadMetrics", e, "{0} failed to report load metrics", this.ToString());
            }
        }

        /// <summary>
        /// Completes an in-flight replication operation on the primary.
        /// </summary>
        /// <param name="operation">Operation completed.</param>
        /// <param name="state">Gets a user-defined object that qualifies or contains information about an asynchronous operation.</param>
        /// <param name="fault">If operation was not replicated successfully, it identifies the error condition.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        protected abstract Task<object> OnReplicationOperationAsync(StateProviderOperation operation, Exception fault, CancellationToken cancellationToken);

        /// <summary>
        /// Used for tracing.
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Uri: {0}. PartitionId: {1}. ReplicaId: {2}",
                this.ServiceInstanceName,
                this.PartitionId, 
                this.ReplicaId);
        }

        /// <summary>
        /// Checks to see if this state provider has encountered an error condition.
        /// </summary>
        /// <returns></returns>
        protected bool IsFaulted()
        {
            return (ReplicatedStateProvider.StatusFaulted == Interlocked.Read(ref this.status));
        }

        /// <summary>
        /// Changes the stats of this state provider from healthy to faulty.
        /// </summary>
        /// <param name="fault">Type of fault encountered.</param>
        protected void SetFaulted(FaultType fault)
        {
            long result = (FaultType.Permanent == fault) ?
                Interlocked.CompareExchange(ref this.status, ReplicatedStateProvider.StatusFaulted, ReplicatedStateProvider.StatusHealthy) :
                ReplicatedStateProvider.StatusHealthy;
            Interlocked.CompareExchange(ref this.faultType, (long)fault, this.faultType);
            if (ReplicatedStateProvider.StatusHealthy == result)
            {
                if (FaultType.Permanent == fault)
                {
                    AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.SetFaulted", "{0} is now faulted with {1}", this.ToString(), fault);
                }
                else
                {
                    AppTrace.TraceSource.WriteWarning("Data.ReplicatedStateProvider.SetFaulted", "{0} is now faulted with {1}", this.ToString(), fault);
                }
                try
                {
                    this.StatefulPartition.ReportFault(fault);
                }
                catch (Exception e)
                {
                    ReplicatedStateProvider.ProcessException("Data.Data.ReplicatedStateProvider.SetFaulted", e, "{0}", this.ToString());
                    Environment.Exit(-1);
                }
            }
        }

        /// <summary>
        /// Returns the type of fault.
        /// </summary>
        /// <returns></returns>
        protected FaultType GetFault()
        {
            long faultType = Interlocked.Read(ref this.faultType);
            return (FaultType)faultType;
        }

        /// <summary>
        /// Checks if a replication exception is retryable.
        /// </summary>
        /// <param name="e">Exception to be tested.</param>
        /// <returns></returns>
        protected virtual bool IsRetryableReplicationException(Exception e)
        {
            return (e is FabricTransientException);
        }

        /// <summary>
        /// Checks if a state provider exception is retryable.
        /// </summary>
        /// <param name="e">Exception to be tested.</param>
        /// <returns></returns>
        protected virtual bool IsRetryableStateProviderException(Exception e)
        {
            return (e is StateProviderTransientException);
        }

        /// <summary>
        /// Checks if a state provider exception is retryable.
        /// </summary>
        /// <param name="e">Exception to be tested.</param>
        /// <returns></returns>
        protected virtual bool IsNonRetryableStateProviderException(Exception e)
        {
            return (e is StateProviderNonRetryableException);
        }

        /// <summary>
        /// Throws exception if the state provider is not in the primary role.
        /// </summary>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        protected virtual void ThrowIfNotWritable()
        {
            PartitionAccessStatus status = this.StatefulPartition.WriteStatus;
            ReplicatedStateProvider.Assert(PartitionAccessStatus.Invalid != status, "invalid partition status");
            if (PartitionAccessStatus.Granted != status)
            {
                if (PartitionAccessStatus.ReconfigurationPending == status || PartitionAccessStatus.NoWriteQuorum == status)
                {
                    AppTrace.TraceSource.WriteWarning("Data.ReplicatedStateProvider.ThrowIfNotWritable", "{0}", this.ToString());

                    throw new StateProviderNotWritableException();
                }
                if (PartitionAccessStatus.NotPrimary == status)
                {
                    AppTrace.TraceSource.WriteWarning("Data.ReplicatedStateProvider.ThrowIfNotWritable", "{0}", this.ToString());

                    throw new FabricNotPrimaryException();
                }
            }
        }

        /// <summary>
        /// Throws exception if the state provider is not readable.
        /// </summary>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNonRetryableException">The state provider is and will not become readable.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        protected virtual void ThrowIfNotReadable()
        {
            PartitionAccessStatus status = this.StatefulPartition.ReadStatus;
            ReplicatedStateProvider.Assert(PartitionAccessStatus.Invalid != status, "invalid partition status");
            if (PartitionAccessStatus.Granted != this.StatefulPartition.ReadStatus)
            {
                if (PartitionAccessStatus.ReconfigurationPending == status || PartitionAccessStatus.NoWriteQuorum == status)
                {
                    AppTrace.TraceSource.WriteWarning("Data.ReplicatedStateProvider.ThrowIfNotReadable", "{0}", this.ToString());

                    throw new StateProviderNotReadableException();
                }
                if (PartitionAccessStatus.NotPrimary == status)
                {
                    AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.ThrowIfNotReadable", "{0}", this.ToString());

                    throw new StateProviderNonRetryableException();
                }
            }
        }

        /// <summary>
        /// Throws exception if the state provider is faulted (transient or permanent).
        /// </summary>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        protected virtual void ThrowIfFaulted()
        {
            if (this.IsFaulted())
            {
                AppTrace.TraceSource.WriteWarning("Data.ReplicatedStateProvider.ThrowIfFaulted", "{0}", this.ToString());
                throw new StateProviderFaultedException();
            }
        }

        /// <summary>
        /// Throws exception if the atomic group is invalid.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group id to be tested.</param>
        /// <exception cref="System.ArgumentOutOfRangeException">The atomic group is invalid.</exception>
        /// <returns></returns>
        protected virtual void ThrowIfInvalidAtomicGroup(long atomicGroupId)
        {
            if (0 > atomicGroupId && FabricReplicatorEx.InvalidAtomicGroupId != atomicGroupId)
            {
                AppTrace.TraceSource.WriteWarning("Data.ReplicatedStateProvider.ThrowIfInvalidAtomicGroup", "{0}", this.ToString());

                throw new ArgumentOutOfRangeException("atomicGroupId");
            }
        }

        /// <summary>
        /// Checks if a replication exception is non-retryable.
        /// </summary>
        /// <param name="e">Exception to be tested.</param>
        /// <returns></returns>
        protected virtual bool IsNonRetryableReplicationException(Exception e)
        {
            return (e is FabricNotPrimaryException || e is FabricObjectClosedException || e is FabricInvalidAtomicGroupException);
        }

        /// <summary>
        /// Stores the replication context after the replication task was successfully issued.
        /// </summary>
        /// <param name="replicationContext">Replication context to be stored.</param>
        void RecordReplicationContext(StateProviderReplicationContext replicationContext)
        {
            if (this.IsOperationCompletionOrdered)
            {
                AppTrace.TraceSource.WriteNoise(
                    "Data.ReplicatedStateProvider.RecordReplicationContext", 
                    "{0} recorded operation {1} atomic group {2}", 
                    this.ToString(), 
                    replicationContext.Operation.SequenceNumber,
                    replicationContext.Operation.AtomicGroupId);
                //
                // Store in-flight replication context.
                //
                replicationContext.Status = StateProviderReplicationContextStatus.InFlight;
                this.inFlightReplicationTasks.Add(replicationContext.Operation.SequenceNumber, replicationContext);
                //
                // Assign the continuation of the replication task. The continuations will be
                // performed in order of the replication sequence numbers.
                //
                replicationContext.Task.ContinueWith((x) =>
                {
                    AppTrace.TraceSource.WriteNoise(
                        "Data.ReplicatedStateProvider.RecordReplicationContext",
                        "{0} in-order operation {1} atomic group {2}",
                        this.ToString(),
                        replicationContext.Operation.SequenceNumber,
                        replicationContext.Operation.AtomicGroupId);
                    //
                    // Mark the replication context as completed.
                    //
                    bool isFaulted = x.IsFaulted;
                    this.inFlightReplicationTasksLock.EnterWriteLock();
                    this.inFlightReplicationTasks[replicationContext.Operation.SequenceNumber].Status = StateProviderReplicationContextStatus.Completed;
                    //
                    // Updated last completed operation sequence number.
                    //
                    if (replicationContext.Operation.SequenceNumber > this.lastCompletedSequenceNumberOnPrimary)
                    {
                        this.lastCompletedSequenceNumberOnPrimary = replicationContext.Operation.SequenceNumber;
                    }

                    AppTrace.TraceSource.WriteNoise(
                        "Data.ReplicatedStateProvider.RecordReplicationContext",
                        "{0} last completed sequence number is {1}",
                        this.ToString(),
                        this.lastCompletedSequenceNumberOnPrimary);

                    if (isFaulted)
                    {
                        Exception e = ReplicatedStateProvider.ProcessException("Data.ReplicatedStateProvider.RecordReplicationContext", x.Exception, "{0}", this.ToString());
                        ReplicatedStateProvider.Assert(
                            this.IsNonRetryableReplicationException(e) || e is OperationCanceledException,
                            string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                        //
                        // Set replication task exception.
                        //
                        replicationContext.Exception = e;
                    }
                    this.inFlightReplicationTasksLock.ExitWriteLock();
                    //
                    // Run continuations for all in-order completed contexts.
                    //
                    this.CompleteMultipleReplicationContextsAsync(replicationContext).ContinueWith((y) =>
                    {
                    },
                    TaskContinuationOptions.ExecuteSynchronously);
                },
                TaskContinuationOptions.ExecuteSynchronously);
            }
            else
            {
                //
                // Set the continuation of the replication task. 
                //
                replicationContext.Task.ContinueWith((x) =>
                {
                    AppTrace.TraceSource.WriteNoise(
                        "Data.ReplicatedStateProvider.RecordReplicationContext",
                        "{0} concurrent operation {1} atomic group {2}",
                        this.ToString(),
                        replicationContext.Operation.SequenceNumber,
                        replicationContext.Operation.AtomicGroupId);
                    if (x.IsFaulted)
                    {
                        ReplicatedStateProvider.Assert(
                            null != x.Exception.InnerException && (this.IsNonRetryableReplicationException(x.Exception.InnerException) || x.Exception.InnerException is OperationCanceledException),
                            string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", x.Exception.InnerException.GetType()));
                        //
                        // Set replication task exception.
                        //
                        replicationContext.Exception = x.Exception.InnerException;
                    }
                    //
                    // Run continuations as they are completed.
                    //
                    this.CompleteMultipleReplicationContextsAsync(replicationContext).ContinueWith((y) =>
                    {
                    },
                    TaskContinuationOptions.ExecuteSynchronously);
                },
                TaskContinuationOptions.ExecuteSynchronously);
            }
        }

        /// <summary>
        /// Completes a single replication context/task. Reports load metrics as well.
        /// </summary>
        /// <param name="replicationContext">Replication context to complete.</param>
        /// <returns></returns>
        async Task CompleteSingleReplicationContextAsync(StateProviderReplicationContext replicationContext)
        {
            Exception exp = null;
            object applyResult = null;
            try
            {
                AppTrace.TraceSource.WriteNoise(
                    "Data.ReplicatedStateProvider.CompleteSingleReplicationContextAsync",
                    "{0} completing operation {1} atomic group {2}",
                    this.ToString(),
                    replicationContext.Operation.SequenceNumber,
                    replicationContext.Operation.AtomicGroupId);
                //
                // Even if the state provider is faulted, this call needs to happen
                // such that the state provider gets to release resources allocated
                // for these completed operations.
                //
                applyResult = await this.OnReplicationOperationAsync(
                    replicationContext.Operation,
                    replicationContext.Exception,
                    CancellationToken.None);
                //
                // Report load metrics.
                //
                this.CatchAllReportLoadMetrics();
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.ReplicatedStateProvider.CompleteSingleReplicationContextAsync", e, "{0}", this.ToString());
                //
                // Capture exception.
                //
                exp = e;
            }
            if (null != exp)
            {
                if (!this.IsFaulted())
                {
                    //
                    // The operation could not be applied. State provider is now faulted.
                    //
                    this.SetFaulted(exp is StateProviderNonRetryableException ? FaultType.Permanent : FaultType.Transient);
                }
                //
                // Complete the caller with exception.
                //
                replicationContext.CompletionSource.SetException(exp);
            }
            else
            {
                if (null != replicationContext.Exception)
                {
                    //
                    // Complete the caller with exception.
                    //
                    replicationContext.CompletionSource.SetException(exp);
                }
                else
                {
                    //
                    // Complete with result of apply.
                    //
                    replicationContext.CompletionSource.SetResult(applyResult);
                }
            }
        }

        /// <summary>
        /// Completes replication contexts/tasks.
        /// </summary>
        /// <param name="replicationContext">Replication context that was completed.</param>
        async Task CompleteMultipleReplicationContextsAsync(StateProviderReplicationContext replicationContext)
        {
            if (this.IsOperationCompletionOrdered)
            {
                AppTrace.TraceSource.WriteNoise(
                    "Data.ReplicatedStateProvider.CompleteMultipleReplicationContextsAsync",
                    "{0} in-order processing of operation {1} atomic group {2}",
                    this.ToString(),
                    replicationContext.Operation.SequenceNumber,
                    replicationContext.Operation.AtomicGroupId);
                //
                // Complete replication contexts in order.
                //
                while (true)
                {
                    List<StateProviderReplicationContext> completedReplicationContexts = null;
                    //
                    // Collect all completed replication contexts starting with the first.
                    //
                    this.inFlightReplicationTasksLock.EnterWriteLock();
                    if (0 != this.inFlightReplicationTasks.Count)
                    {
                        long minKey = this.inFlightReplicationTasks.Keys.Min();
                        if (StateProviderReplicationContextStatus.Completed == this.inFlightReplicationTasks[minKey].Status)
                        {
                            completedReplicationContexts = new List<StateProviderReplicationContext>();
                            IEnumerator<StateProviderReplicationContext> elements = this.inFlightReplicationTasks.Values.GetEnumerator();
                            while (elements.MoveNext())
                            {
                                if (StateProviderReplicationContextStatus.Completed == elements.Current.Status)
                                {
                                    //
                                    // Do not remove the contexts from the in-flight list, but mark them as in progress.
                                    //
                                    elements.Current.Status = StateProviderReplicationContextStatus.InApply;
                                    completedReplicationContexts.Add(elements.Current);
                                }
                                else
                                {
                                    break;
                                }
                            }
                            AppTrace.TraceSource.WriteNoise(
                                "Data.ReplicatedStateProvider.CompleteMultipleReplicationContextsAsync",
                                "{0} found {1} in-order operations completed while in-order processing of operation {2} atomic group {3}",
                                this.ToString(),
                                completedReplicationContexts.Count,
                                replicationContext.Operation.SequenceNumber,
                                replicationContext.Operation.AtomicGroupId);
                        }
                    }
                    this.inFlightReplicationTasksLock.ExitWriteLock();
                    //
                    // Call apply on the state provider in order for all completed replication contexts.
                    //
                    if (null != completedReplicationContexts && 0 != completedReplicationContexts.Count)
                    {
                        for (int x = 0; x < completedReplicationContexts.Count; x++)
                        {
                            long operationSequenceNumber = completedReplicationContexts[x].Operation.SequenceNumber;
                            //
                            // Wait until the state provider has applied the operation.
                            //
                            await this.CompleteSingleReplicationContextAsync(completedReplicationContexts[x]);
                        }
                        //
                        // Remove the in progress contexts from the in-flight list.
                        // Iterate from the beginning, since more replication contexts could have completed.
                        //
                        this.inFlightReplicationTasksLock.EnterWriteLock();
                        for (int x = 0; x < completedReplicationContexts.Count; x++)
                        {
                            this.inFlightReplicationTasks.Remove(completedReplicationContexts[x].Operation.SequenceNumber);
                        }
                        this.inFlightReplicationTasksLock.ExitWriteLock();
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else
            {
                AppTrace.TraceSource.WriteNoise(
                    "Data.ReplicatedStateProvider.CompleteMultipleReplicationContextsAsync",
                    "{0} concurrent processing of operation {1} atomic group {2}",
                    this.ToString(),
                    replicationContext.Operation.SequenceNumber,
                    replicationContext.Operation.AtomicGroupId);
                //
                // Complete replication contexts as they come in.
                //
                await this.CompleteSingleReplicationContextAsync(replicationContext);
            }
        }

        /// <summary>
        /// Traces a task exception.
        /// </summary>
        /// <param name="type">Type of the event.</param>
        /// <param name="ex">The exception to process.</param>
        /// <param name="format">Format string.</param>
        /// <param name="args">The trace arguments.</param>
        /// <returns></returns>
        internal static Exception ProcessException(string type, Exception ex, string format, params object[] args)
        {
            AppTrace.TraceSource.WriteExceptionAsError(type, ex, format, args);
            if (ex is AggregateException)
            {
                foreach (var e in ((AggregateException)ex).InnerExceptions)
                {
                    AppTrace.TraceSource.WriteExceptionAsError(type, e, format, args);
                }
                return ex.InnerException;
            }
            return ex;
        }

        /// <summary>
        /// Helper routine for replicating operations. Throws only non-retryable exceptions.
        /// </summary>
        /// <param name="operation">State provider operation.</param>
        /// <param name="cancellationToken">Cancellation token used to cancel this operation.</param>
        /// <exception cref="System.ArgumentNullException">Operation is null.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The replicator has been closed.</exception>
        /// <exception cref="System.Fabric.FabricTransientException">Replication transient exception.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <returns></returns>
        protected async Task<object> ReplicateOperationAsync(StateProviderOperation operation, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == operation)
            {
                throw new ArgumentNullException("operation");
            }
            long sequenceNumber = operation.SequenceNumber;
            //
            // Create replication context.
            //
            StateProviderReplicationContext replicationContext = new StateProviderReplicationContext();
            replicationContext.CompletionSource = new TaskCompletionSource<object>();
            replicationContext.Operation = operation;
            //
            // Exponential backoff starts at 64s and stops at 1s.
            //
            int exponentialBackoff = 32;
            bool isDone = false;
            while (!isDone)
            {
                //
                // Check if provider is faulted already.
                //
                if (this.IsFaulted())
                {
                    AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.ReplicateOperation", "{0} is faulted", this.ToString());
                    throw new StateProviderFaultedException();
                }
                //
                // Try/Retry replication.
                //
                if (this.IsOperationCompletionOrdered)
                {
                    this.inFlightReplicationTasksLock.EnterWriteLock();
                }
                try
                {
                    //
                    // Stop processing if cancellation was requested.
                    //
                    cancellationToken.ThrowIfCancellationRequested();
                    //
                    // Assume success. Issue replication operation.
                    //
                    isDone = true;
                    if (FabricReplicatorEx.InvalidAtomicGroupId == operation.AtomicGroupId)
                    {
                        replicationContext.Task = this.AtomicGroupStateReplicatorEx.ReplicateOperationAsync(
                            operation.ServiceMetadata,
                            operation.RedoInfo,
                            cancellationToken,
                            ref sequenceNumber);
                    }
                    else
                    {
                        replicationContext.Task = this.AtomicGroupStateReplicatorEx.ReplicateAtomicGroupOperationAsync(
                            operation.AtomicGroupId,
                            operation.ServiceMetadata,
                            operation.RedoInfo,
                            operation.UndoInfo,
                            cancellationToken,
                            ref sequenceNumber);
                    }
                    ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidSequenceNumber != sequenceNumber, "unexpected sequence number");
                    //
                    // Set replication sequence number.
                    //
                    operation.SequenceNumber = sequenceNumber;
                    //
                    // Replication context is now ready for processing.
                    //
                    this.RecordReplicationContext(replicationContext);
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.ReplicatedStateProvider.ReplicateOperation", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(
                        this.IsRetryableReplicationException(e) || this.IsNonRetryableReplicationException(e) || e is OperationCanceledException,
                        string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Retry replication if possible and requested.
                    //
                    if (this.IsRetryableReplicationException(e))
                    {
                        if (exponentialBackoff * 2 < ReplicatedStateProvider.ExponentialBackoffMax)
                        {
                            exponentialBackoff *= 2;
                        }
                        else
                        {
                            exponentialBackoff = ReplicatedStateProvider.ExponentialBackoffMax;
                        }
                        isDone = false;
                    }
                    else
                    {
                        throw e;
                    }
                }
                finally
                {
                    if (this.IsOperationCompletionOrdered)
                    {
                        this.inFlightReplicationTasksLock.ExitWriteLock();
                    }
                }
                //
                // Postpone the completion of this task in case of backoff.
                //
                if (!isDone)
                {
                    AppTrace.TraceSource.WriteWarning("Data.ReplicatedStateProvider.ReplicateOperation", "{0} retry {1}", this.ToString(), exponentialBackoff);
                    await Task.Delay(exponentialBackoff, cancellationToken);
                }
            }
            //
            // Wait for completion of the task.
            //
            await replicationContext.CompletionSource.Task;
            //
            // Return result of executing task.
            //
            return replicationContext.CompletionSource.Task.Result;
        }

        /// <summary>
        /// Helper routine for creating a new atomic group. Throws only non-retryable exceptions.
        /// </summary>
        /// <param name="cancellationToken">Cancellation token used to cancel this operation.</param>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The replicator has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <returns></returns>
        public async Task<long> CreateAtomicGroupAsync(CancellationToken cancellationToken)
        {
            long atomicGroupId = FabricReplicatorEx.InvalidAtomicGroupId;
            //
            // Exponential backoff starts at 64ms and stops at 1s.
            //
            int exponentialBackoff = 32;
            bool isDone = false;
            while (!isDone)
            {
                //
                // Check if provider is faulted already.
                //
                if (this.IsFaulted())
                {
                    AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.CreateAtomicGroupAsync", "{0} is faulted", this.ToString());
                    throw new StateProviderFaultedException();
                }
                try
                {
                    //
                    // Stop processing if cancellation was requested.
                    //
                    cancellationToken.ThrowIfCancellationRequested();
                    //
                    // Issue replication operation.
                    //
                    await this.AtomicGroupStateReplicatorEx.CreateAtomicGroupAsync(cancellationToken, ref atomicGroupId);
                    ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidAtomicGroupId != atomicGroupId, "unexpected atomic group id");
                    isDone = true;
                    //
                    // Trace success.
                    //
                    AppTrace.TraceSource.WriteNoise("Data.ReplicatedStateProvider.CreateAtomicGroupAsync", "{0} atomic group {1} created", this.ToString(), atomicGroupId);
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.ReplicatedStateProvider.CreateAtomicGroupAsync", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(
                        this.IsRetryableReplicationException(e) || this.IsNonRetryableReplicationException(e) || e is OperationCanceledException,
                        string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Retry replication if possible.
                    //
                    if (this.IsRetryableReplicationException(e))
                    {
                        if (exponentialBackoff * 2 < ReplicatedStateProvider.ExponentialBackoffMax)
                        {
                            exponentialBackoff *= 2;
                        }
                        else
                        {
                            exponentialBackoff = ReplicatedStateProvider.ExponentialBackoffMax;
                        }
                        isDone = false;
                    }
                    else
                    {
                        throw e;
                    }
                }
                //
                // Postpone the completion of this task in case of backoff.
                //
                if (!isDone)
                {
                    AppTrace.TraceSource.WriteWarning("Data.ReplicatedStateProvider.CreateAtomicGroupAsync", "{0} retry {1}", this.ToString(), exponentialBackoff);
                    await Task.Delay(exponentialBackoff, cancellationToken);
                }
            }
            return atomicGroupId;
        }

        /// <summary>
        /// Helper routine for terminating an existent atomic group. Throws only non-retryable exceptions.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group to commit.</param>
        /// <param name="cancellationToken">Cancellation token used to cancel this operation.</param>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The replicator has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <returns></returns>
        public async Task<long> CommitAtomicGroupAsync(long atomicGroupId, CancellationToken cancellationToken)
        {
            long sequenceNumber = FabricReplicatorEx.InvalidSequenceNumber;
            //
            // Exponential backoff starts at 64ms and stops at 1s.
            //
            int exponentialBackoff = 32;
            bool isDone = false;
            while (!isDone)
            {
                //
                // Check if provider is faulted already.
                //
                if (this.IsFaulted())
                {
                    AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.CommitAtomicGroupAsync", "{0} is faulted", this.ToString());
                    throw new StateProviderFaultedException();
                }
                try
                {
                    //
                    // Stop processing if cancellation was requested.
                    //
                    cancellationToken.ThrowIfCancellationRequested();
                    //
                    // Issue replication operation.
                    //
                    await this.AtomicGroupStateReplicatorEx.ReplicateAtomicGroupCommitAsync(atomicGroupId, cancellationToken, ref sequenceNumber);
                    ReplicatedStateProvider.Assert(FabricReplicatorEx.InvalidAtomicGroupId != atomicGroupId, "unexpected atomic group id");
                    isDone = true;
                    //
                    // Trace success.
                    //
                    AppTrace.TraceSource.WriteNoise(
                        "Data.ReplicatedStateProvider.CommitAtomicGroupAsync",
                        "{0} atomic group {1} committing at {2}",
                        this.ToString(),
                        atomicGroupId,
                        sequenceNumber);
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.ReplicatedStateProvider.CommitAtomicGroupAsync", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(
                        this.IsRetryableReplicationException(e) || this.IsNonRetryableReplicationException(e) || e is OperationCanceledException,
                        string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Retry replication if possible.
                    //
                    if (this.IsRetryableReplicationException(e))
                    {
                        if (exponentialBackoff * 2 < ReplicatedStateProvider.ExponentialBackoffMax)
                        {
                            exponentialBackoff *= 2;
                        }
                        else
                        {
                            exponentialBackoff = ReplicatedStateProvider.ExponentialBackoffMax;
                        }
                        isDone = false;
                    }
                    else
                    {
                        throw e;
                    }
                }
                //
                // Postpone the completion of this task in case of backoff.
                //
                if (!isDone)
                {
                    AppTrace.TraceSource.WriteWarning("Data.ReplicatedStateProvider.CommitAtomicGroupAsync", "{0} retry {1}", this.ToString(), exponentialBackoff);
                    await Task.Delay(exponentialBackoff, cancellationToken);
                }
            }
            return sequenceNumber;
        }

        /// <summary>
        /// Helper routine for rolling back an existent atomic group. Throws only non-retryable exceptions.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group to roll back.</param>
        /// <param name="cancellationToken">Cancellation token used to cancel this operation.</param>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The replicator has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <returns></returns>
        public async Task RollbackAtomicGroupAsync(long atomicGroupId, CancellationToken cancellationToken)
        {
            //
            // Exponential backoff starts at 64ms and stops at 1s.
            //
            int exponentialBackoff = 32;
            bool isDone = false;
            while (!isDone)
            {
                //
                // Check if provider is faulted already.
                //
                if (this.IsFaulted())
                {
                    AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.RollbackAtomicGroupAsync", "{0} is faulted", this.ToString());
                    throw new StateProviderFaultedException();
                }
                try
                {
                    //
                    // Stop processing if cancellation was requested.
                    //
                    cancellationToken.ThrowIfCancellationRequested();
                    //
                    // Issue replication operation.
                    //
                    await this.AtomicGroupStateReplicatorEx.ReplicateAtomicGroupRollbackAsync(atomicGroupId, cancellationToken);
                    isDone = true;
                    //
                    // Trace success.
                    //
                    AppTrace.TraceSource.WriteNoise(
                        "Data.ReplicatedStateProvider.RollbackAtomicGroupAsync",
                        "{0} atomic group {1} rolling back",
                        this.ToString(),
                        atomicGroupId);
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.ReplicatedStateProvider.RollbackAtomicGroupAsync", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(
                        this.IsRetryableReplicationException(e) || this.IsNonRetryableReplicationException(e) || e is OperationCanceledException,
                        string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Retry replication if possible.
                    //
                    if (this.IsRetryableReplicationException(e))
                    {
                        if (exponentialBackoff * 2 < ReplicatedStateProvider.ExponentialBackoffMax)
                        {
                            exponentialBackoff *= 2;
                        }
                        else
                        {
                            exponentialBackoff = ReplicatedStateProvider.ExponentialBackoffMax;
                        }
                        isDone = false;
                    }
                    else
                    {
                        throw e;
                    }
                }
                //
                // Postpone the completion of this task in case of backoff.
                //
                if (!isDone)
                {
                    AppTrace.TraceSource.WriteWarning("Data.ReplicatedStateProvider.RollbackAtomicGroupAsync", "{0} retry {1}", this.ToString(), exponentialBackoff);
                    await Task.Delay(exponentialBackoff, cancellationToken);
                }
            }
        }

        /// <summary>
        /// Reserves a sequence number from the replicator.
        /// </summary>
        /// <param name="cancellationToken">Cancellation token used to cancel this operation.</param>
        /// <returns></returns>
        protected async Task<long> ReserveSequenceNumberAsync(CancellationToken cancellationToken)
        {
            long sequenceNumber = FabricReplicatorEx.InvalidSequenceNumber;
            //
            // Exponential backoff starts at 64ms and stops at 1s.
            //
            int exponentialBackoff = 32;
            bool isDone = false;
            while (!isDone)
            {
                //
                // Check if provider is faulted already.
                //
                if (this.IsFaulted())
                {
                    AppTrace.TraceSource.WriteError("Data.ReplicatedStateProvider.ReserveSequenceNumberAsync", "{0} is faulted", this.ToString());
                    throw new StateProviderFaultedException();
                }
                try
                {
                    //
                    // Stop processing if cancellation was requested.
                    //
                    cancellationToken.ThrowIfCancellationRequested();
                    //
                    // Issue replication operation.
                    //
                    sequenceNumber = this.AtomicGroupStateReplicatorEx.ReserveSequenceNumber();
                    isDone = true;
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.ReplicatedStateProvider.ReserveSequenceNumberAsync", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(
                        this.IsRetryableReplicationException(e) || this.IsNonRetryableReplicationException(e) || e is OperationCanceledException,
                        string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Retry replication if possible.
                    //
                    if (this.IsRetryableReplicationException(e))
                    {
                        if (exponentialBackoff * 2 < ReplicatedStateProvider.ExponentialBackoffMax)
                        {
                            exponentialBackoff *= 2;
                        }
                        else
                        {
                            exponentialBackoff = ReplicatedStateProvider.ExponentialBackoffMax;
                        }
                        isDone = false;
                    }
                    else
                    {
                        throw e;
                    }
                }
                //
                // Postpone the completion of this task in case of backoff.
                //
                if (!isDone)
                {
                    AppTrace.TraceSource.WriteWarning("Data.ReplicatedStateProvider.ReserveSequenceNumberAsync", "{0} retry {1}", this.ToString(), exponentialBackoff);
                    await Task.Delay(exponentialBackoff, cancellationToken);
                }
            }
            return sequenceNumber;
        }

        /// <summary>
        /// Specifies if replication operations are completed in order on the primary and on the secondary. 
        /// </summary>
        public virtual bool IsOperationCompletionOrdered
        {
            get
            {
                return true;
            }
        }

        /// <summary>
        /// Retrieves the in-flight state for this state provider.
        /// </summary>
        /// <param name="sequenceNumber">Sequnce number of interest.</param>
        /// <returns></returns>
        protected IEnumerator<StateProviderOperation> GetInFlightReplicationOperations(long sequenceNumber)
        {
            if (ReplicaRole.Primary != this.ReplicaRole || !this.IsOperationCompletionOrdered)
            {
                throw new InvalidOperationException();
            }

            IEnumerator<StateProviderOperation> operations = null;
            this.inFlightReplicationTasksLock.EnterReadLock();
            try
            {
                long highestSequenceNumber = sequenceNumber >= this.lastCompletedSequenceNumberOnPrimary ? sequenceNumber : this.lastCompletedSequenceNumberOnPrimary;

                AppTrace.TraceSource.WriteNoise(
                    "Data.ReplicatedStateProvider.GetInFlightReplicationOperations", 
                    "{0} sequence number {1}", 
                    this.ToString(),
                    highestSequenceNumber);
                
                var x = from context in this.inFlightReplicationTasks.Values
                        where context.Operation.SequenceNumber <= highestSequenceNumber 
                        orderby context.Operation.SequenceNumber ascending 
                        select context.Operation;
                operations = x.GetEnumerator();
            }
            catch (Exception e)
            {
                ReplicatedStateProvider.ProcessException("Data.ReplicatedStateProvider.GetInFlightReplicationOperations", e, "{0}", this.ToString());
            }
            finally
            {
                this.inFlightReplicationTasksLock.ExitReadLock();
            }
            return operations;
        }

        #endregion

        #region IDisposable Members

        /// <summary>
        /// 
        /// </summary>
        bool disposed;

        /// <summary>
        /// Defines a method to release allocated unmanaged/managed resources.
        /// </summary>
        public void Dispose()
        {
            if (!this.disposed)
            {
                this.Dispose(true);
                this.disposed = true;
            }
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting managed/unmanaged resources.
        /// </summary>
        /// <param name="disposing">True, if managed resources only are freed.</param>
        protected virtual void Dispose(bool disposing)
        {
            this.inFlightReplicationTasksLock.Dispose();
        }

        #endregion

        #region Misc

        /// <summary>
        /// Checks for a condition; if the condition is false, it will attemp to break in the debugger 
        /// when the debugger is attached. If the debugger is not attached, it will fail fast.
        /// </summary>
        /// <param name="condition">The conditional expression to evaluate.</param>
        /// <param name="message">Debugging nmessage.</param>
        internal static void Assert(bool condition, string message)
        {
            if (!condition)
            {
                AppTrace.TraceSource.WriteError(
                    "Data.ReplicatedStateProvider.Assert",
                    string.Format(CultureInfo.InvariantCulture, "{0} {1}", message, Environment.StackTrace)); 
                if (Debugger.IsAttached)
                {
                    Debugger.Break();
                }
                else
                {
                    Environment.FailFast(message, new ApplicationException(Environment.StackTrace));
                }
            }
        }

        #endregion
    }
}
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
    using System.Fabric.Common.Tracing;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Replicator.Utils;

    /// <summary>
    /// This is for internal use only.
    /// Manages the lifetime of the replica
    /// </summary>
#pragma warning disable 1998
    internal abstract class StatefulServiceReplica : IStatefulServiceReplica
    {
        /// <summary>
        /// Transactional Replicator
        /// </summary>
        private readonly TransactionalReplicator transactionalReplicator;

        /// <summary>
        /// Service initialization parameters.
        /// </summary>
        private StatefulServiceInitializationParameters initializationParameters;

        /// <summary>
        /// Service Partition.
        /// </summary>
        private IStatefulServicePartition partition;

        /// <summary>
        /// Role of the replica.
        /// </summary>
        private ReplicaRole replicaRole = ReplicaRole.Unknown;

        /// <summary>
        /// State of the replica.
        /// </summary>
        private ReplicaState replicaState = ReplicaState.Closed;

        /// <summary>
        /// State replicator.
        /// </summary>
        private IStateReplicator stateReplicator;

        /// <summary>
        /// Tracer for the class.
        /// </summary>
        private Tracer tracer;

        /// <summary>
        /// Initializes a new instance of the StatefulServiceReplica class.
        /// </summary>
        protected StatefulServiceReplica()
        {
            this.transactionalReplicator = new TransactionalReplicator(this.OnDataLossAsync)
            {
                StateProviderFactory = this.CreateStateProvider
            };
        }

        /// <summary>
        /// Types of possible replica states
        /// </summary>
        private enum ReplicaState
        {
            /// <summary>
            /// Replica is in opened state.
            /// </summary>
            Opened = 0,

            /// <summary>
            /// Replica is in closed state.
            /// </summary>
            Closed = 1,

            /// <summary>
            /// Replica is in aborted state.
            /// </summary>
            Aborted = 2,
        }

        /// <summary>
        /// Gets the initialization parameters.
        /// </summary>
        protected StatefulServiceInitializationParameters InitializationParameters
        {
            get { return this.initializationParameters; }
        }

        /// <summary>
        /// Gets the partitionObject object.
        /// </summary>
        protected IStatefulServicePartition Partition
        {
            get { return this.partition; }
        }

        /// <summary>
        /// Gets the role of the replica.
        /// </summary>
        protected ReplicaRole ReplicaRole
        {
            get { return this.replicaRole; }
        }

        /// <summary>
        /// Gets the Transactional Replicator
        /// Returns NULL if property is accessed before OpenAsync
        /// </summary>
        protected TransactionalReplicator TransactionalReplicator
        {
            get { return this.transactionalReplicator; }
        }

        /// <summary>
        /// Abort is called when the replica is being forcibly closed
        /// </summary>
        void IStatefulServiceReplica.Abort()
        {
            try
            {
                FabricEvents.Events.Lifecycle(
                    this.tracer.Type,
                    "Abort: Aborting replica " + this.initializationParameters.ReplicaId);

                this.transactionalReplicator.Abort();

                this.OnAbort();
                this.replicaState = ReplicaState.Aborted;

                FabricEvents.Events.Lifecycle(
                    this.tracer.Type,
                    "Abort: Finished aborting replica " + this.initializationParameters.ReplicaId);
            }
            finally
            {
                TaskScheduler.UnobservedTaskException -= this.ProcessUnobservedTaskException;
            }
        }

        /// <summary>
        /// ChangeRole is called whenever the replica role is being changed.
        /// </summary>
        /// <param name="newRole">The new role of the replica.</param>
        /// <param name="cancellationToken">The cancellation token to cancel this async call.</param>
        /// <returns>Task representing the asynchronous operation.</returns>
        async Task<string> IStatefulServiceReplica.ChangeRoleAsync(
            ReplicaRole newRole,
            CancellationToken cancellationToken)
        {
            var currentRole = this.replicaRole;

            FabricEvents.Events.Lifecycle(
                this.tracer.Type,
                "ChangeRoleAsync: Changing role from: " + currentRole + " To: " + newRole + " Replica: " + this.initializationParameters.ReplicaId);

            await this.transactionalReplicator.ChangeRoleAsync(newRole, cancellationToken).ConfigureAwait(false);
            var address = await this.OnChangeRoleAsync(newRole, cancellationToken).ConfigureAwait(false);

            this.replicaRole = newRole;

            FabricEvents.Events.Lifecycle(
                this.tracer.Type,
                "ChangeRoleAsync: Finished changing role From: " + currentRole + " To: " + newRole + " Replica: " + this.initializationParameters.ReplicaId);

            return address;
        }

        /// <summary>
        /// CloseAsync is called when the replica is going to be closed
        /// </summary>
        /// <param name="cancellationToken">Cancellation token.</param>
        /// <returns>Task representing the asynchronous operation.</returns>
        async Task IStatefulServiceReplica.CloseAsync(CancellationToken cancellationToken)
        {
            try
            {
                FabricEvents.Events.Lifecycle(
                    this.tracer.Type,
                    "CloseAsync: Closing replica " + this.initializationParameters.ReplicaId);

                await this.transactionalReplicator.CloseAsync().ConfigureAwait(false);

                await this.OnCloseAsync(cancellationToken).ConfigureAwait(false);

                this.replicaState = ReplicaState.Closed;

                FabricEvents.Events.Lifecycle(
                    this.tracer.Type,
                    "CloseAsync: Finished closing replica " + this.initializationParameters.ReplicaId);
            }
            finally
            {
                TaskScheduler.UnobservedTaskException -= this.ProcessUnobservedTaskException;
            }
        }

        /// <summary>
        /// This is the very first method called by the framework
        /// Provides all the initialization information
        /// No complex processing should be done here
        /// All heavy lifting should be done in Open
        /// </summary>
        /// <param name="initParameters">Initialization parameters.</param>
        void IStatefulServiceReplica.Initialize(StatefulServiceInitializationParameters initParameters)
        {
            this.tracer = new Tracer(initParameters.PartitionId, initParameters.ReplicaId);

            FabricEvents.Events.Lifecycle(
                this.tracer.Type,
                "Initialize" + " Name: " + initParameters.ServiceTypeName + " Uri: " + initParameters.ServiceName + " PartitionId: " +
                initParameters.PartitionId + " ReplicaId: " + initParameters.ReplicaId);

            this.initializationParameters = initParameters;
        }

        /// <summary>
        /// OpenAsync is called when the replica is going to be actually used
        /// </summary>
        /// <param name="openMode">Open mode.</param>
        /// <param name="partitionObject">Service partition</param>
        /// <param name="cancellationToken">Cancellation token.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        async Task<IReplicator> IStatefulServiceReplica.OpenAsync(
            ReplicaOpenMode openMode,
            IStatefulServicePartition partitionObject,
            CancellationToken cancellationToken)
        {
            FabricEvents.Events.Lifecycle(
                this.tracer.Type,
                "OpenAsync" + " openMode: " + openMode + " replica: " + this.initializationParameters.ReplicaId);

            TaskScheduler.UnobservedTaskException += this.ProcessUnobservedTaskException;

            // Store the partitionObject
            this.partition = partitionObject;

            var statefulServiceContext = new StatefulServiceContext(
                FabricRuntime.GetNodeContext(),
                this.initializationParameters.CodePackageActivationContext,
                this.initializationParameters.ServiceTypeName,
                this.initializationParameters.ServiceName,
                this.initializationParameters.InitializationData,
                this.initializationParameters.PartitionId,
                this.initializationParameters.ReplicaId);

            this.transactionalReplicator.Initialize(statefulServiceContext, partitionObject);

            FabricEvents.Events.Lifecycle(this.tracer.Type, "OpenAsync: StateManager initialized");

            // create the replicator
            // The Windows Fabric Replicator is used to actually replicate the data
            // The ReplicatorSettings are used for configuring the replicator - here we ask for them from the derived class
            // When using service groups Replicator Settings are described in the Settings.xml inside the configuration package.
            // So when the service is part of a service group it should use null as its Replicator Setting.
            FabricReplicator replicator;
            ReliableStateManagerReplicatorSettings replicatorSettings = null;
            var onOpenInvoked = false;

            try
            {
                if (this.Partition is IServiceGroupPartition)
                {
                    replicatorSettings = new ReliableStateManagerReplicatorSettings();
                    ReliableStateManagerReplicatorSettingsUtil.LoadDefaultsIfNotSet(ref replicatorSettings);
                    replicator = this.partition.CreateReplicator(
                        this.transactionalReplicator,
                        ReliableStateManagerReplicatorSettingsUtil.ToReplicatorSettings(replicatorSettings));
                }
                else
                {
                    replicatorSettings = await this.OnOpenAsync(cancellationToken).ConfigureAwait(false);
                    onOpenInvoked = true;

                    if (replicatorSettings == null)
                    {
                        replicatorSettings = new ReliableStateManagerReplicatorSettings();
                    }

                    ReliableStateManagerReplicatorSettingsUtil.LoadDefaultsIfNotSet(ref replicatorSettings);
                    replicator = this.partition.CreateReplicator(
                        this.transactionalReplicator,
                        ReliableStateManagerReplicatorSettingsUtil.ToReplicatorSettings(replicatorSettings));
                }

                ServiceReplicaUtils.SetupLoggerPath(ref replicatorSettings, ref this.initializationParameters);
                var transactionalReplicatorSettings = new TransactionalReplicatorSettings
                {
                    PublicSettings = replicatorSettings
                };

                ReliableStateManagerReplicatorSettingsUtil.LoadInternalSettingsDefault(ref transactionalReplicatorSettings);

                this.stateReplicator = replicator.StateReplicator;
                this.transactionalReplicator.LoggingReplicator.FabricReplicator = this.stateReplicator;
                this.transactionalReplicator.LoggingReplicator.Tracer = this.tracer;

                // Starting local recovery
                await this.transactionalReplicator.OpenAsync(openMode, transactionalReplicatorSettings).ConfigureAwait(false);

                // Change state
                this.replicaState = ReplicaState.Opened;

                FabricEvents.Events.Lifecycle(
                    this.tracer.Type,
                    "OpenAsync: Finished opening replica " + this.initializationParameters.ReplicaId + " ReplicatorAddress: " + replicatorSettings.ReplicatorAddress
                    + " ReplicatorListenAddress: " + replicatorSettings.ReplicatorListenAddress
                    + " ReplicatorPublishAddress: " + replicatorSettings.ReplicatorPublishAddress);

                return replicator;
            }
            catch (Exception e)
            {
                int innerHResult = 0;
                var flattenedException = Utility.FlattenException(e, out innerHResult);

                FabricEvents.Events.Exception_TStatefulServiceReplica(
                    this.tracer.Type,
                    "OpenAsync",
                    flattenedException.GetType().ToString(),
                    flattenedException.Message,
                    flattenedException.HResult != 0 ? flattenedException.HResult : innerHResult,
                    flattenedException.StackTrace);
            
                if (onOpenInvoked)
                {
                    this.OnAbort();
                }

                TaskScheduler.UnobservedTaskException -= this.ProcessUnobservedTaskException;
                throw;
            }
        }

        /// <summary>
        /// Creates state provider.
        /// </summary>
        /// <param name="name">Name of the state provider.</param>
        /// <param name="type">State provider type.</param>
        /// <returns>State provider.</returns>
        internal virtual IStateProvider2 CreateStateProvider(Uri name, Type type)
        {
            Utility.Assert(
                type != null,
                "CreateStateProvider: type is null for state provider name {0}",
                name.OriginalString);
            return (IStateProvider2) Activator.CreateInstance(type);
        }

        /// <summary>
        /// Called in Abort 
        /// The derived class should cleanup its listener and any other cleanup
        /// </summary>
        protected virtual void OnAbort()
        {
        }

        /// <summary>
        /// Replica Change role.
        /// </summary>
        /// <param name="newRole">New role.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        protected virtual async Task<string> OnChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            return string.Empty;
        }

        /// <summary>
        /// Called in Close 
        /// The derived class should cleanup its listener and any other cleanup
        /// </summary>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// Task that represents the asynchronous operation.
        /// Task.Result is the address the service is listening on.
        /// </returns>
        protected virtual async Task OnCloseAsync(CancellationToken cancellationToken)
        {
        }

        /// <summary>
        /// Called when Windows Fabric suspects a dataloss. This is a notification.
        /// </summary>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// Task that represents the asynchronous operation.
        /// </returns>
        protected virtual async Task<bool> OnDataLossAsync(CancellationToken cancellationToken)
        {
            return false;
        }

        /// <summary>
        /// Called in OpenAsync
        /// The derived class should create it's listener over here and any other initialization
        /// </summary>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task result contains the transactional replicator settings.</returns>
        protected virtual async Task<ReliableStateManagerReplicatorSettings> OnOpenAsync(CancellationToken cancellationToken)
        {
            return null;
        }

        /// <summary>
        /// Updates the replicator settings. Security credentials is the only setting that can be changed using this API
        /// </summary>
        /// <param name="updatedSettings"> New settings to be updated</param>
        protected void UpdateReplicatorSettings(ReliableStateManagerReplicatorSettings updatedSettings)
        {
            if (this.replicaState != ReplicaState.Opened)
            {
                throw new FabricException(FabricErrorCode.NotReady);
            }

            this.transactionalReplicator.LoggingReplicator.UpdateReplicatorSettings(updatedSettings);
        }

        /// <summary>
        /// Handler for the unobserved exceptions.
        /// </summary>
        /// <param name="sender">The sender object.</param>
        /// <param name="e">Event args.</param>
        private void ProcessUnobservedTaskException(object sender, UnobservedTaskExceptionEventArgs e)
        {
            // Do not call flatten exception here as it mangles the stack trace
            if (e.Exception is AggregateException)
            {
                foreach (var flattenedException in e.Exception.InnerExceptions)
                {
                    FabricEvents.Events.Exception_TStatefulServiceReplica(
                        this.tracer.Type,
                        "ProcessUnobservedTaskException_AggregateExceptions",
                        flattenedException.GetType().ToString(),
                        flattenedException.Message,
                        flattenedException.HResult,
                        flattenedException.StackTrace);
                }
            }
            else
            {
                FabricEvents.Events.Exception_TStatefulServiceReplica(
                    this.tracer.Type,
                    "ProcessUnobservedTaskException",
                    e.Exception.GetType().ToString(),
                    e.Exception.Message,
                    e.Exception.HResult,
                    e.Exception.StackTrace);
            }

#if DEBUG
            Utility.ProcessUnexpectedException(
                "StatefulServiceReplica",
                this.tracer,
                "ProcessUnobservedTaskException",
                e.Exception);
#endif
        }
    }
}
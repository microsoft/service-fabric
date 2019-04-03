// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Linq;
    using System.Net;
    using System.Net.NetworkInformation;
    using System.Threading;
    using System.Threading.Tasks;

    public class ServiceReplica : IStatefulServiceReplica, IStateProvider
    {
        private static readonly TraceType TraceType = new TraceType("Replica");

        private readonly object replicaRoleLock;
        private readonly IInfrastructureAgentWrapper infrastructureServiceAgent;
        private readonly IInfrastructureCoordinator coordinator;
        private readonly ClusterEndpointSecuritySettingsChangeNotifier clusterEndpointSecuritySettingsChangeNotifier;
        private readonly string replicatorAddress;
        private readonly bool useClusterSecuritySettingsForReplicator;
        private readonly IConfigSection configSection;

        private StatefulServiceInitializationParameters initializationParameters;
        private string serviceId;
        private string serviceEndpoint;
        private FabricReplicator replicator;
        private ReplicaRole replicaRole;
        private IStatefulServicePartition partition;

        private int primaryEpoch;
        private Task coordinatorRunTask;
        private CancellationTokenSource coordinatorCancelTokenSource;

        internal ServiceReplica(
            IInfrastructureAgentWrapper infrastructureServiceAgent,
            IInfrastructureCoordinator coordinator,
            string replicatorAddress,
            bool useClusterSecuritySettingsForReplicator,
            IConfigSection configSection)
        {
            if (infrastructureServiceAgent == null)
                throw new ArgumentNullException("infrastructureServiceAgent");
            if (coordinator == null)
                throw new ArgumentNullException("coordinator");
            if (configSection == null)
                throw new ArgumentNullException("configSection");

            this.infrastructureServiceAgent = infrastructureServiceAgent;
            this.coordinator = coordinator;
            this.replicatorAddress = replicatorAddress;
            this.useClusterSecuritySettingsForReplicator = useClusterSecuritySettingsForReplicator;
            this.configSection = configSection;

            this.replicaRoleLock = new object();
            this.replicaRole = ReplicaRole.Unknown;

            TraceType.WriteInfo("Created replica: coordinator = {0}, replicator address = {1}, useClusterSecuritySettingsForReplicator = {2}",
                this.coordinator.GetType().FullName,
                this.replicatorAddress ?? "<null>",
                this.useClusterSecuritySettingsForReplicator);

            if (this.useClusterSecuritySettingsForReplicator)
            {
                this.clusterEndpointSecuritySettingsChangeNotifier = new ClusterEndpointSecuritySettingsChangeNotifier(this.replicatorAddress, this.UpdateReplicatorSettings);
            }
        }

        public void Initialize(StatefulServiceInitializationParameters initializationParameters)
        {
            if (initializationParameters == null) { throw new ArgumentNullException("initializationParameters"); }

            this.initializationParameters = initializationParameters;

            this.serviceId = string.Format(
                CultureInfo.InvariantCulture,
                "[{0}+{1}]",
                initializationParameters.PartitionId,
                initializationParameters.ReplicaId);
        }

        #region IStatefulServiceReplica API

        ////
        // IStatefulServiceReplica
        ////
        public Task<IReplicator> OpenAsync(ReplicaOpenMode openMode, IStatefulServicePartition partition, CancellationToken cancellationToken)
        {
            if (partition == null) { throw new ArgumentNullException("partition"); }

            Trace.WriteInfo(TraceType, "OpenAsync: {0}, OpenMode={1}", this.serviceId, openMode);

            ReplicatorSettings replicatorSettings = this.CreateReplicatorSettings(this.replicatorAddress);
            this.replicator = partition.CreateReplicator(this, replicatorSettings);
            this.partition = partition;

            this.serviceEndpoint = this.infrastructureServiceAgent.RegisterInfrastructureService(
                this.initializationParameters.PartitionId,
                this.initializationParameters.ReplicaId,
                this.coordinator);

            return Utility.CreateCompletedTask<IReplicator>(this.replicator);
        }

        public Task<string> ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            Trace.ConsoleWriteLine(TraceType, "ChangeRole: {0}, {1}->{2}", this.serviceId, this.replicaRole, newRole);

            lock (this.replicaRoleLock)
            {
                this.replicaRole = newRole;
            }

            if (newRole == ReplicaRole.Primary)
            {
                this.StartCoordinator();
                return Utility.CreateCompletedTask<string>(FinishChangeRole(null));
            }
            else
            {
                Task stopTask = this.StopCoordinatorAsync();
                return stopTask.ContinueWith<string>(this.FinishChangeRole);
            }
        }

        public Task CloseAsync(CancellationToken cancellationToken)
        {
            Trace.WriteInfo(TraceType, "Close: {0}", this.serviceId);

            Task stopTask = this.StopCoordinatorAsync();
            return stopTask.ContinueWith(this.FinishClose);
        }

        public void Abort()
        {
            Trace.WriteInfo(TraceType, "Abort: {0}", this.serviceId);

            // TODO: this.StopCoordinatorAsync?
        }

        #endregion IStatefulServiceReplica API

        #region IStateProvider API

        ////
        // IStateProvider
        ////

        public IOperationDataStream GetCopyContext()
        {
            return null;
        }

        public IOperationDataStream GetCopyState(long sequenceNumber, IOperationDataStream copyContext)
        {
            return null;
        }

        public long GetLastCommittedSequenceNumber()
        {
            return 0;
        }

        public Task<bool> OnDataLossAsync(CancellationToken cancellationToken)
        {
            return Utility.CreateCompletedTask<bool>(false);
        }

        public Task UpdateEpochAsync(Epoch epoch, long previousEpochLastSequenceNumber, CancellationToken cancellationToken)
        {
            int primaryEpoch = (int)(epoch.ConfigurationNumber >> 32);

            Trace.WriteInfo(TraceType, "UpdateEpochAsync: DLN = {0}, CN = {1} (0x{1:X}), primary epoch = {2} (0x{2:X})",
                epoch.DataLossNumber,
                epoch.ConfigurationNumber,
                primaryEpoch);

            this.primaryEpoch = primaryEpoch;

            return Utility.CreateCompletedTask<object>(null);
        }

        #endregion IStateProvider API

        #region Replication Pump

        ////
        // Stateful Service logic:
        //
        // Stateful service feature is only used for primary election so there
        // is no actual state to process. Just keep the replicator queues drained.
        ////

        private void SchedulePumpCopyOperation()
        {
            bool shouldStart = false;
            lock (this.replicaRoleLock)
            {
                Trace.WriteInfo(TraceType, "SchedulePumpCopyOperation {0}", this.replicaRole);
                if (this.replicaRole == ReplicaRole.IdleSecondary)
                {
                    shouldStart = true;
                }
            }

            if (shouldStart)
            {
                Task.Factory.StartNew(() => this.PumpCopyOperation());
            }
        }

        private void SchedulePumpReplicationOperation()
        {
            bool shouldStart = false;
            lock (this.replicaRoleLock)
            {
                Trace.WriteInfo(TraceType, "SchedulePumpReplicationOperation {0}", this.replicaRole);
                if (this.replicaRole == ReplicaRole.ActiveSecondary)
                {
                    shouldStart = true;
                }
            }

            if (shouldStart)
            {
                Task.Factory.StartNew(() => this.PumpReplicationOperation());
            }
        }

        private void PumpCopyOperation()
        {
            Trace.WriteInfo(TraceType, "PumpCopyOperation started");
            try
            {
                IOperationStream stream = this.replicator.StateReplicator.GetCopyStream();
                var task = stream.GetOperationAsync(CancellationToken.None);

                IOperation operation = task.Result;
                if (operation == null)
                {
                    Trace.WriteInfo(TraceType, "Reached end of copy stream");
                }
                else
                {
                    // Don't expect any copy operations
                    this.SchedulePumpCopyOperation();
                }
            }
            catch (Exception e)
            {
                Trace.WriteWarning(TraceType, "PumpCopyOperation: {0}", e);
            }
        }

        private void PumpReplicationOperation()
        {
            Trace.WriteInfo(TraceType, "PumpReplicationOperation started");
            try
            {
                IOperationStream stream = this.replicator.StateReplicator.GetReplicationStream();
                var task = stream.GetOperationAsync(CancellationToken.None);

                IOperation operation = task.Result;
                if (operation == null)
                {
                    Trace.WriteInfo(TraceType, "Reached end of replication stream");
                }
                else
                {
                    // Don't expect any replication operations
                    this.SchedulePumpReplicationOperation();
                }
            }
            catch (Exception e)
            {
                Trace.WriteWarning(TraceType, "PumpReplicationOperation: {0}", e);
            }
        }

        #endregion Replication Pump

        private void StartCoordinator()
        {
            Trace.WriteInfo(TraceType, "Starting coordinator");

            Debug.Assert(this.coordinatorCancelTokenSource == null);
            Debug.Assert(this.coordinatorRunTask == null);

            this.coordinatorCancelTokenSource = new CancellationTokenSource();
            this.coordinatorRunTask = this.RunCoordinatorWrapperAsync();
        }

        private Task RunCoordinatorWrapperAsync()
        {
            if (this.configSection.ReadConfigValue("WrapRunAsync", true))
            {
                TaskCompletionSource<object> tcs = new TaskCompletionSource<object>();
                ThreadPool.QueueUserWorkItem(this.RunCoordinatorWrapperAsyncThreadProc, tcs);
                return tcs.Task;
            }
            else
            {
                return this.coordinator.RunAsync(this.primaryEpoch, this.coordinatorCancelTokenSource.Token);
            }
        }

        private async void RunCoordinatorWrapperAsyncThreadProc(object state)
        {
            var tcs = (TaskCompletionSource<object>)state;
            CancellationToken token = this.coordinatorCancelTokenSource.Token;

            TraceType.WriteInfo("Invoking coordinator RunAsync");
            try
            {
                await this.coordinator.RunAsync(this.primaryEpoch, token);
            }
            catch (Exception e)
            {
                if (!(token.IsCancellationRequested && (e is OperationCanceledException)))
                {
                    TraceType.WriteError("Unhandled exception from coordinator RunAsync: {0}", e);

                    if (this.configSection.ReadConfigValue("CrashOnRunAsyncUnhandledException", true))
                    {
                        // Allow the process to crash. If config is false, then this will fall through
                        // to the "unexpected completion" cases below, and can call ReportFault instead.
                        throw;
                    }
                }
            }

            if (!token.IsCancellationRequested)
            {
                if (this.configSection.ReadConfigValue("CrashOnRunAsyncUnexpectedCompletion", true))
                {
                    TraceType.WriteError("Coordinator RunAsync completed unexpectedly; exiting process");
                    Environment.FailFast("Coordinator RunAsync completed unexpectedly");
                }

                if (this.configSection.ReadConfigValue("ReportFaultOnRunAsyncUnexpectedCompletion", true))
                {
                    TraceType.WriteError("Coordinator RunAsync completed unexpectedly; reporting transient fault");
                    this.partition.ReportFault(FaultType.Transient);
                }
            }

            TraceType.WriteInfo("Coordinator RunAsync completed");
            tcs.SetResult(null);
        }

        private Task StopCoordinatorAsync()
        {
            if (this.coordinatorCancelTokenSource == null)
            {
                return Utility.CreateCompletedTask<object>(null);
            }

            Trace.WriteInfo(TraceType, "Stopping coordinator");
            this.coordinatorCancelTokenSource.Cancel();

            return this.coordinatorRunTask.ContinueWith(t =>
            {
                Trace.WriteNoise(TraceType, "Clearing coordinator run task");
                this.coordinatorRunTask = null;
                this.coordinatorCancelTokenSource = null;
            });
        }

        private string FinishChangeRole(Task completedRunTask)
        {
            Trace.WriteInfo(TraceType, "Finishing ChangeRole");

            if ((this.replicaRole == ReplicaRole.IdleSecondary || this.replicaRole == ReplicaRole.ActiveSecondary) &&
                (this.replicator != null)) // allows for unit testing
            {
                this.SchedulePumpCopyOperation();
                this.SchedulePumpReplicationOperation();
            }

            Trace.WriteInfo(TraceType, "Finished ChangeRole");
            return this.serviceEndpoint;
        }

        private void FinishClose(Task completedRunTask)
        {
            Trace.WriteInfo(TraceType, "Finishing Close");

            this.infrastructureServiceAgent.UnregisterInfrastructureService(
                this.initializationParameters.PartitionId,
                this.initializationParameters.ReplicaId);

            Trace.WriteInfo(TraceType, "Finished Close");
        }

        private ReplicatorSettings CreateReplicatorSettings(string endpoint)
        {
            ReplicatorSettings replicatorSettings = new ReplicatorSettings();

            if (endpoint != null)
            {
                TraceType.WriteInfo(
                    "Replicator address = {0}, useClusterSecuritySettingsForReplicator = {1}",
                    endpoint,
                    this.useClusterSecuritySettingsForReplicator);

                replicatorSettings.ReplicatorAddress = endpoint;

                if (this.useClusterSecuritySettingsForReplicator)
                {
                    replicatorSettings.SecurityCredentials = SecurityCredentials.LoadClusterSettings();
                }
            }

            return replicatorSettings;
        }

        private void UpdateReplicatorSettings(string endpoint)
        {
            if (this.replicator != null)
            {
                ReplicatorSettings replicatorSettings = this.CreateReplicatorSettings(endpoint);
                this.replicator.StateReplicator.UpdateReplicatorSettings(replicatorSettings);
            }
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMonSvc
{
    using System;
    using System.Fabric;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Represents a replica that does not have any state to replicate. 
    /// This class is intended to be used with PrimaryElectionService, which behaves 
    /// as a stateful service to get leader election but does not actually have any state to replicate.
    /// </summary>
    internal class NullStateProviderReplica : IStateProviderReplica, IStateProvider
    {
        private static readonly Task CompletedTask = Task.FromResult(false);
        private readonly TraceWriterWrapper trace;
        private FabricReplicator replicator;
        private IStatefulServicePartition partition;
        private StatefulServiceInitializationParameters initializationParameters;
        private ReplicaRole currentRole;

        public NullStateProviderReplica(TraceWriterWrapper traceWriter)
        {
            this.trace = Guard.IsNotNull(traceWriter, nameof(traceWriter));
        }

        Func<CancellationToken, Task<bool>> IStateProviderReplica.OnDataLossAsync
        {
            set
            {
            }
        }

        public void Initialize(StatefulServiceInitializationParameters initializationParameters)
        {
            this.currentRole = ReplicaRole.Unknown;
            this.initializationParameters = initializationParameters;

            this.TraceInfo("IStatefulServiceReplica::Initialize invoked for service {0}.", this.initializationParameters.ServiceName);
        }

        public Task<IReplicator> OpenAsync(ReplicaOpenMode openMode, IStatefulServicePartition partition, CancellationToken cancellationToken)
        {
            this.TraceInfo("IStatefulServiceReplica::OpenAsync invoked. Open mode: {0}.", openMode);

            this.partition = partition;
            ReplicatorSettings replicatorSettings = this.CreateReplicatorSettings();
            this.replicator = partition.CreateReplicator(this, replicatorSettings);

            this.TraceInfo("IStatefulServiceReplica::OpenAsync completed.");

            return Task.FromResult(this.replicator as IReplicator);
        }

        Task IStateProviderReplica.ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            this.TraceInfo("IStatefulServiceReplica::ChangeRoleAsync invoked. Current role: {0}, new role: {1}.", this.currentRole, newRole);

            // It is necessary to pump the copy stream in order to 
            // enable the system to complete the role change
            if (newRole == ReplicaRole.IdleSecondary)
            {
                this.StartSecondaryCopyAndReplicationPump();
            }
            else if (newRole == ReplicaRole.ActiveSecondary
                && this.currentRole == ReplicaRole.Primary)
            {
                // Start replication pump only when if we are changing from primary to activeSecondary
                this.StartSecondaryReplicationPump();
            }

            this.currentRole = newRole;
            this.TraceInfo("IStatefulServiceReplica::ChangeRoleAsync completed. New role: {0}.", this.currentRole);
            return CompletedTask;
        }

        public Task CloseAsync(CancellationToken cancellationToken)
        {
            this.TraceInfo("IStatefulServiceReplica::CloseAsync invoked. CloseAsync will complete with no-op.");
            return CompletedTask;
        }

        public void Abort()
        {
            this.TraceInfo("IStatefulServiceReplica::Abort invoked. Abort will complete with no-op.");
        }

        public Task BackupAsync(Func<BackupInfo, CancellationToken, Task<bool>> backupCallback)
        {
            return CompletedTask;
        }

        public Task BackupAsync(BackupOption option, TimeSpan timeout, CancellationToken cancellationToken, Func<BackupInfo, CancellationToken, Task<bool>> backupCallback)
        {
            return CompletedTask;
        }

        public Task RestoreAsync(string backupFolderPath)
        {
            return CompletedTask;
        }

        public Task RestoreAsync(string backupFolderPath, RestorePolicy restorePolicy, CancellationToken cancellationToken)
        {
            return CompletedTask;
        }

        public IOperationDataStream GetCopyContext()
        {
            this.TraceInfo("Executing GetCopyContext.");
            return NullOperationDataStream.Instance;
        }

        public IOperationDataStream GetCopyState(long upToSequenceNumber, IOperationDataStream copyContext)
        {
            this.TraceInfo("Executing GetCopyState.");
            return NullOperationDataStream.Instance;
        }

        public long GetLastCommittedSequenceNumber()
        {
            return 0;
        }

        public Task<bool> OnDataLossAsync(CancellationToken cancellationToken)
        {
            return Task.FromResult(false);
        }

        public Task UpdateEpochAsync(Epoch epoch, long previousEpochLastSequenceNumber, CancellationToken cancellationToken)
        {
            return CompletedTask;
        }

        private ReplicatorSettings CreateReplicatorSettings()
        {
            // Although we don't replicate any state, we need to keep the copy stream
            // and the replication stream drained at all times. This is necessary in
            // order to unblock role changes. For these reason, we specify a valid
            // replicator address in the settings.
            ReplicatorSettings replicatorSettings = new ReplicatorSettings();
            string ipaddressOrFqdn = FabricRuntime.GetNodeContext().IPAddressOrFQDN;
            int replicatorPort = FabricRuntime.GetActivationContext().GetEndpoint("ReplicatorEndpoint").Port;
            replicatorSettings.ReplicatorAddress = string.Format(
                                                        CultureInfo.InvariantCulture,
                                                        "{0}:{1}",
                                                        ipaddressOrFqdn,
                                                        replicatorPort);

            this.TraceInfo("Replicator address is {0}", replicatorSettings.ReplicatorAddress);
            return replicatorSettings;
        }

        private void StartSecondaryCopyAndReplicationPump()
        {
            Task.Run(async () => await this.PumpOperationAsync(true));
        }

        private void StartSecondaryReplicationPump()
        {
            Task.Run(async () => await this.PumpOperationAsync(false));
        }

        private async Task PumpOperationAsync(bool isCopy)
        {
            try
            {
                this.TraceInfo("PumpOperationAsync: Pump {0} stream started", isCopy ? "copy" : "replication");

                IOperationStream stream = isCopy ?
                    this.replicator.StateReplicator2.GetCopyStream() :
                    this.replicator.StateReplicator2.GetReplicationStream();

                this.TraceInfo("PumpOperationAsync: obtained IOperationStream instance {0}", stream.GetType().Name);

                var operation = await stream.GetOperationAsync(CancellationToken.None);

                if (operation == null)
                {
                    // Since we are not replicating any data, we always expect null.
                    this.TraceInfo("PumpOperationAsync: Reached end of {0} stream", isCopy ? "copy" : "replication");

                    if (isCopy)
                    {
                        this.StartSecondaryReplicationPump();
                    }
                }
                else
                {
                    // We don't expect any replication operations. It is an error if we get one.
                    string message = string.Format(
                        "PumpOperationAsync: An operation was unexpectedly received while pumping {0} stream.",
                        isCopy ? "copy" : "replication") + this.PartitionAndReplicaId();

                    this.trace.WriteError(message);
                    this.partition.ReportFault(FaultType.Transient);
                }
            }
            catch (Exception ex)
            {
                // This method is not awaited by the caller. 
                // The exception on this thread is not supposed to bubble up the chain. 
                // Hence logging and eating the exception.
                this.trace.Exception(ex);
            }
        }

        private void TraceInfo(string format, params object[] args)
        {
            this.trace.WriteInfo(string.Format(format, args) + this.PartitionAndReplicaId());
        }

        private string PartitionAndReplicaId()
        {
            return string.Format(
                " Partition {0}, replica {1}.",
                this.initializationParameters.PartitionId,
                this.initializationParameters.ReplicaId);
        }

        /// <summary>
        /// Empty implementation of IOperationDataStream. Intended to be used with NullStateProviderReplica.
        /// </summary>
        private class NullOperationDataStream : IOperationDataStream
        {
            public static readonly NullOperationDataStream Instance = new NullOperationDataStream();

            public Task<OperationData> GetNextAsync(CancellationToken cancellationToken)
            {
                return Task.FromResult<OperationData>(null);
            }
        }
    }
}
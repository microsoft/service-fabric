// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common.Tracing;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;

    using Backup;

    internal class SecondaryDrainManager
    {
        private ReplicatedLogManager replicatedLogManager;
        private CheckpointManager checkpointManager;
        private TransactionManager transactionManager;
        private IStateReplicator fabricReplicator;
        private IBackupManager backupManager;

        private TaskCompletionSource<object> copyStreamDrainCompletionTcs;
        private TaskCompletionSource<object> replicationStreamDrainCompletionTcs;
        private RoleContextDrainState roleContextDrainState;
        private OperationProcessor recordsProcessor;
        private ITracer tracer;
        private LogicalSequenceNumber copiedUptoLsn;
        private IStateManager stateManager;
        private TransactionalReplicatorSettings replicatorSettings;
        private RecoveryManager recoveryManager;
        private RecoveredOrCopiedCheckpointLsn recoveredOrCopiedCheckpointLsn;

        public SecondaryDrainManager()
        {
            this.Reuse();
        }

        public LogicalSequenceNumber ReadConsistentAfterLsn
        {
            get; private set;
        }

        public void Open(
            RecoveredOrCopiedCheckpointLsn recoveredOrCopiedCheckpointLsn,
            RoleContextDrainState roleContextDrainState,
            OperationProcessor recordsProcessor,
            IStateReplicator fabricReplicator,
            IBackupManager backupManager,
            CheckpointManager checkpointManager,
            TransactionManager transactionManager,
            ReplicatedLogManager replicatedLogManager,
            IStateManager stateManager,
            TransactionalReplicatorSettings replicatorSettings,
            RecoveryManager recoveryManager,
            ITracer tracer)
        {
            this.recoveredOrCopiedCheckpointLsn = recoveredOrCopiedCheckpointLsn;
            this.recoveryManager = recoveryManager;
            this.replicatorSettings = replicatorSettings;
            this.stateManager = stateManager;
            this.tracer = tracer;
            this.roleContextDrainState = roleContextDrainState;
            this.recordsProcessor = recordsProcessor;
            this.fabricReplicator = fabricReplicator;
            this.backupManager = backupManager;
            this.transactionManager = transactionManager;
            this.checkpointManager = checkpointManager;
            this.replicatedLogManager = replicatedLogManager;
        }

        public void Reuse()
        {
            InitializeCopyAndReplicationDrainTcs();
            this.copiedUptoLsn = LogicalSequenceNumber.MaxLsn;
            this.ReadConsistentAfterLsn = LogicalSequenceNumber.MaxLsn;
            this.replicationStreamDrainCompletionTcs.TrySetResult(null);
            this.copyStreamDrainCompletionTcs.TrySetResult(null);
        }

        public void OnSuccessfulRecovery(LogicalSequenceNumber recoveredLsn)
        {
            this.ReadConsistentAfterLsn = recoveredLsn;
        }

        private void InitializeCopyAndReplicationDrainTcs()
        {
            this.replicationStreamDrainCompletionTcs = new TaskCompletionSource<object>();
            this.copyStreamDrainCompletionTcs = new TaskCompletionSource<object>();
        }

        public async Task WaitForCopyAndReplicationDrainToCompleteAsync()
        {
            await this.copyStreamDrainCompletionTcs.Task.ConfigureAwait(false);
            await this.replicationStreamDrainCompletionTcs.Task.ConfigureAwait(false);
        }

        public async Task WaitForCopyDrainToCompleteAsync()
        {
            await this.copyStreamDrainCompletionTcs.Task.ConfigureAwait(false);
        }

        public void StartBuildingSecondary()
        {
            // The task can start really late in stress conditions. Before completing change role, re-initialize the drain tcs to avoid racing close to go ahead of the drain
            InitializeCopyAndReplicationDrainTcs();
            Task.Run((Func<Task>)this.BuildSecondaryAsync).IgnoreExceptionVoid();
        }

        private async Task BuildSecondaryAsync()
        {
            try
            {
                var copyStream = this.fabricReplicator.GetCopyStream();
                await this.CopyOrBuildReplicaAsync(copyStream).ConfigureAwait(false);
                this.copyStreamDrainCompletionTcs.TrySetResult(null);

                var replicationStream = this.fabricReplicator.GetReplicationStream();
                await this.DrainReplicationStreamAsync(replicationStream).ConfigureAwait(false);
                this.replicationStreamDrainCompletionTcs.TrySetResult(null);
            }
            catch (Exception e)
            {
                int innerHResult = 0;
                var flattenedException = Utility.FlattenException(e, out innerHResult);
                var message = string.Format(
                        CultureInfo.InvariantCulture,
                        "BuildSecondaryAsync" + Environment.NewLine + "    Encountered exception: {0}"
                        + Environment.NewLine + "    Message: {1} HResult: 0x{2:X8}" + Environment.NewLine + "{3}",
                        flattenedException.GetType().ToString(),
                        flattenedException.Message,
                        flattenedException.HResult != 0 ? flattenedException.HResult : innerHResult,
                        flattenedException.StackTrace);

                FabricEvents.Events.Warning(this.tracer.Type, message);

                this.copyStreamDrainCompletionTcs.TrySetResult(null);
                this.replicationStreamDrainCompletionTcs.TrySetResult(null);

                this.roleContextDrainState.ReportPartitionFault();
            }
        }

        private bool CopiedUpdateEpoch(UpdateEpochLogRecord record)
        {
            Utility.Assert(
                this.roleContextDrainState.DrainingStream == DrainingStream.CopyStream,
                "this.recordsProcessor.DrainingStream == DrainingStream.CopyStream");
            Utility.Assert(
                record.Lsn.LSN == this.replicatedLogManager.CurrentLogTailLsn.LSN,
                "record.LastLogicalSequenceNumber.LSN == this.currentLogTailLsn.LSN");

            FabricEvents.Events.UpdateEpoch(
                this.tracer.Type,
                "CopiedUpdateEpoch",
                record.Epoch.DataLossNumber,
                record.Epoch.ConfigurationNumber,
                record.Lsn.LSN,
                this.roleContextDrainState.ReplicaRole.ToString());

            var lastVector = this.replicatedLogManager.ProgressVector.LastProgressVectorEntry;
            if (record.Epoch <= lastVector.Epoch)
            {
                if (this.replicatedLogManager.CurrentLogTailEpoch < lastVector.Epoch)
                {
                    // This case happens during full copy before the first checkpoint
                    Utility.Assert(
                        record.Lsn <= this.recoveredOrCopiedCheckpointLsn.Value,
                        "record.LastLogicalSequenceNumber <= this.recoveredOrCopiedCheckpointLsn");

                    this.replicatedLogManager.AppendWithoutReplication(record, null);
                    if (this.replicatedLogManager.CurrentLogTailEpoch < record.Epoch)
                    {
                        this.replicatedLogManager.SetTailEpoch(record.Epoch);
                    }

                    return false;
                }

                var isInserted = this.replicatedLogManager.ProgressVector.Insert(new ProgressVectorEntry(record));
                if (isInserted == true)
                {
                    // This case happens when a series of primaries fail before
                    // a stable one completes reconfiguration
                    this.replicatedLogManager.AppendWithoutReplication(record, null);
                }
                else
                {
                    ProcessDuplicateRecord(record);
                }

                return false;
            }

            this.replicatedLogManager.UpdateEpochRecord(record);
            return true;
        }

        /// <summary>
        /// Copies or Builds Idle Secondary replica from copyStream populated by the Primary.
        /// </summary>
        /// <param name="copyStream">The copy stream populated by the primary.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        private async Task CopyOrBuildReplicaAsync(IOperationStream copyStream)
        {
            var operation = await copyStream.GetOperationAsync(CancellationToken.None).ConfigureAwait(false);
            if (operation == null)
            {
                return;
            }

            Utility.Assert(operation.Data.Count == 1, "operation.Data.Count == 1");

            CopyHeader copyHeader = CopyHeader.ReadFromOperationData(operation.Data);

            operation.Acknowledge();

            if (copyHeader.Stage == CopyStage.CopyNone)
            {
                // GopalK: The order of the following statements is significant
                Utility.Assert(
                    this.roleContextDrainState.DrainingStream == DrainingStream.Invalid,
                    "this.recordsProcessor.DrainingStream == DrainingStream.Invalid");

                // Since there is no false progress stage, dispose the recovery stream
                this.recoveryManager.DisposeRecoveryReadStreamIfNeeded();

                operation = await copyStream.GetOperationAsync(CancellationToken.None).ConfigureAwait(false);
                Utility.Assert(operation == null, "operation == null");

                var trace = string.Format(
                    CultureInfo.InvariantCulture,
                    "Idle replica is already current with primary replica: {0}",
                    copyHeader.PrimaryReplicaId);

                FabricEvents.Events.CopyOrBuildReplica(this.tracer.Type, trace);

                return;
            }

            BeginCheckpointLogRecord copiedCheckpointRecord;
            bool renamedCopyLogSuccessfully = false;

            if (copyHeader.Stage == CopyStage.CopyState)
            {
                var trace = string.Format(CultureInfo.InvariantCulture, "Idle replica is copying from primary replica: {0}", copyHeader.PrimaryReplicaId);

                // Since there is no false progress stage, dispose the recovery stream
                this.recoveryManager.DisposeRecoveryReadStreamIfNeeded();

                FabricEvents.Events.CopyOrBuildReplica(this.tracer.Type, trace);

                operation = await this.DrainStateStreamAsync(copyStream).ConfigureAwait(false);

                if (operation == null)
                {
                    FabricEvents.Events.CopyOrBuildReplica(
                        this.tracer.Type,
                        "Returning null as copy pump has been aborted");

                    return;
                }

                CopyMetadata copyMetadata = CopyMetadata.ReadFromOperationData(operation.Data);

                this.ReadConsistentAfterLsn = copyMetadata.HighestStateProviderCopiedLsn;

                trace =
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "Copy started. StartingLSN: {0} StartingEpoch: {1},{2} CheckpointLSN: {3} UptoLSN: {4} Highest Checkpointed Lsn {5}"
                        + Environment.NewLine + "Copied ProgressVector: {6}" + Environment.NewLine,
                        copyMetadata.StartingLogicalSequenceNumber,
                        copyMetadata.StartingEpoch.DataLossNumber,
                        copyMetadata.StartingEpoch.ConfigurationNumber,
                        copyMetadata.CheckpointLsn,
                        copyMetadata.UptoLsn,
                        copyMetadata.HighestStateProviderCopiedLsn,
                        copyMetadata.ProgressVector.ToString(Constants.ProgressVectorMaxStringSizeInKb));

                FabricEvents.Events.CopyOrBuildReplica(this.tracer.Type, trace);

                this.transactionManager.TransactionsMap.Reuse();
                this.checkpointManager.ResetStableLsn(copyMetadata.CheckpointLsn);
                this.copiedUptoLsn = copyMetadata.UptoLsn;

                var newLogHead = await this.replicatedLogManager.LogManager.CreateCopyLogAsync(copyMetadata.StartingEpoch, copyMetadata.StartingLogicalSequenceNumber).ConfigureAwait(false);

                this.recoveredOrCopiedCheckpointLsn.Update(copyMetadata.CheckpointLsn);
                this.replicatedLogManager.Reuse(
                    copyMetadata.ProgressVector,
                    null,
                    null,
                    null,
                    InformationLogRecord.InvalidInformationLogRecord,
                    newLogHead,
                    copyMetadata.StartingEpoch,
                    copyMetadata.StartingLogicalSequenceNumber);

                // RD: RDBug 7475439: Utility.Assert(sourceEntry == targetEntry, "currentSourceVector == currentTargetVector");
                // UpdateEpoch lsn is same as starting lsn, so insert UE log record
                if (copyMetadata.StartingLogicalSequenceNumber == copyMetadata.ProgressVector.LastProgressVectorEntry.Lsn)
                {
                    var record = new UpdateEpochLogRecord(
                        copyMetadata.ProgressVector.LastProgressVectorEntry.Epoch,
                        copyMetadata.ProgressVector.LastProgressVectorEntry.PrimaryReplicaId)
                    {
                        Lsn = copyMetadata.StartingLogicalSequenceNumber
                    };

                    FabricEvents.Events.UpdateEpoch(
                        this.tracer.Type,
                        "UpdateEpochRecordDueToFullCopy",
                        record.Epoch.DataLossNumber,
                        record.Epoch.ConfigurationNumber,
                        record.Lsn.LSN,
                        this.roleContextDrainState.ReplicaRole.ToString());

                    // NOTE: Do not use the UpdateEpoch method on logmanager as it adds the entry to the list of progress vectors.
                    // We do not want to do that here as the entry already exists
                    this.replicatedLogManager.AppendWithoutReplication(record, null);
                }

                copiedCheckpointRecord = null;
                if (this.recoveredOrCopiedCheckpointLsn.Value == copyMetadata.StartingLogicalSequenceNumber)
                {
                    this.checkpointManager.FirstBeginCheckpointOnIdleSecondary();
                    copiedCheckpointRecord = this.replicatedLogManager.LastInProgressCheckpointRecord;

                    Utility.Assert(
                        copiedCheckpointRecord.Lsn == this.recoveredOrCopiedCheckpointLsn.Value,
                        "copiedCheckpointRecordLsn {0} != recoveredOrCopiedCheckpointLsn {1}",
                        copiedCheckpointRecord.Lsn, this.recoveredOrCopiedCheckpointLsn.Value);
                    
                    // If this is the UptoLsn, ensure rename is done before continuing
                    if (recoveredOrCopiedCheckpointLsn.Value == this.copiedUptoLsn)
                    {
                        // This ensures we dont get stuck waiting for stable LSN
                        Utility.Assert(
                            checkpointManager.LastStableLsn == this.recoveredOrCopiedCheckpointLsn.Value,
                            "checkpointManager.LastStableLsn {0} == this.recoveredOrCopiedCheckpointLsn.Value {1}",
                            checkpointManager.LastStableLsn, this.recoveredOrCopiedCheckpointLsn.Value);

                        await this.checkpointManager.CompleteFirstCheckpointOnIdleAndRenameLog(copiedCheckpointRecord, this.copiedUptoLsn.LSN).ConfigureAwait(false);
                        renamedCopyLogSuccessfully = true;
                    }
                }

                operation.Acknowledge();
                FabricEvents.Events.CopyOrBuildReplica(this.tracer.Type, "Acked progress ProgressVectorEntry operation");

                operation = await copyStream.GetOperationAsync(CancellationToken.None).ConfigureAwait(false);
            }
            else
            {
                FabricEvents.Events.CopyOrBuildReplica(
                    this.tracer.Type,
                    "Idle replica is building from primary replica: " + copyHeader.PrimaryReplicaId);

                operation = await this.TruncateTailIfNecessary(copyStream).ConfigureAwait(false);
                copiedCheckpointRecord = BeginCheckpointLogRecord.InvalidBeginCheckpointLogRecord;

                // Since the false progress stage is complete, dispose the recovery stream
                this.recoveryManager.DisposeRecoveryReadStreamIfNeeded();
            }

            await this.DrainCopyStreamAsync(copyStream, operation, copiedCheckpointRecord, renamedCopyLogSuccessfully).ConfigureAwait(false);
        }

        private async Task DrainCopyStreamAsync(
            IOperationStream copyStream,
            IOperation operation,
            BeginCheckpointLogRecord copiedCheckpointRecord,
            bool renamedCopyLogSuccessfully)
        {
            FabricEvents.Events.DrainStart(this.tracer.Type, "Copy stream: RenamedCopyLogSuccessfully: " + renamedCopyLogSuccessfully);

            var lastCopiedRecord = LogicalLogRecord.InvalidLogicalLogRecord;
            long copiedRecordNumber = 0, acksOutstanding = 1;

            TaskCompletionSource<object> allOperationsAckedTcs = new TaskCompletionSource<object>();

            try
            {
                if (operation != null)
                {
                    this.roleContextDrainState.OnDrainCopy();

                    do
                    {
                        var data = operation.Data;
#if DEBUG
                        ReplicatedLogManager.ValidateOperationData(data, "DrainCopyStreamAsync LSN: " + operation.SequenceNumber);
#endif
                        lastCopiedRecord = (LogicalLogRecord)LogRecord.FromOperationData(data);

                        await this.LogLogicalRecordOnSecondaryAsync(lastCopiedRecord).ConfigureAwait(false);

                        // After successfully appending the record into the buffer, increment the outstanding ack count
                        var acksRemaining = Interlocked.Increment(ref acksOutstanding);

                        FabricEvents.Events.DrainCopyReceive(
                            this.tracer.Type,
                            copiedRecordNumber,
                            lastCopiedRecord.RecordType.ToString(),
                            lastCopiedRecord.Lsn.LSN,
                            acksRemaining);

                        ++copiedRecordNumber;
                        if (this.replicatorSettings.PublicSettings.MaxCopyQueueSize / 2 <= acksRemaining)
                        {
                            FabricEvents.Events.DrainCopyFlush(
                                this.tracer.Type,
                                copiedRecordNumber,
                                lastCopiedRecord.Lsn.LSN,
                                acksRemaining);

                            this.replicatedLogManager.LogManager.FlushAsync("DrainCopyStream.IsFull").IgnoreExceptionVoid();
                        }

                        var capturedOperation = operation;
                        var capturedRecord = lastCopiedRecord;
                        if (copiedCheckpointRecord == null)
                        {
                            copiedCheckpointRecord = this.replicatedLogManager.LastInProgressCheckpointRecord;
                            if (copiedCheckpointRecord != null)
                            {
                                Utility.Assert(
                                    copiedCheckpointRecord.Lsn == this.recoveredOrCopiedCheckpointLsn.Value,
                                    "copiedCheckpointRecordLsn {0} == recoveredOrCopiedCheckpointLsn {1}",
                                    copiedCheckpointRecord.Lsn,
                                    this.recoveredOrCopiedCheckpointLsn.Value);
                            }
                        }

                        // If pumped the last operation in the copy stream (indicated by copiedUptoLsn), rename the copy log if this was a full copy
                        // as we are guranteed that the replica has all the data needed to be promoted to an active secondary and we could not have lost any state
                        if (copiedCheckpointRecord != null &&
                            copiedCheckpointRecord != BeginCheckpointLogRecord.InvalidBeginCheckpointLogRecord &&
                            lastCopiedRecord.Lsn == this.copiedUptoLsn &&
                            renamedCopyLogSuccessfully == false) // Copied UE record could have same LSN, so this condition is needed
                        {
                            await this.checkpointManager.CompleteFirstCheckpointOnIdleAndRenameLog(copiedCheckpointRecord, this.copiedUptoLsn.LSN).ConfigureAwait(false);
                            renamedCopyLogSuccessfully = true;
                        }

                        lastCopiedRecord.AwaitFlush().ContinueWith(
                            async task =>
                            {
                                var acksPending = Interlocked.Decrement(ref acksOutstanding);

                                if (task.Exception != null)
                                {
                                    // Signal the drain completion task if needed
                                    if (acksPending == 0)
                                    {
                                        allOperationsAckedTcs.TrySetResult(null);
                                    }

                                    return;
                                }

                                capturedOperation.Acknowledge();

                                Utility.Assert(acksPending >= 0, "acksPending {0} >= 0", acksPending);

                                if (acksPending == 0)
                                {
                                    allOperationsAckedTcs.TrySetResult(null);
                                }

                                FabricEvents.Events.DrainCopyNoise(
                                    this.tracer.Type,
                                    capturedRecord.Lsn.LSN,
                                    acksPending);

                                await capturedRecord.AwaitApply().ConfigureAwait(false);
                            }).IgnoreExceptionVoid();

                        var drainTask = copyStream.GetOperationAsync(CancellationToken.None);
                        if (drainTask.IsCompleted == false)
                        {
                            // GopalK: Currently, we cannot wait for copy to finish because copy might get
                            // abandoned if the primary fails and the product waits for pending
                            // copy operations to get acknowledged before electing a new primary
                            this.replicatedLogManager.LogManager.FlushAsync("DrainCopyStream.IsEmpty").IgnoreExceptionVoid();
                            await drainTask.ConfigureAwait(false);
                        }

                        operation = drainTask.GetAwaiter().GetResult();
                    } while (operation != null);
                }
            }

            // This finally block ensures that before we continue, we cancel the first full copy checkpoint during full build
            // Without having this, it is possible that the above code throws out of this method and any lifecycle API like close might get stuck because
            // there is a pending checkpoint that is not yet fully processed
            finally 
            {
                // If the pump prematurely finishes for any reason, it means the copy log cannot be renamed
                if (copiedCheckpointRecord != null &&
                    copiedCheckpointRecord != BeginCheckpointLogRecord.InvalidBeginCheckpointLogRecord &&
                    renamedCopyLogSuccessfully == false)
                {
                    await this.checkpointManager.CancelFirstCheckpointOnIdleDueToIncompleteCopy(copiedCheckpointRecord, this.copiedUptoLsn.LSN);
                }
            }

            await this.replicatedLogManager.FlushInformationRecordAsync(
                InformationEvent.CopyFinished,
                closeLog: false,
                flushInitiator: "DrainCopyStream.IsFinished").ConfigureAwait(false);

            // Awaiting processing of this record,
            // ensures that all operations in the copystream must have been applied Before we complete the drainComplationTcs.
            await this.replicatedLogManager.LastInformationRecord.AwaitProcessing().ConfigureAwait(false);

            await this.recordsProcessor.WaitForLogicalRecordsProcessingAsync().ConfigureAwait(false);

            var acksOpen = Interlocked.Decrement(ref acksOutstanding);
            Utility.Assert(acksOpen >= 0, "acksOpen {0} >= 0", acksOpen);
            if (acksOpen != 0)
            {
                // wait for all the callbacks above to finish running and acknowleding
                await allOperationsAckedTcs.Task.ConfigureAwait(false);
            }

            Utility.Assert(acksOutstanding == 0, "acksOutstanding == 0");

#if !DotNetCoreClr
            // These are new events defined in System.Fabric, existing CoreCLR apps would break 
            // if these events are refernced as it wont be found. As CoreCLR apps carry System.Fabric
            // along with application
            // This is just a mitigation for now. Actual fix being tracked via bug# 11614507

            FabricEvents.Events.DrainCompleted(
                this.tracer.Type,
                "Copy",
                "Completed",
                copiedRecordNumber,
                (uint)lastCopiedRecord.RecordType,
                lastCopiedRecord.Lsn.LSN,
                lastCopiedRecord.Psn.PSN,
                lastCopiedRecord.RecordPosition);
#endif
        }

        private async Task DrainReplicationStreamAsync(IOperationStream replicationStream)
        {
            FabricEvents.Events.DrainStart(this.tracer.Type, "Replication stream");

            TaskCompletionSource<object> allOperationsAckedTcs = new TaskCompletionSource<object>();
            var lastReplicatedRecord = LogicalLogRecord.InvalidLogicalLogRecord;
            long replicatedRecordNumber = 0, acksOutstanding = 1, bytesOutstanding = 0;

            this.roleContextDrainState.OnDrainReplication();

            do
            {
                var drainTask = replicationStream.GetOperationAsync(CancellationToken.None);
                if (drainTask.IsCompleted == false)
                {
                    this.replicatedLogManager.LogManager.FlushAsync("DrainReplicationStream.IsEmpty").IgnoreExceptionVoid();
                    await drainTask.ConfigureAwait(false);
                }

                var operation = drainTask.GetAwaiter().GetResult();
                if (operation != null)
                {
                    var data = operation.Data;
#if DEBUG
                    ReplicatedLogManager.ValidateOperationData(data, "DrainReplicationStream LSN: " + operation.SequenceNumber);
#endif
                    lastReplicatedRecord = (LogicalLogRecord)LogRecord.FromOperationData(data);
                    lastReplicatedRecord.Lsn = new LogicalSequenceNumber(operation.SequenceNumber);

                    await this.LogLogicalRecordOnSecondaryAsync(lastReplicatedRecord).ConfigureAwait(false);
                    var acksRemaining = Interlocked.Increment(ref acksOutstanding);

                    FabricEvents.Events.DrainReplicationReceive(
                        this.tracer.Type,
                        replicatedRecordNumber,
                        (uint)lastReplicatedRecord.RecordType,
                        lastReplicatedRecord.Lsn.LSN,
                        acksRemaining);

                    ++replicatedRecordNumber;

                    long operationSize = Utility.GetOperationSize(data);

                    var bytesRemaining = Interlocked.Add(ref bytesOutstanding, operationSize);
                    if (((this.replicatorSettings.PublicSettings.MaxSecondaryReplicationQueueSize / 2 <= acksRemaining)
                         || ((this.replicatorSettings.PublicSettings.MaxSecondaryReplicationQueueMemorySize > 0)
                             && (this.replicatorSettings.PublicSettings.MaxSecondaryReplicationQueueMemorySize / 2 <= bytesRemaining)))
                        || ((this.replicatorSettings.PublicSettings.MaxPrimaryReplicationQueueSize / 2 <= acksRemaining)
                            || ((this.replicatorSettings.PublicSettings.MaxPrimaryReplicationQueueMemorySize > 0)
                                && (this.replicatorSettings.PublicSettings.MaxPrimaryReplicationQueueMemorySize / 2 <= bytesRemaining))))
                    {
                        FabricEvents.Events.DrainReplicationFlush(
                            this.tracer.Type,
                            replicatedRecordNumber,
                            lastReplicatedRecord.Lsn.LSN,
                            acksRemaining,
                            bytesRemaining);

                        this.replicatedLogManager.LogManager.FlushAsync("DrainReplicationStream.IsFull").IgnoreExceptionVoid();
                    }

                    var capturedOperation = operation;
                    var capturedRecord = lastReplicatedRecord;
                    lastReplicatedRecord.AwaitFlush().IgnoreException().ContinueWith(
                        async task =>
                        {
                            var acksPending = Interlocked.Decrement(ref acksOutstanding);

                            if (task.Exception != null)
                            {
                                // Signal the drain completion task if needed
                                if (acksPending == 0)
                                {
                                    allOperationsAckedTcs.TrySetResult(null);
                                }

                                return;
                            }

                            var bytesPending = Interlocked.Add(ref bytesOutstanding, -operationSize);
                            Utility.Assert(
                                (acksPending >= 0) && (bytesPending >= 0),
                                "(acksPending >= 0) && (bytesPending >= 0)");

                            if (acksPending == 0)
                            {
                                allOperationsAckedTcs.TrySetResult(null);
                            }

                            capturedOperation.Acknowledge();

                            FabricEvents.Events.DrainReplicationNoise(
                                this.tracer.Type,
                                capturedRecord.Lsn.LSN,
                                acksPending,
                                bytesPending);

                            await capturedRecord.AwaitApply().ConfigureAwait(false);
                        }).IgnoreExceptionVoid();
                }
                else
                {
                    await this.replicatedLogManager.FlushInformationRecordAsync(
                        InformationEvent.ReplicationFinished,
                        closeLog: false,
                        flushInitiator: "DrainReplicationstream.IsFinished").ConfigureAwait(false);

                    await this.replicatedLogManager.LastInformationRecord.AwaitProcessing().ConfigureAwait(false);

                    await this.recordsProcessor.WaitForLogicalRecordsProcessingAsync().ConfigureAwait(false);

                    var acksPending = Interlocked.Decrement(ref acksOutstanding);
                    Utility.Assert(acksPending >= 0, "acksPending >= 0");
                    if (acksPending != 0)
                    {
                        await allOperationsAckedTcs.Task.ConfigureAwait(false);
                    }

                    Utility.Assert(acksOutstanding == 0, "acksOutstanding == 0");
                    break;
                }
            } while (true);

#if !DotNetCoreClr
            // These are new events defined in System.Fabric, existing CoreCLR apps would break 
            // if these events are refernced as it wont be found. As CoreCLR apps carry System.Fabric
            // along with application
            // This is just a mitigation for now. Actual fix being tracked via bug# 11614507

            FabricEvents.Events.DrainCompleted(
                this.tracer.Type,
                "Replication",
                "Completed",
                replicatedRecordNumber,
                (uint)lastReplicatedRecord.RecordType,
                lastReplicatedRecord.Lsn.LSN,
                lastReplicatedRecord.Psn.PSN,
                lastReplicatedRecord.RecordPosition);
#endif
        }

        private async Task<IOperation> DrainStateStreamAsync(IOperationStream copyStateStream)
        {
            FabricEvents.Events.DrainStart(this.tracer.Type, "State stream");

            long stateRecordNumber = 0;
            var operation = await copyStateStream.GetOperationAsync(CancellationToken.None).ConfigureAwait(false);
            if (operation == null)
            {
                return null;
            }

            this.roleContextDrainState.OnDrainState();

            this.stateManager.BeginSettingCurrentState();

            do
            {
                var data = operation.Data;

                CopyStage copyStage;
                using (
                    var br =
                        new BinaryReader(
                            new MemoryStream(
                                data[data.Count - 1].Array,
                                data[data.Count - 1].Offset,
                                data[data.Count - 1].Count)))
                {
                    copyStage = (CopyStage)br.ReadInt32();
                }

                if (copyStage == CopyStage.CopyState)
                {
                    var copiedBytes = new List<ArraySegment<byte>>();
                    for (var i = 0; i < data.Count - 1; i++)
                    {
                        copiedBytes.Add(data[i]);
                    }

                    var copiedData = new OperationData(copiedBytes);

                    FabricEvents.Events.DrainStateNoise(
                        this.tracer.Type,
                        "Received state record: " + stateRecordNumber,
                        string.Empty);

                    await this.stateManager.SetCurrentStateAsync(stateRecordNumber, copiedData).ConfigureAwait(false);

                    operation.Acknowledge();

                    FabricEvents.Events.DrainStateNoise(
                        this.tracer.Type,
                        "Acked state record: " + stateRecordNumber,
                        string.Empty);

                    stateRecordNumber++;
                }
                else
                {
                    Utility.Assert(
                        copyStage == CopyStage.CopyProgressVector,
                        "copyStage == CopyStage.CopyProgressVector");
                    break;
                }

                operation = await copyStateStream.GetOperationAsync(CancellationToken.None).ConfigureAwait(false);
            } while (operation != null);

            bool copyCompleted = operation != null;
            
            //RDBug#10479578: If copy is aborted (stream returning null), EndSettingCurrentState API will not be called.
            if (copyCompleted)
            {
                await this.stateManager.EndSettingCurrentStateAsync().ConfigureAwait(false);
            }

#if !DotNetCoreClr
            // These are new events defined in System.Fabric, existing CoreCLR apps would break 
            // if these events are refernced as it wont be found. As CoreCLR apps carry System.Fabric
            // along with application
            // This is just a mitigation for now. Actual fix being tracked via bug# 11614507

            FabricEvents.Events.DrainCompleted(
                    this.tracer.Type,
                    "State",
                    copyCompleted ? "Completed" : "Incomplete",
                    stateRecordNumber,
                    (uint)LogRecordType.Invalid,
                    0,
                    0,
                    0);
#endif
            return operation;
        }

        private async Task LogLogicalRecordOnSecondaryAsync(LogicalLogRecord record)
        {
            var recordLsn = record.Lsn;

            if ((recordLsn < this.replicatedLogManager.CurrentLogTailLsn)
                || ((recordLsn == this.replicatedLogManager.CurrentLogTailLsn) && ((record is UpdateEpochLogRecord) == false)))
            {
                ProcessDuplicateRecord(record);
                return;
            }

            this.StartLoggingOnSecondary(record);

            if (this.replicatedLogManager.LogManager.ShouldThrottleWrites)
            {
                FabricEvents.Events.Warning(this.tracer.Type, "Blocking secondary pump due to pending flush cache size");
                await this.replicatedLogManager.LogManager.WaitForPendingRecordsToFlush().ConfigureAwait(false);
                FabricEvents.Events.Warning(this.tracer.Type, "Continuing secondary pump as flush cache is purged");
            }
            
            var barrierRecord = record as BarrierLogRecord;

            if (barrierRecord != null && 
                roleContextDrainState.DrainingStream != DrainingStream.CopyStream)
            {
                // Only invoke this method on barrier boundaries and only on active replication stream.
                // copy stream will continue pumping and the pending checkpoint during full copy will only complete when we finish pumping all copy operations
                // if we stop pumping, we will hit a deadlock since checkpoint waits for copy pump completion and only on copy pump completion do we complete the checkpoint
                await this.checkpointManager.BlockSecondaryPumpIfNeeded(barrierRecord.LastStableLsn).ConfigureAwait(false);
            }
        }

        private void StartLoggingOnSecondary(LogicalLogRecord record)
        {
            switch (record.RecordType)
            {
                case LogRecordType.BeginTransaction:
                case LogRecordType.Operation:
                case LogRecordType.EndTransaction:
                case LogRecordType.Backup:

                    var bufferedRecordsSizeBytes = this.replicatedLogManager.AppendWithoutReplication(
                        record,
                        this.transactionManager);

                    if (bufferedRecordsSizeBytes > this.replicatorSettings.PublicSettings.MaxRecordSizeInKB * 1024)
                    {
                        this.replicatedLogManager.LogManager.FlushAsync("InitiateLogicalRecordProcessingOnSecondary BufferedRecordsSize: " + bufferedRecordsSizeBytes).IgnoreExceptionVoid();
                    }
                    break;

                case LogRecordType.Barrier:
                    this.checkpointManager.AppendBarrierOnSecondary(record as BarrierLogRecord);
                    break;

                case LogRecordType.UpdateEpoch:
                    this.CopiedUpdateEpoch((UpdateEpochLogRecord)record);

                    break;

                default:
                    Utility.CodingError("Unexpected record type {0}", record.RecordType);
                    break;
            }

            this.checkpointManager.InsertPhysicalRecordsIfNecessaryOnSecondary(this.copiedUptoLsn, this.roleContextDrainState.DrainingStream, record);
        }

        private async Task TruncateTailAsync(LogicalSequenceNumber tailLsn)
        {
            FabricEvents.Events.TruncateTail(this.tracer.Type, tailLsn.LSN);

            Utility.Assert(
                (this.roleContextDrainState.ReplicaRole == ReplicaRole.IdleSecondary) || (this.roleContextDrainState.ReplicaRole == ReplicaRole.ActiveSecondary),
                "(this.Role == ReplicaRole.IdleSecondary) || (this.Role == ReplicaRole.ActiveSecondary)");

            Utility.Assert(
                this.checkpointManager.LastStableLsn <= tailLsn,
                "this.lastStableLsn <= tailLsn. last stable lsn :{0}",
                this.checkpointManager.LastStableLsn.LSN);

            ApplyContext falseProgressApplyContext = ApplyContext.SecondaryFalseProgress;

            var truncateTailMgr = new TruncateTailManager(
                this.replicatedLogManager,
                this.transactionManager.TransactionsMap,
                this.stateManager,
                this.backupManager,
                tailLsn,
                falseProgressApplyContext,
                this.recoveryManager.RecoveryLogsReader,
                this.tracer);

            var currentRecord = await truncateTailMgr.TruncateTailAsync();

            Utility.Assert(
                currentRecord.Lsn == tailLsn,
                "TruncateTail: V1 replicator ensures that lsns are continuous. currentLsn {0} == tailLsn {1}",
                currentRecord.Lsn, tailLsn);

            this.checkpointManager.ResetStableLsn(tailLsn);
            this.recoveryManager.OnTailTruncation(tailLsn);

            // 6450429: Replicator maintains three values for supporting consistent reads and snapshot.
            // These values have to be updated as part of the false progress correction.
            // ReadConsistentAfterLsn is used to ensure that all state providers have applied the highest recovery or copy log operation
            // that they might have seen in their current checkpoint. Reading at a tailLsn below this cause inconsistent reads across state providers.
            // Since this was a FALSE PROGRESS copy, replica is still using the checkpoints it has recovered.
            // We have undone any operation that could have been false progressed in these checkpoints (Fuzzy checkpoint only) and ensured all state providers are applied to the same barrier.
            // Hence readConsistentAfterLsn is set to the new tail of the log.
            this.ReadConsistentAfterLsn = tailLsn;

            // lastAppliedBarrierRecord is used to get the current visibility sequence number.
            // Technically currentRecord (new tail record) may not be a barrier however it has the guarantee of a barrier:
            //      All records before tail, including the tail must have been applied.
            // Hence we the lastAppliedBarrierRecord to the new tail record.
            // Note: No other property but .Lsn can be used.
            this.recordsProcessor.TruncateTail(currentRecord);

            // Last value that is kept in the replicator (Version Manager) is the lastDispatchingBarrier.
            // This is used for read your writes support.
            // In this case it is not necessary to modify it since this replica either has not made any new progress (its own write) or it gets elected primary and before it can do anything else
            // dispatches a new barrier which will become the lastDispatchingBarrier
        }

        private async Task<IOperation> TruncateTailIfNecessary(IOperationStream copyStream)
        {
            var operation = await copyStream.GetOperationAsync(CancellationToken.None).ConfigureAwait(false);
            if (operation == null)
            {
                return null;
            }

            var data = operation.Data;

            CopyStage copyStage;
            using (var br = new BinaryReader(new MemoryStream(data[data.Count - 1].Array, data[data.Count - 1].Offset, data[data.Count - 1].Count)))
            {
                copyStage = (CopyStage)br.ReadInt32();
            }

            Utility.Assert(
                (copyStage == CopyStage.CopyFalseProgress) || (copyStage == CopyStage.CopyLog),
                "(copyStage should be false progress or copy log. Copy stage:{0})",
                copyStage);

            if (copyStage == CopyStage.CopyFalseProgress)
            {
                LogicalSequenceNumber sourceStartingLsn;
                using (var br = new BinaryReader(new MemoryStream(data[0].Array, data[0].Offset, data[0].Count)))
                {
                    sourceStartingLsn = new LogicalSequenceNumber(br.ReadInt64());
                }

                Utility.Assert(sourceStartingLsn < this.replicatedLogManager.CurrentLogTailLsn, "sourceStartingLsn < this.currentLogTailLsn");
                operation.Acknowledge();

                await this.TruncateTailAsync(sourceStartingLsn).ConfigureAwait(false);
                operation = await copyStream.GetOperationAsync(CancellationToken.None).ConfigureAwait(false);
            }

            return operation;
        }

        private static void ProcessDuplicateRecord(LogRecord record)
        {
            record.CompletedFlush(null);
            record.CompletedApply(null);
            record.CompletedProcessing(null);

            return;
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Wave
{
    using System;
    using System.Collections.Generic;
    using System.Dynamic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Data.Common;
    using System.Fabric.Messaging.Stream;
    using Microsoft.ServiceFabric.Replicator;
    using System.Fabric.Store;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Manages wave result processing.
    /// </summary>
    public interface IWaveResultProcessor
    {
        /// <summary>
        /// Wave has now completed and it has a result.
        /// </summary>
        /// <param name="wave">Wave completed.</param>
        /// <param name="result">Result of the wave processing.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task OnWaveResult(IWave wave, IEnumerable<WaveFeedback> result, CancellationToken cancellationToken);
    }

    /// <summary>
    /// Manages waves for the local replica.
    /// </summary>
    public interface IWaveController
    {
        /// <summary>
        /// Used to start a new wave in the system.
        /// </summary>
        /// <param name="waveCommand">Wave command.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>The wave that was started after its propagation.</returns>
        Task<IWave> OnPropagateAsync(Transaction replicatorTransaction, WaveCommand waveCommand, CancellationToken cancellationToken);

        /// <summary>
        /// Retrieves the wave processor for a given wave.
        /// </summary>
        /// <param name="wave">Wave to process.</param>
        /// <returns>Appropriate wave processor for this wave instance/type.</returns>
        IWaveProcessor GetWaveProcessor(IWave wave);

        /// <summary>
        /// Retrieves the wave result processor for the given wave.
        /// </summary>
        /// <param name="wave">Wave with result.</param>
        /// <returns>Wave result processor.</returns>
        IWaveResultProcessor GetWaveResultProcessor(IWave wave);
    }

    /// <summary>
    /// Controls wave propagation.
    /// </summary>
    public abstract class WaveController : IStateProvider2, IWaveController, IInboundStreamCallback
    {
        /// <summary>
        /// Wave type name. Used for traces/asserts.
        /// </summary>
        private const string WaveTypeName = "Wave";

        /// <summary>
        /// Incoming and outgoing streams contains this prefix.
        /// </summary>
        private const string WaveStreamNamePrefix = "wavestream";

        /// <summary>
        /// Used as part of propagation stream names.
        /// </summary>
        private const string WaveStreamPropagate = "propagate";

        /// <summary>
        /// Used as part of feedback stream names.
        /// </summary>
        private const string WaveStreamFeedback = "feedback";

        /// <summary>
        /// Timeout used to retrieve wave processors and wave result processors.
        /// </summary>
        private const int ProcessingTimeoutInSeconds = 256;

        #region Instance Members

        /// <summary>
        /// name of the wave store state provider.
        /// </summary>
        private Uri stateProviderNameWaveStore = new Uri("fabric:/WaveController/WaveStore");

        /// <summary>
        /// Wave store. Maintains state about all waves in this wave processor.
        /// </summary>
        private TStore<string, DynamicObject, StringComparerOrdinal, StringComparerOrdinal, StringRangePartitioner> waveStore;

        /// <summary>
        /// Stream store. Maintains streams originating in or accepted by this wave controller.
        /// </summary>
        private IMessageStreamManager streamStore;

        /// <summary>
        /// State manager.
        /// </summary>
        private TransactionalReplicator replicator;

        /// <summary>
        /// Used to delay inbound stream acceptance before the wave controller is initialized.
        /// </summary>
        private TaskCompletionSource<bool> startAsPrimaryAsync;

        /// <summary>
        /// Used to cancel start as primary calls.
        /// </summary>
        private CancellationTokenSource startAsPrimaryAsyncCancellation;

        /// <summary>
        /// Used for tracing.
        /// </summary>
        private string tracer;

        #endregion

        /// <summary>
        /// The wave controller is a singleton. This is its name.
        /// </summary>
        public static Uri Name
        {
            get
            {
                return new Uri("fabric:/WaveController");
            }
        }

        #region IStateProvider2 Members

        Uri IReliableState.Name
        {
            get
            {
                return WaveController.Name;
            }
        } 
        
        byte[] IStateProvider2.InitializationContext
        {
            get { return null; }
        }

        /// <summary>
        /// Aborts the state provider.
        /// </summary>
        void IStateProvider2.Abort()
        {
            ((IStateProvider2)this).CloseAsync().Wait();
        }

        /// <summary>
        /// Applies operations given by the replicator.
        /// </summary>
        /// <param name="lsn">Operation sequence number.</param>
        /// <param name="transactionBase">Transaction for the operation.</param>
        /// <param name="data">Operation data.</param>
        /// <param name="applyContext">Operation context.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task<object> IStateProvider2.ApplyAsync(long lsn, TransactionBase transaction, OperationData metadata, OperationData data, ApplyContext applyContext)
        {
            // Unused.
            throw new NotImplementedException();
        }

        /// <summary>
        /// Begins copy.
        /// </summary>
        void IStateProvider2.BeginSettingCurrentState()
        {
        }

        /// <summary>
        /// ChangeRole is called whenever the replica role is being changed.
        /// </summary>
        /// <param name="newRole">New replica role.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        async Task IStateProvider2.ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            if (ReplicaRole.Primary == newRole)
            {
                // Start primary processing.
                this.startAsPrimaryAsyncCancellation = new CancellationTokenSource();
                this.startAsPrimaryAsync = new TaskCompletionSource<bool>();
                Task primaryProcessing = Task.Factory.StartNew(async () => { await this.StartPrimaryProcessingAsync(); });
            }
            else
            {
                // Complete what was started in primary role, if anything.
                if (null != this.startAsPrimaryAsyncCancellation)
                {
                    this.startAsPrimaryAsyncCancellation.Cancel();

                    try
                    {
                        await this.startAsPrimaryAsync.Task;
                    }
                    catch
                    {
                        // Do nothing.
                    }

                    this.startAsPrimaryAsyncCancellation.Dispose();
                }

                // This is for the case when the APIs are called in any role other than primary.
                this.startAsPrimaryAsync = new TaskCompletionSource<bool>();
                this.startAsPrimaryAsync.SetException(new FabricNotPrimaryException());
            }
        }

        /// <summary>
        /// Prepares for removing state provider.
        /// </summary>
        /// <param name="transactionBase">transaction that this operation is a part of.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.PrepareForRemoveAsync(Transaction transaction, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Wave controller cannot be removed.
            throw new NotSupportedException();
        }

        /// <summary>
        /// Prepares for checkpoint by snapping the state to be checkpointed.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.PrepareCheckpointAsync(long checkpointLSN, CancellationToken cancellationToken)
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
        Task IStateProvider2.PerformCheckpointAsync(PerformCheckpointMode performMode, CancellationToken cancellationToken)
        {
            var tcs = new TaskCompletionSource<object>();
            tcs.SetResult(null);
            return tcs.Task;
        }

        /// <summary>
        /// Completes checkpoint.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.CompleteCheckpointAsync(CancellationToken cancellationToken)
        {
            var tcs = new TaskCompletionSource<object>();
            tcs.SetResult(null);
            return tcs.Task;
        }

        /// <summary>
        /// Closes the state provider.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.CloseAsync()
        {
            if (null != this.startAsPrimaryAsyncCancellation && !this.startAsPrimaryAsyncCancellation.IsCancellationRequested)
            {
                // Cleanup.
                this.startAsPrimaryAsyncCancellation.Cancel();
                this.startAsPrimaryAsyncCancellation.Dispose();
                this.startAsPrimaryAsyncCancellation = null;
            }

            TaskCompletionSource<bool> ret = new TaskCompletionSource<bool>();
            ret.SetResult(true);
            return ret.Task;
        }

        /// <summary>
        /// Completes copy.
        /// </summary>
        Task IStateProvider2.EndSettingCurrentStateAsync()
        {
            return Task.FromResult(0);
        }

        /// <summary>
        /// State copy.
        /// </summary>
        /// <returns>No state is being copied.</returns>
        IOperationDataStream IStateProvider2.GetCurrentState()
        {
            return null;
        }

        /// <summary>
        /// Initializes the state provider on the primary.
        /// </summary>
        /// <param name="replicator">Transactional replicator.</param>
        /// <param name="name">State provider name.</param>
        /// <param name="initializationContext">Initialization parameters.</param>
        /// <param name="stateProviderId">System generated state provider id.</param>
        void IStateProvider2.Initialize(TransactionalReplicator replicator, Uri name, byte[] initializationContext, long stateProviderId, string workDirectory, IEnumerable<IStateProvider2> children)
        {
            // Check arguments.
            if (null == replicator)
            {
                throw new ArgumentNullException("replicator");
            }

            if (null == name)
            {
                throw new ArgumentNullException("name");
            }

            // Set the state manager.
            this.replicator = replicator;

            // TODO: Consider Asserting if these fail.
            this.replicator.TryAddStateSerializer<string>(name, new StringByteConverter());
            this.replicator.TryAddStateSerializer<DynamicObject>(name, new DynamicByteConverter());

            // Set the trace id.
            this.tracer = string.Format(CultureInfo.InvariantCulture, "{0}@{1}", this.replicator.ServiceContext.PartitionId, this.replicator.ServiceContext.ReplicaId);
        }

        /// <summary>
        /// Called after data loss.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.OnDataLossAsync()
        {
            TaskCompletionSource<object> ret = new TaskCompletionSource<object>();
            ret.SetResult(null);
            return ret.Task;
        }

        /// <summary>
        /// Called after recovery is completed.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.OnRecoveryCompletedAsync()
        {
            TaskCompletionSource<bool> ret = new TaskCompletionSource<bool>();
            ret.SetResult(true);
            return ret.Task;
        }

        /// <summary>
        /// Opens the state provider.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.OpenAsync()
        {
            TaskCompletionSource<bool> ret = new TaskCompletionSource<bool>();
            ret.SetResult(true);
            return ret.Task;
        }

        /// <summary>
        /// Call to recover the state provider checkpoint.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.RecoverCheckpointAsync()
        {
            TaskCompletionSource<bool> ret = new TaskCompletionSource<bool>();
            ret.SetResult(true);
            return ret.Task;
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
        /// Removes the state provider state.
        /// </summary>
        /// <param name="stateProviderId">The state provider id to remove.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.RemoveStateAsync(long stateProviderId)
        {
            TaskCompletionSource<bool> ret = new TaskCompletionSource<bool>();
            ret.SetResult(true);
            return ret.Task;
        }

        /// <summary>
        /// Apply copy operation data.
        /// </summary>
        /// <param name="stateRecordNumber">Copy operation sequence number.</param>
        /// <param name="data">Operation data.</param>
        Task IStateProvider2.SetCurrentStateAsync(long stateRecordNumber, OperationData data)
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// Called to release locks associated with replicated operations.
        /// </summary>
        /// <param name="state">Lock context.</param>
        void IStateProvider2.Unlock(object state)
        {
            // Unused.
            throw new NotImplementedException();
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

        #region IWaveController Members

        /// <summary>
        /// Used to start a new wave in the system.
        /// </summary>
        /// <param name="relicatorTransaction">Replicator transaction.</param>
        /// <param name="waveCommand">Wave command.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public async Task<IWave> OnPropagateAsync(Transaction replicatorTransaction, WaveCommand waveCommand, CancellationToken cancellationToken)
        {
            // Check arguments.
            if (null == replicatorTransaction)
            {
                throw new ArgumentNullException("replicatorTransaction");
            }

            if (null == waveCommand)
            {
                throw new ArgumentNullException("waveCommand");
            }

            // Check state
            if (ReplicaRole.Primary != this.replicator.Role)
            {
                throw new FabricNotPrimaryException();
            }

            if (PartitionAccessStatus.Granted != this.replicator.StatefulPartition.WriteStatus)
            {
                throw new FabricTransientException();
            }

            // Wait for primary role change to complete.
            await this.startAsPrimaryAsync.Task;

            List<IMessageStream> outboundStreams = null;
            IEnumerable<PartitionKey> outboundLinks = null;
            PartitionKey inboundLink = this.BuildInboundLink();
            PartitionKeyEqualityComparer equalityComparer = new PartitionKeyEqualityComparer();

            // Construct a new wave.
            Wave wave = new Wave(waveCommand, true);

            // Retrieve the wave processor for this wave.
            wave.Processor = await this.GetWaveProcessorAsync(wave, TimeSpan.FromSeconds(WaveController.ProcessingTimeoutInSeconds));
            wave.SetInboundLink(inboundLink);

            // Retrieve the outbound links for this wave.
            outboundLinks = await wave.Processor.OnWaveStartedAsync(replicatorTransaction, wave, inboundLink, cancellationToken);
            outboundLinks = outboundLinks.Distinct(equalityComparer).Where(x => !equalityComparer.Equals(x, inboundLink));

            // Fail if no outbound links were obtained.
            if (0 == outboundLinks.Count())
            {
                // There has to be at least one outbound link for an initiator wave.
                throw new InvalidOperationException();
            }
            else
            {
                FabricEvents.Events.Created(this.tracer, wave.Id, "initiator", replicatorTransaction.ToString());

                // Create outbound streams for this wave on each outbound link.
                outboundStreams = await this.CreateOutboundStreamsAsync(replicatorTransaction, wave, outboundLinks);

                // Create wave store transaction.
                IStoreReadWriteTransaction waveStoreTxn = this.waveStore.CreateOrFindTransaction(replicatorTransaction).Value;

                // Set wave outbound links.
                wave.SetOutboundLinks(outboundLinks);

                // Add a new wave in the wave store.
                wave.State = WaveState.Created;
                await this.waveStore.AddAsync(waveStoreTxn, wave.Id, wave, Timeout.InfiniteTimeSpan, cancellationToken);

                // Enlist for transaction complete.
                replicatorTransaction.AddLockContext(new WaveLockContext(this, wave.Id, outboundStreams, replicatorTransaction));
            }

            return wave;
        }

        /// <summary>
        /// Retrieves the wave processor for a given wave.
        /// </summary>
        /// <param name="wave">Wave to process.</param>
        /// <returns>Wave processor.</returns>
        public abstract IWaveProcessor GetWaveProcessor(IWave wave);

        /// <summary>
        /// Retrieves the wave result processor for the given wave.
        /// </summary>
        /// <param name="wave">Wave to process.</param>
        /// <returns>Wave result processor.</returns>
        public abstract IWaveResultProcessor GetWaveResultProcessor(IWave wave);

        #endregion

        #region IInboundStreamCallback Members

        /// <summary>
        /// Establishes an inbound stream and start processing it.
        /// </summary>
        /// <param name="inboundStream">Inbound stream that is created.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public async Task InboundStreamCreated(IMessageStream inboundStream)
        {
            // Wait for primary role initialization to complete.
            await this.startAsPrimaryAsync.Task;

            FabricEvents.Events.InboundStream(this.tracer, inboundStream.StreamName.OriginalString, "created", inboundStream.PartnerIdentity.ServiceInstanceName.OriginalString, this.GetTraceFromPartitionKey(inboundStream.PartnerIdentity));
            Diagnostics.Assert(PersistentStreamState.Open == inboundStream.CurrentState, WaveTypeName, "unexpected state {0} for inbound stream {1}", inboundStream.CurrentState, inboundStream.StreamName.OriginalString);

            // Start processing the incoming stream.
            await this.ProcessInboundStreamAsync(inboundStream);
        }

        /// <summary>
        /// Accepts all inbound streams that are wave based.
        /// </summary>
        /// <param name="sourcePartnerIdentity">Source of the stream.</param>
        /// <param name="inboundStreamName">Stream name.</param>
        /// <param name="inboundStreamId">Stream identifier (name is not unique).</param>
        /// <returns>True, if the stream is accepted.</returns>
        public async Task<bool> InboundStreamRequested(PartitionKey sourcePartnerIdentity, Uri inboundStreamName, Guid inboundStreamId)
        {
            // Wait for primary role initialization to complete.
            await this.startAsPrimaryAsync.Task;

            // Check to see if the stream is one that the wave processor recognizes.
            bool accept = inboundStreamName.OriginalString.Contains(WaveController.WaveStreamNamePrefix);
            FabricEvents.Events.InboundStream(this.tracer, inboundStreamName.OriginalString, accept ? "accepted" : "denied", sourcePartnerIdentity.ServiceInstanceName.OriginalString, this.GetTraceFromPartitionKey(sourcePartnerIdentity));
            return accept;
        }

        /// <summary>
        /// Notification of an imminent deletion of an inbound stream
        /// </summary>
        /// <param name="inboundStream">Stream to be deleted</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task InboundStreamDeleted(IMessageStream inboundStream)
        {
            return Task.FromResult(true);
        }

        #endregion

        /// <summary>
        /// Resumes a wave propagation.
        /// </summary>
        /// <param name="waveId">Wave to propagate.</param>
        /// <param name="outboundStreams">Outbound streams to propagate on.</param>
        /// <param name="replicatorTransaction">Replicator transaction on which the wave was created.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        internal async Task ResumeWavePropagation(string waveId, List<IMessageStream> outboundStreams, Transaction replicatorTransaction)
        {
            try
            {
                // Find wave.
                Wave wave = null;
                using (Transaction transaction = this.replicator.CreateTransaction())
                {
                     IStoreReadWriteTransaction rtx = this.waveStore.CreateStoreReadWriteTransaction(transaction);
                    
                        ConditionalValue<DynamicObject> result = await this.waveStore.GetAsync(rtx, waveId, Timeout.InfiniteTimeSpan, CancellationToken.None);
                        if (!result.HasValue)
                        {
                            FabricEvents.Events.Created(this.tracer, waveId, "stopped", replicatorTransaction.ToString());

                            // The replicator transaction aborted.
                            return;
                        }

                        // Set wave.
                        wave = (Wave)result.Value;
                }

                // Open outbound streams.
                List<Task> openStreamTasks = new List<Task>();
                foreach (IMessageStream x in outboundStreams)
                {
                    FabricEvents.Events.OutboundStream(this.tracer, x.StreamName.OriginalString, "open", x.PartnerIdentity.ServiceInstanceName.OriginalString, this.GetTraceFromPartitionKey(x.PartnerIdentity));

                    // Outbound streams are opened concurrently.
                    openStreamTasks.Add(x.OpenAsync(Timeout.InfiniteTimeSpan));
                }

                // Await all open tasks.
                await Task.WhenAll(openStreamTasks);

                // Propagate wave.
                await this.PropagateWaveAsync(wave, outboundStreams, CancellationToken.None);

                // Close outbound streams.
                List<Task> closeStreamTasks = new List<Task>();
                foreach (IMessageStream x in outboundStreams)
                {
                    FabricEvents.Events.OutboundStream(this.tracer, x.StreamName.OriginalString, "close", x.PartnerIdentity.ServiceInstanceName.OriginalString, this.GetTraceFromPartitionKey(x.PartnerIdentity));

                    // Outbound streams are closed concurrently.
                    closeStreamTasks.Add(x.CloseAsync(Timeout.InfiniteTimeSpan));
                }

                // Await all close tasks.
                await Task.WhenAll(closeStreamTasks);

                // Delete outbound streams.
                using (Transaction transaction = this.replicator.CreateTransaction())
                {
                    foreach (IMessageStream x in outboundStreams)
                    {
                        FabricEvents.Events.OutboundStream(this.tracer, x.StreamName.ToString(), "delete", x.PartnerIdentity.ServiceInstanceName.ToString(), this.GetTraceFromPartitionKey(x.PartnerIdentity));

                        // Outbound streams are closed concurrently.
                        await x.DeleteAsync(transaction, Timeout.InfiniteTimeSpan);
                    }

                    // Commit transaction.
                    await transaction.CommitAsync();
                }
            }
            catch (Exception e)
            {
                // Process exception.
                e = Diagnostics.ProcessException(WaveTypeName, e, string.Empty);
                this.HandleException(e);
            }
        }

        /// <summary>
        /// Decides whether or not to report fault transient.
        /// </summary>
        /// <param name="e">Exception observed.</param>
        private void HandleException(Exception e)
        {
            bool doNotCallReportFault = e is FabricObjectClosedException || e is FabricNotPrimaryException || e is OperationCanceledException || e is FabricNotReadableException;
            if (!doNotCallReportFault)
            {
                try
                {
                    this.replicator.StatefulPartition.ReportFault(FaultType.Transient);
                }
                catch (Exception exception)
                {
                    Diagnostics.ProcessException(WaveTypeName, exception, "ReportFault");
                }
            }
        }

        /// <summary>
        /// Informs the wave processor that the wave processor is now executing.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        private async Task StartPrimaryProcessingAsync()
        {
            try
            {
                PartitionKeyEqualityComparer equalityComparer = new PartitionKeyEqualityComparer();
                List<IMessageStream> pairedOutboundStreams = new List<IMessageStream>();

                // Wait for primary role completion.
                FabricEvents.Events.PrimaryProcessing(this.tracer, "starting");
                await this.WaitForPrimaryRoleCompletionAsync();

                // Retrieve stores.
                FabricEvents.Events.PrimaryProcessing(this.tracer, "retrieve stores");
                await this.CreateOrRetrieveRequiredStateProvidersAsync();

                FabricEvents.Events.PrimaryProcessing(this.tracer, "pair inbound/outbound streams");

                // Iterate over the open inbound streams and make sure they have an open outbound stream pair associated.
                IEnumerable<IMessageStream> inboundStreams = await this.streamStore.CreateInboundStreamsEnumerableAsync();
                foreach (IMessageStream x in inboundStreams.Where(inboundStream => PersistentStreamState.Closed != inboundStream.CurrentState && PersistentStreamState.Deleted != inboundStream.CurrentState && PersistentStreamState.Deleting != inboundStream.CurrentState))
                {
                    // Check if the inbound stream is a propagate stream.
                    if (x.StreamName.OriginalString.Contains(WaveStreamPropagate))
                    {
                        Diagnostics.Assert(x.CurrentState == PersistentStreamState.Open, WaveTypeName, "unexpected state {0} for inbound stream {1}", x.CurrentState, x.StreamName);

                        // Find associated outbound stream.
                        Uri outboundStreamName = this.CreateFeedbackOutboundStreamName(x.StreamName);
                        IMessageStream outboundStream = (await this.streamStore.CreateOutboundStreamsEnumerableAsync()).Where(y => equalityComparer.Equals(y.PartnerIdentity, x.PartnerIdentity) && 0 == Uri.Compare(y.StreamName, outboundStreamName, UriComponents.HttpRequestUrl, UriFormat.UriEscaped, StringComparison.Ordinal)).FirstOrDefault();
                        if (null == outboundStream)
                        {
                            // An outbound stream associated with it needs to be created. Then the inbound stream needs processing.
                            IMessageStream capture = x;
                            Task inboundStreamRestartTask = Task.Factory.StartNew(async () =>
                            {
                                try
                                {
                                    FabricEvents.Events.InboundStreamRestart(this.tracer, capture.StreamName.OriginalString, "process", capture.PartnerIdentity.ServiceInstanceName.OriginalString, this.GetTraceFromPartitionKey(capture.PartnerIdentity));

                                    // Wait for primary role initialization to complete.
                                    await this.startAsPrimaryAsync.Task;

                                    await this.ProcessInboundStreamAsync(capture);
                                }
                                catch (Exception e)
                                {
                                    // Process exception.
                                    e = Diagnostics.ProcessException(WaveTypeName, e, string.Empty);
                                    this.HandleException(e);
                                }
                            });
                        }
                        else
                        {
                            // The outbound stream always gets closed after the inbound stream is closed.
                            Diagnostics.Assert(outboundStream.CurrentState != PersistentStreamState.Closing && outboundStream.CurrentState != PersistentStreamState.Closed, WaveTypeName, "unexpected state {0} outbound stream {0}", outboundStream.CurrentState, outboundStream.StreamName);

                            // Record the paired outbound stream.
                            pairedOutboundStreams.Add(outboundStream);

                            // This is an inbound stream that needs processing.
                            IMessageStream captureInbound = x;
                            IMessageStream captureOutbound = outboundStream;
                            Task inboundStreamRestartTask = Task.Factory.StartNew(async () =>
                            {
                                try
                                {
                                    FabricEvents.Events.OutboundStreamRestart(this.tracer, captureOutbound.StreamName.OriginalString, "open", captureOutbound.PartnerIdentity.ServiceInstanceName.OriginalString, this.GetTraceFromPartitionKey(captureOutbound.PartnerIdentity));

                                    // The stream might be opening, but not yet open.
                                    await captureOutbound.OpenAsync(Timeout.InfiniteTimeSpan);

                                    FabricEvents.Events.InboundStreamRestart(this.tracer, captureInbound.StreamName.OriginalString, "process", captureInbound.PartnerIdentity.ServiceInstanceName.OriginalString, this.GetTraceFromPartitionKey(captureInbound.PartnerIdentity));

                                    // Wait for primary role initialization to complete.
                                    await this.startAsPrimaryAsync.Task;

                                    await this.ProcessIncomingStreamAsync(captureInbound);
                                }
                                catch (Exception e)
                                {
                                    // Process exception.
                                    e = Diagnostics.ProcessException(WaveTypeName, e, string.Empty);
                                    this.HandleException(e);
                                }
                            });
                        }
                    }
                    else if (x.StreamName.ToString().Contains(WaveStreamFeedback))
                    {
                        Diagnostics.Assert(x.CurrentState == PersistentStreamState.Open, WaveTypeName, "unexpected state {0} for inbound stream {1}", x.CurrentState, x.StreamName);

                        // This is an inbound feedback stream that needs processing.
                        IMessageStream capture = x;
                        Task inboundStreamRestartTask = Task.Factory.StartNew(async () =>
                        {
                            try
                            {
                                FabricEvents.Events.InboundStreamRestart(this.tracer, capture.StreamName.OriginalString, "process", capture.PartnerIdentity.ServiceInstanceName.OriginalString, this.GetTraceFromPartitionKey(capture.PartnerIdentity));

                                // Wait for primary role initialization to complete.
                                await this.startAsPrimaryAsync.Task;

                                await this.ProcessIncomingStreamAsync(capture);
                            }
                            catch (Exception e)
                            {
                                // Process exception.
                                e = Diagnostics.ProcessException(WaveTypeName, e, string.Empty);
                                this.HandleException(e);
                            }
                        });
                    }
                }

                FabricEvents.Events.PrimaryProcessing(this.tracer, "compute wave actions");

                // Iterate over all waves and decide what to do with them.
                Uri outgoingStreamName = null;
                List<IMessageStream> outboundStreams = null;
                List<KeyValuePair<Wave, List<IMessageStream>>> propagation = new List<KeyValuePair<Wave, List<IMessageStream>>>();
                List<KeyValuePair<Wave, List<IMessageStream>>> closure = new List<KeyValuePair<Wave, List<IMessageStream>>>();

                using (Transaction transaction = this.replicator.CreateTransaction())
                {
                        // Create wave store transaction.
                        IStoreReadWriteTransaction waveStoreTxn = this.waveStore.CreateStoreReadWriteTransaction(transaction);
                   
                        // Enumerate waves.
                        IEnumerable<KeyValuePair<string, DynamicObject>> enumerateWaves = (await this.waveStore.CreateEnumerableAsync(waveStoreTxn, false, ReadMode.ReadValue)).ToEnumerable();
                        foreach (KeyValuePair<string, DynamicObject> item in enumerateWaves)
                        {
                            Wave wave = (Wave)item.Value;
                            outgoingStreamName = this.CreatePropagateOutboundStreamName(wave);
                            switch (wave.State)
                            {
                                case WaveState.Created:
                                    // Re-open all outbound streams and propagate the wave.
                                    FabricEvents.Events.Restart(this.tracer, wave.Id, "created");
                                    outboundStreams = (await this.streamStore.CreateOutboundStreamsEnumerableAsync()).Where(x => x.CurrentState != PersistentStreamState.Closing && x.CurrentState != PersistentStreamState.Closed && wave.OutboundLinks.Contains(x.PartnerIdentity, equalityComparer) && 0 == Uri.Compare(x.StreamName, outgoingStreamName, UriComponents.HttpRequestUrl, UriFormat.UriEscaped, StringComparison.Ordinal)).ToList();
                                    Diagnostics.Assert(outboundStreams.Count == wave.OutboundLinks.Count(), WaveTypeName, "unexpected wave {0} outbound stream count: actual {1} expected {2}", wave.Id, outboundStreams.Count, wave.OutboundLinks.Count());
                                    propagation.Add(new KeyValuePair<Wave, List<IMessageStream>>(wave, outboundStreams));
                                    break;

                                case WaveState.Started:
                                    // Close all outbound streams, since the wave propagated already.
                                    FabricEvents.Events.Restart(this.tracer, wave.Id, "started");
                                    outboundStreams = (await this.streamStore.CreateOutboundStreamsEnumerableAsync()).Where(x => x.CurrentState != PersistentStreamState.Closed && wave.OutboundLinks.Contains(x.PartnerIdentity, equalityComparer) && 0 == Uri.Compare(x.StreamName, outgoingStreamName, UriComponents.HttpRequestUrl, UriFormat.UriEscaped, StringComparison.Ordinal)).ToList();
                                    if (0 != outboundStreams.Count)
                                    {
                                        closure.Add(new KeyValuePair<Wave, List<IMessageStream>>(wave, outboundStreams));
                                    }

                                    break;

                                case WaveState.Invalid:
                                default:
                                    // Should never get here.
                                    Diagnostics.Assert(false, WaveTypeName, "unexpected state");
                                    break;
                            }
                      }
                }

                FabricEvents.Events.PrimaryProcessing(this.tracer, "restart wave propagation");

                // Restart waves, if needed.
                foreach (KeyValuePair<Wave, List<IMessageStream>> item in propagation)
                {
                    KeyValuePair<Wave, List<IMessageStream>> capture = item;
                    Task propagateWaveTask = Task.Factory.StartNew(async () =>
                    {
                        try
                        {
                            // Open outbound streams.
                            List<Task> openStreamTasks = new List<Task>();
                            foreach (IMessageStream outboundStream in capture.Value)
                            {
                                // The outbound stream always gets closed after the wave is started.
                                Diagnostics.Assert(outboundStream.CurrentState != PersistentStreamState.Closing && outboundStream.CurrentState != PersistentStreamState.Closed, WaveTypeName, "unexpected outbound stream state {0}", outboundStream.CurrentState);

                                FabricEvents.Events.OutboundStreamRestart(this.tracer, outboundStream.StreamName.OriginalString, "open", outboundStream.PartnerIdentity.ServiceInstanceName.OriginalString, this.GetTraceFromPartitionKey(outboundStream.PartnerIdentity));

                                openStreamTasks.Add(outboundStream.OpenAsync(Timeout.InfiniteTimeSpan));
                            }

                            Diagnostics.Assert(0 != openStreamTasks.Count, WaveTypeName, "unexpected outbound stream count {0}", openStreamTasks.Count);

                            // Await all open tasks.
                            await Task.WhenAll(openStreamTasks);

                            // Wait for primary role initialization to complete.
                            await this.startAsPrimaryAsync.Task;

                            // Propagate wave.
                            await this.PropagateWaveAsync(capture.Key, capture.Value, CancellationToken.None);

                            // Close outbound streams.
                            List<Task> closeStreamTasks = new List<Task>();
                            foreach (IMessageStream outboundStream in capture.Value)
                            {
                                FabricEvents.Events.OutboundStreamRestart(this.tracer, outboundStream.StreamName.OriginalString, "close", outboundStream.PartnerIdentity.ServiceInstanceName.OriginalString, this.GetTraceFromPartitionKey(outboundStream.PartnerIdentity));

                                closeStreamTasks.Add(outboundStream.CloseAsync(Timeout.InfiniteTimeSpan));
                            }

                            // Await all close tasks.
                            await Task.WhenAll(closeStreamTasks);

                            // Delete all outbound streams.
                            using (Transaction transaction = this.replicator.CreateTransaction())
                            {
                                foreach (IMessageStream outboundStream in capture.Value)
                                {
                                    FabricEvents.Events.OutboundStream(this.tracer, outboundStream.StreamName.ToString(), "delete", outboundStream.PartnerIdentity.ServiceInstanceName.ToString(), this.GetTraceFromPartitionKey(outboundStream.PartnerIdentity));

                                    // Outbound streams are closed concurrently.
                                    await outboundStream.DeleteAsync(transaction, Timeout.InfiniteTimeSpan);
                                }

                                // Commit transaction.
                                await transaction.CommitAsync();
                            }
                        }
                        catch (Exception e)
                        {
                            // Process exception.
                            e = Diagnostics.ProcessException(WaveTypeName, e, string.Empty);
                            this.HandleException(e);
                        }
                    });
                }

                FabricEvents.Events.PrimaryProcessing(this.tracer, "close used outbound streams");

                // Close outbound streams, if needed.
                foreach (KeyValuePair<Wave, List<IMessageStream>> item in closure)
                {
                    foreach (IMessageStream outboundStream in item.Value)
                    {
                        // The outbound stream always gets closed after the wave is started.
                        Diagnostics.Assert(outboundStream.CurrentState == PersistentStreamState.Open || outboundStream.CurrentState == PersistentStreamState.Closing, WaveTypeName, "unexpected state {0} for outbound stream state {1}", outboundStream.CurrentState, outboundStream.StreamName);

                        IMessageStream capture = outboundStream;
                        Task outboundStreamCloseTask = Task.Factory.StartNew(async () =>
                        {
                            try
                            {
                                FabricEvents.Events.OutboundStreamRestart(this.tracer, capture.StreamName.OriginalString, "close", capture.PartnerIdentity.ServiceInstanceName.OriginalString, this.GetTraceFromPartitionKey(capture.PartnerIdentity));

                                // Close the outbound stream.
                                await capture.CloseAsync(Timeout.InfiniteTimeSpan);

                                // Delete the outbound stream.
                                using (Transaction transaction = this.replicator.CreateTransaction())
                                {
                                    FabricEvents.Events.OutboundStreamRestart(this.tracer, capture.StreamName.ToString(), "delete", capture.PartnerIdentity.ServiceInstanceName.ToString(), this.GetTraceFromPartitionKey(capture.PartnerIdentity));

                                    await capture.DeleteAsync(transaction, Timeout.InfiniteTimeSpan);

                                    // Commit transaction.
                                    await transaction.CommitAsync();
                                }
                            }
                            catch (Exception e)
                            {
                                // Process exception.
                                e = Diagnostics.ProcessException(WaveTypeName, e, string.Empty);
                                this.HandleException(e);
                            }
                        });
                    }
                }

                FabricEvents.Events.PrimaryProcessing(this.tracer, "close orphan outbound streams");

                // Close all outbound streams that do not have a wave associated.
                foreach (IMessageStream outboundStream in (await this.streamStore.CreateOutboundStreamsEnumerableAsync()).Where(outboundStream => outboundStream.CurrentState != PersistentStreamState.Closed))
                {
                    using (Transaction transaction = this.replicator.CreateTransaction())
                    {
                        IStoreReadWriteTransaction rtx = this.waveStore.CreateStoreReadWriteTransaction(transaction);
                            // Check if the outbound stream is a wave stream.
                            if (!outboundStream.StreamName.ToString().Contains(WaveStreamNamePrefix))
                            {
                                // This is another's component stream.
                                continue;
                            }

                            string[] splits = outboundStream.StreamName.OriginalString.Split(new string[] { "/" }, StringSplitOptions.None);
                            ConditionalValue<DynamicObject> seekWave = await this.waveStore.GetAsync(rtx, splits[1], Timeout.InfiniteTimeSpan, CancellationToken.None);
                            if (!seekWave.HasValue && !pairedOutboundStreams.Contains(outboundStream))
                            {
                                IMessageStream capture = outboundStream;
                                Task outboundStreamCloseTask = Task.Factory.StartNew(async () =>
                                {
                                    try
                                    {
                                        FabricEvents.Events.OutboundStreamRestart(this.tracer, capture.StreamName.OriginalString, "orphan", capture.PartnerIdentity.ServiceInstanceName.OriginalString, this.GetTraceFromPartitionKey(capture.PartnerIdentity));

                                        // Close the outbound stream.
                                        await capture.CloseAsync(Timeout.InfiniteTimeSpan);

                                        // Delete the outbound stream.
                                        //using (Transaction transaction = this.replicator.CreateTransaction())
                                        //{
                                            FabricEvents.Events.OutboundStreamRestart(this.tracer, capture.StreamName.ToString(), "delete", capture.PartnerIdentity.ServiceInstanceName.ToString(), this.GetTraceFromPartitionKey(capture.PartnerIdentity));

                                            await capture.DeleteAsync(transaction, Timeout.InfiniteTimeSpan);

                                            // Commit transaction.
                                            await transaction.CommitAsync();
                                        //}
                                    }
                                    catch (Exception e)
                                    {
                                        // Process exception.
                                        e = Diagnostics.ProcessException(WaveTypeName, e, string.Empty);
                                        this.HandleException(e);
                                    }
                                });
                            }
                       }
                }

                FabricEvents.Events.PrimaryProcessing(this.tracer, "completed");

                // Initialization as primary completed.
                this.startAsPrimaryAsync.SetResult(true);
            }
            catch (Exception e)
            {
                // Process exception.
                e = Diagnostics.ProcessException(WaveTypeName, e, string.Empty);
                Diagnostics.Assert(e is FabricNotPrimaryException || e is OperationCanceledException || e is FabricObjectClosedException, string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));

                FabricEvents.Events.PrimaryProcessing(this.tracer, "failed");

                // Initialization as primary completed.
                this.startAsPrimaryAsync.SetException(e);
            }
        }

        /// <summary>
        /// Starts processing of an inbound message stream.
        /// </summary>
        /// <param name="inboundStream">Inbound stream to process.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        private async Task ProcessInboundStreamAsync(IMessageStream inboundStream)
        {
            // If the stream is a propagate stream, then a feedback stream is created. Otherwise it is just processed.
            if (inboundStream.StreamName.OriginalString.Contains(WaveStreamPropagate))
            {
                // Create outbound stream pair.
                await this.CreateAndOpenOutboundStreamAsync(inboundStream);
            }

            // Process incoming messages.
            await this.ProcessIncomingStreamAsync(inboundStream);
        }

        /// <summary>
        /// For each incoming propagate stream, a new outbound feedback stream is created, such that they pair up.
        /// </summary>
        /// <param name="inboundStream">Inbound propagate stream.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        private async Task CreateAndOpenOutboundStreamAsync(IMessageStream inboundStream)
        {
            IMessageStream outboundStream = null;
            using (Transaction replicatorTransaction = await TransactionHelpers.SafeCreateReplicatorTransactionAsync(this.replicator))
            {
                // Create outgoing feedback stream for this incoming stream.
                Uri outboundStreamName = this.CreateFeedbackOutboundStreamName(inboundStream.StreamName);
                switch (inboundStream.PartnerIdentity.Kind)
                {
                    case PartitionKind.Singleton:
                        FabricEvents.Events.OutboundStream(this.tracer, outboundStreamName.OriginalString, "create", inboundStream.PartnerIdentity.ServiceInstanceName.OriginalString, "singleton");
                        outboundStream = await this.streamStore.CreateStreamAsync(replicatorTransaction, inboundStream.PartnerIdentity.ServiceInstanceName, outboundStreamName, int.MaxValue);
                        break;

                    case PartitionKind.Named:
                        FabricEvents.Events.OutboundStream(this.tracer, outboundStreamName.OriginalString, "create", inboundStream.PartnerIdentity.ServiceInstanceName.OriginalString, inboundStream.PartnerIdentity.PartitionName);
                        outboundStream = await this.streamStore.CreateStreamAsync(replicatorTransaction, inboundStream.PartnerIdentity.ServiceInstanceName, inboundStream.PartnerIdentity.PartitionName, outboundStreamName, int.MaxValue);
                        break;

                    case PartitionKind.Numbered:
                        FabricEvents.Events.OutboundStream(this.tracer, outboundStreamName.OriginalString, "create", inboundStream.PartnerIdentity.ServiceInstanceName.OriginalString, inboundStream.PartnerIdentity.PartitionRange.IntegerKeyLow.ToString());
                        outboundStream = await this.streamStore.CreateStreamAsync(replicatorTransaction, inboundStream.PartnerIdentity.ServiceInstanceName, inboundStream.PartnerIdentity.PartitionRange.IntegerKeyLow, outboundStreamName, int.MaxValue);
                        break;
                }

                // Commit replicator transaction.
                await replicatorTransaction.CommitAsync();
            }

            // Open outbound stream.
            FabricEvents.Events.OutboundStream(this.tracer, outboundStream.StreamName.OriginalString, "open", outboundStream.PartnerIdentity.ServiceInstanceName.OriginalString, this.GetTraceFromPartitionKey(outboundStream.PartnerIdentity));
            await outboundStream.OpenAsync(Timeout.InfiniteTimeSpan);
        }
        
        /// <summary>
        /// Starts processing on the incoming stream.
        /// </summary>
        /// <param name="inboundStream">Stream to process.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        private async Task ProcessIncomingStreamAsync(IMessageStream inboundStream)
        {
            // Wait for primary role initialization to complete.
            await this.startAsPrimaryAsync.Task;

            FabricEvents.Events.InboundStream(this.tracer, inboundStream.StreamName.OriginalString, "process", inboundStream.PartnerIdentity.ServiceInstanceName.OriginalString, this.GetTraceFromPartitionKey(inboundStream.PartnerIdentity));

            DynamicByteConverter dynamicConverter = new DynamicByteConverter();
            CancellationToken cancellationToken = CancellationToken.None;
            bool wasInitiator = false;

            // In case of a leaf, it holds the outbound echo stream.
            IMessageStream outboundStream = null;
            Uri outboundStreamName = null;

            // Holds the outbound streams for this wave.
            List<IMessageStream> outboundStreams = null;
            IEnumerable<PartitionKey> outboundLinks = null;
            PartitionKey inboundLink = this.BuildInboundLink();
            PartitionKeyEqualityComparer equalityComparer = new PartitionKeyEqualityComparer();

            // Wave to process.
            Wave wave = null;

            // Wave feedback to process.
            WaveFeedback waveFeedback = null;

            // Feedback collected for this wave.
            IEnumerable<WaveFeedback> waveFeedbackResult = null;

            // Create replicator transaction.
            using (Transaction replicatorTransaction = await TransactionHelpers.SafeCreateReplicatorTransactionAsync(this.replicator))
            {
                // Create wave store transaction.
                IStoreReadWriteTransaction waveStoreTxn = this.waveStore.CreateStoreReadWriteTransaction(replicatorTransaction);
                waveStoreTxn.Isolation = StoreTransactionReadIsolationLevel.ReadRepeatable;
                waveStoreTxn.LockingHints = LockingHints.UpdateLock;

                // Receive message.
                IInboundStreamMessage message = await inboundStream.ReceiveAsync(replicatorTransaction, Timeout.InfiniteTimeSpan);
                while (null != message.Payload)
                {
                    // Deserialize wave or wave result.

                    DynamicObject dynamicObject;

                    using (MemoryStream stream = new MemoryStream(message.Payload))
                    {
                        BinaryReader reader = new BinaryReader(stream);
                        dynamicObject = dynamicConverter.Read(reader);
                    }
                        //dynamicConverter.FromEnumerableByteArray(new List<byte[]> { message.Payload });
                    if (dynamicObject is Wave)
                    {
                        // Set wave. This is a new object, not from the store.
                        wave = (Wave)dynamicObject;

                        FabricEvents.Events.ReceiveWave(this.tracer, wave.Id, inboundStream.StreamName.OriginalString, inboundStream.PartnerIdentity.ServiceInstanceName.OriginalString, this.GetTraceFromPartitionKey(inboundStream.PartnerIdentity), replicatorTransaction.ToString());

                        // Check to see if this wave was seen before. It could have arrived from a different wave processor on a different route.
                        ConditionalValue<DynamicObject> wasWaveSeen = await this.waveStore.GetAsync(waveStoreTxn, wave.Id, Timeout.InfiniteTimeSpan, cancellationToken);
                        if (!wasWaveSeen.HasValue)
                        {
                            // New wave was received.
                            wave.State = WaveState.Created;
                            wave.SetInboundLink(inboundStream.PartnerIdentity);

                            // Retrieve the wave processor for this wave.
                            wave.Processor = await this.GetWaveProcessorAsync(wave, TimeSpan.FromSeconds(WaveController.ProcessingTimeoutInSeconds));

                            // Call the wave processor for the arrival of this new wave.
                            outboundLinks = await wave.Processor.OnWaveStartedAsync(replicatorTransaction, wave, inboundStream.PartnerIdentity, cancellationToken);

                            // Process outbound links for duplicates.
                            outboundLinks = outboundLinks.Distinct(equalityComparer).Where(x => !equalityComparer.Equals(x, inboundLink));

                            // Check to see if the wave needs to be echoed of forwarded.
                            if (0 == outboundLinks.Count())
                            {
                                // The wave is not created in the wave store in this case.
                                FabricEvents.Events.Created(this.tracer, wave.Id, "leaf", replicatorTransaction.ToString());

                                // Get the wave result. This is now treated as a completed leaf.
                                waveFeedbackResult = await wave.Processor.OnWaveCompletedEchoAsync(replicatorTransaction, wave, cancellationToken);

                                // Find the outgoing stream. This must exist since it was created before the incoming stream started processing.
                                outboundStreamName = this.CreateFeedbackOutboundStreamName(inboundStream.StreamName);
                                outboundStream = (await this.streamStore.CreateOutboundStreamsEnumerableAsync()).Where(x => equalityComparer.Equals(inboundStream.PartnerIdentity, x.PartnerIdentity) && 0 == Uri.Compare(x.StreamName, outboundStreamName, UriComponents.HttpRequestUrl, UriFormat.UriEscaped, StringComparison.Ordinal)).FirstOrDefault();
                                Diagnostics.Assert(null != outboundStream, WaveTypeName, "outbound stream {0} must exist", outboundStreamName);

                                // Send back echoes.
                                await this.PropagateEchoAsync(replicatorTransaction, wave, "leaf", outboundStream, waveFeedbackResult);

                                FabricEvents.Events.Completed(this.tracer, wave.Id, "leaf", replicatorTransaction.ToString());
                            }
                            else
                            {
                                FabricEvents.Events.Created(this.tracer, wave.Id, "non-leaf", replicatorTransaction.ToString());

                                // Create outbound streams for this wave on each outbound link.
                                outboundStreams = await this.CreateOutboundStreamsAsync(replicatorTransaction, wave, outboundLinks);

                                // Set wave outbound links. The inbound link is already set.
                                wave.SetOutboundLinks(outboundLinks);

                                // Find the outgoing stream. This must exist since it was created before the incoming stream started processing.
                                outboundStreamName = this.CreateFeedbackOutboundStreamName(inboundStream.StreamName);
                                outboundStream = (await this.streamStore.CreateOutboundStreamsEnumerableAsync()).Where(x => equalityComparer.Equals(inboundStream.PartnerIdentity, x.PartnerIdentity) && 0 == Uri.Compare(x.StreamName, outboundStreamName, UriComponents.HttpRequestUrl, UriFormat.UriEscaped, StringComparison.Ordinal)).FirstOrDefault();
                                Diagnostics.Assert(null != outboundStream, WaveTypeName, "outbound stream {0} must exist", outboundStreamName);
                                wave.SetOutboundFeedbackStream(outboundStream.StreamName);

                                // Reset the outbound stream, otherwise it will get closed.
                                outboundStream = null;

                                // Add a new wave in the wave store.
                                await this.waveStore.AddAsync(waveStoreTxn, wave.Id, wave, Timeout.InfiniteTimeSpan, cancellationToken);

                                // Enlist for transaction complete.
                                replicatorTransaction.AddLockContext(new WaveLockContext(this, wave.Id, outboundStreams, replicatorTransaction));
                            }
                        }
                        else
                        {
                            // Retrieve the wave processor for this wave. This wave is non-leaf since if it was leaf it would not exist.
                            wave = (Wave)wasWaveSeen.Value;

                            FabricEvents.Events.Exists(this.tracer, wave.Id, "non-leaf", replicatorTransaction.ToString());

                            // It can happen that the wave processor is null on restart.
                            if (null == wave.Processor)
                            {
                                wave.Processor = await this.GetWaveProcessorAsync(wave, TimeSpan.FromSeconds(WaveController.ProcessingTimeoutInSeconds));
                            }

                            // Get the wave result. This is now treated as a non-completed leaf.
                            waveFeedbackResult = await wave.Processor.OnWaveExistentEchoAsync(replicatorTransaction, wave, inboundStream.PartnerIdentity, cancellationToken);

                            // Find the outgoing stream. This must exist since it was created before the incoming stream was processed.
                            outboundStreamName = this.CreateFeedbackOutboundStreamName(inboundStream.StreamName);
                            outboundStream = (await this.streamStore.CreateOutboundStreamsEnumerableAsync()).Where(x => equalityComparer.Equals(inboundStream.PartnerIdentity, x.PartnerIdentity) && 0 == Uri.Compare(x.StreamName, outboundStreamName, UriComponents.HttpRequestUrl, UriFormat.UriEscaped, StringComparison.Ordinal)).FirstOrDefault();
                            Diagnostics.Assert(null != outboundStream, WaveTypeName, "outbound stream {0} must exist", outboundStreamName);

                            // Send back echoes.
                            await this.PropagateEchoAsync(replicatorTransaction, wave, "non-leaf", outboundStream, waveFeedbackResult);
                        }
                    }
                    else if (dynamicObject is WaveFeedback)
                    {
                        // Set wave feedback.
                        waveFeedback = (WaveFeedback)dynamicObject;

                        FabricEvents.Events.ReceiveFeedback(this.tracer, waveFeedback.WaveId, inboundStream.StreamName.OriginalString, inboundStream.PartnerIdentity.ServiceInstanceName.OriginalString, this.GetTraceFromPartitionKey(inboundStream.PartnerIdentity), replicatorTransaction.ToString());

                        // Retrieve wave. The wave must exist, since in order to receive an echo, it must have been started.
                        ConditionalValue<DynamicObject> waveGetResult = await this.waveStore.GetAsync(waveStoreTxn, waveFeedback.WaveId, Timeout.InfiniteTimeSpan, cancellationToken);
                        Diagnostics.Assert(waveGetResult.HasValue, WaveTypeName, "wave {0} must exist", waveFeedback.WaveId);
                        wave = (Wave)waveGetResult.Value;

                        // It can happen that the wave processor is null on restart.
                        if (null == wave.Processor)
                        {
                            wave.Processor = await this.GetWaveProcessorAsync(wave, TimeSpan.FromSeconds(WaveController.ProcessingTimeoutInSeconds));
                        }

                        // Process incoming echo.
                        await wave.Processor.OnEchoAsync(replicatorTransaction, wave, inboundStream.PartnerIdentity, waveFeedback, cancellationToken);
                    }

                    // Receive next message.
                    message = await inboundStream.ReceiveAsync(replicatorTransaction, Timeout.InfiniteTimeSpan);
                }

                // Complete wave if all echoes have been processed.
                if (null != waveFeedback)
                {
                    // Mark that an echo has been received on this wave for this source.
                    Wave waveClone = (Wave)wave.Clone();
                    waveClone.SetFeedbackReceived(inboundStream.PartnerIdentity);

                    // Check to see if all echoes were received.
                    if (waveClone.AllEchoesReceived)
                    {
                        // Check to see if this is wave initiator processing.
                        if (!waveClone.IsInitiator)
                        {
                            FabricEvents.Events.Completed(this.tracer, waveClone.Id, "non-leaf", replicatorTransaction.ToString());

                            // It can happen that the wave processor is null on restart.
                            if (null == waveClone.Processor)
                            {
                                waveClone.Processor = await this.GetWaveProcessorAsync(waveClone, TimeSpan.FromSeconds(WaveController.ProcessingTimeoutInSeconds));
                            }

                            // Get the wave result.
                            waveFeedbackResult = await wave.Processor.OnWaveCompletedEchoAsync(replicatorTransaction, waveClone, cancellationToken);

                            // Find the outgoing stream. This must exist since it was created before the incoming stream was processed.
                            outboundStreamName = wave.OutboundFeedbackStreamName;
                            outboundStream = (await this.streamStore.CreateOutboundStreamsEnumerableAsync()).Where(x => 0 == Uri.Compare(x.StreamName, outboundStreamName, UriComponents.HttpRequestUrl, UriFormat.UriEscaped, StringComparison.Ordinal)).FirstOrDefault();
                            Diagnostics.Assert(null != outboundStream, WaveTypeName, "outbound stream {0} must exist", outboundStreamName);

                            // Send back echoes.
                            await this.PropagateEchoAsync(replicatorTransaction, waveClone, "non-leaf", outboundStream, waveFeedbackResult);
                        }
                        else
                        {
                            FabricEvents.Events.Completed(this.tracer, waveClone.Id, "initiator", replicatorTransaction.ToString());

                            // It can happen that the wave processor is null on restart.
                            if (null == waveClone.Processor)
                            {
                                waveClone.Processor = await this.GetWaveProcessorAsync(waveClone, TimeSpan.FromSeconds(WaveController.ProcessingTimeoutInSeconds));
                            }

                            // Get the wave result. The wave result will just be returned to the caller.
                            waveFeedbackResult = await wave.Processor.OnWaveCompletedEchoAsync(replicatorTransaction, waveClone, cancellationToken);
                            wasInitiator = true;
                        }

                        // Delete the wave.
                        await this.waveStore.TryRemoveAsync(waveStoreTxn, waveClone.Id, Timeout.InfiniteTimeSpan, cancellationToken);
                    }
                    else
                    {
                        FabricEvents.Events.Process(this.tracer, waveClone.Id, wave.IsInitiator ? "initiator" : "non-leaf", replicatorTransaction.ToString());

                        // Update wave, since its received feedback has changed.
                        await this.waveStore.UpdateAsync(waveStoreTxn, waveClone.Id, waveClone, Timeout.InfiniteTimeSpan, cancellationToken);
                    }
                }

                // Commit replicator transaction.
                await TransactionHelpers.SafeTerminateReplicatorTransactionAsync(replicatorTransaction, true);
            }

            // If this is the initiator, then return to caller.
            if (wasInitiator)
            {
                IWaveResultProcessor waveResultProcessor = await this.GetWaveResultProcessorAsync(wave, TimeSpan.FromSeconds(WaveController.ProcessingTimeoutInSeconds));
                if (null != waveResultProcessor)
                {
                    await waveResultProcessor.OnWaveResult(wave, waveFeedbackResult, CancellationToken.None);
                }
            }

            // If we are done with the outbound stream, then we can close it.
            if (null != outboundStream)
            {
                FabricEvents.Events.OutboundStream(this.tracer, outboundStream.StreamName.OriginalString, "close", outboundStream.PartnerIdentity.ServiceInstanceName.OriginalString, this.GetTraceFromPartitionKey(outboundStream.PartnerIdentity));

                // Wave was completed.
                await outboundStream.CloseAsync(Timeout.InfiniteTimeSpan);

                // Delete the outbound stream.
                using (Transaction transaction = this.replicator.CreateTransaction())
                {
                    FabricEvents.Events.OutboundStream(this.tracer, outboundStream.StreamName.ToString(), "delete", outboundStream.PartnerIdentity.ServiceInstanceName.ToString(), this.GetTraceFromPartitionKey(outboundStream.PartnerIdentity));

                    await outboundStream.DeleteAsync(transaction, Timeout.InfiniteTimeSpan);

                    // Commit transaction.
                    await transaction.CommitAsync();
                }
            }
        }

        /// <summary>
        /// Retrieves the inbound link for any wave originating in this service instance.
        /// </summary>
        /// <returns>Partition key.</returns>
        private PartitionKey BuildInboundLink()
        {
            // Set the inbound link. This marks the origin point for this wave.
            PartitionKey inboundLink = null;
            switch (this.replicator.StatefulPartition.PartitionInfo.Kind)
            {
                case ServicePartitionKind.Int64Range:
                    inboundLink = new PartitionKey(this.replicator.ServiceContext.ServiceName, ((Int64RangePartitionInformation)this.replicator.StatefulPartition.PartitionInfo).LowKey);
                    break;

                case ServicePartitionKind.Named:
                    inboundLink = new PartitionKey(this.replicator.ServiceContext.ServiceName, ((NamedPartitionInformation)this.replicator.StatefulPartition.PartitionInfo).Name);
                    break;

                case ServicePartitionKind.Singleton:
                    inboundLink = new PartitionKey(this.replicator.ServiceContext.ServiceName);
                    break;
            }

            return inboundLink;
        }

        /// <summary>
        /// Creates a propagate outbound unique stream name for a wave. Deterministic.
        /// </summary>
        /// <param name="wave">Wave propagating on that outbound stream.</param>
        /// <returns>Outbound stream name.</returns>
        private Uri CreatePropagateOutboundStreamName(Wave wave)
        {
            return new Uri(string.Format("{0}:/{1}/{2}#{3}", WaveController.WaveStreamNamePrefix, wave.Id, WaveStreamPropagate, Guid.NewGuid()));
        }

        /// <summary>
        /// Creates an feedback outbound unique stream name for a wave. Deterministic.
        /// </summary>
        /// <param name="inboundStreamName">Wave inbound stream.</param>
        /// <returns>Outbound feedback stream name.</returns>
        private Uri CreateFeedbackOutboundStreamName(Uri inboundStreamName)
        {
            string[] splits = inboundStreamName.OriginalString.Split(new string[] { "/", "#" }, StringSplitOptions.None);
            return new Uri(string.Format("{0}:/{1}/{2}/{3}#{4}", WaveController.WaveStreamNamePrefix, splits[1], WaveStreamFeedback, splits[3], Guid.NewGuid()));
        }

        /// <summary>
        /// Returns the trace string for a partition key.
        /// </summary>
        /// <param name="partitionKey"></param>
        /// <returns>String from partition key.</returns>
        private string GetTraceFromPartitionKey(PartitionKey partitionKey)
        {
            switch (partitionKey.Kind)
            {
                case PartitionKind.Singleton:
                    return "singleton";

                case PartitionKind.Named:
                    return partitionKey.PartitionName;

                case PartitionKind.Numbered:
                    return partitionKey.PartitionRange.IntegerKeyLow.ToString();

                default:
                    throw new NotSupportedException();
            }
        }

        /// <summary>
        /// Creates outbound streams for this wave.
        /// </summary>
        /// <param name="replicatorTransaction">Replicator transaction.</param>
        /// <param name="wave">Wave propagating on these outbound stream.</param>
        /// <param name="outboundLinks">Outbound links that define the outbound streams.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        private async Task<List<IMessageStream>> CreateOutboundStreamsAsync(Transaction replicatorTransaction, Wave wave, IEnumerable<PartitionKey> outboundLinks)
        {
            // Outbound streams created.
            List<IMessageStream> outboundStreams = new List<IMessageStream>();

            // Create outbound streams for this wave on each outbound link.
            Uri outgoingStreamName = this.CreatePropagateOutboundStreamName(wave);
            foreach (PartitionKey x in outboundLinks)
            {
                // Create correspondent outbound stream.
                IMessageStream outboundStream = null;
                switch (x.Kind)
                {
                    case PartitionKind.Singleton:
                        FabricEvents.Events.OutboundStream(this.tracer, outgoingStreamName.OriginalString, "create", x.ServiceInstanceName.OriginalString, "singleton");
                        outboundStream = await this.streamStore.CreateStreamAsync(replicatorTransaction, x.ServiceInstanceName, outgoingStreamName, int.MaxValue);
                        break;

                    case PartitionKind.Named:
                        FabricEvents.Events.OutboundStream(this.tracer, outgoingStreamName.OriginalString, "create", x.ServiceInstanceName.OriginalString, x.PartitionName);
                        outboundStream = await this.streamStore.CreateStreamAsync(replicatorTransaction, x.ServiceInstanceName, x.PartitionName, outgoingStreamName, int.MaxValue);
                        break;

                    case PartitionKind.Numbered:
                        FabricEvents.Events.OutboundStream(this.tracer, outgoingStreamName.OriginalString, "create", x.ServiceInstanceName.OriginalString, x.PartitionRange.IntegerKeyLow.ToString());
                        outboundStream = await this.streamStore.CreateStreamAsync(replicatorTransaction, x.ServiceInstanceName, x.PartitionRange.IntegerKeyLow, outgoingStreamName, int.MaxValue);
                        break;
                }

                // Collect each outbound stream.
                outboundStreams.Add(outboundStream);
            }

            return outboundStreams;
        }

        /// <summary>
        /// Propagates echoes on an outbound stream.
        /// </summary>
        /// <param name="replicatorTransaction">Transaction to make chnages durable.</param>
        /// <param name="wave">Wave for feedback.</param>
        /// <param name="kind">Kind of node.</param>
        /// <param name="outboundStream">Outbound stream to send feedback on.</param>
        /// <param name="waveFeedbackResult">Feedback results.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        private async Task PropagateEchoAsync(Transaction replicatorTransaction, Wave wave, string kind, IMessageStream outboundStream, IEnumerable<WaveFeedback> waveFeedbackResult)
        {
            // Send back echoes.
            foreach (WaveFeedback x in waveFeedbackResult)
            {
                // Set the wave id.
                x.WaveId = wave.Id;

                // Serialize wave result.
                byte[] data;
                using (MemoryStream stream = new MemoryStream())
                {
                    DynamicByteConverter waveFeedbackConverter = new DynamicByteConverter();
                    BinaryWriter writer = new BinaryWriter(stream);
                    waveFeedbackConverter.Write(x, x, writer);
                    data = stream.ToArray();
                }

                FabricEvents.Events.SendFeedback(this.tracer, wave.Id, kind, outboundStream.StreamName.OriginalString, outboundStream.PartnerIdentity.ServiceInstanceName.OriginalString, this.GetTraceFromPartitionKey(outboundStream.PartnerIdentity), replicatorTransaction.ToString());

                // Send the echo on the incoming link.
                await outboundStream.SendAsync(replicatorTransaction, data, Timeout.InfiniteTimeSpan);
            }
        }

        /// <summary>
        /// Propagates a wave on its outbound streams.
        /// </summary>
        /// <param name="wave">Wave to propagate.</param>
        /// <param name="outboundStreams">Outbounds treams on which to propagate the wave.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        private async Task PropagateWaveAsync(Wave wave, List<IMessageStream> outboundStreams, CancellationToken cancellationToken)
        {
            // Create replicator transaction.
            using (Transaction replicatorTransaction = await TransactionHelpers.SafeCreateReplicatorTransactionAsync(this.replicator))
            {
                // Create wave store transaction.
                IStoreReadWriteTransaction waveStoreTxn = this.waveStore.CreateStoreReadWriteTransaction(replicatorTransaction);

                // Update the wave in the wave store.
                Wave waveClone = (Wave)wave.Clone();
                waveClone.State = WaveState.Started;
                await this.waveStore.UpdateAsync(waveStoreTxn, waveClone.Id, waveClone, Timeout.InfiniteTimeSpan, cancellationToken);

                // Create a new empty wave.
                Wave wavePropagated = new Wave(wave.Command, false);
                wavePropagated.Id = wave.Id;

                // Serialize wave.
                DynamicByteConverter waveConverter = new DynamicByteConverter();
                byte[] data;
                using (MemoryStream stream = new MemoryStream())
                {
                    BinaryWriter writer = new BinaryWriter(stream);
                    waveConverter.Write(wavePropagated, wavePropagated, writer);
                    data = stream.ToArray();
                }

                // Send wave on each outbound stream.
                foreach (IMessageStream outboundStream in outboundStreams)
                {
                    FabricEvents.Events.SendWave(this.tracer, wavePropagated.Id, outboundStream.StreamName.OriginalString, outboundStream.PartnerIdentity.ServiceInstanceName.OriginalString, this.GetTraceFromPartitionKey(outboundStream.PartnerIdentity), replicatorTransaction.ToString());

                    await outboundStream.SendAsync(replicatorTransaction, data, Timeout.InfiniteTimeSpan);
                }

                // Commit replicator transaction. 
                await TransactionHelpers.SafeTerminateReplicatorTransactionAsync(replicatorTransaction, true);
            }
        }

        /// <summary>
        /// Waits until the write and read status are granted.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        private async Task WaitForPrimaryRoleCompletionAsync()
        {
            while (true)
            {
                // Retrieve read/write status.
                PartitionAccessStatus writeStatus = this.replicator.StatefulPartition.WriteStatus;
                PartitionAccessStatus readStatus = this.replicator.StatefulPartition.ReadStatus;
                if (writeStatus == PartitionAccessStatus.Granted && readStatus == PartitionAccessStatus.Granted)
                {
                    // Done.
                    break;
                }

                // Check to see if we need to cancel.
                this.startAsPrimaryAsyncCancellation.Token.ThrowIfCancellationRequested();

                // Retry.
                await Task.Delay(100);
            }
        }

        /// <summary>
        /// Retrieves references to the stores required for execution.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        private async Task CreateOrRetrieveRequiredStateProvidersAsync()
        {
            // Retrieve stream manager.
            IMessageStreamManager streamManagerStateProvider = await StreamManager.GetSingletonStreamManagerAsync(this.replicator, null, Timeout.InfiniteTimeSpan, this.startAsPrimaryAsyncCancellation.Token);
            Diagnostics.Assert(null != streamManagerStateProvider, "cannot find stream store");

            // Register stream prefix with this wave controller.
            await streamManagerStateProvider.RegisterCallbackByPrefixAsync(new Uri(string.Format(CultureInfo.InvariantCulture, "{0}:/", WaveStreamNamePrefix)), this);

            // Retrieve or create wave store state provider.
            TStore<string, DynamicObject, StringComparerOrdinal, StringComparerOrdinal, StringRangePartitioner> waveStore = null;
            ConditionalValue<IStateProvider2> waveStoreResult = this.replicator.TryGetStateProvider(this.stateProviderNameWaveStore);
            if (waveStoreResult.HasValue)
            {
                // Wave store was created already.
                waveStore = waveStoreResult.Value as TStore<string, DynamicObject, StringComparerOrdinal, StringComparerOrdinal, StringRangePartitioner>;
            }
            else
            {
                // Wave store will be created now.
                while (true)
                {
                    bool doneRegistration = false;
                    try
                    {
                        using (Transaction transaction = this.replicator.CreateTransaction())
                        {
                            // Create the required state providers.
                            waveStore = new TStore<string, DynamicObject, StringComparerOrdinal, StringComparerOrdinal, StringRangePartitioner>(this.replicator, this.stateProviderNameWaveStore, StoreBehavior.MultiVersion, false);

                            // Register wave store.
                            await this.replicator.AddStateProviderAsync(transaction, waveStore.Name, waveStore, Timeout.InfiniteTimeSpan, CancellationToken.None);

                            // Commit transaction.
                            await transaction.CommitAsync();
                        }

                        doneRegistration = true;
                    }
                    catch (FabricTransientException)
                    {
                        // Retry.
                    }
                    catch (Exception)
                    {
                        // Done.
                        throw;
                    }

                    if (!doneRegistration)
                    {
                        // Sleep for a while.
                        await Task.Delay(100);
                    }
                    else
                    {
                        break;
                    }
                }
            }

            // Set members.
            this.streamStore = streamManagerStateProvider;
            this.waveStore = waveStore;
        }

        /// <summary>
        /// Retrieves the wave processor for a given wave.
        /// </summary>
        /// <param name="wave">Wave to process.</param>
        /// <param name="timeout">Timeout for this operation.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        private async Task<IWaveProcessor> GetWaveProcessorAsync(IWave wave, TimeSpan timeout)
        {
            PartitionAccessStatus writeStatus = PartitionAccessStatus.Invalid;
            IWaveProcessor waveProcessor = this.GetWaveProcessor(wave);
            if (null == waveProcessor)
            {
                using (CancellationTokenSource cancellationTokenSource = new CancellationTokenSource(timeout))
                {
                    while (true)
                    {
                        await Task.Delay(100);

                        // Retry
                        waveProcessor = this.GetWaveProcessor(wave);
                        if (null != waveProcessor)
                        {
                            break;
                        }

                        // Check if timeout expired.
                        cancellationTokenSource.Token.ThrowIfCancellationRequested();

                        // Check partition status.
                        writeStatus = this.replicator.StatefulPartition.WriteStatus;
                        if (PartitionAccessStatus.NotPrimary == writeStatus)
                        {
                            throw new FabricNotPrimaryException();
                        }
                    }
                }
            }

            return waveProcessor;
        }

        /// <summary>
        /// Retrieves the wave result processor for a given wave.
        /// </summary>
        /// <param name="wave">Wave to process.</param>
        /// <param name="timeout">Timeout for this operation.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        private async Task<IWaveResultProcessor> GetWaveResultProcessorAsync(IWave wave, TimeSpan timeout)
        {
            PartitionAccessStatus writeStatus = PartitionAccessStatus.Invalid;
            IWaveResultProcessor waveResultProcessor = this.GetWaveResultProcessor(wave);
            if (null == waveResultProcessor)
            {
                using (CancellationTokenSource cancellationTokenSource = new CancellationTokenSource(timeout))
                {
                    while (true)
                    {
                        await Task.Delay(100);
                        
                        // Retry.
                        waveResultProcessor = this.GetWaveResultProcessor(wave);
                        if (null != waveResultProcessor)
                        {
                            break;
                        }

                        // Check if timeout expired.
                        cancellationTokenSource.Token.ThrowIfCancellationRequested();

                        // Check partition status.
                        writeStatus = this.replicator.StatefulPartition.WriteStatus;
                        if (PartitionAccessStatus.NotPrimary == writeStatus)
                        {
                            throw new FabricNotPrimaryException();
                        }
                    }
                }
            }

            return waveResultProcessor;
        }
    }
}
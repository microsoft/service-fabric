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
    using System.Globalization;
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

    internal class MessageStream : IMessageStream
    {
        #region fields

        private readonly StreamConsolidatedStore consolidatedStore;
        private OutboundSessionDriver outboundSessionDriver;
        private readonly StreamManager streamManager;
        // SyncPoints used to ensure one Receive at a time in each transaction context
        private readonly ConcurrentDictionary<long, SyncPoint<long>> receiveInTransactionContextSyncpoints
            = new ConcurrentDictionary<long, SyncPoint<long>>();

        // closeMessage is volatile, used only to discharge outstanding receive waiters after an inbound stream is closed
        private InboundBaseStreamWireMessage closeMessage;
        // TODO: is this always accessed under waiterSync?  If so make it a simple dictionary; also consolidate access code in a class
        private readonly ConcurrentDictionary<long, MessageWaiter> receiveWaiters;
        private readonly ConcurrentDictionary<long, InboundBaseStreamWireMessage> receivedMessages;
        private readonly object waiterSync;
        private readonly object openCompletionSync;
        private readonly object removeMessageSync;
        private static long traceCounter = -1;
        private SyncPoint<Guid> pauseSyncPoint;

        #endregion

        #region Properties

        public StreamKind Direction { get; private set; }

        public Guid StreamIdentity { get; private set; }

        public Uri StreamName { get; private set; }

        public PartitionKey PartnerIdentity { get; private set; }

        public int MessageQuota { get; private set; }

        public PersistentStreamState CurrentState { get; private set; }

        public long CloseMessageSequenceNumber { get; private set; }

        internal string Tracer { get; private set; }

        internal string TraceType { get; private set; }

        #endregion

        #region cstor

        internal MessageStream(
            StreamKind direction,
            PartitionKey partnerId,
            Guid streamId,
            Uri streamName,
            int sendQuota,
            StreamConsolidatedStore consolidatedStore,
            OutboundSessionDriver sessionDriver,
            StreamManager streamManager,
            PersistentStreamState currentState,
            long closeMessageSequenceNumber = StreamConstants.InitialValueOfLastSequenceNumberInStream)
        {
            this.PartnerIdentity = partnerId;
            this.StreamIdentity = streamId;
            this.CurrentState = currentState;
            this.StreamName = streamName;
            this.MessageQuota = sendQuota;
            this.Direction = direction;
            this.consolidatedStore = consolidatedStore;
            this.outboundSessionDriver = sessionDriver;
            this.streamManager = streamManager;
            this.CloseMessageSequenceNumber = closeMessageSequenceNumber;
            this.pauseSyncPoint = null;
            this.receivedMessages = new ConcurrentDictionary<long, InboundBaseStreamWireMessage>();
            this.receiveWaiters = new ConcurrentDictionary<long, MessageWaiter>();
            this.waiterSync = new object();
            this.openCompletionSync = new object();
            this.removeMessageSync = new object();

            if (this.CloseMessageSequenceNumber == StreamConstants.InitialValueOfLastSequenceNumberInStream)
            {
                this.closeMessage = null;
            }
            else if (this.Direction == StreamKind.Inbound)
            {
                // this is in the recovery case for a closed inbound stream: we reconstruct the close message for any receive operations that may occur
                var tempMessage = new OutboundCloseStreamWireMessage(this.StreamIdentity, this.CloseMessageSequenceNumber);
                var closeMessageData = (IOperationData) tempMessage;
                this.closeMessage = new InboundBaseStreamWireMessage(closeMessageData);
            }

            this.Tracer = this.streamManager.Tracer + " Direction: " + this.Direction + "StreamName: " + this.StreamName + " StreamID: " + this.StreamIdentity
                          + " Partner: " + this.PartnerIdentity;

            this.TraceType = this.streamManager.TraceType;
        }

        #endregion

        #region Stream API Handlers

        // applies to payload messages only
        /// <summary>
        /// Last payload sequence number sent as seen in the committed state outside the context of any service transaction
        /// This is only meaningful for outbound streams, use with inbound streams will throw an InvalidOperationException
        /// </summary>
        /// <returns>last sequence number sent</returns>
        public async Task<long> GetLastSequenceNumberSentAsync()
        {
            if (this.Direction == StreamKind.Inbound)
            {
                throw new ReliableStreamInvalidOperationException(string.Format(CultureInfo.CurrentCulture, SR.Error_Stream_GetLSNSent_Invoked_Inbound));
            }

            if ((this.CurrentState == PersistentStreamState.Initialized) || (this.CurrentState == PersistentStreamState.Opening))
            {
                throw new ReliableStreamInvalidOperationException(
                    string.Format(CultureInfo.CurrentCulture, SR.Error_Stream_GetLSN_Invoked_UnopenedStream, this.CurrentState));
            }

            var isOperating = await this.streamManager.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state
                throw new FabricNotPrimaryException();
            }

            try
            {
                using (var replicatorTransaction = await this.streamManager.CreateReplicatorTransactionAsync(Timeout.InfiniteTimeSpan))
                {
                    var readTransaction = this.consolidatedStore.CreateStoreReadWriteTransaction(replicatorTransaction);
                    var sendSeqNumKey = new StreamMetadataKey(this.StreamIdentity, StreamMetadataKind.SendSequenceNumber);

                    var getSendSeqNumResult =
                        await this.consolidatedStore.GetAsync(
                            readTransaction,
                            new StreamStoreConsolidatedKey(sendSeqNumKey),
                            Timeout.InfiniteTimeSpan,
                            CancellationToken.None);

                    Diagnostics.Assert(getSendSeqNumResult.HasValue, "Send sequence number key not found for active outbound stream");

                    var seqNum = getSendSeqNumResult.Value.MetadataBody as SendStreamSequenceNumber;

                    var result = seqNum.NextSequenceNumber - 1;

                    FabricEvents.Events.GetLastSequenceNumberSent(this.TraceType, this.StreamIdentity.ToString(), result);

                    return result;
                }
            }
            catch (Exception e)
            {
                Tracing.WriteExceptionAsWarning("Stream.GetLastSequenceNumberSentAsync.Exception", e, "{0}", this.Tracer);
                throw;
            }
        }

        // TODO: LastReceiveSequenceNumber fix needed
        // applies to payload messages only
        /// <summary>
        /// Last payload sequence number received as seen in the committed state outside the context of any service transaction
        /// This is only meaningful for inbound streams, use with outbound streams will throw an InvalidOperationException
        /// </summary>
        /// <returns>last sequence number received</returns>
        public async Task<long> GetLastSequenceNumberReceivedAsync()
        {
            if (this.Direction == StreamKind.Outbound)
            {
                throw new ReliableStreamInvalidOperationException(string.Format(CultureInfo.CurrentCulture, SR.Error_Stream_GetLSNReceived_Invoked_Outbound));
            }

            var isOperating = await this.streamManager.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state
                throw new FabricNotPrimaryException();
            }

            try
            {
                using (var replicatorTransaction = await this.streamManager.CreateReplicatorTransactionAsync(Timeout.InfiniteTimeSpan))
                {
                    var readTransaction = this.consolidatedStore.CreateStoreReadWriteTransaction(replicatorTransaction);
                    var receiveSeqNumKey = new StreamMetadataKey(this.StreamIdentity, StreamMetadataKind.ReceiveSequenceNumber);

                    var getReceiveSeqNumResult =
                        await this.consolidatedStore.GetAsync(
                            readTransaction,
                            new StreamStoreConsolidatedKey(receiveSeqNumKey),
                            Timeout.InfiniteTimeSpan,
                            CancellationToken.None);

                    Diagnostics.Assert(getReceiveSeqNumResult.HasValue, "Receive sequence number key not found for active inbound stream");

                    var seqNum = getReceiveSeqNumResult.Value.MetadataBody as ReceiveStreamSequenceNumber;

                    var resultCandidate = seqNum.NextSequenceNumber - 1;

                    // TODO: think about using the new InboundParameters property; more appropriate/consistent?
                    // TODO: document the exact role and reliability of the CloseMessageSequenceNumber in the driver
                    var result = (this.CloseMessageSequenceNumber < 0) || (resultCandidate < this.CloseMessageSequenceNumber)
                        ? resultCandidate
                        : this.CloseMessageSequenceNumber - 1;

                    return result;
                }
            }
            catch (Exception e)
            {
                Tracing.WriteExceptionAsWarning("Stream.GetLastSequenceNumberReceivedAsync.Exception", e, "{0}", this.Tracer);
                throw;
            }
        }

        /// <summary>
        /// Retrieve the delete sequence number of the stream..
        /// </summary>
        /// <returns>Delete Sequence Number for the stream</returns>
        private async Task<long> GetDeleteSequenceNumberAsync()
        {
            if (this.Direction == StreamKind.Inbound)
            {
                throw new ReliableStreamInvalidOperationException(string.Format(CultureInfo.CurrentCulture, SR.Error_Stream_GetDeleted_Invoked_Inbound));
            }

            var isOperating = await this.streamManager.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state
                throw new FabricNotPrimaryException();
            }

            try
            {
                using (var replicatorTransaction = await this.streamManager.CreateReplicatorTransactionAsync(Timeout.InfiniteTimeSpan))
                {
                    var readTransaction = this.consolidatedStore.CreateStoreReadWriteTransaction(replicatorTransaction);

                    var deleteSeqNumKey = new StreamMetadataKey(this.StreamIdentity, StreamMetadataKind.DeleteSequenceNumber);

                    var delSeqNumResult =
                        await this.consolidatedStore.GetAsync(
                            readTransaction,
                            new StreamStoreConsolidatedKey(deleteSeqNumKey),
                            Timeout.InfiniteTimeSpan,
                            CancellationToken.None);

                    Diagnostics.Assert(
                        delSeqNumResult.HasValue,
                        "{0} DeleteSequenceNumber not found for active outbound stream in GetDeleteSequenceNumberAsync",
                        this.Tracer);

                    var seqNum = delSeqNumResult.Value.MetadataBody as DeleteStreamSequenceNumber;
                    var deleteSequenceNumber = seqNum.NextSequenceNumber;

                    FabricEvents.Events.GetDeleteSequenceNumber(this.TraceType, this.StreamIdentity.ToString(), deleteSequenceNumber);
                    return deleteSequenceNumber;
                }
            }
            catch (Exception e)
            {
                Tracing.WriteExceptionAsWarning("Stream.GetDeleteSequenceNumberAsync.Exception", e, "{0}", this.Tracer);
                throw;
            }
        }

        /// <summary>
        /// Only applies to outbound streams -- inbound delete is implicit
        /// A DeleteAsync() call initiates the deletion of the stream via the delete stream protocol.
        /// This captures the intent (stream state set to deleting) in a user supplied transaction, and 
        /// once the service commits the transaction, the delete protocol is initiated.
        /// This may be called when stream is in initialized, closed  or deleting state as this is an idempotent operation.
        /// </summary>
        /// <param name="txn">Service supplied transaction</param>
        /// <param name="timeout">Timeout for the operation to complete</param>
        /// <returns></returns>
        public async Task<bool> DeleteAsync(Transaction txn, TimeSpan timeout)
        {
            txn.ThrowIfNull("Transaction");

            if (!TimeoutHandler.ValidRange(timeout))
            {
                throw new ArgumentOutOfRangeException(SR.Error_Stream_Timeout);
            }

            if (this.Direction != StreamKind.Outbound)
            {
                throw new ReliableStreamInvalidOperationException(SR.Error_Stream_Delete_Inbound);
            }

            var isOperating = await this.streamManager.WaitForOperatingStateAsync();
            if (!isOperating)
            {
                // if we are not in operating state
                throw new FabricNotPrimaryException();
            }

            // The stream can be permanently deleted if in the state of initialized
            // As the service can open the stream in parallel, we take a sync point to ensure inmemory stream state is not mutable for parallel threads
            var syncPoint = new SyncPoint<Guid>(this.streamManager, this.StreamIdentity, this.streamManager.RuntimeResources.PauseStreamSyncpoints);
            try
            {
                await syncPoint.EnterAsync(timeout);

                if (this.CurrentState == PersistentStreamState.Initialized)
                {
                    await
                        this.DeleteOutboundStreamFromConsolidatedStore(
                            txn,
                            this.StreamIdentity,
                            this.StreamName,
                            this.PartnerIdentity,
                            timeout);

                    // As the deletion is in the context of the service transaction, 
                    // We will update stream to deleted state only after the transaction is commited.
                    FabricEvents.Events.DeleteStream("Finish@" + this.TraceType, this.StreamIdentity + "::" + txn);
                    return true;
                }
            }
            catch (Exception e)
            {
                if (e is TimeoutException)
                {
                    FabricEvents.Events.DeleteStreamFailure(
                        "Timeout@" + this.TraceType,
                        this.StreamIdentity.ToString());
                    throw;
                }
                if (e is FabricObjectClosedException)
                {
                    FabricEvents.Events.DeleteStreamFailure(
                        "ObjectClosed@" + this.TraceType,
                        this.StreamIdentity.ToString());
                    throw;
                }
                if (e is FabricNotPrimaryException)
                {
                    FabricEvents.Events.DeleteStreamFailure(
                        "NotPrimary@" + this.TraceType,
                        this.StreamIdentity.ToString());
                    throw;
                }
                if (e is FabricNotReadableException)
                {
                    if (this.streamManager.NotWritable())
                    {
                        FabricEvents.Events.DeleteStreamFailure(
                            "NotPrimary@" + this.TraceType,
                            this.StreamIdentity.ToString());
                        throw new FabricNotPrimaryException();
                    }
                }
            }
            finally
            {
                syncPoint.Dispose();
            }

            // return, if the stream state is already set to deleting, as the intent is already captured
            if (this.CurrentState == PersistentStreamState.Deleting)
            {
                FabricEvents.Events.DeleteStream(this.CurrentState + "@AlreadyDeleting@" + this.TraceType, this.StreamIdentity.ToString());
                return true;
            }

            // The stream received an Delete Ack, on a different thread.
            if (this.CurrentState == PersistentStreamState.Deleted)
            {
                FabricEvents.Events.DeleteStream(this.CurrentState + "@AlreadyDeleted@" + this.TraceType, this.StreamIdentity.ToString());
                return true;
            }

            // If we got here and the state is not Closed we need to throw an error 
            // as stream delete protocol can only be initiated for a closed stream.
            if (this.CurrentState != PersistentStreamState.Closed)
            {
                FabricEvents.Events.DeleteStream(this.CurrentState + "@InvalidState@" + this.TraceType, this.StreamIdentity.ToString());
                throw new ReliableStreamInvalidOperationException(
                    string.Format(CultureInfo.CurrentCulture, SR.Error_Stream_Delete_UnopenedStream, this.CurrentState));
            }

            FabricEvents.Events.DeleteStream("Start@" + this.TraceType, this.StreamIdentity.ToString());
            var remainingTimeout = timeout;

            // We do not need to pause the stream as the stream is already closed at this point.
            try
            {
                // Capture the intent(state == Deleting) at this point.
                var txnLookupResult = this.consolidatedStore.CreateOrFindTransaction(txn);
                var storeTransaction = txnLookupResult.Value;

                var metadataKey = new StreamMetadataKey(this.StreamIdentity, StreamMetadataKind.OutboundStableParameters);
                FabricEvents.Events.DeleteStream("UpdateStable@" + this.TraceType, this.StreamIdentity.ToString());
                await this.consolidatedStore.UpdateWithOutputAsync(
                    storeTransaction,
                    new StreamStoreConsolidatedKey(metadataKey),
                    DeleteAsyncParamsUpdateFunc,
                    remainingTimeout,
                    CancellationToken.None);
                FabricEvents.Events.DeleteStream("Finish@" + this.TraceType, this.StreamIdentity + "::" + txn);
                return true;
            }
            catch (Exception e)
            {
                if (e is TimeoutException)
                {
                    FabricEvents.Events.DeleteStreamFailure("Timeout@" + this.TraceType, this.StreamIdentity.ToString());
                    throw;
                }
                if (e is FabricObjectClosedException)
                {
                    FabricEvents.Events.DeleteStreamFailure("ObjectClosed@" + this.TraceType, this.StreamIdentity.ToString());
                    throw;
                }
                if (e is FabricNotPrimaryException)
                {
                    FabricEvents.Events.DeleteStreamFailure("NotPrimary@" + this.TraceType, this.StreamIdentity.ToString());
                    throw;
                }
                if (e is FabricNotReadableException)
                {
                    if (this.streamManager.NotWritable())
                    {
                        FabricEvents.Events.DeleteStreamFailure("NotPrimary@" + this.TraceType, this.StreamIdentity.ToString());
                        throw new FabricNotPrimaryException();
                    }
                }

                Tracing.WriteExceptionAsError("Stream.DeleteAsync.Failure", e, "{0} Target: {1}", this.Tracer, this.PartnerIdentity);
                Diagnostics.Assert(false, "{0} Target: {1} Stream.DeleteAsync Unexpected Exception {2}", this.Tracer, this.PartnerIdentity, e);
                return false;
            }
        }

        /// <summary>
        /// This is usually called by the consolidatedstore callback after the DeleteStream(outbound) transaction(stream state set to deleting) is commited.
        /// and during restart of the role.
        /// </summary>
        /// <param name="timeout"></param>
        /// <returns></returns>
        internal async Task FinishDeleteAsync(TimeSpan timeout)
        {
            FabricEvents.Events.DeleteStream("StartingToFinish@" + this.TraceType, this.StreamIdentity.ToString());
            var remainingTimeout = timeout;

            // Remember completion (delete ack) could be processed in parallel in a different thread.
            var completion = new TaskCompletionSource<StreamWireProtocolResponse>();

            // If we are here, then the stream deleting has been committed. Now updating stream to 'deleted' state.
            this.CurrentState = PersistentStreamState.Deleting;

            // If a completion is already posted use that one
            completion = this.streamManager.RuntimeResources.DeleteStreamCompletions.GetOrAdd(this.StreamIdentity, completion);

            // Prepare the delete stream request to the partner
            var deleteMessage = new OutboundDeleteStreamWireMessage(this.StreamIdentity);

            // Since Finish Delete is always used in the background, retry this delete protocol until a 'TargetNotified' is received 
            bool retryNeeded;
            var retryCount = StreamConstants.RetryCountForDeleteStream;

            do
            {
                retryNeeded = false;
                // Wait for the continuation response to arrive
                // protect against change role to secondary while we wait -- completion will have FabricNotPrimaryException set if we change role
                using (var changeRoleHandler = new CloseWhenNotPrimary<StreamWireProtocolResponse>(completion, this.streamManager))
                {
                    // check if we lost primary status
                    this.streamManager.ThrowIfNotWritable();

                    // set up to complete the TaskCompletionSource with FabricNotPrimaryException if we change role before finishing
                    var beforeStart = DateTime.UtcNow;
                    await changeRoleHandler.Start(remainingTimeout);
                    var afterStart = DateTime.UtcNow;
                    remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, beforeStart, afterStart);

                    this.outboundSessionDriver.Send(deleteMessage);
                    FabricEvents.Events.DeleteRequestSent(this.TraceType, this.StreamIdentity.ToString());

                    var beforeWaitWithDelay = DateTime.UtcNow;
                    var finished = await TimeoutHandler.WaitWithDelay(remainingTimeout, completion.Task);
                    var afterWaitWithDelay = DateTime.UtcNow;
                    remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, beforeWaitWithDelay, afterWaitWithDelay);

                    // the completion task is never cancelled or faulted
                    if (finished && completion.Task.Status == TaskStatus.RanToCompletion)
                    {
                        var response = completion.Task.Result;
                        switch (response)
                        {
                            case StreamWireProtocolResponse.TargetNotified:
                            case StreamWireProtocolResponse.StreamNotFound:
                                return;
                            // Target not ready case is not required as we check if the callback object exists 
                            // prior to target notification
                            default:
                                Diagnostics.Assert(false, "{0} Unexpected response {1} to DeleteStream message", this.Tracer, response);
                                break;
                        }
                    }
                    else if (finished && completion.Task.Status == TaskStatus.Canceled)
                    {
                        FabricEvents.Events.DeleteStreamFailure("Canceled@" + this.TraceType, this.StreamIdentity.ToString());
                        throw new OperationCanceledException();
                    }
                    else if (finished && completion.Task.Status == TaskStatus.Faulted)
                    {
                        Diagnostics.Assert(
                            completion.Task.Exception.InnerException is FabricNotPrimaryException ||
                            completion.Task.Exception.InnerException is FabricObjectClosedException,
                            "{0} FinishDeleteAsync Completion Faulted with Unexpected exception {1}",
                            this.Tracer,
                            completion.Task.Exception.InnerException);

                        FabricEvents.Events.DeleteStreamFailure("NotPrimary@" + this.TraceType, this.StreamIdentity.ToString());
                        throw new FabricNotPrimaryException();
                    }
                    else
                    {
                        FabricEvents.Events.DeleteStreamFailure("Timeout@" + this.TraceType, this.StreamIdentity.ToString());
                        retryNeeded = true;
                    }
                }

                await Task.Delay(StreamConstants.MaxDelayForDeleteStreamRetry);
            } while (retryNeeded && retryCount-- > 0);

            Diagnostics.Assert(
                retryCount > 0,
                "{0} Retries exhausted and encountered failure in delete completion with Response: {1}",
                this.Tracer,
                completion.Task.Result);
            FabricEvents.Events.DeleteStreamFailure("RetriesExhausted@" + this.TraceType, this.StreamIdentity.ToString());
        }

        /// <summary>
        /// Handler in lieu of await for FinishDeleteAsync call
        /// </summary>
        /// <param name="task"></param>
        internal void FinishDeleteAsyncContinuation(Task task)
        {
            if (task.Exception != null)
            {
                Tracing.WriteExceptionAsError("FinishDeleteAsync", task.Exception, "{0}", this.Tracer);
            }
        }

        /// <summary>
        /// This is called(via consolidated store call back)  when the outbound stream delete tx is commited.
        /// The in memory stream state is set to Deleted and the stream is removed from the runtime resources outbound dictionary.
        /// </summary>
        internal void OutboundStreamDeleted()
        {
            FabricEvents.Events.OutboundStreamDeleted("Start@" + this.TraceType, this.StreamIdentity.ToString());

            this.CurrentState = PersistentStreamState.Deleted;
            MessageStream value = null;
            this.streamManager.RuntimeResources.OutboundStreams.TryRemove(this.StreamIdentity, out value);

            FabricEvents.Events.OutboundStreamDeleted("Finish@" + this.TraceType, this.StreamIdentity.ToString());
        }

        // Only applies to outbound streams -- inbound open is implicit
        /// <summary>
        /// Make the stream operational by negotiating acceptance with the target partition
        /// This is only meaningful for outbound streams, use with inbound streams will throw an InvalidOperationException
        /// </summary>
        /// <returns>true if Open succeeded, false if the stream cannot be started because the name cannot be resolved 
        /// or a connection cannot be established or the stream is rejected by the target</returns>
        public async Task<StreamOpenOutcome> OpenAsync(TimeSpan timeout)
        {
            if (!TimeoutHandler.ValidRange(timeout))
            {
                throw new ArgumentOutOfRangeException(SR.Error_Stream_Timeout);
            }

            if (this.Direction != StreamKind.Outbound)
            {
                throw new ReliableStreamInvalidOperationException(SR.Error_Stream_Open_Inbound);
            }

            var isOperating = await this.streamManager.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state
                throw new FabricNotPrimaryException();
            }

            if (this.CurrentState == PersistentStreamState.Open)
            {
                // we need to account for the possibility that the Opening->Open transition occurred automatically during restart
                // The service needs to be able to perform OpenAsync on recovery in case that was not completed
                // we do need to perform the Opening->Open transition automatically during RestartOutbound because RestartOutbound 
                // may occur as a result of a reset message from the receiver the partner while the service that owns the outbound stream is 
                // awaiting OpenAsync already  -- there are so many races that idempotency is the most robust solution
                return StreamOpenOutcome.PartnerAccepted;
            }

            if ((this.CurrentState != PersistentStreamState.Initialized) && (this.CurrentState != PersistentStreamState.Opening))
            {
                FabricEvents.Events.OpenStream(
                    this.CurrentState + "@InvalidState@" + this.TraceType,
                    this.StreamIdentity.ToString(),
                    this.PartnerIdentity.ToString());
                throw new ReliableStreamInvalidOperationException(SR.Error_Stream_Open_Outbound_Closing);
            }

            FabricEvents.Events.OpenStream("Start@" + this.TraceType, this.StreamIdentity.ToString(), this.PartnerIdentity.ToString());

            var remainingTimeout = timeout;

            PartitionKey resolvedKey = null;

            if (this.PartnerIdentity.Kind == PartitionKind.Numbered &&
                (this.PartnerIdentity.PartitionRange.IntegerKeyLow == this.PartnerIdentity.PartitionRange.IntegerKeyHigh))
            {
                try
                {
                    var beforeResolve = DateTime.UtcNow;
                    resolvedKey = await this.streamManager.SessionConnectionManager.ResolveAndNormalizeTargetPartition(this.PartnerIdentity, remainingTimeout);
                    var afterResolve = DateTime.UtcNow;
                    remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, beforeResolve, afterResolve);
                }
                catch (Exception e)
                {
                    if (e is FabricObjectClosedException)
                    {
                        FabricEvents.Events.OpenStreamFailure(
                            "ObjectClosed@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            this.PartnerIdentity.ToString());
                    }
                    if (e is TimeoutException)
                    {
                        FabricEvents.Events.OpenStreamFailure("Timeout@" + this.TraceType, this.StreamIdentity.ToString(), this.PartnerIdentity.ToString());
                    }
                    else
                    {
                        Tracing.WriteExceptionAsError(
                            "Unexpected Exception in OpenStream calling ResolveAndNormalizeTargetPartition",
                            e,
                            "{0} StreamId = {1} PartnerId = {2}",
                            this.TraceType,
                            this.StreamIdentity,
                            this.PartnerIdentity);
                    }

                    throw;
                }
            }
            else
            {
                resolvedKey = this.PartnerIdentity;
            }

            // Must take lock on resolved key since that is the lock the reset process will take
            var partnerResetSyncPoint = new SyncPoint<PartitionKey>(this.streamManager, resolvedKey, this.streamManager.RuntimeResources.PartnerResetSyncpoints);

            var pauseStreamSyncPoint = new SyncPoint<Guid>(this.streamManager, this.StreamIdentity, this.streamManager.RuntimeResources.PauseStreamSyncpoints);

            try
            {
                var beforeEnterPartnerResetSyncPoint = DateTime.UtcNow;

                // we cannot proceed with open if a partner reset is in progress for the target partition
                // this lock is always taken before the pauseStream lock to avoid deadlock
                await partnerResetSyncPoint.EnterAsync(remainingTimeout);

                var afterEnterPartnerResetSyncPoint = DateTime.UtcNow;

                remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, beforeEnterPartnerResetSyncPoint, afterEnterPartnerResetSyncPoint);

                await pauseStreamSyncPoint.EnterAsync(remainingTimeout);

                var afterEnterPauseStreamSyncPoint = DateTime.UtcNow;

                remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, afterEnterPartnerResetSyncPoint, afterEnterPauseStreamSyncPoint);

                if (this.CurrentState == PersistentStreamState.Initialized)
                {
                    var startStateChange = DateTime.UtcNow;
                    // Its possible that the stream that was created(using CreateStream(Tx) in a service driven transaction was newer commited by the service.
                    // thereby we are opening an store transaction under read isolation(ensures protection from dirty reads)
                    // to ensure the stream has been commited to the durable consolidated store before we proceed with the
                    // OpenAsync() operation.
                    using (var replicatorTransaction = await this.streamManager.CreateReplicatorTransactionAsync(remainingTimeout))
                    {
                        var afterTxnCreate = DateTime.UtcNow;
                        remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, startStateChange, afterTxnCreate);

                        var metadataStoreTransaction = this.consolidatedStore.CreateStoreReadWriteTransaction(replicatorTransaction);
                        metadataStoreTransaction.Isolation = Store.StoreTransactionReadIsolationLevel.ReadCommitted;
                        metadataStoreTransaction.LockingHints = Store.LockingHints.UpdateLock;

                        var metadataKey = new StreamMetadataKey(this.StreamIdentity, StreamMetadataKind.OutboundStableParameters);

                        var getResult = await this.consolidatedStore.GetAsync(
                            metadataStoreTransaction,
                            new StreamStoreConsolidatedKey(metadataKey),
                            remainingTimeout,
                            CancellationToken.None);

                        if (!getResult.HasValue)
                        {
                            throw new ReliableStreamInvalidOperationException(SR.Error_Stream_Open_UncommittedTx);
                        }

                        var parameters = new OutboundStreamStableParameters((OutboundStreamStableParameters) getResult.Value.MetadataBody);

                        Diagnostics.Assert(
                            parameters.CurrentState == PersistentStreamState.Initialized,
                            "{0} Unexpected persistent state {1} during OpenAsync critsec ",
                            this.Tracer,
                            parameters.CurrentState);

                        var afterGet = DateTime.UtcNow;
                        remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, afterTxnCreate, afterGet);

                        // stream exists, make sure we can communicate with the target
                        this.outboundSessionDriver = await OutboundSessionDriver.FindOrCreateDriverAsync(
                            this.PartnerIdentity,
                            this.streamManager,
                            this.streamManager.SessionConnectionManager,
                            remainingTimeout);

                        var afterDriverAcquired = DateTime.UtcNow;
                        remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, afterGet, afterDriverAcquired);

                        await this.consolidatedStore.UpdateWithOutputAsync(
                            metadataStoreTransaction,
                            new StreamStoreConsolidatedKey(metadataKey),
                            OpenAsyncParamsUpdateFunc,
                            remainingTimeout,
                            CancellationToken.None);

                        FabricEvents.Events.OpenStream(
                            "CompletedUpdate@" + replicatorTransaction + "@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            this.PartnerIdentity.ToString());

                        // once we update successfully we commit regardless of timeout
                        await this.streamManager.CompleteReplicatorTransactionAsync(replicatorTransaction, TransactionCompletionMode.Commit);
                        FabricEvents.Events.OpenStream(
                            "CommittedUpdate@" + replicatorTransaction + "@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            this.PartnerIdentity.ToString());

                        this.CurrentState = PersistentStreamState.Opening;

                        var afterTxnComplete = DateTime.UtcNow;
                        remainingTimeout = TimeoutHandler.UpdateTimeout(timeout, afterDriverAcquired, afterTxnComplete);
                    }
                }
            }
            catch (Exception e)
            {
                if (e is TimeoutException)
                {
                    FabricEvents.Events.OpenStreamFailure("Timeout@" + this.TraceType, this.StreamIdentity.ToString(), this.PartnerIdentity.ToString());
                    throw;
                }
                if (e is FabricObjectClosedException)
                {
                    FabricEvents.Events.OpenStreamFailure("ObjectClosed@" + this.TraceType, this.StreamIdentity.ToString(), this.PartnerIdentity.ToString());
                    throw;
                }
                if (e is FabricNotPrimaryException)
                {
                    FabricEvents.Events.OpenStreamFailure("NotPrimary@" + this.TraceType, this.StreamIdentity.ToString(), this.PartnerIdentity.ToString());
                    throw;
                }
                if (e is ReliableStreamInvalidOperationException)
                {
                    throw;
                }
                if (e is FabricNotReadableException)
                {
                    if (this.streamManager.NotWritable())
                    {
                        FabricEvents.Events.OpenStreamFailure(
                            "NotPrimary@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            this.PartnerIdentity.ToString());
                        throw new FabricNotPrimaryException();
                    }
                }
                if (e is FabricElementNotFoundException)
                {
                    FabricEvents.Events.OpenStreamFailure(
                        "ServiceNotFound@" + this.TraceType,
                        this.StreamIdentity.ToString(),
                        this.PartnerIdentity.ToString());
                    throw;
                }

                Tracing.WriteExceptionAsError("Stream.OpenAsync.Failure", e, "{0} Target: {1} Unexpected", this.Tracer, this.PartnerIdentity);
                Diagnostics.Assert(false, "{0} Target: {1} Stream.OpenAsync Unexpected Exception {2}", this.Tracer, this.PartnerIdentity, e);
            }
            finally
            {
                partnerResetSyncPoint.Dispose();
                pauseStreamSyncPoint.Dispose();
            }

            FabricEvents.Events.OpenStream("WaitingToFinish@" + this.TraceType, this.StreamIdentity.ToString(), this.PartnerIdentity.ToString());

            try
            {
                var result = await this.FinishOpenAsync(remainingTimeout);

                FabricEvents.Events.OpenStream(result + "@" + this.TraceType, this.StreamIdentity.ToString(), this.PartnerIdentity.ToString());
                return result;
            }
            catch (Exception e)
            {
                Diagnostics.Assert(
                    e is TimeoutException || e is FabricNotPrimaryException || e is FabricObjectClosedException,
                    "{0} Target: {1} Stream.FinishOpenAsync threw unexpected Exception {2}",
                    this.Tracer,
                    this.PartnerIdentity,
                    e);

                throw;
            }
        }

        /// <summary>
        /// Only applies to outbound streams -- inbound close is implicit
        /// This may be called in Closing state on recovery to await completion of the closure protocol, specifically the signal that
        ///    the receiver service has drained the stream
        /// After closure the SendSequenceNumber in an outbound stream's metadata is the sequence number of the close message
        /// After closure the Receive in an inbound stream's metadata is greater than the sequence number of the close message
        /// The reason for the discrepancy is that on the receive side the fact that a message is a CloseStream message is usually discovered
        /// while fulfilling a ReceiveAsync request -- however that ReceiveAsync request and any concurrent receive requests have already caused 
        /// the Receive to be incremented -- the actual close message sequence number is a metadata item in the InboundStableParameters
        /// </summary>
        /// <param name="timeout"></param>
        /// <returns></returns>
        public async Task<bool> CloseAsync(TimeSpan timeout)
        {
            if (!TimeoutHandler.ValidRange(timeout))
            {
                throw new ArgumentOutOfRangeException(SR.Error_Stream_Timeout);
            }

            if (this.Direction != StreamKind.Outbound)
            {
                throw new ReliableStreamInvalidOperationException(SR.Error_Stream_Close_Inbound);
            }

            var isOperating = await this.streamManager.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state
                throw new FabricNotPrimaryException();
            }

            if (this.CurrentState == PersistentStreamState.Closed)
            {
                // we need to account for the possibility that the Closing->Closed transition occurred automatically during restart
                // The service needs to be able to perform CloseAsync on recovery in case that was not completed
                // we do need to perform the Closing->Closed transition automatically during RestartOutbound because RestartOutbound 
                // may occur as a result of a reset message from the receiver while the service that owns the outbound stream is
                // awaiting CloseAsync already -- there are so many races that idempotency is the most robust solution
                return true;
            }

            // If we got here and the state is not Open or Closing it has to be Initialized or Opening, which is not allowed
            if ((this.CurrentState != PersistentStreamState.Open) && (this.CurrentState != PersistentStreamState.Closing))
            {
                FabricEvents.Events.CloseStream(this.CurrentState + "@InvalidState@" + this.TraceType, this.StreamIdentity.ToString());
                throw new ReliableStreamInvalidOperationException(
                    string.Format(CultureInfo.CurrentCulture, SR.Error_Stream_Close_Unopened, this.CurrentState));
            }

            FabricEvents.Events.CloseStream("Start@" + this.TraceType, this.StreamIdentity.ToString());

            var remainingTimeout = timeout;

            var syncPoint = new SyncPoint<Guid>(this.streamManager, this.StreamIdentity, this.streamManager.RuntimeResources.PauseStreamSyncpoints);

            try
            {
                // Pause the stream as we attempt to close--the transition to Closing state will occur after we successfully commit the persistent state change 
                // but in the mean time we do not want any Sends to occur as we are computing the CloseMessageSequenceNumber

                var beginEnter = DateTime.UtcNow;
                await syncPoint.EnterAsync(remainingTimeout);

                if (this.CurrentState == PersistentStreamState.Open)
                {
                    var startStateChange = DateTime.UtcNow;
                    remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, beginEnter, startStateChange);

                    using (var replicatorTransaction = await this.streamManager.CreateReplicatorTransactionAsync(remainingTimeout))
                    {
                        var storeTransaction = this.consolidatedStore.CreateStoreReadWriteTransaction(replicatorTransaction);
                        storeTransaction.Isolation = Store.StoreTransactionReadIsolationLevel.ReadCommitted;
                        storeTransaction.LockingHints = Store.LockingHints.UpdateLock;

                        var sendSeqNumKey = new StreamMetadataKey(this.StreamIdentity, StreamMetadataKind.SendSequenceNumber);

                        var pastTxnCreate = DateTime.UtcNow;
                        remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, startStateChange, pastTxnCreate);

                        FabricEvents.Events.CloseStream("GetSend@" + this.TraceType, this.StreamIdentity.ToString());
                        var getSeqNumResult =
                            await this.consolidatedStore.GetAsync(
                                storeTransaction,
                                new StreamStoreConsolidatedKey(sendSeqNumKey),
                                remainingTimeout,
                                CancellationToken.None);

                        Diagnostics.Assert(
                            getSeqNumResult.HasValue,
                            "{0} Failed to find SendSequenceNumber in CloseAsync",
                            this.Tracer);

                        var pastGet = DateTime.UtcNow;
                        remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, pastTxnCreate, pastGet);

                        var nextSeqNum = (SendStreamSequenceNumber) getSeqNumResult.Value.MetadataBody;

                        var metadataKey = new StreamMetadataKey(this.StreamIdentity, StreamMetadataKind.OutboundStableParameters);

                        FabricEvents.Events.CloseStream("UpdateStable@" + this.TraceType, this.StreamIdentity.ToString());
                        // TODO: deal with exceptions
                        var updateStateResult =
                            await this.consolidatedStore.UpdateWithOutputAsync(
                                storeTransaction,
                                new StreamStoreConsolidatedKey(metadataKey),
                                (key, value) => { return CloseAsyncParamsUpdateFunc(nextSeqNum.NextSequenceNumber, key, value); },
                                remainingTimeout,
                                CancellationToken.None);

                        // Once past update we commit regardless of timeout
                        await this.streamManager.CompleteReplicatorTransactionAsync(replicatorTransaction, TransactionCompletionMode.Commit);

                        this.CloseMessageSequenceNumber = nextSeqNum.NextSequenceNumber;
                        this.CurrentState = PersistentStreamState.Closing;

                        var pastTxnComplete = DateTime.UtcNow;
                        remainingTimeout = TimeoutHandler.UpdateTimeout(timeout, pastGet, pastTxnComplete);
                    }
                }
            }
            catch (Exception e)
            {
                if (e is TimeoutException)
                {
                    FabricEvents.Events.CloseStreamFailure("Timeout@" + this.TraceType, this.StreamIdentity.ToString());
                    throw;
                }
                if (e is FabricObjectClosedException)
                {
                    FabricEvents.Events.CloseStreamFailure("ObjectClosed@" + this.TraceType, this.StreamIdentity.ToString());
                    throw;
                }
                if (e is FabricNotPrimaryException)
                {
                    FabricEvents.Events.CloseStreamFailure("NotPrimary@" + this.TraceType, this.StreamIdentity.ToString());
                    throw;
                }
                if (e is FabricNotReadableException)
                {
                    if (this.streamManager.NotWritable())
                    {
                        FabricEvents.Events.CloseStreamFailure("NotPrimary@" + this.TraceType, this.StreamIdentity.ToString());
                        throw new FabricNotPrimaryException();
                    }
                }

                Tracing.WriteExceptionAsError("Stream.CloseAsync.Failure", e, "{0} Target: {1}", this.Tracer, this.PartnerIdentity);
                Diagnostics.Assert(false, "{0} Target: {1} Stream.CloseAsync Unexpected Exception {2}", this.Tracer, this.PartnerIdentity, e);
            }
            finally
            {
                syncPoint.Dispose();
            }

            if (this.CurrentState == PersistentStreamState.Closing)
            {
                Diagnostics.Assert(
                    this.CloseMessageSequenceNumber > 0,
                    "{0} found invalid close sequence number {1} in Closing state",
                    this.Tracer,
                    this.CloseMessageSequenceNumber);
            }


            try
            {
                FabricEvents.Events.CloseStream("WaitingToFinish@" + this.TraceType, this.StreamIdentity.ToString());
                var result = await this.FinishCloseAsync(this.CloseMessageSequenceNumber, remainingTimeout);
                FabricEvents.Events.CloseStream("Finish@" + this.TraceType, this.StreamIdentity.ToString());
                return result;
            }
            catch (Exception e)
            {
                Diagnostics.Assert(
                    e is TimeoutException || e is FabricNotPrimaryException,
                    "{0} Target: {1} Stream.FinishCloseAsync threw unexpected Exception {2}",
                    this.Tracer,
                    this.PartnerIdentity,
                    e);

                throw;
            }
        }

        /// <summary>
        /// Async, return to continuation when inbound message available, wait for messages to come in if necessary
        /// Will return a null payload once the stream is closed
        /// This is only meaningful for inbound streams, use with outbound streams will throw an InvalidOperationException
        /// </summary>
        /// <returns></returns>
        public async Task<IInboundStreamMessage> ReceiveAsync(Transaction txn, TimeSpan timeout)
        {
            txn.ThrowIfNull("Transaction");

            if (!TimeoutHandler.ValidRange(timeout))
            {
                throw new ArgumentOutOfRangeException(SR.Error_Stream_Timeout);
            }

            if (this.Direction != StreamKind.Inbound)
            {
                throw new ReliableStreamInvalidOperationException(SR.Error_Stream_Receive_Outbound);
            }

            var isOperating = await this.streamManager.WaitForOperatingStateAsync();
            if (!isOperating)
            {
                // if we are not in operating state
                throw new FabricNotPrimaryException();
            }

            var traceId = Interlocked.Increment(ref traceCounter);

            var txnLookupResult = this.consolidatedStore.CreateOrFindTransaction(txn);
            var storeTransaction = txnLookupResult.Value;

            var receiveSeqNumKey = new StreamMetadataKey(this.StreamIdentity, StreamMetadataKind.ReceiveSequenceNumber);
            var transactionId = storeTransaction.Id;

            var remainingTimeout = timeout;

            long receiveSSN = -1;

            FabricEvents.Events.Receive("Start::" + traceId + "@" + this.TraceType, this.StreamIdentity.ToString(), receiveSSN, txn.ToString());

            // we allow only one receive at a time per transaction to keep the semantics and behavior of timeouts manageable
            var syncPoint = new SyncPoint<long>(this.streamManager, transactionId, this.receiveInTransactionContextSyncpoints);
            IInboundStreamMessage message = null;

            var receiveTimedOut = false;
            var receiveSeqNumIncremented = false;

            try
            {
                var enterTime = DateTime.UtcNow;

                // we only let one ReceiveAsync through in any transaction at a time
                await syncPoint.EnterAsync(remainingTimeout);

                var pastEnterTime = DateTime.UtcNow;
                remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, enterTime, pastEnterTime);

                var currentPersistedState = await this.GetCurrentInboundStreamStateInTransaction(storeTransaction, remainingTimeout);

                var pastgetStateTime = DateTime.UtcNow;
                remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, pastEnterTime, pastgetStateTime);

                if (currentPersistedState == PersistentStreamState.Closed)
                {
                    // close message has been consumed by either a committed transaction or an uncommitted receive in the current transaction
                    Diagnostics.Assert(this.closeMessage != null, "{0} Null this.closeMessage value after CloseStream message was consumed", this.Tracer);
                    return this.closeMessage;
                }

                var oldValue =
                    await this.consolidatedStore.UpdateWithOutputAsync(
                        storeTransaction,
                        new StreamStoreConsolidatedKey(receiveSeqNumKey),
                        ReceiveAsyncSeqNumIncrementFunc,
                        remainingTimeout,
                        CancellationToken.None);

                receiveSeqNumIncremented = true;

                var pastUpdateTime = DateTime.UtcNow;
                remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, pastgetStateTime, pastUpdateTime);

                receiveSSN = ((ReceiveStreamSequenceNumber) oldValue.MetadataBody).NextSequenceNumber;
                // If the timeout is going to be effective after this the transaction will have to be aborted in case of timeout

                FabricEvents.Events.Receive(
                    "WaitingForMessage::" + traceId + "@" + this.TraceType,
                    this.StreamIdentity.ToString(),
                    receiveSSN,
                    txn.ToString());

                Diagnostics.Assert(receiveSSN > 0, "Non-positive receiveSSN acquired in ReceiveAsync");
                var completionHandle = new MessageWaiter(this, receiveSSN, storeTransaction, traceId);
                // AddWaiter here before setting timeout so if the timeout fires we know the waiter is there to be dropped: avoid a timeout race
                this.AddWaiter(storeTransaction, completionHandle);
                FabricEvents.Events.Receive(
                    "AddedWaiter::" + traceId + "@" + this.TraceType,
                    this.StreamIdentity.ToString(),
                    completionHandle.ExpectedSequenceNumber,
                    txn.ToString());

                if (remainingTimeout != Timeout.InfiniteTimeSpan)
                {
                    await completionHandle.SetTimeout(remainingTimeout);
                }

                // Register the stream for this transaction so any waiter can be cleared when the transaction closes
                // TODO: this could benefit from transaction enlistment
                var streamSet = new ConcurrentDictionary<Guid, MessageStream>();
                streamSet = this.streamManager.RuntimeResources.ActiveInboundStreamsByTransaction.GetOrAdd(transactionId, streamSet);

                // if this fails we know this isn't the first receive in this transaction, so no-op is OK
                streamSet.TryAdd(this.StreamIdentity, this);

                message = await completionHandle.Task;
            }
            catch (Exception e)
            {
                if (e is FabricNotPrimaryException)
                {
                    FabricEvents.Events.ReceiveFailure(
                        "NotPrimary::" + traceId + "@" + this.TraceType,
                        this.StreamIdentity.ToString(),
                        receiveSSN,
                        txn.ToString());
                    throw;
                }

                if (e is FabricObjectClosedException)
                {
                    FabricEvents.Events.ReceiveFailure(
                        "ObjectClosed::" + traceId + "@" + this.TraceType,
                        this.StreamIdentity.ToString(),
                        receiveSSN,
                        txn.ToString());
                    throw;
                }

                if (e is TimeoutException)
                {
                    FabricEvents.Events.ReceiveFailure(
                        "Timeout::" + traceId + "@" + this.TraceType,
                        this.StreamIdentity.ToString(),
                        receiveSSN,
                        txn.ToString());
                    receiveTimedOut = true;
                }

                else if (e is FabricNotReadableException)
                {
                    if (this.streamManager.NotWritable())
                    {
                        FabricEvents.Events.ReceiveFailure(
                            "NotPrimary::" + traceId + "@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            receiveSSN,
                            txn.ToString());
                        throw new FabricNotPrimaryException();
                    }
                }

                else
                {
                    Tracing.WriteExceptionAsError(
                        "Stream.ReceiveAsync.Failure",
                        e,
                        "{0} TraceId: {1} Target: {2} Transaction: {3} unexpected",
                        this.Tracer,
                        traceId,
                        this.PartnerIdentity,
                        transactionId);

                    Diagnostics.Assert(false, "{0} unexpected exception {1} in ReceiveAsync", this.Tracer, e);
                }
            }
            finally
            {
                syncPoint.Dispose();
            }

            if (receiveTimedOut)
            {
                if (receiveSeqNumIncremented)
                {
                    try
                    {
                        // even though we use Timeout.InfiniteTimeSpan here we already have a lock on this key so this shouldn't block
                        var result =
                            await this.consolidatedStore.UpdateWithOutputAsync(
                                storeTransaction,
                                new StreamStoreConsolidatedKey(receiveSeqNumKey),
                                ReceiveAsyncSeqNumDecrementFunc,
                                Timeout.InfiniteTimeSpan,
                                CancellationToken.None);
                    }
                    catch (Exception e)
                    {
                        if (e is FabricNotPrimaryException)
                        {
                            FabricEvents.Events.ReceiveFailure(
                                "NotPrimary::" + traceId + "@" + this.TraceType,
                                this.StreamIdentity.ToString(),
                                receiveSSN,
                                txn.ToString());
                            throw;
                        }

                        if (e is FabricObjectClosedException)
                        {
                            FabricEvents.Events.ReceiveFailure(
                                "TransactionRace::" + traceId + "@" + this.TraceType,
                                this.StreamIdentity.ToString(),
                                receiveSSN,
                                txn.ToString());
                            throw;
                        }

                        if (e is FabricNotReadableException)
                        {
                            if (this.streamManager.NotWritable())
                            {
                                FabricEvents.Events.ReceiveFailure(
                                    "NotPrimary::" + traceId + "@" + this.TraceType,
                                    this.StreamIdentity.ToString(),
                                    receiveSSN,
                                    txn.ToString());
                                throw new FabricNotPrimaryException();
                            }
                        }

                        else
                        {
                            Tracing.WriteExceptionAsError(
                                "Stream.ReceiveAsync.Failure",
                                e,
                                "{0} Target: {1} Transaction: {2} unexpected",
                                this.Tracer,
                                this.PartnerIdentity,
                                transactionId);

                            Diagnostics.Assert(false, "{0} unexpected exception {1} in ReceiveAsync", this.Tracer, e);
                        }
                    }
                }

                throw new TimeoutException();
            }

            FabricEvents.Events.Receive("Finish::" + traceId + "@" + this.TraceType, this.StreamIdentity.ToString(), receiveSSN, txn.ToString());

            return message;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="txn"></param>
        /// <param name="data"></param>
        /// <param name="timeout"></param>
        /// <returns></returns>
        public async Task SendAsync(Transaction txn, byte[] data, TimeSpan timeout)
        {
            txn.ThrowIfNull("Transaction");
            data.ThrowIfNull("Data");

            if (!TimeoutHandler.ValidRange(timeout))
            {
                throw new ArgumentOutOfRangeException(SR.Error_Stream_Timeout);
            }

            if (this.Direction != StreamKind.Outbound)
            {
                throw new ReliableStreamInvalidOperationException(SR.Error_Stream_Send_Inbound);
            }

            var isOperating = await this.streamManager.WaitForOperatingStateAsync();

            if (!isOperating)
            {
                // if we are not in operating state
                throw new FabricNotPrimaryException();
            }

            var traceId = Interlocked.Increment(ref traceCounter);

            var txnLookupResult = this.consolidatedStore.CreateOrFindTransaction(txn);
            var storeTransaction = txnLookupResult.Value;

            long sendSequenceNumber = -1;

            FabricEvents.Events.Send("Start::" + traceId + "@" + this.TraceType, this.StreamIdentity.ToString(), sendSequenceNumber, txn.ToString());

            var syncPoint = new SyncPoint<Guid>(this.streamManager, this.StreamIdentity, this.streamManager.RuntimeResources.PauseStreamSyncpoints);

            try
            {
                var enterTime = DateTime.UtcNow;

                await syncPoint.EnterAsync(timeout);

                if (this.CurrentState != PersistentStreamState.Open)
                {
                    FabricEvents.Events.Send(
                        "NotOpenException::" + traceId + "@" + this.TraceType,
                        this.StreamIdentity.ToString(),
                        sendSequenceNumber,
                        txn.ToString());
                    throw new ReliableStreamInvalidOperationException(
                        string.Format(CultureInfo.CurrentCulture, SR.Error_Stream_Send_Unopened, this.CurrentState));
                }

                var pastEnterTime = DateTime.UtcNow;

                // Get the Delete Seq. number for the stream (in a no-lock read only Tx)!
                // This serves the purpose of unblocking receive acks and to continue to process them async and
                // reduces the send transaction footprint to just the send number and message processing.
                var remainingTimeout = TimeoutHandler.UpdateTimeout(timeout, enterTime, pastEnterTime);
                var deleteSequenceNumber = await this.GetDeleteSequenceNumberAsync();
                FabricEvents.Events.Send(
                    "GetDelete::" + traceId + "@" + this.TraceType,
                    this.StreamIdentity.ToString(),
                    deleteSequenceNumber,
                    txn.ToString());
                var pastGetDeleteTime = DateTime.UtcNow;
                remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, pastEnterTime, pastGetDeleteTime);

                // To check for Stream, message quota, Get the Send Seq. number for the stream in a RW Tx.
                var sendSeqNumKey = new StreamMetadataKey(this.StreamIdentity, StreamMetadataKind.SendSequenceNumber);
                var sendSeqNumResult =
                    await this.consolidatedStore.GetAsync(
                        storeTransaction,
                        new StreamStoreConsolidatedKey(sendSeqNumKey),
                        remainingTimeout,
                        CancellationToken.None);

                Diagnostics.Assert(sendSeqNumResult.HasValue, "{0} SendSequenceNumber not found for active outbound stream in Stream.SendAsync", this.Tracer);
                var sendSeqNumBody = sendSeqNumResult.Value;
                sendSequenceNumber = ((SendStreamSequenceNumber) sendSeqNumBody.MetadataBody).NextSequenceNumber;
                FabricEvents.Events.Send("GetSend::" + traceId + "@" + this.TraceType, this.StreamIdentity.ToString(), sendSequenceNumber, txn.ToString());
                var pastGetSendTime = DateTime.UtcNow;

                remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, pastGetDeleteTime, pastGetSendTime);

                if (sendSequenceNumber - deleteSequenceNumber >= this.MessageQuota)
                {
                    FabricEvents.Events.SendFailure(
                        "QuotaExceeded::" + traceId + "@" + this.TraceType,
                        this.StreamIdentity.ToString(),
                        sendSequenceNumber,
                        txn.ToString());
                    throw new ReliableStreamQuotaExceededException();
                }

                await this.consolidatedStore.UpdateWithOutputAsync(
                    storeTransaction,
                    new StreamStoreConsolidatedKey(sendSeqNumKey),
                    SendAsyncSeqNumIncrementFunc,
                    remainingTimeout,
                    CancellationToken.None);

                FabricEvents.Events.Send(
                    "UpdateSend::" + traceId + "@" + this.TraceType,
                    this.StreamIdentity.ToString(),
                    sendSequenceNumber,
                    txn.ToString());
            }
            catch (Exception e)
            {
                if (e is ReliableStreamQuotaExceededException)
                {
                    throw;
                }

                if (e is TimeoutException)
                {
                    FabricEvents.Events.SendFailure(
                        "Timeout::" + traceId + "@" + this.TraceType,
                        this.StreamIdentity.ToString(),
                        sendSequenceNumber,
                        txn.ToString());

                    throw;
                }

                if (e is FabricNotPrimaryException)
                {
                    FabricEvents.Events.SendFailure(
                        "NotPrimary::" + traceId + "@" + this.TraceType,
                        this.StreamIdentity.ToString(),
                        sendSequenceNumber,
                        txn.ToString());
                    throw;
                }

                if (e is FabricObjectClosedException)
                {
                    FabricEvents.Events.SendFailure(
                        "ObjectClosed::" + traceId + "@" + this.TraceType,
                        this.StreamIdentity.ToString(),
                        sendSequenceNumber,
                        txn.ToString());
                    throw;
                }

                if (e is ReliableStreamInvalidOperationException)
                {
                    throw;
                }

                if (e is FabricNotReadableException)
                {
                    if (this.streamManager.NotWritable())
                    {
                        FabricEvents.Events.SendFailure(
                            "NotPrimary::" + traceId + "@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            sendSequenceNumber,
                            txn.ToString());
                        throw new FabricNotPrimaryException();
                    }
                }

                Tracing.WriteExceptionAsError(
                    "Stream.SendAsync.Failure",
                    e,
                    "{0} Target: {1} Transaction: {2} unexpected",
                    this.Tracer,
                    this.PartnerIdentity,
                    storeTransaction.Id);

                Diagnostics.Assert(false, "{0} unexpected exception {1} in SendAsync", this.Tracer, e);
            }
            finally
            {
                syncPoint.Dispose();
            }

            Diagnostics.Assert(sendSequenceNumber > 0, "Non-positive sendSSN acquired in SendAsync");

            // TODO: rethink need and safety of no lock in send situation
            storeTransaction.LockingHints = Store.LockingHints.NoLock;

            var messageKey = new StreamStoreMessageKey(this.StreamIdentity, sendSequenceNumber);
            var messageBody = new StreamStoreMessageBody(data);

            // If the send sequence number update goes through we will go ahead with message Add regardless of timeout
            // -- it should actually not block since this is a unique key within the current transaction
            try
            {
                await this.consolidatedStore.AddAsync(
                    storeTransaction,
                    new StreamStoreConsolidatedKey(messageKey),
                    new StreamStoreConsolidatedBody(messageBody),
                    Timeout.InfiniteTimeSpan,
                    CancellationToken.None);
                FabricEvents.Events.Send("Add::" + traceId + "@" + this.TraceType, this.StreamIdentity.ToString(), sendSequenceNumber, txn.ToString());
            }
            catch (Exception e)
            {
                if (e is FabricNotPrimaryException)
                {
                    FabricEvents.Events.SendFailure(
                        "NotPrimary::" + traceId + "@" + this.TraceType,
                        this.StreamIdentity.ToString(),
                        sendSequenceNumber,
                        txn.ToString());
                    throw;
                }

                if (e is FabricObjectClosedException)
                {
                    FabricEvents.Events.SendFailure(
                        "ObjectClosed::" + traceId + "@" + this.TraceType,
                        this.StreamIdentity.ToString(),
                        sendSequenceNumber,
                        txn.ToString());
                    throw;
                }

                if (e is FabricNotReadableException)
                {
                    if (this.streamManager.NotWritable())
                    {
                        FabricEvents.Events.SendFailure(
                            "NotPrimary::" + traceId + "@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            sendSequenceNumber,
                            txn.ToString());
                        throw new FabricNotPrimaryException();
                    }
                }

                Diagnostics.Assert(false, "{0} Unexpected exception {1} in SendAsync", this.Tracer, e);
            }

            //// rwtxMessage.LockingHints = LockingHints.UpdateLock;

            FabricEvents.Events.Send("Finish::" + traceId + "@" + this.TraceType, this.StreamIdentity.ToString(), sendSequenceNumber, txn.ToString());
        }

        #endregion

        #region Messaging helpers

        internal void SendAck(long sequenceNumber)
        {
            Diagnostics.Assert(this.Direction == StreamKind.Inbound, "Attempt to send Ack from outbound stream");
            var ackMessage = new OutboundStreamPayloadAckMessage(this.StreamIdentity, sequenceNumber);
            this.outboundSessionDriver.Send(ackMessage);

            var sessionId = this.outboundSessionDriver.SessionId;
            FabricEvents.Events.WireMessageSent(
                this.TraceType + "@" + sessionId,
                this.StreamIdentity.ToString(),
                sequenceNumber,
                ackMessage.Kind.ToString());
        }

        internal void SendPayloadMessage(StreamStoreMessageKey key, StreamStoreMessageBody body, bool resend = false)
        {
            Diagnostics.Assert(this.Direction == StreamKind.Outbound, "Attempt to send payload message from inbound stream");
            Diagnostics.Assert(this.outboundSessionDriver != null, "OutboundSessionDriver missing in Stream.SendPayloadMessage");
            var message = new OutboundStreamPayloadMessage(key.StreamId, key.StreamSequenceNumber, body);

            this.outboundSessionDriver.Send(message);
            var sessionId = this.outboundSessionDriver.SessionId;

            if (resend)
            {
                FabricEvents.Events.WireMessageSent(
                    "Resend@" + this.TraceType + "@" + sessionId,
                    this.StreamIdentity.ToString(),
                    key.StreamSequenceNumber,
                    message.Kind.ToString());
            }
            else
            {
                FabricEvents.Events.WireMessageSent(
                    this.TraceType + "@" + sessionId,
                    this.StreamIdentity.ToString(),
                    key.StreamSequenceNumber,
                    message.Kind.ToString());
            }
        }

        internal void SendCloseResponse()
        {
            var responseWireMessage = new OutboundCloseStreamResponseWireMessage(this.StreamIdentity, StreamWireProtocolResponse.CloseStreamCompleted);
            this.outboundSessionDriver.Send(responseWireMessage);
            var sessionId = this.outboundSessionDriver.SessionId;
            FabricEvents.Events.WireMessageSent(
                this.TraceType + "@" + sessionId,
                this.StreamIdentity.ToString(),
                this.CloseMessageSequenceNumber,
                responseWireMessage.Kind.ToString());
        }

        #endregion

        #region Lifecycle protocol handlers

        /// <summary>
        /// Ack that the message has been received. check the ackSequencenumber and if greater
        /// than the current delSequencenumber, update the current delSequenceNumber to ackSequenceNumber
        /// and delete all messages  that are lesser than the ackSequenceNumber 
        /// </summary>
        /// <param name="receivedAck">Received Message</param>
        /// <returns></returns>
        internal async Task AckReceived(InboundBaseStreamWireMessage receivedAck)
        {
            var ackedSequenceNumber = receivedAck.MessageSequenceNumber;

            FabricEvents.Events.AckReceived("Start@" + this.TraceType, this.StreamIdentity.ToString(), ackedSequenceNumber);

            var deleteSeqNumKey = new StreamMetadataKey(this.StreamIdentity, StreamMetadataKind.DeleteSequenceNumber);

            try
            {
                using (var replicatorTransaction = await this.streamManager.CreateReplicatorTransactionAsync(Timeout.InfiniteTimeSpan))
                {
                    var storeTransaction = this.consolidatedStore.CreateStoreReadWriteTransaction(replicatorTransaction);
                    storeTransaction.Isolation = Store.StoreTransactionReadIsolationLevel.ReadCommitted;
                    storeTransaction.LockingHints = Store.LockingHints.UpdateLock;

                    // TODO: deal with exceptions
                    var oldValue =
                        await this.consolidatedStore.UpdateWithOutputAsync(
                            storeTransaction,
                            new StreamStoreConsolidatedKey(deleteSeqNumKey),
                            (key, value) => { return AckReceivedUpdateFunc(ackedSequenceNumber, key, value); },
                            Timeout.InfiniteTimeSpan,
                            CancellationToken.None);

                    var oldDeleteSequenceNumber = ((DeleteStreamSequenceNumber) oldValue.MetadataBody).NextSequenceNumber;
                    if (oldDeleteSequenceNumber > ackedSequenceNumber)
                    {
                        FabricEvents.Events.AckReceived("OutOfOrder@" + this.TraceType, this.StreamIdentity.ToString(), ackedSequenceNumber);
                    }

                    // TODO: should we also assert against current send sequence number?
                    // TODO: look at this nolock option again later right now it seems dangerous
                    storeTransaction.LockingHints = Store.LockingHints.NoLock;

                    for (var deletedMessageNumber = oldDeleteSequenceNumber; deletedMessageNumber <= ackedSequenceNumber; deletedMessageNumber++)
                    {
                        var deletedMessageKey = new StreamStoreMessageKey(this.StreamIdentity, deletedMessageNumber);

                        await this.consolidatedStore.TryRemoveAsync(
                            storeTransaction,
                            new StreamStoreConsolidatedKey(deletedMessageKey),
                            Timeout.InfiniteTimeSpan,
                            CancellationToken.None);
                    }

                    await this.streamManager.CompleteReplicatorTransactionAsync(replicatorTransaction, TransactionCompletionMode.Commit);
                }
            }
            catch (Exception e)
            {
                if (e is FabricNotPrimaryException)
                {
                    FabricEvents.Events.NotPrimaryException(
                        "AckReceived@" + this.TraceType,
                        this.StreamIdentity.ToString(),
                        string.Format(CultureInfo.InvariantCulture, "Sequence: {0}", ackedSequenceNumber));
                    throw;
                }

                if (e is FabricObjectClosedException)
                {
                    FabricEvents.Events.AckReceived("ObjectClosed@" + this.TraceType, this.StreamIdentity.ToString(), ackedSequenceNumber);
                    throw;
                }

                if (e is FabricNotReadableException)
                {
                    if (this.streamManager.NotWritable())
                    {
                        FabricEvents.Events.NotPrimaryException(
                            "AckReceived@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            string.Format(CultureInfo.InvariantCulture, "Sequence: {0}", ackedSequenceNumber));
                        throw new FabricNotPrimaryException();
                    }
                }

                Tracing.WriteExceptionAsError(
                    "Stream.AckReceived.UnexpectedException",
                    e,
                    "{0} Target: {1} Acked: {2}",
                    this.Tracer,
                    this.PartnerIdentity,
                    ackedSequenceNumber);

                Diagnostics.Assert(
                    false,
                    "{0} Target: {1} Acked: {2} Unexpected Exception {2}",
                    this.Tracer,
                    this.PartnerIdentity,
                    ackedSequenceNumber,
                    e);
            }

            FabricEvents.Events.AckReceived("Finish@" + this.TraceType, this.StreamIdentity.ToString(), ackedSequenceNumber);
        }

        /// <summary>
        /// This is a completion phase of the outbound OpenAsync() call to the target partner. 
        /// This is called once the target responds to source Open Stream request.
        /// </summary>
        /// <param name="response">Stream Wire Protocol Response</param>
        /// <returns></returns>
        internal async Task CompleteOpenAsync(StreamWireProtocolResponse response)
        {
            var syncPoint = new SyncPoint<Guid>(this.streamManager, this.StreamIdentity, this.streamManager.RuntimeResources.PauseStreamSyncpoints);

            FabricEvents.Events.CompleteOpenStream("Start@" + this.TraceType, this.StreamIdentity.ToString(), response.ToString());

            if (response != StreamWireProtocolResponse.TargetNotReady)
            {
                try
                {
                    await syncPoint.EnterAsync(Timeout.InfiniteTimeSpan);

                    if (this.CurrentState == PersistentStreamState.Opening)
                    {
                        using (var replicatorTransaction = await this.streamManager.CreateReplicatorTransactionAsync(Timeout.InfiniteTimeSpan))
                        {
                            var metadataStoreTransaction = this.consolidatedStore.CreateStoreReadWriteTransaction(replicatorTransaction);

                            metadataStoreTransaction.Isolation = Store.StoreTransactionReadIsolationLevel.ReadCommitted;
                            metadataStoreTransaction.LockingHints = Store.LockingHints.UpdateLock;

                            var metadataKey = new StreamMetadataKey(this.StreamIdentity, StreamMetadataKind.OutboundStableParameters);

                            // TODO: deal with exceptions
                            var updateStateResult =
                                await this.consolidatedStore.UpdateWithOutputAsync(
                                    metadataStoreTransaction,
                                    new StreamStoreConsolidatedKey(metadataKey),
                                    (key, value) => { return CompleteOpenStreamUpdateFunc(response, key, value); },
                                    Timeout.InfiniteTimeSpan,
                                    CancellationToken.None);

                            await this.streamManager.CompleteReplicatorTransactionAsync(replicatorTransaction, TransactionCompletionMode.Commit);
                        }
                    }

                    this.CurrentState = response == StreamWireProtocolResponse.StreamAccepted ? PersistentStreamState.Open : PersistentStreamState.Closed;
                }
                catch (Exception e)
                {
                    if (e is FabricNotPrimaryException)
                    {
                        FabricEvents.Events.NotPrimaryException(
                            "CompleteOpenStreamProtocol@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            response.ToString());
                        throw;
                    }

                    if (e is FabricObjectClosedException)
                    {
                        FabricEvents.Events.CompleteOpenStream("ObjectClosed@" + this.TraceType, this.StreamIdentity.ToString(), response.ToString());
                        throw;
                    }

                    if (e is FabricNotReadableException)
                    {
                        if (this.streamManager.NotWritable())
                        {
                            FabricEvents.Events.NotPrimaryException(
                                "CompleteOpenStreamProtocol@" + this.TraceType,
                                this.StreamIdentity.ToString(),
                                response.ToString());
                            throw new FabricNotPrimaryException();
                        }
                    }

                    Tracing.WriteExceptionAsError("CompleteOpenStream", e, "{0} Store update failed for StreamID: {1}", this.Tracer, this.StreamIdentity);
                    Diagnostics.Assert(false, "{0} Unexpected exception {1} in CompleteOpenStreamProtocol", this.Tracer, e);

                    // TODO: handle exceptions
                }
                finally
                {
                    syncPoint.Dispose();
                }
            }

            TaskCompletionSource<StreamWireProtocolResponse> tcs = null;
            var success = this.streamManager.RuntimeResources.OpenStreamCompletions.TryGetValue(this.StreamIdentity, out tcs);

            // success is not assured -- if we have gone through a change of role Primary->NotPrimary->Primary then the completions would have been
            // cleared and this might be a response to an OpenStream message sent during the prior Primary incarnation, there will be a retry and completion
            // that will occur in the current incarnation so nothing is lost or wrong
            if (success)
            {
                // TODO: SetResult can hijack the thread to execute the continuation, so make sure that happens on a different thread
                // TODO: fault and cancel are actual possibilities we need to account for
                // maintain idempotency by checking the completion status
                if (tcs.Task.IsCompleted)
                {
                    FabricEvents.Events.CompleteOpenStream("FoundCompletedTask@" + this.TraceType, this.StreamIdentity.ToString(), response.ToString());

                    Diagnostics.Assert(
                        !tcs.Task.IsFaulted && !tcs.Task.IsCanceled,
                        "{0} found fauled or canceled OpenAsync completion task for StreamID: {1}",
                        this.Tracer,
                        this.StreamIdentity);
                }
                else
                {
                    var setResultSuccess = tcs.TrySetResult(response);

                    var setResult = setResultSuccess ? ":SetSuccess" : ":SetFailure";
                    FabricEvents.Events.CompleteOpenStream("SettingResult@" + this.TraceType, this.StreamIdentity.ToString(), response + setResult);
                }
            }
            else
            {
                FabricEvents.Events.CompleteOpenStream("CompletionNotFound@" + this.TraceType, this.StreamIdentity.ToString(), response.ToString());
            }
        }

        /// <summary>
        /// This method is called, sometimes concurrently, by both normal OpenAsync outbound and by RestartOutboundAsync; it is re-entrant and idempotent
        /// </summary>
        /// <param name="timeout"></param>
        /// <returns></returns>
        internal async Task<StreamOpenOutcome> FinishOpenAsync(TimeSpan timeout)
        {
            var currentEra = this.streamManager.Era;
            var counter = Interlocked.Increment(ref traceCounter);
            FabricEvents.Events.OpenStream(
                "StartingToFinish@" + counter + "@" + this.TraceType,
                this.StreamIdentity.ToString(),
                this.PartnerIdentity.ToString());

            if (currentEra == Guid.Empty)
            {
                throw new FabricNotPrimaryException();
            }

            TaskCompletionSource<StreamWireProtocolResponse> completion = null;

            lock (this.openCompletionSync)
            {
                completion = new TaskCompletionSource<StreamWireProtocolResponse>();

                // if a completion is already posted use that one
                completion = this.streamManager.RuntimeResources.OpenStreamCompletions.GetOrAdd(this.StreamIdentity, completion);
            }

            var openMessage = new OutboundOpenStreamWireMessage(this.StreamIdentity, this.StreamName);

            var retryCount = 0;
            bool retryNeeded;
            var outcome = StreamOpenOutcome.Unknown;
            var retryInterval = 0;
            var remainingTimeout = timeout;

            do
            {
                // protect against change role to secondary while we wait -- completion will have FabricNotPrimaryException set if we change role
                bool partnerNotReady;
                using (var changeRoleHandler = new CloseWhenNotPrimary<StreamWireProtocolResponse>(completion, this.streamManager))
                {
                    retryNeeded = false;
                    partnerNotReady = false;

                    var beforeStart = DateTime.UtcNow;

                    // set up to complete the TaskCompletionSource with FabricNotPrimaryException if we change role before finishing
                    await changeRoleHandler.Start(remainingTimeout);

                    var afterStart = DateTime.UtcNow;

                    remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, beforeStart, afterStart);

                    this.outboundSessionDriver.Send(openMessage);
                    FabricEvents.Events.OpenRequestSent(this.TraceType + "@" + (++retryCount), this.StreamIdentity.ToString());

                    /*  
                     * Now wait, but we cannot just wait for the remaining timeout.  If a stream open goes to a target service that that has no other streams 
                     * to/from the source and the target crashes/closes before responding the Open will not complete since the target will not reset streams 
                     * with the source on recovery.  We therefore use a smaller timeout to retry.
                     */

                    var retryTimeout = new TimeSpan(StreamConstants.MinRetryIntervalForOpenStream*TimeSpan.TicksPerMillisecond);

                    if (remainingTimeout != Timeout.InfiniteTimeSpan)
                    {
                        retryTimeout = retryTimeout < remainingTimeout ? retryTimeout : remainingTimeout;
                    }

                    var beforeWaitWithDelay = DateTime.UtcNow;

                    var finished = await TimeoutHandler.WaitWithDelay(retryTimeout, completion.Task);

                    var afterWaitWithDelay = DateTime.UtcNow;

                    // the completion task is never cancelled or faulted
                    if (finished && completion.Task.Status == TaskStatus.RanToCompletion)
                    {
                        var response = completion.Task.Result;

                        switch (response)
                        {
                            case StreamWireProtocolResponse.StreamAccepted:
                                outcome = StreamOpenOutcome.PartnerAccepted;
                                break;
                            case StreamWireProtocolResponse.StreamRejected:
                                outcome = StreamOpenOutcome.PartnerRejected;
                                break;
                            case StreamWireProtocolResponse.StreamPartnerFaulted:
                                outcome = StreamOpenOutcome.PartnerFaulted;
                                break;
                            case StreamWireProtocolResponse.TargetNotAcceptingStreams:
                                outcome = StreamOpenOutcome.PartnerNotListening;
                                break;
                            case StreamWireProtocolResponse.TargetNotReady:
                                outcome = StreamOpenOutcome.PartnerNotReady;
                                break;
                            default:
                                Diagnostics.Assert(false, "{0} Unexpected response {1} to OpenStream message", this.Tracer, response);
                                break;
                        }

                        if (outcome == StreamOpenOutcome.PartnerNotReady)
                        {
                            FabricEvents.Events.OpenStream(
                                "PartnerNotReady@" + counter + "@" + this.TraceType,
                                this.StreamIdentity.ToString(),
                                this.PartnerIdentity.ToString());

                            remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, beforeWaitWithDelay, afterWaitWithDelay);

                            // if there is no time left, the UpdateTimeout above would already throw a TimeoutException, if it did not we will retry
                            retryNeeded = true;
                            partnerNotReady = true;
                        }
                    }
                    else if (finished && completion.Task.Status == TaskStatus.Canceled)
                    {
                        // TODO: look at whether this would ever happen
                        FabricEvents.Events.OpenStreamFailure("Canceled@" + this.TraceType, this.StreamIdentity.ToString(), this.PartnerIdentity.ToString());
                        throw new OperationCanceledException();
                    }
                    else if (finished && completion.Task.Status == TaskStatus.Faulted)
                    {
                        var realException = completion.Task.Exception.InnerException;
                        Diagnostics.Assert(
                            realException is FabricNotPrimaryException || realException is FabricObjectClosedException,
                            "{0} FinishOpenAsync Completion Faulted with Unexpected exception {1}",
                            this.Tracer,
                            realException);

                        FabricEvents.Events.OpenStreamFailure(
                            "NotPrimary@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            this.PartnerIdentity.ToString());
                        throw new FabricNotPrimaryException();
                    }
                    else
                    {
                        Diagnostics.Assert(
                            !finished,
                            "{0} FinishOpenAsync Completion finished with Unexpected TaskStatus {1}",
                            this.Tracer,
                            completion.Task.Status);

                        remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, beforeWaitWithDelay, afterWaitWithDelay);

                        FabricEvents.Events.OpenStream(
                            "RetriableTimeout@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            this.PartnerIdentity.ToString());
                        // if there is no time left, the UpdateTimeout above would already throw a TimeoutException, if it did not we will retry
                        retryNeeded = true;
                    }
                }

                if (retryNeeded)
                {
                    // check that we are still primary
                    this.streamManager.ThrowIfNotWritable();

                    if (partnerNotReady)
                    {
                        // backoff needed since the response would have come immediately
                        var beforeBackOff = DateTime.UtcNow;
                        retryInterval = retryInterval + StreamConstants.BaseDelayForPartnerNotReadyRetry;
                        retryInterval = retryInterval >= StreamConstants.MaxDelayForPartnerNotReadyRetry
                            ? StreamConstants.MaxDelayForPartnerNotReadyRetry
                            : retryInterval;
                        await Task.Delay(retryInterval);
                        var afterBackoff = DateTime.UtcNow;

                        try
                        {
                            remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, beforeBackOff, afterBackoff);
                        }
                        catch (TimeoutException)
                        {
                            // we ran out of retry time
                            return StreamOpenOutcome.PartnerNotReady;
                        }
                    }
                    else
                    {
                        var sessionConnectionManager = this.streamManager.SessionConnectionManager;

                        if (sessionConnectionManager == null)
                        {
                            // this could happen if the replica changed role or closed while we are in this loop
                            throw new FabricNotPrimaryException();
                        }

                        PartitionKey normalizedKey = null;
                        var normalizedKeyFound = sessionConnectionManager.GetNormalizedPartitionKey(this.PartnerIdentity, out normalizedKey);
                        Diagnostics.Assert(
                            normalizedKeyFound,
                            "{0} Found no existing normalized key in SessionConnectionManager.HasEnpointChanged, for Key: {1}",
                            this.streamManager.TraceType,
                            normalizedKey);

                        var beforeHasEndpointChanged = DateTime.UtcNow;

                        var partnerMoved = false;
                        try
                        {
                            // if we keep timing out here this will not work -- OpenAsync should not have a very short timeout -- should we enforce minimum 10 seconds?
                            partnerMoved = await sessionConnectionManager.HasPartitionEraChanged(normalizedKey, remainingTimeout);
                        }
                        catch (TimeoutException)
                        {
                            FabricEvents.Events.OpenStream("Timeout@" + this.TraceType, this.StreamIdentity.ToString(), this.PartnerIdentity.ToString());
                            throw;
                        }

                        var afterHasEndpointChanged = DateTime.UtcNow;

                        if (partnerMoved)
                        {
                            // this is an internal reset -- false implies no response to partner
                            var resetTask = this.streamManager.ResetPartnerStreamsAsync(Guid.Empty, normalizedKey, false)
                                .ContinueWith(this.ResetPartnerStreamsAsyncContinuation);
                        }

                        remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, beforeHasEndpointChanged, afterHasEndpointChanged);
                    }

                    lock (this.openCompletionSync)
                    {
                        FabricEvents.Events.OpenStream(
                            "Retrying@" + counter + "@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            this.PartnerIdentity.ToString());
                        TaskCompletionSource<StreamWireProtocolResponse> oldCompletion = null;
                        // get the completion to replace
                        var success = this.streamManager.RuntimeResources.OpenStreamCompletions.TryGetValue(this.StreamIdentity, out oldCompletion);

                        if (success)
                        {
                            // There is a possible race between FinishOpen called from OpenAsync that is left dangling due to partner failure, and another FinishOpen
                            // occurring during recovery -- both may be released by a single response from the recovered partner and go through this code twice
                            // If that happens the oldCompletion.Task will have been replaced by the first thread and the second thread will find it in WaitingForActivation status
                            if (oldCompletion.Task.IsCompleted)
                            {
                                completion = new TaskCompletionSource<StreamWireProtocolResponse>();
                                success = this.streamManager.RuntimeResources.OpenStreamCompletions.TryUpdate(this.StreamIdentity, completion, oldCompletion);
                            }
                        }

                        if (!success)
                        {
                            Diagnostics.Assert(
                                currentEra != this.streamManager.Era,
                                "{0} OpenStreamCompletion update failed at retry point in FinishOpenAsync without era change, Current Era: {1}",
                                this.Tracer,
                                currentEra);
                            throw new OperationCanceledException();
                        }
                    }
                }
            } while (retryNeeded);

            return outcome;
        }

        /// <summary>
        /// Handler in lieu of await for StreamManager.ResetPartnerStreamsAsync
        /// </summary>
        /// <param name="task"></param>
        private void ResetPartnerStreamsAsyncContinuation(Task task)
        {
            // TODO: exception handling
            if (task.Exception != null)
            {
                if (task.Exception.InnerException is FabricObjectClosedException || task.Exception.InnerException is FabricNotPrimaryException)
                {
                    Tracing.WriteExceptionAsWarning("FinishOpenAsync.ResetPartnerStreamsAsync.Failure", task.Exception, "{0}", this.Tracer);
                }
                else
                {
                    Tracing.WriteExceptionAsError("FinishOpenAsync.ResetPartnerStreamsAsync.Failure", task.Exception, "{0}", this.Tracer);
                }
            }
        }

        /// <summary>
        /// This is the completion phase of the outbound stream DeleteAsync() call to the target partner. 
        /// This is called once the target responds to source delete Stream request.
        /// </summary>
        /// <param name="response"></param>
        /// <returns></returns>
        internal async Task CompleteDeleteAsync(StreamWireProtocolResponse response)
        {
            FabricEvents.Events.CompleteDeleteStream("Start@" + this.TraceType, this.StreamIdentity.ToString(), response.ToString());

            try
            {
                if ((this.CurrentState == PersistentStreamState.Deleting) &&
                    ((response == StreamWireProtocolResponse.TargetNotified) || (response == StreamWireProtocolResponse.StreamNotFound)))
                {
                    using (var replicatorTransaction = await this.streamManager.CreateReplicatorTransactionAsync(Timeout.InfiniteTimeSpan))
                    {
                        // remove outbound stream from store.
                        await this.DeleteOutboundStreamFromConsolidatedStore(
                            replicatorTransaction,
                            this.StreamIdentity,
                            this.StreamName,
                            this.PartnerIdentity,
                            Timeout.InfiniteTimeSpan);

                        await this.streamManager.CompleteReplicatorTransactionAsync(replicatorTransaction, TransactionCompletionMode.Commit);
                    }

                    this.CurrentState = PersistentStreamState.Deleted;
                }
            }
            catch (Exception e)
            {
                if (e is FabricNotPrimaryException)
                {
                    FabricEvents.Events.NotPrimaryException(
                        "CompleteDeleteStreamProtocol@" + this.TraceType,
                        this.StreamIdentity.ToString(),
                        response.ToString());
                    throw;
                }

                if (e is FabricObjectClosedException)
                {
                    FabricEvents.Events.CompleteDeleteStream("ObjectClosed@" + this.TraceType, this.StreamIdentity.ToString(), response.ToString());
                    throw;
                }

                if (e is FabricNotReadableException)
                {
                    if (this.streamManager.NotWritable())
                    {
                        FabricEvents.Events.NotPrimaryException(
                            "CompleteDeleteStreamProtocol@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            response.ToString());
                        throw new FabricNotPrimaryException();
                    }
                }

                Tracing.WriteExceptionAsError("CompleteDeleteStreamProtocol", e, "{0} Store update failed", this.Tracer, this.StreamIdentity);
                Diagnostics.Assert(false, "{0} Unexpected exception {1} in CompleteDeleteStreamProtocol", this.Tracer, e);
            }

            // we do not remove this TaskCompletionSource object from DeleteStreamCompletions to support idempotent DeleteAsyncContinuation calls which may need to find it
            TaskCompletionSource<StreamWireProtocolResponse> tcs = null;
            var success = this.streamManager.RuntimeResources.DeleteStreamCompletions.TryGetValue(this.StreamIdentity, out tcs);

            // success is not assured -- if we have gone through a change of role Primary->NotPrimary->Primary then the completions would have been
            // cleared and this might be a response to an DeleteStream message sent during the prior Primary incarnation, there will be a retry and completion
            // that will occur in the current incarnation so nothing is lost or wrong
            if (success)
            {
                // maintain idempotency by checking the completion status
                if (tcs.Task.IsCompleted)
                {
                    FabricEvents.Events.CompleteDeleteStream("FoundCompletedTask@" + this.TraceType, this.StreamIdentity.ToString(), response.ToString());

                    Diagnostics.Assert(
                        !tcs.Task.IsFaulted && !tcs.Task.IsCanceled,
                        "{0} found faulted or canceled DeleteAsync completion task for StreamID: {1}",
                        this.Tracer,
                        this.StreamIdentity);
                }
                else
                {
                    var setResultSuccess = tcs.TrySetResult(response);

                    var setResult = setResultSuccess ? ":SetSuccess" : ":SetFailure";
                    FabricEvents.Events.CompleteDeleteStream("SettingResult@" + this.TraceType, this.StreamIdentity.ToString(), response + setResult);
                }
            }
            else
            {
                FabricEvents.Events.CompleteDeleteStream("CompletionNotFound@" + this.TraceType, this.StreamIdentity.ToString(), response.ToString());
            }
        }

        /// <summary>
        /// This is the completion phase of the outbound stream CloseAsync() call to the target partner. 
        /// This is called once the target responds to source close Stream request.
        /// </summary>
        /// <param name="response"></param>
        /// <returns></returns>
        internal async Task CompleteCloseAsync(StreamWireProtocolResponse response)
        {
            var syncPoint = new SyncPoint<Guid>(this.streamManager, this.StreamIdentity, this.streamManager.RuntimeResources.PauseStreamSyncpoints);

            FabricEvents.Events.CompleteCloseStream("Start@" + this.TraceType, this.StreamIdentity.ToString(), response.ToString());

            try
            {
                await syncPoint.EnterAsync(Timeout.InfiniteTimeSpan);

                if (this.CurrentState == PersistentStreamState.Closing)
                {
                    using (var replicatorTransaction = await this.streamManager.CreateReplicatorTransactionAsync(Timeout.InfiniteTimeSpan))
                    {
                        var metadataStoreTransaction = this.consolidatedStore.CreateStoreReadWriteTransaction(replicatorTransaction);
                        metadataStoreTransaction.Isolation = Store.StoreTransactionReadIsolationLevel.ReadCommitted;
                        metadataStoreTransaction.LockingHints = Store.LockingHints.UpdateLock;

                        var metadataKey = new StreamMetadataKey(this.StreamIdentity, StreamMetadataKind.OutboundStableParameters);

                        // TODO: deal with exceptions
                        var updateStateResult =
                            await this.consolidatedStore.UpdateWithOutputAsync(
                                metadataStoreTransaction,
                                new StreamStoreConsolidatedKey(metadataKey),
                                CompleteCloseStreamUpdateFunc,
                                Timeout.InfiniteTimeSpan,
                                CancellationToken.None);

                        await this.streamManager.CompleteReplicatorTransactionAsync(replicatorTransaction, TransactionCompletionMode.Commit);
                    }

                    this.CurrentState = PersistentStreamState.Closed;
                }
            }
            catch (Exception e)
            {
                if (e is FabricNotPrimaryException)
                {
                    FabricEvents.Events.NotPrimaryException(
                        "CompleteCloseStreamProtocol@" + this.TraceType,
                        this.StreamIdentity.ToString(),
                        response.ToString());
                    throw;
                }

                if (e is FabricObjectClosedException)
                {
                    FabricEvents.Events.CompleteCloseStream("ObjectClosed@" + this.TraceType, this.StreamIdentity.ToString(), response.ToString());
                    throw;
                }

                if (e is FabricNotReadableException)
                {
                    if (this.streamManager.NotWritable())
                    {
                        FabricEvents.Events.NotPrimaryException(
                            "CompleteCloseStreamProtocol@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            response.ToString());
                        throw new FabricNotPrimaryException();
                    }
                }

                Tracing.WriteExceptionAsError("CompleteCloseStreamProtocol", e, "{0} Store update failed", this.Tracer, this.StreamIdentity);
                Diagnostics.Assert(false, "{0} Unexpected exception {1} in CompleteCloseStreamProtocol", this.Tracer, e);
            }
            finally
            {
                syncPoint.Dispose();
            }

            // we do not remove this TaskCompletionSource object from CloseStreamCompletions to support idempotent CloseAsyncContinuation calls which may need to find it
            TaskCompletionSource<StreamWireProtocolResponse> tcs = null;
            var success = this.streamManager.RuntimeResources.CloseStreamCompletions.TryGetValue(this.StreamIdentity, out tcs);

            // success is not assured -- if we have gone through a change of role Primary->NotPrimary->Primary then the completions would have been
            // cleared and this might be a response to an OpenStream message sent during the prior Primary incarnation, there will be a retry and completion
            // that will occur in the current incarnation so nothing is lost or wrong
            if (success)
            {
                // TODO: SetResult can hijack the thread to execute the continuation, so make sure that happens on a different thread
                // TODO: fault and cancel are actual possibilities we need to account for
                // maintain idempotency by checking the completion status
                if (tcs.Task.IsCompleted)
                {
                    FabricEvents.Events.CompleteCloseStream("FoundCompletedTask@" + this.TraceType, this.StreamIdentity.ToString(), response.ToString());

                    Diagnostics.Assert(
                        !tcs.Task.IsFaulted && !tcs.Task.IsCanceled,
                        "{0} found fauled or canceled CloseAsync completion task for StreamID: {1}",
                        this.Tracer,
                        this.StreamIdentity);
                }
                else
                {
                    var setResultSuccess = tcs.TrySetResult(response);

                    var setResult = setResultSuccess ? ":SetSuccess" : ":SetFailure";
                    FabricEvents.Events.CompleteCloseStream("SettingResult@" + this.TraceType, this.StreamIdentity.ToString(), response + setResult);
                }
            }
            else
            {
                FabricEvents.Events.CompleteCloseStream("CompletionNotFound@" + this.TraceType, this.StreamIdentity.ToString(), response.ToString());
            }
        }

        // this method is called by normal CloseAsync outbound 
        // TODO: make sure locking issues are avoided
        internal async Task<bool> FinishCloseAsync(long closeMessageSequenceNumber, TimeSpan timeout)
        {
            var completion = new TaskCompletionSource<StreamWireProtocolResponse>();

            // if a completion is already posted use that one
            completion = this.streamManager.RuntimeResources.CloseStreamCompletions.GetOrAdd(this.StreamIdentity, completion);

            Diagnostics.Assert(closeMessageSequenceNumber > 0, "Negative closeMessageSequenceNumber set during CloseAsync");
            var closeMessage = new OutboundCloseStreamWireMessage(this.StreamIdentity, closeMessageSequenceNumber);
            this.outboundSessionDriver.Send(closeMessage);
            FabricEvents.Events.CloseRequestSent(this.TraceType, this.StreamIdentity.ToString(), closeMessageSequenceNumber);

            using (var changeRoleHandler = new CloseWhenNotPrimary<StreamWireProtocolResponse>(completion, this.streamManager))
            {
                var remainingTimeout = timeout;

                // check if we lost primary status
                this.streamManager.ThrowIfNotWritable();

                var beforeStart = DateTime.UtcNow;

                // set up to complete the TaskCompletionSource with FabricNotPrimaryException if we change role before finishing
                await changeRoleHandler.Start(remainingTimeout);

                var afterStart = DateTime.UtcNow;

                remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, beforeStart, afterStart);

                var finished = await TimeoutHandler.WaitWithDelay(remainingTimeout, completion.Task);

                // the completion task is never cancelled or faulted
                if (finished && completion.Task.Status == TaskStatus.RanToCompletion)
                {
                    var response = completion.Task.Result;

                    // TODO: cleanup 
                    Diagnostics.Assert(
                        response == StreamWireProtocolResponse.CloseStreamCompleted,
                        "{0} encountered failure in close completion with Response: {1}",
                        this.Tracer,
                        response);

                    return true;
                }

                if (finished && completion.Task.Status == TaskStatus.Canceled)
                {
                    // TODO: look at whether this would ever happen
                    FabricEvents.Events.CloseStreamFailure("Canceled@" + this.TraceType, this.StreamIdentity.ToString());
                    throw new OperationCanceledException();
                }

                if (finished && completion.Task.Status == TaskStatus.Faulted)
                {
                    Diagnostics.Assert(
                        completion.Task.Exception.InnerException is FabricNotPrimaryException,
                        "{0} FinishCloseAsync Completion Faulted with Unexpected exception {1}",
                        this.Tracer,
                        completion.Task.Exception.InnerException);

                    FabricEvents.Events.CloseStreamFailure("NotPrimary@" + this.TraceType, this.StreamIdentity.ToString());
                    throw new FabricNotPrimaryException();
                }

                FabricEvents.Events.CloseStreamFailure("Timeout@" + this.TraceType, this.StreamIdentity.ToString());
                throw new TimeoutException();
            }
        }

        // handler in lieu of await for StreamManager.InboundStreamClosed
        internal void CloseInboundStreamContinuation(Task task)
        {
            // TODO: exception handling
            if (task.Exception != null)
            {
                Diagnostics.ProcessException("StreamManager.InboundStreamClosed.Failure", task.Exception, "{0} Source: {1}", this.Tracer, this.PartnerIdentity);
            }
        }

        internal async Task CloseInboundStream(Store.IStoreReadWriteTransaction transactionContext)
        {
            FabricEvents.Events.CloseInboundStream(
                "Start@" + this.TraceType,
                this.StreamIdentity.ToString(),
                this.CloseMessageSequenceNumber,
                transactionContext.ReplicatorTransactionBase.ToString());

            try
            {
                var metadataKey = new StreamMetadataKey(this.StreamIdentity, StreamMetadataKind.InboundStableParameters);

                // TODO: deal with exceptions
                var updateStateResult =
                    await this.consolidatedStore.UpdateWithOutputAsync(
                        transactionContext,
                        new StreamStoreConsolidatedKey(metadataKey),
                        InboundStreamClosedUpdateFunc,
                        Timeout.InfiniteTimeSpan,
                        CancellationToken.None);
            }
            catch (Exception e)
            {
                if (e is FabricNotPrimaryException)
                {
                    FabricEvents.Events.NotPrimaryException(
                        "CloseInboundStream@" + this.TraceType,
                        this.StreamIdentity.ToString(),
                        transactionContext.ReplicatorTransactionBase.ToString());
                    throw;
                }

                if (e is FabricNotReadableException)
                {
                    if (this.streamManager.NotWritable())
                    {
                        FabricEvents.Events.NotPrimaryException(
                            "CloseInboundStream@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            transactionContext.ReplicatorTransactionBase.ToString());
                        throw new FabricNotPrimaryException();
                    }
                }

                if (e is FabricObjectClosedException)
                {
                    Tracing.WriteExceptionAsWarning("Stream.Close.Failure", e, "{0} Source: {1} Store update failed", this.Tracer, this.PartnerIdentity);
                    throw;
                }

                Tracing.WriteExceptionAsError("Stream.CloseInboundStream.Failure", e, "{0} Source: {1} Store update failed", this.Tracer, this.PartnerIdentity);
                Diagnostics.Assert(false, "{0} Unexpected exception {1}", this.Tracer, e);
            }

            FabricEvents.Events.CloseInboundStream(
                "Finish@" + this.TraceType,
                this.StreamIdentity.ToString(),
                this.CloseMessageSequenceNumber,
                transactionContext.ReplicatorTransactionBase.ToString());
        }

        internal void InboundStreamClosed()
        {
            FabricEvents.Events.InboundStreamClosed("Start@" + this.TraceType, this.StreamIdentity.ToString(), this.CloseMessageSequenceNumber);

            this.CurrentState = PersistentStreamState.Closed;

            // respond to all future receives with the same generic message
            // do all this atomically
            // send Completed message to sender
            lock (this.waiterSync)
            {
                foreach (var pair in this.receiveWaiters)
                {
                    FabricEvents.Events.InboundStreamClosed(
                        "DischargingWaiter@" + this.TraceType,
                        this.StreamIdentity.ToString(),
                        pair.Value.ExpectedSequenceNumber);

                    // Guard against thread hijacking by SetResult
                    Task.Run(async () => await pair.Value.SetResult(this.closeMessage));
                }

                this.receiveWaiters.Clear();
            }

            this.SendCloseResponse();

            FabricEvents.Events.InboundStreamClosed("SentResponse@" + this.TraceType, this.StreamIdentity.ToString(), this.CloseMessageSequenceNumber);
        }

        internal async Task SetCloseMessageSequenceNumber(long messageSequenceNumber, Store.IStoreReadWriteTransaction transactionContext)
        {
            FabricEvents.Events.SetCloseMessageSequenceNumber(
                "Start@" + this.TraceType,
                this.StreamIdentity.ToString(),
                messageSequenceNumber,
                transactionContext.ReplicatorTransactionBase.ToString());

            Diagnostics.Assert(
                this.Direction == StreamKind.Inbound,
                "{0} Partner: {1} SetCloseMessageSequenceNumber called on outbound stream with sequence number {2}",
                this.Tracer,
                this.PartnerIdentity,
                messageSequenceNumber);

            if (this.CloseMessageSequenceNumber > 0)
            {
                Diagnostics.Assert(
                    this.CloseMessageSequenceNumber == messageSequenceNumber,
                    "{0} Source: {1} found two close message sequence numbers {2} and {3}",
                    this.Tracer,
                    this.PartnerIdentity,
                    this.CloseMessageSequenceNumber,
                    messageSequenceNumber);
            }

            this.CloseMessageSequenceNumber = messageSequenceNumber;

            try
            {
                var metadataKey = new StreamMetadataKey(this.StreamIdentity, StreamMetadataKind.InboundStableParameters);

                // TODO: deal with exceptions
                var updateStateResult =
                    await this.consolidatedStore.UpdateWithOutputAsync(
                        transactionContext,
                        new StreamStoreConsolidatedKey(metadataKey),
                        (key, value) => { return SetCloseMessageSequenceNumberUpdateFunc(messageSequenceNumber, key, value); },
                        Timeout.InfiniteTimeSpan,
                        CancellationToken.None);
            }
            catch (Exception e)
            {
                if (e is FabricNotPrimaryException)
                {
                    FabricEvents.Events.NotPrimaryException(
                        "SetCloseMessageSequenceNumber@" + this.TraceType,
                        this.StreamIdentity.ToString(),
                        transactionContext.ReplicatorTransactionBase.ToString());
                    throw;
                }

                if (e is FabricNotReadableException)
                {
                    if (this.streamManager.NotWritable())
                    {
                        FabricEvents.Events.NotPrimaryException(
                            "SetCloseMessageSequenceNumber@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            transactionContext.ReplicatorTransactionBase.ToString());
                        throw new FabricNotPrimaryException();
                    }
                }

                if (e is FabricObjectClosedException)
                {
                    Tracing.WriteExceptionAsWarning(
                        "Stream.SetCloseMessageSequenceNumber.UnexpectedFailure",
                        e,
                        "{0} Source: {1} Store update failed",
                        this.Tracer,
                        this.PartnerIdentity);

                    throw;
                }

                Tracing.WriteExceptionAsError(
                    "Stream.SetCloseMessageSequenceNumber.UnexpectedFailure",
                    e,
                    "{0} Source: {1} Store update failed",
                    this.Tracer,
                    this.PartnerIdentity);
                Diagnostics.Assert(false, "{0} Unexpected exception {1}", this.Tracer, e);
            }

            FabricEvents.Events.SetCloseMessageSequenceNumber(
                "Finish@" + this.TraceType,
                this.StreamIdentity.ToString(),
                messageSequenceNumber,
                transactionContext.ReplicatorTransactionBase.ToString());
        }

        /// <summary>
        /// Update inbound stream state to 'deleted' in consolidated store.
        /// </summary>
        /// <returns></returns>
        internal async Task DeleteInboundStream()
        {
            FabricEvents.Events.DeleteInboundStream("Start@" + this.TraceType, this.StreamIdentity.ToString());

            //  update consolidated store
            using (var replicatorTransaction = await this.streamManager.CreateReplicatorTransactionAsync(Timeout.InfiniteTimeSpan))
            {
                try
                {
                    var txnLookupResult = this.consolidatedStore.CreateOrFindTransaction(replicatorTransaction);
                    Diagnostics.Assert(
                        !txnLookupResult.HasValue,
                        "{0} found existing store transaction for new replicator transaction; TransactionId: {1}",
                        this.Tracer,
                        replicatorTransaction);

                    var metadataStoreTransaction = txnLookupResult.Value;
                    metadataStoreTransaction.Isolation = Store.StoreTransactionReadIsolationLevel.ReadCommitted;

                    var metadataKey = new StreamMetadataKey(this.StreamIdentity, StreamMetadataKind.InboundStableParameters);

                    var updateStateResult =
                        await this.consolidatedStore.UpdateWithOutputAsync(
                            metadataStoreTransaction,
                            new StreamStoreConsolidatedKey(metadataKey),
                            InboundStreamDeletedUpdateFunc,
                            Timeout.InfiniteTimeSpan,
                            CancellationToken.None);

                    await this.streamManager.CompleteReplicatorTransactionAsync(replicatorTransaction, TransactionCompletionMode.Commit);
                }
                catch (Exception e)
                {
                    if (e is FabricNotPrimaryException)
                    {
                        FabricEvents.Events.NotPrimaryException(
                            "DeleteInboundStream@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            replicatorTransaction.ToString());
                        throw;
                    }

                    if (e is FabricNotReadableException)
                    {
                        if (this.streamManager.NotWritable())
                        {
                            FabricEvents.Events.NotPrimaryException(
                                "DeleteInboundStream@" + this.TraceType,
                                this.StreamIdentity.ToString(),
                                replicatorTransaction.ToString());
                            throw new FabricNotPrimaryException();
                        }
                    }

                    if (e is FabricObjectClosedException)
                    {
                        Tracing.WriteExceptionAsWarning(
                            "Stream.DeleteInboundStream.Failure",
                            e,
                            "{0} Source: {1} Store update failed",
                            this.Tracer,
                            this.PartnerIdentity);
                        throw;
                    }

                    Tracing.WriteExceptionAsError(
                        "Stream.DeleteInboundStream.Failure",
                        e,
                        "{0} Source: {1} Store update failed",
                        this.Tracer,
                        this.PartnerIdentity);
                    Diagnostics.Assert(false, "{0} Unexpected exception {1}", this.Tracer, e);
                }
            }
            FabricEvents.Events.DeleteInboundStream("Finish@" + this.TraceType, this.StreamIdentity.ToString());
        }

        #endregion

        #region Recovery protocol handlers

        /// <summary>
        /// Pause on an outbound stream at restart is used by the ResetPartnerStreams process to ensure orderly resend of un-acknowledged messages
        /// The Pause prevents the service from completing new SendAsync operations until the resend is completed
        /// </summary>
        /// <returns></returns>
        internal async Task PauseAsync()
        {
            Diagnostics.Assert(this.Direction == StreamKind.Outbound, "{0} PauseAsync called on inbound stream", this.Tracer);

            FabricEvents.Events.Pause("Start@" + this.TraceType, this.StreamIdentity.ToString(), this.CurrentState.ToString());

            if (this.CurrentState == PersistentStreamState.Initialized)
            {
                // nothing to do, the stream was created but nothing further was committed with it so far by the service
                FabricEvents.Events.Pause("No-Op@" + this.TraceType, this.StreamIdentity.ToString(), this.CurrentState.ToString());
                return;
            }

            if (this.CurrentState == PersistentStreamState.Closed)
            {
                // nothing to do, the stream lifecycle is finished
                FabricEvents.Events.Pause("No-op@" + this.TraceType, this.StreamIdentity.ToString(), this.CurrentState.ToString());
                return;
            }

            var syncPoint = new SyncPoint<Guid>(this.streamManager, this.StreamIdentity, this.streamManager.RuntimeResources.PauseStreamSyncpoints);

            await syncPoint.EnterAsync(Timeout.InfiniteTimeSpan);

            Diagnostics.Assert(
                this.pauseSyncPoint == null,
                "{0} found non-null pauseSyncPoint after entering PauseAsync",
                this.Tracer);

            this.pauseSyncPoint = syncPoint;

            FabricEvents.Events.Pause("Finish@" + this.TraceType, this.StreamIdentity.ToString(), this.CurrentState.ToString());
        }

        /// <summary>
        /// We do not want to wait on this on Restart because the completion of close takes 
        /// indefinite time depending on how soon the remaining messages in the stream are received and drained at the receiver
        /// we need a separate mechanism for the restarted service to wait on CloseAsync completion if need be
        /// This indirection is needed to avoid a compiler warning
        /// </summary>
        internal void FinishOpenOnRestart()
        {
            FabricEvents.Events.OpenOnRestart("Start@" + this.TraceType, this.StreamIdentity.ToString());
            this.FinishOpenAsync(Timeout.InfiniteTimeSpan).ContinueWith(this.FinishOpenOnRestartContinuation);
        }

        internal void FinishOpenOnRestartContinuation(Task<StreamOpenOutcome> openContinuationTask)
        {
            if (openContinuationTask.Exception != null)
            {
                var e = Diagnostics.ProcessExceptionAsError("OpenOnRestart.Exception", openContinuationTask.Exception, "{0} context", this.Tracer);
                Diagnostics.Assert(
                    e is FabricNotPrimaryException || e is FabricObjectClosedException,
                    "{0} Target: {1} Stream.FinishOpenAsync threw unexpected Exception: {2}",
                    this.Tracer,
                    this.PartnerIdentity,
                    e);
            }
            else
            {
                FabricEvents.Events.OpenOnRestart(openContinuationTask.Result + "@" + this.TraceType, this.StreamIdentity.ToString());
            }
        }

        /// <summary>
        /// We do not want to wait on this on Restart because the completion of close takes 
        /// indefinite time depending on how soon the remaining messages in the stream are received and drained at the receiver
        /// we need a separate mechanism for the restarted service to wait on CloseAsync completion if need be
        /// 
        /// This indirection is needed to avoid a compiler warning
        /// </summary>
        /// <param name="closeMessageSequenceNumber"></param>
        internal void FinishCloseOnRestart(long closeMessageSequenceNumber)
        {
            FabricEvents.Events.CloseOnRestart("Start@" + this.TraceType, this.StreamIdentity.ToString());

            try
            {
                this.FinishCloseAsync(closeMessageSequenceNumber, Timeout.InfiniteTimeSpan).ContinueWith(this.FinishCloseOnRestartContinuation);
            }
            catch (Exception e)
            {
                Diagnostics.Assert(
                    e is FabricNotPrimaryException,
                    "{0} Target: {1} Stream.FinishCloseAsync threw unexpected Exception during FinishCloseOnRestart {2}",
                    this.Tracer,
                    this.PartnerIdentity,
                    e);
            }
        }

        internal void FinishCloseOnRestartContinuation(Task<bool> closeContinuationTask)
        {
            if (closeContinuationTask.Exception != null)
            {
                Diagnostics.ProcessExceptionAsError("CloseOnRestart.Exception", closeContinuationTask.Exception, "{0} context", this.Tracer);
            }
            else
            {
                FabricEvents.Events.CloseOnRestart("Finish@" + this.TraceType, this.StreamIdentity.ToString());
            }
        }

        /// <summary>
        /// Finish deleting the stream during the restart of primary
        /// </summary>
        internal void FinishDeleteOnRestart()
        {
            FabricEvents.Events.DeleteOnRestart("Start@" + this.TraceType, this.StreamIdentity.ToString());

            try
            {
                this.FinishDeleteAsync(Timeout.InfiniteTimeSpan).ContinueWith(this.FinishDeleteOnRestartContinuation);
            }
            catch (Exception e)
            {
                Diagnostics.Assert(
                    e is FabricNotPrimaryException,
                    "{0} Target: {1} Stream.FinishDeleteAsync threw unexpected Exception during FinishDeleteOnRestart {2}",
                    this.Tracer,
                    this.PartnerIdentity,
                    e);
            }
        }

        internal void FinishDeleteOnRestartContinuation(Task deleteContinuationTask)
        {
            if (deleteContinuationTask.Exception != null)
            {
                Diagnostics.ProcessExceptionAsError("DeleteOnRestart.Exception", deleteContinuationTask.Exception, "{0} context", this.Tracer);
                Diagnostics.Assert(false, "{0} FinishDeleteOnRestartContinuation encountered exception", this.Tracer);
            }
            else
            {
                FabricEvents.Events.DeleteOnRestart("Finish@" + this.TraceType, this.StreamIdentity.ToString());
            }
        }

        /// <summary>
        /// Restart outbound stream when replica is set to primary or when partner reset happens.
        /// </summary>
        /// <param name="storeTransaction"></param>
        /// <param name="restartAfterPause"></param>
        /// <returns></returns>
        internal async Task RestartOutboundAsync(Store.IStoreTransaction storeTransaction, bool restartAfterPause)
        {
            FabricEvents.Events.RestartOutbound("Start@" + this.TraceType, this.StreamIdentity.ToString(), this.CurrentState.ToString());

            Diagnostics.Assert(this.Direction == StreamKind.Outbound, "Stream.RestartOutbound invoked for inbound stream");

            try
            {
                if (this.CurrentState == PersistentStreamState.Initialized)
                {
                    // nothing to do, the stream was created but nothing further was committed with it so far by the service
                    FabricEvents.Events.RestartOutbound("No-op@" + this.TraceType, this.StreamIdentity.ToString(), this.CurrentState.ToString());
                    return;
                }

                if (this.CurrentState == PersistentStreamState.Closed)
                {
                    // nothing to do, the stream lifecycle is finished
                    FabricEvents.Events.RestartOutbound("No-op@" + this.TraceType, this.StreamIdentity.ToString(), this.CurrentState.ToString());
                    return;
                }

                // in all other cases we need to restore the outbound driver
                this.outboundSessionDriver = await OutboundSessionDriver.FindOrCreateDriverAsync(
                    this.PartnerIdentity,
                    this.streamManager,
                    this.streamManager.SessionConnectionManager,
                    Timeout.InfiniteTimeSpan);

                if (this.CurrentState == PersistentStreamState.Opening)
                {
                    this.FinishOpenOnRestart();
                }
                else if (this.CurrentState == PersistentStreamState.Open || this.CurrentState == PersistentStreamState.Closing)
                {
                    var deleteSeqNumKey = new StreamMetadataKey(this.StreamIdentity, StreamMetadataKind.DeleteSequenceNumber);

                    var result =
                        await this.consolidatedStore.GetAsync(
                            storeTransaction,
                            new StreamStoreConsolidatedKey(deleteSeqNumKey),
                            Timeout.InfiniteTimeSpan,
                            CancellationToken.None);

                    Diagnostics.Assert(
                        result.HasValue,
                        "{0} DeleteSequenceNumber not found for active outbound stream in Stream.RestartOutboundAsync",
                        this.Tracer);

                    var deleteSeqNumBody = result.Value;
                    var deleteSequenceNumber = ((DeleteStreamSequenceNumber) deleteSeqNumBody.MetadataBody).NextSequenceNumber;

                    Func<StreamStoreConsolidatedKey, bool> filter =
                        (key) => (key.StoreKind == StreamStoreKind.MessageStore) && (key.MessageKey.StreamId == this.StreamIdentity);

                    var storeEnumerable
                        = await this.consolidatedStore.CreateEnumerableAsync(storeTransaction, filter, true, Store.ReadMode.CacheResult);

                    var messageNumberExpected = deleteSequenceNumber;

                    var syncEnumerable = storeEnumerable.ToEnumerable();

                    foreach (var nextPair in syncEnumerable)
                    {
                        var messageKey = nextPair.Key.MessageKey;
                        var messageBody = nextPair.Value.MessageBody;

                        // The reason the assert is "<=" is that an Ack can arrive while we are restarting and delete a bunch of messages
                        // to do a stricter assert we would need to use a proper transaction and lock the metadata which seems unnecessary
                        Diagnostics.Assert(
                            messageNumberExpected <= messageKey.StreamSequenceNumber,
                            "{0} Unexpected msessage key sequence number in Stream.RestartOutbound; Expected: {1} Actual: {2}",
                            this.Tracer,
                            messageNumberExpected,
                            messageKey.StreamSequenceNumber);

                        this.SendPayloadMessage(messageKey, messageBody, true);

                        messageNumberExpected++;
                    }
                }

                if (this.CurrentState == PersistentStreamState.Closing)
                {
                    var outboundParamsKey = new StreamMetadataKey(this.StreamIdentity, StreamMetadataKind.OutboundStableParameters);

                    var outboundParamsResult =
                        await this.consolidatedStore.GetAsync(
                            storeTransaction,
                            new StreamStoreConsolidatedKey(outboundParamsKey),
                            Timeout.InfiniteTimeSpan,
                            CancellationToken.None);

                    Diagnostics.Assert(
                        outboundParamsResult.HasValue,
                        string.Format(CultureInfo.InvariantCulture, "OutboundStableParameters missing for StreamId:{0} in Closing state during restart", this.StreamIdentity));

                    var outboundParams = (OutboundStreamStableParameters) outboundParamsResult.Value.MetadataBody;
                    var closeMessageSequenceNumber = outboundParams.CloseMessageSequenceNumber;

                    Diagnostics.Assert(
                        closeMessageSequenceNumber > 0,
                        string.Format(CultureInfo.InvariantCulture, "{0} found negative close sequence number in Closing state during restart", this.Tracer));

                    this.FinishCloseOnRestart(closeMessageSequenceNumber);
                }

                if (this.CurrentState == PersistentStreamState.Deleting)
                {
                    this.FinishDeleteOnRestart();
                }
            }
            catch (Exception e)
            {
                if (e is FabricNotPrimaryException)
                {
                    FabricEvents.Events.NotPrimaryException(
                        "RestartOutbound@" + this.TraceType,
                        this.StreamIdentity.ToString(),
                        storeTransaction.Id.ToString());
                    throw;
                }

                if (e is FabricObjectClosedException)
                {
                    FabricEvents.Events.RestartOutbound("ObjectClosed@" + this.TraceType, this.StreamIdentity.ToString(), this.PartnerIdentity.ToString());
                    throw;
                }

                if (e is FabricNotReadableException)
                {
                    if (this.streamManager.NotWritable())
                    {
                        FabricEvents.Events.NotPrimaryException(
                            "RestartOutbound@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            storeTransaction.Id.ToString());
                        throw new FabricNotPrimaryException();
                    }
                }

                Tracing.WriteExceptionAsError("Stream.RestartOutbound.UnexpectedException", e, "{0}", this.Tracer);
                Diagnostics.Assert(false, "{0} Stream RestartOutbound UnexpectedException {1}", this.Tracer, e);
            }
            finally
            {
                if (restartAfterPause)
                {
                    Diagnostics.Assert(
                        this.pauseSyncPoint != null,
                        "{0} found null pauseSyncPoint in finally clause of RestartOutboundAsync",
                        this.Tracer);

                    var tempSyncPoint = this.pauseSyncPoint;
                    this.pauseSyncPoint = null;
                    tempSyncPoint.Dispose();
                }
            }

            FabricEvents.Events.RestartOutbound("Finish@" + this.TraceType, this.StreamIdentity.ToString(), this.CurrentState.ToString());
        }

        // TODO: Need to deal with stream states other than Open on restart

        /// <summary>
        /// Restart Inbound stream when role is set to primary or when partner reset happens.
        /// </summary>
        /// <returns></returns>
        internal async Task RestartInboundAsync()
        {
            FabricEvents.Events.RestartInbound("Start@" + this.TraceType, this.StreamIdentity.ToString(), this.CurrentState.ToString());
            Diagnostics.Assert(this.Direction == StreamKind.Inbound, "Stream.RestartInbound invoked for outbound stream");

            this.outboundSessionDriver = await OutboundSessionDriver.FindOrCreateDriverAsync(
                this.PartnerIdentity,
                this.streamManager,
                this.streamManager.SessionConnectionManager,
                Timeout.InfiniteTimeSpan);

            if (this.CurrentState == PersistentStreamState.Open)
            {
                var receiveNumber = StreamConstants.FirstMessageSequenceNumber;

                var getSuccess = this.streamManager.RuntimeResources.NextSequenceNumberToDelete.TryGetValue(this.StreamIdentity, out receiveNumber);
                Diagnostics.Assert(getSuccess, "{0} Failed to find NextSequenceNumberToDelete for active inbound stream", this.Tracer);

                // receiveNumber is the (committed) next expected inbound number--starting with FirstMessageSequenceNumber
                // We don't need to ack if the stream is closed since the outbound side will retry CloseStream if needed on recovery
                if (receiveNumber > StreamConstants.FirstMessageSequenceNumber)
                {
                    this.SendAck(receiveNumber - 1);
                }
            }

            if (this.CurrentState == PersistentStreamState.Closed)
            {
                this.SendCloseResponse();
            }

            FabricEvents.Events.RestartInbound("Finish@" + this.TraceType, this.StreamIdentity.ToString(), this.CurrentState.ToString());
        }

        #endregion

        #region Inbound Messaging Driver

        /// <summary>
        /// This is used by StreamManager to remove messages on metadata store commit which confirms that receives committed 
        /// </summary>
        /// <param name="nextReceiveSSN"></param>
        internal void RemoveMessages(long nextReceiveSSN)
        {
            // TODO: we may need to periodically flush messages that come in as retries and have sequence numbers below NextSequenceNumberToDelete

            lock (this.removeMessageSync)
            {
                var firstSequenceNumberToRemove = StreamConstants.FirstMessageSequenceNumber;
                var getSuccess = this.streamManager.RuntimeResources.NextSequenceNumberToDelete.TryGetValue(
                    this.StreamIdentity,
                    out firstSequenceNumberToRemove);
                Diagnostics.Assert(getSuccess, "NextSequenceNumberToDelete not found for active inbound stream");
                // it is possible that firstSequenceNumberToRemove == nextReceiveSSN if receive timed out and the surrounding transaction committed anyway
                // it is also possible that two racing transaction commits for receive will cause store notifications out of order
                if (firstSequenceNumberToRemove >= nextReceiveSSN)
                {
                    return;
                }

                // check if the close message was transactionally received, and also send receive ack
                // TODO: look into making this less chatty, perhaps with a timer
                for (var seqNum = firstSequenceNumberToRemove; seqNum < nextReceiveSSN; seqNum++)
                {
                    // it is possible that commit occurs without waiting for all ReceiveAsyncs to be fulfilled in which case there is a very small chance 
                    // remove will not succeed because the message has not yet been received from the session, hence we will not assert removeSuccess
                    var removeSuccess = this.RemoveMessage(seqNum);
                }

                var updateSuccess = this.streamManager.RuntimeResources.NextSequenceNumberToDelete.TryUpdate(
                    this.StreamIdentity,
                    nextReceiveSSN,
                    firstSequenceNumberToRemove);
                Diagnostics.Assert(updateSuccess, "Update of NextSequenceNumberToDelete failed");
            }
        }

        /// <summary>
        /// Once receive message is processed remove from the cache
        /// </summary>
        /// <param name="seqNum"></param>
        /// <returns></returns>
        private bool RemoveMessage(long seqNum)
        {
            InboundBaseStreamWireMessage removedMessage = null;
            var result = this.receivedMessages.TryRemove(seqNum, out removedMessage);
            return result;
        }

        /// <summary>
        /// This is an waiter that represents an unfulfilled Receive when it cannot be fulfilled immediately.
        /// </summary>
        /// <param name="storeTransaction"></param>
        /// <param name="completionHandle"></param>
        internal void AddWaiter(Store.IStoreReadWriteTransaction storeTransaction, MessageWaiter completionHandle)
        {
            lock (this.waiterSync)
            {
                // lock needed to make sure waiters don't starve: 
                // if the waiter registers after the message arrives it will immediately get matched 
                // message removal occurs at commit of the receiving transaction
                InboundBaseStreamWireMessage receivedMessage = null;
                var messageExists = this.receivedMessages.TryGetValue(completionHandle.ExpectedSequenceNumber, out receivedMessage);

                if (messageExists)
                {
                    // guard against the thread being hijacked by the SetResult
                    Task.Run(async () => await completionHandle.SetResult(receivedMessage));
                }
                else
                {
                    var addSuccess = this.receiveWaiters.TryAdd(storeTransaction.Id, completionHandle);

                    Diagnostics.Assert(
                        addSuccess,
                        "{0} Duplicate MessageWaiter Found ExpectedSequenceNumber: {1} TransactionId: {2}",
                        this.Tracer,
                        completionHandle.ExpectedSequenceNumber,
                        storeTransaction.Id);
                }
            }
        }

        /// <summary>
        /// Process received wire message
        /// </summary>
        /// <param name="receivedMessage"></param>
        internal void MessageReceived(InboundBaseStreamWireMessage receivedMessage)
        {
            var streamSequenceNumber = receivedMessage.MessageSequenceNumber;

            if (receivedMessage.Kind == StreamWireProtocolMessageKind.CloseStream)
            {
                this.closeMessage = receivedMessage;
                this.SendCloseResponseIfAppropriate().ContinueWith(this.SendCloseResponseIfAppropriateContinuation);
            }

            // the message is always inserted here regardless of whether it matches a waiter, 
            // message removal occurs at commit of the receiving transaction
            // TODO: make sure the message is always deleted when consumed and will not reappear
            // add may not succeed in some recovery scenarios but the message we are trying to add must have an identical payload
            var addSuccess = false;
            var nextReceiveNumber = StreamConstants.FirstMessageSequenceNumber;
            var getSuccess = this.streamManager.RuntimeResources.NextSequenceNumberToDelete.TryGetValue(this.StreamIdentity, out nextReceiveNumber);
            Diagnostics.Assert(getSuccess, "{0} Failed to find NextSequenceNumberToDelete for active inbound stream", this.Tracer);

            if (streamSequenceNumber >= nextReceiveNumber)
            {
                // may be a duplicate
                addSuccess = this.receivedMessages.TryAdd(streamSequenceNumber, receivedMessage);
            }

            if (!addSuccess)
            {
                FabricEvents.Events.MessageDroppedAsDuplicate(this.TraceType, this.StreamIdentity.ToString(), receivedMessage.MessageSequenceNumber);
            }

            var matchedWaiters = new Dictionary<long, MessageWaiter>();

            // TODO: receivedMessages and receiveWaiters probably don't need to be Concurrent
            // lock needed to make sure waiters don't starve: 
            // if the waiter registers before the message arrives it is matched here
            lock (this.waiterSync)
            {
                foreach (var pair in this.receiveWaiters)
                {
                    var transactionId = pair.Key;
                    var waiter = pair.Value;

                    if (waiter.ExpectedSequenceNumber == streamSequenceNumber)
                    {
                        matchedWaiters.Add(transactionId, waiter);
                    }
                }


                foreach (var pair in matchedWaiters)
                {
                    var transactionId = pair.Key;
                    var waiter = pair.Value;
                    MessageWaiter removedWaiter;
                    var removeSuccess = this.receiveWaiters.TryRemove(transactionId, out removedWaiter);

                    Diagnostics.Assert(
                        removeSuccess,
                        "Stream.MessageReceived.SetResult.MissingWaiter {0} ExpectedSequenceNumber: {1} TransactionId: {2}",
                        this.Tracer,
                        waiter.ExpectedSequenceNumber,
                        transactionId);

                    Diagnostics.Assert(
                        waiter == removedWaiter,
                        "{0} removed waiter mismatch in Stream.MessageReceived SequenceNumber: {1} TransactionId: {2}",
                        this.Tracer,
                        waiter.ExpectedSequenceNumber,
                        transactionId);

                    Task.Run(async () => await waiter.SetResult(receivedMessage));
                }
            }
        }

        /// <summary>
        /// Used by the transaction notification function in StreamManager to discharge waiters in a closed transaction
        /// </summary>
        /// <param name="storeTransactionId"></param>
        internal void CloseWaiter(long storeTransactionId)
        {
            // we could do this under a  syncPoint guarded by this.streamManager.RuntimeResources.ReceiveInTransactionContextSyncpoints but we won't 
            // because it is too onerous and doesn't eliminate all races so it is possible that some waiters will stop responding after the transaction aborts

            MessageWaiter closedWaiter;
            bool removeSuccess;

            lock (this.waiterSync)
            {
                removeSuccess = this.receiveWaiters.TryRemove(storeTransactionId, out closedWaiter);
            }

            if (removeSuccess)
            {
                closedWaiter.SetTransactionClosed(storeTransactionId);
            }
        }

        /// <summary>
        ///  Used by the timeout function in MessageWaiter when timeout is set successfully
        /// </summary>
        /// <param name="expectedSequenceNumber"></param>
        /// <param name="transactionContext"></param>
        /// <param name="traceId"></param>
        /// <returns></returns>
        internal bool DropWaiter(long expectedSequenceNumber, Store.IStoreReadWriteTransaction transactionContext, long traceId)
        {
            var transactionId = transactionContext.Id;
            var removeSuccess = false;

            lock (this.waiterSync)
            {
                MessageWaiter waiter;
                var getSuccess = this.receiveWaiters.TryGetValue(transactionId, out waiter);

                if (getSuccess && (waiter.ExpectedSequenceNumber == expectedSequenceNumber))
                {
                    MessageWaiter removedWaiter;

                    removeSuccess = this.receiveWaiters.TryRemove(transactionId, out removedWaiter);

                    if (removeSuccess)
                    {
                        Diagnostics.Assert(
                            waiter == removedWaiter,
                            "{0} removed waiter mismatch in Stream.MessageReceived SequenceNumber: {1} TransactionId: {2}",
                            this.Tracer,
                            waiter.ExpectedSequenceNumber,
                            transactionId);

                        FabricEvents.Events.ReceiveFailure(
                            "TimeoutInWaiter::" + traceId + "@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            expectedSequenceNumber,
                            transactionContext.ReplicatorTransactionBase.ToString());
                    }
                    else
                    {
                        FabricEvents.Events.ReceiveFailure(
                            "TimeoutWaiterDropFailed::" + traceId + "@" + this.TraceType,
                            this.StreamIdentity.ToString(),
                            expectedSequenceNumber,
                            transactionContext.ReplicatorTransactionBase.ToString());
                    }
                }
                else
                {
                    FabricEvents.Events.ReceiveFailure(
                        "TimeoutWaiterDropFailed::" + traceId + "@" + this.TraceType,
                        this.StreamIdentity.ToString(),
                        expectedSequenceNumber,
                        transactionContext.ReplicatorTransactionBase.ToString());
                }
            }

            return removeSuccess;
        }

        #endregion

        /// <summary>
        /// Get state of the inbound stream from consolidated store.
        /// </summary>
        /// <param name="transactionContext"></param>
        /// <param name="timeout"></param>
        /// <returns></returns>
        private async Task<PersistentStreamState> GetCurrentInboundStreamStateInTransaction(Store.IStoreTransaction transactionContext, TimeSpan timeout)
        {
            Diagnostics.Assert(this.Direction == StreamKind.Inbound, "{0} GetCurrentInboundStreamStateInTransaction invoken in outbound stream", this.Tracer);

            var metadataKey = new StreamMetadataKey(this.StreamIdentity, StreamMetadataKind.InboundStableParameters);

            var getStateResult =
                await this.consolidatedStore.GetAsync(
                    transactionContext,
                    new StreamStoreConsolidatedKey(metadataKey),
                    timeout,
                    CancellationToken.None);

            Diagnostics.Assert(getStateResult.HasValue, "{0} missing InboundStableParameters for active inbound stream", this.Tracer);
            var parameters = (BaseStreamMetadataBody) getStateResult.Value.MetadataBody;

            return parameters.CurrentState;
        }

        /// <summary>
        /// We look at the current state in the store under exclusive lock.  If the state is closed it means the close message was already consumed
        /// by a committed transaction. If it is not then the commit notification processing for the transaction that transitions the persisted state 
        /// to closed will send the close response.  If this replica dies in the process of sending the close response then when rehydrated it will 
        /// send a reset streams message which will cause the outbouind side to resend CLoseStream if it is in CLosing state.
        /// This way a CloseStream message from the outbound side will not be left dangling.
        /// </summary>
        /// <returns></returns>
        private async Task SendCloseResponseIfAppropriate()
        {
            Diagnostics.Assert(this.Direction == StreamKind.Inbound, "{0} SendCloseResponseIfAppropriate invoked in outbound stream", this.Tracer);

            using (var replicatorTransaction = await this.streamManager.CreateReplicatorTransactionAsync(Timeout.InfiniteTimeSpan))
            {
                var isolationContext = this.consolidatedStore.CreateStoreReadWriteTransaction(replicatorTransaction);
                var currentState = await this.GetCurrentInboundStreamStateInTransaction(isolationContext, Timeout.InfiniteTimeSpan);

                if (currentState == PersistentStreamState.Closed)
                {
                    if (this.outboundSessionDriver == null)
                    {
                        this.outboundSessionDriver = await OutboundSessionDriver.FindOrCreateDriverAsync(
                            this.PartnerIdentity,
                            this.streamManager,
                            this.streamManager.SessionConnectionManager,
                            Timeout.InfiniteTimeSpan);
                    }

                    this.SendCloseResponse();
                }
            }
        }

        /// <summary>
        /// Send Close Response Continuation
        /// </summary>
        /// <param name="closeResponseTask"></param>
        private void SendCloseResponseIfAppropriateContinuation(Task closeResponseTask)
        {
            if (closeResponseTask.Exception != null)
            {
                if (
                    closeResponseTask.Exception.InnerException is FabricObjectClosedException ||
                    closeResponseTask.Exception.InnerException is FabricNotPrimaryException ||
                    closeResponseTask.Exception.InnerException is FabricNotReadableException)
                {
                    Diagnostics.ProcessException("SendCloseResponseIfAppropriate.Exception", closeResponseTask.Exception, "{0} context", this.Tracer);
                }
                else
                {
                    Diagnostics.ProcessExceptionAsError("SendCloseResponseIfAppropriate.Exception", closeResponseTask.Exception, "{0} context", this.Tracer);
                    Diagnostics.Assert(false, "{0} SendCloseResponseIfAppropriate encountered exception {1}", this.Tracer, closeResponseTask.Exception);
                }
            }
        }

        #region UpdateFuncs for Store.UpdateWithFunc

        private static StreamStoreConsolidatedBody OpenAsyncParamsUpdateFunc(StreamStoreConsolidatedKey key, StreamStoreConsolidatedBody body)
        {
            var parameters = new OutboundStreamStableParameters((OutboundStreamStableParameters) body.MetadataBody);

            Diagnostics.Assert(
                parameters.CurrentState == PersistentStreamState.Initialized,
                string.Format(CultureInfo.InvariantCulture, "{0} Stream state != Initialized in OpenAsyncParamsUpdateFunc", key));

            parameters.CurrentState = PersistentStreamState.Opening;

            return new StreamStoreConsolidatedBody(parameters);
        }

        private static StreamStoreConsolidatedBody CloseAsyncParamsUpdateFunc(
            long closeMessageSequenceNumber, StreamStoreConsolidatedKey key, StreamStoreConsolidatedBody body)
        {
            var parameters = new OutboundStreamStableParameters((OutboundStreamStableParameters) body.MetadataBody);

            Diagnostics.Assert(
                parameters.CurrentState == PersistentStreamState.Open,
                string.Format(CultureInfo.InvariantCulture, "{0} Stream state != Open in CloseAsyncParamsUpdateFunc", key));

            parameters.CurrentState = PersistentStreamState.Closing;
            parameters.CloseMessageSequenceNumber = closeMessageSequenceNumber;

            return new StreamStoreConsolidatedBody(parameters);
        }

        private static StreamStoreConsolidatedBody DeleteAsyncParamsUpdateFunc(StreamStoreConsolidatedKey key, StreamStoreConsolidatedBody body)
        {
            var parameters = new OutboundStreamStableParameters((OutboundStreamStableParameters) body.MetadataBody);

            Diagnostics.Assert(
                parameters.CurrentState == PersistentStreamState.Closed,
                string.Format(CultureInfo.InvariantCulture, "{0} Stream state != Closed in DeleteAsyncParamsUpdateFunc", key));

            parameters.CurrentState = PersistentStreamState.Deleting;

            return new StreamStoreConsolidatedBody(parameters);
        }

        /// <summary>
        /// Update stream state based on target response for Open stream request.
        /// </summary>
        /// <param name="response"></param>
        /// <param name="key"></param>
        /// <param name="body"></param>
        /// <returns></returns>
        private static StreamStoreConsolidatedBody CompleteOpenStreamUpdateFunc(
            StreamWireProtocolResponse response, StreamStoreConsolidatedKey key, StreamStoreConsolidatedBody body)
        {
            var outboundParameters = new OutboundStreamStableParameters((OutboundStreamStableParameters) body.MetadataBody);

            // CompleteOpenStreamProtocol is idempotent: retries allowed
            Diagnostics.Assert(
                outboundParameters.CurrentState == PersistentStreamState.Opening || outboundParameters.CurrentState == PersistentStreamState.Open,
                string.Format(
                    CultureInfo.InvariantCulture,
                    "Unexpected outbound stream state: {0} in StreamManager.CompleteOpenStreamProtocol for StreamId: {1}",
                    outboundParameters.CurrentState,
                    key.StreamId));

            Diagnostics.Assert(
                response != StreamWireProtocolResponse.TargetNotReady,
                string.Format(
                    CultureInfo.InvariantCulture,
                    "Unexpected TargetNotReady response value in Stream.CompleteOpenStreamUpdateFunc StreamId: {0}",
                    key.StreamId));


            outboundParameters.AcceptanceResponse = response;

            outboundParameters.CurrentState = response == StreamWireProtocolResponse.StreamAccepted ? PersistentStreamState.Open : PersistentStreamState.Closed;

            return new StreamStoreConsolidatedBody(outboundParameters);
        }

        private static StreamStoreConsolidatedBody CompleteCloseStreamUpdateFunc(StreamStoreConsolidatedKey key, StreamStoreConsolidatedBody body)
        {
            var parameters = new OutboundStreamStableParameters((OutboundStreamStableParameters) body.MetadataBody);

            // CompleteCloseStreamProtocol is idempotent: retries allowed
            Diagnostics.Assert(
                parameters.CurrentState == PersistentStreamState.Closing || parameters.CurrentState == PersistentStreamState.Closed,
                string.Format(
                    CultureInfo.InvariantCulture,
                    "Unexpected outbound stream state in StreamManager.CompleteCloseStreamProtocol, State: {0}",
                    parameters.CurrentState));
            parameters.CurrentState = PersistentStreamState.Closed;

            return new StreamStoreConsolidatedBody(parameters);
        }

        private static StreamStoreConsolidatedBody CompleteDeleteStreamUpdateFunc(StreamStoreConsolidatedKey key, StreamStoreConsolidatedBody body)
        {
            var parameters = new OutboundStreamStableParameters((OutboundStreamStableParameters) body.MetadataBody);

            // CompleteDeleteStreamProtocol is idempotent: retries allowed
            Diagnostics.Assert(
                parameters.CurrentState == PersistentStreamState.Deleting || parameters.CurrentState == PersistentStreamState.Deleted,
                string.Format(
                    CultureInfo.InvariantCulture,
                    "Unexpected outbound stream state in StreamManager.CompleteDeleteStreamProtocol, State: {0}",
                    parameters.CurrentState));
            parameters.CurrentState = PersistentStreamState.Deleted;

            return new StreamStoreConsolidatedBody(parameters);
        }

        private static StreamStoreConsolidatedBody ReceiveAsyncSeqNumIncrementFunc(StreamStoreConsolidatedKey key, StreamStoreConsolidatedBody body)
        {
            var receiveSeqNum = (ReceiveStreamSequenceNumber) body.MetadataBody;
            return new StreamStoreConsolidatedBody(new ReceiveStreamSequenceNumber(receiveSeqNum.NextSequenceNumber + 1));
        }

        private static StreamStoreConsolidatedBody SendAsyncSeqNumIncrementFunc(StreamStoreConsolidatedKey key, StreamStoreConsolidatedBody body)
        {
            var sendSeqNum = (SendStreamSequenceNumber) body.MetadataBody;
            return new StreamStoreConsolidatedBody(new SendStreamSequenceNumber(sendSeqNum.NextSequenceNumber + 1));
        }

        private static StreamStoreConsolidatedBody ReceiveAsyncSeqNumDecrementFunc(StreamStoreConsolidatedKey key, StreamStoreConsolidatedBody body)
        {
            var receiveSeqNum = (ReceiveStreamSequenceNumber) body.MetadataBody;
            return new StreamStoreConsolidatedBody(new ReceiveStreamSequenceNumber(receiveSeqNum.NextSequenceNumber - 1));
        }

        private static StreamStoreConsolidatedBody AckReceivedUpdateFunc(
            long ackedSequenceNumber, StreamStoreConsolidatedKey key, StreamStoreConsolidatedBody body)
        {
            var deleteSeqNum = (DeleteStreamSequenceNumber) body.MetadataBody;

            if (deleteSeqNum.NextSequenceNumber <= ackedSequenceNumber)
            {
                return new StreamStoreConsolidatedBody(new DeleteStreamSequenceNumber(ackedSequenceNumber + 1));
            }

            return new StreamStoreConsolidatedBody(new DeleteStreamSequenceNumber(deleteSeqNum.NextSequenceNumber));
        }

        private static StreamStoreConsolidatedBody InboundStreamClosedUpdateFunc(StreamStoreConsolidatedKey key, StreamStoreConsolidatedBody body)
        {
            var parameters = new InboundStreamStableParameters((InboundStreamStableParameters) body.MetadataBody);

            if (parameters.CurrentState == PersistentStreamState.Open)
            {
                // normal case
                parameters.CurrentState = PersistentStreamState.Closed;
            }
            else
            {
                // During recovery the sender may see a Closing state and resend the CloseStream message
                Diagnostics.Assert(
                    parameters.CurrentState == PersistentStreamState.Closed,
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "Unexpected stream state {0} in StreamManager.InboundStreamClosed Key: {1}",
                        parameters.CurrentState,
                        key));
            }

            return new StreamStoreConsolidatedBody(parameters);
        }

        private static StreamStoreConsolidatedBody SetCloseMessageSequenceNumberUpdateFunc(
            long closeMessageSequenceNumber, StreamStoreConsolidatedKey key, StreamStoreConsolidatedBody body)
        {
            var parameters = new InboundStreamStableParameters((InboundStreamStableParameters) body.MetadataBody)
            {
                CloseMessageSequenceNumber = closeMessageSequenceNumber
            };
            return new StreamStoreConsolidatedBody(parameters);
        }

        private static StreamStoreConsolidatedBody InboundStreamDeletedUpdateFunc(StreamStoreConsolidatedKey key, StreamStoreConsolidatedBody body)
        {
            var parameters = new InboundStreamStableParameters((InboundStreamStableParameters) body.MetadataBody);

            if (parameters.CurrentState == PersistentStreamState.Closed)
            {
                // normal case
                parameters.CurrentState = PersistentStreamState.Deleted;
            }
            else
            {
                // During recovery the sender may see a Deleted state and resend the DeleteStream message
                Diagnostics.Assert(
                    parameters.CurrentState == PersistentStreamState.Deleted,
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "Unexpected stream state {0} in StreamManager.InboundStreamDeleted Key: {1}",
                        parameters.CurrentState,
                        key));
            }

            return new StreamStoreConsolidatedBody(parameters);
        }

        #endregion

        /// <summary>
        /// Delete the given outbound stream and related metadata from the consolidated store.
        /// </summary>
        internal async Task DeleteOutboundStreamFromConsolidatedStore(
            Transaction txn, Guid streamId, Uri streamName, PartitionKey targetPartition, TimeSpan timeout)
        {
            FabricEvents.Events.DeleteOutboundStreamFromConsolidatedStore("Start@" + this.TraceType, streamId.ToString());

            var txnLookupResult = this.consolidatedStore.CreateOrFindTransaction(txn);
            var storeTransaction = txnLookupResult.Value;
            storeTransaction.Isolation = Store.StoreTransactionReadIsolationLevel.ReadCommitted;
            storeTransaction.LockingHints = Store.LockingHints.UpdateLock;

            try
            {
                // Remove stream metadata info (OutboundStable).
                // Removing this first to ensure, to avoid a race when Open() is called on an initialized stream.
                var startChange = DateTime.UtcNow;
                var remainingTimeout = timeout;
                await this.consolidatedStore.TryRemoveAsync(
                    storeTransaction,
                    new StreamStoreConsolidatedKey(new StreamMetadataKey(streamId, StreamMetadataKind.OutboundStableParameters)),
                    remainingTimeout,
                    CancellationToken.None);
                FabricEvents.Events.DeleteOutboundStreamFromConsolidatedStore("Stable@" + this.TraceType, streamId.ToString());

                // it may be possible we timeout before we complete this series of delete txns, 
                // and it may be possible that the user may commit on a dirty (timeout exception) txn, to
                // avoid this issue, we will execute the remaining deletes with infinite timeouts.

                // Remove stream def.
                await this.consolidatedStore.TryRemoveAsync(
                    storeTransaction,
                    new StreamStoreConsolidatedKey(new StreamNameKey(streamName, targetPartition)),
                    Timeout.InfiniteTimeSpan,
                    CancellationToken.None);
                FabricEvents.Events.DeleteOutboundStreamFromConsolidatedStore("StreamName@" + this.TraceType, streamId.ToString());

                // Remove stream metadata info (Send Seq #).
                await this.consolidatedStore.TryRemoveAsync(
                    storeTransaction,
                    new StreamStoreConsolidatedKey(
                        new StreamMetadataKey(
                            streamId,
                            StreamMetadataKind.SendSequenceNumber)),
                    Timeout.InfiniteTimeSpan,
                    CancellationToken.None);
                FabricEvents.Events.DeleteOutboundStreamFromConsolidatedStore("SendSequenceNumber@" + this.TraceType, streamId.ToString());

                // Remove stream metadata info (DeleteSeq.#).
                await this.consolidatedStore.TryRemoveAsync(
                    storeTransaction,
                    new StreamStoreConsolidatedKey(
                        new StreamMetadataKey(
                            streamId,
                            StreamMetadataKind.DeleteSequenceNumber)),
                    Timeout.InfiniteTimeSpan,
                    CancellationToken.None);
                FabricEvents.Events.DeleteOutboundStreamFromConsolidatedStore("DeleteSequenceNumber@" + this.TraceType, streamId.ToString());
            }
            catch (ArgumentException e)
            {
                Tracing.WriteExceptionAsError("DeleteOutboundStreamFromConsolidatedStore", e, "{0} failed for StreamId: {1}", this.Tracer, streamId);
                Diagnostics.Assert(false, "{0} Unexpected Argument exception {1} in DeleteOutboundStreamFromConsolidatedStore", this.Tracer, e);
            }
            catch (FabricNotPrimaryException)
            {
                FabricEvents.Events.DeleteOutboundStreamFromConsolidatedStore("NotPrimary@" + this.TraceType, streamId + "::" + txn);
                throw;
            }
            catch (FabricObjectClosedException)
            {
                FabricEvents.Events.DeleteOutboundStreamFromConsolidatedStore("ObjectClosed@" + this.TraceType, streamId + "::" + txn);
                throw;
            }
            catch (Exception e)
            {
                Tracing.WriteExceptionAsError("DeleteOutboundStreamFromConsolidatedStore", e, "{0} failed for StreamId: {1}", this.Tracer, streamId);
                Diagnostics.Assert(false, "{0} Unexpected exception {1} in DeleteOutboundStreamFromConsolidatedStore", this.Tracer, e);
            }

            FabricEvents.Events.DeleteOutboundStreamFromConsolidatedStore("Finish@" + this.TraceType, streamId.ToString());
        }
    }
}
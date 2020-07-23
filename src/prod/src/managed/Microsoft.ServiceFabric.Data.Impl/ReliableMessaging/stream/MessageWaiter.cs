// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    using System.Fabric.ReliableMessaging;
    using System.Globalization;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;
    using Microsoft.ServiceFabric.Data;

    internal class MessageWaiter : TaskCompletionSource<IInboundStreamMessage>
    {
        private readonly Store.IStoreReadWriteTransaction transactionContext;
        private readonly MessageStream stream;
        private readonly long traceId;

        internal long ExpectedSequenceNumber { get; private set; }

        internal MessageWaiter(MessageStream stream, long expectedSequenceNumber, Store.IStoreReadWriteTransaction context, long traceId)
        {
            this.transactionContext = context;
            this.stream = stream;
            this.ExpectedSequenceNumber = expectedSequenceNumber;
            this.traceId = traceId;
        }

        internal void SetTransactionClosed(long transactionId)
        {
            Diagnostics.Assert(
                transactionId == this.transactionContext.Id,
                "{0} Unexpected mismatch of transactionIds when setting FabricObjectClosedException on receive waiter with ExpectedSequenceNumber: {1} Closed TransactionId: {2} Waiter TransactionId: {3}",
                this.stream.Tracer,
                this.ExpectedSequenceNumber,
                transactionId,
                this.transactionContext.Id);

            var setTransactionClosedSuccess = this.TrySetException(new FabricObjectClosedException());

            if (setTransactionClosedSuccess)
            {
                FabricEvents.Events.Receive(
                    "TransactionAborted::" + this.traceId + "@" + this.stream.TraceType,
                    this.stream.StreamIdentity.ToString(),
                    this.ExpectedSequenceNumber,
                    transactionId.ToString());
            }
            else
            {
                FabricEvents.Events.Receive(
                    "SetTransactionClosedFailure::" + this.traceId + "@" + this.stream.TraceType,
                    this.stream.StreamIdentity.ToString(),
                    this.ExpectedSequenceNumber,
                    transactionId.ToString());
            }
        }

        internal new void SetResult(IInboundStreamMessage message)
        {
            throw new InvalidOperationException(SR.Error_MessageAwaiter_SetResult_Parameter);
        }

        internal async Task SetTimeout(TimeSpan timeout)
        {
            var finished = await TimeoutHandler.WaitWithDelay(timeout, this.Task);

            if (!finished)
            {
                // try to drop the waiter before setting the exception because TrySetException will hijack the thread and will cause the underlying ReceiveAsync to complete
                var dropSuccess = this.stream.DropWaiter(this.ExpectedSequenceNumber, this.transactionContext, this.traceId);

                // if we did not lose a race with an incoming message the drop will succeed
                if (dropSuccess)
                {
                    var success = this.TrySetException(new TimeoutException());

                    Diagnostics.Assert(
                        success,
                        "{0} Unexpected Failure to Set Waiter TimeoutException for Stream: {1} ExpectedSequenceNumber: {2} TransactionContext: {3}",
                        this.stream.TraceType,
                        this.stream.StreamIdentity.ToString(),
                        this.ExpectedSequenceNumber,
                        this.transactionContext.ToString());
                }
            }
        }

        private void SetTimeoutContinuation(Task<bool> task)
        {
            if (task.Result)
            {
                // timeout occurred
                Diagnostics.Assert(
                    this.Task.IsFaulted,
                    "{0} Timeout signal inconsistent with with non-faulted task in message waiter, SequenceNumber: {1} Transaction: {2}",
                    this.stream.Tracer,
                    this.ExpectedSequenceNumber,
                    this.transactionContext.Id);
            }
        }

        // Hides the base SetResult method which we need to override
        // This method is called when the message is actually delivered to a ReceiveAsync call
        // The update to the CloseMessageSequenceNumber property of Inbound is transactional
        internal async Task SetResult(InboundBaseStreamWireMessage message)
        {
            var closeSeqNumber = this.stream.CloseMessageSequenceNumber;

            if (message.Kind == StreamWireProtocolMessageKind.CloseStream)
            {
                // The close message can be a repeat in recovery cases, but the close sequence number must be unique
                // It is also possible that the transaction surrounding this ReceiveAsync will abort and the message 
                // will not in fact be consumed -- and will be received again, but the close sequence number must be unique
                Diagnostics.Assert(
                    closeSeqNumber == StreamConstants.InitialValueOfLastSequenceNumberInStream || closeSeqNumber == message.MessageSequenceNumber,
                    "{0} TraceId::{1} encountered multiple close sequence number values {2} and {3}",
                    this.stream.Tracer,
                    this.traceId,
                    closeSeqNumber,
                    message.MessageSequenceNumber);

                try
                {
                    // this method is idempotent; the close sequence number for a stream is invariant
                    await this.stream.SetCloseMessageSequenceNumber(message.MessageSequenceNumber, this.transactionContext);
                    await this.stream.CloseInboundStream(this.transactionContext);
                }
                catch (Exception e)
                {
                    if (e is FabricObjectClosedException)
                    {
                        FabricEvents.Events.DataMessageDelivery(
                            string.Format(CultureInfo.InvariantCulture, "ObjectClosed@{0}{1}", this.traceId, this.stream.TraceType),
                            this.stream.StreamIdentity.ToString(),
                            message.MessageSequenceNumber,
                            this.transactionContext.Id.ToString());

                        this.TrySetException(e);
                        return;
                    }
                    if (e is FabricNotPrimaryException)
                    {
                        FabricEvents.Events.DataMessageDelivery(
                            string.Format(CultureInfo.InvariantCulture, "NotPrimary@{0}{1}", this.traceId, this.stream.TraceType),
                            this.stream.StreamIdentity.ToString(),
                            message.MessageSequenceNumber,
                            this.transactionContext.Id.ToString());

                        this.TrySetException(e);
                        return;
                    }
                    if (e is FabricNotReadableException)
                    {
                        FabricEvents.Events.DataMessageDelivery(
                            string.Format(CultureInfo.InvariantCulture, "NotReadable@{0}{1}", this.traceId, this.stream.TraceType),
                            this.stream.StreamIdentity.ToString(),
                            message.MessageSequenceNumber,
                            this.transactionContext.Id.ToString());

                        this.TrySetException(e);
                        return;
                    }

                    Tracing.WriteExceptionAsError(
                        "DataMessageDelivery.Failure",
                        e,
                        "Partition::Replica {0} TraceId: {1} MessageNumber: {2} TransactionId: {3}",
                        this.stream.TraceType,
                        this.traceId,
                        message.MessageSequenceNumber,
                        this.transactionContext.Id);
                    Diagnostics.Assert(
                        false,
                        "Unexpected Exception In {0} TraceId: {1} MessageNumber: {2} TransactionId: {3}",
                        this.stream.TraceType,
                        this.traceId,
                        message.MessageSequenceNumber,
                        this.transactionContext.Id);
                }
            }
            else
            {
                Diagnostics.Assert(
                    closeSeqNumber == StreamConstants.InitialValueOfLastSequenceNumberInStream || closeSeqNumber > message.MessageSequenceNumber,
                    "{0} received payload message with sequence number {1} which is greater than the close sequence number {2}",
                    this.stream.Tracer,
                    message.MessageSequenceNumber,
                    closeSeqNumber);
            }

            var setResultSuccess = this.TrySetResult(message);

            if (setResultSuccess)
            {
                FabricEvents.Events.DataMessageDelivery(
                    "Success::" + this.traceId + "@" + this.stream.TraceType,
                    this.stream.StreamIdentity.ToString(),
                    message.MessageSequenceNumber,
                    this.transactionContext.Id.ToString());
            }
            else
            {
                FabricEvents.Events.DataMessageDelivery(
                    "Failure::" + this.traceId + "@" + this.stream.TraceType,
                    this.stream.StreamIdentity.ToString(),
                    message.MessageSequenceNumber,
                    this.transactionContext.Id.ToString());
            }
        }
    }
}
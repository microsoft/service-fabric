// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    #region using directives

    using System.Fabric.ReliableMessaging;
    using System.Fabric.ReliableMessaging.Session;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;

    #endregion

    /// <summary>
    /// This class provides the means to listen and receive all Inbound Messages from an InboundSession and
    /// to allow for corresponding  updates in various streams managed by the StreamManager.
    /// </summary>
    internal class InboundSessionDriver : IDisposable
    {
        // Refers to the active inbound session managed in the session connection manager
        private readonly IReliableMessagingSession inboundSession;
        // For a specific partner
        private readonly PartitionKey partnerKey;
        // Co-ordinates message updates to various streams for the specified partition(Key) that are managed by the StreamManager
        private readonly StreamManager streamManager;
        private readonly string tracer;
        private readonly string traceType;
        // Inbound Session id
        internal Guid SessionId { get; private set; }

        #region cstor

        /// <summary>
        /// Initializes a new instance of the InboundSessionDriver class for a given partition.
        /// </summary>
        /// <param name="partnerKey">Partner Key</param>
        /// <param name="inboundSession">Active Inbound Session</param>
        /// <param name="streamManager">Update corresponding streams in the StreamManager</param>
        internal InboundSessionDriver(PartitionKey partnerKey, IReliableMessagingSession inboundSession, StreamManager streamManager)
        {
            this.partnerKey = partnerKey;
            this.inboundSession = inboundSession;
            this.streamManager = streamManager;

            this.tracer = this.streamManager.Tracer + " Source: " + this.partnerKey;
            this.traceType = this.streamManager.TraceType;

            var snapshot = inboundSession.GetDataSnapshot();
            this.SessionId = snapshot.SessionId;

            // Start some async receive operations and wait for message to come through.
            // Start the corresponding continuation operations when a message is received.
            // TODO: Should the number of receive  correspond to the sessionReceiveQuota.
            // park some receive operations with the session
            for (var i = 0; i < StreamConstants.ConcurrentReceiveOperationCount; i++)
            {
                this.ReceiveOnSession();
            }
        }

        #endregion

        /// <summary>
        /// Aborting the local session.
        /// </summary>
        public void Dispose()
        {
            this.inboundSession.Abort();
        }

        #region methods

        /// <summary>
        /// We park X ReceiveAsyncs with the underlying session using ContinueWith to complete the work
        /// stream quota operates only on the send side since messages are stored only on the send side
        ///
        /// Creates a receive continuation task when a message is received in the inbound session.
        /// TODO: incorporate retry on quota exceeded, return true if we don't encounter session aborted exceptions, false otherwise
        /// </summary>
        /// <returns>Returns false if Receive Quota exceeded, other true</returns>
        internal bool ReceiveOnSession()
        {
            var sessionReceiveQuotaExceeded = false;
            try
            {
                this.inboundSession.ReceiveAsync(CancellationToken.None).ContinueWith(this.ReceiveContinuation);
            }
            catch (Exception e)
            {
                if (e is InvalidOperationException || e is OperationCanceledException)
                {
                    FabricEvents.Events.SessionReceiveFailure(
                        string.Format(CultureInfo.InvariantCulture, "SessionAborted@{0}", this.streamManager.TraceType),
                        this.SessionId.ToString());
                }
                else if (e is ReliableSessionQuotaExceededException)
                {
                    FabricEvents.Events.SessionReceiveFailure(
                        string.Format(CultureInfo.InvariantCulture, "QuotaExceeded@{0}", this.streamManager.TraceType),
                        this.SessionId.ToString());
                    sessionReceiveQuotaExceeded = true;
                }
                else
                {
                    Diagnostics.Assert(
                        false,
                        "{0} InboundSessionDriver.ReceiveAsync Unexpected Exception {1}",
                        this.tracer,
                        e.GetType());
                }
            }

            return sessionReceiveQuotaExceeded;
        }

        /// <summary>
        /// Handler in lieu of await for Stream.AckReceived
        /// </summary>
        /// <param name="task"></param>
        /// <param name="streamId"></param>
        private void AckReceivedContinuation(Task task, Guid streamId)
        {
            // TODO: exception handling
            if (task.Exception != null)
            {
                if (task.Exception.InnerException is FabricObjectClosedException || task.Exception.InnerException is FabricNotPrimaryException)
                {
                    Tracing.WriteExceptionAsWarning("Stream.AckReceived.Failure", task.Exception, "{0} StreamId: {1}", this.tracer, streamId);
                }
                else
                {
                    Tracing.WriteExceptionAsError("Stream.AckReceived.Failure", task.Exception, "{0} StreamId: {1}", this.tracer, streamId);
                }
            }
        }

        /// <summary>
        /// Handler in lieu of await for Stream.MessageReceived
        /// </summary>
        /// <param name="task"></param>
        /// <param name="streamId"></param>
        private void MessageReceivedContinuation(Task task, Guid streamId)
        {
            // TODO: exception handling
            if (task.Exception != null)
            {
                if (task.Exception.InnerException is FabricObjectClosedException || task.Exception.InnerException is FabricNotPrimaryException)
                {
                    Tracing.WriteExceptionAsWarning("Stream.MessageReceived.Failure", task.Exception, "{0} StreamId: {1}", this.tracer, streamId);
                }
                else
                {
                    Tracing.WriteExceptionAsError("Stream.MessageReceived.Failure", task.Exception, "{0} StreamId: {1}", this.tracer, streamId);
                }
            }
        }

        /// <summary>
        /// handler in lieu of await for StreamManager.InboundStreamRequested
        /// </summary>
        /// <param name="task"></param>
        private void InboundStreamRequestedContinuation(Task task)
        {
            // TODO: exception handling
            if (task.Exception != null)
            {
                if (task.Exception.InnerException is FabricObjectClosedException || task.Exception.InnerException is FabricNotPrimaryException)
                {
                    Tracing.WriteExceptionAsWarning("InboundSessionDriver.InboundStreamRequested.Failure", task.Exception, "{0}", this.tracer);
                }
                else
                {
                    Tracing.WriteExceptionAsError("InboundSessionDriver.InboundStreamRequested.Failure", task.Exception, "{0}", this.tracer);
                }
            }
        }

        /// <summary>
        /// Handler in lieu of await for StreamManager.CompleteOpenStreamProtocol
        /// </summary>
        /// <param name="task"></param>
        private void CompleteOpenStreamProtocolContinuation(Task task)
        {
            // TODO: exception handling
            if (task.Exception != null)
            {
                if (task.Exception.InnerException is FabricObjectClosedException || task.Exception.InnerException is FabricNotPrimaryException)
                {
                    Tracing.WriteExceptionAsWarning("InboundSessionDriver.CompleteOpenStreamProtocol.Failure", task.Exception, "{0}", this.tracer);
                }
                else
                {
                    Tracing.WriteExceptionAsError("InboundSessionDriver.CompleteOpenStreamProtocol.Failure", task.Exception, "{0}", this.tracer);
                }
            }
        }

        /// <summary>
        /// Handler in lieu of await for StreamManager.CompleteCloseStreamProtocol
        /// </summary>
        /// <param name="task"></param>
        private void CompleteCloseStreamProtocolContinuation(Task task)
        {
            // TODO: exception handling
            if (task.Exception != null)
            {
                if (task.Exception.InnerException is FabricObjectClosedException || task.Exception.InnerException is FabricNotPrimaryException)
                {
                    Tracing.WriteExceptionAsWarning("InboundSessionDriver.CompleteCloseStreamProtocol.Failure", task.Exception, "{0}", this.tracer);
                }
                else
                {
                    Tracing.WriteExceptionAsError("InboundSessionDriver.CompleteCloseStreamProtocol.Failure", task.Exception, "{0}", this.tracer);
                }
            }
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
                    Tracing.WriteExceptionAsWarning("InboundSessionDriver.ResetPartnerStreamsAsync.Failure", task.Exception, "{0}", this.tracer);
                }
                else
                {
                    Tracing.WriteExceptionAsError("InboundSessionDriver.ResetPartnerStreamsAsync.Failure", task.Exception, "{0}", this.tracer);
                }
            }
        }

        /// <summary>
        /// Handler in lieu of await for StreamManager.RestartPartnerStreamsAsync
        /// </summary>
        /// <param name="task"></param>
        private void RestartPartnerStreamsAsyncContinuation(Task task)
        {
            // TODO: exception handling
            if (task.Exception != null)
            {
                if (task.Exception.InnerException is FabricObjectClosedException || task.Exception.InnerException is FabricNotPrimaryException)
                {
                    Tracing.WriteExceptionAsWarning("InboundSessionDriver.RestartPartnerStreamsAsync.Failure", task.Exception, "{0}", this.tracer);
                }
                else
                {
                    Tracing.WriteExceptionAsError("InboundSessionDriver.RestartPartnerStreamsAsync.Failure", task.Exception, "{0}", this.tracer);
                }
            }
        }

        /// <summary>
        /// handler in lieu of await for StreamManager.DeleteInboundStreamRequested
        /// </summary>
        /// <param name="task"></param>
        /// <param name="streamId"></param>
        private void DeleteInboundStreamRequestedContinuation(Task task, Guid streamId)
        {
            // TODO: exception handling
            if (task.Exception != null)
            {
                if (task.Exception.InnerException is FabricObjectClosedException || task.Exception.InnerException is FabricNotPrimaryException)
                {
                    Tracing.WriteExceptionAsWarning("Stream.DeleteInboundStreamRequested.Failure", task.Exception, "{0} StreamId: {1}", this.tracer, streamId);
                }
                else
                {
                    Tracing.WriteExceptionAsError("Stream.DeleteInboundStreamRequested.Failure", task.Exception, "{0} StreamId: {1}", this.tracer, streamId);
                }
            }
        }

        /// <summary>
        /// handler in lieu of await for StreamManager.CompleteDeleteStreamProtocol
        /// </summary>
        /// <param name="task"></param>
        /// <param name="streamId"></param>
        private void CompleteDeleteStreamProtocolContinuation(Task task, Guid streamId)
        {
            // TODO: exception handling
            if (task.Exception != null)
            {
                if (task.Exception.InnerException is FabricObjectClosedException || task.Exception.InnerException is FabricNotPrimaryException)
                {
                    Tracing.WriteExceptionAsWarning("Stream.CompleteDeleteStreamProtocol.Failure", task.Exception, "{0} StreamId: {1}", this.tracer, streamId);
                }
                else
                {
                    Tracing.WriteExceptionAsError("Stream.CompleteDeleteStreamProtocol.Failure", task.Exception, "{0} StreamId: {1}", this.tracer, streamId);
                }
            }
        }

        /// <summary>
        /// Handler for messages received on the session, will also deal with exceptions that occur when the session is closed or aborted
        /// </summary>
        /// <param name="receiveTask"></param>
        private void ReceiveContinuation(Task<IOperationData> receiveTask)
        {
            // Check for aborted session and other failures
            if (receiveTask.Exception != null)
            {
                var realException = receiveTask.Exception.InnerException;
                if (realException is InvalidOperationException || realException is OperationCanceledException)
                {
                    FabricEvents.Events.SessionReceiveFailure("SessionAborted@" + this.traceType, this.SessionId.ToString());
                }
                else
                {
                    Tracing.WriteExceptionAsError("InboundSessionDriver.ReceiveAsync.Failure", realException, "{0}", this.tracer);
                    Diagnostics.Assert(false, "{0} InboundSessionDriver.ReceiveAsync Unexpected Exception {1}", this.tracer, receiveTask.Exception.GetType());
                }
            }
            else
            {
                var wireMessage = receiveTask.Result;
                var inboundStreamMessage = new InboundBaseStreamWireMessage(wireMessage);
                var kind = inboundStreamMessage.Kind;

                // Check if Stream Manager is available for updates.
                if (this.streamManager.NotWritable())
                {
                    // drop the message
                    FabricEvents.Events.WireMessageReceived(
                        "DroppedNotWritable@" + this.traceType + "@" + this.SessionId.ToString(),
                        inboundStreamMessage.StreamIdentity.ToString(),
                        inboundStreamMessage.MessageSequenceNumber,
                        kind.ToString());
                    return;
                }

                FabricEvents.Events.WireMessageReceived(
                    this.traceType + "@" + this.SessionId.ToString(),
                    inboundStreamMessage.StreamIdentity.ToString(),
                    inboundStreamMessage.MessageSequenceNumber,
                    kind.ToString());

                /*
                 * There are two basic approaches to dealing with messages here
                 * If the message is intended for a stream and the stream is not found it means we are in the middle of a restore process
                 * so we simply drop the message and build it into the protocol that it will be retried eventually
                 * 
                 * If the message is intended for the stream manager to control restart or to drive stream lifecycle the stream manager 
                 * will ensure it is in operating state and if not has a mechanism to wait for that state
                 */
                switch (kind)
                {
                    case StreamWireProtocolMessageKind.ServiceData:
                        this.streamManager.MessageReceived(inboundStreamMessage)
                            .ContinueWith(antecedent => Task.Run(() => this.MessageReceivedContinuation(antecedent, inboundStreamMessage.StreamIdentity)));
                        break;

                    case StreamWireProtocolMessageKind.SequenceAck:
                        this.streamManager.AckReceived(inboundStreamMessage)
                            .ContinueWith(antecedent => Task.Run(() => this.AckReceivedContinuation(antecedent, inboundStreamMessage.StreamIdentity)));
                        break;

                    case StreamWireProtocolMessageKind.OpenStream:
                        this.streamManager.InboundStreamRequested(this.partnerKey, inboundStreamMessage)
                            .ContinueWith(this.InboundStreamRequestedContinuation);
                        break;

                    case StreamWireProtocolMessageKind.CloseStream:
                        this.streamManager.MessageReceived(inboundStreamMessage)
                            .ContinueWith(antecedent => Task.Run(() => this.MessageReceivedContinuation(antecedent, inboundStreamMessage.StreamIdentity)));
                        break;

                    case StreamWireProtocolMessageKind.OpenStreamResponse:
                        this.streamManager.CompleteOpenStreamProtocol(inboundStreamMessage)
                            .ContinueWith(this.CompleteOpenStreamProtocolContinuation);
                        break;

                    case StreamWireProtocolMessageKind.CloseStreamResponse:
                        this.streamManager.CompleteCloseStreamProtocol(inboundStreamMessage)
                            .ContinueWith(this.CompleteCloseStreamProtocolContinuation);
                        break;

                    case StreamWireProtocolMessageKind.ResetPartnerStreams:
                        // The StreamIdentity slot is actually carrying the Guid representing the era of the requester
                        this.streamManager.ResetPartnerStreamsAsync(inboundStreamMessage.StreamIdentity, this.partnerKey)
                            .ContinueWith(this.ResetPartnerStreamsAsyncContinuation);
                        break;

                    case StreamWireProtocolMessageKind.ResetPartnerStreamsResponse:
                        var resetResponseValue = new InboundResetPartnerStreamsResponseValue(inboundStreamMessage);
                        Diagnostics.Assert(
                            resetResponseValue.Response == StreamWireProtocolResponse.ResetPartnerStreamsCompleted,
                            "{0} Unexpected reset partner streams response",
                            this.tracer);

                        // The StreamIdentity slot is actually carrying the Guid representing the era when the ResetPartnerStreams request was sent
                        // We will only process the respone if the request was sent in the current era
                        if (inboundStreamMessage.StreamIdentity == this.streamManager.Era)
                        {
                            this.streamManager.RestartPartnerStreamsAsync(this.partnerKey, this.streamManager.RuntimeResources.OutboundStreams, false)
                                .ContinueWith(this.RestartPartnerStreamsAsyncContinuation);
                        }
                        break;

                    case StreamWireProtocolMessageKind.DeleteStream:
                        this.streamManager.DeleteInboundStreamRequested(this.partnerKey, inboundStreamMessage)
                            .ContinueWith(
                                antecedent => Task.Run(() => this.DeleteInboundStreamRequestedContinuation(antecedent, inboundStreamMessage.StreamIdentity)));
                        break;

                    case StreamWireProtocolMessageKind.DeleteStreamResponse:
                        this.streamManager.CompleteDeleteStreamProtocol(inboundStreamMessage)
                            .ContinueWith(
                                antecedent => Task.Run(() => this.CompleteDeleteStreamProtocolContinuation(antecedent, inboundStreamMessage.StreamIdentity)));
                        break;

                    default:
                        Diagnostics.Assert(
                            false,
                            "{0} Unknown message kind in InboundSessionDriver Kind-as-int: {1}",
                            this.tracer,
                            (int) inboundStreamMessage.Kind);
                        break;
                }

                // Restart the receive operation
                Diagnostics.Assert(this.inboundSession != null, "InboundSessionDriver.ReceiveContinuation.RestartReceive found null session");

                this.ReceiveOnSession();
            }
        }

        #endregion
    }
}
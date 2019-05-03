// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    #region using directives

    using System.Collections.Concurrent;
    using System.Fabric.ReliableMessaging;
    using System.Fabric.ReliableMessaging.Session;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;

    #endregion

    /// <summary>
    /// Outbound Session Driver to handle outbound messages.
    /// </summary>
    internal class OutboundSessionDriver : IDisposable
    {
        // TODO: track when this is disposed for asserts
        /// <summary>
        /// Outbound message queue, enque all messages here to be send out messages in a FIFO fashion using the outbound session.
        /// </summary>
        private readonly ConcurrentQueue<IOperationData> sessionQueue;

        /// <summary>
        /// Use this session(there is one for every unique target Partition), to send out all message in sessionQueue.
        /// </summary>
        private readonly IReliableMessagingSession outboundSession;

        /// <summary>
        /// The associated stream manager, where messages are produced and queued to the sessionQueue.
        /// </summary>
        private readonly StreamManager streamManager;

        /// <summary>
        /// The primary Session Connection Manager, used by the outbound session driver.
        /// </summary>
        private readonly SessionConnectionManager sessionConnectionManager;

        /// <summary>
        /// The partner target partition, there is a single outbound session per partition (see active resources for ref.).
        /// </summary>
        private readonly PartitionKey targetPartition;

        /// <summary>
        /// Used locally to provide a barrier, during sessionQueue operations.
        /// </summary>
        private int drainQueueBarrier;

        /// <summary>
        /// Tracing Info
        /// </summary>
        private readonly string tracer;

        /// <summary>
        /// SessionId for tracing
        /// </summary>
        internal Guid SessionId { get; private set; }

        #region cstor

        /// <summary>
        /// Constructor to create an instance of the OutboundSessionDriver for a given Partition.
        /// </summary>
        /// <param name="targetPartition">Session for the Target Partition</param>
        /// <param name="streamManager">Steam Manager where messages are sent to the session queue</param>
        /// <param name="sessionConnectionManager">Session Connection manager, which creates and actively manages the session</param>
        /// <param name="outboundSession">The session we will use to send out messages</param>
        internal OutboundSessionDriver(
            PartitionKey targetPartition, StreamManager streamManager, SessionConnectionManager sessionConnectionManager,
            IReliableMessagingSession outboundSession)
        {
            // Assign relevent association to targetPartitions, Stream manager, and connection manager.
            this.targetPartition = targetPartition;
            this.streamManager = streamManager;
            this.sessionConnectionManager = sessionConnectionManager;
            // Instantiate the message queue.
            this.sessionQueue = new ConcurrentQueue<IOperationData>();
            // The targetPartition Session
            this.outboundSession = outboundSession;
            // The appropriate Session ID for tracing
            var snapshot = outboundSession.GetDataSnapshot();
            this.SessionId = snapshot.SessionId;
            // Set default values.
            this.drainQueueBarrier = 0;
            this.tracer = streamManager.Tracer + " Target: " + this.targetPartition;
        }

        #endregion

        /// <summary>
        /// Free the associated outBoundSession in session connection manager during dispose.
        /// </summary>
        public void Dispose()
        {
            this.sessionConnectionManager.ClearOutboundSession(this.targetPartition);
        }

        /// <summary>
        /// Dispose OutboundSessionDriver references and the actual dispose of the OutboundSessionDriver in a safe fashion.
        /// This is used in reset partner stream operation, when the OutBoundSessionDriver is disposed and recreated.
        /// </summary>
        /// <param name="targetPartition">OutBoundSessionDriver for this Partition</param>
        /// <param name="streamManager">Reset corresponding OutBoundSessionDriver ref. in stream manager</param>
        /// <returns></returns>
        internal static async Task ClearDriverAsync(PartitionKey targetPartition, StreamManager streamManager)
        {
            FabricEvents.Events.ClearOutboundSessionDriver("Start@" + streamManager.TraceType, targetPartition.ToString());

            // this key must already have been resolved and normalized when the driver was created
            var normalizedPartitionKey = streamManager.SessionConnectionManager.GetNormalizedPartitionKey(targetPartition);
            Diagnostics.Assert(null != normalizedPartitionKey, "normalizedPartitionKey is not expected to be null in ClearDriverAsync().");

            // SyncPoint(lock)  
            using (
                var syncPoint = new SyncPoint<PartitionKey>(
                    streamManager,
                    normalizedPartitionKey,
                    streamManager.RuntimeResources.OutboundSessionDriverSyncpoints))
            {
                // this is entering an await compatible critical section -- syncPoint.Dispose will leave it
                await syncPoint.EnterAsync(Timeout.InfiniteTimeSpan);

                // remove appropriate ref. 
                OutboundSessionDriver driver = null;
                var removeSuccess = streamManager.RuntimeResources.OutboundSessionDrivers.TryRemove(normalizedPartitionKey, out driver);

                // dispose the driver session
                if (removeSuccess)
                {
                    driver.Dispose();
                }
            }

            FabricEvents.Events.ClearOutboundSessionDriver("Finish@" + streamManager.TraceType, targetPartition.ToString());
        }

        /// <summary>
        /// Called to find(or create if one is not found) the appropriate OutboundSessionDriver for a given partition.
        /// This method will ensure that we have a one(targetPartition)-to-one(outBoundSessionDriver) relationship.
        /// 
        /// TODO: this layer must catch all exceptions and decide what to do with them -- lower layer functions like ResolveAndNormalizeTargetPartition
        /// will throw in case of unrecoverable errors like FabricObjectClosedException
        /// </summary>
        /// <param name="targetPartition">Find or Create OutBoundSessionDriver for this Partition</param>
        /// <param name="streamManager">Stream manager that manages the reference to the driver</param>
        /// <param name="sessionConnectionManager">Use this session connection manager to create a new session if one if not found</param>
        /// <param name="timeout">Timeout</param>
        /// <returns>Returns OutboundSessionDriver if successfull, null otherwise</returns>
        internal static async Task<OutboundSessionDriver> FindOrCreateDriverAsync(
            PartitionKey targetPartition,
            StreamManager streamManager,
            SessionConnectionManager sessionConnectionManager,
            TimeSpan timeout)
        {
            OutboundSessionDriver streamOutboundSessionDriver = null;
            // Complete the call within this time.
            var remainingTimeout = timeout;

            try
            {
                FabricEvents.Events.FindOrCreateOutboundSessionDriver("Start@" + streamManager.TraceType, targetPartition.ToString());
                var beforeResolve = DateTime.UtcNow;

                // The sync point pattern is used here because lock() pattern is not compatible with await pattern
                var normalizedPartitionKey = await streamManager.SessionConnectionManager.ResolveAndNormalizeTargetPartition(targetPartition, remainingTimeout);
                Diagnostics.Assert(
                    null != normalizedPartitionKey,
                    "normalizedPartitionKey is not expected to be null in OutboundSessionDriver.FindOrCreateDriverAsync().");

                // Update remaining time for timeout
                var afterResolve = DateTime.UtcNow;
                remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, beforeResolve, afterResolve);

                var syncPoint
                    = new SyncPoint<PartitionKey>(
                        streamManager,
                        normalizedPartitionKey,
                        streamManager.RuntimeResources.OutboundSessionDriverSyncpoints);

                try
                {
                    // this is entering an await compatible critical section -- syncPoint.Dispose will leave it
                    await syncPoint.EnterAsync(remainingTimeout);

                    var afterEnter = DateTime.UtcNow;
                    remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, afterResolve, afterEnter);

                    // Check if the driver already exists 
                    var exists = streamManager.RuntimeResources.OutboundSessionDrivers.TryGetValue(normalizedPartitionKey, out streamOutboundSessionDriver);

                    // TODO: should we refcount how many streams depend on this session?

                    // create a new driver if if it does not exist already
                    if (!exists)
                    {
                        FabricEvents.Events.FindOrCreateOutboundSessionDriver("Creating@" + streamManager.TraceType, normalizedPartitionKey.ToString());

                        // we are actually the first stream attaching to this partition on start or recovery or all previous ones closed

                        // Find or create a new reliable messaging session 
                        var session = await sessionConnectionManager.FindOrCreateOutboundSessionAsync(normalizedPartitionKey, remainingTimeout);
                        Diagnostics.Assert(null != session, "Session is not expected to be null in OutboundSessionDriver.FindOrCreateDriverAsync().");

                        // Create a new outbound session driver
                        streamOutboundSessionDriver = new OutboundSessionDriver(normalizedPartitionKey, streamManager, sessionConnectionManager, session);
                        Diagnostics.Assert(
                            null != streamOutboundSessionDriver,
                            "Stream OutboundSessionDriver is not expected to be null in OutboundSessionDriver.FindOrCreateDriverAsync().");

                        // Add ref. to stream manager active runtime resources
                        var addSuccess = streamManager.RuntimeResources.OutboundSessionDrivers.TryAdd(normalizedPartitionKey, streamOutboundSessionDriver);
                        Diagnostics.Assert(
                            addSuccess,
                            "{0} Unexpected failure to add newSessionDriver in OutboundSessionDriver.FindOrCreateDriverAsync",
                            streamManager.Tracer);
                    }
                    else
                    {
                        FabricEvents.Events.FindOrCreateOutboundSessionDriver("Found@" + streamManager.TraceType, normalizedPartitionKey.ToString());
                    }
                }
                finally
                {
                    syncPoint.Dispose();
                }
                FabricEvents.Events.FindOrCreateOutboundSessionDriver("Finish@" + streamManager.TraceType, normalizedPartitionKey.ToString());
            }
            catch (TimeoutException)
            {
                FabricEvents.Events.FindOrCreateOutboundSessionDriver("Timeout@" + streamManager.TraceType, targetPartition.ToString());
                throw;
            }
            catch (Exception e)
            {
                Tracing.WriteExceptionAsWarning("FindOrCreateDriverAsync.Failure", e, "{0}", streamManager.Tracer);
                throw;
            }

            return streamOutboundSessionDriver;
        }

        /// <summary>
        /// Handler for session send() operation.
        /// Process various exception use cases here.
        /// </summary>
        /// <param name="sendTask">Session Continuation task</param>
        internal void SendContinuation(Task sendTask)
        {
            if (sendTask.Exception != null)
            {
                var realException = sendTask.Exception.InnerException;

                if (realException is InvalidOperationException || realException is OperationCanceledException)
                {
                    FabricEvents.Events.SessionSendFailure("SessionAborted@" + this.streamManager.TraceType, this.SessionId.ToString());

                    // the driver was Disposed usually due to partner reset, this driver is no longer valid: confirm that
                    var snapshot = this.outboundSession.GetDataSnapshot();
                    Diagnostics.Assert(
                        !snapshot.IsOpenForSend,
                        "{0} InvalidOperationException when outboundSession is open for Send in SendContinuation",
                        this.tracer);
                }
                else
                {
                    Tracing.WriteExceptionAsError(
                        "SessionSend.Failure",
                        realException,
                        "{0} OutboundSessionDriver.SendContinuation encountered unexpected exception",
                        this.tracer);
                    Diagnostics.Assert(
                        false,
                        "{0} Unexpected exception {1} in SendContinuation",
                        this.tracer,
                        realException.GetType());
                }
            }
        }

        /// <summary>
        /// Send messages from the session (message) queue in a FIFO fashion.
        /// </summary>
        /// <returns></returns>
        internal bool TryDrainSessionSendQueue()
        {
            bool barrierOpen;
            var messageExists = true;
            var sessionQuotaExceeded = false;
            var driverDisposed = false;

            // if we got through the barrier and exited because we found no message but the sessionQueue is not empty then there was a race that
            // a Send operation lost so we circle back and try sending the message that the Send operation would have tried to send
            do
            {
                // low impact lock - while sending a message to ensure we process sends synchronously in FIFO
                var value = Interlocked.CompareExchange(ref this.drainQueueBarrier, 1, 0);
                barrierOpen = value == 0;

                if (barrierOpen)
                {
                    try
                    {
                        while (!sessionQuotaExceeded && !driverDisposed && messageExists)
                        {
                            IOperationData nextMessage = null;
                            messageExists = this.sessionQueue.TryPeek(out nextMessage);

                            // IMPORTANT: This try-catch code relies on the fact that the COM-interop layer SYNCHRONOUSLY THROWS A FAULT from the BeginFunc OF SendAsync
                            // TODO: find a way to push handling of this transient fault down into the managed wrapper for sessions
                            // TODO: what happens if there is a session Abort? Or a failure of the partner? 
                            if (messageExists)
                            {
                                try
                                {
                                    // drop the Task returned since the only possible exceptions in the task results reflect session abort
                                    // we will get a notification for session abort through the registered interface and deal with it there
                                    this.outboundSession.SendAsync(nextMessage, CancellationToken.None).ContinueWith(this.SendContinuation);
                                }
                                catch (FabricException fabricException)
                                {
                                    var errorCode = fabricException.ErrorCode;

                                    if (errorCode == FabricErrorCode.ReliableSessionQuotaExceeded)
                                    {
                                        FabricEvents.Events.SessionSendFailure("QuotaExceeded@" + this.streamManager.TraceType, this.SessionId.ToString());
                                        sessionQuotaExceeded = true;
                                    }
                                }
                                catch (Exception e)
                                {
                                    if (e is InvalidOperationException || e is OperationCanceledException)
                                    {
                                        FabricEvents.Events.SessionSendFailure("SessionAborted@" + this.streamManager.TraceType, this.SessionId.ToString());

                                        // the driver was Disposed usually due to partner reset, this driver is no longer valid: confirm that
                                        var snapshot = this.outboundSession.GetDataSnapshot();
                                        Diagnostics.Assert(
                                            !snapshot.IsOpenForSend,
                                            "{0} InvalidOperationException when outboundSession is open for Send in TryDrainSessionSendQueue",
                                            this.tracer);
                                        driverDisposed = true;
                                    }
                                    else
                                    {
                                        Tracing.WriteExceptionAsError(
                                            "SendAsync.Failure",
                                            e,
                                            "{0} OutboundSessionDriver.SendContinuation encountered unexpected exception",
                                            this.tracer);
                                        Diagnostics.Assert(
                                            false,
                                            "{0} Unexpected exception {1} in OutboundSessionDriver.SendContinuation",
                                            this.tracer,
                                            e.GetType());
                                    }
                                }
                                // dequeue the message only if the message was successfully handed out to the session for send proc.
                                if (!sessionQuotaExceeded && !driverDisposed)
                                {
                                    var popSuccess = this.sessionQueue.TryDequeue(out nextMessage);
                                    Diagnostics.Assert(
                                        popSuccess,
                                        "{0} Unexpected missing message in OutboundSessionDriver.TryDrainSessionSendQueue",
                                        this.tracer);
                                }
                            }
                        }
                    }
                    finally
                    {
                        value = Interlocked.CompareExchange(ref this.drainQueueBarrier, 0, 1);
                        Diagnostics.Assert(
                            value == 1,
                            "drainQueueBarrier was reset to 0 while inside critical section in OutboundSessionDriver.TryDrainSessionSendQueue");
                    }
                }
            } // Finally check if messages got added to the queue while we were busy processing the earlier message.
            while (barrierOpen && !driverDisposed && !messageExists && this.sessionQueue.Count > 0);

            return sessionQuotaExceeded;
        }

        /// <summary>
        /// Quota exceeded retry method..
        /// </summary>
        /// <returns></returns>
        internal async Task QuotaExceededRetry()
        {
            var sessionQuotaExceeded = true;
            var backOffDelay = StreamConstants.BaseDelayForQuotaExceededRetry;

            while (sessionQuotaExceeded)
            {
                await Task.Delay(backOffDelay);
                FabricEvents.Events.SessionSendFailure("QuotaExceeded.Retry@" + this.streamManager.TraceType, this.SessionId.ToString());
                sessionQuotaExceeded = this.TryDrainSessionSendQueue();
                backOffDelay = (backOffDelay*2) >= StreamConstants.MaxDelayForQuotaExceededRetry
                    ? StreamConstants.MaxDelayForQuotaExceededRetry
                    : (backOffDelay*2);
            }
        }

        /// <summary>
        /// Retry continuation.
        /// </summary>
        /// <param name="task"></param>
        internal void QuotaExceededRetryContinuation(Task task)
        {
            if (task.Exception != null)
            {
                Tracing.WriteExceptionAsError("OutboundSessionDriver.QuotaExceededRetry.Failure", task.Exception, "{0}", this.tracer);
            }
        }

        /// <summary>
        /// Send message with retry functionality.
        /// </summary>
        /// <param name="message">message to be sent.</param>
        internal void Send(IOperationData message)
        {
            // TODO: do we need to worry about OutOfMemory exception?
            this.sessionQueue.Enqueue(message);
            var sessionQuotaExceeded = this.TryDrainSessionSendQueue();

            if (sessionQuotaExceeded)
            {
                Action retryAction = (() => this.QuotaExceededRetry().ContinueWith(this.QuotaExceededRetryContinuation));
                Task.Run(retryAction);
            }
        }
    }
}
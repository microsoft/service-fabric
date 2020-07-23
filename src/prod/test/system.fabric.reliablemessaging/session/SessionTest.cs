// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ReliableSession.Test
{
    using System.Fabric.Interop;
    using System.Fabric.ReliableMessaging.Session;
    using System.Fabric.Test;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    /// <summary>
    /// Class to encapsulate the core session test features (of creating, sending, receiving and verifying messages) while introducing
    /// some real world session abort behaviors.  
    /// </summary>
    internal class SessionTest
    {
        // ctor parameters
        int segmentSize;
        int messageCount;
        int sendWaitPerMessage;

        public int SessionId
        {
            get; 
            private set;
        }

        // internal accumulators
        long totalBytesReceived;
        long totalBytesSent;
        byte[][] flatMessagesSent;
        byte[][][] actualMessagesSent;
        byte[][] flatMessagesReceived;
        byte[][][] actualMessagesReceived;

        // Session parameters and current state
        public SessionDataSnapshot SnapshotInbound;
        public SessionDataSnapshot SnapshotOutbound;

        // externally set properties
        public bool SkipTest;
        public IReliableMessagingSession InboundSession;
        public IReliableMessagingSession OutboundSession;

        /// <summary>
        /// Initialize session test with payload definition (segment size and messages count for a given session).
        /// </summary>
        /// <param name="segSize">Segment size</param>
        /// <param name="messageCount">Message count</param>
        /// <param name="sendWaitInMilliseconds">Wait time between messages send operations</param>
        /// <param name="sessionNumber">For the specified session number, used for tracking purposes.</param>
        public SessionTest(int segSize, int messageCount, int sendWaitInMilliseconds, int sessionNumber)
        {
            this.SkipTest = false;
            this.InboundSession = null;
            this.OutboundSession = null;
            this.totalBytesReceived = 0;
            this.totalBytesSent = 0;
            this.messageCount = messageCount;
            this.segmentSize = segSize;
            this.SessionId = sessionNumber;
            this.sendWaitPerMessage = sendWaitInMilliseconds;
            this.flatMessagesSent = new byte[this.messageCount][];
            this.flatMessagesReceived = new byte[this.messageCount][];
            this.actualMessagesSent = new byte[this.messageCount][][];
            this.actualMessagesReceived = new byte[this.messageCount][][];
        }

        /// <summary>
        /// Run the test (send and receive messages) with sessions abort activity. 
        /// </summary>
        /// <param name="delay"></param>
        public void RunTest(int delay)
        {
            LogHelper.Log("SessionId: {0}; Entering RunTest", this.SessionId);

            Assert.IsNotNull(this.InboundSession, "Null inbound session in RunTest for SessionId#{0}", this.SessionId);
            Assert.IsNotNull(this.OutboundSession, "Null outbound session in RunTest for SessionId#{0}", this.SessionId);

            Thread.Sleep(delay);

            Random myRandGen = new Random(unchecked((int)DateTime.Now.Ticks));

            DateTime start = DateTime.Now;

            LogHelper.Log("SessionId: {0}; Started; MessageCount: {1}", this.SessionId, this.messageCount);

            // Send Action Delegate - Prepare and send the messages
            Action sendAction = (() => this.SendAndRecordMessages(myRandGen));
            // Receive Action Delegate - receive and record the messages
            Action receiveAction = (() => this.ReceiveAndRecordMessages());

            // Start the send and receive actions
            Task sendTask = Task.Factory.StartNew(sendAction);
            Task receiveTask = Task.Factory.StartNew(receiveAction);

            // While the tests are running, randomly abort the sessions.
            bool abortOccurred = this.RunRandomSessionAbortTest(myRandGen);

            // Wait for the send and receive tasks to complete
            bool sendCompleted = sendTask.Wait(Int32.MaxValue);
            bool receiveCompleted = receiveTask.Wait(Int32.MaxValue);

            Assert.IsTrue(sendCompleted, "sendCOmpleted false in RunTest for SessionId: {0} MessageCount: {1} Aborted: {2}", this.SnapshotOutbound.SessionId, this.messageCount, abortOccurred);
            Assert.IsTrue(receiveCompleted, "receiveCompleted false in RunTest for SessionId: {0} MessageCount: {1} Aborted: {2}", this.SnapshotInbound.SessionId, this.messageCount, abortOccurred);

            this.SnapshotInbound = this.InboundSession.GetDataSnapshot();
            this.SnapshotOutbound = this.OutboundSession.GetDataSnapshot();

            DateTime finish = DateTime.Now;

            if (!abortOccurred)
            {

                Assert.AreEqual(this.totalBytesSent, this.totalBytesReceived,
                    "totalBytesSent != totalBytesReceived in RunTest for SessionId#{0}", this.SessionId);

                for (int i = 0; i < this.messageCount; i++)
                {
                    Assert.IsNotNull(this.flatMessagesSent[i], "flatMessagesSent[{0}] is NULL in RunTest for SessionId#{1}", i, this.SessionId);
                    Assert.IsNotNull(this.flatMessagesReceived[i], "flatMessagesReceived[{0}] is NULL in RunTest for SessionId#{1}", i, this.SessionId);
                   
                    // message content is the same: sent and received
                    bool checkContent = Enumerable.SequenceEqual(this.flatMessagesReceived[i], this.flatMessagesSent[i]);

                    //message structure is the same: sent and received
                    int[] sentSizes = this.actualMessagesSent[i].Select(elem => elem.Length).ToArray();
                    int[] receivedSizes = this.actualMessagesReceived[i].Select(elem => elem.Length).ToArray();
                    bool checkStructure = Enumerable.SequenceEqual(sentSizes, receivedSizes);
                    
                    Assert.IsTrue(checkContent, "flatMessagesReceived[{0}] != flatMessagesSent[{0}] in RunTest for SessionId#{1}", i, this.SessionId);
                    Assert.IsTrue(checkStructure, "receivedSizes[{0}] != sentSizes[{0}] in RunTest for SessionId#{1}", i, this.SessionId);
                }

                LogHelper.Log("SessionId: {0}; Completed; MessageCount: {1}; Avg Size: {2}; Duration {3} MilliSeconds", this.SessionId, this.messageCount, this.totalBytesSent / this.messageCount, (finish.Ticks - start.Ticks) / TimeSpan.TicksPerMillisecond);
            }
            else
            {
                LogHelper.Log("SessionId: {0}; Aborted; MessageCount: {1}; Avg Size: {2}; Duration {3} MilliSeconds", this.SessionId, this.messageCount, this.totalBytesSent / this.messageCount, (finish.Ticks - start.Ticks) / TimeSpan.TicksPerMillisecond);
            }


            LogHelper.Log("SessionId: {0}; Exiting RunTest", this.SessionId);
        }

        /// <summary>
        /// Randomly abort sessions(Inbound or outbound) for this. session test instance
        /// </summary>
        /// <param name="localRandGen">Random Gen</param>
        /// <returns>true, One/both of the sessions are aborted.</returns>
        bool RunRandomSessionAbortTest(Random localRandGen)
        {
            Assert.IsNotNull(this.InboundSession, "Null inbound session in RunAbortTest");
            Assert.IsNotNull(this.OutboundSession, "Null outbound session in RunAbortTest");

            bool inboundAbortOccurred = false;
            bool outboundAbortOccurred = false;


            LogHelper.Log("SessionId: {0}; Entering RunRandomSessionAbortTest", this.SessionId);

            // Random decisions about whether to abort, what to abort and in what order
            int nextRand = localRandGen.Next(10, 500);
            bool abortInbound = nextRand % 3 == 0;
            bool abortOutbound = nextRand % 5 == 0;
            int additionalDelay = nextRand % 11;
            bool abortInboundFirst = additionalDelay % 2 == 0;

            if (abortInbound || abortOutbound)
            {
                // we are going to abort one of the sessions, go ahead and do initial delay
                Thread.Sleep(nextRand);
            }

            // if we want to abort both the sessions
            if (abortInbound && abortOutbound)
            {
                if (abortInboundFirst)
                {
                    inboundAbortOccurred = this.AbortSession(this.InboundSession);
                    Thread.Sleep(additionalDelay);
                    outboundAbortOccurred = this.AbortSession(this.OutboundSession);
                }
                else
                {
                    outboundAbortOccurred = this.AbortSession(this.OutboundSession);
                    Thread.Sleep(additionalDelay);
                    inboundAbortOccurred = this.AbortSession(this.InboundSession);
                }
            }
            else if (abortOutbound)
            {
                outboundAbortOccurred = this.AbortSession(this.OutboundSession);
            }
            else if (abortInbound)
            {
                inboundAbortOccurred = this.AbortSession(this.InboundSession);
            }

            LogHelper.Log("SessionId: {0}; Exiting RunRandomSessionAbortTest", this.SessionId);

            return inboundAbortOccurred || outboundAbortOccurred;
        }

        /// <summary>
        /// Abort the given session.
        /// </summary>
        /// <param name="session">Session to Abort</param>
        /// <returns>true if session aborted, false otherwise</returns>
        bool AbortSession(IReliableMessagingSession session)
        {
            bool abortOccurred = true;

            try
            {
                LogHelper.Log("SessionId: {0}; Entering AbortSession", this.SessionId);

                session.Abort();
            }
            catch (Exception e)
            {
                // this may occur because either the session closed before abort or because the abort at the 
                // opposite end caused a message driven abort before the API could be invoked
                Assert.AreEqual(e.GetType(), typeof(InvalidOperationException), "Unexpected exception in AbortSession");
                abortOccurred = false;
            }

            LogHelper.Log("SessionId: {0}; Exiting AbortSession", this.SessionId);
            return abortOccurred;
        }

        /// <summary>
        /// Send and record the message info.
        /// </summary>
        /// <param name="localRandGen"></param>
        /// <returns></returns>
        private bool SendAndRecordMessages(Random localRandGen)
        {
            int numberOfFullSegments = this.messageCount / this.segmentSize;
            int numberOfMessagesSent = 0;

            bool outboundSessionAborted = false;

            LogHelper.Log("SessionId: {0}; Entering SendAndRecordMessages", this.SessionId);

            for (int i = 0; i < numberOfFullSegments && !outboundSessionAborted; i++)
            {
                int lowMark = numberOfMessagesSent;
                int highMark = lowMark + this.segmentSize - 1;
                outboundSessionAborted = this.SendAndRecordMessageRange(lowMark, highMark, this.sendWaitPerMessage, localRandGen);

                if (!outboundSessionAborted)
                {
                    numberOfMessagesSent += this.segmentSize;
                }
            }

            if (numberOfMessagesSent < this.messageCount && !outboundSessionAborted)
            {
                int lowMark = numberOfMessagesSent;
                int highMark = this.messageCount - 1;
                outboundSessionAborted = this.SendAndRecordMessageRange(lowMark, highMark, this.sendWaitPerMessage, localRandGen);
            }

            if (!outboundSessionAborted)
            {
                try
                {
                    Task closeTask = this.OutboundSession.CloseAsync(new CancellationToken());
                    closeTask.Wait();
                }
                catch (AggregateException e)
                {
                    Exception inner = e.InnerException.InnerException;
                    Assert.AreEqual(inner.GetType(), typeof(COMException),
                        "Unexpected AggregateException {0} from Close", inner.GetType().ToString());
                    COMException realException = (COMException)inner;
                    uint hresult = (uint)realException.ErrorCode;
                    uint expectedError = (uint)NativeTypes.FABRIC_ERROR_CODE.E_ABORT;
                    Assert.AreEqual(expectedError, hresult, "Unexpected error HRESULT code {0} from session Close", hresult);
                }
                catch (Exception e)
                {
                    Type exceptionType = e.GetType();
                    Assert.IsTrue(exceptionType == typeof(InvalidOperationException) || exceptionType == typeof(OperationCanceledException),
                        "Unexpected standard exception {0} from Close", e.GetType());
                    outboundSessionAborted = true;
                }
            }

            LogHelper.Log("SessionId: {0}; Exiting SendAndRecordMessages", this.SessionId);

            return outboundSessionAborted;
        }

        /// <summary>
        /// Send and record the Message range.
        /// </summary>
        /// <param name="low"></param>
        /// <param name="high"></param>
        /// <param name="waitInMilliSecondsPerMessage"></param>
        /// <param name="localRandGen"></param>
        /// <returns></returns>
        bool SendAndRecordMessageRange(int low, int high, int waitInMilliSecondsPerMessage, Random localRandGen)
        {
            LogHelper.Log("SessionId: {0}; Starting sendAndRecordMessageRange; Low: {1} High: {2} Wait/Message:{3} ", this.SessionId, low, high, waitInMilliSecondsPerMessage);

            Task[] sendTasks = new Task[high - low + 1];

            DateTime sendStart = DateTime.Now;

            bool sessionAborted = false;

            for (int i = low; i <= high && !sessionAborted; i++)
            {
                byte[][] dataSent = ConstructMessage(i, localRandGen);

                Assert.IsNull(this.flatMessagesSent[i], "flatMessagesSent[{0}] non-null while being initialized", i);
                this.flatMessagesSent[i] = Flatten(dataSent);
                this.actualMessagesSent[i] = dataSent;
                int messageLength = this.flatMessagesSent[i].Length;

                OperationData messageSent = new OperationData(dataSent);

                bool sendSuccess = false;
                int retryCount = 0;

                while (!sendSuccess && !sessionAborted)
                {
                    try
                    {
                        sendTasks[i - low] = this.OutboundSession.SendAsync(messageSent, new CancellationToken());
                        sendSuccess = true;
                    }
                    catch (Exception e)
                    {
                        if (e.GetType() == typeof(InvalidOperationException))
                        {
                            sessionAborted = true;
                        }
                        else
                        {
                            // TODO: this will now be OufOfQuota exception
                            Assert.AreEqual(e.GetType(), typeof(OutOfMemoryException),
                                "Unexpected exception {0} from SendAsync", e.GetType().ToString());

                            LogHelper.Log("SessionId: {0}; Retry send count {1} for message number {2}", this.SessionId, retryCount++, i);
                            Thread.Sleep(100);
                        }
                    }
                }

                if (!sessionAborted)
                {
                    LogHelper.Log("SessionId: {0}; Sent message# {1} size {2}", this.SessionId, i, messageLength);
                    this.totalBytesSent += messageLength;
                }
            }

            // if the number of messages is very small (1 or 2) the wait turns out to be too small so set a floor of 5000 milliseconds on the wait
            bool sendCompleted = false;

            long waitInMilliseconds = 0;

            if (!sessionAborted)
            {
                try
                {
                    DateTime waitStart = DateTime.Now;
                    sendCompleted = Task.WaitAll(sendTasks, Int32.MaxValue);
                    DateTime waitFinish = DateTime.Now;
                    waitInMilliseconds = (waitFinish.Ticks - waitStart.Ticks) / TimeSpan.TicksPerMillisecond;
                }
                catch (AggregateException e)
                {
                    for (int j = 0; j < e.InnerExceptions.Count; j++)
                    {
                        Exception inner = e.InnerExceptions[j].InnerException;
                        Assert.AreEqual(inner.GetType(), typeof(COMException),
                            "Unexpected AggregateException {0} from Send Completed Wait", inner.GetType().ToString());
                        COMException realException = (COMException)inner;
                        uint hresult = (uint)realException.ErrorCode;
                        // this is the simple case where the send buffer is flushed on abort
                        uint expectedError1 = (uint)NativeTypes.FABRIC_ERROR_CODE.E_ABORT;
                        // this is the race condition where abort occurs during execution of the Start of the AsyncSendOperation 
                        uint expectedError2 = (uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_OPERATION;
                        Assert.IsTrue(expectedError1 == hresult || expectedError2 == hresult,
                            "Unexpected error HRESULT code {0} from Send Completed Wait", hresult);
                        sessionAborted = true;
                    }
                }
                catch (Exception e)
                {
                    //VS.Assert.AreEqual(e.GetType(), typeof(System.OperationCanceledException),
                    Assert.Fail("Unexpected standard exception {0} from Send Completed Wait", e.GetType());
                    sessionAborted = true;
                }

                if (sendCompleted)
                {
                    Assert.IsFalse(sessionAborted,
                        "sessionAborted and sendCompleted both true in sendAndRecordMessageRange; SessionID#{0}, Low: {1} High: {2}",
                        this.SessionId, low, high);
                    DateTime sendFinish = DateTime.Now;
                    LogHelper.Log("SessionId: {0}; Intermediate Stats; {1} Messages Sent in {2} Milliseconds", this.SessionId, high - low + 1, (sendFinish.Ticks - sendStart.Ticks) / TimeSpan.TicksPerMillisecond);
                }
                else
                {
                    LogHelper.Log("SessionId: {0}; Failed to complete message range send in time Low: {1} High: {2} Wait/Message: ", this.SessionId, low, high, waitInMilliSecondsPerMessage);
                }
            }

            Assert.IsTrue(sendCompleted || sessionAborted,
                "Unexpected finish state for sendAndRecordMessageRange; SessionID: {0}, Low: {1} High: {2}, waited for {3} Milliseconds; SessionAborted: {4}",
                this.SnapshotOutbound.SessionId, low, high, waitInMilliseconds, sessionAborted);

            LogHelper.Log("SessionId: {0}; Exiting SendAndRecordMessageRange", this.SessionId);

            return sessionAborted;
        }

        /// <summary>
        /// Receive and record the message (used for verification)
        /// </summary>
        /// <returns></returns>
        private bool ReceiveAndRecordMessages()
        {
            bool sessionClosed = false;
            bool sessionAborted = false;

            int messageSequence = 0;
            int messageNumber;

            LogHelper.Log("SessionId: {0}; Entering ReceiveAndRecordMessages", this.SessionId);

            while (!sessionClosed && !sessionAborted)
            {
                try
                {
                    byte[][] dataReceived;

                    Task<IOperationData> receiveTask = this.InboundSession.ReceiveAsync(new CancellationToken());
                    receiveTask.Wait();
                    IOperationData receivedMessageData = receiveTask.Result;
                    dataReceived = receivedMessageData.Select(element => element.Array).ToArray();

                    byte[] flatReceivedMessage = Flatten(dataReceived);
                    int[] parsedReceivedMessage = ParseMessage(flatReceivedMessage);

                    messageNumber = parsedReceivedMessage[0];

                    // ordered delivery check
                    Assert.AreEqual(messageNumber, messageSequence, "Message# {0} received out of order", messageNumber);

                    Assert.IsNull(this.flatMessagesReceived[messageNumber],
                        "flatMessagesReceived[{0}] non-null while being initialized", messageNumber);
                    this.flatMessagesReceived[messageNumber] = flatReceivedMessage;
                    this.actualMessagesReceived[messageNumber] = dataReceived;

                    int messageLength = this.flatMessagesReceived[messageNumber].Length;
                    this.totalBytesReceived += messageLength;

                    LogHelper.Log("SessionId: {0}; received message# {1} size {2}", this.SessionId, messageNumber, messageLength);
                }
                catch (AggregateException e)
                {
                    Exception inner = e.InnerException.InnerException;
                    Assert.AreEqual(inner.GetType(), typeof(COMException),
                        "Unexpected AggregateException {0} from ReceiveAsync", inner.GetType().ToString());
                    COMException realException = (COMException)inner;
                    uint hresult = (uint)realException.ErrorCode;
                    // Normal close scenario
                    uint expectedCloseError = (uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_OBJECT_CLOSED;
                    // If a session abort happened after DequeueOperation was started but before it completed
                    uint expectedCancelError = (uint)NativeTypes.FABRIC_ERROR_CODE.E_ABORT;
                    Assert.IsTrue(expectedCloseError == hresult || expectedCancelError == hresult,
                        "Unexpected error HRESULT code {0} from ReceiveAsync", hresult);
                    sessionClosed = true;
                }
                catch (System.Fabric.FabricObjectClosedException e)
                {
                    LogHelper.Log("SessionId: {0}; standard exception {1} from ReceiveAsync", this.SessionId, e.GetType());
                    sessionAborted = true;
                }
                catch (Exception e)
                {
                    Assert.AreEqual(e.GetType(), typeof(InvalidOperationException),
                        "Unexpected standard exception {0} from ReceiveAsync", e.GetType().ToString());
                    sessionAborted = true;
                }

                messageSequence++;
            }

            LogHelper.Log("SessionId: {0}; Exiting ReceiveAndRecordMessages", this.SessionId);

            return sessionAborted;
        }

        /// <summary>
        /// Flatten to a byte array
        /// </summary>
        /// <param name="source"></param>
        /// <returns></returns>
        public static byte[] Flatten(byte[][] source)
        {
            int[] sourceSizes = source.Select(element => element.Length).ToArray();

            int totalSourceSize = source.Sum(element => element.Length);

            byte[] flatSourceBytes = new byte[totalSourceSize];

            int index = 0;
            int targetIndex = 0;

            foreach (var item in sourceSizes)
            {
                source[index].CopyTo(flatSourceBytes, targetIndex);
                index++;
                targetIndex += item;
            }

           return flatSourceBytes;
        }

        /// <summary>
        /// Constructs a message of random size and fills the message with random numbers
        /// </summary>
        /// <param name="messageNumber"></param>
        /// <param name="localRandGen"></param>
        /// <returns></returns>
        public static byte[][] ConstructMessage(int messageNumber, Random localRandGen)
        {
            // Construct the message of random size
            int messageSize = localRandGen.Next(5, 20);
            byte[][] result = new byte[messageSize + 1][];
            result[0] = BitConverter.GetBytes(messageNumber);

            // Fill the message with random numbers
            for (int i = 0; i < messageSize; i++)
            {
                int segmentSize = localRandGen.Next(100, 1000);
                byte[] nextBuffer = new byte[segmentSize];
                localRandGen.NextBytes(nextBuffer);
                result[i + 1] = nextBuffer;
            }

            return result;
        }

        public static int[] ParseMessage(byte[] flatMessage)
        {
            int[] result = new int[1];
            result[0] = BitConverter.ToInt32(flatMessage, 0);
            return result;
        }
    }
}
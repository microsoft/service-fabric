// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ReliableMessaging.Session
{
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// 
    /// </summary>
    [Serializable]
    public class IntegerPartitionKeyRange : IComparable<IntegerPartitionKeyRange>, IEquatable<IntegerPartitionKeyRange>
    {
        /// <summary>
        /// 
        /// </summary>
        public long IntegerKeyLow;

        /// <summary>
        /// 
        /// </summary>
        public long IntegerKeyHigh;

        /// <summary>
        /// Indicates whether the current object is equal to another object of the same type.
        /// </summary>
        /// <returns>
        /// true if the current object is equal to the <paramref name="other"/> parameter; otherwise, false.
        /// </returns>
        /// <param name="other">An object to compare with this object.</param>
        public bool Equals(IntegerPartitionKeyRange other)
        {
            var result = (this.IntegerKeyLow == other.IntegerKeyLow) && (this.IntegerKeyHigh == other.IntegerKeyHigh);
            return result;
        }

        /// <summary>
        /// Compares the current object with another object of the same type.
        /// </summary>
        /// <returns>
        /// A value that indicates the relative order of the objects being compared. The return value has the following meanings: Value Meaning Less than zero This object is less than the <paramref name="other"/> parameter.Zero This object is equal to <paramref name="other"/>. Greater than zero This object is greater than <paramref name="other"/>. 
        /// </returns>
        /// <param name="other">An object to compare with this object.</param>
        public int CompareTo(IntegerPartitionKeyRange other)
        {
            if (this.IntegerKeyLow == other.IntegerKeyLow)
            {
                return this.IntegerKeyHigh.CompareTo(other.IntegerKeyHigh);
            }
            return this.IntegerKeyLow.CompareTo(other.IntegerKeyLow);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    public class SessionDataSnapshot
    {
        /// <summary>
        /// 
        /// </summary>
        public int InboundMessageQuota;

        /// <summary>
        /// 
        /// </summary>
        public int InboundMessageCount;

        /// <summary>
        /// 
        /// </summary>
        public int SendOperationQuota;

        /// <summary>
        /// 
        /// </summary>
        public int SendOperationCount;

        /// <summary>
        /// 
        /// </summary>
        public int ReceiveOperationQuota;

        /// <summary>
        /// 
        /// </summary>
        public int ReceiveOperationCount;

        /// <summary>
        /// 
        /// </summary>
        public bool IsOutbound;

        /// <summary>
        /// 
        /// </summary>
        public System.Guid SessionId;

        /// <summary>
        /// 
        /// </summary>
        public bool NormalCloseOccurred;

        /// <summary>
        /// 
        /// </summary>
        public bool IsOpenForSend;

        /// <summary>
        /// 
        /// </summary>
        public bool IsOpenForReceive;

        /// <summary>
        /// 
        /// </summary>
        public System.Int64 LastOutboundSequenceNumber;

        /// <summary>
        /// 
        /// </summary>
        public System.Int64 FinalInboundSequenceNumberInSession;
    }

    /// <summary>
    /// Callback supplied by the client of messaging, to grant or deny permission to start an inbound session
    /// </summary>
    public interface IReliableInboundSessionCallback
    {
        /// <summary>
        /// Callback offering a session
        /// Three overloads for singleton, numbered and named partition types
        /// </summary>
        /// <returns> true to grant permission and false to deny</returns>
        bool AcceptInboundSession(
            Uri sourceServiceInstanceName, IReliableMessagingSession session);

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sourceServiceInstanceName"></param>
        /// <param name="partitionKey"></param>
        /// <param name="session"></param>
        /// <returns></returns>
        bool AcceptInboundSession(
            Uri sourceServiceInstanceName, IntegerPartitionKeyRange partitionKey, IReliableMessagingSession session);

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sourceServiceInstanceName"></param>
        /// <param name="partitionKey"></param>
        /// <param name="session"></param>
        /// <returns></returns>
        bool AcceptInboundSession(
            Uri sourceServiceInstanceName, string partitionKey, IReliableMessagingSession session);
    }

    /// <summary>
    /// Callback supplied by the client of messaging, for notification when a session is closed by the partner partition
    /// </summary>
    public interface IReliableSessionAbortedCallback
    {
        /// <summary>
        /// Callback offering a session
        /// Three overloads for singleton, numbered and named partition types
        /// </summary>
        /// <returns> true to grant permission and false to deny</returns>
        void SessionAbortedByPartner(
            bool isInbound, Uri sourceServiceInstanceName, IReliableMessagingSession session);

        /// <summary>
        /// 
        /// </summary>
        /// <param name="isInbound"></param>
        /// <param name="sourceServiceInstanceName"></param>
        /// <param name="partitionKey"></param>
        /// <param name="session"></param>
        void SessionAbortedByPartner(
            bool isInbound, Uri sourceServiceInstanceName, IntegerPartitionKeyRange partitionKey, IReliableMessagingSession session);

        /// <summary>
        /// 
        /// </summary>
        /// <param name="isInbound"></param>
        /// <param name="sourceServiceInstanceName"></param>
        /// <param name="partitionKey"></param>
        /// <param name="session"></param>
        void SessionAbortedByPartner(
            bool isInbound, Uri sourceServiceInstanceName, string partitionKey, IReliableMessagingSession session);
    }

    /// <summary>
    ///  session, used to send and receive messages to a service instance
    /// </summary>
    public interface IReliableMessagingSession
    {
        /// <summary>
        /// Start the session
        /// </summary>
        /// <returns>returns false if the session cannot be started because the name cannot be resolved 
        /// or a connection cannot be established</returns>
        Task OpenAsync(CancellationToken token);

        /// <summary>
        /// Gracefully close the session protocol and release connections and other resources
        /// </summary>
        /// <returns>returns false if the protocol close did not happen gracefully</returns>
        Task CloseAsync(CancellationToken token);

        /// <summary>
        /// Abrupt shutdown of the session protocol and connections and other resources
        /// </summary>   
        void Abort();

        /// <summary>
        /// Async, return to continuation when inbound message available, wait for messages to come in if necessary
        /// </summary>
        /// <returns></returns>
        Task<IOperationData> ReceiveAsync(CancellationToken token);

        /// <summary>
        /// Same as ReceiveAsync, except do not wait for messages to come in, return null IOperationData if there are no messages waiting to be delivered
        /// </summary>
        /// <returns></returns>
        Task<IOperationData> TryReceiveAsync(CancellationToken token);

        /// <summary>
        /// May be delayed locally if outbound buffer is full (throttling case)
        /// Returns to continuation when the message has been acknowledged as received by the target
        /// </summary>
        /// <param name="message"></param>
        /// <param name="token"></param>
        Task SendAsync(IOperationData message, CancellationToken token);

        /// <summary>
        /// Retrieves session data snapshot
        /// </summary>
        /// 
        SessionDataSnapshot GetDataSnapshot();
    }

    /// <summary>
    /// Session manager, used to create  and accept  messaging sessions
    /// </summary>
    public interface IReliableSessionManager
    {
        /// <summary>
        /// Initialize and start the session manager service with owner information as parameter(s)
        /// Three overloads for singleton, numbered and named partition types
        /// </summary>
        /// <returns> The created session</returns>
        Task OpenAsync(IReliableSessionAbortedCallback sessionClosedCallback, CancellationToken cancellationToken);

        /// <summary>
        /// Create the session
        /// Three overloads for singleton, numbered and named partition types
        /// </summary>
        /// <returns> The created session</returns>
        Task<IReliableMessagingSession> CreateOutboundSessionAsync(
            Uri targetServiceInstanceName, string targetEndpoint, CancellationToken cancellationToken);

        /// <summary>
        /// 
        /// </summary>
        /// <param name="targetServiceInstanceName"></param>
        /// <param name="partitionKey"></param>
        /// <param name="targetEndpoint"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        Task<IReliableMessagingSession> CreateOutboundSessionAsync(
            Uri targetServiceInstanceName, long partitionKey, string targetEndpoint, CancellationToken cancellationToken);

        /// <summary>
        /// 
        /// </summary>
        /// <param name="targetServiceInstanceName"></param>
        /// <param name="partitionKey"></param>
        /// <param name="targetEndpoint"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        Task<IReliableMessagingSession> CreateOutboundSessionAsync(
            Uri targetServiceInstanceName, string partitionKey, string targetEndpoint, CancellationToken cancellationToken);

        /// <summary>
        /// Register willingness to accept inbound sessions, with permission granted through
        ///    the IReliableInboundSessionCallback parameter supplied
        /// Temporarily missing the IReliableInboundSessionCallback input parameter
        ///    until KTL async compatible synchronous callback pattern is figured out
        /// </summary>
        /// <returns>the endpoint (FQDN and port) at which the session is listening</returns>
        Task<string> RegisterInboundSessionCallbackAsync(
            IReliableInboundSessionCallback callback, CancellationToken token);

        /// <summary>
        /// Revoke willingness to accept inbound sessions
        /// </summary>
        /// <returns></returns>
        Task UnregisterInboundSessionCallbackAsync(CancellationToken token);

        // <summary>
        // Abrupt shutdown of the session manager and its resources
        // </summary>   
        //void Abort();

        /// <summary>
        /// Graceful shutdown of the session manager and its resources
        /// </summary>
        /// <returns></returns>
        Task CloseAsync(CancellationToken token);
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Replicator;

    /// <summary>
    /// Inbound or Outbound
    /// </summary>
    public enum StreamKind
    {
        /// <summary>
        /// 
        /// </summary>
        Inbound = 1001,

        /// <summary>
        /// 
        /// </summary>
        Outbound = 1002
    }

    /// <summary>
    /// State of the Stream
    /// </summary>
    public enum PersistentStreamState
    {
        /// <summary>
        /// 
        /// </summary>
        Initialized = 2001,

        /// <summary>
        /// 
        /// </summary>
        Opening = 2002,

        /// <summary>
        /// 
        /// </summary>
        Open = 2003,

        /// <summary>
        /// 
        /// </summary>
        Closing = 2004,

        /// <summary>
        /// 
        /// </summary>
        Closed = 2005,

        /// <summary>
        /// 
        /// </summary>
        Deleting = 2006,

        /// <summary>
        /// 
        /// </summary>
        Deleted = 2007
    }

    /// <summary>
    /// Stream Open Outcome
    /// </summary>
    public enum StreamOpenOutcome
    {
        /// <summary>
        /// 
        /// </summary>
        PartnerAccepted = 3001,

        /// <summary>
        /// 
        /// </summary>
        PartnerRejected = 3002,

        /// <summary>
        /// 
        /// </summary>
        PartnerFaulted = 3003,

        /// <summary>
        /// 
        /// </summary>
        PartnerNotReady = 3004,

        /// <summary>
        /// 
        /// </summary>
        PartnerNotListening = 3005,

        /// <summary>
        /// 
        /// </summary>
        Unknown = -10000
    }

    /// <summary>
    /// Inbound Stream Callback interface
    /// </summary>
    public interface IInboundStreamCallback
    {
        /// <summary>
        /// This method is called when the inbound stream has been accepted and its metadata created in the metadata store
        /// At this point the inbound stream is open for receive operations.
        /// </summary>
        /// <param name="stream">The stream object</param>
        Task InboundStreamCreated(IMessageStream stream);

        /// <summary>
        /// This method is called to ask for acceptance of the offered stream
        /// </summary>
        /// <param name="key">The identity of the source partition offering the stream; PartitionKey should really be an interface IPartitionKey</param>
        /// <param name="streamName">The name of the stream (app specific meaning)</param>
        /// <param name="streamId">The unique identity of the stream</param>
        /// <returns></returns>
        Task<bool> InboundStreamRequested(PartitionKey key, Uri streamName, Guid streamId);

        /// <summary>
        /// This method is called when the inbound stream has been deleted and its metadata deleted in the metadata store.
        /// </summary>
        /// <param name="stream">The stream object</param>
        Task InboundStreamDeleted(IMessageStream stream);
    }

    /// <summary>
    /// Stream manager, used to create  and accept  messaging streams
    /// </summary>
    public interface IMessageStreamManager
    {
        /// <summary>
        /// Creates an enumerable of currently active outbound streams
        /// </summary>
        /// <returns>an enumerable collection of stream objects</returns>
        Task<IEnumerable<IMessageStream>> CreateOutboundStreamsEnumerableAsync();

        /// <summary>
        /// Creates an enumerable of currently active inbound streams
        /// </summary>
        /// <returns>an enumerable collection of stream objects</returns>
        Task<IEnumerable<IMessageStream>> CreateInboundStreamsEnumerableAsync();

        /// <summary>
        /// Get a specific inbound stream given its StreamId
        /// </summary>
        /// <returns>Tuple(true,stream) if the stream exists, or Tuple(false,null) if it does not </returns>
        Task<Tuple<bool, IMessageStream>> GetInboundStreamAsync(Guid streamId);

        /// <summary>
        /// Get a specific inbound stream given its Stream Name and Partition Key
        /// </summary>
        /// <returns>Tuple(true,stream) if the stream exists, or Tuple(false,null) if it does not </returns>
        Task<Tuple<bool, IMessageStream>> GetInboundStreamAsync(Uri streamName, PartitionKey partitionKey);

        /// <summary>
        /// Get a specific Inbound stream given its StreamId
        /// </summary>
        /// <returns>Tuple(true,stream) if the stream exists, or Tuple(false,null) if it does not </returns>
        Task<Tuple<bool, IMessageStream>> GetInboundStreamAsync(Transaction streamTransaction, Guid streamId);

        /// <summary>
        /// Get a specific outbound stream given its StreamId
        /// </summary>
        /// <returns>Tuple(true,stream) if the stream exists, or Tuple(false,null) if it does not </returns>
        Task<Tuple<bool, IMessageStream>> GetOutboundStreamAsync(Guid streamId);

        /// <summary>
        /// Get a specific outbound stream given its Stream Name and Partition Key
        /// </summary>
        /// <returns>Tuple(true,stream) if the stream exists, or Tuple(false,null) if it does not </returns>
        Task<Tuple<bool, IMessageStream>> GetOutboundStreamAsync(Uri streamName, PartitionKey partitionKey);

        /// <summary>
        /// Get a specific outbound stream given its Stream Name and Partition Key
        /// </summary>
        /// <returns>Tuple(true,stream) if the stream exists, or Tuple(false,null) if it does not </returns>
        Task<Tuple<bool, IMessageStream>> GetOutboundStreamAsync(Transaction streamTransaction, Uri streamName, PartitionKey partitionKey);

        /// <summary>
        /// Create the stream: singleton target partition; will be collapsed to a single method instead of three, using IPartitionKey
        /// </summary>
        /// <returns> The created stream</returns>
        Task<IMessageStream> CreateStreamAsync(Transaction streamTransaction, Uri serviceInstanceName, Uri streamName, int sendQuota);

        /// <summary>
        /// Create the stream: named target partition
        /// </summary>
        /// <returns> The created stream</returns>
        Task<IMessageStream> CreateStreamAsync(Transaction streamTransaction, Uri serviceInstanceName, string partitionName, Uri streamName, int sendQuota);

        /// <summary>
        /// Create the stream: numbered target partition
        /// </summary>
        /// <returns> The created stream</returns>
        Task<IMessageStream> CreateStreamAsync(Transaction streamTransaction, Uri serviceInstanceName, long partitionNumber, Uri streamName, int sendQuota);

        /// <summary>
        /// Create the stream: target partition encapsulated as PartitionKey object
        /// </summary>
        /// <returns> The created stream</returns>
        Task<IMessageStream> CreateStreamAsync(Transaction streamTransaction, PartitionKey targetPartition, Uri streamName, int sendQuota);

        /// <summary>
        /// Register the call back for a stream name prefix. 
        /// Inbound stream callbacks are routed to this callback when streams matches the prefix.
        /// In case of conflicts, the longest prefix match is selected first. 
        /// </summary>
        /// <param name="streamNamePrefix">Stream Name Prefix - Absolute Uri</param>
        /// <param name="inboundStreamCallback">Callback when stream matches the prefix</param>
        /// <returns>void</returns>
        /// <exception cref="ArgumentException">When stream name prefix is not a valid Absolute Uri, or
        /// when trying to register a duplicate prefix.</exception>
        Task RegisterCallbackByPrefixAsync(Uri streamNamePrefix, IInboundStreamCallback inboundStreamCallback);
    }

    /// <summary>
    /// 
    /// </summary>
    public interface IMessageStreamMetadata
    {
        /// <summary>
        /// 
        /// </summary>
        StreamKind Direction { get; }

        /// <summary>
        /// 
        /// </summary>
        Guid StreamIdentity { get; }

        /// <summary>
        /// 
        /// </summary>
        Uri StreamName { get; }

        /// <summary>
        /// 
        /// </summary>
        int MessageQuota { get; }

        /// <summary>
        /// 
        /// </summary>
        PartitionKey PartnerIdentity { get; }

        /// <summary>
        /// 
        /// </summary>
        PersistentStreamState CurrentState { get; }
    }

    /// <summary>
    /// 
    /// </summary>
    public interface IInboundStreamMessage
    {
        /// <summary>
        /// 
        /// </summary>
        Guid StreamIdentity { get; }

        /// <summary>
        /// 
        /// </summary>
        long MessageSequenceNumber { get; }

        /// <summary>
        /// 
        /// </summary>
        byte[] Payload { get; }
    }

    /// <summary>
    ///  stream, used to send and receive messages to a service instance
    /// </summary>
    public interface IMessageStream : IMessageStreamMetadata
    {
        /// <summary>
        /// Make the stream operational by negotiating acceptance with the target partition
        /// This is only meaningful for outbound streams, use with inbound streams will throw an InvalidOperationException
        /// </summary>
        /// <returns>true if Open succeeded, false if the stream cannot be started because the name cannot be resolved 
        /// or a connection cannot be established or the stream is rejected by the target</returns>
        Task<StreamOpenOutcome> OpenAsync(TimeSpan timeout);

        /// <summary>
        /// Gracefully close the stream protocol and release connections and other resources
        /// This is only meaningful for outbound streams, use with inbound streams will throw an InvalidOperationException
        /// </summary>
        /// <returns>returns true is Close completed successfully, false if Close did not complete gracefully</returns>
        Task<bool> CloseAsync(TimeSpan timeout);

        /// <summary>
        /// Async, return to continuation when inbound message available, wait for messages to come in if necessary
        /// Will return a null payload once the stream is closed
        /// This is only meaningful for inbound streams, use with outbound streams will throw an InvalidOperationException
        /// </summary>
        /// <returns></returns>
        Task<IInboundStreamMessage> ReceiveAsync(Transaction txn, TimeSpan timeout);

        /// <summary>
        /// Completion may be delayed if outbound quota is full (throttling case)
        /// Returns to continuation when the message has been acknowledged as received by the target
        /// This is only meaningful for outbound streams, use with inbound streams will throw an InvalidOperationException
        /// </summary>
        Task SendAsync(Transaction txn, byte[] data, TimeSpan timeout);

        /// <summary>
        /// Last payload sequence number sent as seen in the committed state outside the context of any service transaction
        /// This is only meaningful for outbound streams, use with inbound streams will throw an InvalidOperationException
        /// </summary>
        /// <returns>last sequence number sent</returns>
        Task<long> GetLastSequenceNumberSentAsync();

        /// <summary>
        /// Last payload sequence number received as seen in the committed state outside the context of any service transaction
        /// This is only meaningful for inbound streams, use with outbound streams will throw an InvalidOperationException
        /// </summary>
        /// <returns>last sequence number received</returns>
        Task<long> GetLastSequenceNumberReceivedAsync();

        /// <summary>
        /// Deletes the stream  by negotiating acceptance with the target partition. 
        /// This is only meaningful for outbound streams that are in a closed state, use with inbound streams will throw an InvalidOperationException
        /// </summary>
        /// <returns>Delete outcome of stream in deleting state, deleted or not found</returns>
        Task<bool> DeleteAsync(Transaction txn, TimeSpan timeout);
    }
}
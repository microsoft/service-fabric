// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    #region using directives

    using System.Collections.Concurrent;
    using System.Threading.Tasks;

    #endregion

    /// <summary>
    /// An instance of ActiveResources, maintains all required runtime resources required by the stream manager.
    /// 1. 
    /// </summary>
    internal class ActiveResources
    {
        #region properties

        /// <summary>
        /// Maintains the next sequence to delete(receive confirmed) for each stream.
        /// </summary>
        internal ConcurrentDictionary<Guid, long> NextSequenceNumberToDelete { get; private set; }

        /// <summary>
        /// Collection of Open Stream Completions
        /// </summary>
        internal ConcurrentDictionary<Guid, TaskCompletionSource<StreamWireProtocolResponse>> OpenStreamCompletions { get; private set; }

        /// <summary>
        /// Collection of Close Stream Completions
        /// </summary>
        internal ConcurrentDictionary<Guid, TaskCompletionSource<StreamWireProtocolResponse>> CloseStreamCompletions { get; private set; }

        /// <summary>
        /// Collection of Close Stream Completions
        /// </summary>
        internal ConcurrentDictionary<Guid, TaskCompletionSource<StreamWireProtocolResponse>> DeleteStreamCompletions { get; private set; }

        // <SYNCPoints>
        /// <summary>
        /// Pause Stream Sync points collection to ensure orderly stop and closure of streams.
        /// </summary>
        internal ConcurrentDictionary<Guid, SyncPoint<Guid>> PauseStreamSyncpoints { get; private set; }

        /// <summary>
        /// Outbound Session Driver sync point to ensure orderly creation and deletion of outboundsession driver.
        /// </summary>
        internal ConcurrentDictionary<PartitionKey, SyncPoint<PartitionKey>> OutboundSessionDriverSyncpoints { get; private set; }

        /// <summary>
        /// Partner reset sync point to ensure the orderly reset of streams during Partner reset activity.
        /// </summary>
        internal ConcurrentDictionary<PartitionKey, SyncPoint<PartitionKey>> PartnerResetSyncpoints { get; private set; }

        // </SYNCPoints>

        /// <summary>
        /// Maintains a list of Outbound Streams
        /// </summary>
        internal ConcurrentDictionary<Guid, MessageStream> OutboundStreams { get; private set; }

        /// <summary>
        /// Maintains a list of Inbound Streams
        /// </summary>
        internal ConcurrentDictionary<Guid, MessageStream> InboundStreams { get; private set; }

        /// <summary>
        /// Maintains a list of OutboundSessionDrivers
        /// </summary>
        internal ConcurrentDictionary<PartitionKey, OutboundSessionDriver> OutboundSessionDrivers { get; private set; }

        /// <summary>
        /// Maintains a list of InboundSessionDrivers
        /// </summary>
        internal ConcurrentDictionary<PartitionKey, InboundSessionDriver> InboundSessionDrivers { get; private set; }

        /// <summary>
        /// Maintains a list of active inbound transactions 
        /// </summary>
        internal ConcurrentDictionary<long, ConcurrentDictionary<Guid, MessageStream>> ActiveInboundStreamsByTransaction { get; private set; }

        /// <summary>
        /// Maintains a list of callbacks, we route the callbacks based on a prefix match.
        /// </summary>
        internal InboundCallbacks InboundCallbacks { get; private set; }

        #endregion

        /// <summary>
        /// Creates an instance of Active Resources.
        /// </summary>
        public ActiveResources()
        {
            this.InboundSessionDrivers = new ConcurrentDictionary<PartitionKey, InboundSessionDriver>(new PartitionKeyEqualityComparer());
            this.OutboundSessionDrivers = new ConcurrentDictionary<PartitionKey, OutboundSessionDriver>(new PartitionKeyEqualityComparer());
            this.InboundStreams = new ConcurrentDictionary<Guid, MessageStream>();
            this.OutboundStreams = new ConcurrentDictionary<Guid, MessageStream>();
            this.ActiveInboundStreamsByTransaction = new ConcurrentDictionary<long, ConcurrentDictionary<Guid, MessageStream>>();
            this.PartnerResetSyncpoints = new ConcurrentDictionary<PartitionKey, SyncPoint<PartitionKey>>(new PartitionKeyEqualityComparer());
            this.OutboundSessionDriverSyncpoints = new ConcurrentDictionary<PartitionKey, SyncPoint<PartitionKey>>(new PartitionKeyEqualityComparer());
            this.PauseStreamSyncpoints = new ConcurrentDictionary<Guid, SyncPoint<Guid>>();
            this.CloseStreamCompletions = new ConcurrentDictionary<Guid, TaskCompletionSource<StreamWireProtocolResponse>>();
            this.DeleteStreamCompletions = new ConcurrentDictionary<Guid, TaskCompletionSource<StreamWireProtocolResponse>>();
            this.OpenStreamCompletions = new ConcurrentDictionary<Guid, TaskCompletionSource<StreamWireProtocolResponse>>();
            this.NextSequenceNumberToDelete = new ConcurrentDictionary<Guid, long>();
            this.InboundCallbacks = new InboundCallbacks();
        }

        /// <summary>
        /// Clear the collections when the role is set to non-primary.
        /// </summary>
        internal void Clear()
        {
            this.NextSequenceNumberToDelete.Clear();
            this.OpenStreamCompletions.Clear();
            this.CloseStreamCompletions.Clear();
            this.DeleteStreamCompletions.Clear();
            this.OutboundSessionDrivers.Clear();
            this.InboundSessionDrivers.Clear();
            this.OutboundStreams.Clear();
            this.InboundStreams.Clear();
            this.ActiveInboundStreamsByTransaction.Clear();
            this.InboundCallbacks.Clear();

            // we set these SyncPointCollections to null to signify that they are no longer meaningful, however, references to them are still held by the
            // SyncPoint objects still in these collections as values, and those references will be used to complete the lifecycle of those SyncPoints
            // Construction of new SyncPoints while these fields are null will cause FabricNotPrimaryExceptions
            // TODO: Check this later.
            this.PauseStreamSyncpoints = null;
            this.OutboundSessionDriverSyncpoints = null;
            this.PartnerResetSyncpoints = null;
        }

        /// <summary>
        /// Reset the remaining collections when the role changes to a primary, essentially we start with a clean state of 
        /// active resources when role changes to a primary.
        /// </summary>
        internal void ResetSyncPointCollections()
        {
            this.PauseStreamSyncpoints = new ConcurrentDictionary<Guid, SyncPoint<Guid>>();
            this.OutboundSessionDriverSyncpoints = new ConcurrentDictionary<PartitionKey, SyncPoint<PartitionKey>>(new PartitionKeyEqualityComparer());
            this.PartnerResetSyncpoints = new ConcurrentDictionary<PartitionKey, SyncPoint<PartitionKey>>(new PartitionKeyEqualityComparer());
        }
    }
}
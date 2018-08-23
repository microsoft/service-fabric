// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections.Generic;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents the state of the secondary replicator from the primary replicator’s point of view.</para>
    /// </summary>
    public sealed class RemoteReplicatorStatus
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Query.RemoteReplicatorStatus" /> class.</para>
        /// </summary>
        public RemoteReplicatorStatus()
        {
        }

        /// <summary>
        /// <para>Gets the replica ID of the secondary.</para>
        /// </summary>
        /// <value>
        /// <para>The replica ID.</para>
        /// </value>
        public long ReplicaId { get; internal set; }

        /// <summary>
        /// <para>Gets the last timestamp (in UTC) when an acknowledgement from the secondary replicator was processed in the primary.</para>
        /// </summary>
        /// <value>
        /// <para>The last timestamp when an acknowledgement from the secondary replicator was processed in the primary.</para>
        /// </value>
        /// <remarks>
        /// <para>UTC 0 represents an invalid value, indicating that no acknowledgement messages were ever processed.</para>
        /// </remarks>
        public DateTime LastAcknowledgementProcessedTimeUtc { get; internal set; }

        /// <summary>
        /// <para>Gets the highest replication operation sequence number that the secondary has received from the primary.</para>
        /// </summary>
        /// <value>
        /// <para>The highest replication operation sequence number that the secondary has received from the primary.</para>
        /// </value>
        public long LastReceivedReplicationSequenceNumber { get; private set; }

        /// <summary>
        /// <para>Gets the highest replication operation sequence number that the secondary has applied to its state.</para>
        /// </summary>
        /// <value>
        /// <para>The highest replication operation sequence number that the secondary has applied to its state.</para>
        /// </value>
        public long LastAppliedReplicationSequenceNumber { get; private set; }

        /// <summary>
        /// <para>Gets a value that indicates whether the secondary replica is in the process of being built.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the secondary replica is in the process of being built; 
        /// otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        public bool IsInBuild { get; private set; }

        /// <summary>
        /// <para>Gets the highest copy operation sequence number that the secondary has received from the primary.</para>
        /// </summary>
        /// <value>
        /// <para>The highest copy operation sequence number that the secondary has received from the primary.</para>
        /// </value>
        /// <remarks>
        /// <para>A value of ‘-1’ can be ignored since it indicates that the copy process is complete.</para>
        /// </remarks>
        public long LastReceivedCopySequenceNumber { get; private set; }
        
        /// <summary>
        /// <para>Gets the highest copy operation sequence number that the secondary has applied to its state.</para>
        /// </summary>
        /// <value>
        /// <para>The highest copy operation sequence number that the secondary has applied to its state.</para>
        /// </value>
        /// <remarks>
        /// <para>A value of ‘-1’ can be ignored since it indicates that the copy process is complete.</para>
        /// </remarks>
        public long LastAppliedCopySequenceNumber { get; private set; }

        /// <summary>
        /// <para>Contains acknowledgement numbers for each of the remote replicators</para>
        /// <para>The values are dependent on the status of the replicas. 
        /// Inbuild replicas will contain values pertaining to copy while active replias will not.</para>
        /// </summary>
        /// <value>
        /// <para>RemoteReplicatorAcknowledgementStatus object containing details regarding replication and copy stream acknowledgement. See <see cref="System.Fabric.Query.RemoteReplicatorAcknowledgementStatus" /> for more information. </para>
        /// </value>
        public RemoteReplicatorAcknowledgementStatus RemoteReplicatorAcknowledgementStatus { get; private set; }

        private static unsafe RemoteReplicatorStatus CreateFromNative(NativeTypes.FABRIC_REMOTE_REPLICATOR_STATUS nativeResultItem)
        {
            return new RemoteReplicatorStatus
            {
                ReplicaId = nativeResultItem.ReplicaId,
                LastAcknowledgementProcessedTimeUtc = NativeTypes.FromNativeFILETIME(nativeResultItem.LastAcknowledgementProcessedTimeUtc),
                LastReceivedReplicationSequenceNumber = nativeResultItem.LastReceivedReplicationSequenceNumber,
                LastAppliedReplicationSequenceNumber = nativeResultItem.LastAppliedReplicationSequenceNumber,
                LastReceivedCopySequenceNumber = nativeResultItem.LastReceivedCopySequenceNumber,
                LastAppliedCopySequenceNumber = nativeResultItem.LastAppliedCopySequenceNumber,
                IsInBuild = NativeTypes.FromBOOLEAN(nativeResultItem.IsInBuild),
                RemoteReplicatorAcknowledgementStatus =
                    RemoteReplicatorAcknowledgementStatus.CreateFromNative(
                        *(NativeTypes.FABRIC_REMOTE_REPLICATOR_ACKNOWLEDGEMENT_STATUS*) nativeResultItem.Reserved)
            };
        }

        internal static unsafe IList<RemoteReplicatorStatus> CreateFromNativeList(NativeTypes.FABRIC_REMOTE_REPLICATOR_STATUS_LIST* list)
        {
            var rv = new List<RemoteReplicatorStatus>();

            var nativeArray = (NativeTypes.FABRIC_REMOTE_REPLICATOR_STATUS*)list->Items;
            for (int i = 0; i < list->Count; i++)
            {
                var nativeItem = *(nativeArray + i);
                rv.Add(RemoteReplicatorStatus.CreateFromNative(nativeItem));
            }

            return rv;
        }
    }
}
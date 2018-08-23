// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Provides statistics about the Service Fabric Replicator, when it is functioning in a <see cref="System.Fabric.ReplicaRole.ActiveSecondary" /> role.</para>
    /// </summary>
    public sealed class SecondaryReplicatorStatus : ReplicatorStatus
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Query.SecondaryReplicatorStatus" /> class.</para>
        /// </summary>
        public SecondaryReplicatorStatus()
            : base(ReplicaRole.ActiveSecondary)
        {
        }

        internal SecondaryReplicatorStatus(bool isIdle = false)
            : base(isIdle? ReplicaRole.IdleSecondary : ReplicaRole.ActiveSecondary)
        {
        }

        /// <summary>
        /// <para>Gets the status of the Replication queue.</para>
        /// </summary>
        /// <value>
        /// <para>The status of the replication queue.</para>
        /// </value>
        public ReplicatorQueueStatus ReplicationQueueStatus { get; private set; }

        /// <summary>
        /// <para>Gets the last time-stamp (UTC) at which a replication operation was received from the primary.</para>
        /// </summary>
        /// <value>
        /// <para>The last time-stamp at which a replication operation was received from the primary.</para>
        /// </value>
        /// <remarks>
        /// <para>UTC 0 represents an invalid value, indicating that a replication operation message was never received.</para>
        /// </remarks>
        public DateTime LastReplicationOperationReceivedTimeUtc { get; private set; }

        /// <summary>
        /// <para>Gets a value that indicates whether the replica is currently being built.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the replica is currently being built; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        public bool IsInBuild { get; private set; }

        /// <summary>
        /// <para>Gets the status of the Copy queue.</para>
        /// </summary>
        /// <value>
        /// <para>The status of the copy queue.</para>
        /// </value>
        public ReplicatorQueueStatus CopyQueueStatus { get; private set; }

        /// <summary>
        /// <para>Gets the last time-stamp (UTC) at which a copy operation was received as part of a build from the primary.</para>
        /// </summary>
        /// <value>
        /// <para>The last time-stamp at which a copy operation was received as part of a build from the primary.</para>
        /// </value>
        /// <remarks>
        /// <para>UTC 0 represents an invalid value, indicating that a copy operation message was never received.</para>
        /// </remarks>
        public DateTime LastCopyOperationReceivedTimeUtc { get; private set; }

        /// <summary>
        /// <para>Gets the last time-stamp (UTC) at which an acknowledgment was sent to the primary replicator.</para>
        /// </summary>
        /// <value>
        /// <para>The last time-stamp at which an acknowledgment was sent to the primary replicator.</para>
        /// </value>
        /// <remarks>
        /// <para>UTC 0 represents an invalid value, indicating that an acknowledgment message was never sent.</para>
        /// </remarks>
        public DateTime LastAcknowledgementSentTimeUtc { get; private set; }

        internal static unsafe ReplicatorStatus CreateFromNative(NativeTypes.FABRIC_SECONDARY_REPLICATOR_STATUS_QUERY_RESULT* nativeEntryPoint)
        {
            return new SecondaryReplicatorStatus
            {
                ReplicationQueueStatus = ReplicatorQueueStatus.CreateFromNative((NativeTypes.FABRIC_REPLICATOR_QUEUE_STATUS*)nativeEntryPoint->ReplicatonQueueStatus),
                LastReplicationOperationReceivedTimeUtc = NativeTypes.FromNativeFILETIME(nativeEntryPoint->LastReplicationOperationReceivedTimeUtc),
                IsInBuild = NativeTypes.FromBOOLEAN(nativeEntryPoint->IsInBuild),
                CopyQueueStatus = ReplicatorQueueStatus.CreateFromNative((NativeTypes.FABRIC_REPLICATOR_QUEUE_STATUS*)nativeEntryPoint->CopyQueueStatus),
                LastCopyOperationReceivedTimeUtc = NativeTypes.FromNativeFILETIME(nativeEntryPoint->LastCopyOperationReceivedTimeUtc),
                LastAcknowledgementSentTimeUtc = NativeTypes.FromNativeFILETIME(nativeEntryPoint->LastAcknowledgementSentTimeUtc),
            };
        }
    }
}
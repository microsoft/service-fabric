// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections.Generic;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Provides statistics about the Service Fabric Replicator, when it is functioning in a <see cref="System.Fabric.ReplicaRole.Primary" /> role.</para>
    /// </summary>
    public sealed class PrimaryReplicatorStatus : ReplicatorStatus
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Query.PrimaryReplicatorStatus" /> class.</para>
        /// </summary>
        public PrimaryReplicatorStatus()
            : base(ReplicaRole.Primary)
        {
        }

        /// <summary>
        /// <para>Gets the status of the Replication queue.</para>
        /// </summary>
        /// <value>
        /// <para>The status of the replication queue.</para>
        /// </value>
        public ReplicatorQueueStatus ReplicationQueueStatus { get; internal set; }

        /// <summary>
        /// <para>Gets the status of all the secondary replicas that the primary is aware of.</para>
        /// </summary>
        /// <value>
        /// <para>The list of remote replicators.</para>
        /// </value>
        public IList<RemoteReplicatorStatus> RemoteReplicators { get; internal set; }

        internal static unsafe ReplicatorStatus CreateFromNative(NativeTypes.FABRIC_PRIMARY_REPLICATOR_STATUS_QUERY_RESULT* nativeEntryPoint)
        {
            return new PrimaryReplicatorStatus
            {
                ReplicationQueueStatus = ReplicatorQueueStatus.CreateFromNative((NativeTypes.FABRIC_REPLICATOR_QUEUE_STATUS*)nativeEntryPoint->ReplicatonQueueStatus),
                RemoteReplicators = RemoteReplicatorStatus.CreateFromNativeList((NativeTypes.FABRIC_REMOTE_REPLICATOR_STATUS_LIST*)nativeEntryPoint->RemoteReplicators),
            };
        }
    }
}
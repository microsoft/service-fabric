// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Health;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a stateful service replica.</para>
    /// </summary>
    public sealed class StatefulServiceReplica : Replica
    {
        internal StatefulServiceReplica(
            ServiceReplicaStatus replicaStatus,
            HealthState healthState,
            ReplicaRole replicaRole,
            string replicaAddress,
            string nodeName,
            long replicaId,
            TimeSpan lastInBuildDuration)
            : base(ServiceKind.Stateful, replicaId, replicaStatus, healthState, replicaAddress, nodeName, lastInBuildDuration)
        {
            this.ReplicaRole = replicaRole;
        }

        private StatefulServiceReplica()
            : this(ServiceReplicaStatus.Invalid, HealthState.Invalid, ReplicaRole.Unknown, null, null, 0, TimeSpan.Zero)
        { }

        /// <summary>
        /// <para>Gets the replica role.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.ReplicaRole" />.</para>
        /// </value>
        public ReplicaRole ReplicaRole { get; private set; }

        internal long ReplicaId
        {
            get
            {
                return this.Id;
            }

            private set
            {
                this.Id = value;
            }
        }

        internal static unsafe StatefulServiceReplica FromNative(NativeTypes.FABRIC_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM statefulReplicaResultItem)
        {
            return new StatefulServiceReplica(
                (System.Fabric.Query.ServiceReplicaStatus)statefulReplicaResultItem.ReplicaStatus,
                (HealthState)statefulReplicaResultItem.HealthState,
                (System.Fabric.ReplicaRole)statefulReplicaResultItem.ReplicaRole,
                NativeTypes.FromNativeString(statefulReplicaResultItem.ReplicaAddress),
                NativeTypes.FromNativeString(statefulReplicaResultItem.NodeName),
                statefulReplicaResultItem.ReplicaId,
                TimeSpan.FromSeconds(statefulReplicaResultItem.LastInBuildDurationInSeconds));
        }
    }
}
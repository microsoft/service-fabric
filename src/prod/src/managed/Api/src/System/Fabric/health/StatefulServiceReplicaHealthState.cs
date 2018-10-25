// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Fabric.Interop;
    using System.Fabric.Query;

    /// <summary>
    /// <para>Represents the health state of the stateful service replica, which contains the replica is and the aggregated health state.</para>
    /// </summary>
    public sealed class StatefulServiceReplicaHealthState : ReplicaHealthState
    {
        internal StatefulServiceReplicaHealthState()
            : base(ServiceKind.Stateful)
        {
        }

        /// <summary>
        /// <para>Gets the replica ID.</para>
        /// </summary>
        /// <value>
        /// <para>The replica ID.</para>
        /// </value>
        public long ReplicaId
        {
            get
            {
                return this.Id;
            }

            internal set
            {
                this.Id = value;
            }
        }

        internal static unsafe StatefulServiceReplicaHealthState FromNative(NativeTypes.FABRIC_STATEFUL_SERVICE_REPLICA_HEALTH_STATE nativeState)
        {
            var statefulServiceReplicaState = new StatefulServiceReplicaHealthState();

            statefulServiceReplicaState.Id = nativeState.ReplicaId;
            statefulServiceReplicaState.PartitionId = nativeState.PartitionId;
            statefulServiceReplicaState.AggregatedHealthState = (HealthState)nativeState.AggregatedHealthState;

            return statefulServiceReplicaState;
        }
    }
}
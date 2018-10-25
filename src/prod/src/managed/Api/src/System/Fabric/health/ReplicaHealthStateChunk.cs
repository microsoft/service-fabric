// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// Represents a replica health state chunk, which contains basic health information about the replica.
    /// It is included as child of a partition.
    /// </summary>
    public sealed class ReplicaHealthStateChunk
    {
        /// <summary>
        /// Instantiates a <see cref="System.Fabric.Health.ReplicaHealthStateChunk"/>.
        /// </summary>
        internal ReplicaHealthStateChunk()
        {
        }

        /// <summary>
        /// Gets the aggregated health state of the replica, computed based on all reported health events and the application health policy.
        /// </summary>
        /// <value>The aggregated health state of the replica.</value>
        public HealthState HealthState
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the replica id or the instance id, depending on the service kind.
        /// </summary>
        /// <value>The replica id or the instance id, depending on the service kind.</value>
        public long ReplicaOrInstanceId
        {
            get;
            internal set;
        }

        /// <summary>
        /// Creates a string description of the replica health state chunk.
        /// </summary>
        /// <returns>String description of the health state chunk.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "{0}: {1}",
                this.ReplicaOrInstanceId,
                this.HealthState);
        }

        internal static unsafe ReplicaHealthStateChunk FromNative(NativeTypes.FABRIC_REPLICA_HEALTH_STATE_CHUNK nativeReplicaHealthStateChunk)
        {
            var replicaHealthStateChunk = new ReplicaHealthStateChunk();

            replicaHealthStateChunk.ReplicaOrInstanceId = (long)nativeReplicaHealthStateChunk.ReplicaOrInstanceId;
            replicaHealthStateChunk.HealthState = (HealthState)nativeReplicaHealthStateChunk.HealthState;

            return replicaHealthStateChunk;
        }
    }
}

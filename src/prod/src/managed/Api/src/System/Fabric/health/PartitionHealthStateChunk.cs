// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// Represents a partition health state chunk, which contains basic health information about the partition.
    /// </summary>
    public sealed class PartitionHealthStateChunk
    {
        /// <summary>
        /// Instantiates a <see cref="System.Fabric.Health.PartitionHealthStateChunk"/>.
        /// </summary>
        internal PartitionHealthStateChunk()
        {
        }

        /// <summary>
        /// Gets the partition id.
        /// </summary>
        /// <value>The partition id.</value>
        public Guid PartitionId
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the partition aggregated health state, computed based on all reported health events, the replica children and the application health policy.
        /// </summary>
        /// <value>The aggregated health state of the partition.</value>
        public HealthState HealthState
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the list of the partition replica health state chunks that respect the input filters.
        /// </summary>
        /// <value>The list of the partition replica health state chunks that respect the input filters.</value>
        /// <remarks>
        /// <para>By default, no children are included in results. Users can request to include
        /// some children based on desired health or other information. 
        /// For example, users can request to include all replicas that have health state error.
        /// Regardless of filter value, all children are used to compute partition aggregated health.</para>
        /// </remarks>
        public ReplicaHealthStateChunkList ReplicaHealthStateChunks
        {
            get;
            internal set;
        }

        /// <summary>
        /// Creates a string description of the partition health state chunk.
        /// </summary>
        /// <returns>String description of the health state chunk.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "{0}: {1}",
                this.PartitionId,
                this.HealthState);
        }
        
        internal static unsafe PartitionHealthStateChunk FromNative(NativeTypes.FABRIC_PARTITION_HEALTH_STATE_CHUNK nativePartitionHealthStateChunk)
        {
            var managedPartitionHealthStateChunk = new PartitionHealthStateChunk();

            managedPartitionHealthStateChunk.PartitionId = nativePartitionHealthStateChunk.PartitionId;
            managedPartitionHealthStateChunk.HealthState = (HealthState)nativePartitionHealthStateChunk.HealthState;
            managedPartitionHealthStateChunk.ReplicaHealthStateChunks = ReplicaHealthStateChunkList.CreateFromNativeList(nativePartitionHealthStateChunk.ReplicaHealthStateChunks);

           return managedPartitionHealthStateChunk;
        }
    }
}

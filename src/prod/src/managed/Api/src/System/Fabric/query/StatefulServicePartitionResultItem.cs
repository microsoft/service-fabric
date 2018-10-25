// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Common.Serialization;
    using System.Fabric.Health;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a stateful service partition.</para>
    /// </summary>
    public sealed class StatefulServicePartition : Partition
    {
        internal StatefulServicePartition(
            ServicePartitionInformation partitionInformation,
            long targetReplicaSetSize,
            long minReplicaSetSize,
            HealthState healthState,
            ServicePartitionStatus partitionStatus,
            TimeSpan lastQuorumLossDuration,
            Epoch primaryEpoch)
            : base(ServiceKind.Stateful, partitionInformation, healthState, partitionStatus)
        {
            this.TargetReplicaSetSize = targetReplicaSetSize;
            this.MinReplicaSetSize = minReplicaSetSize;
            this.LastQuorumLossDuration = lastQuorumLossDuration;
            this.PrimaryEpoch = primaryEpoch;
        }

        private StatefulServicePartition()
            : this(null, 0, 0, HealthState.Unknown, ServicePartitionStatus.Invalid, TimeSpan.Zero, new Epoch())
        { }

        /// <summary>
        /// <para>Gets the target replica set size.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Int64" />.</para>
        /// </value>
        public long TargetReplicaSetSize { get; private set; }

        /// <summary>
        /// <para>Gets the minimum replica set size allowed for the partition to keep making progress.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Int64" />.</para>
        /// </value>
        public long MinReplicaSetSize { get; private set; }

        /// <summary>
        /// <para>Gets the last quorum loss duration.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.TimeSpan" />.</para>
        /// </value>
        [JsonCustomization(IsIgnored = true)]
        public TimeSpan LastQuorumLossDuration { get; private set; }

        private long LastQuorumLossDurationInSeconds
        {
            get { return (long)this.LastQuorumLossDuration.TotalSeconds; }
            set { this.LastQuorumLossDuration = TimeSpan.FromTicks(value * TimeSpan.TicksPerSecond); }
        }

        /// <summary>
        /// <para>
        /// Gets the epoch of the partition as seen by the replica
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The epoch of the partition</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.CurrentConfigurationEpoch)]
        public Epoch PrimaryEpoch { get; private set; }

        internal static unsafe StatefulServicePartition FromNative(NativeTypes.FABRIC_STATEFUL_SERVICE_PARTITION_QUERY_RESULT_ITEM statefulPartitionResultItem)
        {
            NativeTypes.FABRIC_SERVICE_PARTITION_INFORMATION* nativePartitionInformation = (NativeTypes.FABRIC_SERVICE_PARTITION_INFORMATION*)statefulPartitionResultItem.PartitionInformation;
            NativeTypes.FABRIC_STATEFUL_SERVICE_PARTITION_QUERY_RESULT_ITEM_EX1* statefulPartitionResultItemEx1 = (NativeTypes.FABRIC_STATEFUL_SERVICE_PARTITION_QUERY_RESULT_ITEM_EX1*)statefulPartitionResultItem.Reserved;
            return new StatefulServicePartition(
                ServicePartitionInformation.FromNative(*nativePartitionInformation),
                statefulPartitionResultItem.TargetReplicaSetSize,
                statefulPartitionResultItem.MinReplicaSetSize,
                (HealthState)statefulPartitionResultItem.HealthState,
                (ServicePartitionStatus)statefulPartitionResultItem.PartitionStatus,
                TimeSpan.FromSeconds(statefulPartitionResultItem.LastQuorumLossDurationInSeconds),
                Epoch.FromNative(statefulPartitionResultItemEx1->PrimaryEpoch));
        }
    }
}
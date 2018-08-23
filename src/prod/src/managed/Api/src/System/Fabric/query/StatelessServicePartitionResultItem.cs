// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Health;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a stateless service partition.</para>
    /// </summary>
    public sealed class StatelessServicePartition : Partition
    {
        internal StatelessServicePartition(
            ServicePartitionInformation partitionInformation,
            long instanceCount,
            HealthState healthState,
            ServicePartitionStatus partitionStatus)
            : base(ServiceKind.Stateless, partitionInformation, healthState, partitionStatus)
        {
            this.InstanceCount = instanceCount;
        }

        private StatelessServicePartition()
            : this(null, 0, HealthState.Unknown, ServicePartitionStatus.Invalid)
        { }

        /// <summary>
        /// <para>Gets the instance count.</para>
        /// </summary>
        /// <value>
        /// <para>The instance count.</para>
        /// </value>
        public long InstanceCount { get; private set; }

        internal static unsafe StatelessServicePartition FromNative(NativeTypes.FABRIC_STATELESS_SERVICE_PARTITION_QUERY_RESULT_ITEM statelessPartitionResultItem)
        {
            NativeTypes.FABRIC_SERVICE_PARTITION_INFORMATION* nativePartitionInformation = (NativeTypes.FABRIC_SERVICE_PARTITION_INFORMATION*)statelessPartitionResultItem.PartitionInformation;
            return new StatelessServicePartition(
                    ServicePartitionInformation.FromNative(*nativePartitionInformation),
                    statelessPartitionResultItem.InstanceCount,
                    (HealthState)statelessPartitionResultItem.HealthState,
                    (ServicePartitionStatus)statelessPartitionResultItem.PartitionStatus);
        }
    }
}
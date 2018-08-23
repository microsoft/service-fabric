// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// <para>Represents the health state of a partition, which contains the partition identifier and its aggregated health state.</para>
    /// </summary>
    public sealed class PartitionHealthState : EntityHealthState
    {
        internal PartitionHealthState()
        {
        }

        /// <summary>
        /// <para>Gets the partition ID.</para>
        /// </summary>
        /// <value>
        /// <para>The partition ID.</para>
        /// </value>
        public Guid PartitionId
        {
            get;
            internal set;
        }

        /// <summary>
        /// Creates a string description of the partition health state.
        /// </summary>
        /// <returns>String description of the <see cref="System.Fabric.Health.PartitionHealthState"/>.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "{0}: {1}",
                this.PartitionId,
                this.AggregatedHealthState);
        }

        internal static unsafe IList<PartitionHealthState> FromNativeList(IntPtr nativeListPtr)
        {
            if (nativeListPtr != IntPtr.Zero)
            {
                var nativeList = (NativeTypes.FABRIC_PARTITION_HEALTH_STATE_LIST*)nativeListPtr;
                return PartitionHealthStateList.FromNativeList(nativeList);
            }
            else
            {
                return new PartitionHealthStateList();
            }
        }

        internal static unsafe PartitionHealthState FromNative(NativeTypes.FABRIC_PARTITION_HEALTH_STATE nativeState)
        {
            var partitionHealthState = new PartitionHealthState();
            partitionHealthState.PartitionId = nativeState.PartitionId;
            partitionHealthState.AggregatedHealthState = (HealthState)nativeState.AggregatedHealthState;

            return partitionHealthState;
        }
    }
}
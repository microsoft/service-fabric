// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>
    /// Represents the SafetyCheck for a partition.
    /// </para>
    /// </summary>
    public sealed class PartitionSafetyCheck : SafetyCheck
    {
        internal PartitionSafetyCheck(SafetyCheckKind kind, Guid partitionId)
            : base(kind)
        {
            this.PartitionId = partitionId;
        }

        /// <summary>
        /// <para>
        /// Gets the ID of the partition that is undergoing a safety check.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The ID of the partition that is undergoing a safety check.</para>
        /// </value>
        public Guid PartitionId
        {
            get;
            private set;
        }

        internal static unsafe PartitionSafetyCheck FromNative(
            SafetyCheckKind kind,
            NativeTypes.FABRIC_PARTITION_SAFETY_CHECK* nativePtr)
        {
            return new PartitionSafetyCheck(kind, nativePtr->PartitionId);
        }
    }
}
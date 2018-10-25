// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents the <see cref="System.Fabric.UpgradeSafetyCheck" /> for a partition.</para>
    /// </summary>
    public sealed class PartitionUpgradeSafetyCheck : UpgradeSafetyCheck
    {
        internal PartitionUpgradeSafetyCheck(UpgradeSafetyCheckKind kind, Guid partitionId)
            : base(kind)
        {
            this.PartitionId = partitionId;
        }

        /// <summary>
        /// <para>Gets the ID of a partition that is undergoing a upgrade safety check.</para>
        /// </summary>
        /// <value>
        /// <para>The ID of a partition that is undergoing a upgrade safety check.</para>
        /// </value>
        public Guid PartitionId
        {
            get;
            private set;
        }

        internal static unsafe PartitionUpgradeSafetyCheck FromNative(
            UpgradeSafetyCheckKind kind,
            NativeTypes.FABRIC_UPGRADE_PARTITION_SAFETY_CHECK* nativePtr)
        {
            return new PartitionUpgradeSafetyCheck(kind, nativePtr->PartitionId);
        }
    }
}
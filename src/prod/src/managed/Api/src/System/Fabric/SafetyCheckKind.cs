// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>
    /// A Service Fabric enumeration that indicates the upgrade safety check performed.
    /// </para>
    /// </summary>
    public enum SafetyCheckKind
    {
        /// <summary>
        /// <para>
        /// Indicates that the upgrade safety check kind is invalid. This value is not used.
        /// </para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_SAFETY_CHECK_KIND.FABRIC_SAFETY_CHECK_KIND_INVALID,

        /// <summary>
        /// <para>
        /// Indicates that if we bring down the node then this will result in global seed node quorum loss.
        /// </para>
        /// </summary>
        EnsureSeedNodeQuorum = NativeTypes.FABRIC_SAFETY_CHECK_KIND.FABRIC_SEED_NODE_SAFETY_CHECK_KIND_ENSURE_QUORUM,

        /// <summary>
        /// <para>
        /// Indicates that there is some partition for which if we bring down the replica on the node, it will result in quorum loss for that partition.
        /// </para>
        /// </summary>
        EnsurePartitionQuorum = NativeTypes.FABRIC_SAFETY_CHECK_KIND.FABRIC_PARTITION_SAFETY_CHECK_KIND_ENSURE_QUORUM,

        /// <summary>
        /// <para>
        /// Indicates that there is some replica on the node that was moved out of this node. Service Fabric is now waiting for the primary to be moved back to this node.
        /// </para>
        /// </summary>
        WaitForPrimaryPlacement = NativeTypes.FABRIC_SAFETY_CHECK_KIND.FABRIC_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_PRIMARY_PLACEMENT,

        /// <summary>
        /// <para>
        /// Indicates that Service Fabric is waiting for a primary replica to be moved out of the node.
        /// </para>
        /// </summary>
        WaitForPrimarySwap = NativeTypes.FABRIC_SAFETY_CHECK_KIND.FABRIC_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_PRIMARY_SWAP,

        /// <summary>
        /// <para>
        /// Indicates that there is some replica on the node that is involved in a reconfiguration and Service Fabric is waiting for the reconfiguration to be complete.
        /// </para>
        /// </summary>
        WaitForReconfiguration = NativeTypes.FABRIC_SAFETY_CHECK_KIND.FABRIC_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_RECONFIGURATION,

        /// <summary>
        /// <para>
        /// Indicates that there is either a replica on the node that is going through copy, or there is a primary replica on the node that is copying data to some 
        /// other replica. In both cases, bringing down the replica on the node will abort the copy.
        /// </para>
        /// </summary>
        WaitForInBuildReplica = NativeTypes.FABRIC_SAFETY_CHECK_KIND.FABRIC_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_INBUILD_REPLICA,

        /// <summary>
        /// <para>
        /// Indicates that there is either a stateless service partition on the node having exactly one instance, or there is a primary replica on the node for 
        /// which the partition is quorum loss. In both cases, bringing down the replica will result in loss of availability.
        /// </para>
        /// </summary>
        EnsureAvailability = NativeTypes.FABRIC_SAFETY_CHECK_KIND.FABRIC_PARTITION_SAFETY_CHECK_KIND_ENSURE_AVAILABILITY
    }
}
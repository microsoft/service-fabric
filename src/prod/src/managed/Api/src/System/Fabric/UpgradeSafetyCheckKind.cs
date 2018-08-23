// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates the <see cref="System.Fabric.UpgradeSafetyCheck" /> that is being performed for a node during upgrade.</para>
    /// </summary>
    public enum UpgradeSafetyCheckKind
    {
        /// <summary>
        /// <para>Indicates that the upgrade safety check kind is invalid. This value is not used.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_UPGRADE_SAFETY_CHECK_KIND.FABRIC_UPGRADE_SAFETY_CHECK_KIND_INVALID,
        
        /// <summary>
        /// <para>Indicates that if we bring down the node then this will result in global seed node quorum loss.</para>
        /// </summary>
        EnsureSeedNodeQuorum = NativeTypes.FABRIC_UPGRADE_SAFETY_CHECK_KIND.FABRIC_UPGRADE_SEED_NODE_SAFETY_CHECK_KIND_ENSURE_QUORUM,
        
        /// <summary>
        /// <para>Indicates that there is some partition for which if we bring down the replica on the node, it will result in quorum loss for that partition.</para>
        /// </summary>
        EnsurePartitionQuorum = NativeTypes.FABRIC_UPGRADE_SAFETY_CHECK_KIND.FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_ENSURE_QUORUM,
        
        /// <summary>
        /// <para>Indicates that there is some replica on the node that was moved out of this node due to upgrade. Service Fabric is now waiting for the primary 
        /// to be moved back to this node.</para>
        /// </summary>
        WaitForPrimaryPlacement = NativeTypes.FABRIC_UPGRADE_SAFETY_CHECK_KIND.FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_PRIMARY_PLACEMENT,
        
        /// <summary>
        /// <para>Indicates that Service Fabric is waiting for a primary replica to be moved out of the node before starting upgrade on that node.</para>
        /// </summary>
        WaitForPrimarySwap = NativeTypes.FABRIC_UPGRADE_SAFETY_CHECK_KIND.FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_PRIMARY_SWAP,
        
        /// <summary>
        /// <para>Indicates that there is some replica on the node that is involved in a reconfiguration. Service Fabric is waiting for the reconfiguration to 
        /// be complete before staring upgrade on that node.</para>
        /// </summary>
        WaitForReconfiguration = NativeTypes.FABRIC_UPGRADE_SAFETY_CHECK_KIND.FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_RECONFIGURATION,
        
        /// <summary>
        /// <para>Indicates that there is either a replica on the node that is going through copy, or there is a primary replica on the node that is copying 
        /// data to some other replica. In both cases, bringing down the replica on the node due to upgrade will abort the copy.</para>
        /// </summary>
        WaitForInbuildReplica = NativeTypes.FABRIC_UPGRADE_SAFETY_CHECK_KIND.FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_INBUILD_REPLICA,
        
        /// <summary>
        /// <para>Indicates that there is either a stateless service partition on the node having exactly one instance, or there is a primary replica on the 
        /// node for which the partition is quorum loss. In both cases, bringing down the replicas due to upgrade will result in loss of availability.</para>
        /// </summary>
        EnsureAvailability = NativeTypes.FABRIC_UPGRADE_SAFETY_CHECK_KIND.FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_ENSURE_AVAILABILITY,

        /// <summary>
        /// <para>Indicates that there is still not enough capacity for resource governed metrics to proceed with the upgrade. This can happen in case a service 
        /// package is increasing resource consumption. Service Fabric is making actions to ensure that the node has enough capacity. </para>
        /// </summary>
        WaitForResourceAvailability = NativeTypes.FABRIC_UPGRADE_SAFETY_CHECK_KIND.FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_RESOURCE_AVAILABILITY
    }
}
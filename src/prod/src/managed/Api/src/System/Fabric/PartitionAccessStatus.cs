// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates the access status of the partition. </para>
    /// </summary>
    /// <remarks>
    /// <para>
    ///     <see cref="System.Fabric.PartitionAccessStatus" /> is used to check that a read or write operation is allowed. When service 
    /// replicas handle a client request, they should verify that the system is in a state that allows processing. By checking the 
    /// <see cref="System.Fabric.IStatefulServicePartition.ReadStatus" /> or <see cref="System.Fabric.IStatefulServicePartition.WriteStatus" /> as 
    /// appropriate, the replica can be notified of conditions that prevent correct operation. Note that write operations might still see an 
    /// exception from the replicator for one of these conditions, because the condition might change between the 
    /// <see cref="System.Fabric.IStatefulServicePartition.WriteStatus" /> check and the call 
    /// to <see cref="System.Fabric.IStateReplicator.ReplicateAsync(System.Fabric.OperationData,System.Threading.CancellationToken,out System.Int64)" />. </para>
    /// </remarks>
    public enum PartitionAccessStatus
    {
        /// <summary>
        /// <para>Indicates that the read or write operation access status is not valid. This value is not returned to the caller.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS.FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID,
        
        /// <summary>
        /// <para>Indicates that the read or write operation access is granted and the operation is allowed. </para>
        /// </summary>
        Granted = NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS.FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED,
        
        /// <summary>
        /// <para>Indicates that the client should try again later, because a reconfiguration is in progress. After the reconfiguration is completed, a 
        /// new status is returned that gives further instructions. The client should retry the operation at this replica</para>
        /// </summary>
        ReconfigurationPending = NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS.FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING,
        
        /// <summary>
        /// <para>Indicates that this client request was received by a replica that is not a Primary replica. The read or write operation cannot be 
        /// performed at this replica. The client should attempt to use the naming service to identify the correct primary replica.</para>
        /// </summary>
        NotPrimary = NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS.FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY,
        
        /// <summary>
        /// <para>Indicates that no write quorum is available and, therefore, no write operation can be accepted. The client should retry the operation at this replica.</para>
        /// </summary>
        NoWriteQuorum = NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS.FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NO_WRITE_QUORUM
    }
}
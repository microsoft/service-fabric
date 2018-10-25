// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specifies the service partition status.</para>
    /// </summary>
    public enum ServicePartitionStatus
    {
        /// <summary>
        /// <para>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_QUERY_SERVICE_PARTITION_STATUS.FABRIC_QUERY_SERVICE_PARTITION_STATUS_INVALID,
        
        /// <summary>
        /// <para>
        ///     Partition is ready.</para>
        /// <para>
        ///     For stateless services there is one up replica</para>
        /// <para>
        ///     For stateful services the number of ready replicas is greater than or equal to the <see cref="System.Fabric.Description.StatefulServiceDescription.MinReplicaSetSize"/></para>
        /// </summary>
        Ready = NativeTypes.FABRIC_QUERY_SERVICE_PARTITION_STATUS.FABRIC_QUERY_SERVICE_PARTITION_STATUS_READY,
        
        /// <summary>
        /// <para>Partition is not ready. This is returned when none of the other states apply.</para>
        /// </summary>
        NotReady = NativeTypes.FABRIC_QUERY_SERVICE_PARTITION_STATUS.FABRIC_QUERY_SERVICE_PARTITION_STATUS_NOT_READY,
        
        /// <summary>
        /// <para>Partition is in quorum loss. This means that less than MinReplicaSetSize number of replicas are participating in quorum.</para>
        /// </summary>
        InQuorumLoss = NativeTypes.FABRIC_QUERY_SERVICE_PARTITION_STATUS.FABRIC_QUERY_SERVICE_PARTITION_STATUS_IN_QUORUM_LOSS,
        
        /// <summary>
        /// <para>Partition is undergoing a reconfiguration.</para>
        /// </summary>
        Reconfiguring = NativeTypes.FABRIC_QUERY_SERVICE_PARTITION_STATUS.FABRIC_QUERY_SERVICE_PARTITION_STATUS_RECONFIGURING,
        
        /// <summary>
        /// <para>Partition is getting deleted.</para>
        /// </summary>
        Deleting = NativeTypes.FABRIC_QUERY_SERVICE_PARTITION_STATUS.FABRIC_QUERY_SERVICE_PARTITION_STATUS_DELETING
    }
}
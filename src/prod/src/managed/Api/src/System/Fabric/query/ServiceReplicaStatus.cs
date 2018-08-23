// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specifies the status of the replica.</para>
    /// </summary>
    public enum ServiceReplicaStatus
    {
        /// <summary>
        /// <para>Invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_QUERY_SERVICE_REPLICA_STATUS.FABRIC_QUERY_SERVICE_REPLICA_STATUS_INVALID,
        
        /// <summary>
        /// <para>Replica is being built. This means that a primary replica is seeding this replica.</para>
        /// </summary>
        InBuild = NativeTypes.FABRIC_QUERY_SERVICE_REPLICA_STATUS.FABRIC_QUERY_SERVICE_REPLICA_STATUS_INBUILD,

        /// <summary>
        /// <para>
        /// The replica has restarted and it is being kept as a standby.
        /// </para>
        /// </summary>
        Standby = NativeTypes.FABRIC_QUERY_SERVICE_REPLICA_STATUS.FABRIC_QUERY_SERVICE_REPLICA_STATUS_STANDBY,

        /// <summary>
        /// <para>Replica is ready.</para>
        /// </summary>
        Ready = NativeTypes.FABRIC_QUERY_SERVICE_REPLICA_STATUS.FABRIC_QUERY_SERVICE_REPLICA_STATUS_READY,
        
        /// <summary>
        /// <para>Replica is down.</para>
        /// </summary>
        Down = NativeTypes.FABRIC_QUERY_SERVICE_REPLICA_STATUS.FABRIC_QUERY_SERVICE_REPLICA_STATUS_DOWN,
        
        /// <summary>
        /// <para>Replica is dropped. This means that the replica has been removed from the replica set. If it is persisted, 
        /// its state has been deleted.</para>
        /// </summary>
        Dropped = NativeTypes.FABRIC_QUERY_SERVICE_REPLICA_STATUS.FABRIC_QUERY_SERVICE_REPLICA_STATUS_DROPPED
    }

    /// <summary>
    /// <para>Enumerates the filter used for matching the replica status for replicas that should be returned by query.</para>
    /// </summary>
    [Flags]
    public enum ServiceReplicaStatusFilter
    {
        /// <summary>
        /// <para>Returns all replicas other than dropped replicas.</para>
        /// </summary>
        Default = NativeTypes.FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER.FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_DEFAULT,
        
        /// <summary>
        /// <para>Returns all replicas.</para>
        /// </summary>
        All = NativeTypes.FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER.FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_ALL,
        
        /// <summary>
        /// <para>Returns all InBuild replicas.</para>
        /// </summary>
        InBuild = NativeTypes.FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER.FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_INBUILD,
        
        /// <summary>
        /// <para>Returns all StandBy replicas.</para>
        /// </summary>
        Standby = NativeTypes.FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER.FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_STANDBY,
        
        /// <summary>
        /// <para>Returns only ready replicas.</para>
        /// </summary>
        Ready = NativeTypes.FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER.FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_READY,
        
        /// <summary>
        /// <para>Returns all down replicas.</para>
        /// </summary>
        Down = NativeTypes.FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER.FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_DOWN,
        
        /// <summary>
        /// <para>Returns all Dropped replicas.</para>
        /// </summary>
        Dropped = NativeTypes.FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER.FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_DROPPED
    }
}
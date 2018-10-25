// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Indicates the role of a stateful service replica. </para>
    /// </summary>
    /// <remarks>
    /// <para>Service Fabric requires different behaviors from a service replica depending on what role it currently performs.</para>
    /// </remarks>
    public enum ReplicaRole
    {
        /// <summary>
        /// <para>Indicates the initial role that a replica is created in.</para>
        /// </summary>
        Unknown = NativeTypes.FABRIC_REPLICA_ROLE.FABRIC_REPLICA_ROLE_UNKNOWN,

        /// <summary>
        /// <para>Specifies that the replica has no responsibility in regard to the replica set.</para>
        /// </summary>
        /// <remarks>
        /// <para>When 
        /// <see cref="System.Fabric.IStatefulServiceReplica.ChangeRoleAsync(System.Fabric.ReplicaRole,System.Threading.CancellationToken)" /> indicates 
        /// this role, it is safe to delete any persistent state that is associated with this replica.</para>
        /// </remarks>
        None = NativeTypes.FABRIC_REPLICA_ROLE.FABRIC_REPLICA_ROLE_NONE,

        /// <summary>
        /// <para>Refers to the replica in the set on which all read and write operations are complete in order to enforce strong consistency semantics. 
        /// Read operations are handled directly by the Primary replica, while write operations must be acknowledged by a quorum of the replicas in the 
        /// replica set. There can only be one Primary replica in a replica set at a time. </para>
        /// </summary>
        Primary = NativeTypes.FABRIC_REPLICA_ROLE.FABRIC_REPLICA_ROLE_PRIMARY,

        /// <summary>
        /// <para>Refers to a replica in the set that receives a state transfer from the Primary replica to prepare for becoming an active Secondary replica. 
        /// There can be multiple Idle Secondary replicas in a replica set at a time. Idle Secondary replicas do not count as a part of a write quorum. </para>
        /// </summary>
        IdleSecondary = NativeTypes.FABRIC_REPLICA_ROLE.FABRIC_REPLICA_ROLE_IDLE_SECONDARY,

        /// <summary>
        /// <para>Refers to a replica in the set that receives state updates from the Primary replica, applies them, and sends acknowledgements back. 
        /// Secondary replicas must participate in the write quorum for a replica set. There can be multiple active Secondary replicas in a replica set 
        /// at a time. The number of active Secondary replicas is configurable that the reliability subsystem should maintain.  </para>
        /// </summary>
        ActiveSecondary = NativeTypes.FABRIC_REPLICA_ROLE.FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY
    }
}
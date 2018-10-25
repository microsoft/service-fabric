// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    /// <summary>
    /// <para>Describes the data that is obtained from the state replicator. </para>
    /// </summary>
    /// <remarks>
    /// <para>
    ///     <see cref="System.Fabric.IOperation" /> is the base interface that describes state changes that are delivered to a Secondary replica. </para>
    /// <para>
    ///     They contain the <see cref="System.Fabric.IStateReplicator.ReplicateAsync(System.Fabric.OperationData,System.Threading.CancellationToken,out System.Int64)" /> and the sequence number and other identifying information.</para>
    /// </remarks>
    public interface IOperation
    {
        /// <summary>
        /// <para>Gets the type of this operation. </para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.OperationType" />.</para>
        /// </value>
        /// <remarks>
        /// <para>The <see cref="System.Fabric.OperationType" /> indicates the type of operation. "Normal" operations are those operations that are sent by non-service grouped services as part of either the copy or replication streams. Other types of operations represent control operations that are specific to service groups.</para>
        /// </remarks>
        OperationType OperationType { get; }

        /// <summary>
        /// <para>Gets the sequence number of this operation. </para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Int64" />.</para>
        /// </value>
        /// <remarks>
        /// <para>
        ///     The sequence number is provided as a part of the <see cref="System.Fabric.IOperation.SequenceNumber" /></para>
        /// <para>
        ///     For operations received from the replication stream (<see cref="System.Fabric.IStateReplicator.GetReplicationStream"/>) the sequence number is the same that the Primary replica that are receives from <see cref="System.Fabric.IStateReplicator.ReplicateAsync(System.Fabric.OperationData,System.Threading.CancellationToken,out System.Int64)" /> method.</para>
        /// </remarks>
        long SequenceNumber { get; }

        /// <summary>
        /// <para>Identifies the atomic group, if this object that implements <see cref="System.Fabric.IOperation" />
        /// is a part of an atomic group. Atomic groups are only available when a service is a part of service group.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Int64" />.</para>
        /// </value>
        long AtomicGroupId { get; }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.OperationData" /> that are provided by the Primary replica.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.OperationData" />.</para>
        /// </value>
        /// <remarks>
        ///   <para />
        /// </remarks>
        OperationData Data { get; }

        /// <summary>
        /// <para>Acknowledges that this operation has been successfully applied at the Secondary replica.  </para>
        /// </summary>
        /// <remarks>
        /// <para>Services should call this method when they have obtained an <see cref="System.Fabric.IOperation" />
        /// from the replicator and successfully applied it to their local store.
        /// For persisted services, calling this method is mandatory because the <see cref="System.Fabric.FabricReplicator" />
        /// does not release additional objects that implement <see cref="System.Fabric.IOperation" />. For volatile services, the replicator
        /// implicitly acknowledges operations when they are received unless they are configured otherwise by setting the value
        /// <see cref="System.Fabric.ReplicatorSettings.RequireServiceAck" /> to true.
        /// An operation must be acknowledged by a quorum of replicas before the Primary replica receives the
        /// <see cref="System.Fabric.IStateReplicator.ReplicateAsync(System.Fabric.OperationData,System.Threading.CancellationToken,out System.Int64)" /> operation complete responses.</para>
        /// </remarks>
        void Acknowledge();
    }
}
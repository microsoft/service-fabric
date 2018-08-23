// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// <para>Defines the behavior that a service must implement to interact with the <see cref="System.Fabric.FabricReplicator" />.</para>
    /// </summary>
    public interface IStateProvider
    {
        /// <summary>
        /// <para>Obtains the last sequence number that the service has committed, also known as Logical Sequence Number (LSN). </para>
        /// </summary>
        /// <returns>
        /// <para>Returns the last committed sequence number.</para>
        /// </returns>
        long GetLastCommittedSequenceNumber();

        /// <summary>
        /// <para>Indicates to a replica that the configuration of a replica set has changed due to a change or attempted change to the primary replica. 
        /// The change occurs due to failure or load balancing of the previous primary replica. Epoch changes act as a barrier by segmenting operations 
        /// into the exact configuration periods in which they were sent by a specific primary replica.</para>
        /// </summary>
        /// <param name="epoch">
        /// <para>The new <see cref="System.Fabric.Epoch" />.</para>
        /// </param>
        /// <param name="previousEpochLastSequenceNumber">
        /// <para> The maximum sequence number (LSN) in the previous epoch.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. It can be used to send a notification 
        /// that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Threading.Tasks.Task" />.</para>
        /// </returns>
        /// <remarks>
        /// <para>The information in the <see cref="System.Fabric.IStateProvider.UpdateEpochAsync(System.Fabric.Epoch,System.Int64,System.Threading.CancellationToken)" /> 
        /// method enables the service to maintain a progress vector, which is a list of each epoch that the replica has received, and the maximum LSN that they contained. </para>
        /// <para>
        ///     The progress vector data along with the current applied maximum LSN is useful for a secondary replica to send during the copy operation to describe the state of the replica.</para> 
        /// <para>
        ///     Comparing progress vectors that are received from secondary replicas during the copy operation enables primary replicas to determine whether the secondary replica is up-to-date, 
        ///     what state must be sent to the secondary replica, and whether the secondary replica has made false progress. </para>
        /// <para>False progress means that a secondary replica reports an LSN in a previous epoch was greater than the LSN that the primary replica has in its progress vector. </para>
        /// </remarks>
        Task UpdateEpochAsync(Epoch epoch, long previousEpochLastSequenceNumber, CancellationToken cancellationToken);

        /// <summary>
        /// <para>Indicates that a write quorum of replicas in this replica set has been lost, and that therefore data loss might have occurred. 
        /// The replica set consists of a majority of replicas, which includes the primary replica. </para>
        /// </summary>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. 
        /// It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Threading.Tasks.Task{T}" /> of type <see cref="System.Boolean" />, that indicates whether the state provider as part of processing this notification has changed its state</para>
        /// </returns>
        /// <remarks>
        /// <para>When the Service Fabric runtime observes the failure of a quorum of replicas, which includes the primary replica, 
        /// it elects a new primary replica and immediately calls this method on the new primary replica. A primary replica that is informed of possible data loss
        /// can choose to restore its state from some external data source or can continue to run with the state that it currently has. If the service continues to run with its current state,
        /// it should return false from this method, which indicates that no state change has been made. If it has restored or altered its state,
        /// such as rolling back incomplete work, it should return true. If true is returned, then the state in other replicas must be assumed to be incorrect.
        /// Therefore, the Service Fabric runtime removes the other replicas from the replica set and recreates them.</para>
        /// </remarks>
        [SuppressMessage(FxCop.Category.MSInternal, FxCop.Rule.UseApprovedGenericsForPrecompiledAssemblies, Justification = "Not precompiled assembly.")]
        Task<bool> OnDataLossAsync(CancellationToken cancellationToken);

        /// <summary>
        /// <para>Obtains context on a secondary replica after it is created and opened to send context to the primary replica.</para>
        /// </summary>
        /// <returns>
        /// <para>Returns <see cref="System.Fabric.IOperationDataStream" />.</para>
        /// </returns>
        /// <remarks>
        /// <para>The primary replica analyzes the context and sends back state via <see cref="System.Fabric.IStateProvider.GetCopyState(System.Int64,System.Fabric.IOperationDataStream)" />.</para>
        /// <para>
        ///     <see cref="System.Fabric.IStateProvider.GetCopyContext" /> is called on newly created, idle secondary replicas and provides
        ///     a mechanism to asynchronously establish a bidirectional conversation with the primary replica. The secondary replica sends <see cref="System.Fabric.OperationData" />
        ///     objects with which the primary replica can determine the progress of collecting context on the secondary replica. The primary replica responds by sending the required state back.
        ///     See <see cref="System.Fabric.IStateProvider.GetCopyState(System.Int64,System.Fabric.IOperationDataStream)" /> at the primary replica for the other half of the exchange. </para>
        /// <para>For in-memory services, the <see cref="System.Fabric.IStateProvider.GetCopyContext" /> method is not called, 
        /// as the state of the secondary replicas is known (they are empty and will require all of the state).</para>
        /// </remarks>
        IOperationDataStream GetCopyContext();

        /// <summary>
        /// <para>Obtains state on a primary replica that is required to build a secondary replica.</para>
        /// </summary>
        /// <param name="upToSequenceNumber">
        /// <para>The maximum last sequence number that should be placed in the copy stream via the <see cref="System.Fabric.IStateReplicator.GetCopyStream" /> method.
        /// LSNs greater than this number are delivered to the secondary replica as a part of the replication stream via the <see cref="System.Fabric.IStateReplicator.GetReplicationStream" /> method.</para>
        /// </param>
        /// <param name="copyContext">
        /// <para>An <see cref="System.Fabric.IOperationDataStream" /> that contains the <see cref="System.Fabric.OperationData" /> objects that are created by the secondary replica. </para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Fabric.IOperationDataStream" />.</para>
        /// </returns>
        /// <remarks>
        /// <para>Just as <see cref="System.Fabric.IStateProvider.GetCopyContext" /> enables the secondary replica to send context to the primary replica via
        /// an <see cref="System.Fabric.IOperationDataStream" />, <see cref="System.Fabric.IStateProvider.GetCopyState(System.Int64,System.Fabric.IOperationDataStream)" /> enables the primary replica to respond with an <see cref="System.Fabric.IOperationDataStream" />. The stream contains objects that are delivered to the secondary replica via the <see cref="System.Fabric.IStateReplicator.GetCopyStream" /> method of the <see cref="System.Fabric.FabricReplicator" /> class. The objects implement <see cref="System.Fabric.IOperation" /> and contain the specified data. </para>
        /// <para> When the primary replica receives this call, it should create and return another <see cref="System.Fabric.IOperationDataStream" /> 
        /// that contains <see cref="System.Fabric.OperationData" />. <see cref="System.Fabric.OperationData" /> represents the data/state that the secondary replica
        /// requires to catch up to the provided <paramref name="upToSequenceNumber" /> maximum LSN. 
        /// How much and which state has to be sent can be determined via the context information that the secondary replica provides via 
        /// <see cref="System.Fabric.IStateProvider.GetCopyContext"/> method.</para>
        /// </remarks>
        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1305:FieldNamesMustNotUseHungarianNotation", Justification = "Not Hungarian notation.")]
        IOperationDataStream GetCopyState(long upToSequenceNumber, IOperationDataStream copyContext);
    }
}
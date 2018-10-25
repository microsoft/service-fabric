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
    /// <para>Exposes replication-related functions of the <see cref="System.Fabric.FabricReplicator" /> class that are used by <see cref="System.Fabric.IStateProvider" /> to replicate state to ensure high availability. </para>
    /// </summary>
    public interface IStateReplicator
    {
        /// <summary>
        /// <para>Replicates state changes from Primary replica to the Secondary replicas and receives a quorum acknowledgement that those state changes have been applied.</para>
        /// </summary>
        /// <param name="operationData">
        /// <para>The <see cref="System.Fabric.OperationData" /> that represents the state change that the Primary replica wants to replicate.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para> A write quorum of replicas that have been lost. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <param name="sequenceNumber">
        /// <para>Long, the LSN of the operation. Note that this is the same value which is returned by the task. Providing it as an out parameter is useful for services which want to prepare the local write to commit when the task finishes.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Threading.Tasks.Task{T}" /> of type long, the LSN of the operation.</para>
        /// </returns>
        /// <exception cref="System.ArgumentException">
        /// <para>Caused by one of the following:</para>
        /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricTransientException">
        /// <para>
        ///     <see cref="System.Fabric.FabricTransientException" /> is a retriable exception. It is caused by one of the following;</para>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode.NoWriteQuorum" /> is returned when the replicator does not currently have write quorum..</para>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ReconfigurationPending" /> is returned when the replicator has a pending reconfiguration.</para>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ReplicationQueueFull" /> is returned when the replicator queue is full.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">
        /// <para>
        ///     <see cref="System.Fabric.FabricNotPrimaryException" /> is caused by one of the following;</para>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode.NotPrimary" /> is returned when the replicator has a pending reconfiguration.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// <para>
        ///     <see cref="System.Fabric.FabricObjectClosedException" /> is caused by one of the following;</para>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ObjectClosed" /> is returned when the replicator has been closed.</para>
        /// </exception>
        /// <exception cref="System.OperationCanceledException">
        /// <para>
        ///     <see cref="System.OperationCanceledException" /> is caused by one of the following;</para>
        /// <para>E_ABORT when replicator cancels an inflight replication operation.</para>
        /// </exception>
        /// <remarks>
        /// <para>Replication at the Primary replica produces the objects that implement <see cref="System.Fabric.IOperation" /> that the Secondary replica obtains from the Replication Stream via <see cref="System.Fabric.IStateReplicator.GetReplicationStream" />, which is followed by <see cref="System.Fabric.IOperationStream.GetOperationAsync(System.Threading.CancellationToken)" />. </para>
        /// <para>The Primary replica has many duties that are related to process state updates. The following steps show the general sequence of events that a Primary replica must handle to replicate and acknowledge a change. </para>
        /// <para>Part 1: Handling incoming requests: Receive request: Write(x) – Service receives a write request, x. CheckArguments – The service checks the arguments of the request. This check helps ensure the consistency of the service’s state.</para>
        /// <para>Check current state – The service examines its current state to ensure that the operation is valid and can or should be performed. This check also helps ensure data consistency. It is performed by the service code.</para>
        /// <para>Acquire Locks – The service should acquire the necessary locks to prevent additional operations from occurring at the same time. This operation helps ensure isolation and consistency.</para>
        /// <para>Attempt Operation (optional) – The service can attempt the operation locally. This step reserves and allocates space and performs all the necessary computations. This step includes everything but the actual commit of the result. This operation improves the durability of the operation and makes later failures very unlikely.</para>
        /// <para>Manufacture the OperationData – An <see cref="System.Fabric.OperationData" /> object is the representation of the
        /// Write(x) that was presented to the service. The <see cref="System.Fabric.OperationData" /> object contains the state change to be transferred with acknowledgement from the Primary replica to the Secondary replicas. The data that the service places in the OperationData defines the atomic update that the <see cref="System.Fabric.FabricReplicator" /> transfers to the Secondary replicas. Note that creating of the 
        /// <see cref="System.Fabric.OperationData" /> object requires one or more byte arrays. The service must itself determine and serialize the change in state, and then provide this set of bytes to the FabricReplicator via <see cref="System.Fabric.IStateReplicator.ReplicateAsync(System.Fabric.OperationData,System.Threading.CancellationToken,out System.Int64)" />. The service sends the operation to the FabricReplicator and receives a logical sequence number (LSN) in return. The LSN is the identity for the operation and helps both the service and Service Fabric ensure that operations are always applied in the same order everywhere.The service should record the OperationData along with its LSN in an ordered list of in-flight operations. This ensures that when the operations are completed, they can be consistently applied in the correct order.</para>
        /// <para>Release Locks - Continue processing or waiting for further requests.</para>
        /// <para>Part 2: Completing requests and responding: The Primary replica receives a callback that indicates that the operation has been applied. ReplicateAsync is completed. This callback indicates that the operation has been acknowledged by a quorum of the replicas in the replica set. When the Primary replica receives this callback, it should perform the following actions: </para>
        /// <para>Find the corresponding operation that is indicated by the long LSN that is returned from ReplicateAsync in the service’s in-flight list and mark it as "QuorumAck’d". </para>
        /// <para>Now, starting at the first operation in the in-flight list, go through the list and locally commit all of the QuorumAck’d operations, finish any changes to the local state and mark the state changes with their corresponding LSN, until the first incomplete operation is encountered. This ensures that ordering is preserved (consistency) and that the operations are actually applied. This takes advantage of the previous durability and isolation preparations. Note: Most services should cache the last committed LSN value so that responses to the <see cref="System.Fabric.IStateProvider.GetLastCommittedSequenceNumber" /> do not require querying the actual store for the greatest LSN. </para>
        /// <para>When an operation is successfully committed at the Primary replica, the Primary replica can now reply to the client that initiated the call and remove the operation from the in-flight list. Continue to wait for the next quorum-acknowledgment callback.</para>
        /// </remarks>
        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.AvoidOutParameters, Justification = "Approved public API.")]
        Task<long> ReplicateAsync(OperationData operationData, CancellationToken cancellationToken, out long sequenceNumber);

        /// <summary>
        ///   <para>Gets copy stream.</para>
        /// </summary>
        /// <returns>
        /// <para>Returns the copy <see cref="System.Fabric.IOperationStream" />. </para>
        /// </returns>
        /// <exception cref="System.Fabric.FabricTransientException">
        /// <para>
        ///     <see cref="System.Fabric.FabricTransientException" /> is a retriable exception. It is caused by one of the following;</para>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ReconfigurationPending" /> is returned when the replicator has a pending reconfiguration.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// <para>
        ///     <see cref="System.Fabric.FabricObjectClosedException" /> is caused by one of the following;</para>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ObjectClosed" /> is returned when the replicator has been closed.</para>
        /// </exception>
        /// <remarks>
        /// <para>The returned CopyStream is an <see cref="System.Fabric.IOperationStream" /> that contains <see cref="System.Fabric.OperationData" /> objects that implement <see cref="System.Fabric.IOperation" />. The <see cref="System.Fabric.OperationData" /> objects are obtained from the CopyState <see cref="System.Fabric.IOperationDataStream" /> that the Primary replica returns from <see cref="System.Fabric.IStateProvider.GetCopyState(System.Int64,System.Fabric.IOperationDataStream)" />. When a replica is created and has to catch up, it should obtain the CopyStream and begin to send, apply, and acknowledge the Copy objects that implement <see cref="System.Fabric.IOperation" />. In parallel, the replica responds to the corresponding <see cref="System.Fabric.IStateProvider.GetCopyContext" /> and <see cref="System.Fabric.IOperationDataStream.GetNextAsync(System.Threading.CancellationToken)" /> calls. The stream is empty when the returned <see cref="System.Fabric.IOperation" /> method is null.</para>
        /// </remarks>
        IOperationStream GetCopyStream();

        /// <summary>
        ///   <para>Gets replication stream.</para>
        /// </summary>
        /// <returns>
        /// <para>Returns the replication <see cref="System.Fabric.IOperationStream" />.</para>
        /// </returns>
        /// <exception cref="System.Fabric.FabricTransientException">
        /// <para>
        ///     <see cref="System.Fabric.FabricTransientException" /> is a retriable exception. It is caused by one of the following;</para>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ReconfigurationPending" /> is returned when the replicator has a pending reconfiguration.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// <para>
        ///     <see cref="System.Fabric.FabricObjectClosedException" /> is caused by one of the following;</para>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ObjectClosed" /> is returned when the replicator has been closed.</para>
        /// </exception>
        /// <remarks>
        /// <para>The ReplicationStream implements <see cref="System.Fabric.IOperationStream" />. The ReplicationStream contains <see cref="System.Fabric.OperationData" /> objects that implement <see cref="System.Fabric.IOperation" />. The objects are provided by the Primary replica via <see cref="System.Fabric.IStateReplicator.ReplicateAsync(System.Fabric.OperationData,System.Threading.CancellationToken,out System.Int64)" />. Generally a Secondary replica should send <see cref="System.Fabric.IOperationStream.GetOperationAsync(System.Threading.CancellationToken)" />. Although Service Fabric does not require services to do so, generally services should transfer all <see cref="System.Fabric.OperationData" /> objects out of the copy stream first, and then transfer operations out of the replication stream. The transfer from both copies in parallel is supported but increases the complexity of applying state updates correctly and is recommended only for advanced services. The stream is empty when the returned <see cref="System.Fabric.IOperation" /> method is null.</para>
        /// </remarks>
        IOperationStream GetReplicationStream();

        /// <summary>
        /// <para>Enables modification of replicator settings during runtime. The only setting which can be modified is the security credentials. </para>
        /// </summary>
        /// <param name="settings">
        /// <para>The new <see cref="System.Fabric.ReplicatorSettings" /> with the updated credentials.</para>
        /// </param>
        /// <exception cref="System.ArgumentException">
        /// <para>Caused by one of the following:</para>
        /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
        /// </exception>
        void UpdateReplicatorSettings(ReplicatorSettings settings);
    }
}
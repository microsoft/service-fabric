// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// <para>Exposes replication-related functions for atomic groups. </para>
    /// </summary>
    /// <remarks>
    /// <para>The <see cref="System.Fabric.IAtomicGroupStateReplicator" /> is available if the service is a member of a service group. The service must implement <see cref="System.Fabric.IAtomicGroupStateProvider" /> and be stateful. When creating a <see cref="System.Fabric.FabricReplicator" /> via <see cref="System.Fabric.IStatefulServicePartition.CreateReplicator(System.Fabric.IStateProvider,System.Fabric.ReplicatorSettings)" />, instead of passing in a regular <see cref="System.Fabric.IStateProvider" />, the service can pass in the <see cref="System.Fabric.IAtomicGroupStateProvider" /> that it implements instead. As a result, it receives a <see cref="System.Fabric.IAtomicGroupStateReplicator" />.</para>
    /// </remarks>
    public interface IAtomicGroupStateReplicator
    {
        /// <summary>
        /// <para>Creates a new atomic group and obtains the ID of the atomic group.</para>
        /// </summary>
        /// <returns>
        /// <para>Returns <see cref="System.Int64" /> the ID of the created atomic group.</para>
        /// </returns>
        /// <remarks>
        /// <para>Atomic groups are used to coordinate a set of changes across the members of a service group.</para>
        /// </remarks>
        long CreateAtomicGroup();

        /// <summary>
        /// <para>Replicates some <see cref="System.Fabric.OperationData" /> as a part of an atomic group.</para>
        /// </summary>
        /// <param name="atomicGroupId">
        /// <para>The ID of the atomic group that is obtained from <see cref="System.Fabric.IAtomicGroupStateReplicator.CreateAtomicGroup" /> and includes the <see cref="System.Fabric.OperationData" />.</para>
        /// </param>
        /// <param name="operationData">
        /// <para>An <see cref="System.Fabric.OperationData" /> to be replicated.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The CancellationToken object that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <param name="sequenceNumber">
        /// <para>The LSN of the operation, as an out parameter.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Threading.Tasks.Task{T}" /> of type long, the LSN of the replicated atomic group operation.</para>
        /// </returns>
        Task<long> ReplicateAtomicGroupOperationAsync(long atomicGroupId, OperationData operationData, CancellationToken cancellationToken, out long sequenceNumber);

        /// <summary>
        /// <para>Asynchronously commits state replication for the atomic group.</para>
        /// </summary>
        /// <param name="atomicGroupId">
        /// <para>The ID of the group to be committed.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <param name="commitSequenceNumber">
        /// <para>The LSN of the commit operation, as an out parameter.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Threading.Tasks.Task{T}" /> of type long, the LSN of the commit operation.</para>
        /// </returns>
        Task<long> ReplicateAtomicGroupCommitAsync(long atomicGroupId, CancellationToken cancellationToken, out long commitSequenceNumber);

        /// <summary>
        /// <para>Asynchronously rolls-back state replication for the atomic group.</para>
        /// </summary>
        /// <param name="atomicGroupId">
        /// <para>The ID of the atomic group to roll back.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <param name="rollbackSequenceNumber">
        /// <para>The LSN of the rollback operation, as an out parameter.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Threading.Tasks.Task{T}" /> of type long, the LSN of the rollback operation.</para>
        /// </returns>
        Task<long> ReplicateAtomicGroupRollbackAsync(long atomicGroupId, CancellationToken cancellationToken, out long rollbackSequenceNumber);
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// <para>Describes additional methods of the <see cref="System.Fabric.IAtomicGroupStateProvider" /> interface that a user service must implement to take advantage of the atomic group functionality of a service group. </para>
    /// </summary>
    public interface IAtomicGroupStateProvider : IStateProvider
    {
        /// <summary>
        /// <para>Commits a particular atomic group.</para>
        /// </summary>
        /// <param name="atomicGroupId">
        /// <para>The ID of the group to be committed.</para>
        /// </param>
        /// <param name="commitSequenceNumber">
        /// <para>The sequence number for the commit operation.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. 
        /// It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Threading.Tasks.Task" />.</para>
        /// </returns>
        Task AtomicGroupCommitAsync(long atomicGroupId, long commitSequenceNumber, CancellationToken cancellationToken);

        /// <summary>
        /// <para>Rolls back a particular atomic group.</para>
        /// </summary>
        /// <param name="atomicGroupId">
        /// <para>The ID of the group to roll back.</para>
        /// </param>
        /// <param name="rollbackSequenceNumber">
        /// <para>The sequence number for the rollback operation.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Threading.Tasks.Task" />.</para>
        /// </returns>
        Task AtomicGroupRollbackAsync(long atomicGroupId, long rollbackSequenceNumber, CancellationToken cancellationToken);

        /// <summary>
        /// <para>Indicates that progress beyond a particular commit sequence number that is provided via <see cref="System.Fabric.IAtomicGroupStateProvider.AtomicGroupCommitAsync(System.Int64,System.Int64,System.Threading.CancellationToken)" /> should be undone. </para>
        /// </summary>
        /// <param name="fromCommitSequenceNumber">
        /// <para>The LSN of a commit operation. All progress past this point should be undone.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Threading.Tasks.Task" />.</para>
        /// </returns>
        Task AtomicGroupUndoProgressAsync(long fromCommitSequenceNumber, CancellationToken cancellationToken);
    }
}
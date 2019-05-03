// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.StateProviders
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric;
    using System.Fabric.Data.Replicator;

    /// <summary>
    /// Interface implemented by state providers.
    /// </summary>
    public abstract class IReplicatedStateProvider
    {
        /// <summary>
        /// Opens the state provider.
        /// </summary>
        /// <param name="openMode">State provider is either new or existent.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        protected abstract Task OnOpenAsync(ReplicaOpenMode openMode, CancellationToken cancellationToken);

        /// <summary>
        /// Changes the role of the state provider. It is usually called from the stateful service replica.
        /// </summary>
        /// <param name="newRole">New role for the state provider.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        protected abstract Task OnChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken);

        /// <summary>
        /// Closes the state provider gracefully.
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        protected abstract Task OnCloseAsync(CancellationToken cancellationToken);

        /// <summary>
        /// Aborts this state provider.
        /// </summary>
        protected abstract void OnAbort();

        /// <summary>
        /// Called when the recovery stream is available for draining.
        /// </summary>
        protected abstract void OnRecovery();

        /// <summary>
        /// Called when the copy stream is available for draining.
        /// </summary>
        protected abstract void OnCopy();

        /// <summary>
        /// Called when the replication stream is available for draining.
        /// </summary>
        protected abstract void OnReplication();

        /// <summary>
        /// Called when a recovery operation needs to be applied by the state provider.
        /// It is called on the primary and secondary to perform recovery stream drain processing.
        /// </summary>
        /// <param name="operation">Operation to apply.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        protected abstract Task OnRecoveryOperationAsync(IOperationEx operation, CancellationToken cancellationToken);

        /// <summary>
        /// Called when a copy operation needs to be applied by the state provider.
        /// It is called on the secondary to perform copy stream drain processing.
        /// </summary>
        /// <param name="operation">Operation to apply.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        protected abstract Task OnCopyOperationAsync(IOperationEx operation, CancellationToken cancellationToken);

        /// <summary>
        /// Called when a replication operation needs to be applied by the state provider.
        /// It is called on the primary to perform undo stream drain processing and 
        /// on the secondary to perform replication stream drain processing.
        /// </summary>
        /// <param name="operation">Operation to apply.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        protected abstract Task OnReplicationOperationAsync(IOperationEx operation, CancellationToken cancellationToken);

        /// <summary>
        /// Called when an atomic group is committed on the primary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group to commit.</param>
        /// <param name="commitSequenceNumber">Sequence number of the commit operation.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        protected abstract Task OnAtomicGroupCommitAsync(long atomicGroupId, long commitSequenceNumber, CancellationToken cancellationToken);

        /// <summary>
        /// Called when an atomic group is rolled back on the primary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group to abort.</param>
        /// <param name="rollbackSequenceNumber">Sequence number of the rollback operation.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        protected abstract Task OnAtomicGroupRollbackAsync(long atomicGroupId, long rollbackSequenceNumber, CancellationToken cancellationToken);

        /// <summary>
        /// Called when an atomic group is aborted on the primary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group to abort.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        protected abstract Task OnAtomicGroupAbortAsync(long atomicGroupId, CancellationToken cancellationToken);

        /// <summary>
        /// Provides processing of the progress vector.
        /// </summary>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        protected abstract Task OnUpdateEpochAsync(CancellationToken cancellationToken);

        /// <summary>
        /// Method used by the state provider to report its load metrics.
        /// </summary>
        protected abstract void OnReportLoadMetrics();
    }
}
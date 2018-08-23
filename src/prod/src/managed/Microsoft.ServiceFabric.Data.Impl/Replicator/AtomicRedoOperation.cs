// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Fabric;
    using System.Threading.Tasks;

    /// <summary>
    /// An abstraction for a single operation transaction that can never be undone
    /// In the event of having to 'undo' this operation due to extremely rare conditions involving multiple failures in the cluster, along with false progress,
    /// the replicator will end up re-building a replica from scratch.
    /// </summary>
    public class AtomicRedoOperation : TransactionBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AtomicRedoOperation"/> class.
        /// </summary>
        /// <param name="transactionId">
        /// The transaction id.
        /// </param>
        /// <param name="replicator">
        /// The transactional replicator.
        /// </param>
        /// <param name="isPrimaryTransaction">
        /// The is primary transaction.
        /// </param>
        private AtomicRedoOperation(long transactionId, ITransactionalReplicator replicator, bool isPrimaryTransaction)
            : base(transactionId, replicator, isPrimaryTransaction)
        {
        }

        /// <summary>
        /// Adds an operation to the state of the atomic redo operation, that is immediately committed.
        /// One atomic redo operation can contain only 1 operation:- and hence the name.
        /// Atomic redo operations cannot be 'committed' or 'rolled' back explicitly.
        /// </summary>
        /// <param name="metaData">metadata bytes pertaining to the operation.</param>
        /// <param name="redo">Byte stream that contains the 'redo' data when this atomic redo operation is to be committed and the effects of this operation are to be applied</param>
        /// <param name="operationContext"> A user defined context</param>
        /// <param name="stateProviderId">Id of the state provider</param>
        /// <exception cref="FabricNotPrimaryException">
        /// If replica role is not primary
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// If Transaction is not active (Already committed or aborted)
        /// </exception>
        public async Task AddOperationAsync(
            OperationData metaData, 
            OperationData redo, 
            object operationContext, 
            long stateProviderId)
        {
            // Can throw InvalidOperationException.
            // Note that this check protects against an Assert in SM that checks that Apply is only called with transactions that have
            // isPrimaryTransaction set to true.
            this.ThrowIfNotPrimaryTransaction();
            this.ThrowIfNotPrimary();

            StateMachine.OnBeginAtomicOperationAddAsync();

            try
            {
                await TransactionalReplicator.AddOperationAsync(this, metaData, redo, operationContext, stateProviderId).ConfigureAwait(false);
                StateMachine.OnAtomicOperationSuccess();
            }
            catch (Exception e)
            {
                var isRetryable = ProcessException(e);

                // If this is a retryable exception, set the this.TransactionalReplicator so that the operation can be retried by the user
                if (isRetryable == true)
                {
                    StateMachine.OnAtomicOperationRetry();
                    throw;
                }

                StateMachine.OnAtomicOperationFaulted();
                throw;
            }
        }

        /// <summary>
        /// The create atomic redo operation.
        /// </summary>
        /// <param name="transactionId">
        /// The transaction id.
        /// </param>
        /// <param name="isPrimaryTransaction">
        /// The is primary transaction.
        /// </param>
        /// <returns>
        /// The <see cref="AtomicRedoOperation"/>.
        /// </returns>
        internal static AtomicRedoOperation CreateAtomicRedoOperation(long transactionId, bool isPrimaryTransaction)
        {
            Utility.Assert(transactionId < 0, "transactionId < 0");
            return new AtomicRedoOperation(transactionId, null, isPrimaryTransaction);
        }

        /// <summary>
        /// Creates an atomic redo operation
        /// </summary>
        /// <param name="transactionalReplicator"> Transactional Replicator object</param>
        /// <returns> An atomic redo operation</returns>
        internal static AtomicRedoOperation CreateAtomicRedoOperationInternal(
            ITransactionalReplicator transactionalReplicator)
        {
            Utility.Assert(
                transactionalReplicator != null,
                "CreateAtomicRedoOperationInternal: transactionalReplicator is null");

            var isPrimaryTransaction = transactionalReplicator.Role == ReplicaRole.Primary;

            var transactionId = CreateTransactionId();
            return new AtomicRedoOperation(-transactionId, transactionalReplicator, isPrimaryTransaction);
        }
    }
}
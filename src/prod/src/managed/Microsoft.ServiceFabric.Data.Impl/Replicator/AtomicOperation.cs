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
    /// An abstraction for a single operation transaction
    /// </summary>
    public class AtomicOperation : TransactionBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AtomicOperation"/> class.
        /// </summary>
        /// <param name="transactionId">
        /// The transaction id.
        /// </param>
        /// <param name="transactionalReplicator">
        /// The transactional replicator.
        /// </param>
        /// <param name="isPrimaryTransaction">
        /// The is primary transaction.
        /// </param>
        private AtomicOperation(
            long transactionId,
            ITransactionalReplicator transactionalReplicator,
            bool isPrimaryTransaction)
            : base(transactionId, transactionalReplicator, isPrimaryTransaction)
        {
        }

        /// <summary>
        /// Adds an operation to the state of the atomic operation, that is immediately committed
        /// One atomic operation can contain only 1 operation:- and hence the name.
        /// Atomic operations cannot be 'committed' or 'rolled' back explicitly.
        /// </summary>
        /// <param name="metaData">metadata bytes pertaining to the operation.</param>
        /// <param name="undo">Byte stream that contains the 'undo' data when this atomic operation is to be aborted and the effects of this operation are to be undone</param>
        /// <param name="redo">Byte stream that contains the 'redo' data when this atomic operation is to be committed and the effects of this operation are to be applied</param>
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
            OperationData undo, 
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
                await TransactionalReplicator.AddOperationAsync(this, metaData, undo, redo, operationContext, stateProviderId).ConfigureAwait(false);
                StateMachine.OnAtomicOperationSuccess();
            }
            catch (Exception e)
            {
                var isRetryable = ProcessException(e);

                // If this is a retryable exception, set the this.TransactionalReplicator so that the operation can be retried by the user
                if (isRetryable)
                {
                    StateMachine.OnAtomicOperationRetry();
                    throw;
                }

                StateMachine.OnAtomicOperationFaulted();
                throw;
            }
        }

        /// <summary>
        /// The create atomic operation.
        /// </summary>
        /// <param name="transactionId">
        /// The transaction id.
        /// </param>
        /// <param name="isPrimaryTransaction">
        /// The is primary transaction.
        /// </param>
        /// <returns>
        /// The <see cref="AtomicOperation"/>.
        /// </returns>
        internal static AtomicOperation CreateAtomicOperation(long transactionId, bool isPrimaryTransaction)
        {
            Utility.Assert(transactionId < 0, "transactionId < 0");
            return new AtomicOperation(transactionId, null, isPrimaryTransaction);
        }

        /// <summary>
        /// Creates an instance of an atomic operation
        /// </summary>
        /// <param name="transactionalReplicator"> Transactional Replicator object</param>
        /// <returns> An atomic operation</returns>
        internal static AtomicOperation CreateAtomicOperationInternal(ITransactionalReplicator transactionalReplicator)
        {
            Utility.Assert(
                transactionalReplicator != null,
                "CreateAtomicOperationInternal: transactionalReplicator is null");

            var isPrimaryTransaction = transactionalReplicator.Role == ReplicaRole.Primary;

            var transactionId = CreateTransactionId();
            return new AtomicOperation(-transactionId, transactionalReplicator, isPrimaryTransaction);
        }
    }
}
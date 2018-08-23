// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Diagnostics;
    using System.Fabric;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// An abstraction for a set of 'operations' that can be all be committed or reverted atomically 
    /// </summary>
    public class Transaction : TransactionBase, ITransaction
    {
        private Operation firstOperation;

        private long visibilitySequenceNumber;

        /// <summary>
        /// Initializes a new instance of the <see cref="Transaction"/> class.
        /// </summary>
        /// <param name="id">
        /// The id.
        /// </param>
        /// <param name="transactionalReplicator">
        /// The replicator.
        /// </param>
        /// <param name="isPrimaryTransaction">
        /// If this is a primary transaction.
        /// </param>
        internal Transaction(long id, ITransactionalReplicator transactionalReplicator, bool isPrimaryTransaction)
            : base(id, transactionalReplicator, isPrimaryTransaction)
        {
            this.firstOperation = null;
            this.visibilitySequenceNumber = LogicalSequenceNumber.InvalidLsn.LSN;
            this.CommitLatencyWatch = new Stopwatch();
        }

        /// <summary>
        /// Gets the transaction id.
        /// </summary>
        public long TransactionId
        {
            get { return this.Id; }
        }

        /// <summary>
        /// Calculates the time it takes to commit a transaction
        /// This is used by the logging replicator to compute the avg commit latency perf counter
        /// It also accounts for the retry times when queue is full/ other transient errors
        /// </summary>
        internal Stopwatch CommitLatencyWatch
        {
            get; private set;
        }

        /// <summary>
        /// Aborts the transaction.
        /// </summary>
        public void Abort()
        {
            StateMachine.OnUserBeginAbort();
            PrivateAbortAsync().IgnoreExceptionVoid();
        }

        /// <summary>
        /// Rolls back this transaction
        /// </summary>
        /// <returns>A sequence number associated with the abort operation</returns>
        /// <exception cref="TransactionFaultedException">
        /// If the Transaction is internally aborted
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// If Transaction is not active (Already committed or aborted)
        /// </exception>
        /// <exception cref="FabricNotPrimaryException">
        /// If role is not primary
        /// </exception>
        public Task<long> AbortAsync()
        {
            StateMachine.OnUserBeginAbort();
            return PrivateAbortAsync();
        }

        /// <summary>
        /// Adds an operation to the state of the transaction
        /// </summary>
        /// <param name="metaData">metadata bytes pertaining to the operation.</param>
        /// <param name="undo">Byte stream that contains the 'undo' data if this transaction is to be aborted and the effects of this operation are to be undone</param>
        /// <param name="redo">Byte stream that contains the 'redo' data if this transaction is to be committed and the effects of this operation are to be applied</param>
        /// <param name="operationContext"> A user defined context</param>
        /// <param name="stateProviderId">Id of the state provider</param>
        /// <returns>A sequence number associated with this operation</returns>
        /// <exception cref="TransactionFaultedException">
        /// If the Transaction is internally aborted
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// If Transaction is not active (Already committed or aborted)
        /// </exception>
        /// <exception cref="FabricNotPrimaryException">
        /// If replica role is not primary
        /// </exception>
        public void AddOperation(
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext,
            long stateProviderId)
        {
            StateMachine.OnAddOperation(() => PrivateAddOperation(metaData, undo, redo, operationContext, stateProviderId));
        }

        /// <summary>
        /// Commits the transaction
        /// </summary>
        /// <exception cref="TransactionFaultedException">
        /// If the Transaction is internally aborted
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// If Transaction is not active (Already committed or aborted)
        /// </exception>
        /// <exception cref="FabricNotPrimaryException">
        /// If replica role is not primary
        /// </exception>
        public Task CommitAsync()
        {
            StateMachine.OnBeginCommit();
            return PrivateCommitAsync();
        }

        /// <summary>
        /// Returns the current visibility sequence number.
        /// </summary>
        /// <returns>
        /// Task that represents the asynchronous operation.
        /// Result is the visibility sequence number.
        /// </returns>
        public async Task<long> GetVisibilitySequenceNumberAsync()
        {
            StateMachine.OnBeginRead();
            try
            {
                long result = await PrivateGetVisibilitySequenceNumberAsync();
                return result;
            }
            finally
            {
                StateMachine.OnEndRead();
            }
        }

        /// <summary>
        /// Creates an instance of a transaction
        /// </summary>
        /// <param name="transactionalReplicator">Transactional Replicator object</param>
        /// <returns>A transaction</returns>
        internal static Transaction CreateTransactionInternal(ITransactionalReplicator transactionalReplicator)
        {
            Utility.Assert(
                transactionalReplicator != null,
                "CreateTransactionInternal: transactionalReplicator is null");

            // TODO: Consider isPrimaryTransaction only when the first AddOperation is called.
            // MCoskun: Note that we do not want CreateTransaction to throw since most customers use "using" and 
            // if we were to throw, then customers would have to use try, catch, finally instead.
            var isPrimaryTransaction = transactionalReplicator.Role == ReplicaRole.Primary;

            var transactionId = CreateTransactionId();
            var transaction = new Transaction(transactionId, transactionalReplicator, isPrimaryTransaction);

            return transaction;
        }
        
        internal override void Dispose(bool isDisposing)
        {
            if (this.isDisposed)
            {
                return;
            }

            Utility.Assert(this.IsValidTransaction == true, "IsValidTransaction == true");

            if (isDisposing == true) 
            {
                // This should never throw to ensure we continue releasing locks
                Task.Run(() => this.AbortDuringDispose()).IgnoreException();
            }

            base.Dispose(isDisposing);
        }

        internal void AbortBadTransaction()
        {
            StateMachine.OnSystemInternallyAbort();
            PrivateAbortAsync().IgnoreExceptionVoid();
        }

        /// <summary>
        /// Throw exceptions from the V1 replicator and abort the transaction if needed.
        /// Transactions with operations throwing FabricReplicationOperationTooLargeException are aborted
        /// to prevent violation of 2 phase locking
        /// </summary>
        /// <param name="e"></param>
        /// <param name="abortNeeded"></param>
        private void ThrowReplicationException(Exception e, bool abortNeeded)
        {
            int innerHResult = 0;
            var flattenedException = Utility.FlattenException(e, out innerHResult);

            if (flattenedException is FabricReplicationOperationTooLargeException)
            {
                if (abortNeeded)
                {
                    AbortBadTransaction();
                }

                throw new TransactionFaultedException(e.Message);
            }
            else
            {
                throw e;
            }
        }

        /// <summary>
        /// Abort API during disposal of transaction
        /// </summary>
        private Task AbortDuringDispose()
        {
            bool abortNecessary = StateMachine.OnUserDisposeBeginAbort(() =>
                {
                    if (this.visibilitySequenceNumber != LogicalSequenceNumber.InvalidLsn.LSN)
                    {
                        this.TransactionalReplicator.UnRegister(this.visibilitySequenceNumber);
                    }
                }
            );

            if (abortNecessary)
            {
                return PrivateAbortAsync();
            }

            return Task.FromResult(true);
        }

        private async Task<long> PrivateGetVisibilitySequenceNumberAsync()
        {
            // Opportunistic check.
            if (this.visibilitySequenceNumber == LogicalSequenceNumber.InvalidLsn.LSN)
            {
                this.visibilitySequenceNumber = await TransactionalReplicator.RegisterAsync().ConfigureAwait(false);
            }

            return this.visibilitySequenceNumber;
        }

        private void PrivateAddOperation(
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext,
            long stateProviderId)
        {
            this.ThrowIfNotPrimaryOrAtomic();

            // Note that this check protects against an Assert in SM that checks that Apply is only called with transactions that have
            // isPrimaryTransaction set to true.
            this.ThrowIfNotPrimaryTransaction();

            if (this.IsWriteTransaction == false)
            {
                // Before buffering this operation and returning back to the user, perform a sanity check on the state provider name if it is registered
                this.TransactionalReplicator.ThrowIfStateProviderIsNotRegistered(stateProviderId, this.TransactionId);

                this.IsWriteTransaction = true;

                // Cache the first operation and do not send it on the wire as yet
                this.firstOperation = new Operation(metaData, redo, undo, operationContext, stateProviderId);
            }
            else
            {
                try
                {
                    if (this.firstOperation != null)
                    {
                        this.TransactionalReplicator.BeginTransaction(
                            this,
                            this.firstOperation.MetaData,
                            this.firstOperation.Undo,
                            this.firstOperation.Redo,
                            this.firstOperation.Context,
                            this.firstOperation.StateProviderId);

                        // If the above call completes successfully, it indicates that the replication started successfully. Now reset the first operation member variable
                        // In cases where the above call fails, the caller retries this add operation during which we will retry the sending of the first operation
                        this.firstOperation = null;
                    }

                    this.TransactionalReplicator.AddOperation(
                        this,
                        metaData,
                        undo,
                        redo,
                        operationContext,
                        stateProviderId);
                }
                catch(Exception e)
                {
                    // Abort the transaction if specific exceptions are thrown from V1 replicator
                    ThrowReplicationException(e, true);
                }
            }
        }

        private async Task PrivateCommitAsync()
        {
            // MCoskun: Note that was TransactionState has moved to Committing it cannot go back to Active.
            // If this invariant changes, releasing read locks here will cause 2PL to be broken.
            this.ReleaseReadLocks();

            if ((this.IsAtomicOperation == false) && (this.IsWriteTransaction == true))
            {
                this.CommitLatencyWatch.Start();

                do
                {
                    try
                    {
                        if (this.firstOperation != null)
                        {
                            // Commit is invoked after only 1 operation in the transaction.
                            // Convert this to a single operation transaction and avoid replicating a commit record
                            await TransactionalReplicator.BeginTransactionAsync(
                                    this,
                                    this.firstOperation.MetaData,
                                    this.firstOperation.Undo,
                                    this.firstOperation.Redo,
                                    this.firstOperation.Context,
                                    this.firstOperation.StateProviderId).ConfigureAwait(false);
                        }
                        else
                        {
                            await TransactionalReplicator.CommitTransactionAsync(this).ConfigureAwait(false);
                        }

                        StateMachine.OnCommitSuccessful();
                        return;
                    }
                    catch (Exception e)
                    {
                        var isRetryable = ProcessException(e);
                        if (isRetryable == false)
                        {
                            StateMachine.OnCommitFaulted();
                            this.ThrowReplicationException(e, false);
                        }
                    }

                    this.TransactionRetryDelay *= RetryBackoffFactor;
                    if (this.TransactionRetryDelay > MaxRetryDelay)
                    {
                        this.TransactionRetryDelay = MaxRetryDelay;
                    }

                    await Task.Delay(this.TransactionRetryDelay).ConfigureAwait(false);
                } while (true);
            }

            // A readonly transaction or atomic operation is considered committed
            StateMachine.OnCommitSuccessful();
        }

        private async Task<long> PrivateAbortAsync()
        {
            if ((this.IsAtomicOperation == false) && (this.IsWriteTransaction == true))
            {
                return await AbortTransactionAsync().ConfigureAwait(false);
            }

            StateMachine.OnAbortSuccessful();
            return LogicalSequenceNumber.InvalidLsn.LSN;
        }

        private async Task<long> AbortTransactionAsync()
        {
            Utility.Assert(
                (this.IsAtomicOperation == false) && (this.IsWriteTransaction == true), 
                "(this.IsAtomicOperation ({0}) == false) && (this.IsWriteTransaction( {1}) == true)",
                this.IsAtomicOperation, this.IsWriteTransaction);

            // Optimization: If the first operation is not yet replicated and this transaction is aborted we need to replicate the abort.
            // Hence just drop the cached first operation and return to user
            if (this.firstOperation != null)
            {
                if (this.firstOperation.Context != null)
                {
                    try 
                    {
                        TransactionalReplicator.SingleOperationTransactionAbortUnlock(this.firstOperation.StateProviderId, this.firstOperation.Context);
                    }
                    catch(Exception e)
                    {
                        Utility.Assert(false, "DynamicStateManager.SingleOperationTransactionAbortUnlock is not expected to throw any exception. Exception {0}:{1}", e.Message, e.StackTrace);
                    }
                }

                this.firstOperation = null;
                StateMachine.OnAbortSuccessful();
                return LogicalSequenceNumber.InvalidLsn.LSN;
            }

            do
            {
                try
                {
                    var lsn = await TransactionalReplicator.AbortTransactionAsync(this).ConfigureAwait(false);
                    StateMachine.OnAbortSuccessful();
                    return lsn;
                }
                catch (Exception e)
                {
                    var isRetryable = ProcessException(e);
                    if (isRetryable == false)
                    {
                        StateMachine.OnAbortFaulted();
                        throw;
                    }
                }

                this.TransactionRetryDelay *= RetryBackoffFactor;
                if (this.TransactionRetryDelay > MaxRetryDelay)
                {
                    this.TransactionRetryDelay = MaxRetryDelay;
                }

                await Task.Delay(this.TransactionRetryDelay).ConfigureAwait(false);
            } while (true);
        }

        internal class Operation
        {
            public Operation(
                OperationData metaData,
                OperationData redo,
                OperationData undo,
                object context,
                long stateProviderId)
            {
                this.MetaData = metaData;
                this.Redo = redo;
                this.Undo = undo;
                this.Context = context;
                this.StateProviderId = stateProviderId;
            }

            public object Context { get; private set; }

            public OperationData MetaData { get; private set; }

            public OperationData Redo { get; private set; }

            public long StateProviderId { get; private set; }

            public OperationData Undo { get; private set; }
        }
    }
}
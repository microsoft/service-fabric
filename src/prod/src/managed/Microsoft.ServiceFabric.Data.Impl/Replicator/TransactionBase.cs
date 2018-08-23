// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// An abstraction for a set of 'operations' that can be all be committed or reverted atomically 
    /// </summary>
    /// <devnote>
    /// MCoskun: Early Read Lock Release Optimization:
    /// 
    /// Transactional replicator uses two phase locking (2PL) to guarantee serializability.
    /// Early read lock optimization takes advantage of this by releasing all read locks for a commiting transaction 
    /// Once the synchronous part of the CommitAsync is called since once the commit starts, expanding phase is over.
    /// 
    /// For aborting/disposing transactions, read locks are realeased at Dispose time just like write lock contexts.
    /// 
    /// From 2PL perspective write locks can also be released but this would cause one of Barrier invariants to be invalidated:
    /// Between two barriers there are never more than one transaction with conflicting locks/keys/items.
    /// One way of releasing the locks early would be to implemented chained transactions.
    /// 
    /// Important Note: 
    /// Early read lock release optimization takes a dependency that once a Transaction moves out of Active state, 
    /// it will never transition back to being Active.
    /// Breaking this invariant can break 2PL and hence serializability guarantees.
    /// 
    /// Note: Unlike Write LockContext (default), public read, removal and scanning of read lock contexts is not supported.
    /// This can be added in future if needed but these methods were not used at all.
    /// For now, it is preferred to minimize expansion of the api surface.
    /// </devnote>
    public abstract class TransactionBase : IDisposable
    {
        /// <summary>
        /// Flag set to true upon Dispose.  Used to make Dispose idempotent.
        /// </summary>
        protected bool isDisposed = false;

        /// <summary>
        /// Starting retry backoff will be this number * RetryBackoffMultiple.
        /// </summary>
        private const int RetryDelay = 64 / RetryBackoffMultiple;

        /// <summary>
        /// Back off factor during retry
        /// </summary>
        private const int RetryBackoffMultiple = 2;

        /// <summary>
        /// Maximum retry delay interval
        /// </summary>
        private const int MaximumRetryDelay = 4 * 1024;

        /// <summary>
        /// An invalid transaction
        /// </summary>
        private static readonly Transaction InvalidTransactionObject = new Transaction(0, null, false);

        /// <summary>
        /// The latest transaction id
        /// </summary>
        private static long lastTransactionId = DateTime.UtcNow.ToFileTimeUtc();

        /// <summary>
        /// Transaction lock
        /// </summary>
        private static object transactionLock = new object();

        /// <summary>
        /// The transaction Id
        /// </summary>
        private readonly long id;

        /// <summary>
        /// This is a primary transaction
        /// </summary>
        private readonly bool isPrimaryTx;

        private ITransactionalReplicator transactionalReplicator;

        /// <summary>
        /// This is a write transaction
        /// </summary>
        private bool isWriteTransaction;

        /// <summary>
        /// The retry delay.
        /// </summary>
        private int transactionRetryDelay;

        /// <summary>
        /// The commit sequence number.
        /// </summary>
        private long commitSequenceNumber = LogicalSequenceNumber.InvalidLsn.LSN;

        /// <summary>
        /// Contains the list of registered read lock contexts.
        /// </summary>
        private List<LockContext> readLockContexts;

        private TransactionStateMachine stateMachine;

        internal TransactionBase(long identifier, ITransactionalReplicator replicator, bool isPrimaryTransaction)
        {
            this.stateMachine = new TransactionStateMachine(identifier);
            this.id = identifier;
            this.LockContexts = new List<LockContext>();
            this.readLockContexts = new List<LockContext>();
            this.isPrimaryTx = isPrimaryTransaction;
            this.transactionalReplicator = replicator;
            this.TransactionRetryDelay = RetryDelay;

            this.commitSequenceNumber = LogicalSequenceNumber.InvalidLsn.LSN;
        }

        /// <summary>
        /// Gets the commit sequence number.
        /// For atomic operations, commit sequence number equals the operation sequence number.
        /// </summary>
        public long CommitSequenceNumber
        {
            get { return this.commitSequenceNumber; }

            internal set
            {
                Utility.Assert(
                    this.commitSequenceNumber == LogicalSequenceNumber.InvalidLsn.LSN ||
                    this.commitSequenceNumber == value,
                    "Commit Sequence Number must only be set once or must be set to the same value");

                Utility.Assert(value > 0, "Commit sequence number must be a positive number");

                if (this.commitSequenceNumber != value)
                {
                    this.commitSequenceNumber = value;
                }
            }
        }

        /// <summary>
        /// Gets the transaction id.
        /// </summary>
        public long Id
        {
            get { return this.id; }
        }

        /// <summary>
        /// This is for internal use only.
        /// Gets the lock contexts.
        /// </summary>
        public List<LockContext> LockContexts { get; protected set; }

        /// <summary>
        /// Gets the transaction state.
        /// </summary>
        internal TransactionState State
        {
            get { return stateMachine.State; }
        }

        internal static Transaction InvalidTransaction
        {
            get { return InvalidTransactionObject; }
        }

        /// <summary>
        /// Gets a value indicating whether this is an atomic operation.
        /// </summary>
        internal bool IsAtomicOperation
        {
            get { return this.Id < 0; }
        }

        /// <summary>
        /// Gets a value indicating whether this is a primary transaction.
        /// </summary>
        internal bool IsPrimaryTransaction
        {
            get { return this.isPrimaryTx; }
        }

        /// <summary>
        /// Gets a value indicating whether this is a valid transaction.
        /// </summary>
        internal bool IsValidTransaction
        {
            get { return this.Id != 0; }
        }

        /// <summary>
        /// The transactional replicator.
        /// </summary>
        internal ITransactionalReplicator TransactionalReplicator
        {
            get { return this.transactionalReplicator; }
        }

        /// <summary>
        /// The maximum allowed delay for a retry operation.
        /// </summary>
        protected static int MaxRetryDelay
        {
            get { return MaximumRetryDelay; }
        }

        /// <summary>
        /// When performing backoff-retry, multiply the wait time by this factor.
        /// </summary>
        protected static int RetryBackoffFactor
        {
            get { return RetryBackoffMultiple; }
        }

        /// <summary>
        /// The last generated transaction Id.
        /// </summary>
        protected static long LastTransactionId
        {
            get { return lastTransactionId; }

            set { lastTransactionId = value; }
        }

        /// <summary>
        /// Lock to be taken when generating unique transaction Ids.
        /// </summary>
        protected static object TransactionLock
        {
            get { return transactionLock; }

            set { transactionLock = value; }
        }
        
        /// <summary>
        /// Protects transaction state
        /// </summary>
        protected TransactionStateMachine StateMachine
        {
            get { return stateMachine; }
        }

        /// <summary>
        /// Whether or not this transaction is a write transaction.
        /// </summary>
        protected bool IsWriteTransaction
        {
            get { return this.isWriteTransaction; }

            set { this.isWriteTransaction = value; }
        }

        /// <summary>
        /// The current retry delay used in backoff-retry.  Will increase by a factor of <see cref="RetryBackoffFactor"/> for each additional retry.
        /// </summary>
        protected int TransactionRetryDelay
        {
            get { return this.transactionRetryDelay; }

            set { this.transactionRetryDelay = value; }
        }

        /// <summary>
        /// Determines whether the two Transactions are equal
        /// </summary>
        /// <param name="left">First transaction</param>
        /// <param name="right">Second transaction</param>
        /// <returns>'true' if both the transactions are equal</returns>
        public static bool operator ==(TransactionBase left, TransactionBase right)
        {
            if (ReferenceEquals(left, null) == true)
            {
                return ReferenceEquals(right, null);
            }

            if (ReferenceEquals(right, null) == true)
            {
                return false;
            }

            return (left.Id == right.Id) && (left.IsPrimaryTransaction == right.IsPrimaryTransaction);
        }

        /// <summary>
        /// Determines whether one transaction is semantically 'greater' than the other
        /// </summary>
        /// <param name="left">First transaction</param>
        /// <param name="right">Second transaction</param>
        /// <returns>'true' if 'left'(First) transaction is 'greater' than 'right'(Second) transaction.
        /// 'greater' could mean 'newer', 'larger data' e.t.c
        /// This is an internal implementation detail that must not be relevant to the client</returns>
        public static bool operator >(TransactionBase left, TransactionBase right)
        {
            if (ReferenceEquals(left, null) == true)
            {
                return ReferenceEquals(right, null);
            }

            if (ReferenceEquals(right, null) == true)
            {
                return false;
            }

            return left.Id > right.Id;
        }

        /// <summary>
        /// Determines whether one transaction is semantically 'greater or equal' to the other
        /// </summary>
        /// <param name="left">First transaction</param>
        /// <param name="right">Second transaction</param>
        /// <returns>'true' if 'left'(First) transaction is 'greater or equal' to 'right'(Second) transaction.
        /// 'greater or equal' could mean 'newer', 'larger data' e.t.c
        /// This is an internal implementation detail that must not be relevant to the client</returns>
        public static bool operator >=(TransactionBase left, TransactionBase right)
        {
            return (left.Id < right.Id) == false;
        }

        /// <summary>
        /// Determines whether the 2 Transactions are equal 
        /// </summary>
        /// <param name="left">First transaction</param>
        /// <param name="right">Second transaction</param>
        /// <returns>'true' if the two transactions are not equal</returns>
        public static bool operator !=(TransactionBase left, TransactionBase right)
        {
            return (left == right) == false;
        }

        /// <summary>
        /// Determines whether one transaction is semantically 'lesser' than the other
        /// </summary>
        /// <param name="left">First transaction</param>
        /// <param name="right">Second transaction</param>
        /// <returns>'true' if 'left'(First) transaction is 'lesser' than 'right'(Second) transaction.
        /// 'lesser' could mean 'older', 'smaller data' e.t.c
        /// This is an internal implementation detail that must not be relevant to the client</returns>
        public static bool operator <(TransactionBase left, TransactionBase right)
        {
            if (ReferenceEquals(left, null) == true)
            {
                return ReferenceEquals(right, null);
            }

            if (ReferenceEquals(right, null) == true)
            {
                return false;
            }

            return left.Id < right.Id;
        }

        /// <summary>
        /// Determines whether one transaction is semantically 'lesser or equal' to the other
        /// </summary>
        /// <param name="left">First transaction</param>
        /// <param name="right">Second transaction</param>
        /// <returns>'true' if 'left'(First) transaction is 'lesser or equal' to the 'right'(Second) transaction.
        /// 'lesser or equal' could mean 'older', 'smaller data' e.t.c
        /// This is an internal implementation detail that must not be relevant to the client</returns>
        public static bool operator <=(TransactionBase left, TransactionBase right)
        {
            return (left.Id > right.Id) == false;
        }

        /// <summary>
        /// Adds a lock context.
        /// </summary>
        /// <param name="lockContext">
        /// The lock context.
        /// </param>
        /// <param name="lockMode">
        /// Mode of the lock.
        /// For committing transaction read locks are released before IO.
        /// </param>
        public void AddLockContext(LockContext lockContext, LockContextMode lockMode = LockContextMode.Write)
        {
            if (lockContext == null)
            {
                throw new ArgumentNullException(
                    string.Format(CultureInfo.CurrentCulture, SR.Error_NullArgument_Formatted, "lockContext"));
            }

            if (lockMode == LockContextMode.Write)
            {
                this.LockContexts.Add(lockContext);
            }
            else
            {
                this.AddReadLock(lockContext);
            }
        }

        /// <summary>
        /// Releases all resources used by the Transaction.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Determines whether the specified Transaction is equal to the current transaction
        /// </summary>
        /// <param name="obj">Transaction to check equality with</param>
        /// <returns>'true' if both the transactions are equal</returns>
        public override bool Equals(object obj)
        {
            if ((obj is TransactionBase) == false)
            {
                return false;
            }

            var arg = (TransactionBase) obj;
            return (this.Id == arg.Id) && (this.IsPrimaryTransaction == arg.IsPrimaryTransaction);
        }

        /// <summary>
        /// Determines whether the specified Transaction is equal to the current transaction
        /// </summary>
        /// <param name="transaction">Transaction to check equality with</param>
        /// <returns>'true' if both the transactions are equal</returns>
        public bool Equals(TransactionBase transaction)
        {
            if (ReferenceEquals(transaction, null) == true)
            {
                return false;
            }

            return (this.Id == transaction.Id) && (this.IsPrimaryTransaction == transaction.IsPrimaryTransaction);
        }

        /// <summary>
        /// Serves as a hash function for this type
        /// </summary>
        /// <returns>A hash code for the current object</returns>
        public override int GetHashCode()
        {
            return this.Id.GetHashCode();
        }

        /// <summary>
        /// Finds the lock context in the lock manager
        /// </summary>
        /// <param name="lockManager">Lock manager object from which the lock is to be looked up</param>
        /// <param name="lockOrKey">name of the lock</param>
        /// <returns>LockContext object if found. null otherwise</returns>
        public LockContext LookupLockContext(ILockManager lockManager, object lockOrKey)
        {
            if (lockManager == null)
            {
                throw new ArgumentNullException(
                    string.Format(CultureInfo.CurrentCulture, SR.Error_NullArgument_Formatted, "lockManager"));
            }

            if (lockOrKey == null)
            {
                throw new ArgumentNullException(
                    string.Format(CultureInfo.CurrentCulture, SR.Error_NullArgument_Formatted, "lockOrKey"));
            }

            foreach (var lockContext in this.LockContexts)
            {
                if (lockContext.IsEqual(lockManager, lockOrKey) == true)
                {
                    return lockContext;
                }
            }

            return null;
        }

        /// <summary>
        /// This is for internal use only.
        /// Removes the lock context
        /// </summary>
        /// <param name="lockManager">Lock manager object which contains the lock to be removed</param>
        /// <param name="lockOrKey">name of the lock</param>
        /// <returns>LockContext object if found. null otherwise</returns>
        public LockContext RemoveLockContext(ILockManager lockManager, object lockOrKey)
        {
            if (lockManager == null)
            {
                throw new ArgumentNullException(
                    string.Format(CultureInfo.CurrentCulture, SR.Error_NullArgument_Formatted, "lockManager"));
            }

            if (lockOrKey == null)
            {
                throw new ArgumentNullException(
                    string.Format(CultureInfo.CurrentCulture, SR.Error_NullArgument_Formatted, "lockOrKey"));
            }

            LockContext removedLockContext = null;
            for (var i = 0; i < this.LockContexts.Count; i++)
            {
                var lockContext = this.LockContexts[i];
                if (lockContext.IsEqual(lockManager, lockOrKey) == true)
                {
                    this.LockContexts.RemoveAt(i);
                    removedLockContext = lockContext;
                    break;
                }
            }

            return removedLockContext;
        }

        /// <summary>
        /// This is for internal use only.
        /// Removes the lock context
        /// </summary>
        /// <param name="lockContext">The lock context to be removed</param>
        public void RemoveLockContext(LockContext lockContext)
        {
            if (lockContext == null)
            {
                throw new ArgumentNullException(
                    string.Format(CultureInfo.CurrentCulture, SR.Error_NullArgument_Formatted, "lockContext"));
            }

            for (var i = 0; i < this.LockContexts.Count; i++)
            {
                if (this.LockContexts[i] == lockContext)
                {
                    this.LockContexts.RemoveAt(i);
                }
            }
        }

        /// <summary>
        /// Converts the value of this instance to a string
        /// </summary>
        /// <returns> A string that represents the current object</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "{0}:{1}", this.Id, this.IsPrimaryTransaction);
        }

        /// <summary>
        /// Creates a transaction
        /// </summary>
        /// <param name="transactionId">Transaction identifier</param>
        /// <param name="isPrimaryTransaction">If the transaction is a primary transaction</param>
        /// <returns>An instance of a transaction object</returns>
        internal static TransactionBase CreateTransaction(long transactionId, bool isPrimaryTransaction)
        {
            long positiveTransactionId;
            TransactionBase transaction;
            if (transactionId >= 0)
            {
                positiveTransactionId = transactionId;

                transaction = new Transaction(transactionId, null, isPrimaryTransaction);
            }
            else
            {
                positiveTransactionId = -transactionId;
                transaction = AtomicOperation.CreateAtomicOperation(transactionId, isPrimaryTransaction);
            }

            lock (TransactionLock)
            {
                if (positiveTransactionId > LastTransactionId)
                {
                    LastTransactionId = positiveTransactionId;
                }
            }

            return transaction;
        }

        /// <summary>
        /// Creates a transaction identifier
        /// </summary>
        /// <returns> A number representing the transaction id</returns>
        internal static long CreateTransactionId()
        {
            var transactionId = DateTime.UtcNow.ToFileTimeUtc();
            lock (TransactionLock)
            {
                if (transactionId > LastTransactionId)
                {
                    LastTransactionId = transactionId;
                }
                else
                {
                    ++LastTransactionId;
                    transactionId = LastTransactionId;
                }
            }

            return transactionId;
        }

        /// <summary>
        /// Releases the resources associated with the transaction
        /// </summary>
        /// <param name="isDisposing">'true' if this transaction is disposing</param>
        /// <remarks>
        /// Since the class does not have any unmanaged resources, Finalize is not implemented.
        /// This means that this method will never be called with isDispose:false.
        /// </remarks>
        internal virtual void Dispose(bool isDisposing)
        {
            if (this.isDisposed)
            {
                return;
            }

            Utility.Assert(this.IsValidTransaction == true, "IsValidTransaction == true");

            // Lock release order is not relevant for correctness.
            // Releasing read locks first just for consistency.
            this.ReleaseReadLocks();

            if (this.LockContexts != null)
            {
                foreach (var context in this.LockContexts)
                {
                    if (context.IsTrackingContext == false)
                    {
                        context.Unlock(LockContextMode.Write);
                    }
                }
            }

            this.isDisposed = true;
        }

        /// <summary>
        /// The throw if not primary or atomic.
        /// </summary>
        /// <exception cref="FabricNotPrimaryException">
        /// If role is not primary
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// If this is an atomic operation
        /// </exception>
        internal void ThrowIfNotPrimaryOrAtomic()
        {
            this.ThrowIfNotPrimary();

            if (this.IsAtomicOperation == true)
            {
                throw new InvalidOperationException(SR.Error_V2ReplInvalidAPIOnAtomicOperation);
            }
        }

        internal void ThrowIfTransactionIsNotActive()
        {
            var state = State;

            if (state == TransactionState.Active)
            {
                // This is fine
            }
            else if (state == TransactionState.Reading)
            {
                // message must indicate 2 reads or write during read is not possible
                throw new InvalidOperationException(TransactionStateMachine.GetTransactionIdFormattedString(SR.Error_ReplicatorTransactionReadInProgress, id));
            }
            else
            {
                throw new InvalidOperationException(TransactionStateMachine.GetTransactionIdFormattedString(SR.Error_ReplicatorTransactionNotActive, id));
            }
        }

        /// <summary>
        /// The process exception.
        /// </summary>
        /// <param name="e">
        /// The e.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        protected static bool ProcessException(Exception e)
        {
            int innerHResult = 0;
            var flattenedException = Utility.FlattenException(e, out innerHResult);

            return flattenedException is FabricTransientException;
        }

        /// <summary>
        /// The throw if not primary.
        /// </summary>
        /// <exception cref="FabricNotPrimaryException">
        /// Throws if role is not primary
        /// </exception>
        protected void ThrowIfNotPrimary()
        {
            if (transactionalReplicator.Role != ReplicaRole.Primary)
            {
                throw new FabricNotPrimaryException();
            }
        }

        /// <summary>
        /// Throw is the transaction is not a primary transaction
        /// </summary>
        protected void ThrowIfNotPrimaryTransaction()
        {
            if (this.isPrimaryTx == false)
            {
                throw new InvalidOperationException(SR.Error_V2ReplNotPrimartTransaction);
            }
        }

        /// <summary>
        /// Release all read locks.
        /// </summary>
        protected void ReleaseReadLocks()
        {
            // Idempotency: If locks have already been released, return.
            if (this.readLockContexts == null)
            {
                return;
            }

            foreach (var context in this.readLockContexts)
            {
                if (context.IsTrackingContext == false)
                {
                    context.Unlock(LockContextMode.Read);
                }
            }

            // This blocks all new AddLockContext calls for LockMode.Read.
            this.readLockContexts = null;
        }

        private void AddReadLock(LockContext lockContext)
        {
            if (this.readLockContexts == null)
            {
                throw new InvalidOperationException("Read locks cannot be added once the read locks has been released.");
            }

            this.readLockContexts.Add(lockContext);
        }
    }
}
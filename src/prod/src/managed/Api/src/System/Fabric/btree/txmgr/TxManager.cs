// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.TransactionManager
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Data.Common;
    using System.Fabric.Data.LockManager;
    using System.Fabric.Data.Replicator;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// 
    /// </summary>
    public interface ITransaction
    {
        /// <summary>
        /// Transaction identifier.
        /// </summary>
        Uri Id { get; }
        /// <summary>
        /// Lock request made a spart of this transaction.
        /// </summary>
        /// <param name="resourceName"></param>
        /// <param name="mode"></param>
        /// <param name="lockTimeout"></param>
        /// <returns></returns>
        Task<ILock> LockAsync(string resourceName, LockMode mode, int lockTimeout);
        /// <summary>
        /// Unlock request dor a previously acquired lock in this transaction.
        /// </summary>
        /// <param name="acquiredLock">Acquired lock.</param>
        /// <returns></returns>
        bool Unlock(ILock acquiredLock);
        /// <summary>
        /// Clear all transaction locks.
        /// </summary>
        void Terminate();
    }
    /// <summary>
    /// Read only transaction. Locks aquired are read locks only.
    /// </summary>
    public interface IReadOnlyTransaction : ITransaction
    { 
    }
    /// <summary>
    /// Read/write transaciton. Read and  write locks can be acquired.
    /// </summary>
    public interface IReadWriteTransaction : ITransaction
    {
        /// <summary>
        /// Atomic group associated with this transaction.
        /// </summary>
        long AtomicGroupId { get; }
        /// <summary>
        /// Transaction execution context.
        /// </summary>
        ICollection<object> Context { get; }
    }
    /// <summary>
    /// Represents the transaction manager interface used by state providers.
    /// </summary>
    public interface ITransactionManager
    {
        /// <summary>
        /// Creates a read only transaciton.
        /// </summary>
        /// <returns></returns>
        IReadOnlyTransaction CreateTransaction();
        /// <summary>
        /// Creates a read/write transaction.
        /// </summary>
        /// <param name="atomicGroupId">Associated atomic group id.</param>
        /// <param name="existing">True, if transaction with the same atmoic group has already been created.</param>
        /// <returns></returns>
        IReadWriteTransaction CreateTransaction(long atomicGroupId, out bool existing);
        /// <summary>
        /// Retrieves a read/write transaction previously created.
        /// </summary>
        /// <param name="atomicGroupId">Associated atomic group id.</param>
        /// <returns></returns>
        IReadWriteTransaction GetTransaction(long atomicGroupId);
        /// <summary>
        /// Removes a transaction from the transaction table and clears the transaction locks.
        /// </summary>
        /// <param name="transaction">Transaction to terminate.</param>
        /// <returns></returns>
        bool RemoveTransaction(ITransaction transaction);
        /// <summary>
        /// Removes a read/write transaction from the transaction table and clears the transaction locks.
        /// </summary>
        /// <param name="atomicGroupId">Associated atmoic group id.</param>
        /// <returns></returns>
        bool RemoveTransaction(long atomicGroupId);
    }
    /// <summary>
    /// Base transaction class implementaiton.
    /// </summary>
    class Transaction : Disposable, ITransaction
    {
        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="lockManager">Lock manager to be used for lock acquisition/release.</param>
        /// <param name="owner">Owner of this trnasaction. Typically this is the identifier of a transaction manager.</param>
        /// <param name="isReadOnly">True, if the transaction is aread only.</param>
        public Transaction(LockManager lockManager, Uri owner, bool isReadOnly)
        {
            this.lockManager = lockManager;
            this.id = new Uri(owner.ToString() + "/" + Guid.NewGuid());
            this.isReadOnly = isReadOnly;

            AppTrace.TraceSource.WriteNoise("Transaction.Transaction", "({0}, {1})", this.id, this.isReadOnly);
        }

        #region Instance Members

        /// <summary>
        /// Transaction identifier.
        /// </summary>
        Uri id;
        /// <summary>
        /// True if the transaction is read only.
        /// </summary>
        bool isReadOnly;
        /// <summary>
        /// Lock manager to be used by this transaction.
        /// </summary>
        LockManager lockManager;
        /// <summary>
        /// Lock requests table.
        /// </summary>
        Dictionary<string, List<ILock>> lockRequests = new Dictionary<string, List<ILock>>();
        /// <summary>
        /// Concurrency control. Concurrent lock requests can be issued.
        /// </summary>
        ReaderWriterLockSlim stateLock = new ReaderWriterLockSlim(LockRecursionPolicy.NoRecursion);
        /// <summary>
        /// True, if transaction is open for bussiness.
        /// </summary>
        bool status = true;

        #endregion

        #region ITransaction Members

        /// <summary>
        /// Transaction identifier.
        /// </summary>
        public Uri Id
        {
            get { return this.id; }
        }
        /// <summary>
        /// Attempts to acquire the specified lock on behalf of this transaction within the given timeout.
        /// </summary>
        /// <param name="resourceName">Resource name to lock.</param>
        /// <param name="mode">Lock mode for this request.</param>
        /// <param name="timeout">Lock timeout for this request.</param>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.ArgumentNullException">Rresource name is null or empty.</exception>
        /// <returns></returns>
        public async Task<ILock> LockAsync(string resourceName, LockMode mode, int timeout)
        {
            //
            // Check arguments.
            //
            if (string.IsNullOrEmpty(resourceName) || string.IsNullOrWhiteSpace(resourceName))
            {
                throw new ArgumentNullException("resourceName");
            }
            if (0 > timeout && Timeout.Infinite != timeout)
            {
                throw new ArgumentException(StringResources.Error_InvalidTimeOut, "timeout");
            }
            if (LockMode.Free == mode)
            {
                throw new ArgumentException(StringResources.Error_InvalidLockMode, "mode");
            }
            if (this.isReadOnly)
            {
                if (!this.lockManager.IsShared(mode))
                {
                    throw new ArgumentException(StringResources.Error_InvalidLockMode, "mode");
                }
            }
            //
            // Check status of this transaction.
            //
            this.stateLock.EnterReadLock();
            try
            {
                if (!this.status)
                {
                    AppTrace.TraceSource.WriteError(
                        "Transaction.Lock", 
                        "({0}, {1}, {2}, {3})", 
                        this.id, 
                        resourceName, 
                        mode, 
                        timeout);

                    throw new FabricObjectClosedException();
                }
            }
            finally
            {
                this.stateLock.ExitReadLock();
            }
            ILock acquiredLock = null;
            try
            {
                //
                // Attempt to acquire lock.
                //
                acquiredLock = await this.lockManager.AcquireLockAsync(this.id, resourceName, mode, timeout);
            }
            catch (Exception e)
            {
                e = (e is AggregateException) ? e.InnerException : e;
                Debug.Assert(e is OutOfMemoryException || e is ArgumentException);
                throw;
            }
            if (LockStatus.Timeout == acquiredLock.Status)
            {
                throw new TimeoutException();
            }
            Debug.Assert(LockStatus.Granted == acquiredLock.Status);
            Debug.Assert(mode == acquiredLock.Mode);
            Debug.Assert(this.id == acquiredLock.Owner);
            //
            // Store the acquired lock.
            //
            this.stateLock.EnterWriteLock();
            try
            {
                //
                // Check status of this transaction.
                //
                if (!this.status)
                {
                    AppTrace.TraceSource.WriteError(
                        "Transaction.Lock", 
                        "({0}, {1}, {2}, {3})", 
                        this.id, 
                        resourceName, 
                        mode, 
                        timeout);

                    throw new FabricObjectClosedException();
                }
                if (!this.lockRequests.ContainsKey(resourceName))
                {
                    //
                    // New lock resource requested.
                    //
                    this.lockRequests.Add(resourceName, new List<ILock>());
                }
                //
                // Add only the first instance of the acquired lock.
                //
                if (1 == acquiredLock.Count)
                {
                    this.lockRequests[resourceName].Add(acquiredLock);
                }

                AppTrace.TraceSource.WriteNoise(
                    "Transaction.Lock", 
                    "({0}, {1}, {2}, {3}, {4}, {5})", 
                    this.id,
                    acquiredLock.ResourceName,
                    acquiredLock.Mode,
                    acquiredLock.Status,
                    acquiredLock.Count,
                    acquiredLock.GrantTime);

                return acquiredLock;
            }
            catch (Exception e)
            {
                Debug.Assert(e is OutOfMemoryException || e is FabricObjectClosedException);
                //
                // Unlock acquired lock.
                //
                UnlockStatus unlockStatus = this.lockManager.ReleaseLock(acquiredLock);
                Debug.Assert(UnlockStatus.Success == unlockStatus);
                throw;
            }
            finally
            {
                this.stateLock.ExitWriteLock();
            }
        }
        /// <summary>
        /// Releases a previously acquired lock as part of this transaction.
        /// </summary>
        /// <param name="acquiredLock">Previously acquired lock.</param>
        /// <returns></returns>
        public bool Unlock(ILock acquiredLock)
        {
            //
            // Check arguments.
            //
            if (null == acquiredLock || null == acquiredLock.ResourceName)
            {
                throw new ArgumentNullException("acquiredLock");
            }
            if (this.isReadOnly)
            {
                if (!this.lockManager.IsShared(acquiredLock.Mode))
                {
                    throw new ArgumentException(StringResources.Error_InvalidLock, "acquiredLock");
                }
            }
            this.stateLock.EnterWriteLock();
            try
            {
                if (!this.status)
                {
                    //
                    // All locks have already been released.
                    //
                    return false;
                }
                if (!this.lockRequests.ContainsKey(acquiredLock.ResourceName) ||
                    !this.lockRequests[acquiredLock.ResourceName].Contains(acquiredLock))
                {
                    //
                    // All locks for this lock resource have been released or
                    // this lock resource was released before.
                    //
                    return false;
                }
                //
                // Release lock.
                //
                UnlockStatus unlockStatus = this.lockManager.ReleaseLock(acquiredLock);
                Debug.Assert(UnlockStatus.Success == unlockStatus);
                //
                // Remove the lock only if this is the last reference to the lock.
                //
                if (0 == acquiredLock.Count)
                {
                    this.lockRequests[acquiredLock.ResourceName].Remove(acquiredLock);
                }

                AppTrace.TraceSource.WriteNoise(
                    "Transaction.Unlock", 
                    "({0}, {1}, {2}, {3}, {4})", 
                    this.id,
                    acquiredLock.ResourceName,
                    acquiredLock.Mode,
                    acquiredLock.Status,
                    acquiredLock.Count);

                return true;
            }
            finally
            {
                this.stateLock.ExitWriteLock();
            }
        }
        /// <summary>
        /// Releases all locks taken as part of this transaction.
        /// </summary>
        public void Terminate()
        {
            AppTrace.TraceSource.WriteNoise("Transaction.Terminate", "{0}", this.id);

            this.stateLock.EnterWriteLock();
            this.status = false;
            Dictionary<string, List<ILock>>.Enumerator enumerate = this.lockRequests.GetEnumerator();
            while (enumerate.MoveNext())
            {
                foreach (ILock x in enumerate.Current.Value)
                {
                    while (0 != x.Count)
                    {
                        UnlockStatus unlockStatus = this.lockManager.ReleaseLock(x);
                        Debug.Assert(UnlockStatus.Success == unlockStatus);

                        AppTrace.TraceSource.WriteNoise(
                            "Transaction.Terminate", 
                            "({0}, {1}, {2}, {3}, {4})", 
                            this.id,
                            x.ResourceName,
                            x.Mode,
                            x.Status,
                            x.Count);
                    }
                }
            }
            this.lockRequests.Clear();
            this.stateLock.ExitWriteLock();
        }

        #endregion

        #region IDisposable Overrides

        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting managed/unmanaged resources.
        /// </summary>
        /// <param name="disposing">True, if managed resources only are freed.</param>
        protected override void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (disposing)
                {
                    AppTrace.TraceSource.WriteNoise("Transaction.Dispose", "{0}", this.id);

                    if (null != this.stateLock)
                    {
                        this.stateLock.Dispose();
                        this.stateLock = null;
                    }
                }
                this.disposed = true;
            }
        }

        #endregion
    }
    /// <summary>
    /// Specialized read only transaction.
    /// </summary>
    class ReadOnlyTransaction : Transaction, IReadOnlyTransaction
    {
        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="lockManager">Lock manager to use.</param>
        /// <param name="owner">Owner for this transaction.</param>
        public ReadOnlyTransaction(LockManager lockManager, Uri owner)
            : base(lockManager, owner, true)
        {
        }
    }
    /// <summary>
    /// 
    /// </summary>
    class ReadWriteTransaction : Transaction, IReadWriteTransaction
    {
        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="atomicGroupId">Associated atomic group id.</param>
        /// <param name="lockManager">Lock manager to use.</param>
        /// <param name="owner">Owner for this transaction.</param>
        public ReadWriteTransaction(long atomicGroupId, LockManager lockManager, Uri owner)
            : base(lockManager, owner, false)
        {
            this.atomicGroupId = atomicGroupId;
        }

        #region Instance Members

        /// <summary>
        /// Atomic group identifying this read/write transaction.
        /// </summary>
        long atomicGroupId = FabricReplicatorEx.InvalidAtomicGroupId;
        /// <summary>
        /// Transaction execution context.
        /// </summary>
        TransactionContextCollection contexts = new TransactionContextCollection();

        #endregion

        #region IReadWriteTransaction Members

        /// <summary>
        /// 
        /// </summary>
        public long AtomicGroupId
        {
            get { return this.atomicGroupId; }
        }
        /// <summary>
        /// 
        /// </summary>
        public ICollection<object> Context
        {
            get { return this.contexts; }
        }

        #endregion
}
    /// <summary>
    /// Lightweight transaction manager. Manages read only and read/write transactions.
    /// </summary>
    sealed class TransactionManager : Disposable, ITransactionManager
    {
        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="owner">Owner of the transaction manager. Typically this is the identifier of a state provider.</param>
        public TransactionManager(Uri owner)
        {
            this.owner = owner;
        }

        #region Instance Members

        /// <summary>
        /// Lock manager to be used for transaction initialization.
        /// </summary>
        LockManager lockManager;
        /// <summary>
        /// Owner of the transaction manager.
        /// </summary>
        Uri owner;
        /// <summary>
        /// Concurrency control. Multiple transactions can be created/terminated concurrently.
        /// </summary>
        ReaderWriterLockSlim stateLock = new ReaderWriterLockSlim(LockRecursionPolicy.NoRecursion);
        /// <summary>
        /// Transaction table. Contains only read/write transactions that have and associated atomic group id.
        /// </summary>
        Dictionary<long, ReadWriteTransaction> rwTransactions = new Dictionary<long, ReadWriteTransaction>();
        /// <summary>
        /// Keeps the Open/Closed status of this lock manager.
        /// </summary>
        bool status;
        /// <summary>
        /// The transaction manager will wait for all read/write transactions to be terminated during closing. 
        /// </summary>
        TaskCompletionSource<bool> drainAllRwTransactions;

        #endregion

        #region ITransactionManager Members

        /// <summary>
        /// Creates a read only transaction.
        /// </summary>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <returns></returns>
        public IReadOnlyTransaction CreateTransaction()
        {
            this.stateLock.EnterReadLock();
            try
            {
                //
                // If the transaction manager is closing, then reject new transaction creation.
                //
                if (!this.status)
                {
                    AppTrace.TraceSource.WriteError(
                        "TransactionManager.IReadOnlyTransaction.CreateTransaction", 
                        "{0}", 
                        this.owner);

                    throw new FabricObjectClosedException();
                }

                AppTrace.TraceSource.WriteNoise(
                    "TransactionManager.IReadOnlyTransaction.CreateTransaction", 
                    "{0}", 
                    this.owner);

                //
                // Do not store read only transactions.
                //
                return new ReadOnlyTransaction(this.lockManager, this.owner);
            }
            finally
            {
                this.stateLock.ExitReadLock();
            }
        }
        /// <summary>
        /// Creates a read/write transaction.
        /// </summary>
        /// <param name="atomicGroupId">Associated atomic group id.</param>
        /// <param name="existing">True, if transaction already exists.</param>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <returns></returns>
        public IReadWriteTransaction CreateTransaction(long atomicGroupId, out bool existing)
        {
            existing = false;
            //
            // Check arguments.
            //
            if (0 > atomicGroupId && FabricReplicatorEx.InvalidAtomicGroupId != atomicGroupId)
            {
                throw new ArgumentException(StringResources.Error_InvalidAtomicGroup, "atomicGroupId");
            }
            this.stateLock.EnterWriteLock();
            try
            {
                //
                // If the transaction manager is closing, then reject new transaction creation.
                //
                if (!this.status)
                {
                    AppTrace.TraceSource.WriteError(
                        "TransactionManager.IReadWriteTransaction.CreateTransaction", 
                        "{0}", 
                        this.owner);

                    throw new FabricObjectClosedException();
                }
                if (FabricReplicatorEx.InvalidAtomicGroupId == atomicGroupId)
                {
                    AppTrace.TraceSource.WriteNoise(
                        "TransactionManager.IReadWriteTransaction.CreateTransaction", 
                        "({0}, {1})", 
                        this.owner,
                        atomicGroupId);
                    //
                    // Do not store trivial transactions.
                    //
                    return new ReadWriteTransaction(atomicGroupId, this.lockManager, this.owner);
                }
                ReadWriteTransaction rwtx = null;
                if (this.rwTransactions.ContainsKey(atomicGroupId))
                {
                    //
                    // Return existing transaction.
                    //
                    rwtx = this.rwTransactions[atomicGroupId];
                    existing = true;
                }
                else
                {
                    //
                    // Create and store new transaction.
                    //
                    rwtx = new ReadWriteTransaction(atomicGroupId, this.lockManager, this.owner);
                    this.rwTransactions.Add(atomicGroupId, rwtx);
                }

                AppTrace.TraceSource.WriteNoise(
                    "TransactionManager.IReadWriteTransaction.CreateTransaction", 
                    "({0}, {1})", 
                    this.owner,
                    atomicGroupId);

                return rwtx;
            }
            finally
            {
                this.stateLock.ExitWriteLock();
            }
        }
        /// <summary>
        /// Retrieves an existent read/write transaction based on its associated atomic group id.
        /// </summary>
        /// <param name="atomicGroupId">Associated atomic group id.</param>
        /// <exception cref="System.ArgumentException">Invalid atmoic group.</exception>
        /// <returns></returns>
        public IReadWriteTransaction GetTransaction(long atomicGroupId)
        {
            IReadWriteTransaction rwtx = null;
            //
            // Check arguments.
            //
            if (0 > atomicGroupId)
            {
                throw new ArgumentException(StringResources.Error_InvalidAtomicGroup, "atomicGroupId");
            }
            this.stateLock.EnterReadLock();
            try
            {
                //
                // Return existent transaction.
                //
                rwtx = this.rwTransactions[atomicGroupId];

                AppTrace.TraceSource.WriteNoise(
                    "TransactionManager.IReadWriteTransaction.GetTransaction",
                    "({0}, {1})",
                    this.owner,
                    atomicGroupId);
            }
            catch (KeyNotFoundException)
            {
                AppTrace.TraceSource.WriteWarning(
                    "TransactionManager.IReadWriteTransaction.GetTransaction",
                    "({0}, {1})",
                    this.owner,
                    atomicGroupId);
            }
            finally
            {
                this.stateLock.ExitReadLock();
            }
            return rwtx;
        }
        /// <summary>
        /// Removes a transaction from the transaction table and terminates it by clearing all its locks.
        /// </summary>
        /// <param name="transaction">Transaction to terminate.</param>
        /// <exception cref="System.ArgumentNullException">Transaction is null.</exception>
        /// <exception cref="System.ArgumentException">Transaction type is unknown.</exception>
        /// <returns></returns>
        public bool RemoveTransaction(ITransaction transaction)
        {
            //
            // Check arguments.
            //
            if (null == transaction)
            {
                throw new ArgumentNullException("transaction");
            }
            //
            // Done, if transaction is read only.
            //
            if (transaction is IReadOnlyTransaction)
            {
                ReadOnlyTransaction rotx = transaction as ReadOnlyTransaction;
                if (null == rotx)
                {
                    throw new ArgumentException(StringResources.Error_InvalidTransaction, "transaction");
                }
                //
                // Release transaction locks.
                //
                rotx.Terminate();
                return false;
            }
            //
            // Done, if transaction is trivial.
            //
            ReadWriteTransaction rwtx = transaction as ReadWriteTransaction;
            if (null == rwtx)
            {
                throw new ArgumentException(StringResources.Error_InvalidTransaction, "transaction");
            }
            if (FabricReplicatorEx.InvalidAtomicGroupId == rwtx.AtomicGroupId)
            {
                //
                // Release transaction locks.
                //
                rwtx.Terminate();
                return false;
            }
            return this.RemoveTransaction(rwtx.AtomicGroupId);
        }
        /// <summary>
        /// Removes a transaction from the transaction table and terminates it by clearing all its locks.
        /// </summary>
        /// <param name="atomicGroupId">Associated atomic group.</param>
        /// <exception cref="System.ArgumentException">Atomic group is invalid.</exception>
        /// <returns></returns>
        public bool RemoveTransaction(long atomicGroupId)
        {
            //
            // Check arguments.
            //
            if (0 > atomicGroupId)
            {
                throw new ArgumentException(StringResources.Error_InvalidAtomicGroup, "atomicGroupId");
            }
            ReadWriteTransaction rwtx = null;
            bool noActiveTransactionsFound = false;
            //
            // Remove transaction before releasing the transaction locks.
            //
            this.stateLock.EnterWriteLock();
            bool exists = this.rwTransactions.ContainsKey(atomicGroupId);
            if (exists)
            {
                rwtx = this.rwTransactions[atomicGroupId];
                this.rwTransactions.Remove(atomicGroupId);
                noActiveTransactionsFound = (0 == this.rwTransactions.Count && !this.status);
                if (!this.status)
                {
                    AppTrace.TraceSource.WriteNoise(
                        "TransactionManager.RemoveTransaction", 
                        "{0} {1} transactions remaining", 
                        this.owner,
                        this.rwTransactions.Count);
                }
            }
            this.stateLock.ExitWriteLock();
            if (exists)
            {
                AppTrace.TraceSource.WriteNoise(
                    "TransactionManager.RemoveTransaction", 
                    "({0}, {1})", 
                    this.owner,
                    atomicGroupId);
                //
                // Release transaction locks.
                //
                rwtx.Terminate();
                //
                // Complete the close if it is in progress.
                //
                if (noActiveTransactionsFound)
                {
                    AppTrace.TraceSource.WriteNoise("TransactionManager.RemoveTransaction", "{0} transactions drained", this.owner);
                    this.drainAllRwTransactions.SetResult(true);
                }
            }
            return exists;
        }

        #endregion

        /// <summary>
        /// Initializes all resources required for the transaction manager.
        /// </summary>
        /// <returns></returns>
        public async Task OpenAsync(CancellationToken cancellationToken)
        {
            this.stateLock.EnterWriteLock();
            try
            {
                AppTrace.TraceSource.WriteNoise("TransactionManager.Open", "{0}", this.owner);
                //
                // Create new lock manager.
                //
                this.lockManager = new LockManager();
                //
                // Open the lock manager.
                //
                await this.lockManager.OpenAsync(this.owner, cancellationToken);
            }
            finally
            {
                this.stateLock.ExitWriteLock();
            }
            this.drainAllRwTransactions = new TaskCompletionSource<bool>();
            //
            // Set the status of this transaction manager to be open for business.
            //
            this.status = true;
        }

        /// <summary>
        /// Closes the transaction manager and waits for all transactions to be terminated.
        /// </summary>
        /// <returns></returns>
        public Task CloseAsync(CancellationToken cancellationToken)
        {
            return this.CloseAsync(true, cancellationToken);
        }

        /// <summary>
        /// The transaction manager terminates immediately.
        /// </summary>
        public void Abort()
        {
            this.CloseAsync(false, CancellationToken.None).Wait();
        }

        /// <summary>
        /// Closes the transaction manager and waits for all transactions to be terminated.
        /// </summary>
        /// <returns></returns>
        async Task CloseAsync(bool drain, CancellationToken cancellationToken)
        {
            int countInFlight = 0;
            AppTrace.TraceSource.WriteNoise("TransactionManager.Close", "{0} - drain {1}", this.owner, drain);
            //
            // Close/Abort the lock manager.
            //
            if (drain)
            {
                await this.lockManager.CloseAsync(cancellationToken);
            }
            else
            {
                this.lockManager.Abort();
            }
            //
            // Check to see if any draining is required.
            //
            this.stateLock.EnterWriteLock();
            if (this.status)
            {
                //
                // Mark the status as closed.
                //
                this.status = false;
                countInFlight = this.rwTransactions.Count;
            }
            this.stateLock.ExitWriteLock();
            //
            // If there are in-flight transactions, then wait for them be terminated.
            //
            if (0 != countInFlight && drain)
            {
                AppTrace.TraceSource.WriteNoise("TransactionManager.Close", "{0} start drain", this.owner);
                await this.drainAllRwTransactions.Task;
                AppTrace.TraceSource.WriteNoise("TransactionManager.Close", "{0} drain ended", this.owner);
            }
        }

        #region IDisposable Overrides

        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting managed/unmanaged resources.
        /// </summary>
        /// <param name="disposing">True, if managed resources only are freed.</param>
        protected override void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (disposing)
                {
                    AppTrace.TraceSource.WriteNoise("TransactionManager.Dispose", "{0}", this.owner);

                    if (null != this.lockManager)
                    {
                        this.lockManager.Dispose();
                        this.lockManager = null;
                    }
                    if (null != this.stateLock)
                    {
                        this.stateLock.Dispose();
                        this.stateLock = null;
                    }
                }
                this.disposed = true;
            }
        }

        #endregion
    }

    /// <summary>
    /// Used for transaction thread-safe context list. This is required
    /// in order to support concurrent writes in the same transaction.
    /// </summary>
    class TransactionContextCollection : ICollection<object>
    {
        /// <summary>
        /// Inner context list.
        /// </summary>
        private List<object> contexts = new List<object>();

        public void Add(object item)
        {
            lock (this)
            {
                this.contexts.Add(item);
            }
        }

        public void Clear()
        {
            lock (this)
            {
                this.contexts.Clear();
            }
        }

        public bool Contains(object item)
        {
            lock (this)
            {
                return this.contexts.Contains(item);
            }
        }

        public void CopyTo(object[] array, int arrayIndex)
        {
            lock (this)
            {
                this.contexts.CopyTo(array, arrayIndex);
            }
        }

        public int Count
        {
            get 
            {
                lock (this)
                {
                    return this.contexts.Count;
                }
            }
        }

        public bool IsReadOnly
        {
            get { return false; }
        }

        public bool Remove(object item)
        {
            lock (this)
            {
                return this.contexts.Remove(item);
            }
        }

        public IEnumerator<object> GetEnumerator()
        {
            lock (this)
            {
                List<object> copy = new List<object>();
                copy.AddRange(this.contexts);
                return copy.GetEnumerator();
            }
        }

        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
        {
            lock (this)
            {
                object[] array = new object[this.contexts.Count];
                this.contexts.CopyTo(array);
                return array.GetEnumerator();
            }
        }
    }
}
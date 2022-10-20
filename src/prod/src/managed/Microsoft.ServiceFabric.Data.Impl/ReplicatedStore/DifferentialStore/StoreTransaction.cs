// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Data.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;
    using Microsoft.ServiceFabric.Replicator;
    using Microsoft.ServiceFabric.Data;

    // Alias.
    using ReplicatorTransactionBase = Microsoft.ServiceFabric.Replicator.TransactionBase;

    /// <summary>
    /// Base class for store transactions.
    /// </summary>
    internal class StoreTransaction : LockContext, ILockContainer
    {
        #region Instance Members

        /// <summary>
        /// Indicates whether strict 2PL is enabled.
        /// </summary>
        private readonly bool enableStrict2PL = false;

        /// <summary>
        /// Status.
        /// </summary>
        private bool isClosed = false;

        /// <summary>
        /// True, if the store transaction is read only.
        /// </summary>
        private bool isReadOnly;

        /// <summary>
        /// Store transaction id.
        /// </summary>
        private long id;

        /// <summary>
        /// Lock requests accumulated as part of this store transaction.
        /// </summary>
        private List<ILock> lockRequests = new List<ILock>();

        /// <summary>
        /// Lock requests associated with prime lock.
        /// </summary>
        private List<PrimeLockRequest> primeLockRequests = new List<PrimeLockRequest>();

        /// <summary>
        /// Used to determine if all activities have quiesced.
        /// </summary>
        private TaskCompletionSource<bool> waitable = new TaskCompletionSource<bool>();

        /// <summary>
        /// Default lock timeout for this store transaction.
        /// </summary>
        private TimeSpan defaultLockTimeout = Timeout.InfiniteTimeSpan;

        /// <summary>
        /// Used for transaction initialization.
        /// </summary>
        private bool isPrimeLockAcquired = false;

        /// <summary>
        /// Read locks are cleared only once.
        /// </summary>
        private long clearReadLocks = 0;

        /// <summary>
        /// Used for tracing.
        /// </summary>
        private string tracer;

        /// <summary>
        /// Store that owns this store transaction.
        /// </summary>
        private IStoreTransactionProvider owner;

        /// <summary>
        /// Replicator transaction associated with this store transaction.
        /// </summary>
        private ReplicatorTransactionBase replicatorTransactionBase;

        #endregion

        /// <summary>
        /// Proper constructor.
        /// </summary>
        /// <param name="id">Store transaction id.</param>
        /// <param name="isReadOnly">True, if the store transaction is read only.</param>
        /// <param name="owner"></param>
        /// <param name="replicatorTransaction"></param>
        /// <param name="tracer"></param>
        /// <param name="enableStrict2PL">Indicates whether strict 2PL is enabled.</param>
        public StoreTransaction(
            long id,
            bool isReadOnly,
            IStoreTransactionProvider owner,
            ReplicatorTransactionBase replicatorTransaction,
            string tracer,
            bool enableStrict2PL)
        {
            this.id = id;
            this.isReadOnly = isReadOnly;
            this.tracer = tracer;
            this.owner = owner;
            this.replicatorTransactionBase = replicatorTransaction;
            this.enableStrict2PL = enableStrict2PL;
        }

        /// <summary>
        /// Finalization code.
        /// </summary>
        ~StoreTransaction()
        {
            this.Dispose(false);
        }

        /// <summary>
        /// Used for tracing.
        /// </summary>
        public string Tracer
        {
            get { return this.tracer; }
        }

        /// <summary>
        /// Store owning the store transaction.
        /// </summary>
        public IStoreTransactionProvider Owner
        {
            get { return this.owner; }
        }

        #region IStoreReadTransaction Members

        /// <summary>
        /// Gets the store transaction id.
        /// </summary>
        public long Id
        {
            get { return this.id; }
        }

        /// <summary>
        /// Gets the replicator transaction base associated with this read/write store transaction.
        /// </summary>
        public ReplicatorTransactionBase ReplicatorTransactionBase
        {
            get
            {
                return this.replicatorTransactionBase;
            }
        }

        /// <summary>
        /// Specifies whether a store transaction was completed.
        /// </summary>
        public virtual bool IsCompleted
        {
            get { throw new NotImplementedException(); }

            set { throw new NotImplementedException(); }
        }

        /// <summary>
        /// Gets/Sets the default lock timeout for this store transaction.
        /// </summary>
        public TimeSpan DefaultTimeout
        {
            get { return this.defaultLockTimeout; }

            set { this.defaultLockTimeout = value; }
        }

        #endregion

        #region ILockContainer Members

        /// <summary>
        /// Acquires a lock in the context of this store transaction.
        /// </summary>
        /// <param name="lockManager">Lock manager that issues the lock.</param>
        /// <param name="lockResourceNameHash">Lock resource name hash.</param>
        /// <param name="lockMode">Lock mode requested.</param>
        /// <param name="lockTimeout">Lock timeout.</param>
        /// <exception cref="TransactionFaultedException" />
        /// <exception cref="InvalidOperationException">Thrown by the replicator (ReplicatorTransactionBase.AddLockContext)</exception>
        /// <exception cref="TimeoutException" />
        /// <returns></returns>
        public async Task<ILock> LockAsync(LockManager lockManager, ulong lockResourceNameHash, LockMode lockMode, int lockTimeout)
        {
            // Check arguments.
            if (LockMode.Free == lockMode)
            {
                throw new ArgumentException(SR.Error_LockMode);
            }

            if (0 > lockTimeout && Timeout.Infinite != lockTimeout)
            {
                throw new ArgumentException(SR.Error_LockTimeout);
            }

            if (this.isReadOnly)
            {
                if (!lockManager.IsShared(lockMode))
                {
                    throw new ArgumentException(SR.Error_LockMode);
                }
            }

            if (this.enableStrict2PL == true && lockMode == LockMode.Shared)
            {
                this.ReplicatorTransactionBase.AddLockContext(this, LockContextMode.Read);
            }

            // Call the lock manager to get the lock.
            var acquiredLock = await lockManager.AcquireLockAsync(this.id, lockResourceNameHash, lockMode, lockTimeout).ConfigureAwait(false);
            Diagnostics.Assert(null != acquiredLock, this.tracer, "incorrect lock");

            // Check is error has occured.
            if (LockStatus.Invalid == acquiredLock.Status)
            {
                throw new TransactionFaultedException(string.Format(CultureInfo.CurrentCulture, SR.Error_ReplicatorTransactionInternallyAborted, this.Id));
            }

            if (LockStatus.Timeout == acquiredLock.Status)
            {
#if DEBUG
                Diagnostics.Assert(acquiredLock.OldestGrantee != -1, "OldestGrantee should not be -1. There should be at least one grantee");
#endif
                throw new TimeoutException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        SR.LockTimeout_TStore_KeyLock,
                        lockMode,
                        this.tracer,
                        lockTimeout,
                        this.id,
                        lockResourceNameHash,
                        acquiredLock.OldestGrantee));
            }

            Diagnostics.Assert(LockStatus.Granted == acquiredLock.Status, this.tracer, "incorrect lock status");
            Diagnostics.Assert(lockMode == acquiredLock.Mode, this.tracer, "incorrect lock mode");
            Diagnostics.Assert(this.id == acquiredLock.Owner, this.tracer, "incorrect lock owner");
            Diagnostics.Assert(0 < acquiredLock.Count, this.tracer, "incorrect lock count");
            Diagnostics.Assert(null != acquiredLock.LockManager, this.tracer, "incorrect lock manager");

            var releaseLastLockAcquired = false;
            lock (this)
            {
                // It's possible that the store transaction has been terminated before this last lock was acquired.
                releaseLastLockAcquired = (this.isClosed == true);

                // Add the acquired lock only once to the lock list.
                if (acquiredLock.Count == 1 && !releaseLastLockAcquired)
                {
                    this.lockRequests.Add(acquiredLock);
                }
            }

            if (releaseLastLockAcquired)
            {
                lockManager.ReleaseLock(acquiredLock);
                throw new TransactionFaultedException(string.Format(CultureInfo.CurrentCulture, SR.Error_ReplicatorTransactionInternallyAborted, this.Id));
            }

            return acquiredLock;
        }

        /// <summary>
        /// Clears all the transaction locks.
        /// </summary>
        public void ClearLocks()
        {
            lock (this)
            {
                if (this.isClosed == true)
                {
                    return;
                }

                // Once locks have been cleared, no new locks should be acquired.
                this.isClosed = true;
            }

            FabricEvents.Events.ClearLocks(this.tracer, this.id);

            foreach (var primeLockRequest in this.primeLockRequests)
            {
                primeLockRequest.LockManager.ReleasePrimeLock(primeLockRequest.LockMode);
            }

            // Reset prime locks.
            this.primeLockRequests = null;

            // Clear all locks acquired.
            foreach (var keyLock in this.lockRequests)
            {
                while (keyLock.Count > 0)
                {
                    keyLock.LockManager.ReleaseLock(keyLock);
                }
            }

            this.lockRequests.Clear();
            this.lockRequests = null;
        }

        #endregion

        /// <summary>
        /// Initializes a store transaction.
        /// </summary>
        /// <param name="lockManager">Lock manager that issues the lock.</param>
        /// <param name="lockMode">Lock mode requested.</param>
        /// <param name="lockTimeout">Lock timeout.</param>
        /// <param name="reacquirePrimeLock">Unconditionally acquire the prime lock, even if it's already been acquired by this store transaction.</param>
        /// <returns>Amount of time left after the lock was taken.</returns>
        public async Task<int> AcquirePrimeLocksAsync(LockManager lockManager, LockMode lockMode, int lockTimeout, bool reacquirePrimeLock)
        {
            Diagnostics.Assert(lockMode == LockMode.Shared || lockMode == LockMode.Exclusive, this.tracer, "unexpected lock mode");

            if (reacquirePrimeLock)
            {
                this.isPrimeLockAcquired = false;
            }

            if (this.isPrimeLockAcquired)
            {
                lock (this)
                {
                    // Lock must have been released already so there is no need to release anything here.
                    if (this.isClosed == true)
                    {
                        throw new TransactionFaultedException(string.Format(CultureInfo.CurrentCulture, SR.Error_ReplicatorTransactionInternallyAborted, this.Id));
                    }
                }

                // Initialization was already performed.
                return lockTimeout;
            }

            var timeoutLeftOver = lockTimeout;
            var start = DateTime.UtcNow;
            FabricEvents.Events.PrimeLocksAsync(this.tracer, (byte)lockMode, this.id, lockTimeout);

            await lockManager.AcquirePrimeLockAsync(lockMode, lockTimeout, CancellationToken.None).ConfigureAwait(false);

            var releaseAcquiredPrimeLock = false;
            lock (this)
            {
                if (this.isClosed == true)
                {
                    releaseAcquiredPrimeLock = true;
                }
                else
                {
                    this.primeLockRequests.Add(new PrimeLockRequest(lockManager, lockMode));
                }
            }

            if (releaseAcquiredPrimeLock)
            {
                lockManager.ReleasePrimeLock(lockMode);

                throw new TransactionFaultedException(string.Format(CultureInfo.CurrentCulture, SR.Error_ReplicatorTransactionInternallyAborted, this.Id));
            }

            // Reset initializer.
            this.isPrimeLockAcquired = true;

            // Compute left-over timeout, if needed.
            if (Timeout.Infinite != lockTimeout && 0 != lockTimeout)
            {
                var end = DateTime.UtcNow;
                var duration = end.Subtract(start);

                if (duration.TotalMilliseconds <= 0)
                {
                    timeoutLeftOver = lockTimeout;
                }
                else if (lockTimeout < duration.TotalMilliseconds)
                {
                    // Inform the caller that locks could not be acquired in the given timeout.
                    timeoutLeftOver = 0;
                }
                else
                {
                    // Next use left-over timeout.
                    timeoutLeftOver = lockTimeout - (int)duration.TotalMilliseconds;
                }
            }

            // Initialization is now done.
            return timeoutLeftOver;
        }

        #region ReplicatorLockContext Members

        public override void Unlock(LockContextMode mode)
        {
            if (mode == LockContextMode.Read)
            {
                if (Interlocked.Increment(ref this.clearReadLocks) == 1)
                {
                    lock (this)
                    {
                        foreach (var keyLock in this.lockRequests)
                        {
                            while (keyLock.Count > 0 && keyLock.Mode == LockMode.Shared)
                            {
                                keyLock.LockManager.ReleaseLock(keyLock);
                            }
                        }

                        return;
                    }
                }
            }
            else
            {
                Diagnostics.Assert(mode == LockContextMode.Write, this.tracer, "Inavlid lock mode {0}", mode);

                // Release all locks.
                this.Dispose();
            }
        }

        #endregion

        #region IDisposable Members

        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting resources.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        #region ReplicatorLockContext Members

        protected override bool IsEqual(object stateOrKey)
        {
            // Should never be called.
            Diagnostics.Assert(false, this.tracer, "should never be called");
            return false;
        }

        #endregion

        /// <summary>
        /// Custom dispose implementation.
        /// </summary>
        /// <param name="disposing"></param>
        protected virtual void Dispose(bool disposing)
        {
            throw new NotImplementedException();
        }

        #endregion
    }
}
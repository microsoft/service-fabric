// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.Fabric.Data.Common;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Replicator;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Lock manager implementation.
    /// </summary>
    internal sealed class LockManager : Disposable
    {
        #region Instance Members

        /// <summary>
        /// Lock compatibility matrix.
        /// </summary>
        private static Dictionary<LockMode, Dictionary<LockMode, LockCompatibility>> lockCompatibility =
            new Dictionary<LockMode, Dictionary<LockMode, LockCompatibility>>();

        /// <summary>
        /// Max compatible lock matrix.
        /// </summary>
        private static Dictionary<LockMode, Dictionary<LockMode, LockMode>> lockConversion =
            new Dictionary<LockMode, Dictionary<LockMode, LockMode>>();

        /// <summary>
        /// Locks.
        /// </summary>
        private LockHashtable[] lockHashTables;

        /// <summary>
        /// Prime lock.
        /// </summary>
        private ReaderWriterAsyncLock primeLock;

        /// <summary>
        /// Keeps the Open/Closed status of this lock manager.
        /// </summary>
        private bool status;

        /// <summary>
        /// Number of lock hash tables for keys.
        /// </summary>
        private ulong lockHashTableCount = (ulong) Environment.ProcessorCount*16;

        /// <summary>
        ///  A lock task cleanup is in progress for key locks hash table.
        /// </summary>
        private int[] lockReleasedCleanupInProgress;

        /// <summary>
        ///  Minimum numbers of entries in the hash table whose locks can be cleared.
        /// </summary>
        private int clearLocksThreshold = 128;

        #endregion

        private string traceType;

        /// <summary>
        /// Initialize lock conversion and lock compatibility tables.
        /// </summary>
        static LockManager()
        {
            //
            // Initialize compatibility and conversion matrices (price paid only once per process).
            //

            //
            // Granted is the first key, requested is the second key, value is the max.
            //
            lockConversion.Add(LockMode.Free, new Dictionary<LockMode, LockMode>());
            lockConversion.Add(LockMode.SchemaStability, new Dictionary<LockMode, LockMode>());
            lockConversion.Add(LockMode.SchemaModification, new Dictionary<LockMode, LockMode>());
            lockConversion.Add(LockMode.Shared, new Dictionary<LockMode, LockMode>());
            lockConversion.Add(LockMode.Update, new Dictionary<LockMode, LockMode>());
            lockConversion.Add(LockMode.Exclusive, new Dictionary<LockMode, LockMode>());
            lockConversion.Add(LockMode.IntentShared, new Dictionary<LockMode, LockMode>());
            lockConversion.Add(LockMode.IntentExclusive, new Dictionary<LockMode, LockMode>());

            lockConversion[LockMode.Free].Add(LockMode.Free, LockMode.Free);
            lockConversion[LockMode.Free].Add(LockMode.SchemaStability, LockMode.SchemaStability);
            lockConversion[LockMode.Free].Add(LockMode.SchemaModification, LockMode.SchemaModification);
            lockConversion[LockMode.Free].Add(LockMode.Shared, LockMode.Shared);
            lockConversion[LockMode.Free].Add(LockMode.Update, LockMode.Update);
            lockConversion[LockMode.Free].Add(LockMode.Exclusive, LockMode.Exclusive);
            lockConversion[LockMode.Free].Add(LockMode.IntentShared, LockMode.IntentShared);
            lockConversion[LockMode.Free].Add(LockMode.IntentExclusive, LockMode.IntentExclusive);

            lockConversion[LockMode.SchemaStability].Add(LockMode.Free, LockMode.SchemaStability);
            lockConversion[LockMode.SchemaStability].Add(LockMode.SchemaStability, LockMode.SchemaStability);
            lockConversion[LockMode.SchemaStability].Add(LockMode.SchemaModification, LockMode.SchemaModification);
            lockConversion[LockMode.SchemaStability].Add(LockMode.Shared, LockMode.Shared);
            lockConversion[LockMode.SchemaStability].Add(LockMode.Update, LockMode.Update);
            lockConversion[LockMode.SchemaStability].Add(LockMode.Exclusive, LockMode.Exclusive);
            lockConversion[LockMode.SchemaStability].Add(LockMode.IntentShared, LockMode.IntentShared);
            lockConversion[LockMode.SchemaStability].Add(LockMode.IntentExclusive, LockMode.IntentExclusive);

            lockConversion[LockMode.SchemaModification].Add(LockMode.Free, LockMode.SchemaModification);
            lockConversion[LockMode.SchemaModification].Add(LockMode.SchemaStability, LockMode.SchemaModification);
            lockConversion[LockMode.SchemaModification].Add(LockMode.SchemaModification, LockMode.SchemaModification);
            lockConversion[LockMode.SchemaModification].Add(LockMode.Shared, LockMode.SchemaModification);
            lockConversion[LockMode.SchemaModification].Add(LockMode.Update, LockMode.SchemaModification);
            lockConversion[LockMode.SchemaModification].Add(LockMode.Exclusive, LockMode.SchemaModification);
            lockConversion[LockMode.SchemaModification].Add(LockMode.IntentShared, LockMode.SchemaModification);
            lockConversion[LockMode.SchemaModification].Add(LockMode.IntentExclusive, LockMode.SchemaModification);

            lockConversion[LockMode.Shared].Add(LockMode.Free, LockMode.Shared);
            lockConversion[LockMode.Shared].Add(LockMode.SchemaStability, LockMode.Shared);
            lockConversion[LockMode.Shared].Add(LockMode.SchemaModification, LockMode.SchemaModification);
            lockConversion[LockMode.Shared].Add(LockMode.Shared, LockMode.Shared);
            lockConversion[LockMode.Shared].Add(LockMode.Update, LockMode.Update);
            lockConversion[LockMode.Shared].Add(LockMode.Exclusive, LockMode.Exclusive);
            lockConversion[LockMode.Shared].Add(LockMode.IntentShared, LockMode.Shared);

            lockConversion[LockMode.Update].Add(LockMode.Free, LockMode.Update);
            lockConversion[LockMode.Update].Add(LockMode.SchemaStability, LockMode.Update);
            lockConversion[LockMode.Update].Add(LockMode.SchemaModification, LockMode.SchemaModification);
            lockConversion[LockMode.Update].Add(LockMode.Shared, LockMode.Update);
            lockConversion[LockMode.Update].Add(LockMode.Update, LockMode.Update);
            lockConversion[LockMode.Update].Add(LockMode.Exclusive, LockMode.Exclusive);
            lockConversion[LockMode.Update].Add(LockMode.IntentShared, LockMode.Update);

            lockConversion[LockMode.Exclusive].Add(LockMode.Free, LockMode.Exclusive);
            lockConversion[LockMode.Exclusive].Add(LockMode.SchemaStability, LockMode.Exclusive);
            lockConversion[LockMode.Exclusive].Add(LockMode.SchemaModification, LockMode.SchemaModification);
            lockConversion[LockMode.Exclusive].Add(LockMode.Shared, LockMode.Exclusive);
            lockConversion[LockMode.Exclusive].Add(LockMode.Update, LockMode.Exclusive);
            lockConversion[LockMode.Exclusive].Add(LockMode.Exclusive, LockMode.Exclusive);
            lockConversion[LockMode.Exclusive].Add(LockMode.IntentShared, LockMode.Exclusive);
            lockConversion[LockMode.Exclusive].Add(LockMode.IntentExclusive, LockMode.Exclusive);

            lockConversion[LockMode.IntentShared].Add(LockMode.Free, LockMode.IntentShared);
            lockConversion[LockMode.IntentShared].Add(LockMode.SchemaStability, LockMode.IntentShared);
            lockConversion[LockMode.IntentShared].Add(LockMode.SchemaModification, LockMode.SchemaModification);
            lockConversion[LockMode.IntentShared].Add(LockMode.Shared, LockMode.Shared);
            lockConversion[LockMode.IntentShared].Add(LockMode.Update, LockMode.Update);
            lockConversion[LockMode.IntentShared].Add(LockMode.Exclusive, LockMode.Exclusive);
            lockConversion[LockMode.IntentShared].Add(LockMode.IntentShared, LockMode.IntentShared);
            lockConversion[LockMode.IntentShared].Add(LockMode.IntentExclusive, LockMode.IntentExclusive);

            lockConversion[LockMode.IntentExclusive].Add(LockMode.Free, LockMode.IntentExclusive);
            lockConversion[LockMode.IntentExclusive].Add(LockMode.SchemaStability, LockMode.IntentExclusive);
            lockConversion[LockMode.IntentExclusive].Add(LockMode.SchemaModification, LockMode.SchemaModification);
            lockConversion[LockMode.IntentExclusive].Add(LockMode.Exclusive, LockMode.Exclusive);
            lockConversion[LockMode.IntentExclusive].Add(LockMode.IntentShared, LockMode.IntentExclusive);
            lockConversion[LockMode.IntentExclusive].Add(LockMode.IntentExclusive, LockMode.IntentExclusive);

            //
            // Granted is the first key, requested is the second key, value is the result of compatibility.
            //
            lockCompatibility.Add(LockMode.Free, new Dictionary<LockMode, LockCompatibility>());
            lockCompatibility.Add(LockMode.SchemaStability, new Dictionary<LockMode, LockCompatibility>());
            lockCompatibility.Add(LockMode.SchemaModification, new Dictionary<LockMode, LockCompatibility>());
            lockCompatibility.Add(LockMode.Shared, new Dictionary<LockMode, LockCompatibility>());
            lockCompatibility.Add(LockMode.Update, new Dictionary<LockMode, LockCompatibility>());
            lockCompatibility.Add(LockMode.Exclusive, new Dictionary<LockMode, LockCompatibility>());
            lockCompatibility.Add(LockMode.IntentShared, new Dictionary<LockMode, LockCompatibility>());
            lockCompatibility.Add(LockMode.IntentExclusive, new Dictionary<LockMode, LockCompatibility>());

            lockCompatibility[LockMode.Free].Add(LockMode.Free, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.Free].Add(LockMode.SchemaStability, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.Free].Add(LockMode.SchemaModification, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.Free].Add(LockMode.Shared, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.Free].Add(LockMode.Update, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.Free].Add(LockMode.Exclusive, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.Free].Add(LockMode.IntentShared, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.Free].Add(LockMode.IntentExclusive, LockCompatibility.NoConflict);

            lockCompatibility[LockMode.SchemaStability].Add(LockMode.Free, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.SchemaStability].Add(LockMode.SchemaStability, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.SchemaStability].Add(LockMode.SchemaModification, LockCompatibility.Conflict);
            lockCompatibility[LockMode.SchemaStability].Add(LockMode.Shared, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.SchemaStability].Add(LockMode.Update, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.SchemaStability].Add(LockMode.Exclusive, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.SchemaStability].Add(LockMode.IntentShared, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.SchemaStability].Add(LockMode.IntentExclusive, LockCompatibility.NoConflict);

            lockCompatibility[LockMode.SchemaModification].Add(LockMode.Free, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.SchemaModification].Add(LockMode.SchemaStability, LockCompatibility.Conflict);
            lockCompatibility[LockMode.SchemaModification].Add(LockMode.SchemaModification, LockCompatibility.Conflict);
            lockCompatibility[LockMode.SchemaModification].Add(LockMode.Shared, LockCompatibility.Conflict);
            lockCompatibility[LockMode.SchemaModification].Add(LockMode.Update, LockCompatibility.Conflict);
            lockCompatibility[LockMode.SchemaModification].Add(LockMode.Exclusive, LockCompatibility.Conflict);
            lockCompatibility[LockMode.SchemaModification].Add(LockMode.IntentShared, LockCompatibility.Conflict);
            lockCompatibility[LockMode.SchemaModification].Add(LockMode.IntentExclusive, LockCompatibility.Conflict);

            lockCompatibility[LockMode.Shared].Add(LockMode.Free, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.Shared].Add(LockMode.SchemaStability, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.Shared].Add(LockMode.SchemaModification, LockCompatibility.Conflict);
            lockCompatibility[LockMode.Shared].Add(LockMode.Shared, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.Shared].Add(LockMode.Update, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.Shared].Add(LockMode.Exclusive, LockCompatibility.Conflict);
            lockCompatibility[LockMode.Shared].Add(LockMode.IntentShared, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.Shared].Add(LockMode.IntentExclusive, LockCompatibility.Conflict);

            lockCompatibility[LockMode.Update].Add(LockMode.Free, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.Update].Add(LockMode.SchemaStability, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.Update].Add(LockMode.SchemaModification, LockCompatibility.Conflict);
            lockCompatibility[LockMode.Update].Add(LockMode.Shared, LockCompatibility.Conflict);
            lockCompatibility[LockMode.Update].Add(LockMode.Update, LockCompatibility.Conflict);
            lockCompatibility[LockMode.Update].Add(LockMode.Exclusive, LockCompatibility.Conflict);
            lockCompatibility[LockMode.Update].Add(LockMode.IntentShared, LockCompatibility.Conflict);
            lockCompatibility[LockMode.Update].Add(LockMode.IntentExclusive, LockCompatibility.Conflict);

            lockCompatibility[LockMode.Exclusive].Add(LockMode.Free, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.Exclusive].Add(LockMode.SchemaStability, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.Exclusive].Add(LockMode.SchemaModification, LockCompatibility.Conflict);
            lockCompatibility[LockMode.Exclusive].Add(LockMode.Shared, LockCompatibility.Conflict);
            lockCompatibility[LockMode.Exclusive].Add(LockMode.Update, LockCompatibility.Conflict);
            lockCompatibility[LockMode.Exclusive].Add(LockMode.Exclusive, LockCompatibility.Conflict);
            lockCompatibility[LockMode.Exclusive].Add(LockMode.IntentShared, LockCompatibility.Conflict);
            lockCompatibility[LockMode.Exclusive].Add(LockMode.IntentExclusive, LockCompatibility.Conflict);

            lockCompatibility[LockMode.IntentShared].Add(LockMode.Free, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.IntentShared].Add(LockMode.SchemaStability, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.IntentShared].Add(LockMode.SchemaModification, LockCompatibility.Conflict);
            lockCompatibility[LockMode.IntentShared].Add(LockMode.Shared, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.IntentShared].Add(LockMode.Update, LockCompatibility.Conflict);
            lockCompatibility[LockMode.IntentShared].Add(LockMode.Exclusive, LockCompatibility.Conflict);
            lockCompatibility[LockMode.IntentShared].Add(LockMode.IntentShared, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.IntentShared].Add(LockMode.IntentExclusive, LockCompatibility.NoConflict);

            lockCompatibility[LockMode.IntentExclusive].Add(LockMode.Free, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.IntentExclusive].Add(LockMode.SchemaStability, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.IntentExclusive].Add(LockMode.SchemaModification, LockCompatibility.Conflict);
            lockCompatibility[LockMode.IntentExclusive].Add(LockMode.Shared, LockCompatibility.Conflict);
            lockCompatibility[LockMode.IntentExclusive].Add(LockMode.Update, LockCompatibility.Conflict);
            lockCompatibility[LockMode.IntentExclusive].Add(LockMode.Exclusive, LockCompatibility.Conflict);
            lockCompatibility[LockMode.IntentExclusive].Add(LockMode.IntentShared, LockCompatibility.NoConflict);
            lockCompatibility[LockMode.IntentExclusive].Add(LockMode.IntentExclusive, LockCompatibility.NoConflict);
        }

        /// <summary>
        /// Initializes all resources required for the lock manager.
        /// </summary>
        public void Open(string traceType)
        {
            this.traceType = traceType;

            //
            // Create lock hash tables and drain tasks.
            //
            this.lockReleasedCleanupInProgress = new int[this.lockHashTableCount];
            this.lockHashTables = new LockHashtable[this.lockHashTableCount];
            for (ulong idx = 0; idx < this.lockHashTableCount; idx++)
            {
                this.lockHashTables[idx] = LockHashtable.Create();
            }

            this.primeLock = new ReaderWriterAsyncLock();

            //
            // Set the status of this lock manager to be open for business.
            //
            this.status = true;
        }

        /// <summary>
        /// Closes the lock manager and waits for all locks to be released. Times out all pending lock requests.
        /// </summary>
        public void Close()
        {
            //
            // Set the status to closed. New lock request will be timed-out immediately.
            //
            this.status = false;

            for (ulong idx = 0; idx < this.lockHashTableCount; idx++)
            {
                this.ClearLocks((int) idx);
            }


            var countGranted = 0;
            foreach (var lockHashTable in this.lockHashTables)
            {
                //
                // Acquire first level lock.
                //
                lockHashTable.LockHashTableLock.EnterWriteLock();

                //
                // Enumerate all entries.
                //
                foreach (var lockEntry in lockHashTable.LockEntries)
                {
                    var hashValue = lockEntry.Value;

                    //
                    // Lock each entry.
                    //
                    Monitor.Enter(hashValue.LockHashValueLock);

                    //
                    // Fail out all waiters.
                    //
                    foreach (var x in hashValue.LockResourceControlBlock.WaitingQueue)
                    {
                        //
                        // Internal state checks.
                        //
                        Diagnostics.Assert(LockStatus.Pending == x.LockStatus, this.traceType, "incorrect lock status");

                        //
                        // Stop the expiration timer for this waiter.
                        //
                        if (x.StopExpire())
                        {
                            x.Dispose();
                        }

                        //
                        // Set the timed out lock status.
                        //
                        x.LockStatus = LockStatus.Invalid;

                        //
                        // Complete the pending waiter task.
                        //
                        var captureX = x;
                        var failWaiterTask = Task.Factory.StartNew(() => { captureX.Waiter.SetResult(captureX); });
                    }

                    //
                    // Clear waiting queue.
                    //
                    hashValue.LockResourceControlBlock.WaitingQueue.Clear();

                    //
                    // Keep track of granted lock count.
                    //
                    countGranted += hashValue.LockResourceControlBlock.GrantedList.Count;

                    //
                    // Unlock each entry.
                    //
                    Monitor.Exit(hashValue.LockHashValueLock);
                }

                //
                // Release first level lock.
                //
                lockHashTable.LockHashTableLock.ExitWriteLock();
            }

            this.primeLock.Close();
        }

        #region prime lock methods

        /// <summary>
        /// Acquires prime lock in the specified mode.
        /// </summary>
        public async Task AcquirePrimeLockAsync(LockMode lockMode, int timeout, CancellationToken cancellationToken)
        {
            Diagnostics.Assert(lockMode == LockMode.Shared || lockMode == LockMode.Exclusive, this.traceType, "incorrect lock mode {0}", lockMode);

            bool lockAcquired;

            if (lockMode == LockMode.Shared)
            {
                lockAcquired = await this.primeLock.AcquireReadLockAsync(timeout, cancellationToken).ConfigureAwait(false);
            }
            else
            {
                lockAcquired = await this.primeLock.AcquireWriteLockAsync(timeout, cancellationToken).ConfigureAwait(false);
            }

            if (lockAcquired == false)
            {
                throw new TimeoutException(
                    string.Format(
                        Globalization.CultureInfo.CurrentCulture,
                        SR.LockTimeout_LockManager_TableLock,
                        lockMode,
                        this.traceType,
                        timeout));
            }
        }

        /// <summary>
        /// Releases prime lock.
        /// </summary>
        public void ReleasePrimeLock(LockMode lockMode)
        {
            Diagnostics.Assert(lockMode == LockMode.Shared || lockMode == LockMode.Exclusive, this.traceType, "incorrect lock mode {0}", lockMode);
            if (lockMode == LockMode.Shared)
            {
                this.primeLock.ReleaseReadLock();
            }
            else
            {
                this.primeLock.ReleaseWriteLock();
            }
        }

        #endregion

        /// <summary>
        /// Starts locking a given lock resource in a given mode.
        /// </summary>
        /// <param name="owner">The lock owner for this request.</param>
        /// <param name="resourceNameHash">Resource name hash to be locked.</param>
        /// <param name="mode">Lock mode required by this request.</param>
        /// <param name="timeout">Specifies the timeout until this request is kept alive.</param>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.ArgumentException">The specified timeout is invalid.</exception>
        /// <returns></returns>
        public Task<ILock> AcquireLockAsync(long owner, ulong resourceNameHash, LockMode mode, int timeout)
        {
            //
            // Check arguments.
            //
            if (0 > timeout && timeout != System.Threading.Timeout.Infinite)
            {
                throw new ArgumentException(SR.Error_Timeout);
            }

            if (LockMode.Free == mode)
            {
                throw new ArgumentException(SR.Error_Mode);
            }

            LockHashValue hashValue;
            var lockHashTableIndex = (int) (resourceNameHash%this.lockHashTableCount);
            var lockHashTable = this.lockHashTables[lockHashTableIndex];
            var isGranted = false;
            var isPending = false;
            var duplicate = false;
            var isUpgrade = false;

            //
            // Acquire first level lock.
            //
            lockHashTable.LockHashTableLock.EnterReadLock();

            //
            // Check to see if the lock manager can accept requests.
            //
            if (!this.status)
            {
                //
                // Release first level lock.
                //
                lockHashTable.LockHashTableLock.ExitReadLock();

                //
                // Timeout lock request immediately.
                //
                return Task.FromResult<ILock>(new LockControlBlock(this, owner, resourceNameHash, mode, timeout, LockStatus.Invalid, false));
            }

            LockMode tableLockMode = LockMode.Shared;
            bool lockHashFound = lockHashTable.LockEntries.TryGetValue(resourceNameHash, out hashValue);
            if (!lockHashFound)
            {
                lockHashTable.LockHashTableLock.ExitReadLock();
                lockHashTable.LockHashTableLock.EnterWriteLock();

                lockHashFound = lockHashTable.LockEntries.TryGetValue(resourceNameHash, out hashValue);
                tableLockMode = LockMode.Exclusive;
            }

            //
            // Find the lock resource entry, if it exists.
            //
            if (lockHashFound)
            {
                //
                // Acquire second level lock.
                //
                Monitor.Enter(hashValue.LockHashValueLock);

                //
                // Release first level lock.
                //
                switch (tableLockMode)
                {
                    case LockMode.Shared:
                        lockHashTable.LockHashTableLock.ExitReadLock();
                        break;
                    case LockMode.Exclusive:
                        lockHashTable.LockHashTableLock.ExitWriteLock();
                        break;
                    default:
                        Diagnostics.Assert(false, this.traceType, "Unhandled lock mode={0}", tableLockMode);
                        break;
                }

                //
                // Attempt to find the lock owner in the granted list.
                //
                var lockControlBlock = hashValue.LockResourceControlBlock.LocateLockClient(owner, isWaitingList: false);
                if (null == lockControlBlock)
                {
                    //
                    // Lock owner not found in the granted list. 
                    //
                    if (0 != hashValue.LockResourceControlBlock.WaitingQueue.Count)
                    {
                        //
                        // There are already waiters for this lock. The request therefore cannot be granted in fairness.
                        //
                        isPending = true;
                    }
                    else
                    {
                        //
                        // There are no waiters for the this lock. Grant only if the lock mode is compatible with the existent granted mode.
                        //
                        if (IsCompatible(mode, hashValue.LockResourceControlBlock.LockModeGranted))
                        {
                            isGranted = true;
                        }
                        else
                        {
                            isPending = true;
                        }
                    }
                }
                else
                {
                    //
                    // Lock owner found in the granted list.
                    //
                    if (mode == lockControlBlock.LockMode)
                    {
                        //
                        // This is a duplicate request from the same lock owner with the same lock mode.
                        // Grant it immediately, since it is already granted.
                        //
                        isGranted = true;
                        duplicate = true;
                    }
                    else
                    {
                        //
                        // The lock owner is requesting a new lock mode (upgrade or downgrade).
                        // There are two cases for which the request will be granted.
                        // 1. The lock mode requested is compatible with the existent granted mode.
                        // 2. There is a single lock owner in the granted list.
                        //
                        if (IsCompatible(mode, hashValue.LockResourceControlBlock.LockModeGranted) ||
                            hashValue.LockResourceControlBlock.IsSingleLockOwnerGranted(owner))
                        {
                            //
                            // Fairness might be violated in this case, in case existent waiters exist.
                            // The reason for violating fairness is that the lock is already granted to this lock owner.
                            //
                            isGranted = true;
                        }
                        else
                        {
                            isPending = true;
                            isUpgrade = true;
                        }
                    }
                }

                //
                // Internal state checks.
                //
                Diagnostics.Assert(isGranted ^ isPending, this.traceType, "The lock should be either granted or pending, but not both.");
                if (isGranted)
                {
                    //
                    // Check to see if the lock request is a duplicate request.
                    //
                    if (!duplicate)
                    {
                        //
                        // The lock request is added to the granted list and the granted mode is re-computed.
                        //
                        lockControlBlock = new LockControlBlock(this, owner, resourceNameHash, mode, timeout, LockStatus.Granted, false);
                        lockControlBlock.GrantTime = DateTime.UtcNow.Ticks;

                        //
                        // Grant the max lock mode.
                        //
                        hashValue.LockResourceControlBlock.GrantedList.Add(lockControlBlock);

                        //
                        // Set the lock resource granted lock status.
                        //
                        hashValue.LockResourceControlBlock.LockModeGranted = ConvertToMaxLockMode(
                            mode,
                            hashValue.LockResourceControlBlock.LockModeGranted);
                    }
                    else
                    {
                        //
                        // Return existent lock control block.
                        //
                        Diagnostics.Assert(null != lockControlBlock, this.traceType, "lock control block must exist");
                        lockControlBlock.LockCount++;
                    }

                    //
                    // Release second level lock.
                    //
                    Monitor.Exit(hashValue.LockHashValueLock);

                    //
                    // Return immediately.
                    //
                    return Task.FromResult<ILock>(lockControlBlock);
                }
                else
                {
                    //
                    // Check to see if this is an instant lock.
                    //
                    if (0 == timeout)
                    {
                        long oldestGrantee = hashValue.LockResourceControlBlock.GrantedList[0].Owner;
                        //
                        // Release second level lock.
                        //
                        Monitor.Exit(hashValue.LockHashValueLock);

                        //
                        // Timeout lock request immediately.
                        //
                        return Task.FromResult<ILock>(new LockControlBlock(this, owner, resourceNameHash, mode, timeout, LockStatus.Timeout, false, oldestGrantee));
                    }

                    //
                    // The lock request is added to the waiting list.
                    //
                    lockControlBlock = new LockControlBlock(this, owner, resourceNameHash, mode, timeout, LockStatus.Pending, isUpgrade);
                    if (!isUpgrade)
                    {
                        //
                        // Add the new lock control block to the waiting queue.
                        //
                        hashValue.LockResourceControlBlock.WaitingQueue.Add(lockControlBlock);
                    }
                    else
                    {
                        //
                        // Add the new lock control block before any lock control block that is not upgraded, 
                        // but after the last upgraded lock control block.
                        //
                        var index = 0;
                        for (; index < hashValue.LockResourceControlBlock.WaitingQueue.Count; index++)
                        {
                            if (!hashValue.LockResourceControlBlock.WaitingQueue[index].IsUpgraded)
                            {
                                break;
                            }
                        }

                        hashValue.LockResourceControlBlock.WaitingQueue.Insert(index, lockControlBlock);
                    }

                    //
                    // Set the waiting completion task source on the lock control block.
                    //
                    var completion = new TaskCompletionSource<ILock>();
                    lockControlBlock.Waiter = completion;

                    //
                    // Start the expiration for this lock control block.
                    //
                    lockControlBlock.StartExpire();

                    //
                    // Release second level lock.
                    //
                    Monitor.Exit(hashValue.LockHashValueLock);

                    //
                    // Done with this request. The task is pending.
                    //
                    return completion.Task;
                }
            }
            else
            {
                //
                // New lock resource being created.
                //
                hashValue = LockHashValue.Create();

                //
                // Store lock resource name with its lock control block.
                //
                lockHashTable.LockEntries.Add(resourceNameHash, hashValue);

                //
                // Create new lock control block.
                //
                var lockControlBlock = new LockControlBlock(this, owner, resourceNameHash, mode, timeout, LockStatus.Granted, false);
                lockControlBlock.GrantTime = DateTime.UtcNow.Ticks;

                //
                // Add the new lock control block to the granted list.
                //
                hashValue.LockResourceControlBlock.GrantedList.Add(lockControlBlock);

                //
                // Set the lock resource granted lock status.
                //
                hashValue.LockResourceControlBlock.LockModeGranted = ConvertToMaxLockMode(mode, LockMode.Free);

                //
                // Release first level lock.
                //
                switch (tableLockMode)
                {
                    case LockMode.Shared:
                        lockHashTable.LockHashTableLock.ExitReadLock();
                        break;
                    case LockMode.Exclusive:
                        lockHashTable.LockHashTableLock.ExitWriteLock();
                        break;
                    default:
                        Diagnostics.Assert(false, this.traceType, "Unhandled lock mode={0}", tableLockMode);
                        break;
                }

                //
                // Return immediately.
                //
                return Task.FromResult<ILock>(lockControlBlock);
            }
        }

        /// <summary>
        /// Unlocks a previously granted lock.
        /// </summary>
        /// <param name="acquiredLock">Lock to release.</param>
        /// <returns></returns>
        public UnlockStatus ReleaseLock(ILock acquiredLock)
        {
            //
            // Check arguments.
            //
            if (null == acquiredLock)
            {
                throw new ArgumentNullException(SR.Error_AcquiredLock);
            }

            var lockControlBlock = acquiredLock as LockControlBlock;
            if (null == lockControlBlock)
            {
                throw new ArgumentException(SR.Error_AcquiredLock);
            }

            LockHashValue lockHashValue;
            var lockHashTableIndex = (int) (lockControlBlock.LockResourceNameHash%this.lockHashTableCount);
            var lockHashTable = this.lockHashTables[lockHashTableIndex];

            //
            // Acquire first level lock.
            //
            lockHashTable.LockHashTableLock.EnterReadLock();

            //
            // Find the right lock resource, if it exists.
            //
            if (lockHashTable.LockEntries.TryGetValue(lockControlBlock.LockResourceNameHash, out lockHashValue))
            {
                // counter that tracks entries that are qualified for clear locks.
                var clearLocksCount = 0;
                if (0 == this.lockReleasedCleanupInProgress[lockHashTableIndex])
                {
                    foreach (var hashValue in lockHashTable.LockEntries.Values)
                    {
                        if (0 == hashValue.LockResourceControlBlock.GrantedList.Count && 0 == hashValue.LockResourceControlBlock.WaitingQueue.Count)
                        {
                            clearLocksCount++;
                        }

                        if (clearLocksCount > this.clearLocksThreshold)
                        {
                            break;
                        }
                    }
                }

                if (clearLocksCount > this.clearLocksThreshold)
                {
                    this.lockReleasedCleanupInProgress[lockHashTableIndex] = 1;
                    Task.Factory.StartNew(() => { this.ClearLocks(lockHashTableIndex); });
                }

                //
                // Acquire second level lock.
                //
                Monitor.Enter(lockHashValue.LockHashValueLock);

                //
                // Release first level lock.
                //
                lockHashTable.LockHashTableLock.ExitReadLock();

                if (!lockHashValue.LockResourceControlBlock.GrantedList.Contains(lockControlBlock))
                {
                    //
                    // Release second level lock.
                    //
                    Monitor.Exit(lockHashValue.LockHashValueLock);

                    //
                    // If no lock owner found, return immediately.
                    //
                    return UnlockStatus.NotGranted;
                }

                //
                // Decrement the lock count for this granted lock.
                //
                lockControlBlock.LockCount--;
                Diagnostics.Assert(0 <= lockControlBlock.LockCount, this.traceType, "incorrect lock count");

                //
                // Check if the lock can be removed.
                //
                if (0 == lockControlBlock.LockCount)
                {
                    //
                    // Remove the lock control block for this lock owner.
                    //
                    lockHashValue.LockResourceControlBlock.GrantedList.Remove(lockControlBlock);

                    //
                    // This lock control block cannot be reused after this call.
                    //
                    if (lockControlBlock.StopExpire())
                    {
                        lockControlBlock.Dispose();
                    }

                    //
                    // Recompute the new grantees and lock granted mode.
                    //
                    this.RecomputeLockGrantees(lockHashValue, lockControlBlock, false);
                }

                //
                // Release second level lock.
                //
                Monitor.Exit(lockHashValue.LockHashValueLock);

                return UnlockStatus.Success;
            }
            else
            {
                //
                // Release first level lock.
                //
                lockHashTable.LockHashTableLock.ExitReadLock();

                //
                // Return immediately.
                //
                return UnlockStatus.UnknownResource;
            }
        }

        /// <summary>
        /// Returns true, if the lock mode is considered sharable.
        /// </summary>
        /// <param name="mode">Lock mode to inspect.</param>
        /// <returns></returns>
        public bool IsShared(LockMode mode)
        {
            return LockMode.IntentShared == mode || LockMode.Shared == mode || LockMode.SchemaStability == mode;
        }

        /// <summary>
        /// Proceeds with the expiration of a pending lock request.
        /// </summary>
        /// <param name="lockControlBlock">Lock control block representing the pending request.</param>
        /// <returns></returns>
        internal bool ExpireLock(LockControlBlock lockControlBlock)
        {
            //
            // Internal state checks.
            //
            Diagnostics.Assert(null != lockControlBlock, this.traceType, "lock control block must be valid");

            LockHashValue lockHashValue;
            var lockHashTableIndex = (int) (lockControlBlock.LockResourceNameHash%this.lockHashTableCount);
            var lockHashTable = this.lockHashTables[lockHashTableIndex];

            //
            // Acquire first level lock.
            //
            lockHashTable.LockHashTableLock.EnterWriteLock();

            //
            // Find the righ lock resource.
            //
            if (lockHashTable.LockEntries.TryGetValue(lockControlBlock.LockResourceNameHash, out lockHashValue))
            {
                //
                // Acquire second level lock.
                //
                Monitor.Enter(lockHashValue.LockHashValueLock);

                //
                // Release first level lock.
                //
                lockHashTable.LockHashTableLock.ExitWriteLock();

                //
                // Find the lock control block for this lock owner in the waiting list.
                //
                var idx = lockHashValue.LockResourceControlBlock.WaitingQueue.IndexOf(lockControlBlock);
                if (-1 == idx)
                {
                    //
                    // Release second level lock.
                    //
                    Monitor.Exit(lockHashValue.LockHashValueLock);

                    //
                    // There must have been an unlock that granted it in the meantime or it has expired already before.
                    //
                    return false;
                }

                //
                // Internal state checks.
                //
                Diagnostics.Assert(lockControlBlock.LockStatus == LockStatus.Pending, this.traceType, "incorrect lock status");

                //
                // Remove lock owner from waiting list.
                //
                lockHashValue.LockResourceControlBlock.WaitingQueue.RemoveAt(idx);

                //
                // Clear the lock timer.
                //
                lockControlBlock.StopExpire();

                //
                // Set the correct lock status.
                //
                lockControlBlock.LockStatus = LockStatus.Timeout;
                lockControlBlock.LockCount = 0;

                // Since we have the lockHashValue lock, GrantedList should not change underneath us
                lockControlBlock.oldestGrantee = lockHashValue.LockResourceControlBlock.GrantedList[0].Owner;

                //
                // Recompute the new grantees and lock granted mode.
                //
                if (0 == idx)
                {
                    this.RecomputeLockGrantees(lockHashValue, lockControlBlock, true);
                }

                //
                // Release second level lock.
                //
                Monitor.Exit(lockHashValue.LockHashValueLock);

                //
                // Complete the waiter.
                //
                lockControlBlock.Waiter.SetResult(lockControlBlock);

                return true;
            }
            else
            {
                //
                // Release first level lock.
                //
                lockHashTable.LockHashTableLock.ExitWriteLock();
            }

            return false;
        }

        #region IDisposable Overrides

        /// <summary>
        /// 
        /// </summary>
        /// <param name="disposing"></param>
        protected override void Dispose(bool disposing)
        {
            if (!this.Disposed)
            {
                if (disposing)
                {
                    if (null != this.lockHashTables)
                    {
                        foreach (var lockHashTable in this.lockHashTables)
                        {
                            lockHashTable.Close();
                        }

                        this.lockHashTables = null;
                    }

                    if (null != this.primeLock)
                    {
                        this.primeLock = null;
                    }

                    this.Disposed = true;
                }
            }
        }

        #endregion

        /// <summary>
        /// Verifies the compatibility between and existing lock mode that was granted and
        /// the lock mode requested for the same lock resource.
        /// </summary>
        /// <param name="modeRequested"></param>
        /// <param name="modeGranted"></param>
        /// <returns></returns>
        private static bool IsCompatible(LockMode modeRequested, LockMode modeGranted)
        {
            return LockCompatibility.NoConflict == lockCompatibility[modeGranted][modeRequested];
        }

        /// <summary>
        /// Returns max lock mode of two lock modes.
        /// </summary>
        /// <param name="modeRequested">Existing lock mode.</param>
        /// <param name="modeGranted">Additional lock mode.</param>
        /// <returns></returns>
        private static LockMode ConvertToMaxLockMode(LockMode modeRequested, LockMode modeGranted)
        {
            return lockConversion[modeGranted][modeRequested];
        }

        /// <summary>
        /// Recomputes the lock granted mode for a given lock resource. 
        /// It can be called when a lock request has expired or when a granted lock is released.
        /// </summary>
        /// <param name="lockHashValue">Lock hash value containing the lock resource.</param>
        /// <param name="releasedLockControlBlock">Lock control block for the lock that is being released.</param>
        /// <param name="isExpired">Whether it is a waiter that is expiring.</param>
        private void RecomputeLockGrantees(LockHashValue lockHashValue, LockControlBlock releasedLockControlBlock, bool isExpired)
        {
            var resourceNameHash = releasedLockControlBlock.LockResourceNameHash;
            var lockHashTableIndex = (int) (resourceNameHash%this.lockHashTableCount);

            //
            // Need to find waiters that can be woken up.
            //
            var waitersWokenUpSuccess = new List<LockControlBlock>();

            //
            // Check if there is only one lock owner in the granted list
            //
            var isSingleLockOwner = lockHashValue.LockResourceControlBlock.IsSingleLockOwnerGranted(long.MinValue);

            LockMode currentGrantedMode = lockHashValue.LockResourceControlBlock.LockModeGranted;

            //
            // Validate that the granted list is consistent
            //
            if (!isExpired && lockHashValue.LockResourceControlBlock.GrantedList.Count > 0)
            {
#if DEBUG
                foreach (var grantedLockControlBlock in lockHashValue.LockResourceControlBlock.GrantedList)
                {
                    Diagnostics.Assert(grantedLockControlBlock.LockStatus == LockStatus.Granted, this.traceType, "incorret lock status");
                }

                if (!isSingleLockOwner)
                {
                    ValidateLockResourceControlBlock(lockHashValue.LockResourceControlBlock, releasedLockControlBlock);
                }
#endif

                if (currentGrantedMode == LockMode.Update && releasedLockControlBlock.LockMode == LockMode.Update)
                {
                    // If releasing an updater and granted list is not empty, then new mode is shared unless there is another same-owner updater
                    lockHashValue.LockResourceControlBlock.LockModeGranted = LockMode.Shared;
                    foreach (var grantedLockControlBlock in lockHashValue.LockResourceControlBlock.GrantedList)
                    {
                        if (grantedLockControlBlock.LockMode == LockMode.Update)
                        {
                            Diagnostics.Assert(
                                grantedLockControlBlock.LockOwner == releasedLockControlBlock.LockOwner, this.traceType,
                                "All updates lock should belong to the same owner");

                            lockHashValue.LockResourceControlBlock.LockModeGranted = LockMode.Update;
                            break;
                        }
                    }
                }

                if (isSingleLockOwner)
                {
                    // Upgrade the lock to the grantee with the highest lock mode
                    var modeToGrant = LockMode.Free;
                    foreach (var grantedLockControlBlock in lockHashValue.LockResourceControlBlock.GrantedList)
                    {
                        modeToGrant = ConvertToMaxLockMode(grantedLockControlBlock.LockMode, modeToGrant);
                        if (modeToGrant == LockMode.Exclusive)
                        {
                            // Can't go higher than exclusive, so break out early
                            break;
                        }
                    }

                    lockHashValue.LockResourceControlBlock.LockModeGranted = modeToGrant;
                }

            }

            if (lockHashValue.LockResourceControlBlock.GrantedList.Count == 0)
            {
                lockHashValue.LockResourceControlBlock.LockModeGranted = LockMode.Free;
            }

            //
            // Go over remaining waiting queue and determine if any new waiter can be granted the lock.
            //
            LockControlBlock lockControlBlock = null;
            for (var index = 0; index < lockHashValue.LockResourceControlBlock.WaitingQueue.Count; index++)
            {
                lockControlBlock = lockHashValue.LockResourceControlBlock.WaitingQueue[index];

                //
                // Internal state checks.
                //
                Diagnostics.Assert(lockControlBlock.LockStatus == LockStatus.Pending, this.traceType, "incorrect lock status");

                //
                // Check lock compatibility.
                //
                if (IsCompatible(lockControlBlock.LockMode, lockHashValue.LockResourceControlBlock.LockModeGranted) ||
                    lockHashValue.LockResourceControlBlock.IsSingleLockOwnerGranted(lockControlBlock.LockOwner))
                {
                    //
                    // Set lock status to success.
                    //
                    lockControlBlock.LockStatus = LockStatus.Granted;
                    lockControlBlock.GrantTime = DateTime.UtcNow.Ticks;

                    //
                    // Upgrade the lock mode.
                    //
                    lockHashValue.LockResourceControlBlock.LockModeGranted = ConvertToMaxLockMode(
                        lockControlBlock.LockMode,
                        lockHashValue.LockResourceControlBlock.LockModeGranted);

                    //
                    // This waiter has successfully waited for the lock.
                    //
                    waitersWokenUpSuccess.Add(lockControlBlock);
                    lockHashValue.LockResourceControlBlock.GrantedList.Add(lockControlBlock);

                    //
                    // Stop the expiration timer for this waiter.
                    //
                    lockControlBlock.StopExpire();
                }
                else
                {
                    //
                    // Cannot unblock the rest of the waiters since at least one was found 
                    // that has an incompatible lock mode.
                    //
                    break;
                }
            }

            //
            // Remove all granted waiters from the waiting queue and complete them.
            //
            for (var index = 0; index < waitersWokenUpSuccess.Count; index++)
            {
                var completedWaiter = waitersWokenUpSuccess[index];
                lockHashValue.LockResourceControlBlock.WaitingQueue.Remove(completedWaiter);
                var awakeWaiterTask = Task.Factory.StartNew(() => { completedWaiter.Waiter.SetResult(completedWaiter); });
            }

            //
            // If there are no granted or waiting lock owner, we can clean up this lock resource lazily.
            //
            if (0 == lockHashValue.LockResourceControlBlock.GrantedList.Count &&
                0 == lockHashValue.LockResourceControlBlock.WaitingQueue.Count)
            {
                // If the lock manager is closing, speed up its cleanup.
                if (!this.status)
                {
                    Task.Factory.StartNew(() => { this.ClearLocks(lockHashTableIndex); });
                }
            }
        }

        private void ValidateLockResourceControlBlock(
            LockResourceControlBlock lockResourceControlBlock,
            LockControlBlock releasedLock)
        {
            var currentGrantedMode = lockResourceControlBlock.LockModeGranted;
            var releaseMode = releasedLock.LockMode;

            if (releaseMode == LockMode.Shared && currentGrantedMode == LockMode.Shared)
            {
                // Granted list must be all readers
                foreach (var grantee in lockResourceControlBlock.GrantedList)
                {
                    Diagnostics.Assert(grantee.LockMode == LockMode.Shared, this.traceType, "Expected a shared lock");
                }
            }
            else if (releaseMode == LockMode.Shared && currentGrantedMode == LockMode.Exclusive)
            {
                Diagnostics.Assert(false, this.traceType, "There must not be any shared locks if an exclusive has been granted");
            }
            else if (releaseMode == LockMode.Shared && currentGrantedMode == LockMode.Update)
            {
                // Granted list should be many readers and same-owner updaters
                long updatesOwner = -1;

                foreach (var grantee in lockResourceControlBlock.GrantedList)
                {
                    if (grantee.LockMode == LockMode.Update)
                    {
                        if (updatesOwner == -1)
                        {
                            updatesOwner = grantee.LockOwner;
                        }

                        Diagnostics.Assert(grantee.LockOwner == updatesOwner, this.traceType, "All updates should be owned by the same owner");
                    }

                    Diagnostics.Assert(grantee.LockMode == LockMode.Shared || grantee.LockMode == LockMode.Update, this.traceType, "Lock resource with Update granted mode has invalid granted list");
                }
            }

            else if (releaseMode == LockMode.Exclusive && currentGrantedMode == LockMode.Shared)
            {
                Diagnostics.Assert(false, this.traceType, "When releasing an Exclusive lock the current granted mode should not be Shared");
            }
            else if (releaseMode == LockMode.Exclusive && currentGrantedMode == LockMode.Exclusive)
            {
                Diagnostics.Assert(false, this.traceType, "When releasing an Exclusive lock there should not be another Exclusive lock");
            }
            else if (releaseMode == LockMode.Exclusive && currentGrantedMode == LockMode.Update)
            {
                Diagnostics.Assert(false, this.traceType, "When releasing an Exclusive lock the current granted mode cannot be Update");
            }

            else if (releaseMode == LockMode.Update && currentGrantedMode == LockMode.Shared)
            {
                Diagnostics.Assert(false, this.traceType, "When releasing an Update lock the current granted mode should not be Shared");
            }
            else if (releaseMode == LockMode.Update && currentGrantedMode == LockMode.Exclusive)
            {
                Diagnostics.Assert(false, this.traceType, "Cannot have an update lock if an exclusive lock is held");
            }
            else if (releaseMode == LockMode.Update && currentGrantedMode == LockMode.Update)
            {
                // Must be readers and updaters, all updaters from the same owner
                long updatesOwner = -1;

                foreach (var grantee in lockResourceControlBlock.GrantedList)
                {
                    if (grantee.LockMode == LockMode.Update)
                    {
                        if (updatesOwner == -1)
                        {
                            updatesOwner = grantee.LockOwner;
                        }

                        Diagnostics.Assert(grantee.LockOwner == updatesOwner, this.traceType, "All updates should be owned by the same owner");
                    }

                    Diagnostics.Assert(grantee.LockMode == LockMode.Shared || grantee.LockMode == LockMode.Update,
                        this.traceType,
                        "Grantees should be readers or updaters");
                }
            }
        }

        /// <summary>
        /// Clears an unused lock resource entry.
        /// </summary>
        /// <param name="lockHashTableIndex">Lock hash table to be cleaned up.</param>
        private void ClearLocks(int lockHashTableIndex)
        {
            var lockHashTable = this.lockHashTables[lockHashTableIndex];
            var lockHashItemsToBeCleared = new List<KeyValuePair<ulong, LockHashValue>>();

            //
            // Acquire first level lock.
            //
            lockHashTable.LockHashTableLock.EnterWriteLock();

            foreach (var item in lockHashTable.LockEntries)
            {
                //
                // Acquire second level lock.
                //
                Monitor.Enter(item.Value.LockHashValueLock);

                //
                // If there are no granted or waiting lock owners, we can clean up this lock resource.
                //
                var wasCleared = false;
                if (0 == item.Value.LockResourceControlBlock.GrantedList.Count && 0 == item.Value.LockResourceControlBlock.WaitingQueue.Count)
                {
                    lockHashItemsToBeCleared.Add(item);
                    wasCleared = true;
                }

                if (!wasCleared && !this.status)
                {
                    foreach (var x in item.Value.LockResourceControlBlock.GrantedList)
                    {
                        Diagnostics.Assert(LockStatus.Granted == x.LockStatus, this.traceType, "incorrect lock status");
                    }
                }

                //
                // Release second level lock.
                //
                Monitor.Exit(item.Value.LockHashValueLock);
            }

            // Remove all cleared locks.
            foreach (var item in lockHashItemsToBeCleared)
            {
                lockHashTable.LockEntries.Remove(item.Key);
            }

            //
            // Indicate that the next clean up task can start if needed.
            //
            this.lockReleasedCleanupInProgress[lockHashTableIndex] = 0;

            //
            // Release first level lock.
            //
            lockHashTable.LockHashTableLock.ExitWriteLock();

            //
            // Dispose all cleared locks.
            //
            foreach (var item in lockHashItemsToBeCleared)
            {
                item.Value.Close();
            }
        }
    }
}
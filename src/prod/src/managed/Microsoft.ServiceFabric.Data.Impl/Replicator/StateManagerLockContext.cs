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
    using System.Fabric.Common.Tracing;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Lock context for state manager.
    /// </summary>
    internal class StateManagerLockContext : IDisposable
    {
        /// <summary>
        /// Indicates that the lock context has been disposed
        /// </summary>
        private bool isDisposed;

        /// <summary>
        /// Key lock.
        /// </summary>
        private ReaderWriterAsyncLock keylock;

        /// <summary>
        /// Lock waiter count.
        /// </summary>
        private int lockWaiterCount;

        /// <summary>
        /// Initializes a new instance of the <see cref="StateManagerTransactionContext"/> class.
        /// </summary>
        public StateManagerLockContext(Uri key, long stateProviderId, StateProviderMetadataManager metadataManager)
        {
            Utility.Assert(key != null, "key cannot be null on lock context.");
            Utility.Assert(metadataManager != null, "metadataManager cannot be null on lock context.");

            this.Key = key;
            this.keylock = new ReaderWriterAsyncLock();
            this.MetadataManager = metadataManager;
            this.StateProviderId = stateProviderId;
            this.GrantorCount = 0;
        }

        /// <summary>
        /// Gets or sets grantor count
        /// </summary>
        public int GrantorCount { get; set; }

        /// <summary>
        /// Gets key for lock context.
        /// </summary>
        public Uri Key { get; private set; }

        /// <summary>
        /// Gets or sets lock mode
        /// </summary>
        public StateManagerLockMode LockMode { get; set; }

        /// <summary>
        /// Gets the number of waiters who have requested the lock.
        /// </summary>
        public int LockWaiterCount
        {
            get { return this.lockWaiterCount; }
        }

        /// <summary>
        /// Gets or sets Metadata manager reference.
        /// </summary>
        public StateProviderMetadataManager MetadataManager { get; private set; }

        /// <summary>
        /// Gets or sets tate provider id.
        /// </summary>
        public long StateProviderId { get; set; }

        /// <summary>
        /// Acquires read lock
        /// </summary>
        public async Task AcquireReadLockAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            try
            {
                Interlocked.Increment(ref this.lockWaiterCount);

                // AsyncLock api will be fixed to take a timespan after tstore's lock manager is fixed, until then casting it.
                var lockAcquired =
                    await this.keylock.AcquireReadLockAsync((int) timeout.TotalMilliseconds, cancellationToken).ConfigureAwait(false);

                if (!lockAcquired)
                {
                    throw new TimeoutException(
                        string.Format(
                            CultureInfo.InvariantCulture,
                            SR.Error_SMLockContext_TimedOut,
                            this.Key.OriginalString));
                }

                // Set the mode only after acquiring the lock.
                this.LockMode = StateManagerLockMode.Read;
            }
            catch (Exception e)
            {
                if (e is FabricObjectClosedException)
                {
                    // Lock has been disposed.
                    if (this.isDisposed)
                    {
                        throw new ObjectDisposedException(
                            string.Format(
                                CultureInfo.InvariantCulture,
                                SR.Error_SMLockContext_Disposed,
                                this.Key.OriginalString),
                            e);
                    }
                }

                throw;
            }
            finally
            {
                Interlocked.Decrement(ref this.lockWaiterCount);
            }
        }

        /// <summary>
        /// Acquires write lock.
        /// </summary>
        public async Task AcquireWriteLockAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            try
            {
                Interlocked.Increment(ref this.lockWaiterCount);

                // AsyncLock api will be fixed to take a timespan after tstore's lock manager is fixed, until then casting it.
                var lockAcquired =
                    await this.keylock.AcquireWriteLockAsync((int) timeout.TotalMilliseconds, cancellationToken).ConfigureAwait(false);

                if (!lockAcquired)
                {
                    throw new TimeoutException(
                        string.Format(
                            CultureInfo.InvariantCulture,
                            SR.Error_SMLockContext_TimedOut,
                            this.Key.OriginalString));
                }

                // Set the mode only after acquiring the lock.
                this.LockMode = StateManagerLockMode.Write;
            }
            catch (Exception e)
            {
                if (e is FabricObjectClosedException)
                {
                    // Lock has been disposed.
                    if (this.isDisposed)
                    {
                        throw new ObjectDisposedException(
                            string.Format(
                                CultureInfo.InvariantCulture,
                                SR.Error_SMLockContext_Disposed,
                                this.Key.OriginalString),
                            e);
                    }
                }

                throw;
            }
            finally
            {
                Interlocked.Decrement(ref this.lockWaiterCount);
            }
        }

        /// <summary>
        /// Closes the lock. This is called when state manager metadata needs to be disposed.
        /// </summary>
        public void Close()
        {
            this.keylock.Close();
        }

        /// <summary>
        /// Disposes the lock. This is called when a sepcific lock needs to be removed.
        /// </summary>
        public void Dispose()
        {
            this.isDisposed = true;
            this.keylock.Close();
        }

        /// <summary>
        /// Releases the lock.
        /// </summary>
        public void Release(long transactionId)
        {
            // TODO preethas : Remove transaction id or add transaction id to acquire API it is not in sync with the acquire API
            // Release the right kind of lock
            if (this.LockMode == StateManagerLockMode.Write)
            {
                this.GrantorCount--;
                if (this.GrantorCount == 0)
                {
                    this.keylock.ReleaseWriteLock();
                    FabricEvents.Events.LockContextReleaseWriteLock(
                        this.MetadataManager.TraceType,
                        this.Key.OriginalString,
                        transactionId);

                    if (transactionId != StateManagerConstants.InvalidTransactionId)
                    {
                        HashSet<Uri> keyLockCollection = null;
                        this.MetadataManager.InflightTransactions.TryRemove(transactionId, out keyLockCollection);
                    }
                }
            }
            else
            {
                this.keylock.ReleaseReadLock();
                FabricEvents.Events.LockContextReleaseReadLock(
                    this.MetadataManager.TraceType,
                    this.Key.OriginalString,
                    transactionId);

                if (transactionId != StateManagerConstants.InvalidTransactionId)
                {
                    HashSet<Uri> keyLockCollection = null;
                    this.MetadataManager.InflightTransactions.TryRemove(transactionId, out keyLockCollection);
                }
            }
        }

        /// <summary>
        /// Release locks.
        /// </summary>
        public void Unlock(StateManagerTransactionContext transactionContext)
        {
            this.MetadataManager.Unlock(transactionContext);
        }
    }
}
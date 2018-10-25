// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;

    /// <summary>
    /// Metadata Manager for state provider.
    /// </summary>
    internal class StateProviderMetadataManager
    {
        /// <summary>
        /// Maps state provider id to active metadata.
        /// </summary>
        /// <devnote>
        /// StateProviderIdMap value changed to the Metadata from IStateProvider2 to solve the AddOperation takes in
        /// Name bug, since we can get the StateProviderId from Metadata.
        /// </devnote>
        private ConcurrentDictionary<long, Metadata> stateProviderIdMap;

        /// <summary>
        /// Trace information.
        /// </summary>
        private string traceType;

        /// <summary>
        /// Contains state manager metadata.
        /// </summary>
        private ConcurrentDictionary<Uri, Metadata> inMemoryState;

        /// <summary>
        /// Locks associated with every metadata entry.
        /// </summary>
        private ConcurrentDictionary<Uri, StateManagerLockContext> keylocks;

        /// <summary>
        /// Initializes a new instance of the <see cref="StateProviderMetadataManager"/> class.
        /// </summary>
        public StateProviderMetadataManager(ILoggingReplicator loggingReplicator, string traceType)
        {
            this.TraceType = traceType;
            this.keylocks = new ConcurrentDictionary<Uri, StateManagerLockContext>(AbsoluteUriEqualityComparer.Comparer);
            this.inMemoryState = new ConcurrentDictionary<Uri, Metadata>(AbsoluteUriEqualityComparer.Comparer);
            this.DeletedStateProviders = new ConcurrentDictionary<long, Metadata>();
            this.LoggingReplicator = loggingReplicator;
            this.CopyOrCheckpointMetadataSnapshot = new Dictionary<long, Metadata>();
            this.CopyOrCheckpointLock = new ReaderWriterLockSlim();
            this.InflightTransactions = new ConcurrentDictionary<long, HashSet<Uri>>();
            this.stateProviderIdMap = new ConcurrentDictionary<long, Metadata>();
            this.ParentToChildernStateProviderMap = new ConcurrentDictionary<long, IList<Metadata>>();
        }

        /// <summary>
        /// Gets or sets lock that protects the copy or checkpoint snapshot of metadata.
        /// </summary>
        public ReaderWriterLockSlim CopyOrCheckpointLock { get; private set; }

        /// <summary>
        /// Gets or sets the copy or checkpoint snapshot.
        /// </summary>
        public Dictionary<long, Metadata> CopyOrCheckpointMetadataSnapshot { get; set; }

        /// <summary>
        /// Gets or sets the list of state providers that have been soft deleted.
        /// </summary>
        public ConcurrentDictionary<long, Metadata> DeletedStateProviders { get; set; }

        /// <summary>
        /// Gets or sets the in-flight transactions.
        /// </summary>
        public ConcurrentDictionary<long, HashSet<Uri>> InflightTransactions { get; set; }

        /// <summary>
        /// Gets or sets the logging replicator.
        /// </summary>
        public ILoggingReplicator LoggingReplicator { get; private set; }

        /// <summary>
        /// Concurrent dictionary that maps inflight parent state providers to their children's metadata.
        /// </summary>
        public ConcurrentDictionary<long, IList<Metadata>> ParentToChildernStateProviderMap { get; set; }

        internal string TraceType
        {
            get { return this.traceType; }

            set { this.traceType = value; }
        }

        /// <summary>
        /// Acquires lock and adds key to the dictionary.
        /// </summary>
        public async Task AcquireLockAndAddAsync(
            Uri key,
            Metadata metadata,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            StateManagerLockContext stateManagerLockContext = null;
            try
            {
                if (!this.keylocks.ContainsKey(key))
                {
                    var addResult = this.keylocks.TryAdd(
                        key,
                        new StateManagerLockContext(key, metadata.StateProviderId, this));
                    Utility.Assert(
                        addResult,
                        "{0}: AcquireLockAndAddAsync: key {1} add failed because the lock key is already present in the dictionary.",
                        this.traceType,
                        key.OriginalString);
                }

                stateManagerLockContext = this.keylocks[key];
                await stateManagerLockContext.AcquireWriteLockAsync(timeout, cancellationToken).ConfigureAwait(false);
                stateManagerLockContext.GrantorCount++;
                var result = this.inMemoryState.TryAdd(key, metadata);
                Utility.Assert(
                    result,
                    "{0}: AcquireLockAndAddAsync: add has failed because the key {1} is already present.",
                    this.traceType,
                    key.OriginalString);

                // Add to id-state provider map as well.
                result = this.stateProviderIdMap.TryAdd(metadata.StateProviderId, metadata);
                Utility.Assert(
                    result,
                    "{0}: AcquireLockAndAddAsync: failed to add into stateprovideridmap because the key {1} is already present.",
                    this.traceType,
                    key.OriginalString);
            }
            finally
            {
                if (stateManagerLockContext != null)
                {
                    stateManagerLockContext.Release(StateManagerConstants.InvalidTransactionId);
                }
            }
        }

        /// <summary>
        /// Adds key and value to in-memory state. Lock is handled by the caller.
        /// </summary>
        public bool Add(Uri key, Metadata metadata)
        {
            var result = this.inMemoryState.TryAdd(key, metadata);

            if (!metadata.TransientCreate)
            {
                // Add to id-state provider map.
                var addedToStateProviderIdMap = this.stateProviderIdMap.TryAdd(
                    metadata.StateProviderId,
                    metadata);
                Utility.Assert(
                    result,
                    "{0}: MetadataManager::Add: failed to add key {1} to state provider id map. SPID: {2}", 
                    this.traceType,
                    key.OriginalString,
                    metadata.StateProviderId);
            }

            return result;
        }

        /// <summary>
        /// Checks if the dictionary has any elements.
        /// </summary>
        public bool Any()
        {
            return this.inMemoryState.Any();
        }

        /// <summary>
        /// Checks if key is present.
        /// </summary>
        /// <devnote>Read lock is not needed here since the calling api holds a lock.</devnote>
        public bool ContainsKey(Uri key)
        {
            if (this.inMemoryState.ContainsKey(key) && !this.inMemoryState[key].TransientCreate)
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Disposes key locks.
        /// </summary>
        public void Dispose()
        {
            if (this.keylocks != null)
            {
                foreach (var keylock in this.keylocks)
                {
                    var stateManagerLockContext = keylock.Value;
                    stateManagerLockContext.Close();
                }
            }
        }

        /// <summary>
        /// Checks if key is present.
        /// </summary>
        public IStateProvider2 GetActiveStateProvider(long stateproviderId)
        {
            Metadata metadata = null;
            var result = this.stateProviderIdMap.TryGetValue(stateproviderId, out metadata);
            Utility.Assert(
                result,
                "{0}: GetActiveStateProvider: Stateprovider id {1} is not present in the stateprovider-id map",
                this.traceType,
                stateproviderId);
            Utility.Assert(
                metadata != null,
                "{0}: GetActiveStateProvider: Stateprovider id {1} is null in stateprovider-id map",
                this.traceType,
                stateproviderId);

            return metadata.StateProvider;
        }

        /// <summary>
        /// Gets the list of deleted state providers.
        /// </summary>
        public IEnumerable<Metadata> GetDeletedStateProviders()
        {
            foreach (var kvp in this.DeletedStateProviders)
            {
                yield return kvp.Value;
            }
        }

        /// <summary>
        /// Gets metadata associated with the given key. (Locking is done by the caller.)
        /// </summary>
        public Metadata GetMetadata(Uri key, bool allowTransientState)
        {
            Metadata metadata = null;
            if (this.inMemoryState.TryGetValue(key, out metadata))
            {
                if (!allowTransientState && metadata.TransientCreate)
                {
                    return null;
                }
            }

            return metadata;
        }

        /// <summary>
        /// Gets metadata for the given key.
        /// </summary>
        public Metadata GetMetadata(Uri key)
        {
            Metadata metadata = null;
            if (this.inMemoryState.TryGetValue(key, out metadata))
            {
                if (metadata.TransientCreate)
                {
                    return null;
                }
            }

            return metadata;
        }

        /// <summary>
        /// Gets metadata for the given state provider id.
        /// </summary>
        public Metadata GetMetadata(long stateProviderId)
        {
            Metadata metadata = null;
            this.stateProviderIdMap.TryGetValue(stateProviderId, out metadata);
            return metadata;
        }

        /// <summary>
        /// Gets metadata collection that is not in transient state.
        /// </summary>
        public IEnumerable<Metadata> GetMetadataCollection()
        {
            foreach (var kvp in this.inMemoryState)
            {
                if(!kvp.Value.TransientCreate)
                {
                    yield return kvp.Value;
                }
            }
        }

        /// <summary>
        /// Checks if the state provider present in the deleted list
        /// </summary>
        public bool IsStateProviderDeleted(long key)
        {
            Metadata metadata = null;
            if (this.DeletedStateProviders.TryGetValue(key, out metadata))
            {
                Utility.Assert(
                    (metadata.MetadataMode == MetadataMode.DelayDelete && metadata.DeleteLsn != -1)
                    || metadata.MetadataMode == MetadataMode.FalseProgress,
                    "{0}: IsStateProviderDeleted: invalid metadata mode for state provider {1}",
                    this.traceType,
                    metadata);
                return metadata.MetadataMode == MetadataMode.DelayDelete;
            }

            return false;
        }

        /// <summary>
        /// Checks if the state provider present in the deleted list
        /// </summary>
        public bool IsStateProviderStale(long key)
        {
            if (this.DeletedStateProviders.ContainsKey(key))
            {
                var metadata = this.DeletedStateProviders[key];
                Utility.Assert(
                    (metadata.MetadataMode == MetadataMode.DelayDelete && metadata.DeleteLsn != -1)
                    || metadata.MetadataMode == MetadataMode.FalseProgress,
                    "{0}: IsStateProviderStale: invalid metadata mode for state provider {1}",
                    this.traceType,
                    metadata);
                return metadata.MetadataMode == MetadataMode.FalseProgress;
            }

            return false;
        }

        /// <summary>
        /// Locks for read.
        /// </summary>
        public async Task<StateManagerLockContext> LockForReadAsync(
            Uri key,
            long stateProviderId,
            TransactionBase transaction,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            Utility.Assert(
                key != null,
                "{0}: LockForReadAsync: key != null. SPID: {1}",
                this.traceType,
                stateProviderId);
            Utility.Assert(
                stateProviderId != 0,
                "{0}: LockForReadAsync: stateProviderId is zero. Key: {1} SPID: {2}",
                this.traceType,
                key,
                stateProviderId);
            Utility.Assert(
                transaction != null,
                "{0}: LockForReadAsync: transaction is null. Key: {1} SPID: {2}",
                this.traceType,
                key,
                stateProviderId);

            StateManagerLockContext lockContext = null;
            FabricEvents.Events.StateProviderMetadataManagerLockForRead(
                this.TraceType,
                key.OriginalString,
                stateProviderId,
                transaction.Id);

            while (true)
            {
                lockContext = this.keylocks.GetOrAdd(key, k => new StateManagerLockContext(k, stateProviderId, this));

                // If this transaction already has a write lock on the key lock, then do not acquire the lock, instead increment the grantor count.
                // This check is needed only if the transaction is holding this key in the write mode, in the read mode it can acquire the read lock again
                // as it will not cause a deadlock.
                if (lockContext.LockMode == StateManagerLockMode.Write
                    && this.DoesTransactionContainKeyLock(transaction.Id, key))
                {
                    // It is safe to do this grantor count increment outside the lock since this transaction already holds a write lock. 
                    // This lock is just treated as a write lock.
                    lockContext.GrantorCount++;
                    break;
                }

                // With the removal of the global lock, when a lock is removed during an inflight transaction this exception can happen.
                // It can be solved by either ref counting or handling disposed exception, since this is a rare situation and ref counting 
                // needs to be done every time. Gopalk has acked this recommendation.
                try
                {
                    await lockContext.AcquireReadLockAsync(timeout, cancellationToken).ConfigureAwait(false);
                    break;
                }
                catch (ObjectDisposedException)
                {
                    // Ignore and continue
                }
                catch (Exception e)
                {
                    FabricEvents.Events.MetadataManagerAcquireReadLockException(
                        this.TraceType,
                        key.OriginalString,
                        transaction.Id,
                        e.Message,
                        e.StackTrace);
                    throw;
                }
            }

            HashSet<Uri> keyLocks = this.InflightTransactions.GetOrAdd(transaction.Id, k => new HashSet<Uri>(AbsoluteUriEqualityComparer.Comparer));
            keyLocks.Add(key);

            return lockContext;
        }

        /// <summary>
        /// Locks for write.
        /// </summary>
        public async Task<StateManagerLockContext> LockForWriteAsync(
            Uri key,
            long stateProviderId,
            TransactionBase transaction,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            Utility.Assert(
                key != null,
                "{0}: LockForWriteAsync: key != null. SPID: {1}",
                this.traceType,
                stateProviderId);
            Utility.Assert(
                stateProviderId != 0,
                "{0}: LockForWriteAsync: stateProviderId is zero. Key: {1} SPID: {2}",
                this.traceType,
                key,
                stateProviderId);
            Utility.Assert(
                transaction != null,
                "{0}: LockForWriteAsync: transaction is null. Key: {1} SPID: {2}",
                this.traceType,
                key,
                stateProviderId);

            FabricEvents.Events.StateProviderMetadataManagerLockForWrite(
                this.TraceType,
                key.OriginalString,
                stateProviderId,
                transaction.Id);

            StateManagerLockContext lockContext = null;

            var incrementIndex = 0;

            while (true)
            {
                lockContext = this.keylocks.GetOrAdd(key, k => new StateManagerLockContext(k, stateProviderId, this));

                // If this transaction already has a write lock on the key lock, then do not acquire the lock, instead just increment the grantor count
                if (lockContext.LockMode == StateManagerLockMode.Write
                    && this.DoesTransactionContainKeyLock(transaction.Id, key))
                {
                    // It is safe to do this grantor count increment outside the lock since this transaction already holds this lock.
                    lockContext.GrantorCount++;
                    break;
                }
                else if (lockContext.LockMode == StateManagerLockMode.Read
                         && this.DoesTransactionContainKeyLock(transaction.Id, key))
                {
                    // If this transaction has a read lock on the key lock, then release read lock and acquire the write lock.
                    // SM only supports read committed so it is okay to release this lock.
                    lockContext.Release(transaction.Id);

                    // Increment grantor count by 2 to account for both the operations, but after lock acquisition.
                    incrementIndex = 2;
                }
                else
                {
                    incrementIndex = 1;
                }

                // With the removal of the global lock, when a lock is removed during an inflight transaction this exception can happen.
                // It can be solved by either ref counting or handling disposed exception, since this is a rare situation and ref counting
                // needs to be done every time. Gopalk has acked this recommendation.
                try
                {
                    await lockContext.AcquireWriteLockAsync(timeout, cancellationToken).ConfigureAwait(false);
                    lockContext.GrantorCount = lockContext.GrantorCount + incrementIndex;
                    break;
                }
                catch (ObjectDisposedException)
                {
                    // Ignore and continue
                }
                catch (Exception e)
                {
                    FabricEvents.Events.MetadataManagerAcquireWriteLockException(
                        this.TraceType,
                        key.OriginalString,
                        transaction.Id,
                        e.Message,
                        e.StackTrace);
                    throw;
                }
            }

            HashSet<Uri> keyLocks = this.InflightTransactions.GetOrAdd(transaction.Id, k => new HashSet<Uri>(AbsoluteUriEqualityComparer.Comparer));
            keyLocks.Add(key);

            return lockContext;
        }

        /// <summary>
        /// Move state providers from deleted list to active list.
        /// </summary>
        public void MoveStateProvidersToDeletedList()
        {
            foreach (var metadata in this.GetMetadataCollection())
            {
                var addToDeleteList = this.DeletedStateProviders.TryAdd(metadata.StateProviderId, metadata);
                Utility.Assert(
                    addToDeleteList,
                    "{0}: MoveStateProvidersToDeletedList: Failed to add state provider {1} to delete list", 
                    this.traceType,
                    metadata);
                metadata.MetadataMode = MetadataMode.FalseProgress;

                FabricEvents.Events.BeginSetCurrentLocalStateAddStateProvider(
                    this.TraceType,
                    metadata.StateProviderId);
            }

            // Empty the stateprovider id look up.
            this.stateProviderIdMap.Clear();
        }

        /// <summary>
        /// Removes data and the lock from the dictionary. 
        /// </summary>
        /// <devnote>This method is always expected to be called within a lock so no lock is acquired here</devnote>
        public void Remove(Uri key, long transactionId, TimeSpan timeout)
        {
            var lockContext = this.keylocks[key];

            Metadata metadataToBeRemoved = null;

            // remove data
            var removeData = this.inMemoryState.TryRemove(key, out metadataToBeRemoved);
            Utility.Assert(
                removeData,
                "{0}: Remove: Failed to remove data for key {1}", 
                this.traceType,
                key.OriginalString);

            // remove lock
            this.RemoveLock(lockContext, transactionId);
        }

        /// <summary>
        /// Sets transient to false. Lock is acquired by the caller.
        /// </summary>
        public void ResetTransientState(Uri key)
        {
            var metadata = this.inMemoryState[key];
            metadata.TransientCreate = false;

            // Add to id-state provider map.
            var result = this.stateProviderIdMap.TryAdd(metadata.StateProviderId, metadata);
            Utility.Assert(
                result,
                "{0}: ResetTransientState: failed to add {1} to state provider id map.",
                this.traceType,
                metadata);
        }

        /// <summary>
        /// Moves state provider from the delete list to the active list.
        /// </summary>
        public async Task<Metadata> ResurrectStateProviderAsync(long stateProviderId)
        {
            var metadata = this.DeletedStateProviders[stateProviderId];

            Utility.Assert(
                metadata != null,
                "{0}: ResurrectStateProviderAsync: Metadata {1} should be present in the deleted list",
                this.traceType,
                metadata.StateProvider.Name);
            await this.AcquireLockAndAddAsync(metadata.Name, metadata, Timeout.InfiniteTimeSpan, CancellationToken.None).ConfigureAwait(false);

            // Reset delete lsn.
            metadata.DeleteLsn = -1;

            // Reset mode to active.
            metadata.MetadataMode = MetadataMode.Active;

            // soft delete does not close sp, so do not open.
            // await metadata.StateProvider.OpenAsync().ConfigureAwait(false);
            Metadata dataToBeRemoved = null;
            var removeResult = this.DeletedStateProviders.TryRemove(stateProviderId, out dataToBeRemoved);
            Utility.Assert(
                removeResult,
                "{0}: ResurrectStateProviderAsync: failed to remove state provider {1} from deleted list ",
                this.traceType,
                stateProviderId);

            return metadata;
        }

        /// <summary>
        /// Deletes state provider key from the dictionary. This is called as a part of a transaction only. Hence lock has already been acquired and no lock is needed.
        /// </summary>
        public void SoftDelete(Uri key, MetadataMode metadataMode)
        {
            var lockContext = this.keylocks[key];

            Metadata metadata = null;
            this.inMemoryState.TryGetValue(key, out metadata);
            Utility.Assert(
                metadata != null,
                "{0}: SoftDelete: Metadata cannot be null for state provider: {1}",
                this.traceType,
                key.OriginalString);
            Utility.Assert(
                metadata.StateProvider != null,
                "{0}: SoftDelete: State provider cannot be null for {1}",
                this.traceType,
                key.OriginalString);

            // close the state provider. not closing for now, to avoid race conditions with copy.
            // await metadata.StateProvider.CloseAsync().ConfigureAwait(false);
            metadata.MetadataMode = metadataMode;
            var addMetadataToDeleteList = this.DeletedStateProviders.TryAdd(metadata.StateProviderId, metadata);
            Utility.Assert(
                addMetadataToDeleteList,
                "{0}: SoftDelete: failed to add state provider {1} to delete list",
                this.traceType,
                key.OriginalString);

            // only data is removed here, lock should not be removed here as unlock of this transaction needs the lock.
            Metadata metadataToBeRemoved = null;
            var isRemoved = this.inMemoryState.TryRemove(key, out metadataToBeRemoved);
            Utility.Assert(
                isRemoved,
                "{0}: SoftDelete: failed to remove data for key  {1}", 
                this.traceType,
                key.OriginalString);

            // Remove from id-stateprovider provider map.
            Metadata outMetadata = null;
            isRemoved = this.stateProviderIdMap.TryRemove(metadata.StateProviderId, out outMetadata);
            Utility.Assert(
                isRemoved,
                "{0}: SoftDelete: failed to remove data for key {1} from stateprovider id map",
                this.traceType,
                metadata.StateProviderId);
        }

        /// <summary>
        /// Realease all the resources associated with the transaction.
        /// </summary>
        public void Unlock(StateManagerTransactionContext transactionContext)
        {
            var lockContext = transactionContext.LockContext;
            var metadata = this.GetMetadata(lockContext.Key, true);

            if (transactionContext.OperationType == OperationType.Add)
            {
                if (metadata != null)
                {
                    if (metadata.StateProviderId == lockContext.StateProviderId)
                    {
                        if (metadata.TransientCreate)
                        {
                            // Delete lock entry and data.
                            this.Remove(lockContext.Key, transactionContext.TransactionId, Timeout.InfiniteTimeSpan);
                        }
                        else
                        {
                            // successful create.
                            lockContext.Release(transactionContext.TransactionId);

                            FabricEvents.Events.StateProviderMetadataManagerUnlock(
                                this.TraceType,
                                transactionContext.TransactionId);
                        }
                    }
                    else
                    {
                        // creation of a state provider that already exists
                        lockContext.Release(transactionContext.TransactionId);
                    }
                }
                else
                {
                    // metadata has been removed but there is a subsequent add in the same transaction.
                    this.RemoveLock(lockContext, transactionContext.TransactionId);
                }
            }
            else if (transactionContext.OperationType == OperationType.Remove)
            {
                if (metadata != null)
                {
                    if (metadata.StateProviderId == lockContext.StateProviderId)
                    {
                        if (metadata.TransientDelete)
                        {
                            metadata.TransientDelete = false;
                            lockContext.Release(transactionContext.TransactionId);
                            FabricEvents.Events.StateProviderMetadataManagerUnlock(
                                this.TraceType,
                                transactionContext.TransactionId);
                        }
                        else
                        {
                            // Delete got aborted.
                            lockContext.Release(transactionContext.TransactionId);
                        }
                    }
                }
                else if (this.DeletedStateProviders.ContainsKey(lockContext.StateProviderId))
                {
                    // state provider has been deleted (this is used on false progress as well)
                    FabricEvents.Events.StateProviderMetadataManagerUnlockDelete(
                        this.TraceType,
                        lockContext.Key.OriginalString,
                        transactionContext.TransactionId);

                    // data has beeen deleted, delete the lock entry.
                    this.RemoveLock(lockContext, transactionContext.TransactionId);
                }
                else
                {
                    // trying to delete a state provider that does not exist.
                    this.RemoveLock(lockContext, transactionContext.TransactionId);
                }
            }
            else
            {
                Utility.Assert(
                    transactionContext.OperationType == OperationType.Read,
                    "{0}: Unlock: operation type should be a read type. OperationType: {1}",
                    this.traceType,
                    transactionContext.OperationType);

                // Reading a state provider that does not exist
                if (metadata == null)
                {
                    this.RemoveLock(lockContext, transactionContext.TransactionId);
                }
                else
                {
                    transactionContext.LockContext.Release(transactionContext.TransactionId);
                }
            }
        }

        /// <summary>
        /// Gets lock context for the given key.
        /// </summary>
        /// <devnote>Exposed for testability only.</devnote>
        internal StateManagerLockContext GetLockContext(Uri key)
        {
            StateManagerLockContext lockContext = null;
            this.keylocks.TryGetValue(key, out lockContext);
            return lockContext;
        }

        private bool DoesTransactionContainKeyLock(long transactionId, Uri key)
        {
            if (transactionId == StateManagerConstants.InvalidTransactionId)
            {
                return false;
            }

            HashSet<Uri> keys = null;
            if (this.InflightTransactions.TryGetValue(transactionId, out keys))
            {
                return keys.Contains(key);
            }

            return false;
        }

        /// <summary>
        /// Removes the lock entry if there are no waiters, else just releases the lock.
        /// </summary>
        /// <param name="lockContext"></param>
        /// <param name="transactionId"></param>
        private void RemoveLock(StateManagerLockContext lockContext, long transactionId)
        {
            if (lockContext.GrantorCount == 1)
            {
                // Remove transaction id since this lock is getting disposed withoutr a release.
                if (transactionId != StateManagerConstants.InvalidTransactionId)
                {
                    HashSet<Uri> keyLockCollection = null;
                    this.InflightTransactions.TryRemove(transactionId, out keyLockCollection);
                }

                // Remove the lock, and then dispose it. 
                // Once the lock is removed from keylocks, a new lock with the same key can be created but it does not affect correctness.
                StateManagerLockContext valueTobeRemoved = null;
                var removeKey = this.keylocks.TryRemove(lockContext.Key, out valueTobeRemoved);
                Utility.Assert(
                    removeKey,
                    "{0}: RemoveLock: Failed to remove lock for key {1}",
                    this.traceType,
                    lockContext.Key.OriginalString);

                lockContext.Dispose();
                FabricEvents.Events.StateProviderMetadataManagerRemoveLock(
                    this.TraceType,
                    lockContext.Key.OriginalString,
                    transactionId);
            }
            else
            {
                lockContext.Release(transactionId);
            }
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Notifications;
    
    // Alias.
    using ReplicatorTransaction = Microsoft.ServiceFabric.Replicator.Transaction;
    using ReplicatorTransactionBase = Microsoft.ServiceFabric.Replicator.TransactionBase;

    /// <summary>
    /// Store change event handler.
    /// </summary>
    /// <typeparam name="TKey">Key type.</typeparam>
    /// <typeparam name="TValue">Value type.</typeparam>
    /// <param name="sender">Store producing the event.</param>
    /// <param name="e">Store changed notification event.</param>
    internal delegate void NotifyStoreChangedEventHandler<TKey, TValue>(object sender, EventArgs e);

    /// <summary>
    /// Specifies the type of the read.
    /// </summary>
    internal enum ReadMode : int
    {
        /// <summary>
        /// Does not read the value.
        /// </summary>
        Off =               0,

        /// <summary>
        /// Reads the value but does not put it into the in-memory cache if it was not in the in-memory cache already.
        /// </summary>
        ReadValue =         1,

        /// <summary>
        /// Reads the value and puts it into the in-memory cache if not already in cache.
        /// </summary>
        CacheResult =       2,
    }

    /// <summary>
    /// Transaction read isolation levels supported.
    /// </summary>
    internal enum StoreTransactionReadIsolationLevel : byte
    {
        Snapshot = 0,
        ReadCommittedSnapshot = 1,
        ReadCommitted = 2,
        ReadRepeatable = 4,
    }

    /// <summary>
    /// Hints for locking.
    /// </summary>
    internal enum LockingHints : long
    {
        None = 0,
        NoLock = 1,
        ReadPast = 2,
        UpdateLock = 4,
    }

    /// <summary>
    /// Types of store changed events.
    /// </summary>
    internal enum StoreChangeEventType
    {
        Item = 0,
        Store = 1,
    }

    /// <summary>
    /// Types of store item modifications.
    /// </summary>
    internal enum StoreItemChangedEventType
    {
        Added = 0,
        Updated = 1,
        Removed = 2,
    }

    /// <summary>
    /// Type of store modifications.
    /// </summary>
    internal enum StoreChangedEventType
    {
        None = 0,
        Changed = 1,
        Clear = 2,
    }

    /// <summary>
    /// Base transaction interface.
    /// </summary>
    internal interface IStoreTransaction
    {
        /// <summary>
        /// Transaction id.
        /// </summary>
        long Id { get; }

        /// <summary>
        /// Gets/Sets the default lock timeout for this store transaction.
        /// </summary>
        TimeSpan DefaultTimeout { get; set; }

        /// <summary>
        /// Read isolation level.
        /// </summary>
        StoreTransactionReadIsolationLevel Isolation { get; set; }

        /// <summary>
        /// Locking hints.
        /// </summary>
        LockingHints LockingHints { get; set; }
    }

    /// <summary>
    /// Write transaction.
    /// </summary>
    internal interface IStoreWriteTransaction : IStoreTransaction
    {
        /// <summary>
        /// Retrieves the replicator associated transaction.
        /// </summary>
        ReplicatorTransactionBase ReplicatorTransactionBase { get; }
    }

    /// <summary>
    /// Read/Write transaction.
    /// </summary>
    internal interface IStoreReadWriteTransaction : IStoreWriteTransaction
    {
    }

    /// <summary>
    /// Store specific transactions.
    /// </summary>
    internal interface IStoreTransactionProvider
    {
        /// <summary>
        /// Creates a reader/writer transaction.
        /// </summary>
        /// <param name="replicatorTransaction">Associated replicator transaction.</param>
        /// <returns></returns>
        IStoreReadWriteTransaction CreateStoreReadWriteTransaction(ReplicatorTransaction replicatorTransaction);

        /// <summary>
        /// Creates or retrieves a reader/writer transaction.
        /// </summary>
        /// <param name="replicatorTransaction">Associated replicator transaction.</param>
        /// <returns></returns>
        ConditionalValue<IStoreReadWriteTransaction> CreateOrFindTransaction(ReplicatorTransaction replicatorTransaction);
    }

    /// <summary>
    /// Used for building inverted indexes.
    /// </summary>
    /// <typeparam name="TValue">Value used for extraction.</typeparam>
    /// <typeparam name="TExtracted">Extracted value type.</typeparam>
    internal interface IValueExtractor<TValue, TExtracted>
    {
        /// <summary>
        /// Given a value, it extracts its parts.
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        IList<TExtracted> Extract(TValue value);
    }

    /// <summary>
    /// Store public interface.
    /// </summary>
    internal interface IStore<TKey, TValue>
    {
        /// <summary>
        /// Gets the number of key/value pairs contained in the store.
        /// </summary>
        /// <remarks>
        /// This property does not have transactional semantics. 
        /// It represents the BEST EFFORT number of items in the store at the moment when the property was accessed.
        /// </remarks>
        long Count { get; }

        /// <summary>
        /// Adds a new key and value into the store.
        /// </summary>
        /// <param name="transaction">Store transaction that owns this operation.</param>
        /// <param name="key">Key to insert.</param>
        /// <param name="value">Value to insert.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>The version of the newly inserted key/value pair.</returns>
        Task AddAsync(
            IStoreWriteTransaction transaction,
            TKey key,
            TValue value,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Updates the value for a given key.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Value to update.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>Version of the newly updated key/value pair.</returns>
        Task UpdateAsync(
            IStoreWriteTransaction transaction,
            TKey key,
            TValue value,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Conditionally updates the value of a given key based on a previously obtained version.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Value to update.</param>
        /// <param name="conditionalCheckVersion">Version to use for the conditional check.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>True and new version of the newly updated key/value pair, if conditional check succeeded.
        /// False and existent version, if the conditional check failed.</returns>
        Task<bool> TryUpdateAsync(
            IStoreWriteTransaction transaction,
            TKey key,
            TValue value,
            long conditionalCheckVersion,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Conditionally updates the value of a given key based on a previously obtained version.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="newValue">Value to update.</param>
        /// <param name="comparisonValue">Value to use for the conditional check.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>True and new version of the newly updated key/value pair, if conditional check succeeded.
        /// False and existent version, if the conditional check failed.</returns>
        Task<bool> TryUpdateAsync(
            IStoreWriteTransaction transaction,
            TKey key,
            TValue newValue,
            TValue comparisonValue,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Deletes a key and a value from the store.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to delete.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>True and new version, if the delete has been performed.</returns>
        Task<ConditionalValue<TValue>> TryRemoveAsync(
            IStoreWriteTransaction transaction,
            TKey key,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Conditionally deletes a key from the store.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to delete.</param>
        /// <param name="conditionalCheckVersion">Conditional version to be used for the delete.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>True and new version, if the conditional check succeeded.
        /// False and existent version, if the conditional check failed.</returns>
        Task<ConditionalValue<TValue>> TryRemoveAsync(
            IStoreWriteTransaction transaction,
            TKey key,
            long conditionalCheckVersion,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Removes all data from the store.
        /// </summary>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task ClearAsync(TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Gets the value for an existent key or adds the key and value if key does not exist.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to insert.</param>
        /// <param name="value">Value to insert.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>If the key does not exist, it returns the version and the value of the newly inserted key/value pair.
        /// If the key already exists, it returns the version and value of the existent key/value pair.</returns>
        Task<TValue> GetOrAddAsync(
            IStoreWriteTransaction transaction,
            TKey key,
            TValue value,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Gets the value for an existent key or adds the key and value if key does not exist.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to insert.</param>
        /// <param name="valueFactory">Delegate that produces the value to insert. This delegate will be called under lock.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>If the key does not exist, it returns the version and the value of the newly inserted key/value pair.
        /// If the key already exists, it returns the version and value of the existent key/value pair.</returns>
        Task<TValue> GetOrAddAsync(
            IStoreWriteTransaction transaction,
            TKey key,
            Func<TKey, TValue> valueFactory,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Inserts or updates a given key.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to insert/update.</param>
        /// <param name="addValue">Value to insert/update.</param>
        /// <param name="timeout"></param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <param name="updateValue"></param>
        /// <returns>If the key exists, it updates its value to the new value. If the key does not exist, it is inserted.
        /// Returns the version of the exitent or newly added key.</returns>
        Task AddOrUpdateAsync(
            IStoreWriteTransaction transaction,
            TKey key,
            TValue addValue,
            TValue updateValue,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Inserts or updates a given key.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to insert/update.</param>
        /// <param name="addValue">Value to add.</param>
        /// <param name="updateValueFactory">Delegate that creates the value to update.</param>
        /// <param name="timeout"></param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>If the key exists, it updates its value to the new value. If the key does not exist, it is inserted.
        /// Returns the version of the exitent or newly added key.</returns>
        Task<TValue> AddOrUpdateAsync(
            IStoreWriteTransaction transaction,
            TKey key,
            TValue addValue,
            Func<TKey, TValue, TValue> updateValueFactory,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Inserts or updates a given key.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to insert/update.</param>
        /// <param name="addValueFactory">Delegate that creates the value to add.</param>
        /// <param name="updateValueFactory">Delegate that creates the value to update.</param>
        /// <param name="timeout"></param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>If the key exists, it updates its value to the new value. If the key does not exist, it is inserted.
        /// Returns the version of the exitent or newly added key.</returns>
        Task<TValue> AddOrUpdateAsync(
            IStoreWriteTransaction transaction,
            TKey key,
            Func<TKey, TValue> addValueFactory,
            Func<TKey, TValue, TValue> updateValueFactory,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Updates the value for a given key and returns its value before the update. Blind update.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Value to update.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>New version and old value.</returns>
        Task<TValue> UpdateWithOutputAsync(
            IStoreWriteTransaction transaction,
            TKey key,
            TValue value,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Updates the value for a given key and returns its value before the update. Blind update.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="updateValueFactory">Delegate that provides the value to update.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>New version and old value.</returns>
        Task<TValue> UpdateWithOutputAsync(
            IStoreWriteTransaction transaction,
            TKey key,
            Func<TKey, TValue, TValue> updateValueFactory,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Determines whether the store contains the specified key.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>True and existent version of the item with that key, if it exists.</returns>
        Task<bool> ContainsKeyAsync(
            IStoreTransaction transaction,
            TKey key,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Gets the value for a given key.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>True, existent version, and existent value of the item with that key, if it exists.</returns>
        Task<ConditionalValue<TValue>> GetAsync(
            IStoreTransaction transaction,
            TKey key,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<ConditionalValue<KeyValuePair<TKey, TValue>>> GetNextAsync(
            IStoreTransaction transaction,
            TKey key,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Creates an ordered enumerable over the store items.
        /// </summary>
        /// <param name="storeTransaction">Transaction that owns this operation.</param>
        /// <param name="isOrdered">Specifies if the enumerable is required to be ordered.</param>
        /// <param name="readMode">Specifies the read mode.</param>
        /// <returns></returns>
        Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(IStoreTransaction storeTransaction, bool isOrdered, ReadMode readMode);

        /// <summary>
        /// Creates an async enumerator over the store items.
        /// </summary>
        /// <param name="storeTransaction">Transaction that owns this operation.</param>
        /// <param name="isOrdered">Specifies if the enumerable is required to be ordered.</param>
        /// <param name="readMode">Specifies the read mode.</param>
        /// <returns></returns>
        Task<IAsyncEnumerator<KeyValuePair<TKey, TValue>>> CreateAsyncEnumeratorAsync(IStoreTransaction storeTransaction, bool isOrdered, ReadMode readMode);

        /// <summary>
        /// Creates an ordered enumerable over the store items within a specified range.
        /// </summary>
        /// <param name="storeTransaction">Transaction that owns this operation.</param>
        /// <param name="firstKey">Key range start.</param>
        /// <param name="lastKey">Key range end.</param>
        /// <param name="isOrdered">Specifies if the enumerable is required to be ordered.</param>
        /// <param name="readMode">Specifies the read mode.</param>
        /// <returns></returns>
        Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(IStoreTransaction storeTransaction, TKey firstKey, TKey lastKey, bool isOrdered, ReadMode readMode);

        /// <summary>
        /// Creates an async enumerator over the store items.
        /// </summary>
        /// <param name="storeTransaction">Transaction that owns this operation.</param>
        /// <param name="firstKey">Key range start.</param>
        /// <param name="lastKey">Key range end.</param>
        /// <param name="isOrdered">Specifies if the enumerable is required to be ordered.</param>
        /// <param name="readMode">Specifies the read mode.</param>
        /// <returns></returns>
        Task<IAsyncEnumerator<KeyValuePair<TKey, TValue>>> CreateAsyncEnumeratorAsync(IStoreTransaction storeTransaction, TKey firstKey, TKey lastKey, bool isOrdered, ReadMode readMode);

        /// <summary>
        /// Creates an ordered enumerable over the store items.
        /// </summary>
        /// <param name="storeTransaction">Transaction that owns this operation.</param>
        /// <param name="filter">Key filter routine.</param>
        /// <param name="isOrdered">Specifies if the enumerable is required to be ordered.</param>
        /// <param name="readMode">Specifies the read mode.</param>
        /// <returns></returns>
        Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(IStoreTransaction storeTransaction, Func<TKey, bool> filter, bool isOrdered, ReadMode readMode);

        /// <summary>
        /// Creates an async enumerator over the store items.
        /// </summary>
        /// <param name="storeTransaction">Transaction that owns this operation.</param>
        /// <param name="filter">Key filter routine.</param>
        /// <param name="isOrdered">Specifies if the enumerable is required to be ordered.</param>
        /// <param name="readMode">Specifies the read mode.</param>
        /// <returns></returns>
        Task<IAsyncEnumerator<KeyValuePair<TKey, TValue>>> CreateAsyncEnumeratorAsync(IStoreTransaction storeTransaction, Func<TKey, bool> filter, bool isOrdered, ReadMode readMode);

        /// <summary>
        /// Creates an ordered enumerable over the store items within a specified range.
        /// </summary>
        /// <param name="storeTransaction">Transaction that owns this operation.</param>
        /// <param name="firstKey">Key range start.</param>
        /// <param name="lastKey">Key range end.</param>
        /// <param name="filter">Key filter routine.</param>
        /// <param name="isOrdered">Specifies if the enumerable is required to be ordered.</param>
        /// <param name="readMode">Specifies the read mode.</param>
        /// <returns></returns>
        Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(IStoreTransaction storeTransaction, TKey firstKey, TKey lastKey, Func<TKey, bool> filter, bool isOrdered, ReadMode readMode);

        /// <summary>
        /// Creates an ordered enumerable over the store items within a specified range.
        /// </summary>
        /// <param name="storeTransaction">Transaction that owns this operation.</param>
        /// <param name="firstKey">Key range start.</param>
        /// <param name="lastKey">Key range end.</param>
        /// <param name="filter">Key filter routine.</param>
        /// <param name="isOrdered">Specifies if the enumerable is required to be ordered.</param>
        /// <param name="readMode">Specifies the read mode.</param>
        /// <returns></returns>
        Task<IAsyncEnumerator<KeyValuePair<TKey, TValue>>> CreateAsyncEnumeratorAsync(IStoreTransaction storeTransaction, TKey firstKey, TKey lastKey, Func<TKey, bool> filter, bool isOrdered, ReadMode readMode);

        /// <summary>
        /// Removes all the keys in the given store.
        /// </summary>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentException">Invalid store transaction or invalid timeout.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <returns>True,if keys are deleted, false otherwise.</returns>
        Task<bool> RemoveAllAsync(TimeSpan timeout, CancellationToken cancellationToken);
    }

    /// <summary>
    /// Store changes notification interface.
    /// </summary>
    /// <typeparam name="TKey">Key type.</typeparam>
    /// <typeparam name="TValue">Value type.</typeparam>
    internal interface INotifyDictionaryChanged<TKey, TValue>
    {
        /// <summary>
        /// Notification handler for store changes.
        /// </summary>
        event EventHandler<NotifyDictionaryChangedEventArgs<TKey, TValue>> DictionaryChanged;
    }

    /// <summary>
    /// Exposes load information for the current store.
    /// </summary>
    internal interface ILoadMetricProvider
    {
        IReadOnlyDictionary<string, long> LoadMetrics { get; }
    }
}
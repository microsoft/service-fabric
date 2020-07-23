// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.StateProviders
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Diagnostics;
    using System.Collections.Specialized;
    using System.IO;
    using System.Text;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Data.Common;
    using System.Fabric.Data.Btree;
    using System.Fabric.Data.LockManager;
    using System.Fabric.Data.TransactionManager;
    using System.Fabric.Data.Replicator;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// Async dictionary.
    /// </summary>
    /// <typeparam name="K">The type of keys in the dictionary.</typeparam>
    /// <typeparam name="V">The type of values in the dictionary.</typeparam>
    public interface IDictionaryAsync<K, V>
    {
        /// <summary>
        /// Adds a key and value to the dictionary.
        /// </summary>
        /// <param name="key">Key to add.</param>
        /// <param name="value">Value to add.</param>
        /// <returns></returns>
        Task AddAsync(K key, V value);

        /// <summary>
        /// Removes the element with the specified key from the ditionary.
        /// </summary>
        /// <param name="key">Key to remove.</param>
        /// <returns></returns>
        Task<bool> RemoveAsync(K key);

        /// <summary>
        /// Removes all items from the dictionary.
        /// </summary>
        /// <returns></returns>
        Task ClearAsync();

        /// <summary>
        /// Gets the value associated with the specified key.
        /// </summary>
        /// <param name="key">The key whose value to get.</param>
        /// <returns></returns>
        Task<Tuple<bool, V>> TryGetValueAsync(K key);

        /// <summary>
        /// Conditionally updates the value associated with the specified key.
        /// </summary>
        /// <param name="key">Key to update.</param>
        /// <param name="newValue">New value.</param>
        /// <param name="comparisonValue">Comparison value.</param>
        /// <returns></returns>
        Task<bool> TryUpdateAsync(K key, V newValue, V comparisonValue);

        /// <summary>
        /// Removes the element with the specified key from the dictionary.
        /// </summary>
        /// <param name="key">Key to remove.</param>
        /// <returns></returns>
        Task<Tuple<bool, V>> TryRemoveAsync(K key);

        /// <summary>
        /// Adds a key and value to the dictionary.
        /// </summary>
        /// <param name="key">Key to add.</param>
        /// <param name="value">Value to add.</param>
        /// <returns></returns>
        Task<bool> TryAddAsync(K key, V value);

        /// <summary>
        /// Used to add a key/value pair to the dictionary if the key does not already exist. If the key exists, the existent value is returned.
        /// </summary>
        /// <param name="key">Key to add or retrieve.</param>
        /// <param name="value">Value to add.</param>
        /// <returns></returns>
        Task<V> GetOrAddAsync(K key, V value);

        /// <summary>
        /// Adds a key/value pair to the dictionary by using the specified function, if the key does not already exist.
        /// </summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key.</param>
        /// <returns></returns>
        Task<V> GetOrAddAsync(K key, Func<K, V> valueFactory);

        /// <summary>
        /// Adds a key/value pair to the dictionary if the key does not already exist, or updated a key/value pair in the dictionary if the key already exists. 
        /// </summary>
        /// <param name="key">Key to add or update.</param>
        /// <param name="value">Value to add or update.</param>
        /// <returns></returns>
        Task<V> AddOrUpdateAsync(K key, V value);

        /// <summary>
        /// Uses the specified functions to add a key/value pair to the dictionary if the key does not already exist, 
        /// or to update a key/value pair in the dictionary if the key already exists.
        /// </summary>
        /// <param name="key">Key to add or update.</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <returns></returns>
        Task<V> AddOrUpdateAsync(K key, Func<K, V> addValueFactory, Func<K, V, V> updateValueFactory);

        /// <summary>
        /// Updates the value of an existent key and returns the old value.
        /// </summary>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Updated value.</param>
        /// <returns></returns>
        Task<V> UpdateAsync(K key, V value);

        /// <summary>
        /// Adds a key and value to the dictionary.
        /// </summary>
        /// <param name="key">Key to add.</param>
        /// <param name="value">Value to add.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task AddAsync(K key, V value, TimeSpan timeout);

        /// <summary>
        /// Removes the element with the specified key from the ditionary.
        /// </summary>
        /// <param name="key">Key to remove.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<bool> RemoveAsync(K key, TimeSpan timeout);

        /// <summary>
        /// Removes all items from the dictionary.
        /// </summary>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task ClearAsync(TimeSpan timeout);

        /// <summary>
        /// Gets the value associated with the specified key.
        /// </summary>
        /// <param name="key">The key whose value to get.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<Tuple<bool, V>> TryGetValueAsync(K key, TimeSpan timeout);

        /// <summary>
        /// Conditionally updates the value associated with the specified key.
        /// </summary>
        /// <param name="key">Key to update.</param>
        /// <param name="newValue">New value.</param>
        /// <param name="comparisonValue">Comparison value.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<bool> TryUpdateAsync(K key, V newValue, V comparisonValue, TimeSpan timeout);

        /// <summary>
        /// Removes the element with the specified key from the dictionary.
        /// </summary>
        /// <param name="key">Key to remove.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<Tuple<bool, V>> TryRemoveAsync(K key, TimeSpan timeout);

        /// <summary>
        /// Adds a key and value to the dictionary.
        /// </summary>
        /// <param name="key">Key to add.</param>
        /// <param name="value">Value to add.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<bool> TryAddAsync(K key, V value, TimeSpan timeout);

        /// <summary>
        /// Used to add a key/value pair to the dictionary if the key does not already exist. If the key exists, the existent value is returned.
        /// </summary>
        /// <param name="key">Key to add or retrieve.</param>
        /// <param name="value">Value to add.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<V> GetOrAddAsync(K key, V value, TimeSpan timeout);

        /// <summary>
        /// Adds a key/value pair to the dictionary by using the specified function, if the key does not already exist.
        /// </summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<V> GetOrAddAsync(K key, Func<K, V> valueFactory, TimeSpan timeout);

        /// <summary>
        /// Adds a key/value pair to the dictionary if the key does not already exist, or updated a key/value pair in the dictionary if the key already exists. 
        /// </summary>
        /// <param name="key">Key to add or update.</param>
        /// <param name="value">Value to add or update.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<V> AddOrUpdateAsync(K key, V value, TimeSpan timeout);

        /// <summary>
        /// Uses the specified functions to add a key/value pair to the dictionary if the key does not already exist, 
        /// or to update a key/value pair in the dictionary if the key already exists.
        /// </summary>
        /// <param name="key">Key to add or update.</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<V> AddOrUpdateAsync(K key, Func<K, V> addValueFactory, Func<K, V, V> updateValueFactory, TimeSpan timeout);

        /// <summary>
        /// Updates the value of an existent key and returns the old value.
        /// </summary>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Updated value.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<V> UpdateAsync(K key, V value, TimeSpan timeout);

        /// <summary>
        /// Adds a key and value to the dictionary.
        /// </summary>
        /// <param name="key">Key to add.</param>
        /// <param name="value">Value to add.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task AddAsync(K key, V value, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Removes the element with the specified key from the ditionary.
        /// </summary>
        /// <param name="key">Key to remove.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<bool> RemoveAsync(K key, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Removes all items from the dictionary.
        /// </summary>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task ClearAsync(TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Gets the value associated with the specified key.
        /// </summary>
        /// <param name="key">The key whose value to get.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<Tuple<bool, V>> TryGetValueAsync(K key, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Conditionally updates the value associated with the specified key.
        /// </summary>
        /// <param name="key">Key to update.</param>
        /// <param name="newValue">New value.</param>
        /// <param name="comparisonValue">Comparison value.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<bool> TryUpdateAsync(K key, V newValue, V comparisonValue, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Removes the element with the specified key from the dictionary.
        /// </summary>
        /// <param name="key">Key to remove.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<Tuple<bool, V>> TryRemoveAsync(K key, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Adds a key and value to the dictionary.
        /// </summary>
        /// <param name="key">Key to add.</param>
        /// <param name="value">Value to add.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<bool> TryAddAsync(K key, V value, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Used to add a key/value pair to the dictionary if the key does not already exist. If the key exists, the existent value is returned.
        /// </summary>
        /// <param name="key">Key to add or retrieve.</param>
        /// <param name="value">Value to add.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<V> GetOrAddAsync(K key, V value, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Adds a key/value pair to the dictionary by using the specified function, if the key does not already exist.
        /// </summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<V> GetOrAddAsync(K key, Func<K, V> valueFactory, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Adds a key/value pair to the dictionary if the key does not already exist, or updated a key/value pair in the dictionary if the key already exists. 
        /// </summary>
        /// <param name="key">Key to add or update.</param>
        /// <param name="value">Value to add or update.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<V> AddOrUpdateAsync(K key, V value, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Uses the specified functions to add a key/value pair to the dictionary if the key does not already exist, 
        /// or to update a key/value pair in the dictionary if the key already exists.
        /// </summary>
        /// <param name="key">Key to add or update.</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<V> AddOrUpdateAsync(K key, Func<K, V> addValueFactory, Func<K, V, V> updateValueFactory, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Updates the value of an existent key and returns the old value.
        /// </summary>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Updated value.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<V> UpdateAsync(K key, V value, TimeSpan timeout, CancellationToken cancellationToken);
    }

    /// <summary>
    /// Async collection.
    /// </summary>
    /// <typeparam name="T">The type of the elements in the collection.</typeparam>
    public interface ICollectionAsync<T>
    {
        /// <summary>
        /// Adds an item to the collection.
        /// </summary>
        /// <param name="item">Item to add.</param>
        /// <returns></returns>
        Task AddAsync(T item);

        /// <summary>
        /// Adds the specified key and value to the dictionary.
        /// </summary>
        /// <param name="item">The structure representing the key and value to add.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <returns></returns>
        Task AddAsync(T item, TimeSpan timeout);

        /// <summary>
        /// Adds the specified key and value to the dictionary.
        /// </summary>
        /// <param name="item">The structure representing the key and value to add.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task AddAsync(T item, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Removes an item from the collection.
        /// </summary>
        /// <param name="item">Item to remove.</param>
        /// <returns></returns>
        Task<bool> RemoveAsync(T item);

        /// <summary>
        /// Removes an item from the collection.
        /// </summary>
        /// <param name="item">Item to remove.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <returns></returns>
        Task<bool> RemoveAsync(T item, TimeSpan timeout);

        /// <summary>
        /// Removes an item from the collection.
        /// </summary>
        /// <param name="item">Item to remove.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<bool> RemoveAsync(T item, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Removes all items from the collection.
        /// </summary>
        /// <returns></returns>
        Task ClearAsync();

        /// <summary>
        /// Removes all items from the collection.
        /// </summary>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task ClearAsync(TimeSpan timeout);

        /// <summary>
        /// Removes all items from the collection.
        /// </summary>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task ClearAsync(TimeSpan timeout, CancellationToken cancellationToken);
    }

    /// <summary>
    /// Async atomic dictionary.
    /// </summary>
    /// <typeparam name="K">The type of keys in the dictionary.</typeparam>
    /// <typeparam name="V">The type of values in the dictionary.</typeparam>
    public interface IAtomicDictionaryAsync<K, V>
    {
        /// <summary>
        /// Adds a key and value to the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to add.</param>
        /// <param name="value">Value to add.</param>
        /// <returns></returns>
        Task AddAsync(long atomicGroupId, K key, V value);

        /// <summary>
        /// Removes the element with the specified key from the ditionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to remove.</param>
        /// <returns></returns>
        Task<bool> RemoveAsync(long atomicGroupId, K key);

        /// <summary>
        /// Adds a set of items to the dictionary. All or none are added.
        /// </summary>
        /// <param name="items">Items to be added.</param>
        /// <returns></returns>
        Task AddAsync(IEnumerable<KeyValuePair<K, V>> items);

        /// <summary>
        /// Removes a set of keys from the ditionary.
        /// </summary>
        /// <param name="keys">Keys to remove.</param>
        /// <returns></returns>
        Task<bool> RemoveAsync(IEnumerable<K> keys);

        /// <summary>
        /// Updates a set of keys to a given value.
        /// </summary>
        /// <param name="keys">Keys to remove.</param>
        /// <param name="value">New value for each key.</param>
        /// <returns></returns>
        Task UpdateAsync(IEnumerable<K> keys, V value);

        /// <summary>
        /// Conditionally updates the value of a given key.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="newValue">New value.</param>
        /// <param name="comparisonValue">Value used for comparison.</param>
        /// <returns></returns>
        Task<bool> TryUpdateAsync(long atomicGroupId, K key, V newValue, V comparisonValue);

        /// <summary>
        /// Removes a key from the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to remove.</param>
        /// <returns></returns>
        Task<Tuple<bool, V>> TryRemoveAsync(long atomicGroupId, K key);

        /// <summary>
        /// Adds a key and a value to the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to add.</param>
        /// <param name="value">Value to add.</param>
        /// <returns></returns>
        Task<bool> TryAddAsync(long atomicGroupId, K key, V value);

        /// <summary>
        /// Used to add a key/value pair to the dictionary if the key does not already exist. If the key exists, the existent value is returned.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to add or retrieve.</param>
        /// <param name="value">Value to add.</param>
        /// <returns></returns>
        Task<V> GetOrAddAsync(long atomicGroupId, K key, V value);

        /// <summary>
        /// Adds a key/value pair to the dictionary if the key does not already exist, or updated a key/value pair in the dictionary if the key already exists. 
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to add or update.</param>
        /// <param name="value">Value to add or update.</param>
        /// <returns></returns>
        Task<V> AddOrUpdateAsync(long atomicGroupId, K key, V value);

        /// <summary>
        /// Updates the value of an existent key and returns the old value.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Updated value.</param>
        /// <returns></returns>
        Task<V> UpdateAsync(long atomicGroupId, K key, V value);

        /// <summary>
        /// Updates a set of keys to new values. All keys are updated or none is updated.
        /// </summary>
        /// <param name="keys">Keys to update.</param>
        /// <param name="values">Values to update.</param>
        /// <returns></returns>
        Task UpdateAsync(IEnumerable<K> keys, IEnumerable<V> values);

        /// <summary>
        /// Replaces an existent key to a new key with the same value.
        /// </summary>
        /// <param name="existentKey">Key to replace.</param>
        /// <param name="newKey">New key.</param>
        /// <returns></returns>
        Task<bool> ReplaceAsync(K existentKey, K newKey);

        /// <summary>
        /// Replaces an existent key to a new key and value and returns the value of the old key.
        /// </summary>
        /// <param name="existentKey">Existent key.</param>
        /// <param name="newKey">New key.</param>
        /// <param name="newValue">New value.</param>
        /// <returns></returns>
        Task<Tuple<bool, V>> ReplaceAsync(K existentKey, K newKey, V newValue);
        
        /// <summary>
        /// Adds a key and value to the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to add.</param>
        /// <param name="value">Value to add.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task AddAsync(long atomicGroupId, K key, V value, TimeSpan timeout);

        /// <summary>
        /// Removes the element with the specified key from the ditionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to remove.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<bool> RemoveAsync(long atomicGroupId, K key, TimeSpan timeout);

        /// <summary>
        /// Adds a set of items to the dictionary. All or none are added.
        /// </summary>
        /// <param name="items">Items to be added.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task AddAsync(IEnumerable<KeyValuePair<K, V>> items, TimeSpan timeout);

        /// <summary>
        /// Removes a set of keys from the ditionary.
        /// </summary>
        /// <param name="keys">Keys to remove.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<bool> RemoveAsync(IEnumerable<K> keys, TimeSpan timeout);

        /// <summary>
        /// Updates a set of keys to a given value.
        /// </summary>
        /// <param name="keys">Keys to remove.</param>
        /// <param name="value">New value for each key.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task UpdateAsync(IEnumerable<K> keys, V value, TimeSpan timeout);

        /// <summary>
        /// Conditionally updates the value of a given key.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="newValue">New value.</param>
        /// <param name="comparisonValue">Value used for comparison.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<bool> TryUpdateAsync(long atomicGroupId, K key, V newValue, V comparisonValue, TimeSpan timeout);

        /// <summary>
        /// Removes a key from the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to remove.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<Tuple<bool, V>> TryRemoveAsync(long atomicGroupId, K key, TimeSpan timeout);

        /// <summary>
        /// Adds a key and a value to the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to add.</param>
        /// <param name="value">Value to add.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<bool> TryAddAsync(long atomicGroupId, K key, V value, TimeSpan timeout);

        /// <summary>
        /// Used to add a key/value pair to the dictionary if the key does not already exist. If the key exists, the existent value is returned.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to add or retrieve.</param>
        /// <param name="value">Value to add.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<V> GetOrAddAsync(long atomicGroupId, K key, V value, TimeSpan timeout);

        /// <summary>
        /// Adds a key/value pair to the dictionary if the key does not already exist, or updated a key/value pair in the dictionary if the key already exists. 
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to add or update.</param>
        /// <param name="value">Value to add or update.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<V> AddOrUpdateAsync(long atomicGroupId, K key, V value, TimeSpan timeout);

        /// <summary>
        /// Updates the value of an existent key and returns the old value.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Updated value.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<V> UpdateAsync(long atomicGroupId, K key, V value, TimeSpan timeout);

        /// <summary>
        /// Updates a set of keys to new values. All keys are updated or none is updated.
        /// </summary>
        /// <param name="keys">Keys to update.</param>
        /// <param name="values">Values to update.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task UpdateAsync(IEnumerable<K> keys, IEnumerable<V> values, TimeSpan timeout);

        /// <summary>
        /// Replaces an existent key to a new key with the same value.
        /// </summary>
        /// <param name="existentKey">Key to replace.</param>
        /// <param name="newKey">New key.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<bool> ReplaceAsync(K existentKey, K newKey, TimeSpan timeout);

        /// <summary>
        /// Replaces an existent key to a new key and value and returns the value of the old key.
        /// </summary>
        /// <param name="existentKey">Existent key.</param>
        /// <param name="newKey">New key.</param>
        /// <param name="newValue">New value.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<Tuple<bool, V>> ReplaceAsync(K existentKey, K newKey, V newValue, TimeSpan timeout);

        /// <summary>
        /// Adds a key and value to the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to add.</param>
        /// <param name="value">Value to add.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task AddAsync(long atomicGroupId, K key, V value, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Removes the element with the specified key from the ditionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to remove.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<bool> RemoveAsync(long atomicGroupId, K key, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Adds a set of items to the dictionary. All or none are added.
        /// </summary>
        /// <param name="items">Items to be added.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task AddAsync(IEnumerable<KeyValuePair<K, V>> items, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Removes a set of keys from the ditionary.
        /// </summary>
        /// <param name="keys">Keys to remove.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<bool> RemoveAsync(IEnumerable<K> keys, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Updates a set of keys to a given value.
        /// </summary>
        /// <param name="keys">Keys to remove.</param>
        /// <param name="value">New value for each key.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task UpdateAsync(IEnumerable<K> keys, V value, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Conditionally updates the value of a given key.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="newValue">New value.</param>
        /// <param name="comparisonValue">Value used for comparison.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<bool> TryUpdateAsync(long atomicGroupId, K key, V newValue, V comparisonValue, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Removes a key from the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to remove.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<Tuple<bool, V>> TryRemoveAsync(long atomicGroupId, K key, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Adds a key and a value to the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to add.</param>
        /// <param name="value">Value to add.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<bool> TryAddAsync(long atomicGroupId, K key, V value, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Used to add a key/value pair to the dictionary if the key does not already exist. If the key exists, the existent value is returned.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to add or retrieve.</param>
        /// <param name="value">Value to add.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<V> GetOrAddAsync(long atomicGroupId, K key, V value, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Adds a key/value pair to the dictionary if the key does not already exist, or updated a key/value pair in the dictionary if the key already exists. 
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to add or update.</param>
        /// <param name="value">Value to add or update.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<V> AddOrUpdateAsync(long atomicGroupId, K key, V value, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Updates the value of an existent key and returns the old value.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Updated value.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<V> UpdateAsync(long atomicGroupId, K key, V value, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Updates a set of keys to new values. All keys are updated or none is updated.
        /// </summary>
        /// <param name="keys">Keys to update.</param>
        /// <param name="values">Values to update.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task UpdateAsync(IEnumerable<K> keys, IEnumerable<V> values, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Replaces an existent key to a new key with the same value.
        /// </summary>
        /// <param name="existentKey">Key to replace.</param>
        /// <param name="newKey">New key.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<bool> ReplaceAsync(K existentKey, K newKey, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Replaces an existent key to a new key and value and returns the value of the old key.
        /// </summary>
        /// <param name="existentKey">Existent key.</param>
        /// <param name="newKey">New key.</param>
        /// <param name="newValue">New value.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<Tuple<bool, V>> ReplaceAsync(K existentKey, K newKey, V newValue, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Uses the specified functions to add a key/value pair to the dictionary if the key does not already exist, 
        /// or to update a key/value pair in the dictionary if the key already exists.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">Key to add or update.</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <returns></returns>
        Task<V> AddOrUpdateAsync(long atomicGroupId, K key, Func<K, V> addValueFactory, Func<K, V, V> updateValueFactory);

        /// <summary>
        /// Uses the specified functions to add a key/value pair to the dictionary if the key does not already exist, 
        /// or to update a key/value pair in the dictionary if the key already exists.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">Key to add or update.</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<V> AddOrUpdateAsync(long atomicGroupId, K key, Func<K, V> addValueFactory, Func<K, V, V> updateValueFactory, TimeSpan timeout);

        /// <summary>
        /// Uses the specified functions to add a key/value pair to the dictionary if the key does not already exist, 
        /// or to update a key/value pair in the dictionary if the key already exists.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">Key to add or update.</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<V> AddOrUpdateAsync(long atomicGroupId, K key, Func<K, V> addValueFactory, Func<K, V, V> updateValueFactory, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Adds a key/value pair to the dictionary by using the specified function, if the key does not already exist.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key.</param>
        /// <returns></returns>
        Task<V> GetOrAddAsync(long atomicGroupId, K key, Func<K, V> valueFactory);

        /// <summary>
        /// Adds a key/value pair to the dictionary by using the specified function, if the key does not already exist.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <returns></returns>
        Task<V> GetOrAddAsync(long atomicGroupId, K key, Func<K, V> valueFactory, TimeSpan timeout);

        /// <summary>
        /// Adds a key/value pair to the dictionary by using the specified function, if the key does not already exist.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<V> GetOrAddAsync(long atomicGroupId, K key, Func<K, V> valueFactory, TimeSpan timeout, CancellationToken cancellationToken);
    }

    /// <summary>
    /// Atomic async collection.
    /// </summary>
    /// <typeparam name="T">The type of the elements in the collection.</typeparam>
    public interface IAtomicCollectionAsync<T>
    {
        /// <summary>
        /// Adds an item to the collection.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="item">Item to be added to the collection.</param>
        /// <param name="timeout">Timeout in milliseconds given for the operation to complete.</param>
        /// <returns></returns>
        Task AddAsync(long atomicGroupId, T item, TimeSpan timeout);

        /// <summary>
        /// Adds an item to the collection.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="item">Item to be added to the collection.</param>
        /// <param name="timeout">Timeout in milliseconds given for the operation to complete.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task AddAsync(long atomicGroupId, T item, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Removes an item from the collection.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="item">Item to be removed from the collection.</param>
        /// <param name="timeout">Timeout in milliseconds given for the operation to complete.</param>
        /// <returns></returns>
        Task<bool> RemoveAsync(long atomicGroupId, T item, TimeSpan timeout);

        /// <summary>
        /// Removes an item from the collection.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group this operation is part of.</param>
        /// <param name="item">Item to be removed from the collection.</param>
        /// <param name="timeout">Timeout in milliseconds given for the operation to complete.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<bool> RemoveAsync(long atomicGroupId, T item, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Adds items to the collection.
        /// </summary>
        /// <param name="items">Items to be added.</param>
        /// <param name="timeout">Timeout in milliseconds given for the operation to complete.</param>
        /// <returns></returns>
        Task AddAsync(IEnumerable<T> items, TimeSpan timeout);

        /// <summary>
        /// Adds items to the collection.
        /// </summary>
        /// <param name="items">Items to be added.</param>
        /// <param name="timeout">Timeout in milliseconds given for the operation to complete.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task AddAsync(IEnumerable<T> items, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Removes items from the collection.
        /// </summary>
        /// <param name="items">Items to be removed.</param>
        /// <param name="timeout">Timeout in milliseconds given for the operation to complete.</param>
        /// <returns></returns>
        Task<bool> RemoveAsync(IEnumerable<T> items, TimeSpan timeout);

        /// <summary>
        /// Removes items from the collection.
        /// </summary>
        /// <param name="items">Items to be removed.</param>
        /// <param name="timeout">Timeout in milliseconds given for the operation to complete.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<bool> RemoveAsync(IEnumerable<T> items, TimeSpan timeout, CancellationToken cancellationToken);
    }

    /// <summary>
    /// Supports a simple iteration over a generic collection.
    /// </summary>
    /// <typeparam name="T">The type of objects to enumerate.</typeparam>
    public interface IEnumeratorAsync<out T> : IDisposable
    {
        /// <summary>
        /// Gets the element in the collection at the current position of the enumerator.
        /// </summary>
        T Current { get; }

        /// <summary>
        /// Advances the enumerator to the next element of the collection.
        /// </summary>
        /// <returns></returns>
        Task<bool> MoveNextAsync();

        /// <summary>
        /// Sets the enumerator to its initial position, which is before the first element in the collection.
        /// </summary>
        /// <returns></returns>
        Task ResetAsync();
    }

    /// <summary>
    /// Async enumerator for a keyed collection.
    /// </summary>
    /// <typeparam name="K">The type of keys in the enumeration.</typeparam>
    /// <typeparam name="V">The type of values in the enumeration.</typeparam>
    public interface IEnumerableAsync<out T>
    {
        /// <summary>
        /// Returns an enumerator that iterates through a collection.
        /// </summary>
        /// <returns></returns>
        IEnumeratorAsync<T> GetAsyncEnumerator();
    }

    /// <summary>
    /// Async enumerator for a keyed collection that provides read isolation.
    /// </summary>
    /// <typeparam name="K">The type of keys in the enumeration.</typeparam>
    /// <typeparam name="V">The type of values in the enumeration.</typeparam>
    public interface IRangeEnumerableAsync<K, V>
    {
        /// <summary>
        /// Returns an enumerator that iterates through the entire collection.
        /// </summary>
        /// <param name="timeout">Timeout in milliseconds given for each iteration to complete.</param>
        /// <returns></returns>
        IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorSerializable(TimeSpan timeout);

        /// <summary>
        /// Returns an enumerator that iterates through a range of the collection.
        /// </summary>
        /// <param name="keyStart">Range start key.</param>
        /// <param name="keyEnd">Range end key.</param>
        /// <param name="timeout">Timeout in milliseconds given for each iteration to complete.</param>
        /// <returns></returns>
        IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorSerializable(K keyStart, K keyEnd, TimeSpan timeout);

        /// <summary>
        /// Returns an enumerator that iterates through a range of the collection and performs prefix match.
        /// </summary>
        /// <param name="keyStart">Range start key.</param>
        /// <param name="keyEnd">Range end key.</param>
        /// <param name="keyPrefixMatch">Key used for prefix match.</param>
        /// <param name="timeout">Timeout in milliseconds given for each iteration to complete.</param>
        /// <returns></returns>
        IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorSerializable(K keyStart, K keyEnd, K keyPrefixMatch, TimeSpan timeout);

        /// <summary>
        /// Returns an enumerator that iterates through a collection using repeatable read isolation.
        /// </summary>
        /// <param name="timeout">Timeout in milliseconds given for each iteration to complete.</param>
        /// <returns></returns>
        IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorRepeatableRead(TimeSpan timeout);

        /// <summary>
        /// Returns an enumerator that iterates through a collection using repeatable read isolation.
        /// </summary>
        /// <param name="keyStart">Range start key.</param>
        /// <param name="keyEnd">Range end key.</param>
        /// <param name="timeout">Timeout in milliseconds given for each iteration to complete.</param>
        /// <returns></returns>
        IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorRepeatableRead(K keyStart, K keyEnd, TimeSpan timeout);

        /// <summary>
        /// Returns an enumerator that iterates through a collection using repeatable read isolation.
        /// </summary>
        /// <param name="keyStart">Range start key.</param>
        /// <param name="keyEnd">Range end key.</param>
        /// <param name="keyPrefixMatch">Key used for prefix matching.</param>
        /// <param name="timeout">Timeout in milliseconds given for each iteration to complete.</param>
        /// <returns></returns>
        IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorRepeatableRead(K keyStart, K keyEnd, K keyPrefixMatch, TimeSpan timeout);

        /// <summary>
        /// Returns an enumerator that iterates through a collection using read committed isolation.
        /// </summary>
        /// <param name="timeout">Timeout in milliseconds given for each iteration to complete.</param>
        /// <returns></returns>
        IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorReadCommitted(TimeSpan timeout);

        /// <summary>
        /// Returns an enumerator that iterates through a collection using read committed isolation.
        /// </summary>
        /// <param name="keyStart">Range start key.</param>
        /// <param name="keyEnd">Range end key.</param>
        /// <param name="timeout">Timeout in milliseconds given for each iteration to complete.</param>
        /// <returns></returns>
        IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorReadCommitted(K keyStart, K keyEnd, TimeSpan timeout);

        /// <summary>
        /// Returns an enumerator that iterates through a collection using read committed isolation.
        /// </summary>
        /// <param name="keyStart">Range start key.</param>
        /// <param name="keyEnd">Range end key.</param>
        /// <param name="keyPrefixMatch">Key used for prefix matching.</param>
        /// <param name="timeout">Timeout in milliseconds given for each iteration to complete.</param>
        /// <returns></returns>
        IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorReadCommitted(K keyStart, K keyEnd, K keyPrefixMatch, TimeSpan timeout);

        /// <summary>
        /// Returns an enumerator that iterates through a collection using read uncommitted isolation.
        /// </summary>
        /// <param name="timeout">Timeout in milliseconds given for each iteration to complete.</param>
        /// <returns></returns>
        IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorReadUncommitted(TimeSpan timeout);

        /// <summary>
        /// Returns an enumerator that iterates through a collection using read uncommitted isolation.
        /// </summary>
        /// <param name="keyStart">Range start key.</param>
        /// <param name="keyEnd">Range end key.</param>
        /// <param name="timeout">Timeout in milliseconds given for each iteration to complete.</param>
        /// <returns></returns>
        IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorReadUncommitted(K keyStart, K keyEnd, TimeSpan timeout);

        /// <summary>
        /// Returns an enumerator that iterates through a collection using read uncommitted isolation.
        /// </summary>
        /// <param name="keyStart">Range start key.</param>
        /// <param name="keyEnd">Range end key.</param>
        /// <param name="keyPrefixMatch">Key used for prefix matching.</param>
        /// <param name="timeout">Timeout in milliseconds given for each iteration to complete.</param>
        /// <returns></returns>
        IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorReadUncommitted(K keyStart, K keyEnd, K keyPrefixMatch, TimeSpan timeout);

        /// <summary>
        /// Returns an enumerator that iterates through a collection using unprotected reads.
        /// </summary>
        /// <param name="timeout">Timeout in milliseconds given for each iteration to complete.</param>
        /// <returns></returns>
        IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorReadUnprotected(TimeSpan timeout);

        /// <summary>
        /// Returns an enumerator that iterates through a collection using unprotected reads.
        /// </summary>
        /// <param name="keyStart">Range start key.</param>
        /// <param name="keyEnd">Range end key.</param>
        /// <param name="timeout">Timeout in milliseconds given for each iteration to complete.</param>
        /// <returns></returns>
        IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorReadUnprotected(K keyStart, K keyEnd, TimeSpan timeout);

        /// <summary>
        /// Returns an enumerator that iterates through a collection using unprotected reads.
        /// </summary>
        /// <param name="keyStart">Range start key.</param>
        /// <param name="keyEnd">Range end key.</param>
        /// <param name="keyPrefixMatch">Key used for prefix matching.</param>
        /// <param name="timeout">Timeout in milliseconds given for each iteration to complete.</param>
        /// <returns></returns>
        IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorReadUnprotected(K keyStart, K keyEnd, K keyPrefixMatch, TimeSpan timeout);
    }

    /// <summary>
    /// Represents a thread-safe replicated sorted collection of key/value pairs that can be accessed by multiple threads concurrently.
    /// </summary>
    /// <typeparam name="K">The type of keys in the dictionary.</typeparam>
    /// <typeparam name="V">The type of values in the dictionary.</typeparam>
    /// <typeparam name="KC">Key comparator.</typeparam>
    /// <typeparam name="KBC">Key serializer.</typeparam>
    /// <typeparam name="VBC">Value serializer.</typeparam>
    public class SortedDictionaryStateProvider<K, V, KC, KBC, VBC> :
        ReplicatedStateProvider,
        IDictionary<K, V>,
        IDictionaryAsync<K, V>,
        IAtomicDictionaryAsync<K, V>,
        ICollection<KeyValuePair<K, V>>,
        ICollectionAsync<KeyValuePair<K, V>>,
        IAtomicCollectionAsync<KeyValuePair<K, V>>,
        IEnumerable<KeyValuePair<K, V>>,
        IReadOnlyDictionary<K, V>, 
        IReadOnlyCollection<KeyValuePair<K, V>>,
        IDictionary, 
        ICollection,
        IEnumerable,
        IEnumerableAsync<KeyValuePair<K, V>>,
        IRangeEnumerableAsync<K, V>,
        INotifyCollectionChanged
        where KC : IKeyComparable, new() 
        where KBC : IKeyBitConverter<K>, new()
        where VBC : IValueBitConverter<V>, new()
    {
        /// <summary>
        /// Constructor.
        /// </summary>
        public SortedDictionaryStateProvider()
        {
        }

        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="allowReadableSecondary">True, if the secondary replica allows reads.</param>
        public SortedDictionaryStateProvider(bool allowReadableSecondary)
        {
            this.allowReadableSecondary = allowReadableSecondary;
        }

        #region Instance Members

        /// <summary>
        /// Btree store (volatile or persisted).
        /// </summary>
        Btree<K, V, KBC, VBC> btreeStore = new Btree<K, V, KBC, VBC>();

        /// <summary>
        /// Btree store lock. 
        /// </summary>
        internal const string btreeLock = "btreeStoreLock";

        /// <summary>
        /// Configuration used to initialize the btree store.
        /// </summary>
        BtreeConfigurationDescription btreeConfiguration;

        /// <summary>
        /// Key comparison description.
        /// </summary>
        KC keyComparison = new KC();

        /// <summary>
        /// Transaction manager.
        /// </summary>
        System.Fabric.Data.TransactionManager.TransactionManager txManager;

        /// <summary>
        /// True, if the state provider allows reads on secondary replicas.
        /// </summary>
        bool allowReadableSecondary;

        /// <summary>
        /// True if the state provider is in the primary role or the secondary role and replication has started.
        /// </summary>
        bool isReadable;

        /// <summary>
        /// Last statistics read from the btree.
        /// </summary>
        BtreeStatistics lastStatistics = new BtreeStatistics();

        /// <summary>
        /// Load metric names.
        /// </summary>
        const string loadMetricNameMemoryUsageInBytes = "MemoryUsageInBytes";
        const string loadMetricNameStorageUsageInBytes = "StorageUsageInBytes";

        #endregion

        #region IDictionary<K, V> Members

        /// <summary>
        /// Adds the specified key and value to the dictionary.
        /// </summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value of the element to add. The value can be null for reference types.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        public void Add(K key, V value)
        {
            this.AddAsync(key, value).Wait();
        }

        /// <summary>
        /// Determines whether the dictionary contains a specific key.
        /// </summary>
        /// <param name="key">The key to locate in the dictionary.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <returns></returns>
        public virtual bool ContainsKey(K key)
        {
            return this.Contains(new KeyValuePair<K, V>(key, default(V)), false);
        }

        /// <summary>
        /// Gets an ICollection<K> containing the keys of the dictionary.
        /// </summary>
        public ICollection<K> Keys
        {
            get 
            {
                List<K> keys = new List<K>();
                using (IEnumerator<KeyValuePair<K, V>> enumerator = this.GetEnumerator())
                {
                    while (enumerator.MoveNext())
                    {
                        keys.Add(enumerator.Current.Key);
                    }
                }
                return keys;
            }
        }

        /// <summary>
        /// Gets an ICollection<V> containing the values of the dictionary.
        /// </summary>
        public ICollection<V> Values
        {
            get
            {
                List<V> values = new List<V>();
                using (IEnumerator<KeyValuePair<K, V>> enumerator = this.GetEnumerator())
                {
                    while (enumerator.MoveNext())
                    {
                        values.Add(enumerator.Current.Value);
                    }
                }
                return values;
            }
        }

        /// <summary>
        /// Removes the value with the specified key from the dictionary.
        /// </summary>
        /// <param name="key">The key of the element to remove.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public bool Remove(K key)
        {
            return this.RemoveAsync(key).Result;
        }

        /// <summary>
        /// Attempts to get the value associated with the specified key.
        /// </summary>
        /// <param name="key">The key of the value to get.</param>
        /// <param name="value">When this method returns, contains the value that has the specified key.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentNullException">A specified timeout has expired.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <returns></returns>
        public bool TryGetValue(K key, out V value)
        {
            Exception exp = null;
            value = default(V);
            Tuple<bool, V> result = null;
            try
            {
                result = this.TryGetValueAsync(key).Result;
                value = result.Item2;
            }
            catch (Exception e)
            {
                exp = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.TryGetValue", e, "{0}", this.ToString());
            }
            if (null != exp)
            {
                if (exp is ArgumentException || exp is TimeoutException || exp is OperationCanceledException)
                {
                    return false;
                }
                throw exp;
            }
            return result.Item1;
        }

        /// <summary>
        /// Gets or sets the value associated with the specified key.
        /// </summary>
        /// <param name="key">The key of the value to get or set.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Collections.Generic.KeyNotFoundException">The key was not found.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public V this[K key]
        {
            get
            {
                V value = default(V);
                bool found = this.TryGetValue(key, out value);
                if (!found)
                {
                    throw new KeyNotFoundException();
                }
                return value;
            }
            set
            {
                this.AddOrUpdateAsync(key, value).Wait();
            }
        }

        /// <summary>
        /// Adds the specified key and value to the dictionary.
        /// </summary>
        /// <param name="item">The structure representing the key and value to add.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        public void Add(KeyValuePair<K, V> item)
        {
            this.Add(item.Key, item.Value);
        }

        /// <summary>
        /// Removes all keys and values.
        /// </summary>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        public void Clear()
        {
            this.ClearAsync().Wait();
        }

        /// <summary>
        /// Determines whether the dictionary contains a specific element.
        /// </summary>
        /// <param name="item">The element to locate in the dictionary.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <returns></returns>
        public virtual bool Contains(KeyValuePair<K, V> item)
        {
            return this.Contains(item, true);
        }

        /// <summary>
        /// Copies the elements of the dictionary to an array, starting at a particular array index.
        /// </summary>
        /// <param name="array">The one-dimensional array that is the destination of the elements copied from the collection. The array must have zero-based indexing.</param>
        /// <param name="arrayIndex">The zero-based index in array at which copying begins.</param>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.ArgumentNullException">array is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">arrayIndex is less than 0.</exception>
        /// <exception cref="System.ArgumentException">The number of elements in the source collection is greater than the available space from arrayIndex to the end of the destination array.</exception>
        public void CopyTo(KeyValuePair<K, V>[] array, int arrayIndex)
        {
            if (null == array)
            {
                throw new ArgumentNullException("array", StringResources.Error_NullArray);
            }
            if (0 > arrayIndex)
            {
                throw new ArgumentOutOfRangeException("arrayIndex");
            }
            List<KeyValuePair<K, V>> items = new List<KeyValuePair<K, V>>();
            using (IEnumerator<KeyValuePair<K, V>> enumerator = this.GetEnumerator())
            {
                while (enumerator.MoveNext())
                {
                    items.Add(enumerator.Current);
                }
            }
            if (array.Count() < arrayIndex + items.Count)
            {
                throw new ArgumentException(StringResources.Error_IncorrectIndex, "arrayIndex");
            }
            for (int idx = 0; idx < items.Count; idx++)
            {
                array[arrayIndex + idx] = items[idx];
            }
            items.Clear();
        }

        /// <summary>
        /// Snapshots the number of key/value pairs contained in the dictionary.
        /// </summary>
        public int Count
        {
            get { return (int)this.btreeStore.Statistics.RecordCount; }
        }

        /// <summary>
        /// Gets a value indicating whether the dictionary is read-only.
        /// </summary>
        public bool IsReadOnly
        {
            get { return false; }
        }

        /// <summary>
        /// Removes the value with the specified key from the dictionary.
        /// </summary>
        /// <param name="item">The element to remove.</param>
        /// <returns></returns>
        public bool Remove(KeyValuePair<K, V> item)
        {
            return this.RemoveAsync(item).Result;
        }

        /// <summary>
        /// Checks if a key exceeds the key max size.
        /// </summary>
        /// <param name="key">Key to check.</param>
        void ThrowIfKeySizeExceeded(BtreeKey<K, KBC> key)
        {
            if (null != key)
            {
                if (key.Bytes.Count() > this.keyComparison.KeyComparison.MaximumKeySizeInBytes)
                {
                    throw new ArgumentOutOfRangeException("key");
                }
            }
        }

        /// <summary>
        /// Returns an enumerator that iterates through the dictionary. Serializable isolation level.
        /// </summary>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        public virtual IEnumerator<KeyValuePair<K, V>> GetEnumerator()
        {
            this.ThrowIfNotReadable();
            IReadOnlyTransaction rotx = this.txManager.CreateTransaction();
            IBtreeScan scan = this.btreeStore.CreateScan();
            ReplicatedStateProvider.Assert(null != scan, "unexpected null scan");
            return new SerializableSortedDictionaryEnumerator<K, V, KC, KBC, VBC>(
                scan, 
                null, 
                null, 
                rotx, 
                Timeout.Infinite);
        }

        /// <summary>
        /// Returns an enumerator that iterates through the dictionary. Serializable isolation level.
        /// </summary>
        /// <param name="keyStart">Range begin key.</param>
        /// <param name="keyEnd">Range end key.</param>
        /// <exception cref="System.ArgumentOutOfRangeException">The key exceeds maximum size.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        public virtual IEnumerator<KeyValuePair<K, V>> GetEnumerator(K keyStart, K keyEnd)
        {
            this.ThrowIfNotReadable();
            BtreeKey<K, KBC> btreeKeyStart = (null != keyStart) ? new BtreeKey<K, KBC>(keyStart) : null;
            BtreeKey<K, KBC> btreeKeyEnd = (null != keyEnd) ? new BtreeKey<K, KBC>(keyEnd) : null;
            this.ThrowIfKeySizeExceeded(btreeKeyStart);
            this.ThrowIfKeySizeExceeded(btreeKeyEnd);
            IReadOnlyTransaction rotx = this.txManager.CreateTransaction();
            IBtreeScan scan = this.btreeStore.CreateScan();
            ReplicatedStateProvider.Assert(null != scan, "unexpected null scan");
            return new SerializableSortedDictionaryEnumerator<K, V, KC, KBC, VBC>(
                scan, 
                btreeKeyStart, 
                btreeKeyEnd, 
                rotx,
                Timeout.Infinite);
        }

        /// <summary>
        /// Returns an async enumerator that iterates through the dictionary. 
        /// </summary>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        public IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumerator()
        {
            return this.GetAsyncEnumeratorSerializable();
        }

        /// <summary>
        /// Returns an async enumerator that iterates through the dictionary. Serializable isolation level.
        /// </summary>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        public IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorSerializable()
        {
            this.ThrowIfNotReadable();
            IReadOnlyTransaction rotx = this.txManager.CreateTransaction();
            IBtreeScan scan = this.btreeStore.CreateScan();
            ReplicatedStateProvider.Assert(null != scan, "unexpected null scan");
            return new SerializableSortedDictionaryEnumeratorAsync<K, V, KC, KBC, VBC>(
                scan, 
                null, 
                null, 
                null, 
                rotx,
                Timeout.Infinite);
        }

        /// <summary>
        /// Returns an enumerator that iterates through the entire collection.
        /// </summary>
        /// <param name="timeout">Timeout in milliseconds given for each iteration to complete.</param>
        /// <returns></returns>
        public IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorSerializable(TimeSpan timeout)
        {
            this.ThrowIfNotReadable();
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            IReadOnlyTransaction rotx = this.txManager.CreateTransaction();
            IBtreeScan scan = this.btreeStore.CreateScan();
            ReplicatedStateProvider.Assert(null != scan, "unexpected null scan");
            return new SerializableSortedDictionaryEnumeratorAsync<K, V, KC, KBC, VBC>(
                scan,
                null,
                null,
                null,
                rotx,
                milliseconds);
        }

        /// <summary>
        /// Returns an async enumerator that iterates through the dictionary. Serializable isolation level.
        /// </summary>
        /// <param name="keyStart">Start enumeration key.</param>
        /// <param name="keyEnd">End enumeration  key.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the enumeration to proceed.</param>
        /// <exception cref="System.ArgumentOutOfRangeException">The key exceeds maximum size.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        public IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorSerializable(K keyStart, K keyEnd, TimeSpan timeout)
        {
            this.ThrowIfNotReadable();
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            BtreeKey<K, KBC> btreeKeyStart = (null != keyStart) ? new BtreeKey<K, KBC>(keyStart) : null;
            BtreeKey<K, KBC> btreeKeyEnd = (null != keyEnd) ? new BtreeKey<K, KBC>(keyEnd) : null;
            this.ThrowIfKeySizeExceeded(btreeKeyStart);
            this.ThrowIfKeySizeExceeded(btreeKeyEnd);
            IReadOnlyTransaction rotx = this.txManager.CreateTransaction();
            IBtreeScan scan = this.btreeStore.CreateScan();
            ReplicatedStateProvider.Assert(null != scan, "unexpected null scan");
            return new SerializableSortedDictionaryEnumeratorAsync<K, V, KC, KBC, VBC>(
                scan, 
                btreeKeyStart, 
                btreeKeyEnd, 
                null, 
                rotx,
                milliseconds);
        }

        /// <summary>
        /// Returns an async enumerator that iterates through the dictionary. Serializable isolation level.
        /// </summary>
        /// <param name="keyStart">Start enumeration key.</param>
        /// <param name="keyEnd">End enumeration  key.</param>
        /// <param name="keyPrefixMatch">Prefix match enumeration  key.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the enumeration to proceed.</param>
        /// <exception cref="System.ArgumentNullException">The prefix key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">The key exceeds maximum size.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        public IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorSerializable(K keyStart, K keyEnd, K keyPrefixMatch, TimeSpan timeout)
        {
            //
            // Check arguments.
            //
            if (null == keyPrefixMatch)
            {
                throw new ArgumentNullException("keyPrefixMatch", StringResources.Error_NullKeyPrefix);
            }
            this.ThrowIfNotReadable();
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            BtreeKey<K, KBC> btreeKeyPrefixMatch = new BtreeKey<K, KBC>(keyPrefixMatch);
            BtreeKey<K, KBC> btreeKeyStart = (null != keyStart) ? new BtreeKey<K, KBC>(keyStart) : null;
            BtreeKey<K, KBC> btreeKeyEnd = (null != keyEnd) ? new BtreeKey<K, KBC>(keyEnd) : null;
            this.ThrowIfKeySizeExceeded(btreeKeyPrefixMatch);
            this.ThrowIfKeySizeExceeded(btreeKeyStart);
            this.ThrowIfKeySizeExceeded(btreeKeyEnd);
            IReadOnlyTransaction rotx = this.txManager.CreateTransaction();
            IBtreeScan scan = this.btreeStore.CreateScan();
            ReplicatedStateProvider.Assert(null != scan, "unexpected null scan");
            return new SerializableSortedDictionaryEnumeratorAsync<K, V, KC, KBC, VBC>(
                scan,
                btreeKeyStart,
                btreeKeyEnd,
                btreeKeyPrefixMatch,
                rotx,
                milliseconds);
        }

        /// <summary>
        /// Returns an enumerator that iterates through the dictionary. Repeatable read isolation level.
        /// </summary>
        /// <param name="timeout">The timeout in milliseconds allowed for the enumeration to proceed.</param>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        public virtual IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorRepeatableRead(TimeSpan timeout)
        {
            this.ThrowIfNotReadable();
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            IReadOnlyTransaction rotx = this.txManager.CreateTransaction();
            IBtreeScan scan = this.btreeStore.CreateScan();
            ReplicatedStateProvider.Assert(null != scan, "unexpected null scan");
            return new ReadIsolatedSortedDictionaryEnumeratorAsync<K, V, KC, KBC, VBC>(
                this.btreeStore, 
                scan, 
                null,
                null,
                null,
                rotx,
                ReadIsolationLevel.Repeatable,
                milliseconds);
        }

        /// <summary>
        /// Returns the timeout in milliseconds for a given timespan.
        /// </summary>
        /// <param name="timeout">Timeout.</param>
        /// <returns></returns>
        int GetTimeoutInMilliseconds(TimeSpan timeout)
        {
            if (Timeout.InfiniteTimeSpan == timeout)
            {
                return Timeout.Infinite;
            }
            double milliseconds = timeout.TotalMilliseconds;
            if ((TimeSpan.Zero > timeout) || (milliseconds > int.MaxValue))
            {
                throw new ArgumentException(StringResources.Error_ArgumentInvalid, "timeout");
            }
            return (int)milliseconds;
        }

        /// <summary>
        /// Returns an enumerator that iterates through the dictionary. Repeatable read isolation level.
        /// </summary>
        /// <param name="keyStart">Start enumeration key.</param>
        /// <param name="keyEnd">End enumeration  key.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the enumeration to proceed.</param>
        /// <exception cref="System.ArgumentOutOfRangeException">The key exceeds maximum size.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        public virtual IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorRepeatableRead(K keyStart, K keyEnd, TimeSpan timeout)
        {
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            this.ThrowIfNotReadable();
            BtreeKey<K, KBC> btreeKeyStart = (null != keyStart) ? new BtreeKey<K, KBC>(keyStart) : null;
            BtreeKey<K, KBC> btreeKeyEnd = (null != keyEnd) ? new BtreeKey<K, KBC>(keyEnd) : null;
            this.ThrowIfKeySizeExceeded(btreeKeyStart);
            this.ThrowIfKeySizeExceeded(btreeKeyEnd);
            IReadOnlyTransaction rotx = this.txManager.CreateTransaction();
            IBtreeScan scan = this.btreeStore.CreateScan();
            ReplicatedStateProvider.Assert(null != scan, "unexpected null scan");
            return new ReadIsolatedSortedDictionaryEnumeratorAsync<K, V, KC, KBC, VBC>(
                this.btreeStore,
                scan,
                btreeKeyStart,
                btreeKeyEnd,
                null,
                rotx,
                ReadIsolationLevel.Repeatable,
                milliseconds);
        }

        /// <summary>
        /// Returns an async enumerator that iterates through the dictionary. Repeatable read isolation level.
        /// </summary>
        /// <param name="keyStart">Start enumeration key.</param>
        /// <param name="keyEnd">End enumeration  key.</param>
        /// <param name="keyPrefixMatch">Prefix match enumeration  key.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the enumeration to proceed.</param>
        /// <exception cref="System.ArgumentNullException">The prefix key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">The key exceeds maximum size.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        public IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorRepeatableRead(K keyStart, K keyEnd, K keyPrefixMatch, TimeSpan timeout)
        {
            //
            // Check arguments.
            //
            if (null == keyPrefixMatch)
            {
                throw new ArgumentNullException("keyPrefixMatch", StringResources.Error_NullKeyPrefix);
            }
            this.ThrowIfNotReadable();
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            BtreeKey<K, KBC> btreeKeyPrefixMatch = new BtreeKey<K, KBC>(keyPrefixMatch);
            BtreeKey<K, KBC> btreeKeyStart = (null != keyStart) ? new BtreeKey<K, KBC>(keyStart) : null;
            BtreeKey<K, KBC> btreeKeyEnd = (null != keyEnd) ? new BtreeKey<K, KBC>(keyEnd) : null;
            this.ThrowIfKeySizeExceeded(btreeKeyPrefixMatch);
            this.ThrowIfKeySizeExceeded(btreeKeyStart);
            this.ThrowIfKeySizeExceeded(btreeKeyEnd);
            IReadOnlyTransaction rotx = this.txManager.CreateTransaction();
            IBtreeScan scan = this.btreeStore.CreateScan();
            ReplicatedStateProvider.Assert(null != scan, "unexpected null scan");
            return new ReadIsolatedSortedDictionaryEnumeratorAsync<K, V, KC, KBC, VBC>(
                this.btreeStore,
                scan,
                btreeKeyStart,
                btreeKeyEnd,
                btreeKeyPrefixMatch,
                rotx,
                ReadIsolationLevel.Repeatable,
                milliseconds);
        }

        /// <summary>
        /// Returns an enumerator that iterates through the dictionary. Read committed isolation level.
        /// </summary>
        /// <param name="timeout">The timeout in milliseconds allowed for the enumeration to proceed.</param>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        public virtual IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorReadCommitted(TimeSpan timeout)
        {
            this.ThrowIfNotReadable();
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            IReadOnlyTransaction rotx = this.txManager.CreateTransaction();
            return new ReadIsolatedSortedDictionaryEnumeratorAsync<K, V, KC, KBC, VBC>(
                this.btreeStore,
                this.btreeStore.CreateScan(),
                null,
                null,
                null,
                rotx,
                ReadIsolationLevel.Committed,
                milliseconds);
        }

        /// <summary>
        /// Returns an enumerator that iterates through the dictionary. Read committed isolation level.
        /// </summary>
        /// <param name="keyStart">Start enumeration key.</param>
        /// <param name="keyEnd">End enumeration  key.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the enumeration to proceed.</param>
        /// <exception cref="System.ArgumentOutOfRangeException">The key exceeds maximum size.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        public virtual IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorReadCommitted(K keyStart, K keyEnd, TimeSpan timeout)
        {
            this.ThrowIfNotReadable();
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            BtreeKey<K, KBC> btreeKeyStart = (null != keyStart) ? new BtreeKey<K, KBC>(keyStart) : null;
            BtreeKey<K, KBC> btreeKeyEnd = (null != keyEnd) ? new BtreeKey<K, KBC>(keyEnd) : null;
            this.ThrowIfKeySizeExceeded(btreeKeyStart);
            this.ThrowIfKeySizeExceeded(btreeKeyEnd);
            IReadOnlyTransaction rotx = this.txManager.CreateTransaction();
            IBtreeScan scan = this.btreeStore.CreateScan();
            ReplicatedStateProvider.Assert(null != scan, "unexpected null scan");
            return new ReadIsolatedSortedDictionaryEnumeratorAsync<K, V, KC, KBC, VBC>(
                this.btreeStore,
                scan,
                btreeKeyStart,
                btreeKeyEnd,
                null,
                rotx,
                ReadIsolationLevel.Committed,
                milliseconds);
        }

        /// <summary>
        /// Returns an async enumerator that iterates through the dictionary. Read committed isolation level.
        /// </summary>
        /// <param name="keyStart">Start enumeration key.</param>
        /// <param name="keyEnd">End enumeration  key.</param>
        /// <param name="keyPrefixMatch">Prefix match enumeration  key.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the enumeration to proceed.</param>
        /// <exception cref="System.ArgumentNullException">The prefix key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">The key exceeds maximum size.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        public IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorReadCommitted(K keyStart, K keyEnd, K keyPrefixMatch, TimeSpan timeout)
        {
            //
            // Check arguments.
            //
            if (null == keyPrefixMatch)
            {
                throw new ArgumentNullException("keyPrefixMatch", StringResources.Error_NullKeyPrefix);
            }
            this.ThrowIfNotReadable();
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            BtreeKey<K, KBC> btreeKeyPrefixMatch = new BtreeKey<K, KBC>(keyPrefixMatch);
            BtreeKey<K, KBC> btreeKeyStart = (null != keyStart) ? new BtreeKey<K, KBC>(keyStart) : null;
            BtreeKey<K, KBC> btreeKeyEnd = (null != keyEnd) ? new BtreeKey<K, KBC>(keyEnd) : null;
            this.ThrowIfKeySizeExceeded(btreeKeyPrefixMatch);
            this.ThrowIfKeySizeExceeded(btreeKeyStart);
            this.ThrowIfKeySizeExceeded(btreeKeyEnd);
            IReadOnlyTransaction rotx = this.txManager.CreateTransaction();
            IBtreeScan scan = this.btreeStore.CreateScan();
            ReplicatedStateProvider.Assert(null != scan, "unexpected null scan");
            return new ReadIsolatedSortedDictionaryEnumeratorAsync<K, V, KC, KBC, VBC>(
                this.btreeStore,
                scan,
                btreeKeyStart,
                btreeKeyEnd,
                btreeKeyPrefixMatch,
                rotx,
                ReadIsolationLevel.Committed,
                milliseconds);
        }

        /// <summary>
        /// Returns an async enumerator that iterates through the dictionary. Read uncomitted isolation level.
        /// </summary>
        /// <param name="timeout">The timeout in milliseconds allowed for the enumeration to proceed.</param>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        public IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorReadUncommitted(TimeSpan timeout)
        {
            this.ThrowIfNotReadable();
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            IReadOnlyTransaction rotx = this.txManager.CreateTransaction();
            IBtreeScan scan = this.btreeStore.CreateScan();
            ReplicatedStateProvider.Assert(null != scan, "unexpected null scan");
            return new ReadIsolatedSortedDictionaryEnumeratorAsync<K, V, KC, KBC, VBC>(
                this.btreeStore,
                scan,
                null,
                null,
                null,
                rotx,
                ReadIsolationLevel.Uncommitted,
                milliseconds);
        }

        /// <summary>
        /// Returns an async enumerator that iterates through the dictionary. Read uncomitted isolation level.
        /// </summary>
        /// <param name="keyStart">Start enumeration key.</param>
        /// <param name="keyEnd">End enumeration  key.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the enumeration to proceed.</param>
        /// <exception cref="System.ArgumentOutOfRangeException">The key exceeds maximum size.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        public IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorReadUncommitted(K keyStart, K keyEnd, TimeSpan timeout)
        {
            this.ThrowIfNotReadable();
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            BtreeKey<K, KBC> btreeKeyStart = (null != keyStart) ? new BtreeKey<K, KBC>(keyStart) : null;
            BtreeKey<K, KBC> btreeKeyEnd = (null != keyEnd) ? new BtreeKey<K, KBC>(keyEnd) : null;
            this.ThrowIfKeySizeExceeded(btreeKeyStart);
            this.ThrowIfKeySizeExceeded(btreeKeyEnd);
            IReadOnlyTransaction rotx = this.txManager.CreateTransaction();
            IBtreeScan scan = this.btreeStore.CreateScan();
            ReplicatedStateProvider.Assert(null != scan, "unexpected null scan");
            return new ReadIsolatedSortedDictionaryEnumeratorAsync<K, V, KC, KBC, VBC>(
                this.btreeStore,
                scan,
                btreeKeyStart,
                btreeKeyEnd,
                null,
                rotx,
                ReadIsolationLevel.Uncommitted,
                milliseconds);
        }

        /// <summary>
        /// Returns an async enumerator that iterates through the dictionary. Read uncomitted isolation level.
        /// </summary>
        /// <param name="keyStart">Start enumeration key.</param>
        /// <param name="keyEnd">End enumeration  key.</param>
        /// <param name="keyPrefixMatch">Prefix match enumeration  key.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the enumeration to proceed.</param>
        /// <exception cref="System.ArgumentNullException">The prefix key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">The key exceeds maximum size.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        public IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorReadUncommitted(K keyStart, K keyEnd, K keyPrefixMatch, TimeSpan timeout)
        {
            //
            // Check arguments.
            //
            if (null == keyPrefixMatch)
            {
                throw new ArgumentNullException("keyPrefixMatch", StringResources.Error_NullKeyPrefix);
            }
            this.ThrowIfNotReadable();
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            BtreeKey<K, KBC> btreeKeyPrefixMatch = new BtreeKey<K, KBC>(keyPrefixMatch);
            BtreeKey<K, KBC> btreeKeyStart = (null != keyStart) ? new BtreeKey<K, KBC>(keyStart) : null;
            BtreeKey<K, KBC> btreeKeyEnd = (null != keyEnd) ? new BtreeKey<K, KBC>(keyEnd) : null;
            this.ThrowIfKeySizeExceeded(btreeKeyPrefixMatch);
            this.ThrowIfKeySizeExceeded(btreeKeyStart);
            this.ThrowIfKeySizeExceeded(btreeKeyEnd);
            IReadOnlyTransaction rotx = this.txManager.CreateTransaction();
            IBtreeScan scan = this.btreeStore.CreateScan();
            ReplicatedStateProvider.Assert(null != scan, "unexpected null scan");
            return new ReadIsolatedSortedDictionaryEnumeratorAsync<K, V, KC, KBC, VBC>(
                this.btreeStore,
                scan,
                btreeKeyStart,
                btreeKeyEnd,
                btreeKeyPrefixMatch,
                rotx,
                ReadIsolationLevel.Uncommitted,
                milliseconds);
        }

        /// <summary>
        /// Returns an async enumerator that iterates through the dictionary. Read unprotected isolation level.
        /// </summary>
        /// <param name="timeout">The timeout in milliseconds allowed for the enumeration to proceed.</param>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        public IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorReadUnprotected(TimeSpan timeout)
        {
            this.ThrowIfNotReadable();
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            IReadOnlyTransaction rotx = this.txManager.CreateTransaction();
            IBtreeScan scan = this.btreeStore.CreateScan();
            ReplicatedStateProvider.Assert(null != scan, "unexpected null scan");
            return new ReadIsolatedSortedDictionaryEnumeratorAsync<K, V, KC, KBC, VBC>(
                this.btreeStore,
                scan,
                null,
                null,
                null,
                rotx,
                ReadIsolationLevel.Unprotected,
                milliseconds);
        }

        /// <summary>
        /// Returns an async enumerator that iterates through the dictionary. Read unprotected isolation level.
        /// </summary>
        /// <param name="keyStart">Start enumeration key.</param>
        /// <param name="keyEnd">End enumeration  key.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the enumeration to proceed.</param>
        /// <exception cref="System.ArgumentOutOfRangeException">The key exceeds maximum size.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        public IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorReadUnprotected(K keyStart, K keyEnd, TimeSpan timeout)
        {
            this.ThrowIfNotReadable();
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            BtreeKey<K, KBC> btreeKeyStart = (null != keyStart) ? new BtreeKey<K, KBC>(keyStart) : null;
            BtreeKey<K, KBC> btreeKeyEnd = (null != keyEnd) ? new BtreeKey<K, KBC>(keyEnd) : null;
            this.ThrowIfKeySizeExceeded(btreeKeyStart);
            this.ThrowIfKeySizeExceeded(btreeKeyEnd);
            IReadOnlyTransaction rotx = this.txManager.CreateTransaction();
            IBtreeScan scan = this.btreeStore.CreateScan();
            ReplicatedStateProvider.Assert(null != scan, "unexpected null scan");
            return new ReadIsolatedSortedDictionaryEnumeratorAsync<K, V, KC, KBC, VBC>(
                this.btreeStore,
                scan,
                btreeKeyStart,
                btreeKeyEnd,
                null,
                rotx,
                ReadIsolationLevel.Unprotected,
                milliseconds);
        }

        /// <summary>
        /// Returns an async enumerator that iterates through the dictionary. Read unprotected isolation level.
        /// </summary>
        /// <param name="keyStart">Start enumeration key.</param>
        /// <param name="keyEnd">End enumeration  key.</param>
        /// <param name="keyPrefixMatch">Prefix match enumeration  key.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the enumeration to proceed.</param>
        /// <exception cref="System.ArgumentNullException">The prefix key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">The key exceeds maximum size.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        public IEnumeratorAsync<KeyValuePair<K, V>> GetAsyncEnumeratorReadUnprotected(K keyStart, K keyEnd, K keyPrefixMatch, TimeSpan timeout)
        {
            //
            // Check arguments.
            //
            if (null == keyPrefixMatch)
            {
                throw new ArgumentNullException("keyPrefixMatch", StringResources.Error_NullKeyPrefix);
            }
            this.ThrowIfNotReadable();
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            BtreeKey<K, KBC> btreeKeyPrefixMatch = new BtreeKey<K, KBC>(keyPrefixMatch);
            BtreeKey<K, KBC> btreeKeyStart = (null != keyStart) ? new BtreeKey<K, KBC>(keyStart) : null;
            BtreeKey<K, KBC> btreeKeyEnd = (null != keyEnd) ? new BtreeKey<K, KBC>(keyEnd) : null;
            this.ThrowIfKeySizeExceeded(btreeKeyPrefixMatch);
            this.ThrowIfKeySizeExceeded(btreeKeyStart);
            this.ThrowIfKeySizeExceeded(btreeKeyEnd);
            IReadOnlyTransaction rotx = this.txManager.CreateTransaction();
            IBtreeScan scan = this.btreeStore.CreateScan();
            ReplicatedStateProvider.Assert(null != scan, "unexpected null scan");
            return new ReadIsolatedSortedDictionaryEnumeratorAsync<K, V, KC, KBC, VBC>(
                this.btreeStore,
                scan,
                btreeKeyStart,
                btreeKeyEnd,
                btreeKeyPrefixMatch,
                rotx,
                ReadIsolationLevel.Unprotected,
                milliseconds);
        }

        /// <summary>
        /// Returns an enumerator that iterates through a collection.
        /// </summary>
        /// <returns></returns>
        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }

        #endregion

        #region IDictionaryAsync<K, V> Members

        /// <summary>
        /// Adds the specified key and value to the dictionary.
        /// </summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value of the element to add. The value can be null for reference types.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task AddAsync(K key, V value)
        {
            return this.AddAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, value, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Adds the specified key and value to the dictionary.
        /// </summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value of the element to add. The value can be null for reference types.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task AddAsync(K key, V value, TimeSpan timeout)
        {
            return this.AddAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, value, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Adds the specified key and value to the dictionary.
        /// </summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value of the element to add. The value can be null for reference types.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task AddAsync(K key, V value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.AddAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, value, timeout, cancellationToken);
        }

        /// <summary>
        /// Removes the value with the specified key from the dictionary.
        /// </summary>
        /// <param name="key">The key of the element to remove.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> RemoveAsync(K key)
        {
            return this.RemoveAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Removes the value with the specified key from the dictionary.
        /// </summary>
        /// <param name="key">The key of the element to remove.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> RemoveAsync(K key, TimeSpan timeout)
        {
            return this.RemoveAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Removes the value with the specified key from the dictionary.
        /// </summary>
        /// <param name="key">The key of the element to remove.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> RemoveAsync(K key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.RemoveAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, timeout, cancellationToken);
        }

        /// <summary>
        /// Removes all keys and values.
        /// </summary>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public async Task ClearAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            Exception exp = null;
            bool wasErased = false;
            long atomicGroupId = FabricReplicatorEx.InvalidAtomicGroupId;
            long sequenceNumber = FabricReplicatorEx.InvalidSequenceNumber;
            bool existing = false;
            Tuple<bool, IRedoUndoInformation> result = null;
            IReadWriteTransaction rwtx = null;
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            //
            // Create dictionary add operation.
            //
            SortedDictionaryStateProviderClearOperation operation = new SortedDictionaryStateProviderClearOperation();
            //
            // Make sure writes are allowed.
            //
            this.ThrowIfFaulted();
            this.ThrowIfNotWritable();

            try
            {
                AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.Clear", "{0}", this.ToString());
                //
                // Create trivial read/write transaction. 
                //
                rwtx = this.txManager.CreateTransaction(atomicGroupId, out existing);
                ReplicatedStateProvider.Assert(!existing, "unexpected existing transaction");
                //
                // Acquire appropriate locks. 
                //
                await this.AcquireWriteLocks(null, BtreeOperationType.Erase, rwtx, milliseconds);
                cancellationToken.ThrowIfCancellationRequested();
                //
                // Reserve operation sequence number for the insert operation. 
                //
                sequenceNumber = await this.ReserveSequenceNumberAsync(cancellationToken);
                //
                // Delete everything in the btree store.
                //
                result = await this.btreeStore.EraseAsync(sequenceNumber, cancellationToken);
                wasErased = result.Item1;
                ReplicatedStateProvider.Assert((wasErased && null != result.Item2.Redo) || (!wasErased && null == result.Item2.Redo), "erase");
                //
                // Set operation redo/metadata.
                //
                operation.SequenceNumber = sequenceNumber;
                operation.AtomicGroupId = atomicGroupId;
                operation.RedoInfo = result.Item2.Redo;
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Clear", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || e is TimeoutException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(sequenceNumber);
                //
                // Transaction was just created, destroy it since it won't be committed or rolled back.
                //
                this.txManager.RemoveTransaction(rwtx);
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                throw e;
            }
            if (wasErased)
            {
                try
                {
                    //
                    // Set operation transaction.
                    //
                    operation.Tx = rwtx;
                    //
                    // Replicate clear operation. Cancellation not allowed from here on.
                    //
                    await this.ReplicateOperationAsync(operation, CancellationToken.None);
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Clear", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(
                        this.IsNonRetryableReplicationException(e) || e is OutOfMemoryException || e is StateProviderFaultedException,
                        string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Store original exception.
                    //
                    exp = e;
                }
            }
            if (!wasErased || null != exp)
            {
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(sequenceNumber);
                //
                // If transaction was just created, destroy it since it won't be committed or rolled back.
                //
                if (!existing && null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
            }
            if (null != exp)
            {
                throw exp;
            }
        }

        /// <summary>
        /// Removes all keys and values.
        /// </summary>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task ClearAsync(TimeSpan timeout)
        {
            return this.ClearAsync(timeout, CancellationToken.None);
        }

        /// <summary>
        /// Removes all keys and values.
        /// </summary>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task ClearAsync()
        {
            return this.ClearAsync(Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Attempts to get the value associated with the specified key.
        /// </summary>
        /// <param name="key">The key of the value to get.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <returns></returns>
        public async Task<Tuple<bool, V>> TryGetValueAsync(K key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == key)
            {
                throw new ArgumentNullException("key", StringResources.Error_NullKey);
            }
            Exception exp = null;
            //
            // Create key and value bytes.
            //
            BtreeKey<K, KBC> btreeKey = new BtreeKey<K, KBC>(key);
            this.ThrowIfKeySizeExceeded(btreeKey);
            IBtreeValue btreeValue = null;
            IReadOnlyTransaction rotx = null;
            //
            // Make sure reads are allowed.
            //
            this.ThrowIfFaulted();
            this.ThrowIfNotReadable();

            try
            {
                AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.TryGetValue", "{0}", this.ToString());
                //
                // Create read transaction. 
                //
                rotx = this.txManager.CreateTransaction();
                //
                // Acquire appropriate locks. 
                //
                await this.AcquireReadLocks(btreeKey, rotx, Timeout.Infinite);
                cancellationToken.ThrowIfCancellationRequested();
                //
                // Check to see if the value is in the btree store.
                //
                btreeValue = this.btreeStore.SeekAsync(btreeKey, cancellationToken).Result;
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.TryGetValue", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || e is TimeoutException || e is OperationCanceledException || e is FabricObjectClosedException || e is KeyNotFoundException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));

                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                //
                // Store original exception.
                //
                exp = e;
            }
            //
            // Release transaction locks.
            //
            this.txManager.RemoveTransaction(rotx);
            //
            // Retrow if necessary.
            //
            if (null != exp && !(exp is TimeoutException || exp is OperationCanceledException || exp is KeyNotFoundException))
            {
                throw exp;
            }
            //
            // Return found value.
            //
            return new Tuple<bool, V>((null != btreeValue), (null != btreeValue) ? (new BtreeValue<V, VBC>(btreeValue.Bytes)).Value : default(V));
        }

        /// <summary>
        /// Attempts to get the value associated with the specified key.
        /// </summary>
        /// <param name="key">The key of the value to get.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <returns></returns>
        public Task<Tuple<bool, V>> TryGetValueAsync(K key, TimeSpan timeout)
        {
            return this.TryGetValueAsync(key, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Attempts to get the value associated with the specified key.
        /// </summary>
        /// <param name="key">The key of the value to get.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <returns></returns>
        public Task<Tuple<bool, V>> TryGetValueAsync(K key)
        {
            return this.TryGetValueAsync(key, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Compares the existing value for the specified key with a specified value, and if they are equal, updates the key with a third value.
        /// </summary>
        /// <param name="key">The key whose value is compared with comparisonValue and possibly replaced.</param>
        /// <param name="newValue">The value that replaces the value of the element that has the specified key if the comparison results in equality.</param>
        /// <param name="comparisonValue">The value that is compared to the value of the element that has the specified key.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> TryUpdateAsync(K key, V newValue, V comparisonValue)
        {
            return this.TryUpdateAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, newValue, comparisonValue, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Compares the existing value for the specified key with a specified value, and if they are equal, updates the key with a third value.
        /// </summary>
        /// <param name="key">The key whose value is compared with comparisonValue and possibly replaced.</param>
        /// <param name="newValue">The value that replaces the value of the element that has the specified key if the comparison results in equality.</param>
        /// <param name="comparisonValue">The value that is compared to the value of the element that has the specified key.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> TryUpdateAsync(K key, V newValue, V comparisonValue, TimeSpan timeout)
        {
            return this.TryUpdateAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, newValue, comparisonValue, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Compares the existing value for the specified key with a specified value, and if they are equal, updates the key with a third value.
        /// </summary>
        /// <param name="key">The key whose value is compared with comparisonValue and possibly replaced.</param>
        /// <param name="newValue">The value that replaces the value of the element that has the specified key if the comparison results in equality.</param>
        /// <param name="comparisonValue">The value that is compared to the value of the element that has the specified key.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> TryUpdateAsync(K key, V newValue, V comparisonValue, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.TryUpdateAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, newValue, comparisonValue, timeout, cancellationToken);
        }

        /// <summary>
        /// Attempts to remove and return the value that has the specified key from the dictionary.
        /// </summary>
        /// <param name="key">The key of the element to remove and return.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<Tuple<bool, V>> TryRemoveAsync(K key)
        {
            return this.TryRemoveAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Attempts to remove and return the value that has the specified key from the dictionary.
        /// </summary>
        /// <param name="key">The key of the element to remove and return.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<Tuple<bool, V>> TryRemoveAsync(K key, TimeSpan timeout)
        {
            return this.TryRemoveAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Attempts to remove and return the value that has the specified key from the dictionary.
        /// </summary>
        /// <param name="key">The key of the element to remove and return.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<Tuple<bool, V>> TryRemoveAsync(K key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.TryRemoveAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, timeout, cancellationToken);
        }

        /// <summary>
        /// Attempts to add the specified key and value to the dictionary.
        /// </summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value of the element to add. The value can be null for reference types.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> TryAddAsync(K key, V value)
        {
            return this.TryAddAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, value, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Attempts to add the specified key and value to the dictionary.
        /// </summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value of the element to add. The value can be null for reference types.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> TryAddAsync(K key, V value, TimeSpan timeout)
        {
            return this.TryAddAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, value, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Attempts to add the specified key and value to the dictionary.
        /// </summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value of the element to add. The value can be null for reference types.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> TryAddAsync(K key, V value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.TryAddAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, value, timeout, cancellationToken);
        }

        /// <summary>
        /// Adds a key/value pair to the dictionary, if the key does not already exist.
        /// </summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added, if the key does not already exist.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> GetOrAddAsync(K key, V value)
        {
            return this.GetOrAddAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, value, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Adds a key/value pair to the dictionary, if the key does not already exist.
        /// </summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added, if the key does not already exist.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> GetOrAddAsync(K key, V value, TimeSpan timeout)
        {
            return this.GetOrAddAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, value, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Adds a key/value pair to the dictionary, if the key does not already exist.
        /// </summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added, if the key does not already exist.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> GetOrAddAsync(K key, V value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetOrAddAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, value, timeout, cancellationToken);
        }

        /// <summary>
        /// Add a key/value pair to the dictionary if the key does not already exist, or to update a key/value pair in the dictionary if the key already exists.
        /// </summary>
        /// <param name="key">The key to be added or whose value should be updated.</param>
        /// <param name="value">The value for an absent key.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> AddOrUpdateAsync(K key, V value)
        {
            return this.AddOrUpdateAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, value, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Add a key/value pair to the dictionary if the key does not already exist, or to update a key/value pair in the dictionary if the key already exists.
        /// </summary>
        /// <param name="key">The key to be added or whose value should be updated.</param>
        /// <param name="value">The value for an absent key.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> AddOrUpdateAsync(K key, V value, TimeSpan timeout)
        {
            return this.AddOrUpdateAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, value, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Add a key/value pair to the dictionary if the key does not already exist, or to update a key/value pair in the dictionary if the key already exists.
        /// </summary>
        /// <param name="key">The key to be added or whose value should be updated.</param>
        /// <param name="value">The value for an absent key.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> AddOrUpdateAsync(K key, V value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.AddOrUpdateAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, value, timeout, cancellationToken);
        }

        /// <summary>
        /// Updates the value of a key from the dictionary and returns the old value.
        /// </summary>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Value used for update.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> UpdateAsync(K key, V value)
        {
            return UpdateAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, value, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Updates the value of a key from the dictionary and returns the old value.
        /// </summary>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Value used for update.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> UpdateAsync(K key, V value, TimeSpan timeout)
        {
            return UpdateAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, value, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Updates the value of a key from the dictionary and returns the old value.
        /// </summary>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Value used for update.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> UpdateAsync(K key, V value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return UpdateAsync(FabricReplicatorEx.InvalidAtomicGroupId, key, value, timeout, cancellationToken);
        }

        /// <summary>
        /// Uses the specified functions to add a key/value pair to the dictionary if the key does not already exist, 
        /// or to update a key/value pair in the dictionary if the key already exists.
        /// </summary>
        /// <param name="key">Key to add or update.</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> AddOrUpdateAsync(K key, Func<K, V> addValueFactory, Func<K, V, V> updateValueFactory)
        {
            return this.AddOrUpdateAsync(key, addValueFactory, updateValueFactory, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Uses the specified functions to add a key/value pair to the dictionary if the key does not already exist, 
        /// or to update a key/value pair in the dictionary if the key already exists.
        /// </summary>
        /// <param name="key">Key to add or update.</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> AddOrUpdateAsync(K key, Func<K, V> addValueFactory, Func<K, V, V> updateValueFactory, TimeSpan timeout)
        {
            return this.AddOrUpdateAsync(key, addValueFactory, updateValueFactory, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Uses the specified functions to add a key/value pair to the dictionary if the key does not already exist, 
        /// or to update a key/value pair in the dictionary if the key already exists.
        /// </summary>
        /// <param name="key">Key to add or update.</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> AddOrUpdateAsync(K key, Func<K, V> addValueFactory, Func<K, V, V> updateValueFactory, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Adds a key/value pair to the dictionary by using the specified function, if the key does not already exist.
        /// </summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> GetOrAddAsync(K key, Func<K, V> valueFactory)
        {
            return GetOrAddAsync(key, valueFactory, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Adds a key/value pair to the dictionary by using the specified function, if the key does not already exist.
        /// </summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> GetOrAddAsync(K key, Func<K, V> valueFactory, TimeSpan timeout)
        {
            return GetOrAddAsync(key, valueFactory, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Adds a key/value pair to the dictionary by using the specified function, if the key does not already exist.
        /// </summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> GetOrAddAsync(K key, Func<K, V> valueFactory, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        #endregion

        #region IAtomicDictionaryAsync<K, V> Members

        /// <summary>
        /// Adds the specified key and value to the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value of the element to add. The value can be null for reference types.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task AddAsync(long atomicGroupId, K key, V value)
        {
            return this.AddAsync(atomicGroupId, key, value, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Adds the specified key and value to the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value of the element to add. The value can be null for reference types.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task AddAsync(long atomicGroupId, K key, V value, TimeSpan timeout)
        {
            return this.AddAsync(atomicGroupId, key, value, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Removes the value with the specified key from the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to remove.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> RemoveAsync(long atomicGroupId, K key)
        {
            return this.RemoveAsync(atomicGroupId, key, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Removes the value with the specified key from the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to remove.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> RemoveAsync(long atomicGroupId, K key, TimeSpan timeout)
        {
            return this.RemoveAsync(atomicGroupId, key, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Adds the specified keys and values to the dictionary. All are added or none are added.
        /// </summary>
        /// <param name="items">The list of structures representing the keys and values to add.</param>
        /// <exception cref="System.ArgumentNullException">Key is null or list is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists or list is empty.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task AddAsync(IEnumerable<KeyValuePair<K, V>> items)
        {
            return this.AddAsync(items, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Adds the specified keys and values to the dictionary. All are added or none are added.
        /// </summary>
        /// <param name="items">The list of structures representing the keys and values to add.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for each item to be added.</param>
        /// <exception cref="System.ArgumentNullException">Key is null or list is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists or list is empty.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task AddAsync(IEnumerable<KeyValuePair<K, V>> items, TimeSpan timeout)
        {
            return this.AddAsync(items, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Removes the values with the specified keys from the dictionary.
        /// </summary>
        /// <param name="items">The list of keys to remove.</param>
        /// <exception cref="System.ArgumentNullException">Key is null or list is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> RemoveAsync(IEnumerable<K> keys)
        {
            return this.RemoveAsync(keys, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Removes the values with the specified keys from the dictionary.
        /// </summary>
        /// <param name="items">The list of keys to remove.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for operation to proceed.</param>
        /// <exception cref="System.ArgumentNullException">Key is null or list is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> RemoveAsync(IEnumerable<K> keys, TimeSpan timeout)
        {
            return this.RemoveAsync(keys, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Updates a set of key with the same value.
        /// </summary>
        /// <param name="keys">Keys to update.</param>
        /// <param name="value">Value to replace.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task UpdateAsync(IEnumerable<K> keys, V value)
        {
            return this.UpdateAsync(keys, value, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Updates a set of key with the same value.
        /// </summary>
        /// <param name="keys">Keys to update.</param>
        /// <param name="value">Value to replace.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task UpdateAsync(IEnumerable<K> keys, V value, TimeSpan timeout)
        {
            return this.UpdateAsync(keys, value, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Compares the existing value for the specified key with a specified value, and if they are equal, updates the key with a third value.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key whose value is compared with comparisonValue and possibly replaced.</param>
        /// <param name="newValue">The value that replaces the value of the element that has the specified key if the comparison results in equality.</param>
        /// <param name="comparisonValue">The value that is compared to the value of the element that has the specified key.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> TryUpdateAsync(long atomicGroupId, K key, V newValue, V comparisonValue)
        {
            return this.TryUpdateAsync(atomicGroupId, key, newValue, comparisonValue, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Compares the existing value for the specified key with a specified value, and if they are equal, updates the key with a third value.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key whose value is compared with comparisonValue and possibly replaced.</param>
        /// <param name="newValue">The value that replaces the value of the element that has the specified key if the comparison results in equality.</param>
        /// <param name="comparisonValue">The value that is compared to the value of the element that has the specified key.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> TryUpdateAsync(long atomicGroupId, K key, V newValue, V comparisonValue, TimeSpan timeout)
        {
            return this.TryUpdateAsync(atomicGroupId, key, newValue, comparisonValue, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Attempts to remove and return the value that has the specified key from the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to remove and return.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<Tuple<bool, V>> TryRemoveAsync(long atomicGroupId, K key)
        {
            return this.TryRemoveAsync(atomicGroupId, key, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Attempts to remove and return the value that has the specified key from the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to remove and return.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<Tuple<bool, V>> TryRemoveAsync(long atomicGroupId, K key, TimeSpan timeout)
        {
            return this.TryRemoveAsync(atomicGroupId, key, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Attempts to add the specified key and value to the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value of the element to add. The value can be null for reference types.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> TryAddAsync(long atomicGroupId, K key, V value)
        {
            return TryAddAsync(atomicGroupId, key, value, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Attempts to add the specified key and value to the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value of the element to add. The value can be null for reference types.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> TryAddAsync(long atomicGroupId, K key, V value, TimeSpan timeout)
        {
            return TryAddAsync(atomicGroupId, key, value, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Adds a key/value pair to the dictionary, if the key does not already exist.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added, if the key does not already exist.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> GetOrAddAsync(long atomicGroupId, K key, V value)
        {
            return this.GetOrAddAsync(atomicGroupId, key, value, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Adds a key/value pair to the dictionary, if the key does not already exist.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added, if the key does not already exist.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> GetOrAddAsync(long atomicGroupId, K key, V value, TimeSpan timeout)
        {
            return this.GetOrAddAsync(atomicGroupId, key, value, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Add a key/value pair to the dictionary if the key does not already exist, or to update a key/value pair in the dictionary if the key already exists.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key to be added or whose value should be updated.</param>
        /// <param name="value">The value for an absent key.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> AddOrUpdateAsync(long atomicGroupId, K key, V value)
        {
            return this.AddOrUpdateAsync(atomicGroupId, key, value, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Add a key/value pair to the dictionary if the key does not already exist, or to update a key/value pair in the dictionary if the key already exists.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key to be added or whose value should be updated.</param>
        /// <param name="value">The value for an absent key.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> AddOrUpdateAsync(long atomicGroupId, K key, V value, TimeSpan timeout)
        {
            return this.AddOrUpdateAsync(atomicGroupId, key, value, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Updates the value of a key from the dictionary and returns the old value.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Value used for update.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> UpdateAsync(long atomicGroupId, K key, V value)
        {
            return this.UpdateAsync(atomicGroupId, key, value, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Updates the value of a key from the dictionary and returns the old value.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Value used for update.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> UpdateAsync(long atomicGroupId, K key, V value, TimeSpan timeout)
        {
            return this.UpdateAsync(atomicGroupId, key, value, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Updates the value of a key from the dictionary and returns the old value.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Value used for update.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Collections.Generic.KeyNotFoundException">The key was not found.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public async Task<V> UpdateAsync(long atomicGroupId, K key, V value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == key)
            {
                throw new ArgumentNullException("key", StringResources.Error_NullKey);
            }
            this.ThrowIfInvalidAtomicGroup(atomicGroupId);

            Exception exp = null;
            bool existing = false;
            Tuple<IBtreeValue, IRedoUndoInformation> result = null;
            long sequenceNumber = FabricReplicatorEx.InvalidSequenceNumber;
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            //
            // Create dictionary update operation.
            //
            SortedDictionaryStateProviderUpdateOperation<K, V> operation = new SortedDictionaryStateProviderUpdateOperation<K, V>();
            operation.Key = key;
            operation.NewValue = value;
            //
            // Create key and value bytes.
            //
            BtreeKey<K, KBC> btreeKey = new BtreeKey<K, KBC>(key);
            this.ThrowIfKeySizeExceeded(btreeKey);
            BtreeValue<V, VBC> btreeValue = new BtreeValue<V, VBC>(value);
            BtreeValue<V, VBC> btreeOldValue = null;
            IReadWriteTransaction rwtx = null;
            //
            // Make sure writes are allowed.
            //
            this.ThrowIfFaulted();
            this.ThrowIfNotWritable();

            try
            {
                AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.Update2", "{0}", this.ToString());
                //
                // Create or get read/write transaction for the given atomic group. 
                //
                rwtx = this.txManager.CreateTransaction(atomicGroupId, out existing);
                //
                // Acquire appropriate locks. 
                //
                await this.AcquireWriteLocks(btreeKey, BtreeOperationType.Update, rwtx, milliseconds);
                cancellationToken.ThrowIfCancellationRequested();
                //
                // Reserve operation sequence number for the insert operation. 
                //
                sequenceNumber = await this.ReserveSequenceNumberAsync(cancellationToken);
                //
                // Update the btree store.
                //
                result = await this.btreeStore.UpdateWithOutputAsync(btreeKey, btreeValue, sequenceNumber, cancellationToken);
                ReplicatedStateProvider.Assert(null != result.Item2.Redo && null != result.Item2.Undo, "unexpected update with output redo/undo");
                //
                // Set operation old value.
                //
                btreeOldValue = new BtreeValue<V, VBC>(result.Item1.Bytes);
                operation.OldValue = btreeOldValue.Value;
                //
                // Set operation redo/undo/metadata.
                //
                operation.SequenceNumber = sequenceNumber;
                operation.AtomicGroupId = atomicGroupId;
                operation.RedoInfo = result.Item2.Redo;
                operation.UndoInfo = result.Item2.Undo;
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Update2", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || e is KeyNotFoundException || e is TimeoutException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(sequenceNumber);
                //
                // If transaction was just created, destroy it since it won't be committed or rolled back.
                //
                if (!existing && null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                throw e;
            }
            try
            {
                //
                // Set operation transaction.
                //
                operation.Tx = rwtx;
                //
                // Replicate update operation. Cancellation not allowed from here on.
                //
                await this.ReplicateOperationAsync(operation, CancellationToken.None);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Update2", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    this.IsNonRetryableReplicationException(e) || e is OutOfMemoryException || e is StateProviderFaultedException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Store original exception.
                //
                exp = e;
            }
            if (null != exp)
            {
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(sequenceNumber);
                //
                // If transaction was just created, destroy it since it won't be committed or rolled back.
                //
                if (!existing && null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                throw exp;
            }
            //
            // Return old updated value.
            //
            return btreeOldValue.Value;
        }

        /// <summary>
        /// Updates a set a keys to new values. All key are updated or none is updated.
        /// </summary>
        /// <param name="keys">Keys to be updated.</param>
        /// <param name="values">New values used as part of the update operations.</param>
        /// <exception cref="System.ArgumentNullException">Key is null or list is null.</exception>
        /// <exception cref="System.ArgumentException">Key is specified in the list more than once.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task UpdateAsync(IEnumerable<K> keys, IEnumerable<V> values)
        {
            return this.UpdateAsync(keys, values, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Updates a set a keys to new values. All key are updated or none is updated.
        /// </summary>
        /// <param name="keys">Keys to be updated.</param>
        /// <param name="values">New values used as part of the update operations.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for each key and value to be updated.</param>
        /// <exception cref="System.ArgumentNullException">Key is null or list is null.</exception>
        /// <exception cref="System.ArgumentException">Key is specified in the list more than once.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task UpdateAsync(IEnumerable<K> keys, IEnumerable<V> values, TimeSpan timeout)
        {
            return this.UpdateAsync(keys, values, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Replaces an existent key with a new key. The value remains unchanged.
        /// </summary>
        /// <param name="existentKey">Key to replace.</param>
        /// <param name="newKey">New key used for update.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> ReplaceAsync(K existentKey, K newKey)
        {
            return this.ReplaceAsync(existentKey, newKey, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Replaces an existent key with a new key. The value remains unchanged.
        /// </summary>
        /// <param name="existentKey">Key to replace.</param>
        /// <param name="newKey">New key used for update.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> ReplaceAsync(K existentKey, K newKey, TimeSpan timeout)
        {
            return this.ReplaceAsync(existentKey, newKey, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Replaces an existent key to a new key and value and returns the value of the old key.
        /// </summary>
        /// <param name="existentKey">Key to replace.</param>
        /// <param name="newKey">New key used for update.</param>
        /// <param name="newValue">New value used for update.</param>
        /// <returns>The value of the existent key if the replace is successful.</returns>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <summary>
        public Task<Tuple<bool, V>> ReplaceAsync(K existentKey, K newKey, V newValue)
        {
            return this.ReplaceAsync(existentKey, newKey, newValue, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Replaces an existent key to a new key and value and returns the value of the old key.
        /// </summary>
        /// <param name="existentKey">Key to replace.</param>
        /// <param name="newKey">New key used for update.</param>
        /// <param name="newValue">New value used for update.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <returns>The value of the existent key if the replace is successful.</returns>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <summary>
        public Task<Tuple<bool, V>> ReplaceAsync(K existentKey, K newKey, V newValue, TimeSpan timeout)
        {
            return this.ReplaceAsync(existentKey, newKey, newValue, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Adds the specified key and value to the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value of the element to add. The value can be null for reference types.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public virtual async Task AddAsync(long atomicGroupId, K key, V value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == key)
            {
                throw new ArgumentNullException("key", StringResources.Error_NullKey);
            }
            this.ThrowIfInvalidAtomicGroup(atomicGroupId);

            bool existing = false;
            long sequenceNumber = FabricReplicatorEx.InvalidSequenceNumber;
            Exception exp = null;
            IRedoUndoInformation infoRedoUndo = null;
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            //
            // Create dictionary add operation.
            //
            SortedDictionaryStateProviderAddOperation<K, V> operation = new SortedDictionaryStateProviderAddOperation<K, V>();
            operation.Key = key;
            operation.Value = value;
            //
            // Create key and value bytes.
            //
            BtreeKey<K, KBC> btreeKey = new BtreeKey<K, KBC>(key);
            this.ThrowIfKeySizeExceeded(btreeKey);
            BtreeValue<V, VBC> btreeValue = new BtreeValue<V, VBC>(value);
            IReadWriteTransaction rwtx = null;
            //
            // Make sure writes are allowed.
            //
            this.ThrowIfFaulted();
            this.ThrowIfNotWritable();

            try
            {
                AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.Add", "{0}", this.ToString());
                //
                // Create or get read/write transaction for the given atomic group. 
                //
                rwtx = this.txManager.CreateTransaction(atomicGroupId, out existing);
                //
                // Acquire appropriate locks. 
                //
                await this.AcquireWriteLocks(btreeKey, BtreeOperationType.Insert, rwtx, milliseconds);
                cancellationToken.ThrowIfCancellationRequested();
                //
                // Reserve operation sequence number for the insert operation. 
                //
                sequenceNumber = await this.ReserveSequenceNumberAsync(cancellationToken);
                //
                // Insert into btree store.
                //
                infoRedoUndo = await this.btreeStore.InsertAsync(btreeKey, btreeValue, sequenceNumber, cancellationToken);
                if (null == infoRedoUndo)
                {
                    ReplicatedStateProvider.Assert(false, "unexpected insert redo/undo info");
                }
                ReplicatedStateProvider.Assert(null != infoRedoUndo.Redo && null != infoRedoUndo.Undo, "unexpected insert redo/undo");
                //
                // Set operation redo/undo/metadata.
                //
                operation.SequenceNumber = sequenceNumber;
                operation.AtomicGroupId = atomicGroupId;
                operation.RedoInfo = infoRedoUndo.Redo;
                operation.UndoInfo = infoRedoUndo.Undo;
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Add", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is ArgumentException || e is OutOfMemoryException || e is TimeoutException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(sequenceNumber);
                //
                // If transaction was just created, destroy it since it won't be committed or rolled back.
                //
                if (!existing && null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                throw e;
            }
            try
            {
                //
                // Set operation transaction.
                //
                operation.Tx = rwtx;
                //
                // Replicate add operation. Cancellation not allowed from here on.
                //
                await this.ReplicateOperationAsync(operation, CancellationToken.None);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Add", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    this.IsNonRetryableReplicationException(e) || e is OutOfMemoryException || e is StateProviderFaultedException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Store original exception.
                //
                exp = e;
            }
            if (null != exp)
            {
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(sequenceNumber);
                //
                // If transaction was just created, destroy it since it won't be committed or rolled back.
                //
                if (!existing && null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                throw exp;
            }
        }

        /// <summary>
        /// Removes the value with the specified key from the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to remove.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> RemoveAsync(long atomicGroupId, K key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == key)
            {
                throw new ArgumentNullException("key", StringResources.Error_NullKey);
            }
            this.ThrowIfInvalidAtomicGroup(atomicGroupId);
            //
            // Perform remove.
            //
            KeyValuePair<K, V> item = new KeyValuePair<K, V>(key, default(V));
            return this.RemoveAsync(atomicGroupId, item, false, timeout, cancellationToken);
        }

        /// <summary>
        /// Updates a set a keys to new values. All key are updated or none is updated.
        /// </summary>
        /// <param name="keys">Keys to be updated.</param>
        /// <param name="values">New values used as part of the update operations.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for each key and value to be updated.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null or list is null.</exception>
        /// <exception cref="System.ArgumentException">Key is specified in the list more than once.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public async Task UpdateAsync(IEnumerable<K> keys, IEnumerable<V> values, TimeSpan timeout, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == keys)
            {
                throw new ArgumentNullException("keys", StringResources.Error_NullKey);
            }
            if (null == values)
            {
                throw new ArgumentNullException("values", StringResources.Error_NullOrEmptyValue);
            }
            if (0 == keys.Count())
            {
                throw new ArgumentException(StringResources.Error_InvalidKeys, "keys");
            }
            if (0 == values.Count())
            {
                throw new ArgumentException(StringResources.Error_InvalidValues, "values");
            }
            if (keys.Count() != values.Count())
            {
                throw new ArgumentException(StringResources.Error_InvalidCount, "keys");
            }
            AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.Update(keys,values)", "{0}", this.ToString()); 
            
            bool isCommit = true;
            bool existing = false;
            IReadWriteTransaction rwtx = null;
            long atomicGroupId = FabricReplicatorEx.InvalidAtomicGroupId;
            try
            {
                //
                // Reserve atomic group id.
                //
                atomicGroupId = await this.ReserveSequenceNumberAsync(cancellationToken);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Update(keys,values)", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                throw e;
            }
            try
            {
                //
                // Create or get read/write transaction for the given atomic group. 
                //
                rwtx = this.txManager.CreateTransaction(atomicGroupId, out existing);
                ReplicatedStateProvider.Assert(!existing, "unexpected existing transaction");
                //
                // Create atomic group.
                //
                atomicGroupId = await this.CreateAtomicGroupAsync(cancellationToken);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Update(keys,values)", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e) || e is StateProviderFaultedException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(atomicGroupId);
                //
                // Remove just created transaction.
                //
                if (null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                throw e;
            }
            Exception exp = null;
            try
            {
                AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.Update(keys,values)", "{0} processing", this.ToString());
                //
                // Enumerate over the items.
                //
                using (IEnumerator<K> keyEnumerator = keys.GetEnumerator())
                {
                    using (IEnumerator<V> valueEnumerator = values.GetEnumerator())
                    {
                        while (valueEnumerator.MoveNext())
                        {
                            keyEnumerator.MoveNext();
                            //
                            // Update each item as part of the same atomic group. Cancellation not allowed from here on.
                            //
                            await this.UpdateAsync(atomicGroupId, keyEnumerator.Current, valueEnumerator.Current, timeout, cancellationToken);
                        }
                    }
                }
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Update(keys,values)", e, "{0} updating", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is ArgumentException || e is TimeoutException || e is OutOfMemoryException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                
                if (e is ArgumentException || e is TimeoutException || e is OperationCanceledException)
                {
                    //
                    // The atomic group will be rolled back.
                    //
                    isCommit = false;
                    //
                    // Store original exception.
                    //
                    exp = e;
                }
                else 
                { 
                    //
                    // If UpdateAsync encountered an OutOfMemoryException it had already faulted the state provider.
                    //
                    throw e; 
                }
            }
            try
            {
                //
                // Commit/Rollback atomic group. Cancellation not allowed from here on.
                //
                if (isCommit)
                {
                    await this.CommitAtomicGroupAsync(atomicGroupId, CancellationToken.None);
                }
                else
                {
                    await this.RollbackAtomicGroupAsync(atomicGroupId, CancellationToken.None);
                }
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Update(keys,values)", e, "{0} - {1}", this.ToString(), isCommit ? "commit" : "rollback");
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || this.IsNonRetryableReplicationException(e) || e is StateProviderFaultedException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Remove just created transaction.
                //
                if (null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                if (null != exp)
                {
                    //
                    // Store new exception.
                    //
                    exp = e;
                }
            }
            //
            // Retrow if necessary.
            //
            if (null != exp)
            {
                throw exp;
            }
        }

        /// <summary>
        /// Adds the specified keys and values to the dictionary. All are added or none are added.
        /// </summary>
        /// <param name="items">The list of structures representing the keys and values to add.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for each item to be added.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null or list is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists or list is empty.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public async Task AddAsync(IEnumerable<KeyValuePair<K, V>> items, TimeSpan timeout, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == items)
            {
                throw new ArgumentNullException("items", StringResources.Error_NullItems);
            }
            if (0 == items.Count())
            {
                throw new ArgumentException(StringResources.Error_InvalidItems, "items");
            }
            AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.Add(items)", "{0}", this.ToString());
            
            bool isCommit = true;
            bool existing = false;
            IReadWriteTransaction rwtx = null;
            long atomicGroupId = FabricReplicatorEx.InvalidAtomicGroupId;
            try
            {
                //
                // Reserve atomic group id.
                //
                atomicGroupId = await this.ReserveSequenceNumberAsync(cancellationToken);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Add(items)", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                throw e;
            }
            try
            {
                //
                // Create or get read/write transaction for the given atomic group. 
                //
                rwtx = this.txManager.CreateTransaction(atomicGroupId, out existing);
                ReplicatedStateProvider.Assert(!existing, "unexpected existing transaction");
                //
                // Create atomic group.
                //
                atomicGroupId = await this.CreateAtomicGroupAsync(cancellationToken);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Add(items)", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e) || e is StateProviderFaultedException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(atomicGroupId);
                //
                // Remove just created transaction.
                //
                if (null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                throw e;
            }
            Exception exp = null;
            try
            {
                AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.Add(items)", "{0} processing", this.ToString());
                //
                // Add each item as part of the same atomic group. Cancellation not allowed from here on.
                //
                foreach (var x in items)
                {
                    await this.AddAsync(atomicGroupId, x.Key, x.Value, timeout, cancellationToken);
                }
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Add(items)", e, "{0} adding", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is ArgumentException || e is TimeoutException || e is OutOfMemoryException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                
                if (e is ArgumentException || e is TimeoutException || e is OperationCanceledException)
                {
                    //
                    // The atomic group will be rolled back.
                    //
                    isCommit = false;
                    //
                    // Store original exception.
                    //
                    exp = e;
                }
                else 
                { 
                    //
                    // If AddAsync encountered an OutOfMemoryException it had already faulted the state provider.
                    //
                    throw e; 
                }
            }
            try
            {
                //
                // Commit/Rollback atomic group. Cancellation not allowed from here on.
                //
                if (isCommit)
                {
                    await this.CommitAtomicGroupAsync(atomicGroupId, CancellationToken.None);
                }
                else
                {
                    await this.RollbackAtomicGroupAsync(atomicGroupId, CancellationToken.None);
                }
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Add(items)", e, "{0} - {1}", this.ToString(), isCommit ? "commit" : "rollback");
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || this.IsNonRetryableReplicationException(e) || e is StateProviderFaultedException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Remove just created transaction.
                //
                if (null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                if (null != exp)
                {
                    //
                    // Store new exception.
                    //
                    exp = e;
                }
            }
            //
            // Retrow if necessary.
            //
            if (null != exp)
            {
                throw exp;
            }
        }

        /// <summary>
        /// Removes the values with the specified keys from the dictionary.
        /// </summary>
        /// <param name="items">The list of keys to remove.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null or list is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public async Task<bool> RemoveAsync(IEnumerable<K> keys, TimeSpan timeout, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == keys)
            {
                throw new ArgumentNullException("keys", StringResources.Error_NullKeys);
            }
            if (0 == keys.Count())
            {
                throw new ArgumentException(StringResources.Error_InvalidKeys, "keys");
            }
            AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.Remove(keys)", "{0}", this.ToString());

            bool isCommit = true;
            bool existing = false;
            bool wasRemoved = false;
            IReadWriteTransaction rwtx = null;
            long atomicGroupId = FabricReplicatorEx.InvalidAtomicGroupId;
            TimeSpan operationTimeout = TimeSpan.FromMilliseconds(timeout.TotalMilliseconds / keys.Count());
            try
            {
                //
                // Reserve atomic group id.
                //
                atomicGroupId = await this.ReserveSequenceNumberAsync(cancellationToken);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Remove(keys)", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                throw e;
            }
            try
            {
                //
                // Create or get read/write transaction for the given atomic group. 
                //
                rwtx = this.txManager.CreateTransaction(atomicGroupId, out existing);
                ReplicatedStateProvider.Assert(!existing, "unexpected existing transaction");
                //
                // Create atomic group.
                //
                atomicGroupId = await this.CreateAtomicGroupAsync(cancellationToken);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Remove(keys)", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e) || e is StateProviderFaultedException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(atomicGroupId);
                //
                // Remove just created transaction.
                //
                this.txManager.RemoveTransaction(rwtx);
                throw e;
            }
            Exception exp = null;
            try
            {
                AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.Remove(keys)", "{0} processing", this.ToString());
                //
                // Remove each key as part of the same atomic group. Cancellation not allowed from here on.
                //
                foreach (var x in keys)
                {
                    wasRemoved |= await this.RemoveAsync(atomicGroupId, x, operationTimeout, cancellationToken);
                }
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Remove(keys)", e, "{0} removing", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || e is TimeoutException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));

                if (e is ArgumentException || e is TimeoutException || e is OperationCanceledException)
                {
                    //
                    // The atomic group will be rolled back.
                    //
                    isCommit = false;
                    //
                    // Store original exception.
                    //
                    exp = e;
                }
                else
                {
                    //
                    // If RemoveAsync encountered an OutOfMemoryException it had already faulted the state provider.
                    //
                    throw e;
                }
            }
            try
            {
                //
                // Commit/Rollback atomic group. Cancellation not allowed from here on.
                //
                if (isCommit)
                {
                    await this.CommitAtomicGroupAsync(atomicGroupId, CancellationToken.None);
                }
                else
                {
                    await this.RollbackAtomicGroupAsync(atomicGroupId, CancellationToken.None);
                }
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Remove(keys)", e, "{0} - {1}", this.ToString(), isCommit ? "commit" : "rollback");
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || this.IsNonRetryableReplicationException(e) || e is StateProviderFaultedException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Remove just created transaction.
                //
                if (null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                if (null != exp)
                {
                    //
                    // Store new exception.
                    //
                    exp = e;
                }
            }
            //
            // Retrow if necessary.
            //
            if (null != exp)
            {
                throw exp;
            }
            return wasRemoved;
        }

        /// <summary>
        /// Replaces an existent key with a new key. The value remains unchanged.
        /// </summary>
        /// <param name="existentKey">Key to replace.</param>
        /// <param name="newKey">New key used for update.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public async Task<bool> ReplaceAsync(K existentKey, K newKey, TimeSpan timeout, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == existentKey)
            {
                throw new ArgumentNullException("existentKey", StringResources.Error_NullExistentKey);
            }
            if (null == newKey)
            {
                throw new ArgumentNullException("newKey", StringResources.Error_NullNewKey);
            }
            BtreeKey<K, KBC> existentBtreeKey = new BtreeKey<K, KBC>(existentKey);
            this.ThrowIfKeySizeExceeded(existentBtreeKey);
            BtreeKey<K, KBC> newBtreeKey = new BtreeKey<K, KBC>(newKey);
            this.ThrowIfKeySizeExceeded(newBtreeKey);
            AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.Replace(key,key)", "{0}", this.ToString());

            bool wasRemoved = false;
            bool isCommit = true;
            bool existing = false;
            IReadWriteTransaction rwtx = null;
            long atomicGroupId = FabricReplicatorEx.InvalidAtomicGroupId;
            try
            {
                //
                // Reserve atomic group id.
                //
                atomicGroupId = await this.ReserveSequenceNumberAsync(cancellationToken);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Replace(key,key)", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                throw e;
            }
            try
            {
                //
                // Create or get read/write transaction for the given atomic group. 
                //
                rwtx = this.txManager.CreateTransaction(atomicGroupId, out existing);
                ReplicatedStateProvider.Assert(!existing, "unexpected existing transaction");
                //
                // Create atomic group.
                //
                atomicGroupId = await this.CreateAtomicGroupAsync(cancellationToken);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Replace(key,key)", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e) || e is StateProviderFaultedException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(atomicGroupId);
                //
                // Remove just created transaction.
                //
                if (null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                throw e;
            }
            Exception exp = null;
            TimeSpan operationTimeout = TimeSpan.FromMilliseconds(timeout.TotalMilliseconds / 2);
            try
            {
                AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.Replace(key,key)", "{0} processing", this.ToString());
                //
                // Delete the existent key returning the existent value. Cancellation not allowed from here on.
                //
                Tuple<bool, V> removeResult = await this.TryRemoveAsync(atomicGroupId, existentKey, operationTimeout, cancellationToken);
                wasRemoved = removeResult.Item1;
                //
                // Insert the new key with the old value.
                //
                if (wasRemoved)
                {
                    await this.AddAsync(atomicGroupId, newKey, removeResult.Item2, operationTimeout, cancellationToken);
                }
                else
                {
                    //
                    // The atomic group should rollback.
                    //
                    isCommit = false;
                }
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Replace(key,key)", e, "{0} replacing", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is ArgumentException || e is TimeoutException || e is OutOfMemoryException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));

                if (e is ArgumentException || e is TimeoutException || e is OperationCanceledException)
                {
                    //
                    // The atomic group will be rolled back.
                    //
                    isCommit = false;
                    //
                    // Store original exception.
                    //
                    exp = e;
                }
                else 
                { 
                    //
                    // If AddAsync/RemoveAsync encountered an OutOfMemoryException it had already faulted the state provider.
                    //
                    throw e; 
                }
            }
            try
            {
                //
                // Commit/Rollback atomic group. Cancellation not allowed from here on.
                //
                if (isCommit)
                {
                    await this.CommitAtomicGroupAsync(atomicGroupId, CancellationToken.None);
                }
                else
                {
                    await this.RollbackAtomicGroupAsync(atomicGroupId, CancellationToken.None);
                }
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Replace(key,key)", e, "{0} - {1}", this.ToString(), isCommit ? "commit" : "rollback");
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || this.IsNonRetryableReplicationException(e) || e is StateProviderFaultedException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Remove just created transaction.
                //
                if (null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                if (null != exp)
                {
                    //
                    // Store new exception.
                    //
                    exp = e;
                }
            }
            //
            // Retrow if necessary.
            //
            if (null != exp)
            {
                throw exp;
            }
            return wasRemoved;
        }

        /// <summary>
        /// Replaces an existent key to a new key and value and returns the value of the old key.
        /// </summary>
        /// <param name="existentKey">Key to replace.</param>
        /// <param name="newKey">New key used for update.</param>
        /// <param name="newValue">New value used for update.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>The value of the existent key if the replace is successful.</returns>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <summary>
        public async Task<Tuple<bool, V>> ReplaceAsync(K existentKey, K newKey, V newValue, TimeSpan timeout, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == newKey)
            {
                throw new ArgumentNullException("newKey", StringResources.Error_NullNewKey);
            }
            if (null == existentKey)
            {
                throw new ArgumentNullException("existentKey", StringResources.Error_NullExistentKey);
            }
            BtreeKey<K, KBC> existentBtreeKey = new BtreeKey<K, KBC>(existentKey);
            this.ThrowIfKeySizeExceeded(existentBtreeKey);
            BtreeKey<K, KBC> newBtreeKey = new BtreeKey<K, KBC>(newKey);
            this.ThrowIfKeySizeExceeded(newBtreeKey);
            AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.Replace(key,key,value)", "{0}", this.ToString());

            Tuple<bool, V> result = null;
            bool isCommit = true;
            bool existing = false;
            IReadWriteTransaction rwtx = null;
            long atomicGroupId = FabricReplicatorEx.InvalidAtomicGroupId;
            try
            {
                //
                // Reserve atomic group id.
                //
                atomicGroupId = await this.ReserveSequenceNumberAsync(cancellationToken);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Replace(key,key,value)", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                throw e;
            }
            try
            {
                //
                // Create or get read/write transaction for the given atomic group. 
                //
                rwtx = this.txManager.CreateTransaction(atomicGroupId, out existing);
                ReplicatedStateProvider.Assert(!existing, "unexpected existing transaction");
                //
                // Create atomic group.
                //
                atomicGroupId = await this.CreateAtomicGroupAsync(cancellationToken);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Replace(key,key,value)", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e) || e is StateProviderFaultedException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(atomicGroupId);
                //
                // Remove just created transaction.
                //
                if (null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                throw e;
            }
            Exception exp = null;
            TimeSpan operationTimeout = TimeSpan.FromMilliseconds(timeout.TotalMilliseconds / 2);
            try
            {
                AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.Replace(key,key,value)", "{0} processing", this.ToString());
                //
                // Delete the existent key returning the existent value. Cancellation not allowed from here on.
                //
                result = await this.TryRemoveAsync(atomicGroupId, existentKey, operationTimeout, cancellationToken);
                //
                // Insert the new key with the new value.
                //
                if (result.Item1)
                {
                    await this.AddAsync(atomicGroupId, newKey, newValue, operationTimeout, cancellationToken);
                }
                else
                {
                    //
                    // The atomic group should rollback.
                    //
                    isCommit = false;
                }
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Replace(key,key,value)", e, "{0} replacing", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is ArgumentException || e is TimeoutException || e is OutOfMemoryException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));

                if (e is ArgumentException || e is TimeoutException || e is OperationCanceledException)
                {
                    //
                    // The atomic group will be rolled back.
                    //
                    isCommit = false;
                    //
                    // Store original exception.
                    //
                    exp = e;
                }
                else 
                { 
                    //
                    // If AddAsync/RemoveAsync encountered an OutOfMemoryException it had already faulted the state provider.
                    //
                    throw e; 
                }
            }
            try
            {
                //
                // Commit/Rollback atomic group. Cancellation not allowed from here on.
                //
                if (isCommit)
                {
                    await this.CommitAtomicGroupAsync(atomicGroupId, CancellationToken.None);
                }
                else
                {
                    await this.RollbackAtomicGroupAsync(atomicGroupId, CancellationToken.None);
                }
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Replace(key,key,value)", e, "{0} - {1}", this.ToString(), isCommit ? "commit" : "rollback");
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || this.IsNonRetryableReplicationException(e) || e is StateProviderFaultedException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Remove just created transaction.
                //
                if (null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                if (null != exp)
                {
                    //
                    // Store new exception.
                    //
                    exp = e;
                }
            }
            //
            // Retrow if necessary.
            //
            if (null != exp)
            {
                throw exp;
            }
            return result;
        }

        /// <summary>
        /// Updates a set of key with the same value.
        /// </summary>
        /// <param name="keys">Keys to update.</param>
        /// <param name="value">Value to replace.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public async Task UpdateAsync(IEnumerable<K> keys, V value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == keys)
            {
                throw new ArgumentNullException("keys", StringResources.Error_NullKeys);
            }
            if (0 == keys.Count())
            {
                throw new ArgumentException(StringResources.Error_InvalidKeys, "keys");
            }
            bool isCommit = true;
            bool existing = false;
            IReadWriteTransaction rwtx = null;
            long atomicGroupId = FabricReplicatorEx.InvalidAtomicGroupId;
            try
            {
                //
                // Reserve atomic group id.
                //
                atomicGroupId = await this.ReserveSequenceNumberAsync(cancellationToken);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Update(keys,value)", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                throw e;
            }
            try
            {
                //
                // Create or get read/write transaction for the given atomic group. 
                //
                rwtx = this.txManager.CreateTransaction(atomicGroupId, out existing);
                ReplicatedStateProvider.Assert(!existing, "unexpected existing transaction");
                //
                // Create atomic group.
                //
                atomicGroupId = await this.CreateAtomicGroupAsync(cancellationToken);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Update(keys,value)", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e) || e is StateProviderFaultedException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(atomicGroupId);
                //
                // Remove just created transaction.
                //
                if (null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                throw e;
            }
            Exception exp = null;
            TimeSpan operationTimeout = TimeSpan.FromMilliseconds(timeout.TotalMilliseconds / keys.Count());
            try
            {
                AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.Update(keys,value)", "{0} processing", this.ToString());
                //
                // Update each key as part of the same atomic group. Cancellation not allowed from here on.
                //
                foreach (var x in keys)
                {
                    await this.UpdateAsync(atomicGroupId, x, value, operationTimeout, cancellationToken);
                }
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Update(keys,value)", e, "{0} updating", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is ArgumentException || e is TimeoutException || e is OutOfMemoryException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));

                if (e is ArgumentException || e is TimeoutException || e is OperationCanceledException)
                {
                    //
                    // The atomic group will be rolled back.
                    //
                    isCommit = false;
                    //
                    // Store original exception.
                    //
                    exp = e;
                }
                else 
                { 
                    //
                    // If UpdateAsync encountered an OutOfMemoryException it had already faulted the state provider.
                    //
                    throw e; 
                }
            }
            try
            {
                //
                // Commit/Rollback atomic group. Cancellation not allowed from here on.
                //
                if (isCommit)
                {
                    await this.CommitAtomicGroupAsync(atomicGroupId, CancellationToken.None);
                }
                else
                {
                    await this.RollbackAtomicGroupAsync(atomicGroupId, CancellationToken.None);
                }
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Update(keys,value)", e, "{0} - {1}", this.ToString(), isCommit ? "commit" : "rollback");
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || this.IsNonRetryableReplicationException(e) || e is StateProviderFaultedException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Remove just created transaction.
                //
                if (null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                if (null != exp)
                {
                    //
                    // Store new exception.
                    //
                    exp = e;
                }
            }
            //
            // Retrow if necessary.
            //
            if (null != exp)
            {
                throw exp;
            }
        }

        /// <summary>
        /// Compares the existing value for the specified key with a specified value, and if they are equal, updates the key with a third value.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key whose value is compared with comparisonValue and possibly replaced.</param>
        /// <param name="newValue">The value that replaces the value of the element that has the specified key if the comparison results in equality.</param>
        /// <param name="comparisonValue">The value that is compared to the value of the element that has the specified key.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public async virtual Task<bool> TryUpdateAsync(long atomicGroupId, K key, V newValue, V comparisonValue, TimeSpan timeout, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == key)
            {
                throw new ArgumentNullException("key", StringResources.Error_NullKey);
            }
            this.ThrowIfInvalidAtomicGroup(atomicGroupId);

            Exception exp = null;
            bool wasUpdated = false;
            long sequenceNumber = FabricReplicatorEx.InvalidSequenceNumber;
            bool existing = false;
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            //
            // Create dictionary update operation.
            //
            SortedDictionaryStateProviderUpdateOperation<K, V> operation = new SortedDictionaryStateProviderUpdateOperation<K, V>();
            operation.Key = key;
            operation.NewValue = newValue;
            //
            // Create key and value bytes.
            //
            BtreeKey<K, KBC> btreeKey = new BtreeKey<K, KBC>(key);
            this.ThrowIfKeySizeExceeded(btreeKey);
            BtreeValue<V, VBC> btreeOldValue = new BtreeValue<V, VBC>(comparisonValue);
            BtreeValue<V, VBC> btreeNewValue = new BtreeValue<V, VBC>(newValue);
            Tuple<bool, IRedoUndoInformation> result = null;
            IReadWriteTransaction rwtx = null;
            //
            // Make sure writes are allowed.
            //
            this.ThrowIfFaulted();
            this.ThrowIfNotWritable();

            try
            {
                AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.TryUpdate", "{0}", this.ToString());
                //
                // Create or get read/write transaction for the given atomic group. 
                //
                rwtx = this.txManager.CreateTransaction(atomicGroupId, out existing);
                //
                // Acquire appropriate locks. 
                //
                await this.AcquireWriteLocks(btreeKey, BtreeOperationType.Update, rwtx, milliseconds);
                cancellationToken.ThrowIfCancellationRequested();
                //
                // Reserve operation sequence number for the delete operation. 
                //
                sequenceNumber = await this.ReserveSequenceNumberAsync(cancellationToken);
                //
                // Conditional update in btree store.
                //
                result = await this.btreeStore.ConditionalUpdateAsync(btreeKey, btreeNewValue, 0, btreeOldValue, sequenceNumber, cancellationToken);
                wasUpdated = result.Item1;
                ReplicatedStateProvider.Assert(
                    (wasUpdated && (null != result.Item2.Redo && null != result.Item2.Undo)) || (!wasUpdated && (null == result.Item2.Redo && null == result.Item2.Undo)),
                    "unexpected conditional update redo/undo");
                //
                // Set operation old value.
                //
                operation.OldValue = comparisonValue;
                //
                // Set operation redo/undo/metadata.
                //
                operation.SequenceNumber = sequenceNumber;
                operation.AtomicGroupId = atomicGroupId;
                operation.RedoInfo = result.Item2.Redo;
                operation.UndoInfo = result.Item2.Undo;
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.TryUpdate", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is ArgumentException || e is OutOfMemoryException || e is TimeoutException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(sequenceNumber);
                //
                // If transaction was just created, destroy it since it won't be committed or rolled back.
                //
                if (!existing && null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                throw e;
            }
            if (wasUpdated)
            {
                try
                {
                    //
                    // Set operation transaction.
                    //
                    operation.Tx = rwtx;
                    //
                    // Replicate update operation. Cancellation not allowed from here on.
                    //
                    await this.ReplicateOperationAsync(operation, CancellationToken.None);
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.TryUpdate", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(
                        this.IsNonRetryableReplicationException(e) || e is OutOfMemoryException || e is StateProviderFaultedException,
                        string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Store original exception.
                    //
                    exp = e;
                }
            }
            if (!wasUpdated || null != exp)
            {
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(sequenceNumber);
                //
                // If transaction was just created, destroy it since it won't be committed or rolled back.
                //
                if (!existing && null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
            }
            if (null != exp)
            {
                throw exp;
            }
            return wasUpdated;
        }

        /// <summary>
        /// Attempts to remove and return the value that has the specified key from the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to remove and return.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public async Task<Tuple<bool, V>> TryRemoveAsync(long atomicGroupId, K key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == key)
            {
                throw new ArgumentNullException("key", StringResources.Error_NullKey);
            }
            this.ThrowIfInvalidAtomicGroup(atomicGroupId);

            Exception exp = null;
            bool wasRemoved = false;
            long sequenceNumber = FabricReplicatorEx.InvalidSequenceNumber;
            bool existing = false;
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            //
            // Create dictionary remove operation.
            //
            SortedDictionaryStateProviderRemoveOperation<K, V> operation = new SortedDictionaryStateProviderRemoveOperation<K, V>();
            operation.Key = key;
            //
            // Create key and value bytes.
            //
            BtreeKey<K, KBC> btreeKey = new BtreeKey<K, KBC>(key);
            this.ThrowIfKeySizeExceeded(btreeKey);
            Tuple<IBtreeValue, IRedoUndoInformation> result = null;
            IReadWriteTransaction rwtx = null;
            //
            // Make sure writes are allowed.
            //
            this.ThrowIfFaulted();
            this.ThrowIfNotWritable();

            try
            {
                AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.TryRemove", "{0}", this.ToString());
                //
                // Create or get read/write transaction for the given atomic group. 
                //
                rwtx = this.txManager.CreateTransaction(atomicGroupId, out existing);
                //
                // Acquire appropriate locks. 
                //
                await this.AcquireWriteLocks(btreeKey, BtreeOperationType.Delete, rwtx, milliseconds);
                cancellationToken.ThrowIfCancellationRequested();
                //
                // Reserve operation sequence number for the delete operation. 
                //
                sequenceNumber = await this.ReserveSequenceNumberAsync(cancellationToken);
                //
                // Delete with output from btree store.
                //
                result = await this.btreeStore.DeleteWithOutputAsync(btreeKey, sequenceNumber, cancellationToken);
                wasRemoved = (null != result.Item1);
                ReplicatedStateProvider.Assert(
                    (wasRemoved && (null != result.Item2.Redo && null != result.Item2.Undo)) || (!wasRemoved && (null == result.Item2.Redo && null == result.Item2.Undo)),
                    "unexpected delete with output redo/undo");
                //
                // Set operation redo/undo/metadata.
                //
                operation.SequenceNumber = sequenceNumber;
                operation.AtomicGroupId = atomicGroupId;
                operation.RedoInfo = result.Item2.Redo;
                operation.UndoInfo = result.Item2.Undo;
                //
                // Set the correct value in the operation.
                //
                operation.Value = wasRemoved ? (new BtreeValue<V, VBC>(result.Item1.Bytes)).Value : default(V);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.TryRemove", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is ArgumentException || e is OutOfMemoryException || e is TimeoutException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(sequenceNumber);
                //
                // If transaction was just created, destroy it since it won't be committed or rolled back.
                //
                if (!existing && null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                throw ;
            }
            if (wasRemoved)
            {
                try
                {
                    //
                    // Set operation transaction.
                    //
                    operation.Tx = rwtx;
                    //
                    // Replicate remove operation. Cancellation not allowed from here on.
                    //
                    await this.ReplicateOperationAsync(operation, CancellationToken.None);
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.TryRemove", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(
                        this.IsNonRetryableReplicationException(e) || e is OutOfMemoryException || e is StateProviderFaultedException,
                        string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Store original exception.
                    //
                    exp = e;
                }
            }
            if (!wasRemoved || null != exp)
            {
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(sequenceNumber);
                //
                // If transaction was just created, destroy it since it won't be committed or rolled back.
                //
                if (!existing && null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
            }
            if (null != exp)
            {
                throw exp;
            }
            return new Tuple<bool, V>(wasRemoved, operation.Value);
        }

        /// <summary>
        /// Attempts to add the specified key and value to the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value of the element to add. The value can be null for reference types.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public async Task<bool> TryAddAsync(long atomicGroupId, K key, V value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Exception exp = null;
            try
            {
                await this.AddAsync(atomicGroupId, key, value, timeout, cancellationToken);
            }
            catch (Exception e)
            {
                exp = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.TryAddAsync", e, "{0}", this.ToString());
            }
            if (null != exp)
            {
                if (exp is ArgumentException || exp is TimeoutException || exp is OperationCanceledException)
                {
                    return false;
                }
                throw exp;
            }
            return true;
        }

        /// <summary>
        /// Adds a key/value pair to the dictionary, if the key does not already exist.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added, if the key does not already exist.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public async Task<V> GetOrAddAsync(long atomicGroupId, K key, V value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == key)
            {
                throw new ArgumentNullException("key", StringResources.Error_NullKey);
            }
            this.ThrowIfInvalidAtomicGroup(atomicGroupId);

            Exception exp = null;
            bool wasAdded = false;
            long sequenceNumber = FabricReplicatorEx.InvalidSequenceNumber;
            bool existing = false;
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            //
            // Create dictionary add operation.
            //
            SortedDictionaryStateProviderAddOperation<K, V> operation = new SortedDictionaryStateProviderAddOperation<K, V>();
            operation.Key = key;
            operation.Value = value;
            //
            // Create key and value bytes.
            //
            BtreeKey<K, KBC> btreeKey = new BtreeKey<K, KBC>(key);
            this.ThrowIfKeySizeExceeded(btreeKey);
            BtreeValue<V, VBC> btreeValue = new BtreeValue<V, VBC>(value);
            Tuple<IBtreeValue, IRedoUndoInformation> result = null;
            IReadWriteTransaction rwtx = null;
            //
            // Make sure writes are allowed.
            //
            this.ThrowIfFaulted();
            this.ThrowIfNotWritable();

            try
            {
                AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.GetOrAdd", "{0}", this.ToString());
                //
                // Create or get read/write transaction for the given atomic group. 
                //
                rwtx = this.txManager.CreateTransaction(atomicGroupId, out existing);
                //
                // Acquire appropriate locks. 
                //
                await this.AcquireWriteLocks(btreeKey, BtreeOperationType.Insert, rwtx, milliseconds);
                cancellationToken.ThrowIfCancellationRequested();
                //
                // Reserve operation sequence number for the delete operation. 
                //
                sequenceNumber = await this.ReserveSequenceNumberAsync(cancellationToken);
                //
                // Attemp insert into the btree store.
                //
                result = await this.btreeStore.InsertWithOutputAsync(btreeKey, btreeValue, sequenceNumber, cancellationToken);
                wasAdded = (null == result.Item1);
                ReplicatedStateProvider.Assert(
                    (wasAdded && (null != result.Item2.Redo && null != result.Item2.Undo)) || (!wasAdded && (null == result.Item2.Redo && null == result.Item2.Undo)),
                    "unexpected insert with output redo/undo");
                //
                // Set operation redo/undo/metadata.
                //
                operation.SequenceNumber = sequenceNumber;
                operation.AtomicGroupId = atomicGroupId;
                operation.RedoInfo = result.Item2.Redo;
                operation.UndoInfo = result.Item2.Undo;
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.GetOrAdd", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || e is TimeoutException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(sequenceNumber);
                //
                // If transaction was just created, destroy it since it won't be committed or rolled back.
                //
                if (!existing && null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                throw e;
            }
            if (wasAdded)
            {
                try
                {
                    //
                    // Set operation transaction.
                    //
                    operation.Tx = rwtx;
                    //
                    // Replicate add operation. Cancellation not allowed from here on.
                    //
                    await this.ReplicateOperationAsync(operation, CancellationToken.None);
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.GetOrAdd", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(
                        this.IsNonRetryableReplicationException(e) || e is OutOfMemoryException || e is StateProviderFaultedException,
                        string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Store original exception.
                    //
                    exp = e;
                }
                if (null != exp)
                {
                    //
                    // Cancel sequence number.
                    //
                    this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(sequenceNumber);
                    //
                    // If transaction was just created, destroy it since it won't be committed or rolled back.
                    //
                    if (!existing && null != rwtx)
                    {
                        this.txManager.RemoveTransaction(rwtx);
                    }
                    throw exp;
                }
                //
                // Key was inserted.
                //
                return value;
            }
            else
            {
                ReplicatedStateProvider.Assert(null != result.Item1, "unexpected item");
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(sequenceNumber);
                //
                // If transaction was just created, destroy it since it won't be committed or rolled back.
                //
                if (!existing && null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                //
                // Key was not inserted since it existed already.
                //
                return (new BtreeValue<V, VBC>(result.Item1.Bytes)).Value;
            }
        }

        /// <summary>
        /// Add a key/value pair to the dictionary if the key does not already exist, or to update a key/value pair in the dictionary if the key already exists.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key to be added or whose value should be updated.</param>
        /// <param name="value">The value for an absent key.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public async Task<V> AddOrUpdateAsync(long atomicGroupId, K key, V value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == key)
            {
                throw new ArgumentNullException("key", StringResources.Error_NullKey);
            }
            this.ThrowIfInvalidAtomicGroup(atomicGroupId);

            Exception exp = null;
            bool wasUpdated = false;
            long sequenceNumber = FabricReplicatorEx.InvalidSequenceNumber;
            bool existing = false;
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            //
            // Create dictionary operation (add or update).
            //
            SortedDictionaryStateProviderOperation operation = null;
            //
            // Create key and value bytes.
            //
            BtreeKey<K, KBC> btreeKey = new BtreeKey<K, KBC>(key);
            this.ThrowIfKeySizeExceeded(btreeKey);
            BtreeValue<V, VBC> btreeValue = new BtreeValue<V, VBC>(value);
            Tuple<IBtreeValue, IRedoUndoInformation> result = null;
            IReadWriteTransaction rwtx = null;
            //
            // Make sure writes are allowed.
            //
            this.ThrowIfFaulted();
            this.ThrowIfNotWritable();

            try
            {
                AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.AddOrUpdate", "{0}", this.ToString());
                //
                // Create or get read/write transaction for the given atomic group. 
                //
                rwtx = this.txManager.CreateTransaction(atomicGroupId, out existing);
                //
                // Acquire appropriate locks. 
                //
                await this.AcquireWriteLocks(btreeKey, BtreeOperationType.Insert, rwtx, milliseconds);
                cancellationToken.ThrowIfCancellationRequested();
                //
                // Reserve operation sequence number for the delete operation. 
                //
                sequenceNumber = await this.ReserveSequenceNumberAsync(cancellationToken);
                //
                // Upsert into the btree store.
                //
                result = await this.btreeStore.UpsertAsync(btreeKey, btreeValue, sequenceNumber, cancellationToken);
                wasUpdated = (null != result.Item1);
                ReplicatedStateProvider.Assert(
                    (wasUpdated && (null != result.Item2.Redo && null != result.Item2.Undo)) || (!wasUpdated && (null == result.Item2.Redo && null == result.Item2.Undo)),
                    "unexpected upsert redo/undo");
                //
                // Create operation.
                //
                if (!wasUpdated)
                {
                    operation = new SortedDictionaryStateProviderAddOperation<K, V>();
                    ((SortedDictionaryStateProviderAddOperation<K, V>)operation).Key = key;
                    ((SortedDictionaryStateProviderAddOperation<K, V>)operation).Value = value;
                }
                else
                {
                    operation = new SortedDictionaryStateProviderUpdateOperation<K, V>();
                    ((SortedDictionaryStateProviderUpdateOperation<K, V>)operation).Key = key;
                    ((SortedDictionaryStateProviderUpdateOperation<K, V>)operation).NewValue = value;
                    ((SortedDictionaryStateProviderUpdateOperation<K, V>)operation).OldValue = (new BtreeValue<V, VBC>(result.Item1.Bytes)).Value;
                }
                //
                // Set operation redo/undo/metadata.
                //
                operation.SequenceNumber = sequenceNumber;
                operation.AtomicGroupId = atomicGroupId;
                operation.RedoInfo = result.Item2.Redo;
                operation.UndoInfo = result.Item2.Undo;
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.AddOrUpdate", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || e is TimeoutException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(sequenceNumber);
                //
                // If transaction was just created, destroy it since it won't be committed or rolled back.
                //
                if (!existing && null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                throw e;
            }
            try
            {
                //
                // Set operation transaction.
                //
                operation.Tx = rwtx;
                //
                // Replicate add or update operation. Cancellation not allowed from here on.
                //
                await this.ReplicateOperationAsync(operation, CancellationToken.None);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.AddOrUpdate", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    this.IsNonRetryableReplicationException(e) || e is OutOfMemoryException || e is StateProviderFaultedException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Store original exception.
                //
                exp = e;
            }
            if (null != exp)
            {
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(sequenceNumber);
                //
                // If transaction was just created, destroy it since it won't be committed or rolled back.
                //
                if (!existing && null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                throw exp;
            }
            //
            // Key was inserted/updated.
            //
            return wasUpdated ? (new BtreeValue<V, VBC>(result.Item1.Bytes)).Value : value;
        }

        /// <summary>
        /// Uses the specified functions to add a key/value pair to the dictionary if the key does not already exist, 
        /// or to update a key/value pair in the dictionary if the key already exists.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">Key to add or update.</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> AddOrUpdateAsync(long atomicGroupId, K key, Func<K, V> addValueFactory, Func<K, V, V> updateValueFactory)
        {
            return this.AddOrUpdateAsync(atomicGroupId, key, addValueFactory, updateValueFactory, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Uses the specified functions to add a key/value pair to the dictionary if the key does not already exist, 
        /// or to update a key/value pair in the dictionary if the key already exists.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">Key to add or update.</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> AddOrUpdateAsync(long atomicGroupId, K key, Func<K, V> addValueFactory, Func<K, V, V> updateValueFactory, TimeSpan timeout)
        {
            return this.AddOrUpdateAsync(atomicGroupId, key, addValueFactory, updateValueFactory, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Uses the specified functions to add a key/value pair to the dictionary if the key does not already exist, 
        /// or to update a key/value pair in the dictionary if the key already exists.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">Key to add or update.</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> AddOrUpdateAsync(long atomicGroupId, K key, Func<K, V> addValueFactory, Func<K, V, V> updateValueFactory, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Adds a key/value pair to the dictionary by using the specified function, if the key does not already exist.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> GetOrAddAsync(long atomicGroupId, K key, Func<K, V> valueFactory)
        {
            return GetOrAddAsync(atomicGroupId, key, valueFactory, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Adds a key/value pair to the dictionary by using the specified function, if the key does not already exist.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> GetOrAddAsync(long atomicGroupId, K key, Func<K, V> valueFactory, TimeSpan timeout)
        {
            return GetOrAddAsync(atomicGroupId, key, valueFactory, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Adds a key/value pair to the dictionary by using the specified function, if the key does not already exist.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key.</param>
        /// <param name="timeout">The timeout that specifies how long this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<V> GetOrAddAsync(long atomicGroupId, K key, Func<K, V> valueFactory, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        #endregion

        #region ICollectionAsync<KeyValuePair<K, V>> Members

        /// <summary>
        /// Adds the specified key and value to the dictionary.
        /// </summary>
        /// <param name="item">The structure representing the key and value to add.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task AddAsync(KeyValuePair<K, V> item)
        {
            return this.AddAsync(item.Key, item.Value, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Adds the specified key and value to the dictionary.
        /// </summary>
        /// <param name="item">The structure representing the key and value to add.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task AddAsync(KeyValuePair<K, V> item, TimeSpan timeout)
        {
            return this.AddAsync(item.Key, item.Value, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Adds the specified key and value to the dictionary.
        /// </summary>
        /// <param name="item">The structure representing the key and value to add.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task AddAsync(KeyValuePair<K, V> item, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.AddAsync(item.Key, item.Value, timeout, cancellationToken);
        }

        /// <summary>
        /// Removes the key and value from the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="item">The item to remove.</param>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> RemoveAsync(KeyValuePair<K, V> item)
        {
            return this.RemoveAsync(FabricReplicatorEx.InvalidAtomicGroupId, item, true, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Removes the key and value from the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="item">The item to remove.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> RemoveAsync(KeyValuePair<K, V> item, TimeSpan timeout)
        {
            return this.RemoveAsync(FabricReplicatorEx.InvalidAtomicGroupId, item, true, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Removes the key and value from the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="item">The item to remove.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task<bool> RemoveAsync(KeyValuePair<K, V> item, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.RemoveAsync(FabricReplicatorEx.InvalidAtomicGroupId, item, true, timeout, cancellationToken);
        }
        
        #endregion

        #region IAtomicCollectionAsync<KeyValuePair<K, V>> Members

        /// <summary>
        /// Adds the specified item to the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="item">The item to add.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task AddAsync(long atomicGroupId, KeyValuePair<K, V> item, TimeSpan timeout)
        {
            return this.AddAsync(atomicGroupId, item.Key, item.Value, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Adds the specified item to the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="item">The item to add.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public Task AddAsync(long atomicGroupId, KeyValuePair<K, V> item, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.AddAsync(atomicGroupId, item.Key, item.Value, timeout, cancellationToken);
        }

        /// <summary>
        /// Removes the item from the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="item">The item to remove.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public virtual Task<bool> RemoveAsync(long atomicGroupId, KeyValuePair<K, V> item, TimeSpan timeout)
        {
            return this.RemoveAsync(atomicGroupId, item, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Removes the item from the dictionary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="item">The item to remove.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public virtual Task<bool> RemoveAsync(long atomicGroupId, KeyValuePair<K, V> item, TimeSpan timeout, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == item.Key)
            {
                throw new ArgumentNullException("item", StringResources.Error_NullKey);
            }
            this.ThrowIfInvalidAtomicGroup(atomicGroupId);
            //
            // Perform remove.
            //
            return this.RemoveAsync(atomicGroupId, item, true, timeout, cancellationToken);
        }

        /// <summary>
        /// Removes the specified items from the dictionary.
        /// </summary>
        /// <param name="items">The list of items to remove.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <exception cref="System.ArgumentNullException">Key is null or list is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public virtual Task<bool> RemoveAsync(IEnumerable<KeyValuePair<K, V>> items, TimeSpan timeout)
        {
            return this.RemoveAsync(items, timeout, CancellationToken.None);
        }
        
        /// <summary>
        /// Removes the specified items from the dictionary.
        /// </summary>
        /// <param name="items">The list of items to remove.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentNullException">Key is null or list is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public virtual async Task<bool> RemoveAsync(IEnumerable<KeyValuePair<K, V>> items, TimeSpan timeout, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == items)
            {
                throw new ArgumentNullException("items", StringResources.Error_NullItems);
            }
            if (0 == items.Count())
            {
                throw new ArgumentException(StringResources.Error_InvalidItems, "items");
            }
            AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.Remove(items)", "{0}", this.ToString());

            bool isCommit = true;
            bool existing = false;
            bool wasRemoved = false;
            IReadWriteTransaction rwtx = null;
            long atomicGroupId = FabricReplicatorEx.InvalidAtomicGroupId;
            try
            {
                //
                // Reserve atomic group id.
                //
                atomicGroupId = await this.ReserveSequenceNumberAsync(cancellationToken);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Remove(items)", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                throw e;
            }
            try
            {
                //
                // Create or get read/write transaction for the given atomic group. 
                //
                rwtx = this.txManager.CreateTransaction(atomicGroupId, out existing);
                ReplicatedStateProvider.Assert(!existing, "unexpected existing transaction");
                //
                // Create atomic group.
                //
                atomicGroupId = await this.CreateAtomicGroupAsync(cancellationToken);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Remove(items)", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e) || e is StateProviderFaultedException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(atomicGroupId);
                //
                // Remove just created transaction.
                //
                this.txManager.RemoveTransaction(rwtx);
                throw e;
            }
            Exception exp = null;
            TimeSpan operationTimeout = TimeSpan.FromMilliseconds(timeout.TotalMilliseconds / items.Count());
            try
            {
                AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.Remove(items)", "{0} processing", this.ToString());
                //
                // Remove each item as part of the same atomic group.
                //
                foreach (var x in items)
                {
                    wasRemoved |= await this.RemoveAsync(atomicGroupId, x, operationTimeout, cancellationToken);
                }
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Remove(items)", e, "{0} removing", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));

                if (e is ArgumentException || e is TimeoutException || e is OperationCanceledException)
                {
                    //
                    // The atomic group will be rolled back.
                    //
                    isCommit = false;
                    //
                    // Store original exception.
                    //
                    exp = e;
                }
                else
                {
                    //
                    // If RemoveAsync encountered an OutOfMemoryException it had already faulted the state provider.
                    //
                    throw e;
                }
            }
            try
            {
                //
                // Commit/Rollback atomic group. Cancellation not allowed from here on.
                //
                if (isCommit)
                {
                    await this.CommitAtomicGroupAsync(atomicGroupId, CancellationToken.None);
                }
                else
                {
                    await this.RollbackAtomicGroupAsync(atomicGroupId, CancellationToken.None);
                }
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Remove(items)", e, "{0} - {1}", this.ToString(), isCommit ? "commit" : "rollback");
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || this.IsNonRetryableReplicationException(e) || e is StateProviderFaultedException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Remove just created transaction.
                //
                if (null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                if (null != exp)
                {
                    //
                    // Store new exception.
                    //
                    exp = e;
                }
            }
            //
            // Retrow if necessary.
            //
            if (null != exp)
            {
                throw exp;
            }
            return wasRemoved;
        }

        #endregion

        #region INotifyCollectionChanged Members

        /// <summary>
        /// Collection changed notification.
        /// </summary>
        public event NotifyCollectionChangedEventHandler CollectionChanged;

        /// <summary>
        /// Raises the notification for each modification done to the dictionary.
        /// </summary>
        /// <param name="action"></param>
        /// <param name="change1"></param>
        /// <param name="change2"></param>
        void NotifyCollectionChanged(NotifyCollectionChangedAction action, object change1, object change2)
        {
            NotifyCollectionChangedEventHandler eventHandler = this.CollectionChanged;
            if (null != eventHandler)
            {
                switch (action)
                {
                    case NotifyCollectionChangedAction.Add:
                        eventHandler(this, new NotifyCollectionChangedEventArgs(action, change1));
                        break;
                    case NotifyCollectionChangedAction.Remove:
                        eventHandler(this, new NotifyCollectionChangedEventArgs(action, change1));
                        break;
                    case NotifyCollectionChangedAction.Reset:
                        eventHandler(this, new NotifyCollectionChangedEventArgs(action));
                        break;
                    case NotifyCollectionChangedAction.Replace:
                        eventHandler(this, new NotifyCollectionChangedEventArgs(action, change1, change2));
                        break;
                    case NotifyCollectionChangedAction.Move:
                        ReplicatedStateProvider.Assert(false, "unexpected notification");
                        break;
                }
            }
        }  

        #endregion

        #region IAtomicGroupStateProviderEx Members Overrides

        /// <summary>
        /// Forward call to the  btree store which has this information.
        /// </summary>
        /// <returns></returns>
        public override long GetLastCommittedSequenceNumber()
        {
            long storeLsn = this.btreeStore.GetLastCommittedSequenceNumber();
            AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.GetLastCommittedSequenceNumber", "{0} returns {1}", this.ToString(), storeLsn);
            return storeLsn;
        }

        /// <summary>
        /// Asks the state provider to copy its partial copy context.
        /// </summary>
        /// <returns></returns>
        public override IOperationDataStreamEx GetCopyContext()
        {
            this.ThrowIfFaulted();
            //
            // Partial copy is not supported.
            //
            return null;
        }

        /// <summary>
        /// Retrieves the copy stream from the btree store.
        /// </summary>
        /// <param name="upToSequenceNumber">Sequence number used for copy.</param>
        /// <param name="copyContext">Copy context. Used for partial copy. Unused.</param>
        /// <returns>Copy operation stream.</returns>
        public override IOperationDataStreamEx GetCopyState(long upToSequenceNumber, IOperationDataStreamEx copyContext)
        {
            this.ThrowIfFaulted();
            //
            // Copy performed by the btree store.
            //
            return this.btreeStore.GetCopyState(upToSequenceNumber);
        }

        /// <summary>
        /// Checkpoints the state provider.
        /// </summary>
        /// <param name="operationSequenceNumber">State mutations up to this sequence number must be made stable.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        public override Task CheckpointAsync(long operationSequenceNumber, CancellationToken cancellationToken)
        {
            this.ThrowIfFaulted();

            return this.btreeStore.CheckpointAsync(operationSequenceNumber, cancellationToken);
        }

        /// <summary>
        /// Informs the state provider of the progress in the operation stream.
        /// </summary>
        /// <param name="operationSequenceNumber">State mutations up to this sequence number could be made stable.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        public override Task<long> OnOperationStableAsync(long operationSequenceNumber, CancellationToken cancellationToken)
        {
            this.ThrowIfFaulted();

            return this.btreeStore.OnOperationStableAsync(operationSequenceNumber, cancellationToken);
        }

        #endregion

        #region IReplicatedStateProvider Member Overrides

        /// <summary>
        /// Opens the state provider.
        /// </summary>
        /// <param name="openMode">Used to distinguish between new and existent store.</param>
        /// <param name="cancellationToken">Cancellation token.</param>
        /// <returns></returns>
        protected override async Task OnOpenAsync(ReplicaOpenMode openMode, CancellationToken cancellationToken)
        {
            //
            // Initialize transaction manager.
            //
            AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.OnOpen", "{0} initialize transaction manager", this.ToString());
            this.txManager = new System.Fabric.Data.TransactionManager.TransactionManager(this.ServiceInstanceName);
            await this.txManager.OpenAsync(cancellationToken);
            //
            // Create configuration structs.
            //
            this.btreeConfiguration = new BtreeConfigurationDescription();
            this.btreeConfiguration.StorageConfiguration = new BtreeStorageConfigurationDescription();
            //
            // Set btree configuration.
            //
            this.btreeConfiguration.KeyComparison = this.keyComparison.KeyComparison;
            this.btreeConfiguration.PartitionId = this.PartitionId;
            this.btreeConfiguration.ReplicaId = this.ReplicaId;
            // 
            // Set btree storage configuration.
            //
            this.btreeConfiguration.StorageConfiguration.IsVolatile = this.IsVolatile;
            this.btreeConfiguration.StorageConfiguration.MaximumMemoryInMB = 1024;
            this.btreeConfiguration.StorageConfiguration.MaximumPageSizeInKB = 64;
            this.btreeConfiguration.StorageConfiguration.StoreDataInline = this.IsVolatile;
            if (!this.IsVolatile)
            {
                this.btreeConfiguration.StorageConfiguration.MaximumStorageInMB = 1024;
                this.btreeConfiguration.StorageConfiguration.Path = this.ConstructPath();
            }
            this.btreeConfiguration.StorageConfiguration.RetriesBeforeTimeout = 100;
            //
            // Initialize btree store.
            //
            AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.OnOpen", "{0} initialize store", this.ToString());
            await this.btreeStore.OpenAsync(this.btreeConfiguration, ReplicaOpenMode.Existing == openMode, cancellationToken);
            //
            // Populate current statistics.
            //
            BtreeStatistics stats = this.btreeStore.Statistics;
            this.lastStatistics.RecordCount = stats.RecordCount;
            this.lastStatistics.PageCount = stats.PageCount;
            this.lastStatistics.MemoryUsageInBytes = stats.MemoryUsageInBytes;
            this.lastStatistics.StorageUsageInBytes = stats.StorageUsageInBytes;
            ReplicatedStateProvider.Assert(this.lastStatistics.RecordCount >= 0, "unexpected record count");
            ReplicatedStateProvider.Assert(this.lastStatistics.PageCount >= 0, "unexpected page count");
            ReplicatedStateProvider.Assert(this.lastStatistics.MemoryUsageInBytes >= 0, "unexpected memory usage");
            ReplicatedStateProvider.Assert(this.lastStatistics.StorageUsageInBytes >= 0, "unexpected storage usage");
        }

        /// <summary>
        /// Closes the state provider.
        /// </summary>
        /// <param name="cancellationToken">Cancellation token used to cancel this operation.</param>
        /// <returns></returns>
        protected override async Task OnCloseAsync(CancellationToken cancellationToken)
        {
            //
            // Close btree store.
            //
            AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.OnClose", "{0} close store", this.ToString());
            await this.btreeStore.CloseAsync(false, cancellationToken);
            //
            // Close transaction manager.
            //
            AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.OnClose", "{0} close transaction manager", this.ToString());
            await this.txManager.CloseAsync(cancellationToken);
        }

        /// <summary>
        /// Aborts the state provider.
        /// </summary>
        protected override void OnAbort()
        {
            //
            // Abort btree store.
            //
            AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.OnAbort", "{0} abort store", this.ToString());
            this.btreeStore.Abort();
            //
            // Close transaction manager.
            //
            AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.OnAbort", "{0} abort transaction manager", this.ToString());
            this.txManager.Abort();
        }

        /// <summary>
        /// Siganls the state provider that the replica role is changing.
        /// </summary>
        /// <param name="newRole">New role for the state provider.</param>
        /// <param name="cancellationToken">Cancellation token used to cancel this operation.</param>
        /// <returns></returns>
        protected override async Task OnChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            //
            // Perform role change.
            //
            if (ReplicaRole.Unknown == this.ReplicaRole)
            {
                if (ReplicaRole.Primary == newRole)
                {
                    //
                    // Nothing to do.
                    //
                }
                if (ReplicaRole.IdleSecondary == newRole)
                {
                    //
                    // Nothing to do.
                    //
                }
                if (ReplicaRole.None == newRole)
                {
                    //
                    // Close btree store. Erase the storage associated with it.
                    //
                    AppTrace.TraceSource.WriteNoise(
                        "Data.SortedDictionaryStateProvider.OnChangeRole",
                        "{0} ({1}->{2}) erase store",
                        this.ToString(),
                        this.ReplicaRole,
                        newRole);
                    await this.btreeStore.CloseAsync(true, cancellationToken);
                    //
                    // Abort transaction manager.
                    //
                    AppTrace.TraceSource.WriteNoise(
                        "Data.SortedDictionaryStateProvider.OnChangeRole",
                        "{0} ({1}->{2}) abort transaction manager",
                        this.ToString(),
                        this.ReplicaRole,
                        newRole);
                    this.txManager.Abort();
                }
            }
            if (ReplicaRole.IdleSecondary == this.ReplicaRole)
            {
                if (ReplicaRole.Primary == newRole)
                {
                    //
                    // Nothing to do.
                    //
                }
                if (ReplicaRole.ActiveSecondary == newRole)
                {
                    //
                    // Nothing to do.
                    //
                }
                if (ReplicaRole.None == newRole)
                {
                    //
                    // Close btree store. Erase the storage associated with it.
                    //
                    AppTrace.TraceSource.WriteNoise(
                        "Data.SortedDictionaryStateProvider.OnChangeRole",
                        "{0} ({1}->{2}) erase store",
                        this.ToString(),
                        this.ReplicaRole,
                        newRole);
                    await this.btreeStore.CloseAsync(true, cancellationToken);
                    //
                    // Abort transaction manager.
                    //
                    AppTrace.TraceSource.WriteNoise(
                        "Data.SortedDictionaryStateProvider.OnChangeRole",
                        "{0} ({1}->{2}) abort transaction manager",
                        this.ToString(),
                        this.ReplicaRole,
                        newRole);
                    this.txManager.Abort();
                }
            }
            if (ReplicaRole.ActiveSecondary == this.ReplicaRole)
            {
                if (ReplicaRole.Primary == newRole)
                {
                    //
                    // Close transaction manager.
                    //
                    AppTrace.TraceSource.WriteNoise(
                        "Data.SortedDictionaryStateProvider.OnChangeRole",
                        "{0} ({1}->{2}) close transaction manager",
                        this.ToString(),
                        this.ReplicaRole,
                        newRole);
                    await this.txManager.CloseAsync(cancellationToken);
                    //
                    // Re-open transaction manager.
                    //
                    AppTrace.TraceSource.WriteNoise(
                        "Data.SortedDictionaryStateProvider.OnChangeRole",
                        "{0} ({1}->{2}) open transaction manager",
                        this.ToString(),
                        this.ReplicaRole,
                        newRole);
                    await this.txManager.OpenAsync(cancellationToken);
                }
                if (ReplicaRole.None == newRole)
                {
                    //
                    // Close btree store. Erase the storage associated with it.
                    //
                    AppTrace.TraceSource.WriteNoise(
                        "Data.SortedDictionaryStateProvider.OnChangeRole",
                        "{0} ({1}->{2}) erase store",
                        this.ToString(),
                        this.ReplicaRole,
                        newRole);
                    await this.btreeStore.CloseAsync(true, cancellationToken);
                    //
                    // Abort transaction manager.
                    //
                    AppTrace.TraceSource.WriteNoise(
                        "Data.SortedDictionaryStateProvider.OnChangeRole",
                        "{0} ({1}->{2}) abort transaction manager",
                        this.ToString(),
                        this.ReplicaRole,
                        newRole);
                    this.txManager.Abort();
                }
            }
            if (ReplicaRole.Primary == this.ReplicaRole)
            {
                if (ReplicaRole.ActiveSecondary == newRole)
                {
                    //
                    // Close transaction manager.
                    //
                    AppTrace.TraceSource.WriteNoise(
                        "Data.SortedDictionaryStateProvider.OnChangeRole",
                        "{0} ({1}->{2}) close transaction manager",
                        this.ToString(),
                        this.ReplicaRole,
                        newRole);
                    await this.txManager.CloseAsync(cancellationToken);
                    //
                    // Re-open transaction manager.
                    //
                    AppTrace.TraceSource.WriteNoise(
                        "Data.SortedDictionaryStateProvider.OnChangeRole",
                        "{0} ({1}->{2}) open transaction manager",
                        this.ToString(),
                        this.ReplicaRole,
                        newRole);
                    await this.txManager.OpenAsync(cancellationToken);
                }
                if (ReplicaRole.None == newRole)
                {
                    //
                    // Close btree store. Erase the storage associated with it.
                    //
                    AppTrace.TraceSource.WriteNoise(
                        "Data.SortedDictionaryStateProvider.OnChangeRole",
                        "{0} ({1}->{2}) erase store",
                        this.ToString(),
                        this.ReplicaRole,
                        newRole);
                    await this.btreeStore.CloseAsync(true, cancellationToken);
                    //
                    // Abort transaction manager.
                    //
                    AppTrace.TraceSource.WriteNoise(
                        "Data.SortedDictionaryStateProvider.OnChangeRole",
                        "{0} ({1}->{2}) abort transaction manager",
                        this.ToString(),
                        this.ReplicaRole,
                        newRole);
                    this.txManager.Abort();
                }
            }
        }

        /// <summary>
        /// Reports state provider load metrics in case they have changed.
        /// </summary>
        protected override void OnReportLoadMetrics()
        {
            LoadMetric memoryUsageInBytesLoad = null;
            LoadMetric storageUsageInBytesLoad = null;
            bool haveStatsChanged = false;
            //
            // Snapshot old and new statistics.
            //
            int memoryUsageInBytes = this.lastStatistics.MemoryUsageInBytes;
            int storageUsageInBytes = this.lastStatistics.StorageUsageInBytes;
            BtreeStatistics currentStatistics = this.btreeStore.Statistics;
            if (this.lastStatistics.MemoryUsageInBytes != Interlocked.CompareExchange(ref this.lastStatistics.MemoryUsageInBytes, currentStatistics.MemoryUsageInBytes, currentStatistics.MemoryUsageInBytes))
            {
                memoryUsageInBytesLoad = new LoadMetric(SortedDictionaryStateProvider<K, V, KC, KBC, VBC>.loadMetricNameMemoryUsageInBytes, currentStatistics.MemoryUsageInBytes);
                haveStatsChanged = true;
            }
            else
            {
                memoryUsageInBytesLoad = new LoadMetric(SortedDictionaryStateProvider<K, V, KC, KBC, VBC>.loadMetricNameMemoryUsageInBytes, memoryUsageInBytes);
            }
            if (this.lastStatistics.StorageUsageInBytes != Interlocked.CompareExchange(ref this.lastStatistics.StorageUsageInBytes, currentStatistics.StorageUsageInBytes, currentStatistics.StorageUsageInBytes))
            {
                storageUsageInBytesLoad = new LoadMetric(SortedDictionaryStateProvider<K, V, KC, KBC, VBC>.loadMetricNameStorageUsageInBytes, currentStatistics.StorageUsageInBytes);
                haveStatsChanged = true;
            }
            else
            {
                storageUsageInBytesLoad = new LoadMetric(SortedDictionaryStateProvider<K, V, KC, KBC, VBC>.loadMetricNameStorageUsageInBytes, storageUsageInBytes);
            }
            ReplicatedStateProvider.Assert(this.lastStatistics.MemoryUsageInBytes >= 0, "unexpected memory usage");
            ReplicatedStateProvider.Assert(this.lastStatistics.StorageUsageInBytes >= 0, "unexpected storage usage");
            //
            // Update other stats that are not reported as load metrics.
            //
            Interlocked.CompareExchange(ref this.lastStatistics.RecordCount, currentStatistics.RecordCount, currentStatistics.RecordCount);
            Interlocked.CompareExchange(ref this.lastStatistics.PageCount, currentStatistics.PageCount, currentStatistics.PageCount);
            ReplicatedStateProvider.Assert(this.lastStatistics.RecordCount >= 0, "unexpected record count");
            ReplicatedStateProvider.Assert(this.lastStatistics.PageCount >= 0, "unexpected page count");
            //
            // Check to see if load metrics have changed.
            //
            if (haveStatsChanged)
            {
                AppTrace.TraceSource.WriteNoise(
                    "Data.SortedDictionaryStateProvider.ReportLoadMetrics", 
                    "{0} ({1}, {2}) ({3}, {4})", 
                    this.ToString(),
                    memoryUsageInBytesLoad.Name,
                    memoryUsageInBytesLoad.Value,
                    storageUsageInBytesLoad.Name,
                    storageUsageInBytesLoad.Value);
                //
                // Report load metrics.
                //
                this.StatefulPartition.ReportLoad(new LoadMetric[] { memoryUsageInBytesLoad, storageUsageInBytesLoad });
            }
        }

        /// <summary>
        /// Called when a copy operation needs to be applied by the state provider.
        /// It is called on the secondary to perform copy stream drain processing.
        /// </summary>
        /// <param name="operation">Operation to apply.</param>
        /// <param name="cancellationToken">Cancellation token used to cancel processing.</param>
        /// <returns></returns>
        protected override Task OnCopyOperationAsync(IOperationEx operation, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise(
                "Data.SortedDictionaryStateProvider.OnCopyOperation",
                "{0} operation {1}",
                this.ToString(),
                operation.SequenceNumber);

            if (OperationTypeEx.EndOfStream == operation.OperationType)
            {
                //
                // Do nothing.
                //
                var tcs = new TaskCompletionSource<bool>();
                tcs.SetResult(true);
                return tcs.Task;
            }
            else
            {
                try
                {
                    //
                    // Apply in the btree store.
                    //
                    return this.btreeStore.ApplyCopyDataAsync(operation.Data, cancellationToken);
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.OnCopyOperation", e, "{0}", this.ToString());
                    //
                    // Partial copy not supported.
                    //
                    throw new StateProviderNonRetryableException();
                }
            }
        }

        /// <summary>
        /// Called when a recovery or replication operation needs to be applied by the state provider.
        /// </summary>
        /// <param name="isRecovery">True if this operation is part of recovery processing. Otherwise it is part of replication processing.</param>
        /// <param name="operation">Operation to apply.</param>
        /// <param name="cancellationToken">Cancellation token used to cancel processing.</param>
        /// <returns></returns>
        async Task ApplyRecoveryOrReplicationOperationAsync(bool isRecovery, IOperationEx operation, CancellationToken cancellationToken)
        {
            bool existing = false;
            IReadWriteTransaction rwtx = null;
            IBtreeOperation btreeOperation = null;           
            if (isRecovery)
            {
                if (this.allowReadableSecondary)
                {
                    AppTrace.TraceSource.WriteNoise(
                        "Data.SortedDictionaryStateProvider.ApplyRecoveryOperationAsync",
                        "{0} operation {1} readable",
                        this.ToString(),
                        operation.SequenceNumber);

                    switch (operation.OperationType)
                    {
                        case OperationTypeEx.CreateAtomicGroup:
                            //
                            // Create transaction for the new atomic group.
                            //
                            rwtx = this.txManager.CreateTransaction(operation.AtomicGroupId, out existing);
                            ReplicatedStateProvider.Assert(!existing, "unexpected existing transaction");
                            break;
                        case OperationTypeEx.CommitAtomicGroup:
                        case OperationTypeEx.RollbackAtomicGroup:
                            //
                            // Release transaction locks.
                            //
                            this.txManager.RemoveTransaction(operation.AtomicGroupId);
                            //
                            // Record operation stable.
                            //
                            await this.btreeStore.OnOperationStableAsync(operation.SequenceNumber, CancellationToken.None);
                            break;
                        case OperationTypeEx.Redo:
                        case OperationTypeEx.Undo:
                            //
                            // Decode operation first.
                            //
                            btreeOperation = await this.btreeStore.ApplyWithOutputAsync(operation.SequenceNumber, operation.Data, true, CancellationToken.None);
                            if (null == btreeOperation)
                            {
                                ReplicatedStateProvider.Assert(false, "null btree operation");
                            }
                            ReplicatedStateProvider.Assert(null != btreeOperation.Key, "null btree operation key");
                            //
                            // Acquire appropriate locks. Timeouts on applying replication operations are infinite.
                            //
                            rwtx = this.txManager.GetTransaction(operation.AtomicGroupId);
                            ReplicatedStateProvider.Assert(null != rwtx, "null transaction");
                            await this.AcquireWriteLocks(btreeOperation.Key, btreeOperation.OperationType, rwtx, Timeout.Infinite);
                            //
                            // Apply redo/undo operation in the btree store. 
                            //
                            btreeOperation = await this.btreeStore.ApplyWithOutputAsync(operation.SequenceNumber, operation.Data, false, CancellationToken.None);
                            ReplicatedStateProvider.Assert(
                                null != btreeOperation && null != btreeOperation.Key && null != btreeOperation.Value,
                                "unexpected apply with output");
                            break;
                        case OperationTypeEx.SingleOperation:
                            //
                            // Decode operation first.
                            //
                            btreeOperation = await this.btreeStore.ApplyWithOutputAsync(operation.SequenceNumber, operation.Data, true, CancellationToken.None);
                            if (null == btreeOperation)
                            {
                                ReplicatedStateProvider.Assert(false, "null btree operation");
                            }
                            ReplicatedStateProvider.Assert(
                                BtreeOperationType.Erase == btreeOperation.OperationType && null == btreeOperation.Key && null == btreeOperation.Value,
                                "unexpected btree operation");
                            //
                            // Acquire appropriate locks. Timeouts on applying replication/recovery operations are infinite.
                            //
                            rwtx = this.txManager.CreateTransaction(FabricReplicatorEx.InvalidAtomicGroupId, out existing);
                            ReplicatedStateProvider.Assert(!existing, "unexpected existing transaction");
                            await this.AcquireWriteLocks(btreeOperation.Key, btreeOperation.OperationType, rwtx, Timeout.Infinite);
                            //
                            // Apply redo only operation in the btree store.
                            //
                            btreeOperation = await this.btreeStore.ApplyWithOutputAsync(operation.SequenceNumber, operation.Data, false, CancellationToken.None);
                            ReplicatedStateProvider.Assert(
                                null != btreeOperation && BtreeOperationType.Erase == btreeOperation.OperationType && null == btreeOperation.Key && null == btreeOperation.Value,
                                "unexpected apply with output");
                            //
                            // Release transaction locks.
                            //
                            this.txManager.RemoveTransaction(rwtx);
                            break;
                        case OperationTypeEx.RedoPassComplete:
                            //
                            // Mark the state provider as readable.
                            //
                            this.isReadable = true;

                            AppTrace.TraceSource.WriteNoise(
                                "Data.SortedDictionaryStateProvider.ApplyRecoveryOperationAsync",
                                "{0} becomes readable",
                                this.ToString());
                            break;
                        case OperationTypeEx.UndoPassComplete:
                            //
                            // Nothing to do.
                            //
                            break;
                        case OperationTypeEx.EndOfStream:
                            //
                            // Nothing to do.
                            //
                            break;
                    }
                }
                else
                {
                    AppTrace.TraceSource.WriteNoise(
                        "Data.SortedDictionaryStateProvider.ApplyRecoveryOperationAsync",
                        "{0} operation {1}",
                        this.ToString(),
                        operation.SequenceNumber);

                    switch (operation.OperationType)
                    {
                        case OperationTypeEx.CreateAtomicGroup:
                            //
                            // Nothing to do.
                            //
                            break;
                        case OperationTypeEx.CommitAtomicGroup:
                        case OperationTypeEx.RollbackAtomicGroup:
                            //
                            // Record operation stable.
                            //
                            await this.btreeStore.OnOperationStableAsync(operation.SequenceNumber, CancellationToken.None);
                            break;
                        case OperationTypeEx.Redo:
                        case OperationTypeEx.Undo:
                            //
                            // Apply redo/undo operation in the btree store.
                            //
                            btreeOperation = await this.btreeStore.ApplyWithOutputAsync(operation.SequenceNumber, operation.Data, false, CancellationToken.None);
                            ReplicatedStateProvider.Assert(
                                null != btreeOperation && null != btreeOperation.Key && null != btreeOperation.Value,
                                "unexpected apply with output");
                            break;
                        case OperationTypeEx.SingleOperation:
                            //
                            // Apply redo only operation in the btree store.
                            //
                            btreeOperation = await this.btreeStore.ApplyWithOutputAsync(operation.SequenceNumber, operation.Data, false, CancellationToken.None);
                            ReplicatedStateProvider.Assert(
                                null != btreeOperation && BtreeOperationType.Erase == btreeOperation.OperationType && null == btreeOperation.Key && null == btreeOperation.Value,
                                "unexpected apply with output");
                            break;
                        case OperationTypeEx.RedoPassComplete:
                        case OperationTypeEx.UndoPassComplete:
                            //
                            // Nothing to do.
                            //
                            break;
                        case OperationTypeEx.EndOfStream:
                            //
                            // Nothing to do.
                            //
                            break;
                    }
                }
            }
            else
            {
                //
                // We are either in the online rollback stream processing on the primary or 
                // in the replication stream processing on the secondary.
                //
                if (ReplicaRole.Primary == this.ReplicaRole)
                {
                    AppTrace.TraceSource.WriteNoise(
                        "Data.SortedDictionaryStateProvider.ApplyReplicationOperationAsync",
                        "{0} operation {1} undo stream",
                        this.ToString(),
                        operation.SequenceNumber);

                    NotifyCollectionChangedEventHandler eventHandler = null;
                    switch (operation.OperationType)
                    {
                        case OperationTypeEx.CreateAtomicGroup:
                        case OperationTypeEx.AbortAtomicGroup:
                        case OperationTypeEx.CommitAtomicGroup:
                        case OperationTypeEx.Redo:
                        case OperationTypeEx.SingleOperation:
                            ReplicatedStateProvider.Assert(false, "unexpected operation type");
                            break;
                        case OperationTypeEx.RollbackAtomicGroup:
                            //
                            // Fire notifications if needed.
                            //
                            eventHandler = this.CollectionChanged;
                            if (null != eventHandler)
                            {
                                //
                                // Retrieve transaction.
                                //
                                rwtx = this.txManager.GetTransaction(operation.AtomicGroupId);
                                ReplicatedStateProvider.Assert(null != rwtx, "unexpected null transaction");
                                //
                                // Batch fire notifications.
                                //
                                this.FireAllCollectionChangedNotifications(rwtx);
                            }
                            //
                            // Release transaction locks.
                            //
                            this.txManager.RemoveTransaction(operation.AtomicGroupId);
                            break;
                        case OperationTypeEx.Undo:
                            //
                            // Locks are still being held at this time.
                            //
                            btreeOperation = await this.btreeStore.ApplyWithOutputAsync(operation.SequenceNumber, operation.Data, false, CancellationToken.None);
                            ReplicatedStateProvider.Assert(
                                null != btreeOperation && null != btreeOperation.Key && null != btreeOperation.Value,
                                "unexpected apply with output");
                            //
                            // Reconstruct notifications if needed.
                            //
                            eventHandler = this.CollectionChanged;
                            if (null != eventHandler)
                            {
                                //
                                // Retrieve transaction.
                                //
                                rwtx = this.txManager.GetTransaction(operation.AtomicGroupId);
                                ReplicatedStateProvider.Assert(null != rwtx, "unexpected null transaction");
                                //
                                // Reconstruct state provider operation.
                                //
                                if (null == btreeOperation)
                                {
                                    ReplicatedStateProvider.Assert(null != btreeOperation, "unexpected btree operation");
                                }
                                StateProviderOperation stateProviderOperation = null;
                                switch (btreeOperation.OperationType)
                                {
                                    case BtreeOperationType.Insert:
                                        stateProviderOperation = new SortedDictionaryStateProviderAddOperation<K, V>();
                                        ((SortedDictionaryStateProviderAddOperation<K, V>)stateProviderOperation).Key = (new BtreeKey<K, KBC>(btreeOperation.Key.Bytes)).Key;
                                        ((SortedDictionaryStateProviderAddOperation<K, V>)stateProviderOperation).Value = (new BtreeValue<V, VBC>(btreeOperation.Value.Bytes)).Value;
                                        break;
                                    case BtreeOperationType.Delete:
                                        stateProviderOperation = new SortedDictionaryStateProviderRemoveOperation<K, V>();
                                        ((SortedDictionaryStateProviderRemoveOperation<K, V>)stateProviderOperation).Key = (new BtreeKey<K, KBC>(btreeOperation.Key.Bytes)).Key;
                                        ((SortedDictionaryStateProviderRemoveOperation<K, V>)stateProviderOperation).Value = (new BtreeValue<V, VBC>(btreeOperation.Value.Bytes)).Value;
                                        break;
                                    case BtreeOperationType.Update:
                                        stateProviderOperation = new SortedDictionaryStateProviderUpdateOperation<K, V>();
                                        ((SortedDictionaryStateProviderUpdateOperation<K, V>)stateProviderOperation).Key = (new BtreeKey<K, KBC>(btreeOperation.Key.Bytes)).Key;
                                        ((SortedDictionaryStateProviderUpdateOperation<K, V>)stateProviderOperation).NewValue = (new BtreeValue<V, VBC>(btreeOperation.Value.Bytes)).Value;
                                        break;
                                    case BtreeOperationType.Erase:
                                        stateProviderOperation = new SortedDictionaryStateProviderClearOperation();
                                        break;
                                    case BtreeOperationType.PartialUpdate:
                                        ReplicatedStateProvider.Assert(false, "unexpected operation type");
                                        break;
                                }
                                ReplicatedStateProvider.Assert(null != stateProviderOperation, "unexpected null operation");
                                //
                                // Notification will be fired when the transaction is completely rollback.
                                //
                                rwtx.Context.Add(stateProviderOperation);
                            }
                            break;
                        case OperationTypeEx.EndOfStream:
                            //
                            // Nothing to do.
                            //
                            break;
                    }
                }
                else
                {
                    if (this.allowReadableSecondary)
                    {
                        AppTrace.TraceSource.WriteNoise(
                            "Data.SortedDictionaryStateProvider.ApplyReplicationOperationAsync",
                            "{0} operation {1} readable",
                            this.ToString(),
                            operation.SequenceNumber);

                        switch (operation.OperationType)
                        {
                            case OperationTypeEx.CreateAtomicGroup:
                                //
                                // Create transaction for the new atomic group.
                                //
                                rwtx = this.txManager.CreateTransaction(operation.AtomicGroupId, out existing);
                                ReplicatedStateProvider.Assert(!existing, "unexpected existing transaction");
                                break;
                            case OperationTypeEx.CommitAtomicGroup:
                            case OperationTypeEx.RollbackAtomicGroup:
                                //
                                // Release transaction locks.
                                //
                                this.txManager.RemoveTransaction(operation.AtomicGroupId);
                                //
                                // Record operation stable.
                                //
                                await this.btreeStore.OnOperationStableAsync(operation.SequenceNumber, CancellationToken.None);
                                break;
                            case OperationTypeEx.AbortAtomicGroup:
                                //
                                // Release transaction locks.
                                //
                                this.txManager.RemoveTransaction(operation.AtomicGroupId);
                                break;
                            case OperationTypeEx.Redo:
                            case OperationTypeEx.Undo:
                                //
                                // Decode operation first.
                                //
                                btreeOperation = await this.btreeStore.ApplyWithOutputAsync(operation.SequenceNumber, operation.Data, true, CancellationToken.None);
                                if (null == btreeOperation)
                                {
                                    ReplicatedStateProvider.Assert(false, "null btree operation");
                                }
                                ReplicatedStateProvider.Assert(null != btreeOperation.Key, "null btree operation key");
                                //
                                // Acquire appropriate locks. Timeouts on applying replication operations are infinite.
                                //
                                rwtx = this.txManager.GetTransaction(operation.AtomicGroupId);
                                ReplicatedStateProvider.Assert(null != rwtx, "unexpected null transaction");
                                await this.AcquireWriteLocks(btreeOperation.Key, btreeOperation.OperationType, rwtx, Timeout.Infinite);
                                //
                                // Apply redo/undo operation in the btree store. 
                                //
                                btreeOperation = await this.btreeStore.ApplyWithOutputAsync(operation.SequenceNumber, operation.Data, false, CancellationToken.None);
                                ReplicatedStateProvider.Assert(
                                    null != btreeOperation && null != btreeOperation.Key && null != btreeOperation.Value,
                                    "unexpected apply with output");
                                break;
                            case OperationTypeEx.SingleOperation:
                                //
                                // Decode operation first.
                                //
                                btreeOperation = await this.btreeStore.ApplyWithOutputAsync(operation.SequenceNumber, operation.Data, true, CancellationToken.None);
                                if (null == btreeOperation)
                                {
                                    ReplicatedStateProvider.Assert(false, "null btree operation");
                                }
                                ReplicatedStateProvider.Assert(BtreeOperationType.Erase == btreeOperation.OperationType, "unexpected apply with output");
                                //
                                // Acquire appropriate locks. Timeouts on applying replication operations are infinite.
                                //
                                rwtx = this.txManager.CreateTransaction(FabricReplicatorEx.InvalidAtomicGroupId, out existing);
                                ReplicatedStateProvider.Assert(!existing, "unexpected existing transaction");
                                await this.AcquireWriteLocks(btreeOperation.Key, btreeOperation.OperationType, rwtx, Timeout.Infinite);
                                //
                                // Apply redo only operation in the btree store.
                                //
                                btreeOperation = await this.btreeStore.ApplyWithOutputAsync(operation.SequenceNumber, operation.Data, false, CancellationToken.None);
                                ReplicatedStateProvider.Assert(
                                    null != btreeOperation && BtreeOperationType.Erase == btreeOperation.OperationType,
                                    "unexpected apply with output");
                                //
                                // Release transaction locks.
                                //
                                this.txManager.RemoveTransaction(rwtx);
                                break;
                            case OperationTypeEx.EndOfStream:
                                //
                                // Nothing to do.
                                //
                                break;
                            case OperationTypeEx.RedoPassComplete: /* Work item 1189766. */
                                //
                                // Mark the state provider as readable.
                                //
                                this.isReadable = true;

                                AppTrace.TraceSource.WriteNoise(
                                    "Data.SortedDictionaryStateProvider.ApplyReplicationOperationAsync",
                                    "{0} becomes readable",
                                    this.ToString());
                                break;
                        }
                    }
                    else
                    {
                        AppTrace.TraceSource.WriteNoise(
                            "Data.SortedDictionaryStateProvider.ApplyReplicationOperationAsync",
                            "{0} operation {1}",
                            this.ToString(),
                            operation.SequenceNumber);

                        switch (operation.OperationType)
                        {
                            case OperationTypeEx.CreateAtomicGroup:
                            case OperationTypeEx.AbortAtomicGroup:
                                //
                                // Nothing to do.
                                //
                                break;
                            case OperationTypeEx.CommitAtomicGroup:
                            case OperationTypeEx.RollbackAtomicGroup:
                                //
                                // Record operation stable.
                                //
                                await this.btreeStore.OnOperationStableAsync(operation.SequenceNumber, CancellationToken.None);
                                break;
                            case OperationTypeEx.Redo:
                            case OperationTypeEx.Undo:
                                //
                                // Apply redo/undo operation.
                                //
                                await this.btreeStore.ApplyWithOutputAsync(operation.SequenceNumber, operation.Data, false, CancellationToken.None);
                                break;
                            case OperationTypeEx.SingleOperation:
                                //
                                // Apply redo only operation.
                                //
                                await this.btreeStore.ApplyWithOutputAsync(operation.SequenceNumber, operation.Data, false, CancellationToken.None);
                                break;
                            case OperationTypeEx.RedoPassComplete: /* Work item 1189766. */
                            case OperationTypeEx.EndOfStream:
                                //
                                // Nothing to do.
                                //
                                break;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Called when a recovery operation needs to be applied by the state provider.
        /// It is called on the primary and secondary to perform recovery stream drain processing.
        /// </summary>
        /// <param name="operation">Operation to apply.</param>
        /// <param name="cancellationToken">Cancellation token used to cancel processing.</param>
        /// <returns></returns>
        protected override Task OnRecoveryOperationAsync(IOperationEx operation, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise(
                "Data.SortedDictionaryStateProvider.OnRecoveryOperation",
                "{0} operation {1}",
                this.ToString(),
                operation.SequenceNumber);

            try
            {
                //
                // Apply in the btree store.
                //
                return this.ApplyRecoveryOrReplicationOperationAsync(true, operation, cancellationToken);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.OnRecoveryOperation", e, "{0}", this.ToString());
                //
                // Replica unusable.
                //
                throw new StateProviderNonRetryableException();
            }
        }

        /// <summary>
        /// Called when a replication operation needs to be applied by the state provider.
        /// It is called on the primary to perform undo stream drain processing and 
        /// on the secondary to perform replication stream drain processing.
        /// </summary>
        /// <param name="operation">Operation to apply.</param>
        /// <param name="cancellationToken">Cancellation token used to cancel processing.</param>
        /// <returns></returns>
        protected override Task OnReplicationOperationAsync(IOperationEx operation, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise(
                "Data.SortedDictionaryStateProvider.OnReplicationOperation",
                "{0} operation {1}",
                this.ToString(),
                operation.SequenceNumber);

            try
            {
                //
                // Apply in the btree store.
                //
                return this.ApplyRecoveryOrReplicationOperationAsync(false, operation, cancellationToken);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.OnReplicationOperation", e, "{0}", this.ToString());
                //
                // Replica unusable.
                //
                throw new StateProviderNonRetryableException();
            }
        }

        /// <summary>
        /// Called when an atomic group is committed on the primary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group to commit.</param>
        /// <param name="commitSequenceNumber">Sequence number of the commit operation.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        protected override async Task OnAtomicGroupCommitAsync(long atomicGroupId, long commitSequenceNumber, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise(
                "Data.SortedDictionaryStateProvider.OnAtomicGroupCommit",
                "{0} atomic group {1} operation {2}",
                this.ToString(),
                atomicGroupId,
                commitSequenceNumber);
            //
            // Retrieve transaction for the given atomic group.
            //
            IReadWriteTransaction rwtx = this.txManager.GetTransaction(atomicGroupId);
            ReplicatedStateProvider.Assert(null != rwtx, "unexpected null transaction");
            if (!this.IsFaulted())
            {
                //
                // Fire notifications if needed.
                //
                NotifyCollectionChangedEventHandler eventHandler = this.CollectionChanged;
                if (null != eventHandler)
                {
                    //
                    // Batch fire notifications.
                    //
                    this.FireAllCollectionChangedNotifications(rwtx);
                }
            }
            //
            // Release transaction locks.
            //
            this.txManager.RemoveTransaction(rwtx);
            //
            // Record operation stable.
            //
            await this.btreeStore.OnOperationStableAsync(commitSequenceNumber, CancellationToken.None);
        }

        /// <summary>
        /// Called when an atomic group is rolled back on the primary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group to abort.</param>
        /// <param name="rollbackSequenceNumber">Sequence number of the rollback operation.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        protected override async Task OnAtomicGroupRollbackAsync(long atomicGroupId, long rollbackSequenceNumber, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise(
                "Data.SortedDictionaryStateProvider.OnAtomicGroupRollback",
                "{0} atomic group {1}",
                this.ToString(),
                atomicGroupId);
            //
            // Record operation stable.
            //
            await this.btreeStore.OnOperationStableAsync(rollbackSequenceNumber, CancellationToken.None);
        }

        /// <summary>
        /// Called when an atomic group is aborted on the primary.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group to abort.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        protected override Task OnAtomicGroupAbortAsync(long atomicGroupId, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise(
                "Data.SortedDictionaryStateProvider.OnAtomicGroupAbort", 
                "{0} atomic group {1}", 
                this.ToString(),
                atomicGroupId);
            //
            // Release transaction locks.
            //
            this.txManager.RemoveTransaction(atomicGroupId);
            //
            // Complete this call synchronously.
            //
            TaskCompletionSource<bool> synchronousCompletion = new TaskCompletionSource<bool>();
            synchronousCompletion.SetResult(true);
            return synchronousCompletion.Task;
        }

        #endregion

        #region ReplicatedStateProvider Member Overrides 

        /// <summary>
        /// Completes an in-flight replication operation on the primary.
        /// </summary>
        /// <param name="operation">Operation completed.</param>
        /// <param name="fault">If operation was not replicated successfully, it identifies the error condition.</param>
        /// <param name="cancellationToken">Cancellation token used to cancel this operation.</param>
        /// <returns></returns>
        protected async override Task<object> OnReplicationOperationAsync(StateProviderOperation operation, Exception fault, CancellationToken cancellationToken)
        {
            Exception exp = null;
            IReadWriteTransaction rwtx = ((SortedDictionaryStateProviderOperation)operation).Tx;
            ReplicatedStateProvider.Assert(operation.AtomicGroupId == rwtx.AtomicGroupId, "unexpected atomic group");

            AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.OnReplicationOperationAsync", "{0}", this.ToString());
            //
            // Complete this call synchronously.
            //
            TaskCompletionSource<object> synchronousCompletion = new TaskCompletionSource<object>();
            //
            // If there was a replication exception or the state provider is faulted, do not apply.
            //
            if (null != fault && !this.IsFaulted())
            {
                try
                {
                    //
                    // Apply operation. 
                    //
                    NotifyCollectionChangedEventHandler eventHandler = this.CollectionChanged;
                    if (FabricReplicatorEx.InvalidAtomicGroupId == rwtx.AtomicGroupId)
                    {
                        if (null != eventHandler)
                        {
                            //
                            // Fire notifications under a lock such that notifications from concurrent transactions are not interleaved.
                            //
                            lock (this)
                            {
                                //
                                // Fire notification of collection changed.
                                //
                                this.FireCollectionChangedNotification(operation);
                            }
                        }
                    }
                    else
                    {
                        if (null != eventHandler)
                        {
                            //
                            // Notification will be fired when and if the transaction commits.
                            //
                            rwtx.Context.Add(operation);
                        }
                    }
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.OnReplicationOperationAsync", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(e is OutOfMemoryException, string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Store original exception.
                    //
                    exp = e;
                }
            }
            //
            // Release transaction locks, if the transaction is trivial. 
            // If the transaction is not trivial, its locks will be released during commit/rollback/abort.
            //
            if (null != fault && FabricReplicatorEx.InvalidAtomicGroupId == rwtx.AtomicGroupId)
            {
                //
                // Release locks.
                //
                this.txManager.RemoveTransaction(rwtx);
                //
                // Record operation stable.
                //
                await this.btreeStore.OnOperationStableAsync(operation.SequenceNumber, CancellationToken.None);
            }
            //
            // If an error occured, throw exception.
            //
            if (null != exp)
            {
                synchronousCompletion.SetException(exp);
            }
            else
            {
                synchronousCompletion.SetResult(null);
            }
            return synchronousCompletion.Task;
        }

        /// <summary>
        /// Specifies if replication operations are completed in order on the primary. 
        /// </summary>
        public override bool IsOperationCompletionOrdered
        {
            get
            {
                return false;
            }
        }

        #endregion

        #region IReadOnlyDictionary<K, V> Members

        /// <summary>
        /// Gets a collection containing the keys in the dictionary.
        /// </summary>
        IEnumerable<K> IReadOnlyDictionary<K, V>.Keys
        {
            get 
            {
                return new SerializableSortedDictionaryKeyEnumerable<K, V>(this.GetEnumerator());
            }
        }

        /// <summary>
        /// Gets a collection containing the values in the dictionary.
        /// </summary>
        IEnumerable<V> IReadOnlyDictionary<K, V>.Values
        {
            get 
            {
                return new SerializableSortedDictionaryValueEnumerable<K, V>(this.GetEnumerator());
            }
        }

        #endregion

        #region IDictionary Members
        
        /// <summary>
        /// Adds the specified key and value to the dictionary.
        /// </summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value of the element to add. The value can be null for reference types.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentException">An element with the same key already exists.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        public void Add(object key, object value)
        {
            this.Add((K)key, (V)value);
        }

        /// <summary>
        /// Determines whether the dictionary contains a specific key.
        /// </summary>
        /// <param name="key">The key to locate in the dictionary.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <returns></returns>
        public bool Contains(object key)
        {
            return this.ContainsKey((K)key);
        }

        /// <summary>
        /// Returns an enumerator that iterates through the dictionary.
        /// </summary>
        /// <returns></returns>
        IDictionaryEnumerator IDictionary.GetEnumerator()
        {
            return this.GetEnumerator() as IDictionaryEnumerator;
        }

        /// <summary>
        /// Gets a value indicating whether the dictionary has a fixed size.
        /// </summary>
        public bool IsFixedSize
        {
            get { return false; }
        }

        /// <summary>
        /// Gets an ICollection object containing the keys of the dictionary.
        /// </summary>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        ICollection IDictionary.Keys
        {
            get 
            {
                List<K> keys = new List<K>();
                using (IEnumerator<KeyValuePair<K, V>> enumerator = this.GetEnumerator())
                {
                    while (enumerator.MoveNext())
                    {
                        keys.Add(enumerator.Current.Key);
                    }
                }
                return keys;
            }
        }

        /// <summary>
        /// Gets an ICollection object containing the values in the dictionary.
        /// </summary>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <returns></returns>
        ICollection IDictionary.Values
        {
            get 
            {
                List<V> values = new List<V>();
                using (IEnumerator<KeyValuePair<K, V>> enumerator = this.GetEnumerator())
                {
                    while (enumerator.MoveNext())
                    {
                        values.Add(enumerator.Current.Value);
                    }
                }
                return values;
            }
        }

        /// <summary>
        /// Removes the value with the specified key from the dictionary.
        /// </summary>
        /// <param name="key">The key of the element to remove.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">Atomic group id is invalid.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public void Remove(object key)
        {
            this.Remove((K)key);
        }

        /// <summary>
        /// Gets or sets the value associated with the specified key.
        /// </summary>
        /// <param name="key">The key of the value to get or set.</param>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.Collections.Generic.KeyNotFoundException">The key was not found.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        public object this[object key]
        {
            get
            {
                V value = default(V);
                bool found = this.TryGetValue((K)key, out value);
                if (!found)
                {
                    throw new KeyNotFoundException();
                }
                return value;
            }
            set
            {
                this.AddOrUpdateAsync((K)key, (V)value).Wait();
            }
        }

        /// <summary>
        /// Copies the dictionary value collection elements to an existing one-dimensional array, starting at the specified array index        
        /// </summary>
        /// <param name="array">The one-dimensional array that is the destination of the elements copied from dictionary value collection. 
        /// The array must have zero-based indexing.</param>
        /// <param name="index">The zero-based index in array at which copying begins.</param>
        public void CopyTo(Array array, int index)
        {
            if (null == array)
            {
                throw new ArgumentNullException("array", StringResources.Error_NullArray);
            }
            if (0 > index || index > array.Length)
            {
                throw new ArgumentOutOfRangeException("index");
            }
            ICollection<V> values = this.Values;
            if (index + values.Count > array.Length)
            {
                throw new ArgumentOutOfRangeException("index");
            }
            for (int idx = 0; idx < values.Count; idx++)
            {
                array.SetValue(values.ElementAt(idx), index + idx);
            }
        }

        /// <summary>
        /// Gets a value indicating whether access to the dictionary is synchronized with the SyncRoot.
        /// </summary>
        public bool IsSynchronized
        {
            get { return false; }
        }

        /// <summary>
        /// Gets an object that can be used to synchronize access to the dictionary.
        /// </summary>
        public object SyncRoot
        {
            get { return this; }
        }

        #endregion

        #region Helper Methods

        /// <summary>
        /// Acquires necessary btree and key locks in a given read transaction.
        /// </summary>
        /// <param name="key">Key to lock.</param>
        /// <param name="operationType">Btree operation.</param>
        /// <param name="rwtx">Read transaction.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <returns></returns>
        async Task AcquireReadLocks(IBtreeKey key, IReadOnlyTransaction rotx, int timeout)
        {
            if (null == key)
            {
                ReplicatedStateProvider.Assert(false, "null key");
            }
            ReplicatedStateProvider.Assert(null != key.Bytes, "null key bytes");
            //
            // Build the key lock.
            //
            string keyLockResourceName = CRC64.ToCRC64String(key.Bytes);
            //
            // Intent shared lock allows multiple read and write operation to execute concurrently on different keys.
            //
            int lockTimeout = timeout;
            DateTime dtStart = DateTime.UtcNow;
            await rotx.LockAsync(SortedDictionaryStateProvider<K, V, KC, KBC, VBC>.btreeLock, LockMode.IntentShared, lockTimeout);
            //
            // Compute left-over timeout, if needed.
            //
            if (Timeout.Infinite != timeout)
            {
                lockTimeout = 0;
                DateTime dtEnd = DateTime.UtcNow;
                TimeSpan tsDuration = dtEnd - dtStart;
                ReplicatedStateProvider.Assert(0 <= tsDuration.TotalMilliseconds, "unexpected duration");
                if (timeout < tsDuration.TotalMilliseconds)
                {
                    throw new TimeoutException();
                }
                lockTimeout = timeout - (int)tsDuration.TotalMilliseconds;
            }
            //
            // Shared lock on this key allows a multiple reads to occur on this key at any given time. 
            //
            await rotx.LockAsync(keyLockResourceName, LockMode.Shared, lockTimeout);
        }

        /// <summary>
        /// Acquires necessary btree and key locks in a given read/write transaction.
        /// </summary>
        /// <param name="key">Key to lock.</param>
        /// <param name="operationType">Btree operation.</param>
        /// <param name="rwtx">Read/write transaction.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <returns></returns>
        async Task AcquireWriteLocks(IBtreeKey key, BtreeOperationType operationType, IReadWriteTransaction rwtx, int timeout)
        {
            if (null == key)
            {
                ReplicatedStateProvider.Assert(false, "null key");
            }
            ReplicatedStateProvider.Assert(null != key.Bytes, "null key bytes");
            //
            // Build the key lock.
            //
            string keyLockResourceName = CRC64.ToCRC64String(key.Bytes);
            //
            // Identify the btree store lock.
            //
            int lockTimeout = timeout;
            DateTime dtStart = DateTime.UtcNow;
            switch (operationType)
            {
                case BtreeOperationType.Insert:
                case BtreeOperationType.Delete:
                case BtreeOperationType.Update:
                    //
                    // Intent exclusive lock allows multiple write operation to execute concurrently on different keys.
                    //
                    await rwtx.LockAsync(SortedDictionaryStateProvider<K, V, KC, KBC, VBC>.btreeLock, LockMode.IntentExclusive, lockTimeout);
                    break;
                case BtreeOperationType.Erase:
                    //
                    // This is the only write operation allowed.
                    //
                    await rwtx.LockAsync(SortedDictionaryStateProvider<K, V, KC, KBC, VBC>.btreeLock, LockMode.Exclusive, lockTimeout);
                    break;
                case BtreeOperationType.PartialUpdate:
                    ReplicatedStateProvider.Assert(false, "unexpected operation type");
                    break;
            }
            if (BtreeOperationType.Erase != operationType)
            {
                //
                // No key lock required.
                //
                return;
            }
            //
            // Compute left-over timeout, if needed.
            //
            if (Timeout.Infinite != timeout)
            {
                lockTimeout = 0;
                DateTime dtEnd = DateTime.UtcNow;
                TimeSpan tsDuration = dtEnd - dtStart;
                ReplicatedStateProvider.Assert(0 <= tsDuration.TotalMilliseconds, "unexpected duration");
                if (timeout < tsDuration.TotalMilliseconds)
                {
                    throw new TimeoutException();
                }
                lockTimeout = timeout - (int)tsDuration.TotalMilliseconds;
            }
            //
            // Exclusive lock on this key allows a single write to occur on this key at any given time. 
            //
            await rwtx.LockAsync(keyLockResourceName, LockMode.Exclusive, lockTimeout);
        }

        /// <summary>
        /// Internal method of performing a remove operation.
        /// </summary>
        /// <param name="atomicGroupId">Atomic group in which this operation is participating.</param>
        /// <param name="item">The item to remove.</param>
        /// <param name="isConditional">Specifies if the operation is conditional based on the value of the element to remove.</param>
        /// <param name="timeout">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricNotPrimaryException">The state provider is not in primary role.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotWritableException">The state provider is not writable.</exception>
        /// <returns></returns>
        async Task<bool> RemoveAsync(long atomicGroupId, KeyValuePair<K, V> item, bool isConditional, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Exception exp = null;
            bool wasRemoved = false;
            long sequenceNumber = FabricReplicatorEx.InvalidSequenceNumber;
            bool existing = false;
            //
            // Create dictionary remove operation.
            //
            SortedDictionaryStateProviderRemoveOperation<K, V> operation = new SortedDictionaryStateProviderRemoveOperation<K, V>();
            operation.Key = item.Key;
            operation.Value = item.Value;
            //
            // Create key and value bytes.
            //
            BtreeKey<K, KBC> btreeKey = new BtreeKey<K, KBC>(item.Key);
            this.ThrowIfKeySizeExceeded(btreeKey);
            BtreeValue<V, VBC> btreeValue = isConditional ? new BtreeValue<V,VBC>(item.Value) : null;
            Tuple<bool, IRedoUndoInformation> result = null;
            IReadWriteTransaction rwtx = null;
            int milliseconds = this.GetTimeoutInMilliseconds(timeout);
            //
            // Make sure writes are allowed.
            //
            this.ThrowIfFaulted();
            this.ThrowIfNotWritable();

            try
            {
                AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.Remove(conditional)", "{0}", this.ToString());
                //
                // Create or get read/write transaction for the given atomic group. 
                //
                rwtx = this.txManager.CreateTransaction(atomicGroupId, out existing);
                //
                // Acquire appropriate locks. 
                //
                await this.AcquireWriteLocks(btreeKey, BtreeOperationType.Delete, rwtx, milliseconds);
                cancellationToken.ThrowIfCancellationRequested();
                //
                // Reserve operation sequence number for the delete operation. 
                //
                sequenceNumber = await this.ReserveSequenceNumberAsync(cancellationToken);
                //
                // Delete from btree store.
                //
                result = isConditional ?
                    await this.btreeStore.ConditionalDeleteAsync(btreeKey, 0, btreeValue, sequenceNumber, cancellationToken) :
                    await this.btreeStore.DeleteAsync(btreeKey, sequenceNumber, cancellationToken);
                wasRemoved = result.Item1;
                ReplicatedStateProvider.Assert(
                    (wasRemoved && (null != result.Item2.Redo && null != result.Item2.Undo)) || (!wasRemoved && (null == result.Item2.Redo && null == result.Item2.Undo)),
                    "unexpected delete redo/undo");
                //
                // Set operation redo/undo/metadata.
                //
                operation.SequenceNumber = sequenceNumber;
                operation.AtomicGroupId = atomicGroupId;
                operation.RedoInfo = result.Item2.Redo;
                operation.UndoInfo = result.Item2.Undo;
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Remove(conditional)", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is ArgumentException || e is KeyNotFoundException || e is OutOfMemoryException || e is TimeoutException || e is OperationCanceledException || this.IsNonRetryableReplicationException(e),
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(sequenceNumber);
                //
                // If transaction was just created, destroy it since it won't be committed or rolled back.
                //
                if (!existing && null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
                if (e is OutOfMemoryException)
                {
                    //
                    // The operation could not be performed. State provider is now faulted.
                    //
                    this.SetFaulted(FaultType.Transient);
                }
                //
                // Ignore key not found exception.
                //
                if (!(e is KeyNotFoundException))
                {
                    throw e;
                }
            }
            if (wasRemoved)
            {
                try
                {
                    //
                    // Set operation transaction.
                    //
                    operation.Tx = rwtx;
                    //
                    // Replicate remove operation. Cancellation not allowed from here on.
                    //
                    await this.ReplicateOperationAsync(operation, CancellationToken.None);
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Remove(conditional)", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(
                        this.IsNonRetryableReplicationException(e) || e is OutOfMemoryException || e is StateProviderFaultedException,
                        string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Store original exception.
                    //
                    exp = e;
                }
            }
            if (!wasRemoved || null != exp)
            {
                //
                // Cancel sequence number.
                //
                this.AtomicGroupStateReplicatorEx.CancelSequenceNumber(sequenceNumber);
                //
                // If transaction was just created, destroy it since it won't be committed or rolled back.
                //
                if (!existing && null != rwtx)
                {
                    this.txManager.RemoveTransaction(rwtx);
                }
            }
            if (null != exp)
            {
                throw exp;
            }
            return wasRemoved;
        }

        /// <summary>
        /// Determines whether the dictionary contains a specific element.
        /// </summary>
        /// <param name="item">The element to locate in the dictionary.</param>
        /// <param name="isConditional">Specifies if the operation is conditional based on the value of the element.</param>
        /// <returns></returns>
        /// <exception cref="System.ArgumentNullException">Key is null.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">The object has been closed.</exception>
        bool Contains(KeyValuePair<K, V> item, bool isConditional)
        {
            //
            // Check arguments.
            //
            if (null == item.Key)
            {
                throw new ArgumentNullException("item", StringResources.Error_NullKey);
            }
            Exception exp = null;
            //
            // Create key and value bytes.
            //
            BtreeKey<K, KBC> btreeKey = new BtreeKey<K, KBC>(item.Key);
            this.ThrowIfKeySizeExceeded(btreeKey);
            IBtreeValue btreeValue = null;
            IReadOnlyTransaction rotx = null;
            //
            // Make sure reads are allowed.
            //
            this.ThrowIfFaulted();
            this.ThrowIfNotReadable();

            try
            {
                AppTrace.TraceSource.WriteNoise("Data.SortedDictionaryStateProvider.Contains", "{0}", this.ToString());
                //
                // Create read transaction. 
                //
                rotx = this.txManager.CreateTransaction();
                //
                // Acquire appropriate locks. 
                //
                this.AcquireReadLocks(btreeKey, rotx, Timeout.Infinite).Wait();
                //
                // Check to see if the value is in the btree store.
                //
                btreeValue = this.btreeStore.SeekAsync(btreeKey, CancellationToken.None).Result;
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SortedDictionaryStateProvider.Contains", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || e is TimeoutException || this.IsNonRetryableReplicationException(e) || e is KeyNotFoundException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));

                if (e is OutOfMemoryException)
                {
                    this.SetFaulted(FaultType.Transient);
                }
                //
                // Store original exception.
                //
                exp = e;
            }
            //
            // Release all locks.
            //
            this.txManager.RemoveTransaction(rotx);
            //
            // Retrow if necessary.
            //
            if (null != exp)
            {
                if (exp is KeyNotFoundException)
                {
                    return false;
                }
                else
                {
                    throw exp;
                }
            }
            //
            // If the check is conditional, if the value is found for the key and is the same, return true. 
            // Otherwise, if the key is found, return true.
            //
            return isConditional ? EqualityComparer<V>.Default.Equals(item.Value, (new BtreeValue<V, VBC>(btreeValue.Bytes).Value)) : (null != btreeValue);
        }

        /// <summary>
        /// Fires collection changed notifications for add/remove/update/clear that are part of the same transaction.
        /// </summary>
        /// <param name="rwtx">Transaction containing notifications.</param>
        void FireAllCollectionChangedNotifications(IReadWriteTransaction rwtx)
        {
            lock (this)
            {
                //
                // Fire notifications under a lock such that notifications from concurrent transactions are not interleaved.
                //
                foreach (var x in rwtx.Context)
                {
                    //
                    // Fire notification of collection changed.
                    //
                    this.FireCollectionChangedNotification((StateProviderOperation)x);
                }
            }
        }

        /// <summary>
        /// Fires collection changed notifications for add/remove/update/clear.
        /// </summary>
        /// <param name="operation">Operation that triggers the collection notification.</param>
        void FireCollectionChangedNotification(StateProviderOperation operation)
        {
            try
            {
                //
                // Apply operation. Switch processing based on operation type.
                //
                if (operation is SortedDictionaryStateProviderAddOperation<K, V>)
                {
                    SortedDictionaryStateProviderAddOperation<K, V> addOperation = (SortedDictionaryStateProviderAddOperation<K, V>)operation;
                    //
                    // Fire notification of collection changed (item added).
                    //
                    this.NotifyCollectionChanged(NotifyCollectionChangedAction.Add, new KeyValuePair<K, V>(addOperation.Key, addOperation.Value), null);
                }
                if (operation is SortedDictionaryStateProviderRemoveOperation<K, V>)
                {
                    SortedDictionaryStateProviderRemoveOperation<K, V> removeOperation = (SortedDictionaryStateProviderRemoveOperation<K, V>)operation;
                    //
                    // Fire notification of collection changed (item removed).
                    //
                    this.NotifyCollectionChanged(NotifyCollectionChangedAction.Remove, new KeyValuePair<K, V>(removeOperation.Key, removeOperation.Value), null);
                }
                if (operation is SortedDictionaryStateProviderClearOperation)
                {
                    SortedDictionaryStateProviderClearOperation clearOperation = (SortedDictionaryStateProviderClearOperation)operation;
                    //
                    // Fire notification on collection reset (all items removed).
                    //
                    this.NotifyCollectionChanged(NotifyCollectionChangedAction.Reset, null, null);
                }
                if (operation is SortedDictionaryStateProviderUpdateOperation<K, V>)
                {
                    SortedDictionaryStateProviderUpdateOperation<K, V> updateOperation = (SortedDictionaryStateProviderUpdateOperation<K, V>)operation;
                    //
                    // Fire notification on collection replace (item updated).
                    //
                    this.NotifyCollectionChanged(
                        NotifyCollectionChangedAction.Replace, 
                        new KeyValuePair<K, V>(updateOperation.Key, updateOperation.OldValue), 
                        new KeyValuePair<K, V>(updateOperation.Key, updateOperation.NewValue));
                }
            }
            catch (Exception e)
            {
                //
                // Ignore any errors thrown from the notification.
                //
                ReplicatedStateProvider.ProcessException(
                    "Data.SortedDictionaryStateProvider.FireCollectionChangedNotification",
                    e,
                    "{0}",
                    this.ToString());
            }
        }

        /// <summary>
        /// Constructs the file name for this state provider.
        /// </summary>
        /// <returns></returns>
        string ConstructPath()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(this.PartitionId.ToString("N"));
            sb.Append("_");
            sb.Append(this.ReplicaId);
            sb.Append("_");
            sb.Append(CRC64.ToCRC64String(Encoding.Unicode.GetBytes(this.ServiceInstanceName.ToString())));
            sb.Append(".lss");
            return Path.Combine(this.CodePackageActivationContext.WorkDirectory, sb.ToString());
        }

        /// <summary>
        /// Throws exception if the state provider is not readable.
        /// </summary>WriteError("Data.SortedDictionaryStateProvider
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNonRetryableException">The state provider is and will not become readable.</exception>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderNotReadableException">The state provider is not readable.</exception>
        protected override void ThrowIfNotReadable()
        {
            PartitionAccessStatus status = this.StatefulPartition.ReadStatus;
            ReplicatedStateProvider.Assert(PartitionAccessStatus.Invalid != status, "invalid partition status");
            if (PartitionAccessStatus.Granted != this.StatefulPartition.ReadStatus)
            {
                if (ReplicaRole.Primary == this.ReplicaRole || !this.isReadable)
                {
                    AppTrace.TraceSource.WriteWarning("Data.SortedDictionaryStateProvider.ThrowIfNotReadable", "{0}", this.ToString());

                    throw new StateProviderNotReadableException();
                }
            }
        }

        /// <summary>
        /// Throws exception if the state provider is faulted (transient or permanent).
        /// </summary>
        /// <exception cref="System.Fabric.Data.StateProvider.StateProviderFaultedException">The state provider is faulted.</exception>
        protected override void ThrowIfFaulted()
        {
            if (this.IsFaulted() || FaultType.Invalid != this.GetFault())
            {
                AppTrace.TraceSource.WriteWarning("Data.SortedDictionaryStateProvider.ThrowIfFaulted", "{0}", this.ToString());
                throw new StateProviderFaultedException();
            }
        }

        #endregion
    }
}
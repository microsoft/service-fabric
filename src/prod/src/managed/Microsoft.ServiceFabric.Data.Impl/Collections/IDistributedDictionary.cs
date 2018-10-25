// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Replicator;

    /// <summary>
    /// Replicated Dictionary interface.
    /// </summary>
    /// <typeparam name="TKey">The type of keys in the dictionary.</typeparam>
    /// <typeparam name="TValue">The type of values in the dictionary.</typeparam>
    internal interface IDistributedDictionary<TKey, TValue> : IDistributedCollection
    {
        /// <summary>
        /// Gets the number of key/value pairs contained in the dictionary.
        /// </summary>
        /// <remarks>
        /// This property does not have transactional semantics. 
        /// It represents the BEST EFFORT number of items in the dictionary at the moment when the property was accessed.
        /// </remarks>
        long Count { get; }

        /// <summary>
        /// Adds a key/value pair to the Distributed Dictionary if the key does not already exist, or updates a key/value pair if the key already exists.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to be added or whose value should be updated</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task<TValue> AddOrUpdateAsync(
            Transaction tx,
            TKey key,
            Func<TKey, TValue> addValueFactory,
            Func<TKey, TValue, TValue> updateValueFactory);

        /// <summary>
        /// Adds a key/value pair to the Distributed Dictionary if the key does not already exist, or updates a key/value pair if the key already exists.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to be added or whose value should be updated</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task<TValue> AddOrUpdateAsync(
            Transaction tx,
            TKey key,
            Func<TKey, TValue> addValueFactory,
            Func<TKey, TValue, TValue> updateValueFactory,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Adds a key/value pair to the DistributedCollection if they key does not already exist, or updates a key/value pair in the DistributedDictionary if the key already exists.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to be added or whose value should be updated</param>
        /// <param name="addValue">The value to be added for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <returns>
        /// The new value for the key. This will be either be addValue (if the key was absent) or the result of updateValueFactory (if the key was present).
        /// </returns>
        Task<TValue> AddOrUpdateAsync(Transaction tx, TKey key, TValue addValue, Func<TKey, TValue, TValue> updateValueFactory);

        /// <summary>
        /// Adds a key/value pair to the DistributedCollection if they key does not already exist, or updates a key/value pair in the DistributedDictionary if the key already exists.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to be added or whose value should be updated</param>
        /// <param name="addValue">The value to be added for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// The new value for the key. This will be either be addValue (if the key was absent) or the result of updateValueFactory (if the key was present).
        /// </returns>
        Task<TValue> AddOrUpdateAsync(
            Transaction tx, TKey key, TValue addValue, Func<TKey, TValue, TValue> updateValueFactory, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Removes all keys and values.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task ClearAsync();

        /// <summary>
        /// Removes all keys and values.
        /// </summary>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task ClearAsync(TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Determines whether the DistributedDictionary Contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to locate.s</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains a boolean that indicates whether the key exists.
        /// </returns>
        Task<bool> ContainsKeyAsync(Transaction tx, TKey key);

        /// <summary>
        /// Determines whether the DistributedDictionary Contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to locate.s</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains a boolean that indicates whether the key exists.
        /// </returns>
        Task<bool> ContainsKeyAsync(Transaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Determines whether the DistributedDictionary Contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to locate.s</param>
        /// <param name="lockMode">Lock to be used for the operation.</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains a boolean that indicates whether the key exists.
        /// </returns>
        Task<bool> ContainsKeyAsync(Transaction tx, TKey key, LockMode lockMode);

        /// <summary>
        /// Determines whether the DistributedDictionary Contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to locate.s</param>
        /// <param name="lockMode">Lock to be used for the operation.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains a boolean that indicates whether the key exists.
        /// </returns>
        Task<bool> ContainsKeyAsync(Transaction tx, TKey key, LockMode lockMode, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Creates an async enumerable over the DistributedDictionary.
        /// </summary>
        /// <param name="txn">Transaction object.</param>
        /// <returns>IAsyncEnumerable that represents all the key values.</returns>
        Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(
            ITransaction txn);

        /// <summary>
        /// Creates an async enumerable over the DistributedDictionary.
        /// </summary>
        /// <param name="txn">Transaction object.</param>
        /// <param name="enumerationMode">Enumeration mode to use.</param>
        /// <returns>IAsyncEnumerable that represents all the key values.</returns>
        Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(
            ITransaction txn,
            EnumerationMode enumerationMode);

        /// <summary>
        /// Creates an enumerable over the DistributedDictionary.
        /// </summary>
        /// <param name="txn">Transaction object.</param>
        /// <param name="filter">Key filter routine.</param>
        /// <param name="enumerationMode">Enumeration mode to use.</param>
        /// <returns>IAsyncEnumerable that represents all the key values.</returns>
        Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(
            ITransaction txn,
            Func<TKey, bool> filter,
            EnumerationMode enumerationMode);

        /// <summary>
        /// Adds a key/value pair to the DistributedDictionary if the key does not already exists.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains the value of the key. This will be either the existing value for the key if the key is already in the dictionary, or the new value for the key as returned by valueFactory if the key was not in the dictionary.
        /// </returns>
        Task<TValue> GetOrAddAsync(Transaction tx, TKey key, Func<TKey, TValue> valueFactory);

        /// <summary>
        /// Adds a key/value pair to the DistributedDictionary if the key does not already exists.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains the value of the key. This will be either the existing value for the key if the key is already in the dictionary, or the new value for the key as returned by valueFactory if the key was not in the dictionary.
        /// </returns>
        Task<TValue> GetOrAddAsync(Transaction tx, TKey key, Func<TKey, TValue> valueFactory, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Adds a key/value pair to the DistributedDictionary if the key does not already exist.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added, if the key does not already exist</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains the value of the key
        /// if the key is already in the dictionary, 
        /// or the new value if the key was not in the dictionary.
        /// </returns>
        Task<TValue> GetOrAddAsync(Transaction tx, TKey key, TValue value);

        /// <summary>
        /// Adds a key/value pair to the DistributedDictionary if the key does not already exist.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added, if the key does not already exist</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains the value of the key
        /// if the key is already in the dictionary, 
        /// or the new value if the key was not in the dictionary.
        /// </returns>
        Task<TValue> GetOrAddAsync(Transaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Add the specified key value pair.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a boolean indicating whether the
        /// the key/value pair was added.
        /// </returns>
        Task<bool> TryAddAsync(Transaction tx, TKey key, TValue value);

        /// <summary>
        /// Add the specified key value pair.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a boolean indicating whether the
        /// the key/value pair was added.
        /// </returns>
        Task<bool> TryAddAsync(Transaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Attempts to get the value associated with the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a ConditionalValue of TValue.
        /// ConditionalValue.HasValue indicates whether the get operation was successful.
        /// ConditionalValue.Value is set to the value if the get operation was successful.
        /// </returns>
        Task<ConditionalValue<TValue>> TryGetValueAsync(Transaction tx, TKey key);

        /// <summary>
        /// Attempts to get the value associated with the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a ConditionalValue of TValue.
        /// ConditionalValue.HasValue indicates whether the get operation was successful.
        /// ConditionalValue.Value is set to the value if the get operation was successful.
        /// </returns>
        Task<ConditionalValue<TValue>> TryGetValueAsync(Transaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Attempts to get the value associated with the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to get.</param>
        /// <param name="lockMode">Lock to be used for the get operation.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a ConditionalValue of TValue.
        /// ConditionalValue.HasValue indicates whether the get operation was successful.
        /// ConditionalValue.Value is set to the value if the get operation was successful.
        /// </returns>
        Task<ConditionalValue<TValue>> TryGetValueAsync(Transaction tx, TKey key, LockMode lockMode);

        /// <summary>
        /// Attempts to get the value associated with the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to get.</param>
        /// <param name="lockMode">Lock to be used for the get operation.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a ConditionalValue of TValue.
        /// ConditionalValue.HasValue indicates whether the get operation was successful.
        /// ConditionalValue.Value is set to the value if the get operation was successful.
        /// </returns>
        Task<ConditionalValue<TValue>> TryGetValueAsync(Transaction tx, TKey key, LockMode lockMode, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Attempts to remove and return the value with the specified key
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to remove.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The ConditionalValue.HasValue indicate whether the key/value pair was removed.
        /// The ConditionalValue.Value is the removed value.
        /// </returns>
        Task<ConditionalValue<TValue>> TryRemoveAsync(Transaction tx, TKey key);

        /// <summary>
        /// Attempts to remove and return the value with the specified key
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to remove.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The ConditionalValue.HasValue indicate whether the key/value pair was removed.
        /// The ConditionalValue.Value is the removed value.
        /// </returns>
        Task<ConditionalValue<TValue>> TryRemoveAsync(Transaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Compares the existing version for the specified key with a specified version,
        /// and if they are equal, updates the key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The value that replaces the value of the element with key.</param>
        /// <param name="newValue">
        /// The value that replaces the value of the element with key if the comparison
        /// results in equality.</param>
        /// <param name="comparisionValue">The value that is compared to the value of the element with key.</param>
        /// <returns>A task that represents the asynchronous operation.</returns>
        Task<bool> TryUpdateAsync(Transaction tx, TKey key, TValue newValue, TValue comparisionValue);

        /// <summary>
        /// Compares the existing version for the specified key with a specified version,
        /// and if they are equal, updates the key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The value that replaces the value of the element with key.</param>
        /// <param name="newValue">
        /// The value that replaces the value of the element with key if the comparison
        /// results in equality.</param>
        /// <param name="comparisionValue">The value that is compared to the value of the element with key.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>A task that represents the asynchronous operation.</returns>
        Task<bool> TryUpdateAsync(Transaction tx, TKey key, TValue newValue, TValue comparisionValue, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Add the specified key/value pair.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added.</param>
        /// <returns>A task that represents the asynchronous operation.</returns>
        Task AddAsync(Transaction tx, TKey key, TValue value);

        /// <summary>
        /// Add the specified key/value pair.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>A task that represents the asynchronous operation.</returns>
        Task AddAsync(Transaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Update the specified key/value pair.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to update.</param>
        /// <param name="value">The value to be added.</param>
        /// <returns>A task that represents the asynchronous operation.</returns>
        Task UpdateAsync(Transaction tx, TKey key, TValue value);

        /// <summary>
        /// Update the specified key/value pair.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to update.</param>
        /// <param name="value">The value to be added.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>A task that represents the asynchronous operation.</returns>
        Task UpdateAsync(Transaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken);
    }
}
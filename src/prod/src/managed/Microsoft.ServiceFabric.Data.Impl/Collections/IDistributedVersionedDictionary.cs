// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Store;
    using System.Threading;
    using System.Threading.Tasks;

    using Transaction = System.Fabric.Replication.Transaction;

    /// <summary>
    /// Versioned Dictionary
    /// </summary>
    /// <typeparam name="TKey">The type of keys in the dictionary.</typeparam>
    /// <typeparam name="TValue">The type of values in the dictionary.</typeparam>
    internal interface IDistributedVersionedDictionary<TKey, TValue> : IDistributedDictionary<TKey, TValue>
    {
        /// <summary>
        /// Gets all versions for a given key.
        /// </summary>
        /// <param name="tx">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <returns>True, all versions for the key, if it exists.</returns>
        Task<Tuple<bool, IEnumerable<Tuple<bool, IVersion, TValue>>>> TryGetVersionsAsync(Transaction tx, TKey key);

        /// <summary>
        /// Gets all versions for a given key.
        /// </summary>
        /// <param name="tx">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>True, all versions for the key, if it exists.</returns>
        Task<Tuple<bool, IEnumerable<Tuple<bool, IVersion, TValue>>>> TryGetVersionsAsync(Transaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Gets all versions for a given key.
        /// </summary>
        /// <param name="tx">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="since">Version bounding the versions returned.</param>
        /// <param name="until">Upper bound for the versions to be returned.</param>
        /// <returns>True, all versions for the key, if it exists.</returns>
        Task<Tuple<bool, IEnumerable<Tuple<bool, IVersion, TValue>>>> TryGetVersionsAsync(Transaction tx, TKey key, DateTime since, DateTime until);

        /// <summary>
        /// Gets all versions for a given key.
        /// </summary>
        /// <param name="tx">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="since">Version bounding the versions returned.</param>
        /// <param name="until">Upper bound for the versions to be returned.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>True, all versions for the key, if it exists.</returns>
        Task<Tuple<bool, IEnumerable<Tuple<bool, IVersion, TValue>>>> TryGetVersionsAsync(Transaction tx, TKey key, DateTime since, DateTime until, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Gets all versions for a given key.
        /// </summary>
        /// <param name="tx">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="since">Version bounding the versions returned.</param>
        /// <param name="until">Upper bound for the versions to be returned.</param>
        /// <returns>True, all versions for the key, if it exists.</returns>
        Task<Tuple<bool, IEnumerable<Tuple<bool, IVersion, TValue>>>> TryGetVersionsAsync(Transaction tx, TKey key, IVersion since, IVersion until);

        /// <summary>
        /// Gets all versions for a given key.
        /// </summary>
        /// <param name="tx">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="since">Version bounding the versions returned.</param>
        /// <param name="until">Upper bound for the versions to be returned.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>True, all versions for the key, if it exists.</returns>
        Task<Tuple<bool, IEnumerable<Tuple<bool, IVersion, TValue>>>> TryGetVersionsAsync(Transaction tx, TKey key, IVersion since, IVersion until, TimeSpan timeout, CancellationToken cancellationToken);
    }
}
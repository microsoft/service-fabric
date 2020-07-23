// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Store
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Provide simple contract over a persistent store with key value semantics
    /// </summary>
    /// <typeparam name="TKey"></typeparam>
    /// <typeparam name="TValue">Type of entity to be stored</typeparam>
    public interface IPersistentStore<TKey, TValue>
    {
        /// <summary>
        /// Get stored entity. Throws if not present
        /// </summary>
        /// <param name="key">key to identity the requested entity</param>
        /// <param name="token"></param>
        /// <returns>stored entity</returns>
        Task<TValue> GetEntityAsync(TKey key, CancellationToken token);

        /// <summary>
        /// StructuredStore a particular entity
        /// </summary>
        /// <param name="key">key to identity the entity</param>
        /// <param name="entity">entity to be stored</param>
        /// <param name="token"></param>
        Task SetEntityAsync(TKey key, TValue entity, CancellationToken token);

        /// <summary>
        /// StructuredStore a particular entity
        /// </summary>
        /// <param name="key">key to identity the entity</param>
        /// <param name="entity">entity to be stored</param>
        /// <param name="updateIfPresent"></param>
        /// <param name="token"></param>
        Task SetEntityAsync(TKey key, TValue entity, Func<TKey, TValue, TValue> updateIfPresent, CancellationToken token);

        /// <summary>
        /// Set a particular entity value if not already present
        /// </summary>
        /// <param name="key"></param>
        /// <param name="entity"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task SetIfNotPresentAsync(TKey key, TValue entity, CancellationToken token);

        /// <summary>
        /// Clear all items from the store.
        /// </summary>
        /// <param name="token"></param>
        /// <returns></returns>
        Task ClearAsync(CancellationToken token);

        /// <summary>
        /// Gets all values in store.
        /// </summary>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<IDictionary<TKey, TValue>> GetAllValuesAsync(CancellationToken token);

        /// <summary>
        /// Gets the values stored in store during a particular period.
        /// </summary>
        /// <param name="duration"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<IDictionary<TKey, TValue>> GetValuesInDurationAsync(Duration duration, CancellationToken token);

        /// <summary>
        /// Checks if a key is present
        /// </summary>
        /// <param name="key">Key to check</param>
        /// <param name="token"></param>
        /// <returns>True if present, otherwise false</returns>
        Task<bool> IsKeyPresentAsync(TKey key, CancellationToken token);

        /// <summary>
        /// Get the size of data stored in this store.
        /// </summary>
        /// <param name="token"></param>
        /// <returns></returns>
        /// <remarks>
        /// The returned value is the size of user data and doesn't account for small amount
        /// metadata that store implementation may/mayn't be using.
        /// </remarks>
        Task<long> GetStorageInUseAsync(CancellationToken token);

        /// <summary>
        /// Remove an entity if it is present.
        /// </summary>
        /// <param name="key"></param>
        /// <param name="token"></param>
        /// <returns>true if key was removed</returns>
        Task<bool> RemoveEntityIfPresentAsync(TKey key, CancellationToken token);
    }
}
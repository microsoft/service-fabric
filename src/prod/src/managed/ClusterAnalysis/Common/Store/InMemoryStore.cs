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
    /// An in memory Persistent store implementation. This is mainly used in unit tests.
    /// </summary>
    /// <remarks>
    /// In future, it can be moved to separate unit test project. This is not Thread Safe
    /// </remarks>
    public class InMemoryStore<TKey, TValue> : IPersistentStore<TKey, TValue>, IStoreProvider
    {
        private static IStoreProvider provider;

        private IDictionary<TKey, TValue> dictionaryStore = new Dictionary<TKey, TValue>();

        /// <summary>
        /// Create a reference of <see cref="InMemoryStore{Tkey,TValue}"/>
        /// </summary>
        /// <remarks>
        /// Private constructor to ensure that creation can be controlled.
        /// </remarks>
        private InMemoryStore()
        {
        }

        /// <summary>
        /// Get the provider Instance
        /// </summary>
        public static IStoreProvider StateProvider
        {
            get { return provider ?? (provider = new InMemoryStore<TKey, TValue>()); }
        }

        /// <summary>
        /// Get stored entity. Throws if not present
        /// </summary>
        /// <param name="key">key to identity the requested entity</param>
        /// <param name="token"></param>
        /// <returns>stored entity</returns>
        public Task<TValue> GetEntityAsync(TKey key, CancellationToken token)
        {
            return Task.FromResult(this.dictionaryStore[key]);
        }

        public Task ClearAsync(CancellationToken token)
        {
            this.dictionaryStore.Clear();
            return Task.FromResult(true);
        }

        /// <inheritdoc />
        public Task<IDictionary<TKey, TValue>> GetAllValuesAsync(CancellationToken token)
        {
            return Task.FromResult(this.dictionaryStore);
        }

        /// <inheritdoc />
        public Task<IDictionary<TKey, TValue>> GetValuesInDurationAsync(Duration duration, CancellationToken token)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Checks if a key is present
        /// </summary>
        /// <param name="key">Key to check</param>
        /// <param name="token"></param>
        /// <returns>True if present, otherwise false</returns>
        public Task<bool> IsKeyPresentAsync(TKey key, CancellationToken token)
        {
            return Task.FromResult(this.dictionaryStore.ContainsKey(key));
        }

        /// <inheritdoc />
        public Task<long> GetStorageInUseAsync(CancellationToken token)
        {
            throw new NotImplementedException();
        }

        /// <inheritdoc />
        public Task<bool> RemoveEntityIfPresentAsync(TKey key, CancellationToken token)
        {
            return Task.FromResult(this.dictionaryStore.Remove(key));
        }

        /// <summary>
        /// StructuredStore a particular entity
        /// </summary>
        /// <param name="key">key to identity the entity</param>
        /// <param name="entity">entity to be stored</param>
        /// <param name="token"></param>
        public async Task SetEntityAsync(TKey key, TValue entity, CancellationToken token)
        {
            await Task.FromResult(this.dictionaryStore[key] = entity);
        }

        /// <inheritdoc />
        public Task SetEntityAsync(TKey key, TValue entity, Func<TKey, TValue, TValue> updateIfPresent, CancellationToken token)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// StructuredStore multiples entities in single Transaction
        /// </summary>
        /// <param name="pairs">pairs to be persisted</param>
        /// <param name="token"></param>
        public async Task SetMultipleEntitiesAsync(IList<KeyValuePair<TKey, TValue>> pairs, CancellationToken token)
        {
            foreach (var singlePair in pairs)
            {
                await Task.FromResult(this.dictionaryStore[singlePair.Key] = singlePair.Value);
            }
        }

        /// <inheritdoc />
        public async Task SetIfNotPresentAsync(TKey key, TValue entity, CancellationToken token)
        {
            if (await this.IsKeyPresentAsync(key, token).ConfigureAwait(false))
            {
                return;
            }

            this.dictionaryStore.Add(new KeyValuePair<TKey, TValue>(key, entity));
        }

        /// <inheritdoc />
        public Task<IPersistentStore<Guid, string>> CreatePersistentStoreKeyGuidValueStringAsync(
            string storeIdentifier,
            DataRetentionPolicy retentionPolicy,
            CancellationToken token)
        {
            return Task.FromResult((IPersistentStore<Guid, string>)new InMemoryStore<Guid, string>());
        }

        public Task<IPersistentStore<Guid, DateTime>> CreatePersistentStoreKeyGuidValueDateTimeAsync(string storeIdentifier, DataRetentionPolicy retentionPolicy, CancellationToken token)
        {
            return Task.FromResult((IPersistentStore<Guid, DateTime>)new InMemoryStore<Guid, DateTime>());
        }

        /// <inheritdoc />
        public Task<IPersistentStore<string, string>> CreatePersistentStoreForStringsAsync(
            string storeIdentifier,
            DataRetentionPolicy retentionPolicy,
            CancellationToken token)
        {
            return Task.FromResult((IPersistentStore<string, string>)new InMemoryStore<string, string>());
        }

        /// <inheritdoc />
        public Task<IPersistentStore<string, DateTime>> CreatePersistentStoreForTimeAsync(
            string storeIdentifier,
            DataRetentionPolicy retentionPolicy,
            CancellationToken token)
        {
            return Task.FromResult((IPersistentStore<string, DateTime>)new InMemoryStore<string, DateTime>());
        }

        /// <inheritdoc />
        public Task<IPersistentStore<string, int>> CreatePersistentStoreForIntAsync(
            string storeIdentifier,
            DataRetentionPolicy retentionPolicy,
            CancellationToken token)
        {
            return Task.FromResult((IPersistentStore<string, int>)new InMemoryStore<string, int>());
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Store
{
    using System;
    using System.Collections.Concurrent;
    using System.Threading;
    using System.Threading.Tasks;
    using ClusterAnalysis.Common.Log;
    using Util;

    public sealed class TypedObjectStore<T> where T : IHasUniqueIdentity, IComparable<T>
    {
        private readonly IStoreProvider storeProvider;
        private readonly CancellationToken cancellationToken;
        private readonly ILogger logger;
        private IPersistentStore<Guid, string> underlyingPersistedStore;
        private string storeIdentifier;
        private DataRetentionPolicy dataRetentionPolicy;

        public TypedObjectStore(
            ILogger logger,
            IStoreProvider storeProvider,
            string uniqueStoreIdentifier,
            DataRetentionPolicy retentionPolicy,
            CancellationToken cancellationToken)
        {
            Assert.IsNotNull(logger, "logger != null");
            Assert.IsNotNull(storeProvider, "storeProvider != null");
            Assert.IsNotNull(retentionPolicy, "retentionPolicy != null");
            Assert.IsNotEmptyOrNull(uniqueStoreIdentifier, "uniqueStoreIdentifier != null/empty");

            this.storeProvider = storeProvider;
            this.dataRetentionPolicy = retentionPolicy;
            this.storeIdentifier = uniqueStoreIdentifier;
            this.cancellationToken = cancellationToken;
            this.logger = logger;
        }

        public async Task<T> GetOrAddTypedObjectAsync(Guid analysisKey, T objectToBeStored)
        {
            await this.InitializeStoreAsync().ConfigureAwait(false);

            if (!(await this.IsPresentAsync(analysisKey).ConfigureAwait(false)))
            {
                await this.PersistTypedObjectAsync(objectToBeStored).ConfigureAwait(false);
            }

            var metatadataAsString = await this.underlyingPersistedStore.GetEntityAsync(analysisKey, this.cancellationToken).ConfigureAwait(false);

            var context = HandyUtil.DeSerialize<T>(metatadataAsString);

            return context;
        }

        public async Task<T> GetTypedObjectAsync(Guid analysisKey)
        {
            await this.InitializeStoreAsync().ConfigureAwait(false);

            var metatadataAsString = await this.underlyingPersistedStore.GetEntityAsync(analysisKey, this.cancellationToken).ConfigureAwait(false);

            var context = HandyUtil.DeSerialize<T>(metatadataAsString);

            return context;
        }

        public async Task<ConcurrentDictionary<Guid, T>> GetCurrentSnapshotOfAllTypedObjectsAsync()
        {
            await this.InitializeStoreAsync().ConfigureAwait(false);

            var dictionary = new ConcurrentDictionary<Guid, T>();
            var inMemorySnap = await this.underlyingPersistedStore.GetAllValuesAsync(this.cancellationToken).ConfigureAwait(false);
            foreach (var entry in inMemorySnap)
            {
                var metadata = HandyUtil.DeSerialize<T>(entry.Value);

                dictionary.AddOrUpdate(entry.Key, metadata, (s, details) => metadata);
            }

            return dictionary;
        }

        public async Task<IMaxHeap<T>> GetCurrentSnapshot()
        {
            await this.InitializeStoreAsync().ConfigureAwait(false);

            var maxHeap = new MaxHeapImpl<T>();
            var inMemorySnap = await this.underlyingPersistedStore.GetAllValuesAsync(this.cancellationToken).ConfigureAwait(false);
            foreach (var entry in inMemorySnap)
            {
                var metadata = HandyUtil.DeSerialize<T>(entry.Value);

                maxHeap.Insert(metadata);
            }

            return maxHeap;
        }

        public async Task<bool> IsPresentAsync(Guid analysisKey)
        {
            await this.InitializeStoreAsync().ConfigureAwait(false);

            return await this.underlyingPersistedStore.IsKeyPresentAsync(analysisKey, this.cancellationToken).ConfigureAwait(false);
        }

        public async Task PersistTypedObjectAsync(T metadata)
        {
            await this.InitializeStoreAsync().ConfigureAwait(false);

            var serializedContext = HandyUtil.Serialize(metadata);

            await this.underlyingPersistedStore.SetEntityAsync(metadata.GetUniqueIdentity(), serializedContext, this.cancellationToken).ConfigureAwait(false);
        }

        public async Task DeletedTypedObjectAsync(T metadata)
        {
            await this.InitializeStoreAsync().ConfigureAwait(false);

            await this.underlyingPersistedStore.RemoveEntityIfPresentAsync(metadata.GetUniqueIdentity(), this.cancellationToken);
        }

        public async Task ClearAllAsync()
        {
            await this.InitializeStoreAsync().ConfigureAwait(false);
            await this.underlyingPersistedStore.ClearAsync(this.cancellationToken);
        }

        private async Task InitializeStoreAsync()
        {
            if (this.underlyingPersistedStore == null)
            {
                this.underlyingPersistedStore = await this.storeProvider.CreatePersistentStoreKeyGuidValueStringAsync(
                    this.storeIdentifier,
                    this.dataRetentionPolicy,
                    this.cancellationToken).ConfigureAwait(false);
            }
        }
    }
}
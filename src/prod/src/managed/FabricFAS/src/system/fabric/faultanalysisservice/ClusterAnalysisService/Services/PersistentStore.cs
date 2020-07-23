// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.ClusterAnalysis.Services
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using global::ClusterAnalysis.AnalysisCore.Helpers;
    using global::ClusterAnalysis.Common;
    using global::ClusterAnalysis.Common.Store;
    using global::ClusterAnalysis.Common.Util;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Collections;

    /// <summary>
    /// Implementation of persistent store using reliable collections
    /// </summary>
    [SuppressMessage("ReSharper", "AccessToDisposedClosure", Justification = "Reviewed. Suppression is OK here.")]
    public class PersistentStoreProvider : IStoreProvider
    {
        private static IStoreProvider provider;

        private IReliableStateManager stateManager;

        private IList<PersistentStore<string, int>> allStringIntStores;

        private IList<PersistentStore<Guid, int>> allGuidIntStores;

        private IList<PersistentStore<string, string>> allStringStringStores;

        private IList<PersistentStore<Guid, string>> allGuidStringStores;

        private IList<PersistentStore<string, DateTime>> allStringDateTimeStores;

        private IList<PersistentStore<Guid, DateTime>> allGuidDateTimeStores;

        private PersistentStoreProvider(IReliableStateManager manager)
        {
            this.stateManager = manager;
            this.allStringIntStores = new List<PersistentStore<string, int>>();
            this.allStringStringStores = new List<PersistentStore<string, string>>();
            this.allStringDateTimeStores = new List<PersistentStore<string, DateTime>>();

            this.allGuidIntStores = new List<PersistentStore<Guid, int>>();
            this.allGuidStringStores = new List<PersistentStore<Guid, string>>();
            this.allGuidDateTimeStores = new List<PersistentStore<Guid, DateTime>>();
        }

        /// <summary>
        /// Get the provider Instance
        /// </summary>
        public static IStoreProvider GetStateProvider(IReliableStateManager manager)
        {
            Assert.IsNotNull(manager);
            return provider ?? (provider = new PersistentStoreProvider(manager));
        }

        #region IStoreProvider_Abstractions

        public async Task<IPersistentStore<string, DateTime>> CreatePersistentStoreForTimeAsync(string storeIdentifier, DataRetentionPolicy retentionPolicy, CancellationToken token)
        {
            Assert.IsNotEmptyOrNull(storeIdentifier, "Identifier can't be null/empty");
            Assert.IsNotNull(retentionPolicy, "Retention Policy can't be null/empty");
            var store = new PersistentStore<string, DateTime>(this.stateManager, storeIdentifier, retentionPolicy);
            await store.InitializeAsync(token).ConfigureAwait(false);

            this.allStringDateTimeStores.Add(store);
            return store;
        }

        public async Task<IPersistentStore<string, int>> CreatePersistentStoreForIntAsync(string storeIdentifier, DataRetentionPolicy retentionPolicy, CancellationToken token)
        {
            Assert.IsNotEmptyOrNull(storeIdentifier, "Identifier can't be null/empty");
            Assert.IsNotNull(retentionPolicy, "Retention Policy can't be null/empty");
            var store = new PersistentStore<string, int>(this.stateManager, storeIdentifier, retentionPolicy);
            await store.InitializeAsync(token).ConfigureAwait(false);

            this.allStringIntStores.Add(store);
            return store;
        }

        /// <inheritdoc />
        public async Task<IPersistentStore<string, string>> CreatePersistentStoreForStringsAsync(string storeIdentifier, DataRetentionPolicy retentionPolicy, CancellationToken token)
        {
            Assert.IsNotEmptyOrNull(storeIdentifier, "Identifier can't be null/empty");
            Assert.IsNotNull(retentionPolicy, "Retention Policy can't be null/empty");
            var store = new PersistentStore<string, string>(this.stateManager, storeIdentifier, retentionPolicy);
            await store.InitializeAsync(token).ConfigureAwait(false);

            this.allStringStringStores.Add(store);
            return store;
        }

        public async Task<IPersistentStore<Guid, string>> CreatePersistentStoreKeyGuidValueStringAsync(string storeIdentifier, DataRetentionPolicy retentionPolicy, CancellationToken token)
        {
            Assert.IsNotEmptyOrNull(storeIdentifier, "Identifier can't be null/empty");
            Assert.IsNotNull(retentionPolicy, "Retention Policy can't be null/empty");
            var store = new PersistentStore<Guid, string>(this.stateManager, storeIdentifier, retentionPolicy);
            await store.InitializeAsync(token).ConfigureAwait(false);

            this.allGuidStringStores.Add(store);
            return store;
        }

        public async Task<IPersistentStore<Guid, DateTime>> CreatePersistentStoreKeyGuidValueDateTimeAsync(string storeIdentifier, DataRetentionPolicy retentionPolicy, CancellationToken token)
        {
            Assert.IsNotEmptyOrNull(storeIdentifier, "Identifier can't be null/empty");
            Assert.IsNotNull(retentionPolicy, "Retention Policy can't be null/empty");
            var store = new PersistentStore<Guid, DateTime>(this.stateManager, storeIdentifier, retentionPolicy);
            await store.InitializeAsync(token).ConfigureAwait(false);

            this.allGuidDateTimeStores.Add(store);
            return store;
        }
      
        public async Task<long> GetTotalStorageInUse(CancellationToken token)
        {
            long bytesInUse = 0;
            foreach (var oneStore in this.allStringIntStores)
            {
                bytesInUse += await oneStore.GetStorageInUseAsync(token).ConfigureAwait(false);
            }

            foreach (var oneStore in this.allStringStringStores)
            {
                bytesInUse += await oneStore.GetStorageInUseAsync(token).ConfigureAwait(false);
            }

            foreach (var oneStore in this.allStringDateTimeStores)
            {
                bytesInUse += await oneStore.GetStorageInUseAsync(token).ConfigureAwait(false);
            }

            foreach (var oneStore in this.allGuidIntStores)
            {
                bytesInUse += await oneStore.GetStorageInUseAsync(token).ConfigureAwait(false);
            }

            foreach (var oneStore in this.allGuidStringStores)
            {
                bytesInUse += await oneStore.GetStorageInUseAsync(token).ConfigureAwait(false);
            }

            foreach (var oneStore in this.allGuidDateTimeStores)
            {
                bytesInUse += await oneStore.GetStorageInUseAsync(token).ConfigureAwait(false);
            }

            return bytesInUse;
        }

        #endregion IStoreProvider_Abstractions
       
        private class PersistentStore<TKey, TValue> : IPersistentStore<TKey, TValue> where TKey : IComparable<TKey>, IEquatable<TKey>
        {
            private const string TotalStoredSizeInBytesKey = "SizeInByte";

            private const string MetadataStoreCollectionIdentifierSuffix = "MetadataMetricsCollection";

            private string storeIdentifier;

            private string metadataStoreIdentifier;

            private IReliableStateManager stateManager;

            private IReliableDictionary<TKey, StoredEntity<TValue>> defaultCollection;

            private IReliableDictionary<string, long> metadataMetricsCollection;

            private AgeBasedRetentionPolicy dataRetentionPolicy;

            public PersistentStore(IReliableStateManager manager, string identifier, DataRetentionPolicy retentionPolicy)
            {
                Assert.IsNotNull(manager, "State manager can't be null");
                Assert.IsNotEmptyOrNull(identifier);
                Assert.IsNotNull(retentionPolicy);
                Assert.IsTrue(retentionPolicy is AgeBasedRetentionPolicy, string.Format(CultureInfo.InvariantCulture, "TargetType '{0}' Retention Policy not supported", retentionPolicy.GetType()));
                this.stateManager = manager;
                this.storeIdentifier = identifier;
                this.dataRetentionPolicy = retentionPolicy as AgeBasedRetentionPolicy;
                this.metadataStoreIdentifier = identifier + MetadataStoreCollectionIdentifierSuffix;
            }

            /// <inheritdoc />
            public async Task<long> GetStorageInUseAsync(CancellationToken token)
            {
                using (var tx = this.stateManager.CreateTransaction())
                {
                    var attemptedGet =
                    await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() => this.metadataMetricsCollection.TryGetValueAsync(tx, TotalStoredSizeInBytesKey), token)
                                .ConfigureAwait(false);

                    return !attemptedGet.HasValue ? 0 : attemptedGet.Value;
                }
            }

            /// <inheritdoc />
            public async Task<TValue> GetEntityAsync(TKey key, CancellationToken token)
            {
                Assert.IsNotNull(key);

                using (var tx = this.stateManager.CreateTransaction())
                {
                    var attemptedGet =
                        await
                            FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() => this.defaultCollection.TryGetValueAsync(tx, key), token)
                                .ConfigureAwait(false);

                    if (!attemptedGet.HasValue)
                    {
                        throw new KeyNotFoundException(string.Format(CultureInfo.InvariantCulture, "Key '{0}' Not found in store", key));
                    }

                    return attemptedGet.Value.Entity;
                }
            }

            /// <inheritdoc />
            public async Task ClearAsync(CancellationToken token)
            {
                using (var tx = this.stateManager.CreateTransaction())
                {
                    await this.defaultCollection.ClearAsync().ConfigureAwait(false);
                    await this.metadataMetricsCollection.ClearAsync().ConfigureAwait(false);
                    await tx.CommitAsync().ConfigureAwait(false);
                }

                using (var tx = this.stateManager.CreateTransaction())
                {
                    await this.stateManager.RemoveAsync(tx, this.defaultCollection.Name);
                    await this.stateManager.RemoveAsync(tx, this.metadataStoreIdentifier);
                    await tx.CommitAsync().ConfigureAwait(false);
                }
            }

            /// <inheritdoc />
            public async Task<bool> IsKeyPresentAsync(TKey key, CancellationToken token)
            {
                Assert.IsNotNull(key);

                ConditionalValue<StoredEntity<TValue>> attemptedGet;
                using (var tx = this.stateManager.CreateTransaction())
                {
                    attemptedGet =
                        await
                            FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() => this.defaultCollection.TryGetValueAsync(tx, key), token)
                                .ConfigureAwait(false);

                    await tx.CommitAsync();
                }

                return attemptedGet.HasValue;
            }

            /// <inheritdoc />
            public async Task SetEntityAsync(TKey key, TValue entity, CancellationToken token)
            {
                Assert.IsNotNull(key);
                Assert.IsNotNull(object.Equals(entity, default(TValue)));

                await this.SetEntityAsync(
                    key,
                    entity,
                    (k, v) => entity,
                    token);
            }

            /// <inheritdoc />
            public async Task SetEntityAsync(TKey key, TValue entity, Func<TKey, TValue, TValue> updateIfPresent, CancellationToken token)
            {
                Assert.IsNotNull(key);
                Assert.IsNotNull(object.Equals(entity, default(TValue)));
                Assert.IsNotNull(updateIfPresent);

                var currentTime = DateTime.UtcNow;
                var wrappedValue = new StoredEntity<TValue>(currentTime) { LastTouchedTimeUtc = currentTime, Entity = entity };
                long? sizeDelta = null;

                // Set the entity value
                await this.SetEntityInternalAsync(
                    this.defaultCollection,
                    key,
                    wrappedValue,
                    (k, v) =>
                    {
                        // Grab Old size
                        var beforeUpdateSize = v.GetSize();

                        // Update the value
                        v.Entity = updateIfPresent(k, v.Entity);
                        v.LastTouchedTimeUtc = DateTime.UtcNow;

                        // Calculate Delta
                        sizeDelta = v.GetSize() - beforeUpdateSize;
                        return v;
                    },
                    token);

                using (var tx = this.stateManager.CreateTransaction())
                {
                    await
                        FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                            () => this.metadataMetricsCollection.AddOrUpdateAsync(tx, TotalStoredSizeInBytesKey, 0, (k, oldSize) => sizeDelta.HasValue ? oldSize + sizeDelta.Value : wrappedValue.GetSize() + Encoding.Unicode.GetByteCount(key.ToString())),
                            token).ConfigureAwait(false);

                    await tx.CommitAsync().ConfigureAwait(false);
                }
            }

            /// <summary>
            /// Add a key only if it is not already present
            /// </summary>
            /// <param name="key"></param>
            /// <param name="entity"></param>
            /// <param name="token"></param>
            /// <returns></returns>
            public async Task SetIfNotPresentAsync(TKey key, TValue entity, CancellationToken token)
            {
                bool isPresent = await this.IsKeyPresentAsync(key, token);
                if (!isPresent)
                {
                    await this.SetEntityAsync(key, entity, token);
                }
            }

            public async Task<bool> RemoveEntityIfPresentAsync(TKey key, CancellationToken token)
            {
                Assert.IsNotNull(key);
                ConditionalValue<StoredEntity<TValue>> tryRemove;
                using (var tx = this.stateManager.CreateTransaction())
                {
                    tryRemove =
                        await
                            FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() => this.defaultCollection.TryRemoveAsync(tx, key), token)
                                .ConfigureAwait(false);

                    await tx.CommitAsync();
                }

                return tryRemove.HasValue;
            }

            /// <inheritdoc />
            public Task<IDictionary<TKey, TValue>> GetAllValuesAsync(CancellationToken token)
            {
                return this.GetValuesInDurationAsync(new Duration(DateTime.MinValue, DateTime.MaxValue), token);
            }

            /// <inheritdoc />
            public async Task<IDictionary<TKey, TValue>> GetValuesInDurationAsync(Duration duration, CancellationToken token)
            {
                Assert.IsNotNull(duration);
                using (var tx = this.stateManager.CreateTransaction())
                {
                    var enumerable =
                        await
                            FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() => this.defaultCollection.CreateEnumerableAsync(tx), token)
                                .ConfigureAwait(false);

                    Assert.IsNotNull(enumerable, "Enumerable is Null. Unexpected");

                    var map = new Dictionary<TKey, TValue>();
                    var enumerator = enumerable.GetAsyncEnumerator();
                    while (await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() => enumerator.MoveNextAsync(token), token).ConfigureAwait(false))
                    {
                        var time = enumerator.Current.Value.CreateTimeUtc;

                        if (time >= duration.StartTime && time < duration.EndTime)
                        {
                            map.Add(enumerator.Current.Key, enumerator.Current.Value.Entity);
                        }
                    }

                    await tx.CommitAsync().ConfigureAwait(false);

                    return map;
                }
            }

            // TODO: Better Manage Scrub Task
            public async Task InitializeAsync(CancellationToken token)
            {
                this.defaultCollection =
                    await
                        FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                            () => this.stateManager.GetOrAddAsync<IReliableDictionary<TKey, StoredEntity<TValue>>>(this.storeIdentifier),
                            CancellationToken.None).ConfigureAwait(false);

                this.metadataMetricsCollection =
                    await
                        FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                            () => this.stateManager.GetOrAddAsync<IReliableDictionary<string, long>>(this.metadataStoreIdentifier),
                            token).ConfigureAwait(false);

                if (!this.dataRetentionPolicy.Equals(AgeBasedRetentionPolicy.Forever))
                {
                    var scrubTask =
                        Task.Run(() => this.RunScrubRegularlyAsync(token), token).ContinueWith(
                            task =>
                            {
                                if (task.Status != TaskStatus.Faulted)
                                {
                                    return;
                                }

                                if (task.Exception.InnerException is OperationCanceledException && token.IsCancellationRequested)
                                {
                                    return;
                                }

                                if (task.Exception.InnerException is FabricObjectClosedException || task.Exception.InnerException is FabricNotPrimaryException)
                                {
                                    return;
                                }

                                var data = HandyUtil.ExtractInformationFromException(task.Exception);
                                Environment.FailFast(data, task.Exception);
                            });
                }
            }

            #region Private_Abstractions

            private async Task SetEntityInternalAsync<TU>(
                IReliableDictionary<TKey, TU> dictionary,
                TKey key,
                TU entity,
                Func<TKey, TU, TU> updateIfExisting,
                CancellationToken token)
            {
                // Handling FabricNotPrimaryException and re-throwing it as Operation cancelled. This is really to 
                // to get around the platform gap where the store and the runtime sends their own set of notification
                // to user when current replica is no longer primary. The store sends this by throwing this particular
                // exception for any write and runtime will signal the cancellation token. In case of both, I need to behave the same
                // i.e. don't do any further writes and just stop asap. Easiest way I can find this is by failing all writes
                // with operation cancelled exception. The app is already handling this exception and treating it as if cancellation token
                // has been signalled.
                // That said, not the most ideal and I've an email thread with Runtime folks.
                using (var tx = this.stateManager.CreateTransaction())
                {
                    await
                        FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                            () => dictionary.AddOrUpdateAsync(tx, key, entity, updateIfExisting),
                            token).ConfigureAwait(false);

                    await tx.CommitAsync().ConfigureAwait(false);
                }
            }

            private async Task RunScrubRegularlyAsync(CancellationToken token)
            {
                while (!token.IsCancellationRequested)
                {
                    await this.ScrubRecordsAsync(token).ConfigureAwait(false);
                    await Task.Delay(this.dataRetentionPolicy.GapBetweenScrubs, token).ConfigureAwait(false);
                }
            }

            private async Task ScrubRecordsAsync(CancellationToken token)
            {
                var keysToBeRemoved = new List<TKey>();
                long sizeReduction = 0;
                using (var tx = this.stateManager.CreateTransaction())
                {
                    var enumerable =
                        await
                            FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() => this.defaultCollection.CreateEnumerableAsync(tx), token)
                                .ConfigureAwait(false);

                    var enumerator = enumerable.GetAsyncEnumerator();
                    while (await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() => enumerator.MoveNextAsync(token), token).ConfigureAwait(false))
                    {
                        token.ThrowIfCancellationRequested();

                        if (DateTime.UtcNow - enumerator.Current.Value.LastTouchedTimeUtc <= this.dataRetentionPolicy.MaxAgeOfRecord)
                        {
                            continue;
                        }

                        keysToBeRemoved.Add(enumerator.Current.Key);
                        sizeReduction += Encoding.Unicode.GetByteCount(enumerator.Current.Key.ToString()) + enumerator.Current.Value.GetSize();
                    }

                    foreach (var key in keysToBeRemoved)
                    {
                        token.ThrowIfCancellationRequested();

                        // Best effort cleanup. If it fails in this scrub, it will likely get picked up in next one :)
                        var closureKeyCopy = key;
                        await
                            FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() => this.defaultCollection.TryRemoveAsync(tx, closureKeyCopy), token)
                                .ConfigureAwait(false);
                    }

                    await tx.CommitAsync().ConfigureAwait(false);
                }

                using (var tx = this.stateManager.CreateTransaction())
                {
                    await
                        FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                            () => this.metadataMetricsCollection.AddOrUpdateAsync(tx, TotalStoredSizeInBytesKey, 0, (k, v) => v - sizeReduction),
                            token).ConfigureAwait(false);

                    await tx.CommitAsync().ConfigureAwait(false);
                }
            }

            #endregion Private_Abstractions

#pragma warning disable 693
            [DataContract]
            private class StoredEntity<T>
#pragma warning restore 693
            {
                /// <summary>
                /// This is a good approximation of the Datetime size. Not accurate, but good enough.
                /// </summary>
                private const ushort DatetimeSize = 8;

                public StoredEntity(DateTime createTime)
                {
                    this.CreateTimeUtc = createTime;
                    this.LastTouchedTimeUtc = createTime;
                }

                [DataMember(IsRequired = true)]
                public DateTime CreateTimeUtc { get; set; }

                [DataMember(IsRequired = true)]
                public DateTime LastTouchedTimeUtc { get; set; }

                [DataMember(IsRequired = true)]
                public T Entity { get; set; }

                public long GetSize()
                {
                    // The constant part is the metadata size.
                    if (typeof(T) == typeof(int))
                    {
                        return (2 * DatetimeSize) + sizeof(int);
                    }

                    if (typeof(T) == typeof(string))
                    {
                        return (2 * DatetimeSize) + Encoding.Unicode.GetByteCount((string)(object)this.Entity);
                    }

                    if (typeof(T) == typeof(DateTime))
                    {
                        return (2 * DatetimeSize) + DatetimeSize;
                    }

                    if (typeof(T) == typeof(long))
                    {
                        return (2 * DatetimeSize) + sizeof(long);
                    }

                    throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "TargetType '{0}' is not supported", typeof(T)));
                }

                /// <inheritdoc />
                public override string ToString()
                {
                    return string.Format(CultureInfo.InvariantCulture, "CreateTime: '{0}', LastTouchedTime: '{1}', Entity: '{2}'", this.CreateTimeUtc, this.LastTouchedTimeUtc, this.Entity);
                }
            }
        }
    }
}
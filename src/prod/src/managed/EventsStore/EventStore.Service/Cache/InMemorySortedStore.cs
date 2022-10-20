// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.Cache
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// In memory store
    /// </summary>
    /// <typeparam name="TKey"></typeparam>
    /// <typeparam name="TValue"></typeparam>
    internal class InMemorySortedStore<TKey, TValue> : ISortedStore<TKey, TValue> where TKey : IComparable, ITimeComparable where TValue : IComparable
    {
        private SortedList<TKey, TValue> underlyingStore;

        public InMemorySortedStore()
        {
            this.underlyingStore = new SortedList<TKey, TValue>(new Comparer());
        }

        /// <inheritdoc />
        public Task<TKey> GetEarliestKeyAsync()
        {
            return Task.FromResult(this.underlyingStore.Keys.FirstOrDefault());
        }

        /// <inheritdoc />
        public Task<TKey> GetLatestKeyAsync()
        {
            return Task.FromResult(this.underlyingStore.Keys.LastOrDefault());
        }

        /// <inheritdoc />
        public Task<bool> ContainsKeyAsync(TKey key, CancellationToken token)
        {
            return Task.FromResult(this.underlyingStore.ContainsKey(key));
        }

        /// <inheritdoc />
        public Task<int> GetItemCountAsync(CancellationToken token)
        {
            return Task.FromResult(this.underlyingStore.Count);
        }

        /// <inheritdoc />
        public Task<TValue> GetValueAsync(TKey key, CancellationToken token)
        {
            if (this.underlyingStore.ContainsKey(key))
            {
                return Task.FromResult(this.underlyingStore[key]);
            }

            throw new KeyNotFoundException("Key Not found" + key);
        }

        /// <inheritdoc />
        public Task<IEnumerable<TValue>> GetValuesInRangeAsync(DateTimeOffset startTime, DateTimeOffset endTime, CancellationToken token)
        {
            IList<TValue> collection = new List<TValue>();
            foreach (var oneKvp in this.underlyingStore)
            {
                if (oneKvp.Key.CompareTimeRangeOrder(startTime, endTime) == 0)
                {
                    collection.Add(oneKvp.Value);
                }
            }

            return Task.FromResult((IEnumerable<TValue>)collection);
        }

        /// <inheritdoc />
        public Task<bool> RemoveAsync(TKey key, CancellationToken token)
        {
            return Task.FromResult(this.underlyingStore.Remove(key));
        }

        /// <inheritdoc />
        public Task<int> RemoveFirstXKeysAsync(int keyCountToRemove, CancellationToken token)
        {
            var itemsRemovedCount = 0;
            var keys = this.underlyingStore.Keys;

            var keysToRemove = new List<TKey>();

            // Sorted list gives .Keys as a sorted list.
            foreach (var oneKey in this.underlyingStore.Keys)
            {
                if (itemsRemovedCount++ < keyCountToRemove)
                {
                    keysToRemove.Add(oneKey);
                    continue;
                }

                break;
            }

            foreach (var oneKey in keysToRemove)
            {
                this.underlyingStore.Remove(oneKey);
            }

            return Task.FromResult(keyCountToRemove);
        }

        /// <inheritdoc />
        public Task<int> RemoveOldKeysAsync(DateTimeOffset earliestAllowedTime, CancellationToken token)
        {
            var keysToRemove = new List<TKey>();
            foreach (var oneKey in this.underlyingStore.Keys)
            {
                if (oneKey.CompareTimeOrder(earliestAllowedTime) < 0)
                {
                    keysToRemove.Add(oneKey);
                    continue;
                }

                break;
            }

            var itemsRemovedCount = 0;
            foreach (var oneKey in keysToRemove)
            {
                itemsRemovedCount++;

                this.underlyingStore.Remove(oneKey);
            }

            return Task.FromResult(itemsRemovedCount);
        }

        /// <inheritdoc />
        public Task<int> RemoveSelectedItemsAsync(Func<TKey, TValue, bool> condition, CancellationToken token)
        {
            Assert.IsNotNull(condition, "condition != null");

            var keysToRemove = new List<TKey>();
            foreach (var oneKeyValue in this.underlyingStore)
            {
                if (condition(oneKeyValue.Key, oneKeyValue.Value))
                {
                    keysToRemove.Add(oneKeyValue.Key);
                    continue;
                }
            }

            var itemsRemovedCount = 0;
            foreach (var oneKey in keysToRemove)
            {
                itemsRemovedCount++;

                this.underlyingStore.Remove(oneKey);
            }

            return Task.FromResult(itemsRemovedCount);
        }

        /// <inheritdoc />
        public Task StoreAsync(IDictionary<TKey, TValue> valueMap, CancellationToken token)
        {
            IList<string> str = new List<string>();
            foreach (var oneKeyValue in valueMap)
            {
                if (this.underlyingStore.ContainsKey(oneKeyValue.Key))
                {
                    continue;
                }

                this.underlyingStore.Add(oneKeyValue.Key, oneKeyValue.Value);
            }

            return Task.FromResult(true);
        }

        /// <inheritdoc />
        public Task StoreAsync(TKey key, TValue value, CancellationToken token)
        {
            if (this.underlyingStore.ContainsKey(key))
            {
                return Task.FromResult(false);
            }

            this.underlyingStore.Add(key, value);

            return Task.FromResult(true);
        }

        public Task<bool> ClearAsync(CancellationToken token)
        {
            this.underlyingStore.Clear();
            return Task.FromResult(true);
        }

        /// <inheritdoc />
        private class Comparer : IComparer<TKey>
        {
            public int Compare(TKey x, TKey y)
            {
                return x.CompareTo(y);
            }
        }
    }
}
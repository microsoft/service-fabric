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
    using EventStore.Service.LogProvider;

    internal class Cache<TKey, TValue> where TKey : ICacheKey, ITimeComparable where TValue : ICacheValue
    {
        private const int CacheBuildIncrementMultiplierThreshold = 100;

        private ISortedStore<TKey, TValue> underlyingStore;

        private CachePolicy cachePolicy;

        private string cacheName;

        private SemaphoreSlim cacheSingleAccess;

        public Cache(string name, ISortedStore<TKey, TValue> store, DataFetcher<TKey, TValue> fetcher, CachePolicy policy)
        {
            Assert.IsNotNull(fetcher, "fetcher != null");
            Assert.IsNotNull(policy, "policy != null");
            Assert.IsNotNull(store, "store != null");
            Assert.IsNotEmptyOrNull(name, "Cache Name != null");
            this.cacheName = name;
            this.underlyingStore = store;
            this.DataFetcher = fetcher;
            this.cachePolicy = policy;
            this.StartTime = this.EndTime = DateTimeOffset.MinValue;
            this.cacheSingleAccess = new SemaphoreSlim(1);
        }

        /// <summary>
        /// Friendly Name of the Cache
        /// </summary>
        public string Name { get { return this.cacheName; } }

        /// <summary>
        /// Cache includes data upto this time.
        /// </summary>
        public DateTimeOffset EndTime { get; private set; }

        /// <summary>
        /// Cache includes data from this time onward.
        /// </summary>
        public DateTimeOffset StartTime { get; private set; }

        /// <summary>
        /// Fetches data for updating the cache.
        /// </summary>
        public DataFetcher<TKey, TValue> DataFetcher { get; private set; }

        /// <summary>
        /// Get the Number of Items in the cache.
        /// </summary>
        /// <param name="token">Cancellation token</param>
        public Task<int> GetItemCountAsync(CancellationToken token)
        {
            return this.underlyingStore.GetItemCountAsync(token);
        }

        /// <summary>
        /// Check if a specified time range data is cached in the cache.
        /// </summary>
        /// <param name="startTime"></param>
        /// <param name="endTime"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        public Task<bool> IsTimeRangePresentInCacheAsync(DateTimeOffset startTime, DateTimeOffset endTime, CancellationToken token)
        {
            if (startTime >= this.StartTime && endTime <= this.EndTime)
            {
                return Task.FromResult(true);
            }

            return Task.FromResult(false);
        }

        public async Task InitHydrateAsync(CancellationToken token)
        {
            var finalHydrateEndTime = DateTimeOffset.UtcNow;
            var finalHydrateStartTime = finalHydrateEndTime - this.cachePolicy.MaxCacheMaintainDuration;

            var iterationHydrateEndTime = finalHydrateEndTime;
            var iterationHydrateStartTime = finalHydrateEndTime - this.cachePolicy.CacheBuildIncrement;
            if (iterationHydrateStartTime < finalHydrateStartTime)
            {
                iterationHydrateStartTime = finalHydrateStartTime;
            }

            // Update the store inside the lock.
            await this.cacheSingleAccess.WaitAsync(token).ConfigureAwait(false);

            int itemsFetched = 0;
            var countOfCalls = 0;

            try
            {
                var incrementMultiplier = 1;
                while (iterationHydrateStartTime >= finalHydrateStartTime && itemsFetched < this.cachePolicy.MaxCacheItemCount)
                {
                    countOfCalls++;

                    var data = await this.DataFetcher.FetchDataAsync(iterationHydrateStartTime, iterationHydrateEndTime, token).ConfigureAwait(false);

                    // If the number of values are below a certain threshold, the next time we use larger interval for the query. If count is above threshold,
                    // we reset the multiplier.
                    var dataCount = data.Count;
                    if (dataCount < CacheBuildIncrementMultiplierThreshold)
                    {
                        incrementMultiplier++;
                    }
                    else
                    {
                        incrementMultiplier = 1;
                    }

                    if (dataCount > 0)
                    {
                        await this.AddDataUnlockedAsync(data, token).ConfigureAwait(false);
                        itemsFetched += dataCount;
                    }

                    if (iterationHydrateStartTime == finalHydrateStartTime)
                    {
                        break;
                    }

                    iterationHydrateEndTime = iterationHydrateStartTime;
                    iterationHydrateStartTime = iterationHydrateEndTime - TimeSpan.FromTicks(this.cachePolicy.CacheBuildIncrement.Ticks * incrementMultiplier);
                    if (iterationHydrateStartTime < finalHydrateStartTime)
                    {
                        iterationHydrateStartTime = finalHydrateStartTime;
                    }
                }

                if (this.StartTime == DateTimeOffset.MinValue)
                {
                    this.StartTime = iterationHydrateStartTime;
                }

                this.EndTime = finalHydrateEndTime;
            }
            finally
            {
                this.cacheSingleAccess.Release();
            }

            EventStoreLogger.Logger.LogMessage(
                "InitHydrateAsync: Cache: {0} Init hydration took : {1}ms across: {2} calls. Total: {3} Items Fetched",
                this.Name,
                (DateTimeOffset.UtcNow - finalHydrateEndTime).TotalMilliseconds,
                countOfCalls,
                itemsFetched);
        }

        /// <summary>
        /// Trigger an Update to the cache
        /// </summary>
        /// <param name="token"></param>
        /// <returns></returns>
        public async Task UpdateAsync(CancellationToken token)
        {
            // Fetch the data outside lock.
            var queryStartTime = this.EndTime;
            if (queryStartTime == DateTimeOffset.MinValue)
            {
                queryStartTime = DateTime.UtcNow - this.cachePolicy.MaxCacheMaintainDuration;
            }

            var currentTime = DateTimeOffset.UtcNow;

            // We always go back 5 minutes from start time. This is to account for delay in data showing up in Azure tables.
            // We couldn't use TimeStamp field in Azure as I see pretty inconsistent behavior with this field - that is - I see this field getting updated
            // even on read queries. So, we need to use the time the event was written for filtering and therefore the need to account for this 5 minute delay.
            var data = await this.DataFetcher.FetchDataAsync(queryStartTime - TimeSpan.FromMinutes(5), currentTime, token).ConfigureAwait(false);

            var queryTimeTaken = DateTimeOffset.UtcNow - currentTime;
            if (queryTimeTaken > TimeSpan.FromSeconds(10))
            {
                EventStoreLogger.Logger.LogMessage("UpdateAsync:: Cache: {0}, Fetched: {1} entries which took: {2}ms", this.cacheName, data.Count, queryTimeTaken.TotalMilliseconds);
            }

            // Update the store inside the lock.
            await this.cacheSingleAccess.WaitAsync(token).ConfigureAwait(false);

            try
            {
                // Remove keys that are older than max duration we maintain. If some items are removed, we update the cache start/end time.
                var itemsRemoved = await this.underlyingStore.RemoveOldKeysAsync(DateTimeOffset.UtcNow - this.cachePolicy.MaxCacheMaintainDuration, token).ConfigureAwait(false);
                if (itemsRemoved > 0)
                {
                    await UpdateCacheStartTimeAsync(token).ConfigureAwait(false);
                    EventStoreLogger.Logger.LogMessage("UpdateAsync:: Cache: {0}, Items Removed : {1}. New StartTime: {2}", this.cacheName, itemsRemoved, this.StartTime);
                }

                // Now add the new Data and update the cache start/end time.
                await this.AddDataUnlockedAsync(data, token).ConfigureAwait(false);

                this.EndTime = currentTime;
            }
            finally
            {
                this.cacheSingleAccess.Release();
            }
        }

        public async Task<IEnumerable<TValue>> GetCacheItemsAsync(DateTimeOffset startTime, DateTimeOffset endTime, CancellationToken token)
        {
            await this.cacheSingleAccess.WaitAsync(token).ConfigureAwait(false);

            try
            {
                var values = await this.underlyingStore.GetValuesInRangeAsync(startTime, endTime, token).ConfigureAwait(false);
                return values;
            }
            finally
            {
                this.cacheSingleAccess.Release();
            }
        }

        public async Task<bool> ClearCacheAsync(CancellationToken token)
        {
            await this.cacheSingleAccess.WaitAsync(token).ConfigureAwait(false);

            try
            {
                EventStoreLogger.Logger.LogMessage("Cache: {0} cleared", this.Name);
                return await this.underlyingStore.ClearAsync(token).ConfigureAwait(false);
            }
            finally
            {
                this.cacheSingleAccess.Release();
            }
        }

        /// <summary>
        /// Used for Debugging.
        /// </summary>
        /// <param name="fileName"></param>
        internal void Dump(string fileName)
        {
            var values = this.underlyingStore.GetValuesInRangeAsync(DateTimeOffset.MinValue, DateTimeOffset.MaxValue, CancellationToken.None).GetAwaiter().GetResult();
            System.IO.File.WriteAllLines(fileName, values.Select(item => item.ToString()).ToArray());
        }

        private async Task AddDataUnlockedAsync(IDictionary<TKey, TValue> newData, CancellationToken token)
        {
            await this.underlyingStore.StoreAsync(newData, token).ConfigureAwait(false);

            // it is more efficient to add the data first and then remove the extra ones.
            var overItems = await this.underlyingStore.GetItemCountAsync(token).ConfigureAwait(false) - this.cachePolicy.MaxCacheItemCount;
            if (overItems > 0)
            {
                var keysRemoved = await this.underlyingStore.RemoveFirstXKeysAsync(overItems, token).ConfigureAwait(false);
                EventStoreLogger.Logger.LogMessage(
                    "AddDataAsync:: Cache: {0}, {1} count of Keys removed. Cache Items Count: {2}",
                    this.cacheName,
                    keysRemoved,
                    await this.underlyingStore.GetItemCountAsync(token).ConfigureAwait(false));

                await this.UpdateCacheStartTimeAsync(token);
            }
        }

        private async Task UpdateCacheStartTimeAsync(CancellationToken token)
        {
            // Please note that we are intentionally setting the starttime as the endtime for the first key.
            // Think of this way. Let's say you did a query within range X-Y and got 100 results. All the keys in
            // this range are marked with X and Y as start/end time. During one of the scrubbing, 70 of these were deleted.
            // So, we can't just set the cache start time to X. So instead, what we have done is that we set it to the time the first
            // Key was originally written.
            var earliestKey = await this.underlyingStore.GetEarliestKeyAsync().ConfigureAwait(false);
            if (earliestKey != null)
            {
                this.StartTime = earliestKey.DataFetchEndTime < earliestKey.OriginalCreateTime ? earliestKey.DataFetchEndTime : earliestKey.OriginalCreateTime;
            }
        }
    }
}
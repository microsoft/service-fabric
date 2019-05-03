// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.Cache
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common.Tracing;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using ClusterAnalysis.Common.Log;
    using EventStore.Service.LogProvider;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader;

    internal class AzureTableCachedStorageAccess : IAzureTableStorageAccess
    {
        private static string CacheHydrateOps = "CacheHydrateOps";

        private static string CacheUpdateOps = "CacheUpdateOps";

        private static TimeSpan DelayInFreshnessOfResult = TimeSpan.FromMinutes(1);

        private static TimeSpan DurationBetweenCacheUpdate = TimeSpan.FromSeconds(30);

        private static TimeSpan DurationBetweenRetries = TimeSpan.FromSeconds(5);

        private static TimeSpan DurationBetweenCacheTelemetryDump = TimeSpan.FromMinutes(30);

        private static int OperationFailedRetryCount = 2;

        private static string[] SepcialTableList = { "DeploymentIds" };

        private DateTimeOffset LastCacheTelemetryDump;

        private CancellationTokenSource cancellationTokenSourceForBackgroundUpdater;

        private Cache<AzureTableCacheKey, AzureTableCacheValue> applicationCache;

        private IAzureTableStorageAccess azureTableDirectAccess;

        private IList<Cache<AzureTableCacheKey, AzureTableCacheValue>> cacheList;

        private CachePolicy cachingPolicy;

        private Cache<AzureTableCacheKey, AzureTableCacheValue> clusterCache;

        private Cache<AzureTableCacheKey, AzureTableCacheValue> correlationCache;

        private ILogger logger;

        private Cache<AzureTableCacheKey, AzureTableCacheValue> nodeCache;

        private Cache<AzureTableCacheKey, AzureTableCacheValue> partitionCache;

        private Cache<AzureTableCacheKey, AzureTableCacheValue> replicaCache;

        private Cache<AzureTableCacheKey, AzureTableCacheValue> serviceCache;

        private IDictionary<string, bool> tableExistsMap;

        public AzureTableCachedStorageAccess(CachePolicy policy, string name, string key, bool useHttps, string tableNamePrefix, string deploymentId)
        {
            Assert.IsNotNull(policy, "policy");

            this.logger = EventStoreLogger.Logger;

            this.cachingPolicy = policy;

            this.tableExistsMap = new Dictionary<string, bool>();

            this.LastCacheTelemetryDump = DateTimeOffset.UtcNow;

            this.cancellationTokenSourceForBackgroundUpdater = null;

            this.azureTableDirectAccess = new AccountAndKeyTableStorageAccess(name, key, useHttps);

            this.nodeCache = new Cache<AzureTableCacheKey, AzureTableCacheValue>(
                "NodeCache",
                new InMemorySortedStore<AzureTableCacheKey, AzureTableCacheValue>(),
                new DataFetcher<AzureTableCacheKey, AzureTableCacheValue>(Mapping.NodeTable, tableNamePrefix, deploymentId, this.cachingPolicy.MaxCacheItemCount, this.azureTableDirectAccess),
                this.cachingPolicy);

            this.partitionCache = new Cache<AzureTableCacheKey, AzureTableCacheValue>(
                "PartitionCache",
                new InMemorySortedStore<AzureTableCacheKey, AzureTableCacheValue>(),
                new DataFetcher<AzureTableCacheKey, AzureTableCacheValue>(Mapping.PartitionTable, tableNamePrefix, deploymentId, this.cachingPolicy.MaxCacheItemCount, this.azureTableDirectAccess),
                this.cachingPolicy);

            this.applicationCache = new Cache<AzureTableCacheKey, AzureTableCacheValue>(
                "ApplicationCache",
                new InMemorySortedStore<AzureTableCacheKey, AzureTableCacheValue>(),
                new DataFetcher<AzureTableCacheKey, AzureTableCacheValue>(Mapping.AppTable, tableNamePrefix, deploymentId, this.cachingPolicy.MaxCacheItemCount, this.azureTableDirectAccess),
                this.cachingPolicy);

            this.serviceCache = new Cache<AzureTableCacheKey, AzureTableCacheValue>(
                "ServiceCache",
                new InMemorySortedStore<AzureTableCacheKey, AzureTableCacheValue>(),
                new DataFetcher<AzureTableCacheKey, AzureTableCacheValue>(Mapping.ServiceTable, tableNamePrefix, deploymentId, this.cachingPolicy.MaxCacheItemCount, this.azureTableDirectAccess),
                this.cachingPolicy);

            this.replicaCache = new Cache<AzureTableCacheKey, AzureTableCacheValue>(
                "ReplicaCache",
                new InMemorySortedStore<AzureTableCacheKey, AzureTableCacheValue>(),
                new DataFetcher<AzureTableCacheKey, AzureTableCacheValue>(Mapping.ReplicaTable, tableNamePrefix, deploymentId, this.cachingPolicy.MaxCacheItemCount, this.azureTableDirectAccess),
                this.cachingPolicy);

            this.clusterCache = new Cache<AzureTableCacheKey, AzureTableCacheValue>(
                "ClusterCache",
                new InMemorySortedStore<AzureTableCacheKey, AzureTableCacheValue>(),
                new DataFetcher<AzureTableCacheKey, AzureTableCacheValue>(Mapping.ClusterTable, tableNamePrefix, deploymentId, this.cachingPolicy.MaxCacheItemCount, this.azureTableDirectAccess),
                this.cachingPolicy);

            this.correlationCache = new Cache<AzureTableCacheKey, AzureTableCacheValue>(
                "CorrelationCache",
                new InMemorySortedStore<AzureTableCacheKey, AzureTableCacheValue>(),
                new DataFetcher<AzureTableCacheKey, AzureTableCacheValue>(Mapping.CorrelationTable, tableNamePrefix, deploymentId, this.cachingPolicy.MaxCacheItemCount, this.azureTableDirectAccess),
                this.cachingPolicy);

            this.cacheList = new List<Cache<AzureTableCacheKey, AzureTableCacheValue>>();
            this.cacheList.Add(this.nodeCache);
            this.cacheList.Add(this.partitionCache);
            this.cacheList.Add(this.applicationCache);
            this.cacheList.Add(this.serviceCache);
            this.cacheList.Add(this.replicaCache);
            this.cacheList.Add(this.clusterCache);
            this.cacheList.Add(this.correlationCache);
        }

        public async Task<bool> DoesTableExistsAsync(string tableName, CancellationToken token)
        {
            if (this.tableExistsMap.ContainsKey(tableName))
            {
                return this.tableExistsMap[tableName];
            }

            this.logger.LogMessage("CacheMiss: DoesTableExistsAsync - TableName: {0}", tableName);
            var tableExists = await this.azureTableDirectAccess.DoesTableExistsAsync(tableName, token).ConfigureAwait(false);
            this.tableExistsMap[tableName] = tableExists;

            return tableExists;
        }

        public Task<AzureTableQueryResult> ExecuteQueryAsync(string tableName, CancellationToken token)
        {
            return this.azureTableDirectAccess.ExecuteQueryAsync(tableName, token);
        }

        public Task<AzureTableQueryResult> ExecuteQueryAsync(string tableName, IFilterBuilder filterBuilder, string propertyFilter, Func<AzureTablePropertyBag, bool> clientSideFilter, DateTimeOffset startTime, DateTimeOffset endTime, CancellationToken token)
        {
            return this.ExecuteQueryAsync(tableName, filterBuilder, propertyFilter, clientSideFilter, startTime, endTime, -1, token);
        }

        ///  <inheritdoc />
        ///  <remarks>
        ///  The cache is updated every X seconds. So, by design, it is behind by latest time by at least X seconds. Even though X is pretty low,
        ///  use can query this interval repeatedly causing cache misses. Therefore we have decided that the data the API will return will always be delayed by
        ///  X seconds. So, for instance, if use queries till 'CurrentTime', we return results upto 'CurrentTime - X' to ensure that we have a cache hit.
        ///  </remarks>
        public async Task<AzureTableQueryResult> ExecuteQueryAsync(string tableName, IFilterBuilder filterBuilder, string propertyFilter, Func<AzureTablePropertyBag, bool> clientSideFilter, DateTimeOffset startTime, DateTimeOffset endTime, int takeCount, CancellationToken token)
        {
            var cache = this.GetCacheForThisTable(tableName);
            Assert.IsNotNull(cache, string.Format("Couldn't find a cache matching table {0}", tableName));

            if (endTime - startTime < DelayInFreshnessOfResult)
            {
                return new AzureTableQueryResult(new List<AzureTablePropertyBag>());
            }

            // When we look at the cache hit, we account for delay that cache introduces.
            if (!await cache.IsTimeRangePresentInCacheAsync(startTime, endTime - DelayInFreshnessOfResult, token).ConfigureAwait(false))
            {
                // Used for Debugging during Cache Miss scenario.
                //var dumpFileName = DateTimeOffset.UtcNow.UtcDateTime.Ticks.ToString();
                //Dump(cache, dumpFileName);

                FabricEvents.Events.EventStoreCacheMiss(cache.Name, cache.StartTime.UtcDateTime, cache.EndTime.UtcDateTime, startTime.UtcDateTime, endTime.UtcDateTime);

                var fetchedValues = await cache.DataFetcher.FetchDataAsync(new AzureTimeLoggedBasedFilterBuilder(), propertyFilter, clientSideFilter, startTime, endTime, takeCount, token).ConfigureAwait(false);

                // If Client side filter is not null, apply it.
                var allResults = fetchedValues.Select(item => item.Value.AzureTablePropertyBag);
                if (clientSideFilter != null)
                {
                    allResults = allResults.Where(item => clientSideFilter(item));
                }

                return new AzureTableQueryResult(allResults);
            }

            var values = await cache.GetCacheItemsAsync(startTime, endTime, token).ConfigureAwait(false);
            if (clientSideFilter != null)
            {
                values = values.Where(item => clientSideFilter(item.AzureTablePropertyBag));
            }

            if (takeCount != -1 && values.Count() > takeCount)
            {
                EventStoreLogger.Logger.LogMessage("Take Count is : {0} and there are : {1} Entries", takeCount, values.Count());

                // Values are stored in sorted order by time. If we need to return a subset, we would want to return the latest ones.
                values = values.Skip(Math.Max(0, values.Count()) - takeCount);
            }

            return new AzureTableQueryResult(values.Select(item => item.AzureTablePropertyBag));
        }

        public Task KickOffUpdaterAsync(CancellationToken token)
        {
            this.cancellationTokenSourceForBackgroundUpdater = CancellationTokenSource.CreateLinkedTokenSource(token);

            return TaskRunner.RunAsync(
                "CacheUpdateTask", () => this.UpdateRegularlyAsync(cancellationTokenSourceForBackgroundUpdater.Token),
                null,
                this.cancellationTokenSourceForBackgroundUpdater.Token);
        }

        public async Task ReleaseAccessAsync(CancellationToken token)
        {
            // Signal cancellation to the backup thread.
            if (this.cancellationTokenSourceForBackgroundUpdater != null)
            {
                EventStoreLogger.Logger.LogMessage("ReleaseAccessAsync:: Signalling cancellation to the background updater");
                this.cancellationTokenSourceForBackgroundUpdater.Cancel();
                this.cancellationTokenSourceForBackgroundUpdater = null;
            }

            foreach (var oneCache in this.cacheList)
            {
                await oneCache.ClearCacheAsync(token).ConfigureAwait(false);
            }
        }

        // For Debugging
        internal static void Dump(Cache<AzureTableCacheKey, AzureTableCacheValue> cache, string fileName)
        {
            cache.Dump(cache.Name + fileName);
        }

        private async Task ExecuteCacheOpWithRetryAsync(string cacheName, string operationName, Func<Task> func, CancellationToken token)
        {
            if (func == null)
            {
                throw new ArgumentNullException("func");
            }

            int retryCount = OperationFailedRetryCount;
            bool isRetry = false;
            while (true)
            {
                try
                {
                    if (isRetry)
                    {
                        await Task.Delay(DurationBetweenRetries, token).ConfigureAwait(false);
                        isRetry = false;
                    }

                    await func().ConfigureAwait(false);

                    break;
                }
                catch (Exception exp)
                {
                    this.logger.LogWarning("Operation: {0}, on cache: {1}, encountered Exception : {2}", operationName, cacheName, exp);
                    if (retryCount-- <= 0)
                    {
                        this.logger.LogWarning("Giving up on operation: {0} on cache: {1}", operationName, cacheName);
                        throw;
                    }

                    isRetry = true;
                }
            }
        }

        private Cache<AzureTableCacheKey, AzureTableCacheValue> GetCacheForThisTable(string tableName)
        {
            if (tableName.EndsWith(Mapping.NodeTable))
            {
                return this.nodeCache;
            }

            if (tableName.EndsWith(Mapping.PartitionTable))
            {
                return this.partitionCache;
            }

            if (tableName.EndsWith(Mapping.AppTable))
            {
                return this.applicationCache;
            }

            if (tableName.EndsWith(Mapping.ServiceTable))
            {
                return this.serviceCache;
            }

            if (tableName.EndsWith(Mapping.ReplicaTable))
            {
                return this.replicaCache;
            }

            if (tableName.EndsWith(Mapping.ClusterTable))
            {
                return this.clusterCache;
            }

            if (tableName.EndsWith(Mapping.CorrelationTable))
            {
                return this.correlationCache;
            }

            return null;
        }

        private async Task UpdateAsync(CancellationToken token)
        {
            foreach (var oneCache in this.cacheList)
            {
                await this.ExecuteCacheOpWithRetryAsync(oneCache.Name, CacheUpdateOps, () => oneCache.UpdateAsync(token), token);
            }
        }

        private async Task UpdateRegularlyAsync(CancellationToken token)
        {
            try
            {
                // Hydrate the Caches.
                foreach (var oneCache in this.cacheList)
                {
                    EventStoreLogger.Logger.LogMessage("Hydrating Cache: {0}", oneCache.Name);
                    await this.ExecuteCacheOpWithRetryAsync(oneCache.Name, CacheHydrateOps, () => oneCache.InitHydrateAsync(token), token);
                }

                // Kick off regular update of the caches.
                while (!token.IsCancellationRequested)
                {
                    var updateStartTime = DateTimeOffset.UtcNow;

                    await UpdateAsync(token).ConfigureAwait(false);

                    var updateTimeTaken = DateTimeOffset.UtcNow - updateStartTime;

                    if (updateTimeTaken < DurationBetweenCacheUpdate)
                    {
                        await Task.Delay(DurationBetweenCacheUpdate - updateTimeTaken, token).ConfigureAwait(false);
                    }

                    await this.WriteTelemetryAsync(token).ConfigureAwait(false);
                }
            }
            catch (OperationCanceledException oce)
            {
                this.logger.LogMessage("UpdateRegularlyAsync:: Operation Cancellation. Exp '{0}'. Shutting Down", oce);
            }
            catch (Exception exp)
            {
                this.logger.LogWarning("UpdateRegularlyAsync:: Encountered Exception '{0}'", exp);
                throw;
            }
        }

        private async Task WriteTelemetryAsync(CancellationToken token)
        {
            if (DateTimeOffset.UtcNow - this.LastCacheTelemetryDump < DurationBetweenCacheTelemetryDump)
            {
                return;
            }

            try
            {
                var process = Process.GetCurrentProcess();
                int memorySizeInMb = 0;
                using (var perfCounter = new PerformanceCounter())
                {
                    perfCounter.CategoryName = "Process";
                    perfCounter.CounterName = "Working Set - Private";
                    perfCounter.InstanceName = process.ProcessName;
                    memorySizeInMb = Convert.ToInt32(perfCounter.NextValue()) / (int)(1024 *1024);
                    perfCounter.Close();
                }

                FabricEvents.Events.EventStorePerfTelemetry(memorySizeInMb);
            }
            catch
            {
            }

            foreach (var oneCache in this.cacheList)
            {
                var cacheCount = await oneCache.GetItemCountAsync(token).ConfigureAwait(false);
                var durationInHours = (oneCache.EndTime - oneCache.StartTime).TotalHours;
                FabricEvents.Events.EventStoreCacheTelemetry(oneCache.Name, cacheCount, durationInHours);
            }

            this.LastCacheTelemetryDump = DateTimeOffset.UtcNow;
        }
    }
}
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
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader;

    internal sealed class DataFetcher<TKey, TValue> where TKey : ICacheKey where TValue : ICacheValue
    {
        private const int MaxTakeCount = 1000;

        private string tableName;

        private IAzureTableStorageAccess azureTableAccess;

        private string decoratedTableName;

        private string namePrefix;

        private string deploymentId;

        private IFilterBuilder filterBuilder;

        private int takeCount;

        public DataFetcher(string tableName, string namePrefix, string deploymentId, int maxSingleTakeCount, IAzureTableStorageAccess azureTableAccess)
        {
            Assert.IsNotEmptyOrNull(tableName, "tableName != null");
            Assert.IsNotNull(azureTableAccess, "azureTableAccess != null");
            this.tableName = tableName;
            this.filterBuilder = new AzureTimeLoggedBasedFilterBuilder();
            this.azureTableAccess = azureTableAccess;
            this.namePrefix = namePrefix;
            this.deploymentId = deploymentId;
            this.decoratedTableName = null;
            this.takeCount = maxSingleTakeCount;
        }

        public Task<IDictionary<TKey, TValue>> FetchDataAsync(DateTimeOffset startTime, DateTimeOffset endTime, CancellationToken token)
        {
            return this.FetchDataAsync(startTime, endTime, this.takeCount, token);
        }

        public Task<IDictionary<TKey, TValue>> FetchDataAsync(IFilterBuilder filterBuilder, DateTimeOffset startTime, DateTimeOffset endTime, CancellationToken token)
        {
            return this.FetchDataInternalAsync(filterBuilder, string.Empty, null, startTime, endTime, this.takeCount, token);
        }

        public Task<IDictionary<TKey, TValue>> FetchDataAsync(DateTimeOffset startTime, DateTimeOffset endTime, int takeCount, CancellationToken token)
        {
            return this.FetchDataInternalAsync(this.filterBuilder, string.Empty, null, startTime, endTime, takeCount, token);
        }

        public Task<IDictionary<TKey, TValue>> FetchDataAsync(IFilterBuilder filterBuilder, DateTimeOffset startTime, DateTimeOffset endTime, int takeCount, CancellationToken token)
        {
            return this.FetchDataInternalAsync(filterBuilder, string.Empty, null, startTime, endTime, takeCount, token);
        }

        public Task<IDictionary<TKey, TValue>> FetchDataAsync(IFilterBuilder filterBuilder, string propertyFilter, Func<AzureTablePropertyBag, bool> clientSideFilter, DateTimeOffset startTime, DateTimeOffset endTime, int takeCount, CancellationToken token)
        {
            return this.FetchDataInternalAsync(filterBuilder, propertyFilter, clientSideFilter, startTime, endTime, takeCount, token);
        }

        private async Task<IDictionary<TKey, TValue>> FetchDataInternalAsync(IFilterBuilder receivedFilterBuilder, string propertyFilter, Func<AzureTablePropertyBag, bool> clientSideFilter, DateTimeOffset startTime, DateTimeOffset endTime, int takeCount, CancellationToken token)
        {
            if (this.decoratedTableName == null)
            {
                var decorator = new ServiceFabricAzureAccessDataDecorator(this.azureTableAccess, this.namePrefix, this.deploymentId);
                this.decoratedTableName = await decorator.DecorateTableNameAsync(this.tableName, token).ConfigureAwait(false);
            }

            if (takeCount == -1)
            {
                takeCount = MaxTakeCount;
            }

            var azureQueryResult = await this.azureTableAccess.ExecuteQueryAsync(this.decoratedTableName, receivedFilterBuilder, propertyFilter, clientSideFilter, startTime, endTime, takeCount, token).ConfigureAwait(false);

            return azureQueryResult.AzureTableProperties.ToDictionary(
                oneRow => (TKey)(ICacheKey)new AzureTableCacheKey(oneRow.PartitionKey, oneRow.RowKey, startTime, endTime, oneRow.TimeLogged, oneRow.TimeStamp),
                oneRow => (TValue)(ICacheValue)new AzureTableCacheValue(oneRow));
        }
    }
}
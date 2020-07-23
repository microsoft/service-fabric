// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Auth;
    using Microsoft.WindowsAzure.Storage.Table;

    public class AccountAndKeyTableStorageAccess : IAzureTableStorageAccess
    {
        /// <summary>
        /// The CloudTableClient instance to query on.
        /// </summary>
        private CloudTableClient client;

        /// <summary>
        /// Initializes a new instance of the <see cref="AccountAndKeyTableStorageAccess" /> class.
        /// <param name="name">Azure storage account name.</param>
        /// <param name="key">Azure storage account key.</param>
        /// <param name="useHttps">Whether or not to use HTTPS.</param>
        /// <param name="endpointSuffix">Endpoint suffix responsible for identifying public versus national cloud endpoints (core.windows.net).</param>
        /// </summary>
        public AccountAndKeyTableStorageAccess(string name, string key, bool useHttps, string endpointSuffix = null)
        {
            if (string.IsNullOrWhiteSpace(name))
            {
                throw new ArgumentNullException("name");
            }

            if (string.IsNullOrWhiteSpace(key))
            {
                throw new ArgumentNullException("key");
            }

            var credentials = new StorageCredentials(name, key);
            var account = endpointSuffix == null
                ? new CloudStorageAccount(credentials, useHttps)
                : new CloudStorageAccount(credentials, endpointSuffix, useHttps);
            this.client = account.CreateCloudTableClient();
        }

        /// <inheritdoc />
        public Task<bool> DoesTableExistsAsync(string tableName, CancellationToken token)
        {
            var table = this.client.GetTableReference(tableName);
            return table.ExistsAsync();
        }

        public Task<AzureTableQueryResult> ExecuteQueryAsync(string tableName, CancellationToken token)
        {
            return this.ExecuteQueryAsyncInternal(tableName, string.Empty, null, -1, token);
        }

        public Task<AzureTableQueryResult> ExecuteQueryAsync(string tableName, IFilterBuilder filterBuilder, string propertyFilter, Func<AzureTablePropertyBag, bool> clientSideFilter, DateTimeOffset startTime, DateTimeOffset endTime, CancellationToken token)
        {
            if (filterBuilder == null)
            {
                throw new ArgumentNullException("filterBuilder");
            }

            return this.ExecuteQueryAsyncInternal(tableName, filterBuilder.BuildFilter(propertyFilter, startTime, endTime), clientSideFilter, -1, token);
        }

        public Task<AzureTableQueryResult> ExecuteQueryAsync(string tableName, IFilterBuilder filterBuilder, string propertyFilter, Func<AzureTablePropertyBag, bool> clientSideFilter, DateTimeOffset startTime, DateTimeOffset endTime, int count, CancellationToken token)
        {
            if (filterBuilder == null)
            {
                throw new ArgumentNullException("filterBuilder");
            }

            return this.ExecuteQueryAsyncInternal(tableName, filterBuilder.BuildFilter(propertyFilter, startTime, endTime), clientSideFilter, count, token);
        }

        /// <inheritdoc />
        private async Task<AzureTableQueryResult> ExecuteQueryAsyncInternal(string tableName, string queryFilter, Func<AzureTablePropertyBag, bool> clientSideFilter, int count, CancellationToken token)
        {
            var table = this.client.GetTableReference(tableName);
            var query = string.IsNullOrEmpty(queryFilter) ? new TableQuery<DynamicTableEntity>() : (new TableQuery<DynamicTableEntity>()).Where(queryFilter);
            if (count > 0)
            {
                query = query.Take(count);
            }

            IEnumerable<AzureTablePropertyBag> propertyBags = new List<AzureTablePropertyBag>();

            try
            {
                var azureResults = await table.ExecuteTableQueryAsync(query, token).ConfigureAwait(false);

                propertyBags = azureResults.Select(
                item => new AzureTablePropertyBag(
                    GetTimeFromRowKey(item.RowKey, item.Properties),
                    item.Timestamp,
                    item.PartitionKey,
                    item.RowKey,
                    item.Properties.ToDictionary(
                        kvp => kvp.Key,
                        kvp => kvp.Value.PropertyAsObject.ToString())));

                if (clientSideFilter != null)
                {
                    propertyBags = propertyBags.Where(item => clientSideFilter(item));
                }

                return new AzureTableQueryResult(propertyBags);
            }
            catch (StorageException exp)
            {
                // For some reason, extended error info is null. Temp Fix.
                var errorCode = exp.RequestInformation?.HttpStatusMessage;
                if (!string.IsNullOrEmpty(errorCode))
                {
                    if (errorCode == "Not Found")
                    {
                        return new AzureTableQueryResult(propertyBags); ;
                    }
                }

                throw;
            }
        }

        /// <summary>
        /// Gets the DateTime (UTC) from the RowKey of the event record.
        /// </summary>
        /// <param name="rowKey">The RowKey of the record.</param>
        /// <param name="properties"></param>
        /// <returns>The DateTime represented by the RowKey.</returns>
        private static DateTimeOffset GetTimeFromRowKey(string rowKey, IDictionary<string, EntityProperty> properties)
        {
            if (string.IsNullOrWhiteSpace(rowKey))
            {
                return DateTimeOffset.MinValue;
            }

            // Figure out whether this record has timestamps in chronological order or reverse chronological order
            EntityProperty dcaVersionProperty = properties?[WellKnownFields.DcaVersion];
            bool reverseChronologicalOrder = dcaVersionProperty == null || dcaVersionProperty.Int32Value < 0;
            string[] parts = rowKey.Split('_');
            if (parts.Length == 5)
            {
                long ticks;
                if (long.TryParse(parts[0], out ticks))
                {
                    ticks = reverseChronologicalOrder ? (DateTime.MaxValue.Ticks - ticks) : ticks;
                    if (ticks >= DateTime.MinValue.Ticks && ticks <= DateTime.MaxValue.Ticks)
                    {
                        return new DateTimeOffset(ticks, TimeSpan.Zero);
                    }
                }
            }

            return new DateTime(0, DateTimeKind.Utc);
        }

        public Task ReleaseAccessAsync(CancellationToken token)
        {
            return Task.FromResult(true);
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Extensions for Azure Table query methods.
    /// </summary>
    internal static class AzureTableExtensions
    {
        public static async Task<IEnumerable<DynamicTableEntity>> ExecuteTableQueryAsync(
            this CloudTable table,
            TableQuery<DynamicTableEntity> tableQuery,
            CancellationToken token)
        {
            var nextQuery = tableQuery;
            var takeCount = tableQuery.TakeCount;
            var tableContinuationToken = default(TableContinuationToken);
            var results = new List<DynamicTableEntity>();

            do
            {
                TableQuerySegment<DynamicTableEntity> queryResult;

                try
                {
                    //Execute the next query segment async. Remove the token from call to get around the error in .Net core (Figure it out).
                    queryResult = await table.ExecuteQuerySegmentedAsync<DynamicTableEntity>(nextQuery, tableContinuationToken).ConfigureAwait(false);
                }
                catch (StorageException exp)
                {
                    // For some reason, extended error info is null. Temp Fix.
                    var errorCode = exp.RequestInformation?.HttpStatusMessage;
                    if (!string.IsNullOrEmpty(errorCode))
                    {
                        if (errorCode == "Not Found")
                        {
                            return results;
                        }
                    }

                    throw;
                }

                //Add current set of results to our list.
                results.AddRange(queryResult.Results);

                tableContinuationToken = queryResult.ContinuationToken;

                //Continuation token is not null, it implies there are more results to be queried
                if (tableContinuationToken != null && takeCount.HasValue)
                {
                    //Query has a take count, calculate the remaining number of items to load.
                    var itemsToLoad = takeCount.Value - results.Count;

                    //If more items to load, update query take count, or else set next query to null.
                    if (itemsToLoad > 0)
                    {
                        nextQuery.TakeCount = itemsToLoad;
                    }
                    else
                    {
                        nextQuery = null;
                    }
                }
            } while (tableContinuationToken != null && nextQuery != null && !token.IsCancellationRequested);

            return results;
        }
    }
}

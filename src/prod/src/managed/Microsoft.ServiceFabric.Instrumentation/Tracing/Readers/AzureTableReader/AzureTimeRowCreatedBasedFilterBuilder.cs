// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader
{
    using System;
    using Microsoft.WindowsAzure.Storage.Table;

    public class AzureTimeRowCreatedBasedFilterBuilder : IFilterBuilder
    {
        /// <summary>
        /// DCA-specific RowKey that could be excluded from query results.
        /// </summary>
        public const string StatusEntityRowKey = "FFFFFFFFFFFFFFFFFFFF";

        public string BuildFilter(string propertyFilter, DateTimeOffset startTime, DateTimeOffset endTime)
        {
            // Always exclude entities with RowKey='FFFFFFFFFFFFFFFFFFFF'
            string finalFilter = TableQuery.GenerateFilterCondition(WellKnownFields.RowKey, QueryComparisons.NotEqual, StatusEntityRowKey);

            if (!string.IsNullOrWhiteSpace(propertyFilter))
            {
                finalFilter = TableQuery.CombineFilters(finalFilter, TableOperators.And, propertyFilter);
            }

            string timeFilter = null;
            if (startTime != DateTimeOffset.MinValue)
            {
                timeFilter = CreateTimestampFilter(startTime, true);
            }

            if (endTime != DateTimeOffset.MaxValue)
            {
                var endFilter = CreateTimestampFilter(endTime, false);
                timeFilter = (timeFilter == null) ? endFilter : TableQuery.CombineFilters(timeFilter, TableOperators.And, endFilter);
            }

            if (!string.IsNullOrWhiteSpace(timeFilter))
            {
                finalFilter = TableQuery.CombineFilters(finalFilter, TableOperators.And, timeFilter);
            }

            return finalFilter;
        }

        private static string CreateTimestampFilter(DateTimeOffset timestamp, bool isStartTime)
        {
            return TableQuery.GenerateFilterConditionForDate(
                WellKnownFields.TimeStamp,
                isStartTime ? QueryComparisons.GreaterThanOrEqual : QueryComparisons.LessThan,
                timestamp);
        }
    }
}
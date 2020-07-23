// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader
{
    using System;
    using System.Globalization;
    using Microsoft.WindowsAzure.Storage.Table;

    public class AzureTimeLoggedBasedFilterBuilder : IFilterBuilder
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
            // Filter for records created by V3 and older code, where row keys were in chronological order
            string timestampTicks = (timestamp.Ticks + (isStartTime ? 0 : 1)).ToString("D20", CultureInfo.InvariantCulture);
            var timestampCondition = TableQuery.GenerateFilterCondition(
                WellKnownFields.RowKey,
                isStartTime ? QueryComparisons.GreaterThanOrEqual : QueryComparisons.LessThanOrEqual,
                timestampTicks);
            var dcaVersionCondition = TableQuery.GenerateFilterConditionForInt(
                WellKnownFields.DcaVersion,
                QueryComparisons.GreaterThanOrEqual,
                0);
            string version3StyleFilter = TableQuery.CombineFilters(timestampCondition, TableOperators.And, dcaVersionCondition);

            // Filter for records created by the current code, where row keys are in reverse chronological order
            timestampTicks = (DateTime.MaxValue.Ticks - timestamp.Ticks + (isStartTime ? 1 : 0)).ToString("D20", CultureInfo.InvariantCulture);
            timestampCondition = TableQuery.GenerateFilterCondition(
                WellKnownFields.RowKey,
                isStartTime ? QueryComparisons.LessThanOrEqual : QueryComparisons.GreaterThanOrEqual,
                timestampTicks);
            dcaVersionCondition = TableQuery.GenerateFilterConditionForInt(WellKnownFields.DcaVersion, QueryComparisons.LessThan, 0);

            string filter = TableQuery.CombineFilters(timestampCondition, TableOperators.And, dcaVersionCondition);

            return TableQuery.CombineFilters(version3StyleFilter, TableOperators.Or, filter);
        }
    }
}
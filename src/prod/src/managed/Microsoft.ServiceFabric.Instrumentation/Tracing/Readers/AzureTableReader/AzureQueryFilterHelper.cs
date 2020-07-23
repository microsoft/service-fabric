// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader;
using Microsoft.WindowsAzure.Storage.Table;

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader
{
    internal static class AzureTableQueryHelper
    {
        private const int MaxNumberOfRecordsToReturn = 500;

        private const int MaxNumberOfRecordsToQuery = 5000;

        private const int MaxFilterCount = 12;

        public static async Task<IEnumerable<AzureTablePropertyBag>> ExecuteQueryWithFilterAsync(
            IAzureTableStorageAccess tableAccess,
            string tableName,
            IEnumerable<TraceRecord> recordsInterestedIn,
            IEnumerable<TraceRecordFilter> recordFilters,
            DateTimeOffset startTime,
            DateTimeOffset endTime,
            CancellationToken token)
        {
            var serverSideQueryFilter = string.Empty;
            bool serverSideQueryCreated = TryCreateQueryFilterForTable(tableName, recordsInterestedIn, recordFilters, out serverSideQueryFilter);

            if (!serverSideQueryCreated)
            {
                serverSideQueryFilter = string.Empty;
            }

            Func<AzureTablePropertyBag, bool> clientSideFilteringFunction = (oneResult) => IsMatch(oneResult, recordFilters);

            var queryResult = await tableAccess.ExecuteQueryAsync(
                tableName,
                new AzureTimeLoggedBasedFilterBuilder(),
                serverSideQueryFilter,
                clientSideFilteringFunction,
                startTime,
                endTime,
                MaxNumberOfRecordsToQuery,
                token).ConfigureAwait(false);

            var returnResult = queryResult.AzureTableProperties.OrderByDescending(item => item.TimeLogged).ToList();
            if (returnResult.Count() > MaxNumberOfRecordsToReturn)
            {
                returnResult = returnResult.Take(MaxNumberOfRecordsToReturn).ToList();
            }

            return returnResult;
        }

        private static bool IsMatch(AzureTablePropertyBag oneResult, IEnumerable<TraceRecordFilter> filters)
        {
            if (!oneResult.UserDataMap.ContainsKey(WellKnownFields.EventType))
            {
                throw new MissingFieldException("Well Known field Event Type is Missing in Azure Data");
            }

            // First check for Type filter.
            var matchingFilter = filters.SingleOrDefault(item =>
            {
                var azureEventType = Mapping.TraceRecordAzureDataMap.First(firstMapping => firstMapping.Type == item.RecordType).EventType;
                return azureEventType.Equals(oneResult.UserDataMap[WellKnownFields.EventType]);
            });

            // If filter for a Type is missing, it implies requester is not interested in that Type, so we move on.
            if (matchingFilter == null)
            {
                // This implies we are not interested in this record
                return false;
            }

            // If no property level filter is specified, it implies user needs all records of this type and we keep the record.
            if (!matchingFilter.Filters.Any())
            {
                return true;
            }

            // If requester has specified property level filtering, we apply those filters.
            bool allFieldFiltersMatch = true;
            foreach (var onePropertyFilter in matchingFilter.Filters)
            {
                if (!oneResult.UserDataMap.ContainsKey(onePropertyFilter.Name))
                {
                    throw new MissingFieldException(string.Format("Filter field : {0} is Missing in data", onePropertyFilter.Name));
                }

                if (!oneResult.UserDataMap[onePropertyFilter.Name].Equals(onePropertyFilter.Value.ToString(), StringComparison.InvariantCultureIgnoreCase))
                {
                    allFieldFiltersMatch = false;
                    break;
                }
            }

            return allFieldFiltersMatch;
        }

        public static bool TryCreateQueryFilterForTable(string tableName, IEnumerable<TraceRecord> records, IEnumerable<TraceRecordFilter> filters, out string queryFilter)
        {
            // Partition key filtering is required if Entire table is not in Scope.
            var isQueryScopeTableLevel = IsEntireTableInScope(tableName, filters);

            queryFilter = CreateEventVersionFilter(records);
            if (isQueryScopeTableLevel)
            {
                var uniqueProperties = GetUniquePropertyFilters(filters);
                foreach (var oneUniqueProperty in uniqueProperties)
                {
                    queryFilter = TableQuery.CombineFilters(queryFilter, TableOperators.And, CreateAzureTableQueryPropertyFilter(oneUniqueProperty.Name, oneUniqueProperty.Value, QueryComparisons.Equal));
                }

                return true;
            }

            var count = GetActualFilterCount(filters);
            if (count > MaxFilterCount)
            {
                queryFilter = null;
                return false;
            }

            var restOfThefilter = string.Empty;
            foreach (var oneRecord in records)
            {
                var mapping = Mapping.TraceRecordAzureDataMap.Single(item => item.Type == oneRecord.GetType());

                var azureFilterForCurrentRecord = CreateQueryFilterForSingleRecord(oneRecord, filters.FirstOrDefault(item => item.RecordType == oneRecord.GetType()));

                // Add filter for this record to rest of the filters for this table. Please note, this would be 'OR' join.
                if (azureFilterForCurrentRecord != string.Empty)
                {
                    restOfThefilter = restOfThefilter == string.Empty ? azureFilterForCurrentRecord : TableQuery.CombineFilters(restOfThefilter, TableOperators.Or, azureFilterForCurrentRecord);
                }
            }

            if (!string.IsNullOrEmpty(restOfThefilter))
            {
                queryFilter = TableQuery.CombineFilters(queryFilter, TableOperators.And, restOfThefilter);
            }

            return true;
        }

        // Create filter at Record level.
        public static string CreateQueryFilterForSingleRecord(TraceRecord oneRecord, TraceRecordFilter userFilterForRecord)
        {
            var mapping = Mapping.TraceRecordAzureDataMap.Single(item => item.Type == oneRecord.GetType());

            var azureFilterForCurrentRecord = string.Empty;

            // Calculate partition filter (if data type is static)
            if (mapping.PartitionDataType == PartitionDataType.StaticData)
            {
                var queryFilter = TableQuery.GenerateFilterCondition(WellKnownFields.PartitionKey, QueryComparisons.Equal, mapping.PartitionKeyDetail);
                azureFilterForCurrentRecord = string.IsNullOrEmpty(azureFilterForCurrentRecord) ? queryFilter : TableQuery.CombineFilters(azureFilterForCurrentRecord, TableOperators.And, queryFilter);
            }

            if (userFilterForRecord == null || !userFilterForRecord.Filters.Any())
            {
                return azureFilterForCurrentRecord;
            }

            // If Partition Filter data is dynamic, try to create partition filter.
            var userSpecifiedPropertyFilterForCurrentRecord = new List<PropertyFilter>(userFilterForRecord.Filters);
            if (mapping.PartitionDataType == PartitionDataType.PropertyName)
            {
                var userFilterForPartitionProperty = userSpecifiedPropertyFilterForCurrentRecord.SingleOrDefault(item => item.Name == mapping.PartitionKeyDetail);
                if (userFilterForPartitionProperty != null)
                {
                    var queryFilter = CreateAzureTableQueryPropertyFilter(WellKnownFields.PartitionKey, userFilterForPartitionProperty.Value, QueryComparisons.Equal);
                    azureFilterForCurrentRecord = string.IsNullOrEmpty(azureFilterForCurrentRecord) ? queryFilter : TableQuery.CombineFilters(azureFilterForCurrentRecord, TableOperators.And, queryFilter);
                    userSpecifiedPropertyFilterForCurrentRecord.Remove(userFilterForPartitionProperty);
                }

            }

            // Now Create partition for rest of the property user may have specified.
            var azurePropertyFilterForCurrentRecord = string.Empty;
            foreach (var onePropertyFilter in userSpecifiedPropertyFilterForCurrentRecord)
            {
                var queryFilter = CreateAzureTableQueryPropertyFilter(onePropertyFilter);

                azurePropertyFilterForCurrentRecord = string.IsNullOrEmpty(azurePropertyFilterForCurrentRecord)
                    ? queryFilter
                    : TableQuery.CombineFilters(azurePropertyFilterForCurrentRecord, TableOperators.And, queryFilter);
            }

            // Add property filter.
            if (azurePropertyFilterForCurrentRecord != string.Empty)
            {
                azureFilterForCurrentRecord = azureFilterForCurrentRecord == string.Empty ? azurePropertyFilterForCurrentRecord : TableQuery.CombineFilters(azureFilterForCurrentRecord, TableOperators.And, azurePropertyFilterForCurrentRecord);
            }

            return azureFilterForCurrentRecord;
        }

        public static string CreateEventVersionFilter(IEnumerable<TraceRecord> recordsInterestedIn)
        {
            var uniqueVersions = recordsInterestedIn.Select(item => item.GetMinimumVersionOfProductWherePresent()).Distinct();
            if (uniqueVersions.Count() == 1)
            {
                return CreateAzureTableQueryPropertyFilter(WellKnownFields.EventVersion, uniqueVersions.ElementAt(0), QueryComparisons.GreaterThanOrEqual);
            }

            throw new NotSupportedException("In this Release, only same Version is Supported");
        }

        public static string CreateAzureTableQueryPropertyFilter(PropertyFilter propertyFilter)
        {
            Assert.IsNotNull(propertyFilter, "filter != null");
            return CreateAzureTableQueryPropertyFilter(propertyFilter.Name, propertyFilter.Value, QueryComparisons.Equal);
        }

        public static string CreateAzureTableQueryPropertyFilter(string propertyName, object value, string queryComparison)
        {
            Assert.IsNotNull(value, "value != null");

            // When DCA pushes data into Azure, if any property has a '.' in it, it is replaced by '_'. We are taking care of it
            // below while creating filter for query to Azure table.
            propertyName = propertyName.Replace('.', '_');
            if (value.GetType() == typeof(Guid))
            {
                if ((Guid)value != Guid.Empty)
                {
                    return TableQuery.GenerateFilterConditionForGuid(propertyName, queryComparison, (Guid)value);
                }
            }

            if (value.GetType() == typeof(string))
            {
                if ((string)value != null)
                {
                    return TableQuery.GenerateFilterCondition(propertyName, queryComparison, (string)value);
                }
            }

            if (value.GetType() == typeof(int))
            {
                return TableQuery.GenerateFilterConditionForInt(propertyName, queryComparison, (int)value);
            }

            if (value.GetType() == typeof(Int64))
            {
                return TableQuery.GenerateFilterConditionForLong(propertyName, queryComparison, (long)value);
            }

            throw new ArgumentException(string.Format("Type {0} Not supported in Filter Creation", value.GetType()));
        }

        private static IList<PropertyFilter> GetUniquePropertyFilters(IEnumerable<TraceRecordFilter> filters)
        {
            IList<PropertyFilter> uniquePropertyFilters = new List<PropertyFilter>();
            foreach (var onePropertyFilter in filters.SelectMany(item => item.Filters))
            {
                if (uniquePropertyFilters.Contains(onePropertyFilter))
                {
                    continue;
                }

                uniquePropertyFilters.Add(onePropertyFilter);
            }

            return uniquePropertyFilters;
        }

        private static int GetActualFilterCount(IEnumerable<TraceRecordFilter> filters)
        {
            // Each filter at least contribute one filter i.e. the Type filter
            var count = filters.Count();

            // Now check for property level filtering within each Record.
            foreach (var oneFilter in filters)
            {
                count += oneFilter.Filters.Count();
            }

            return count;
        }

        private static string CreatePartitionKeyFilter(TraceRecord oneRecord, TraceRecordFilter userFilterForRecord)
        {
            var partitionKeyFilter = string.Empty;

            var mapping = Mapping.TraceRecordAzureDataMap.Single(item => item.Type == oneRecord.GetType());

            // Calculate partition filter (if data type is static)
            if (mapping.PartitionDataType == PartitionDataType.StaticData)
            {
                var queryFilter = TableQuery.GenerateFilterCondition(WellKnownFields.PartitionKey, QueryComparisons.Equal, mapping.PartitionKeyDetail);
                partitionKeyFilter = string.IsNullOrEmpty(partitionKeyFilter) ? queryFilter : TableQuery.CombineFilters(partitionKeyFilter, TableOperators.And, queryFilter);
            }

            if (userFilterForRecord == null || !userFilterForRecord.Filters.Any())
            {
                return partitionKeyFilter;
            }

            // If Partition Filter data is dynamic, try to create partition filter.
            if (mapping.PartitionDataType == PartitionDataType.PropertyName)
            {
                var userFilterForPartitionProperty = userFilterForRecord.Filters.SingleOrDefault(item => item.Name == mapping.PartitionKeyDetail);
                if (userFilterForPartitionProperty != null)
                {
                    var queryFilter = CreateAzureTableQueryPropertyFilter(WellKnownFields.PartitionKey, userFilterForPartitionProperty.Value, QueryComparisons.Equal);
                    partitionKeyFilter = string.IsNullOrEmpty(partitionKeyFilter) ? queryFilter : TableQuery.CombineFilters(partitionKeyFilter, TableOperators.And, queryFilter);
                }
            }

            return partitionKeyFilter;
        }

        private static bool IsEntireTableInScope(string tableName, IEnumerable<TraceRecordFilter> filters)
        {
            var result = Mapping.TraceRecordAzureDataMap.GroupBy(item => item.TableName).ToDictionary(gdc => gdc.Key, gdc => gdc.Select(item => item.Type).ToList());

            var key = result.Keys.SingleOrDefault(item => tableName.EndsWith(item));
            if (key == null)
            {
                throw new System.Exception(string.Format("Unexpected table Name: {0}", tableName));
            }

            var typesInTable = result[key];
            if (typesInTable.Count() != filters.Count())
            {
                return false;
            }

            foreach (var oneInterestedType in filters)
            {
                if (!typesInTable.Contains(oneInterestedType.RecordType))
                {
                    return false;
                }
            }

            return true;
        }
    }
}
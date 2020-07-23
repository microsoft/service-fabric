// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.Exception;

    public class ServiceFabricAzureTableRecordSession : TraceRecordSession
    {
        private IAzureTableStorageAccess tableAccess;

        private ServiceFabricAzureAccessDataDecorator dataDecorator;

        public ServiceFabricAzureTableRecordSession(string accountName, string accountKey, bool useHttps, string tableNamePrefix, string deploymentId)
        {
            Assert.IsNotNull(accountName, "accountName != null");
            Assert.IsNotNull(accountKey, "accountKey != null");
            this.tableAccess = new AccountAndKeyTableStorageAccess(accountName, accountKey, useHttps);
            this.dataDecorator = new ServiceFabricAzureAccessDataDecorator(this.tableAccess, tableNamePrefix, deploymentId);
        }

        public ServiceFabricAzureTableRecordSession(IAzureTableStorageAccess access, string tableNamePrefix, string deploymentId)
        {
            Assert.IsNotNull(access, "access != null");
            this.tableAccess = access;
            this.dataDecorator = new ServiceFabricAzureAccessDataDecorator(this.tableAccess, tableNamePrefix, deploymentId);
        }

        /// <inheritdoc />
        public override bool IsServerSideFilteringAvailable
        {
            get { return true; }
        }

        /// <inheritdoc />
        public override async Task StartReadingAsync(DateTimeOffset startTime, DateTimeOffset endTime, CancellationToken token)
        {
            await this.StartReadingAsync(new List<TraceRecordFilter>(), startTime, endTime, token);
        }

        public override async Task StartReadingAsync(IList<TraceRecordFilter> filters, DateTimeOffset startTime, DateTimeOffset endTime, CancellationToken token)
        {
            // Read all the Events and then dispatch them one by one.
            var values = await this.GatherResultsAcrossTablesAsync(filters, startTime, endTime, token).ConfigureAwait(false);

            foreach (var oneValue in values)
            {
                token.ThrowIfCancellationRequested();

                ValidMandatoryFields(oneValue);
 
                // We can make this a mandatory field. TODO
                if(!oneValue.UserDataMap.ContainsKey(WellKnownFields.EventVersion))
                {
                    continue;
                }

                var metadata = Mapping.TraceRecordAzureDataMap.SingleOrDefault(
                    item => item.EventType == oneValue.UserDataMap[WellKnownFields.EventType] &&
                            item.TaskName.ToString().Equals(oneValue.UserDataMap[WellKnownFields.TaskName]));

                if (metadata == null)
                {
                    throw new Exception(
                        string.Format(
                            CultureInfo.InvariantCulture,
                            "Can't Reverse transform for unknown Type. Type: {0}, Task: {1}",
                            oneValue.UserDataMap[WellKnownFields.EventType],
                            oneValue.UserDataMap[WellKnownFields.TaskName]));
                }

                var traceRecordRead = (TraceRecord)Activator.CreateInstance(metadata.Type, new object[] { null });
                var traceRecord = this.Lookup(traceRecordRead.Identity);
                if (traceRecord?.Target == null)
                {
                    continue;
                }

                // Today, DCA, if it sees an ID field in event data, removes it and sets it value as the partition key. We need
                // to do the reverse transformation now.
                if (!oneValue.UserDataMap.ContainsKey(WellKnownFields.EventDataId) && oneValue.PartitionKey != string.Format(CultureInfo.InvariantCulture, "{0}.{1}", metadata.EventType, metadata.TaskName))
                {
                    oneValue.UserDataMap.Add(WellKnownFields.EventDataId, oneValue.PartitionKey);
                }

                // TODO: This is a bug in DCA. It doesn't upload Critical information to the Tables today, so 
                // I've to resort to filling bogus values. We need to fix DCA.
                traceRecord.Level = TraceLevel.Info;
                traceRecord.TracingProcessId = 1;
                traceRecord.ThreadId = 1;
                traceRecord.Version = byte.Parse(oneValue.UserDataMap[WellKnownFields.EventVersion]);
                traceRecord.TimeStamp = oneValue.TimeLogged.UtcDateTime;
                traceRecord.PropertyValueReader = ValueReaderCreator.CreatePropertyBagValueReader(traceRecord.GetType(), traceRecord.Version, oneValue.UserDataMap);

                this.DispatchAsync(traceRecord, CancellationToken.None).GetAwaiter().GetResult();
            }
        }

        // Depending on how events are stored in Azure tables, we may need to query across multiple tables.
        private async Task<IEnumerable<AzureTablePropertyBag>> GatherResultsAcrossTablesAsync(
            IList<TraceRecordFilter> filters,
            DateTimeOffset startTime,
            DateTimeOffset endTime,
            CancellationToken token)
        {
            // So, we may be looking for multiple records spread across many tables. However, we only want to query a Table once.
            // In below step, we form a map between a Table and all the records we want to look for inside that table.
            IDictionary<string, IList<TraceRecord>> tableAndRecordsInItMap = this.GetTableAndRecordsInItMap();

            // Now we go over each table and query it.
            var results = new List<AzureTablePropertyBag>();
            foreach (var oneTableKvp in tableAndRecordsInItMap)
            {
                var decoratedTableName = await this.dataDecorator.DecorateTableNameAsync(oneTableKvp.Key, token).ConfigureAwait(false);
                results.AddRange(await AzureTableQueryHelper.ExecuteQueryWithFilterAsync(this.tableAccess, decoratedTableName, oneTableKvp.Value, filters, startTime, endTime, token));
            }

            return results;
        }

        private IDictionary<string, IList<TraceRecord>> GetTableAndRecordsInItMap()
        {
            IDictionary<string, IList<TraceRecord>> tableAndRecordsInItMap = new Dictionary<string, IList<TraceRecord>>();

            var recordsBeingWatched = this.TraceIdentifierAndTraceTemplateMap.Values;
            foreach (var oneRecord in recordsBeingWatched)
            {
                var transform = Mapping.TraceRecordAzureDataMap.SingleOrDefault(item => item.Type == oneRecord.GetType());
                if (transform == null)
                {
                    throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "Don't know how to Read Type: {0}", oneRecord.GetType()));
                }

                if (tableAndRecordsInItMap.ContainsKey(transform.TableName))
                {
                    tableAndRecordsInItMap[transform.TableName].Add(oneRecord);
                }
                else
                {
                    tableAndRecordsInItMap[transform.TableName] = new List<TraceRecord> { oneRecord };
                }
            }

            return tableAndRecordsInItMap;
        }

        private static void ValidMandatoryFields(AzureTablePropertyBag azureOnRecord)
        {
            if (!azureOnRecord.UserDataMap.ContainsKey(WellKnownFields.EventType))
            {
                throw new PropertyMissingException(WellKnownFields.EventType);
            }

            if (!azureOnRecord.UserDataMap.ContainsKey(WellKnownFields.TaskName))
            {
                throw new PropertyMissingException(WellKnownFields.TaskName);
            }
        }
    }
}
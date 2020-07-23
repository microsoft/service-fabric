// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.Dca;

namespace AzureTableUploaderTest
{
    using Microsoft.WindowsAzure.Storage.Table;
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Runtime.InteropServices;
    using Tools.EtlReader;
    using WEX.TestExecution;
    using AzureTableDownloader;

    class NonIdBasedEventSource : EventSource
    {
        // ID of NonIdBased, taken from ETW event manifest
        private const ushort NonIdBasedTaskId = 2;
        // ID of NonIdBased.NoId1, taken from ETW event manifest
        private const ushort Event1 = 3;
        // ID of NonIdBased.NoId2, taken from ETW event manifest
        private const ushort Event2 = 4;
        // Table name, taken from ETW event manifest
        private const string TableName = "NonIdBasedTable";
        // Task name, taken from ETW event manifest
        private const string TaskName = "NonIdBased";
        // Event types, taken from ETW event manifest
        private const string EventType1 = "NoId1";
        private const string EventType2 = "NoId2";

        private const int ReverseChronologicalRowKeyFlag = unchecked((int)0x80000000);

        private const int TotalEventCount = 100;
        private int eventsDelivered;
        private List<DateTime> eventTimestamps;

        internal NonIdBasedEventSource(Guid providerId, DateTime startTime) 
                    : base(providerId, startTime)
        {
            eventsDelivered = 0;
            this.eventTimestamps = new List<DateTime>();
        }

        private void InitializeEventData(int eventsDelivered, out IntPtr userData, out ushort userDataLength)
        {
            // Allocate the buffer
            int size = Marshal.SizeOf(typeof(int));
            IntPtr buffer = Marshal.AllocHGlobal(size);

            // Write to the buffer
            Marshal.WriteInt32(buffer, eventsDelivered);

            userData = buffer;
            userDataLength = (ushort)size;
        }

        private void InitializeEventDataWithVersion(int eventsDelivered, out IntPtr userData, out ushort userDataLength)
        {
            // Allocate the buffer for two fields:
            // 1. data (int)
            // 2. dca_version (int)
            int size = Marshal.SizeOf(typeof(int)) * 2;
            IntPtr buffer = Marshal.AllocHGlobal(size);

            // Write to the buffer
            int offset = 0;
            Marshal.WriteInt32(buffer, offset, eventsDelivered);
            offset = offset + Marshal.SizeOf(typeof(int));
            Marshal.WriteInt32(buffer, offset, 1 /* version */);

            userData = buffer;
            userDataLength = (ushort)size;
        }

        internal override bool GetNextEvent(out EventRecord eventRecord)
        {
            eventRecord = new EventRecord();
            if (eventsDelivered == TotalEventCount)
            {
                return false;
            }
            DateTime eventTimestamp;
            InitializeEventRecord(
                NonIdBasedTaskId,
                ((eventsDelivered % 2) == 0) ? Event1 : Event2,
                0,
                eventsDelivered * AzureTableUploaderTest.EventIntervalMinutes,
                ref eventRecord,
                out eventTimestamp);
            this.eventTimestamps.Add(eventTimestamp);

            IntPtr userData;
            ushort userDataLength;
            if ((eventsDelivered % 2) == 0)
            {
                InitializeEventDataWithVersion(eventsDelivered, out userData, out userDataLength);
            }
            else
            {
                InitializeEventData(eventsDelivered, out userData, out userDataLength);
            }
            UpdateEventRecordBuffer(ref eventRecord, userData, userDataLength);
            eventsDelivered++;
            return true;
        }

        internal void VerifyUpload(CloudTableClient tableClient)
        {
            CloudTable table = tableClient.GetTableReference(String.Concat(AzureTableUploaderTest.TableNamePrefix, TableName));
            TableQuery<DynamicTableEntity> tableQuery = new TableQuery<DynamicTableEntity>();
            IEnumerable<DynamicTableEntity> entities = table.ExecuteQuery(tableQuery);

            bool[] eventUploaded = new bool[TotalEventCount];
            foreach (DynamicTableEntity entity in entities)
            {
                // Validate task name
                string taskName = entity.Properties[AzureTableUploaderTest.TaskNameProperty].StringValue;
                AzureTableUploaderTest.VerifyEntityProperty(
                    entity,
                    AzureTableUploaderTest.TaskNameProperty,
                    table.Name,
                    new string[] {TaskName},
                    taskName);

                // Validate event type
                string eventType = entity.Properties[AzureTableUploaderTest.EventTypeProperty].StringValue;
                AzureTableUploaderTest.VerifyEntityProperty(
                    entity,
                    AzureTableUploaderTest.EventTypeProperty,
                    table.Name,
                    new string[] { EventType1, EventType2 },
                    eventType);

                // Validate partition key
                AzureTableUploaderTest.VerifyEntityProperty(
                    entity,
                    AzureTableDownloader.PartitionKeyProperty,
                    table.Name,
                    new string[] { String.Concat(taskName, ".", eventType) },
                    entity.PartitionKey);

                // Verify DCA version
                AzureTableUploaderTest.VerifyEntityProperty(
                    entity,
                    AzureTableUploaderTest.DcaVersionProperty,
                    table.Name,
                    eventType.Equals(EventType1) ? new int[] { (1 | ReverseChronologicalRowKeyFlag) } : new int[] { ReverseChronologicalRowKeyFlag },
                    (int) entity.Properties[AzureTableUploaderTest.DcaVersionProperty].Int32Value);

                // Verify data
                int data = (int) entity.Properties[AzureTableUploaderTest.DataProperty].Int32Value;
                eventUploaded[data] = true;
                if (eventType.Equals(EventType1))
                {
                    if (0 != (data % 2))
                    {
                        Utility.TraceSource.WriteError(
                            AzureTableUploaderTest.TraceType,
                            "Property {0} for entity with PK: {1}, RK: {2} in table {3} has invalid value {4}. Value should be an even number.",
                            AzureTableUploaderTest.DataProperty,
                            entity.PartitionKey,
                            entity.RowKey,
                            table.Name,
                            data);
                        Verify.Fail("Invalid property value in table");
                    }
                }
                else
                {
                    if (0 == (data % 2))
                    {
                        Utility.TraceSource.WriteError(
                            AzureTableUploaderTest.TraceType,
                            "Property {0} for entity with PK: {1}, RK: {2} in table {3} has invalid value {4}. Value should be an odd number.",
                            AzureTableUploaderTest.DataProperty,
                            entity.PartitionKey,
                            entity.RowKey,
                            table.Name,
                            data);
                        Verify.Fail("Invalid property value in table");
                    }
                }
            }

            // Verify that all records were uploaded
            for (int i = 0; i < eventUploaded.Length; i++)
            {
                if (false == eventUploaded[i])
                {
                    Utility.TraceSource.WriteError(
                        AzureTableUploaderTest.TraceType,
                        "Entity with value {0} for property {1} was not uploaded to table {2}.",
                        i,
                        AzureTableUploaderTest.DataProperty,
                        table.Name);
                    Verify.Fail("Entity not uploaded");
                }
            }
        }

        internal bool VerifyDeletion(CloudTableClient tableClient, DateTime deletionCutoffTimeLow, DateTime deletionCutoffTimeHigh)
        {
            CloudTable table = tableClient.GetTableReference(String.Concat(AzureTableUploaderTest.TableNamePrefix, TableName));

            // Verify that the entities that should have been deleted are already deleted
            string cutoffTimeTicks = (DateTime.MaxValue.Ticks - deletionCutoffTimeLow.Ticks).ToString("D20", CultureInfo.InvariantCulture);
            TableQuery<DynamicTableEntity> oldEntityQuery = (new TableQuery<DynamicTableEntity>())
                                                             .Where(TableQuery.GenerateFilterCondition(
                                                                        "RowKey",
                                                                        QueryComparisons.GreaterThan,
                                                                        cutoffTimeTicks));
            IEnumerable<DynamicTableEntity> deletedEntities = table.ExecuteQuery(oldEntityQuery);
            if (deletedEntities.Count() > 0)
            {
                // Don't fail the test yet. Just write a warning and return. We can 
                // only predict the approximate deletion time, so maybe we are checking
                // too early. The caller will check again later.
                Utility.TraceSource.WriteWarning(
                    AzureTableUploaderTest.TraceType,
                    "All old entities have not yet been deleted from table {0}. We expect all entities with tick count less than {1} (RowKey greater than {2}) to be deleted.",
                    table.Name,
                    deletionCutoffTimeLow.Ticks,
                    cutoffTimeTicks);
                return false;
            }

            // Verify that the entities we expect to be present have not been deleted
            int expectedEntityCount = this.eventTimestamps
                                         .Where(t => { return (t.CompareTo(deletionCutoffTimeHigh) > 0); })
                                         .Count();
            cutoffTimeTicks = (DateTime.MaxValue.Ticks - deletionCutoffTimeHigh.Ticks).ToString("D20", CultureInfo.InvariantCulture);
            TableQuery<DynamicTableEntity> availableEntityQuery = (new TableQuery<DynamicTableEntity>())
                                                                     .Where(TableQuery.GenerateFilterCondition(
                                                                                "RowKey",
                                                                                QueryComparisons.LessThan,
                                                                                cutoffTimeTicks));
            int availableEntityCount = table.ExecuteQuery(availableEntityQuery).Count();
            if (availableEntityCount != expectedEntityCount)
            {
                // Fail the test immediately. The absence of these entities indicate
                // that the deletion has already happened and some entities were 
                // unexpectedly deleted.
                Utility.TraceSource.WriteError(
                    AzureTableUploaderTest.TraceType,
                    "Some entities have been unexpectedly deleted from table {0}. We expected to find {1} entities with tick count greater than {2} (RowKey less than {3}), but found only {4}.",
                    table.Name,
                    expectedEntityCount,
                    deletionCutoffTimeHigh.Ticks,
                    cutoffTimeTicks,
                    availableEntityCount);
                Verify.Fail("Entities unexpectedly deleted from table.");
            }

            return true;
        }
    }
}
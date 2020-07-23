// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.Dca;

namespace AzureTableUploaderTest
{
    using FabricDCA;
    using Microsoft.WindowsAzure.Storage.Table;
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Runtime.InteropServices;
    using Tools.EtlReader;
    using WEX.TestExecution;

    class IdBasedEventSource : EventSource
    {
        // ID of IdBased, taken from ETW event manifest
        private const ushort IdBasedTaskId = 1;
        // ID of IdBased.IdAndInstance1, taken from ETW event manifest
        private const ushort EventIdAndInstance1 = 0;
        // ID of IdBased.IdAndInstance2, taken from ETW event manifest
        private const ushort EventIdAndInstance2 = 1;
        // ID of IdBased.IdOnly, taken from ETW event manifest
        private const ushort EventIdOnly = 2;
        // ID of IdBased.IdAndInstanceDeletion, taken from ETW event manifest
        private const ushort EventIdAndInstanceDeletion = 5;
        // ID of IdBased.IdOnlyDeletion, taken from ETW event manifest
        private const ushort EventIdOnlyDeletion = 6;
        // Table names, take from ETW event manifest
        private static string[] TableNames = new string[] { "IdBasedTable1", "IdBasedTable2" };
        // Event to table map, inferred from ETW event manifest
        private static Dictionary<ushort, string> eventToTableMap = new Dictionary<ushort, string>()
        {
            { EventIdAndInstance1, TableNames[0] },
            { EventIdAndInstance2, TableNames[1] },
            { EventIdOnly, TableNames[1] },
            { EventIdAndInstanceDeletion, TableNames[0] },
            { EventIdOnlyDeletion, TableNames[1] },
        };

        private static ushort[] eventTypes = new ushort[] { EventIdAndInstance1, EventIdAndInstance2, EventIdOnly };
        private static Dictionary<ushort, ushort> deletionEventTypes = new Dictionary<ushort, ushort>()
        {
            { EventIdAndInstance1, EventIdAndInstanceDeletion },
            { EventIdAndInstance2, EventIdOnlyDeletion },
            { EventIdOnly, UInt16.MaxValue },
        };

        internal const int PartitionCount = 4;
        internal const int MaxInstancesPerPartition = 10;
        internal const int EventsPerInstance = 4;

        private int eventTypeIndex;
        private int partitionIndex;
        private int instanceIndex;
        private int eventIndex;

        private Dictionary<string, List<DateTime>> eventTimestamps = new Dictionary<string, List<DateTime>>();

        private int InstancesForPartition(int partIdx)
        {
            return (partIdx % 2) == 0 ? MaxInstancesPerPartition : (MaxInstancesPerPartition / 5);
        }
        private int maxInstancesForCurrentPartition
        {
            get
            {
                // For odd-numbered partitions, add very few events so that all of them
                // are old enough to be deleted when we perform our deletion pass
                //
                // For even-numbered partitions, add many events so that only some of them
                // are old enough to be deleted when we perform our deletion pass
                return InstancesForPartition(this.partitionIndex);
            }
        }
        private bool isLastEventForCurrentPartition
        {
            get
            {
                return ((this.instanceIndex == (this.maxInstancesForCurrentPartition - 1)) &&
                        (this.eventIndex == (EventsPerInstance - 1)));
            }
        }
        private bool PartitionShouldBeDeleted(int partIdx)
        {
            return partIdx >= (PartitionCount / 2);
        }
        private bool shouldDeleteCurrentPartitionObject
        {
            get
            {
                // Delete objects corresponding to some of the partitions and don't
                // delete for the others
                return PartitionShouldBeDeleted(this.partitionIndex);
            }
        }
        private int EventsForPartition(int partIdx)
        {
            return EventsPerInstance * InstancesForPartition(partIdx);
        }
        private int eventsDeliveredForCurrentPartition;

        private object stateLock;

        internal IdBasedEventSource(Guid providerId, DateTime startTime)
                    : base(providerId, startTime)
        {
            this.eventTypeIndex = 0;
            this.partitionIndex = 0;
            this.instanceIndex = 0;
            this.eventIndex = 0;
            this.eventsDeliveredForCurrentPartition = 0;
            this.stateLock = new object();
            this.eventTimestamps = new Dictionary<string, List<DateTime>>();
            foreach (string tableName in TableNames)
            {
                this.eventTimestamps[tableName] = new List<DateTime>();
            }
        }

        private void InitializeIdOnlyEventData(int id, int indexWithinPartition, out IntPtr userData, out ushort userDataLength)
        {
            // Allocate the buffer for the following fields:
            // 1. id (int)
            // 2. data (int)
            // 3. dca_version (int)
            int size = Marshal.SizeOf(typeof(int)) * 3;
            IntPtr buffer = Marshal.AllocHGlobal(size);

            // Write to the buffer
            int offset = 0;
            Marshal.WriteInt32(buffer, offset, id);
            offset = offset + Marshal.SizeOf(typeof(int));
            Marshal.WriteInt32(buffer, offset, indexWithinPartition);
            offset = offset + Marshal.SizeOf(typeof(int));
            Marshal.WriteInt32(buffer, offset, 0 /* version */);

            userData = buffer;
            userDataLength = (ushort)size;
        }

        private void InitializeIdAndInstanceEventData(int id, long instance, int indexWithinPartition, out IntPtr userData, out ushort userDataLength)
        {
            // Allocate the buffer for the following fields:
            // 1. id (int)
            // 2. dca_instance (long)
            // 3. data (int)
            // 4. dca_version (int)
            int size = (Marshal.SizeOf(typeof(int)) * 3) + Marshal.SizeOf(typeof(long));
            IntPtr buffer = Marshal.AllocHGlobal(size);

            // Write to the buffer
            int offset = 0;
            Marshal.WriteInt32(buffer, offset, id);
            offset = offset + Marshal.SizeOf(typeof(int));
            Marshal.WriteInt64(buffer, offset, instance);
            offset = offset + Marshal.SizeOf(typeof(long));
            Marshal.WriteInt32(buffer, offset, indexWithinPartition);
            offset = offset + Marshal.SizeOf(typeof(int));
            Marshal.WriteInt32(buffer, offset, 0 /* version */);

            userData = buffer;
            userDataLength = (ushort)size;
        }

        internal override bool GetNextEvent(out EventRecord eventRecord)
        {
            eventRecord = new EventRecord();
            ushort eventId;
            int id;
            long instance;
            int indexWithinPartition;
            lock (stateLock)
            {
                if (this.eventTypeIndex == eventTypes.Length)
                {
                    return false;
                }

                eventId = eventTypes[this.eventTypeIndex];
                ushort deletionEventId = deletionEventTypes[eventId];
                if ((UInt16.MaxValue != deletionEventId) &&
                    this.isLastEventForCurrentPartition &&
                    this.shouldDeleteCurrentPartitionObject)
                {
                    eventId = deletionEventId;
                }

                id = this.partitionIndex;
                instance = this.instanceIndex;

                indexWithinPartition = this.eventsDeliveredForCurrentPartition;
                this.eventsDeliveredForCurrentPartition++;

                (this.eventIndex)++;
                if (EventsPerInstance == this.eventIndex)
                {
                    this.eventIndex = 0;
                    (this.instanceIndex)++;
                    if (this.maxInstancesForCurrentPartition == this.instanceIndex)
                    {
                        this.instanceIndex = 0;
                        (this.partitionIndex)++;
                        this.eventsDeliveredForCurrentPartition = 0;
                        if (PartitionCount == this.partitionIndex)
                        {
                            this.partitionIndex = 0;
                            (this.eventTypeIndex)++;
                        }
                    }
                }
            }

            DateTime eventTimestamp;
            InitializeEventRecord(
                IdBasedTaskId,
                eventId,
                (EventIdOnly == eventId) ? (byte) 1 : (byte) 0,
                indexWithinPartition * AzureTableUploaderTest.EventIntervalMinutes,
                ref eventRecord,
                out eventTimestamp);
            List<DateTime> eventTimestampList = this.eventTimestamps[eventToTableMap[eventId]];
            lock (eventTimestampList)
            {
                eventTimestampList.Add(eventTimestamp);
            }
            

            IntPtr userData;
            ushort userDataLength;
            if ((EventIdOnly == eventId) || (EventIdOnlyDeletion == eventId))
            {
                InitializeIdOnlyEventData(id, indexWithinPartition, out userData, out userDataLength);
            }
            else
            {
                InitializeIdAndInstanceEventData(id, instance, indexWithinPartition, out userData, out userDataLength);
            }

            UpdateEventRecordBuffer(ref eventRecord, userData, userDataLength); 
            return true;
        }

        internal void VerifyUpload(CloudTableClient tableClient)
        {
            foreach (string tableName in TableNames)
            {
                CloudTable table = tableClient.GetTableReference(String.Concat(AzureTableUploaderTest.TableNamePrefix, tableName));

                // Verify partition count
                TableQuery<DynamicTableEntity> statusRecordQuery = new TableQuery<DynamicTableEntity>();
                statusRecordQuery = (new TableQuery<DynamicTableEntity>())
                                 .Where(TableQuery.GenerateFilterCondition(
                                            "RowKey",
                                            QueryComparisons.Equal,
                                            AzureTableQueryableEventUploader.StatusEntityRowKey));
                IEnumerable<DynamicTableEntity> statusRecords = table.ExecuteQuery(statusRecordQuery);
                int partitionCount = 0;
                foreach (DynamicTableEntity statusRecord in statusRecords)
                {
                    // Verify that the object's deletion state is correct
                    bool isDeleted = (bool) statusRecord.Properties["IsDeleted"].BooleanValue;
                    long instance = (long) statusRecord.Properties["Instance"].Int64Value;
                    if (false == PartitionShouldBeDeleted(partitionCount))
                    {
                        if (isDeleted)
                        {
                            Utility.TraceSource.WriteError(
                                AzureTableUploaderTest.TraceType,
                                "Object represented by partition {0} in table {1} is in a deleted state, but it shouldn't be.",
                                partitionCount,
                                tableName);
                            Verify.Fail("Unexpected deletion state");
                        }
                        long latestInstance = (InstancesForPartition(partitionCount) - 1);
                        if (instance != latestInstance)
                        {
                            Utility.TraceSource.WriteError(
                                AzureTableUploaderTest.TraceType,
                                "Status record for partition {0} in table {1} does not have the expected instance value. Expected value: {2}. Actual value: {3}.",
                                partitionCount,
                                tableName,
                                latestInstance,
                                instance);
                            Verify.Fail("Unexpected instance value in status record");
                        }
                    }
                    else
                    {
                        if (false == isDeleted)
                        {
                            Utility.TraceSource.WriteError(
                                AzureTableUploaderTest.TraceType,
                                "Object represented by partition {0} in table {1} should be in a deleted state, but it isn't.",
                                partitionCount,
                                tableName);
                            Verify.Fail("Unexpected deletion state");
                        }
                        long specialInstanceForDeletedObject;
                        unchecked
                        {
                            specialInstanceForDeletedObject = (long)UInt64.MaxValue;
                        }
                        long latestInstance = (InstancesForPartition(partitionCount) - 1);
                        if ((instance != specialInstanceForDeletedObject) &&
                            (instance != latestInstance))
                        {
                            Utility.TraceSource.WriteError(
                                AzureTableUploaderTest.TraceType,
                                "Status record for partition {0} in table {1} does not have the expected instance value. Acceptable values: {2}, {3}. Actual value: {4}.",
                                partitionCount,
                                tableName,
                                specialInstanceForDeletedObject,
                                latestInstance,
                                instance);
                            Verify.Fail("Unexpected instance value in status record");
                        }
                    }

                    // Verify records within the partition
                    VerifyPartitionEntities(table, partitionCount);

                    partitionCount++;
                }
                if (PartitionCount != partitionCount)
                {
                    Utility.TraceSource.WriteError(
                        AzureTableUploaderTest.TraceType,
                        "Table {0} does not have the expected number of partitions. Expected partition count: {1}. Actual partition count: {2}",
                        tableName,
                        PartitionCount,
                        partitionCount);
                    Verify.Fail("Unexpected partition count in table");
                }
            }
        }

        private void VerifyPartitionEntities(CloudTable table, int partIdx)
        {
            // Get records in the partition
            TableQuery<DynamicTableEntity> partitionQuery = new TableQuery<DynamicTableEntity>();
            partitionQuery = (new TableQuery<DynamicTableEntity>())
                             .Where(TableQuery.GenerateFilterCondition(
                                        "PartitionKey",
                                        QueryComparisons.Equal,
                                        partIdx.ToString()));
            IEnumerable<DynamicTableEntity> partitionRecords = table.ExecuteQuery(partitionQuery);
            if (0 == partitionRecords.Count())
            {
                Utility.TraceSource.WriteError(
                    AzureTableUploaderTest.TraceType,
                    "Table {0} partition {1} has no records.",
                    table.Name,
                    partIdx);
                Verify.Fail("No records found in partition");
            }

            bool hasDeletionEventWithIdOnly = (partitionRecords
                                               .Where(r => ((r.Properties.ContainsKey(AzureTableUploaderTest.EventTypeProperty)) &&
                                                            (r.Properties[AzureTableUploaderTest.EventTypeProperty].StringValue.Equals("IdOnlyDeletion"))))
                                               .Count()) > 0;
            int expectedEntityCount = EventsForPartition(partIdx);
            EqualityComparerDataProperty dataComparer = new EqualityComparerDataProperty();
            if (table.Name.Equals(String.Concat(AzureTableUploaderTest.TableNamePrefix, TableNames[1])))
            {
                // Get records without instance information
                IEnumerable<DynamicTableEntity> recordsWithoutInstance = partitionRecords.Where(
                    r => ((false == r.Properties.ContainsKey("dca_instance")) && 
                          (false == r.RowKey.Equals(AzureTableQueryableEventUploader.StatusEntityRowKey))));

                // Verify the records
                VerifyPartitionEntities(
                    table, 
                    recordsWithoutInstance, 
                    partIdx,
                    hasDeletionEventWithIdOnly ? (expectedEntityCount+1) : expectedEntityCount, 
                    dataComparer,
                    hasDeletionEventWithIdOnly ? 1 : 0,
                    false);
            }

            // Get records with instance information
            IEnumerable<DynamicTableEntity> recordsWithInstance = partitionRecords.Where(
                r => (r.Properties.ContainsKey("dca_instance")));

            // Verify the records
            VerifyPartitionEntities(
                table, 
                recordsWithInstance, 
                partIdx,
                hasDeletionEventWithIdOnly ? (expectedEntityCount-1) : expectedEntityCount, 
                dataComparer,
                0,
                true);

            // Verify the instance information in the records
            VerifyInstanceInformation(table, recordsWithInstance, partIdx, hasDeletionEventWithIdOnly);
        }

        private void VerifyPartitionEntities(
                        CloudTable table, 
                        IEnumerable<DynamicTableEntity> records, 
                        int partIdx,
                        int expectedEntityCount, 
                        EqualityComparerDataProperty dataComparer,
                        int expectedEntitiesWithDuplicateData,
                        bool recordsContainInstanceInfo)
        {
            string recordDescription = recordsContainInstanceInfo ? "records with instance information" : "records without instance information";
 
            // Verify the entity count
            int numRecords = records.Count();
            if (numRecords != expectedEntityCount)
            {
                Utility.TraceSource.WriteError(
                    AzureTableUploaderTest.TraceType,
                    "Table {0} partition {1} does not have the expected number of {2}. Expected records: {3}, actual records: {4}.",
                    table.Name,
                    partIdx,
                    recordDescription,
                    expectedEntityCount,
                    numRecords);
                Verify.Fail("Unexpected count of records");
            }

            // Verify that the records do not have duplicate data
            IEnumerable<DynamicTableEntity> distinctRecords =
                records.Distinct(dataComparer);
            int numDistinctRecords = distinctRecords.Count();
            if ((numRecords - numDistinctRecords) != expectedEntitiesWithDuplicateData)
            {
                Utility.TraceSource.WriteError(
                    AzureTableUploaderTest.TraceType,
                    "We found duplicate data in {0} {1}, but we expected duplicate data in {2} records. Table {3}, partition {4}.",
                    (numRecords - numDistinctRecords),
                    recordDescription,
                    expectedEntitiesWithDuplicateData,
                    table.Name,
                    partIdx);
                Verify.Fail("Unexpected partition records with duplicate data");
            }

            // Verify that the data values are as expected
            int maxDataValue = records.Max(
                r => { return (int)r.Properties[AzureTableUploaderTest.DataProperty].Int32Value; });
            if (maxDataValue != (numDistinctRecords - 1))
            {
                Utility.TraceSource.WriteError(
                    AzureTableUploaderTest.TraceType,
                    "Unexpected maximum data value for {0} in table {1} partition {2}. Expected max: {3}, Actual max: {4}.",
                    recordDescription,
                    table.Name,
                    partIdx,
                    (numRecords - 1),
                    maxDataValue);
                Verify.Fail("Unexpected maximum entity data value in partition");
            }
            int minDataValue = records.Min(
                r => { return (int)r.Properties[AzureTableUploaderTest.DataProperty].Int32Value; });
            if (minDataValue != 0)
            {
                Utility.TraceSource.WriteError(
                    AzureTableUploaderTest.TraceType,
                    "Unexpected minimum data value for {0} in table {1} partition {2}. Expected min: {3}, Actual min: {4}.",
                    recordDescription,
                    table.Name,
                    partIdx,
                    0,
                    minDataValue);
                Verify.Fail("Unexpected maximum entity data value in partition");
            }
        }

        private void VerifyInstanceInformation(
                         CloudTable table,
                         IEnumerable<DynamicTableEntity> records,
                         int partIdx,
                         bool hasDeletionEventWithIdOnly)
        {
            // Verify that the number of instances is what we expect
            int expectedInstanceCount = InstancesForPartition(partIdx);
            EqualityComparerInstanceProperty instanceComparer = new EqualityComparerInstanceProperty();
            IEnumerable<DynamicTableEntity> distinctRecords = records.Distinct(instanceComparer);
            int numInstances = distinctRecords.Count();
            if (expectedInstanceCount != numInstances)
            {
                Utility.TraceSource.WriteError(
                    AzureTableUploaderTest.TraceType,
                    "Unexpected instance count for table {0} partition {1}. Expected instance count: {2}, Actual instance count: {3}.",
                    table.Name,
                    partIdx,
                    expectedInstanceCount,
                    numInstances);
                Verify.Fail("Unexpected instance count");
            }

            // Verify that the instance values are as expected
            long maxInstanceValue = records.Max(
                r => { return (long)r.Properties[AzureTableUploaderTest.DcaInstanceProperty].Int64Value; });
            if (maxInstanceValue != (numInstances - 1))
            {
                Utility.TraceSource.WriteError(
                    AzureTableUploaderTest.TraceType,
                    "Unexpected maximum instance value for table {0} partition {1}. Expected max: {2}, Actual max: {3}.",
                    table.Name,
                    partIdx,
                    (numInstances - 1),
                    maxInstanceValue);
                Verify.Fail("Unexpected maximum entity data value in partition");
            }
            long minInstanceValue = records.Min(
                r => { return (long)r.Properties[AzureTableUploaderTest.DcaInstanceProperty].Int64Value; });
            if (minInstanceValue != 0)
            {
                Utility.TraceSource.WriteError(
                    AzureTableUploaderTest.TraceType,
                    "Unexpected minimum instance value for table {0} partition {1}. Expected min: {2}, Actual min: {3}.",
                    table.Name,
                    partIdx,
                    0,
                    minInstanceValue);
                Verify.Fail("Unexpected maximum entity data value in partition");
            }

            // Verify that each instance has the expected number of events
            for (long i = 0; i <= maxInstanceValue; i++)
            {
                int eventsForInstance = records
                                        .Where(r => { return (i == (long)r.Properties[AzureTableUploaderTest.DcaInstanceProperty].Int64Value); })
                                        .Count();
                int expectedEventsForInstance = ((i != maxInstanceValue) || (false == hasDeletionEventWithIdOnly)) ? EventsPerInstance : (EventsPerInstance - 1);
                if (eventsForInstance != expectedEventsForInstance)
                {
                    Utility.TraceSource.WriteError(
                        AzureTableUploaderTest.TraceType,
                        "Unexpected event count for instance {0} in table {1} partition {2}. Expected count: {3}, Actual count: {4}.",
                        i,
                        table.Name,
                        partIdx,
                        expectedEventsForInstance,
                        eventsForInstance);
                    Verify.Fail("Unexpected event count for instance");
                }
            }
        }

        class EqualityComparerDataProperty : IEqualityComparer<DynamicTableEntity>
        {
            public int GetHashCode(DynamicTableEntity obj)
            {
                return obj.Properties[AzureTableUploaderTest.DataProperty].Int32Value.GetHashCode();
            }

            public bool Equals(DynamicTableEntity x, DynamicTableEntity y)
            {
                int xValue = (int) x.Properties[AzureTableUploaderTest.DataProperty].Int32Value;
                int yValue = (int) y.Properties[AzureTableUploaderTest.DataProperty].Int32Value;
                return (xValue == yValue);
            }
        }

        class EqualityComparerInstanceProperty : IEqualityComparer<DynamicTableEntity>
        {
            public int GetHashCode(DynamicTableEntity obj)
            {
                return obj.Properties[AzureTableUploaderTest.DcaInstanceProperty].Int64Value.GetHashCode();
            }

            public bool Equals(DynamicTableEntity x, DynamicTableEntity y)
            {
                long xValue = (long)x.Properties[AzureTableUploaderTest.DcaInstanceProperty].Int64Value;
                long yValue = (long)y.Properties[AzureTableUploaderTest.DcaInstanceProperty].Int64Value;
                return (xValue == yValue);
            }
        }

        internal bool VerifyDeletion(CloudTableClient tableClient, DateTime deletionCutoffTimeLow, DateTime deletionCutoffTimeHigh)
        {
            foreach (string tableName in TableNames)
            {
                if (false == VerifyDeletionForTable(
                                tableClient,
                                tableName,
                                deletionCutoffTimeLow,
                                deletionCutoffTimeHigh))
                {
                    return false;
                }
            }
            return true;
        }

        private bool VerifyDeletionForTable(
                        CloudTableClient tableClient, 
                        string tableName, 
                        DateTime deletionCutoffTimeLow, 
                        DateTime deletionCutoffTimeHigh)
        {
            CloudTable table = tableClient.GetTableReference(String.Concat(AzureTableUploaderTest.TableNamePrefix, tableName));

            // Verify deletion of old entities from each partition
            for (int i=0; i < PartitionCount; i++)
            {
                if (false == VerifyDeletionForPartition(
                                tableClient,
                                table,
                                tableName,
                                i,
                                deletionCutoffTimeLow,
                                deletionCutoffTimeHigh))
                {
                    return false;
                }
            }

            // Verify that the entities we expect to be present have not been deleted
            int expectedEntityCount = this.eventTimestamps[tableName]
                                         .Where(t => { return (t.CompareTo(deletionCutoffTimeHigh) > 0); })
                                         .Count();
            string cutoffTimeTicks = (DateTime.MaxValue.Ticks - deletionCutoffTimeHigh.Ticks).ToString("D20", CultureInfo.InvariantCulture);
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

        private bool VerifyDeletionForPartition(
                        CloudTableClient tableClient, 
                        CloudTable table,
                        string tableName, 
                        int partIdx, 
                        DateTime deletionCutoffTimeLow, 
                        DateTime deletionCutoffTimeHigh)
        {
            // Get the entities that are older than the deletion cutoff time
            string cutoffTimeTicks = (DateTime.MaxValue.Ticks - deletionCutoffTimeLow.Ticks).ToString("D20", CultureInfo.InvariantCulture);
            TableQuery<DynamicTableEntity> oldEntityQuery = (new TableQuery<DynamicTableEntity>())
                                                             .Where(TableQuery.CombineFilters(
                                                                        TableQuery.GenerateFilterCondition(
                                                                            "PartitionKey",
                                                                            QueryComparisons.Equal,
                                                                            partIdx.ToString()),
                                                                        TableOperators.And,
                                                                        TableQuery.CombineFilters(
                                                                            TableQuery.GenerateFilterCondition(
                                                                                "RowKey",
                                                                                QueryComparisons.GreaterThan,
                                                                                cutoffTimeTicks), 
                                                                            TableOperators.And, 
                                                                            TableQuery.GenerateFilterCondition(
                                                                                "RowKey",
                                                                                QueryComparisons.LessThan,
                                                                                AzureTableQueryableEventUploader.StatusEntityRowKey))));
            IEnumerable<DynamicTableEntity> oldEntities = table.ExecuteQuery(oldEntityQuery);
            if (PartitionShouldBeDeleted(partIdx))
            {
                // Since the object representing this partition should've been deleted,
                // we should expect to see any entities that are older than the deletion
                // cutoff time.
                if (oldEntities.Count() > 0)
                {
                    // Don't fail the test yet. Just write a warning and return. We can 
                    // only predict the approximate deletion time, so maybe we are checking
                    // too early. The caller will check again later.
                    Utility.TraceSource.WriteWarning(
                        AzureTableUploaderTest.TraceType,
                        "All old entities have not yet been deleted from table {0}, partition {1}. We expect all entities with tick count less than {2} (RowKey greater than {3}) to be deleted.",
                        partIdx,
                        table.Name,
                        deletionCutoffTimeLow.Ticks,
                        cutoffTimeTicks);
                    return false;
                }
            }
            else
            {
                int oldEntityCount = oldEntities.Count();
                // We allow up to one old entity that is not associated with the current instance to 
                // be present. This is because we expect the DCA to not delete the newest old event 
                // that is old enough to be deleted. We expect the DCA to retain this event so that 
                // the table contains full history for the retention period. 
                // However, we cannot enforce a strict check for exactly one old entity to be present
                // because in the test we can only predict the approximate cutoff time for deletion. 
                // Therefore, we cannot predict which side of the cutoff time the old entity retained
                // by DCA will fall on. Depending on timing, it could end up on either side, so we 
                // relax the check to allow either 0 or 1 old entity that is not associated with the
                // current instance.
                if (oldEntityCount > 1)
                {
                    // Since the object representing this partition should not have been deleted,
                    // we expect that the entities associated with the latest instance of this 
                    // object to be present, even if they are older than the deletion cutoff time.
                    if (oldEntityCount < EventsPerInstance)
                    {
                        // Fail the test immediately. Some entities have been unexpectedly deleted.
                        Utility.TraceSource.WriteError(
                            AzureTableUploaderTest.TraceType,
                            "Some entities have been unexpectedly deleted from table {0}. We expected to find at least {1} entities with tick count less than {2} (RowKey greater than {3}), but found only {4}.",
                            table.Name,
                            EventsPerInstance,
                            deletionCutoffTimeLow.Ticks,
                            cutoffTimeTicks,
                            oldEntityCount);
                        Verify.Fail("Entities unexpectedly deleted from table.");
                    }

                    // Make sure that the old entities we found are associated with the latest
                    // instance of the object.

                    // Get the latest instance from the status record
                    TableOperation queryStatusOperation = TableOperation.Retrieve<DynamicTableEntity>(
                                                              partIdx.ToString(),
                                                              AzureTableQueryableEventUploader.StatusEntityRowKey);
                    TableResult queryResult = table.Execute(queryStatusOperation);
                    DynamicTableEntity statusRecord = queryResult.Result as DynamicTableEntity;
                    long latestInstance = (long)statusRecord.Properties["Instance"].Int64Value;

                    // Verify that the instance on the old entity matches the latest instance
                    foreach (DynamicTableEntity oldEntity in oldEntities)
                    {
                        long oldEntityInstance = (long)oldEntity.Properties[AzureTableUploaderTest.DcaInstanceProperty].Int64Value;
                        if (oldEntityInstance != latestInstance)
                        {
                            // We found an old entity that is not associated with the 
                            // latest instance of the object. However, don't fail
                            // the test yet. Just write a warning and return. We can 
                            // only predict the approximate deletion time, so maybe we 
                            // are checking too early. The caller will check again later.
                            Utility.TraceSource.WriteWarning(
                                AzureTableUploaderTest.TraceType,
                                "Found an old entity in table {0}, partition {1} that is not associated with the latest instance of the object. Entity instance: {2}, latest instance: {3}.",
                                table.Name,
                                partIdx,
                                oldEntityInstance,
                                latestInstance);
                            return false;
                        }
                    }
                }
            }

            return true;
        }
    }
}
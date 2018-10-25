// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Linq;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Reflection;
    using System.Text.RegularExpressions;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Table;

    internal class AzureTableQueryableCsvUploader : IDcaConsumer, ICsvFileSink
    {
        // Constants
        private const string eventConfigFileName = "TableEvents.config";
        private const string eventDataTypeConfigFileName = "table.json";
        private const string IdFieldName = "S_id";
        private const string DeletionEventIndicator = "DEL";
        private const string VersionFieldName = "I_dca_version";
        private const string InstanceFieldName = "l_dca_instance";
        private const string TaskNameProperty = "TaskName";
        private const string EventTypeProperty = "EventType";
        internal const string StatusEntityRowKey = "FFFFFFFFFFFFFFFFFFFF";
        private const string OldLogDeletionTimerIdSuffix = "_OldEntityDeletionTimer";
        private const string DeploymentIdTableShortName = "DeploymentIds";
        private const int MaxAzureRuntimeDeploymentIDLength = 16;
        private const int ReverseChronologicalRowKeyFlag = unchecked((int) 0x80000000);

        // For some of the events, the field to be used as the partition key 
        // can contain characters that are not allowed in Azure partition keys.
        // For such events, we replace the disallowed character with an alternate.
        private static readonly Dictionary<string, CharacterReplacement> CharReplaceMap = new Dictionary<string, CharacterReplacement>()
        {
           { "Services", new CharacterReplacement() {Original = '/', Substitute = '$'} },
           { "Applications", new CharacterReplacement() {Original = '/', Substitute = '$'} },
        };

        // Settings for uploading data to the table store.
        private TableUploadSettings tableUploadSettings;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Consumer initialization parameters
        private ConsumerInitializationParameters initParam;

        // Configuration reader
        private readonly ConfigReader configReader;

        // Object that implements miscellaneous utility functions
        private readonly AzureUtility azureUtility;

        // The deployment ID that we use as part of the table name
        private string effectiveDeploymentId;

        // Name of the deployment ID table
        private string deploymentIdTableName;
        
        // Information that helps determine whether or not we 
        // should delete old logs
        //
        // We do not want the DCA on every node in the cluster
        // to be deleting old logs from tables. So we make the
        // DCA on the FMM node do it. 
        //
        // This is only an optimization to prevent all nodes from 
        // duplicating the deletion work. So it doesn't have to be 
        // 100% accurate. If the FMM were to move from one node to
        // another, it is possible that for a particular deletion
        // pass, both nodes attempt the deletion because they both
        // think that they are the FMM node. But this should be a 
        // transient condition and eventually one of the nodes would
        // "realize" that it is no longer the FMM node.
        private DateTime fmmEventTimestampSnapshot;

        // Timer to delete old logs
        private readonly DcaTimer oldLogDeletionTimer;

        // Whether or not we are in the process of stopping
        private bool stopping;

        // Whether or not the object has been disposed
        private bool disposed;

        // Table config file helper
        private CsvConfigFileHelper csvConfigFileHelper;

        // Data type Helper
        private CsvDataTypesHelper csvDataTypesHelper;

        public AzureTableQueryableCsvUploader(ConsumerInitializationParameters initParam)
        {
            // Initialization
            this.stopping = false;
            this.initParam = initParam;
            this.logSourceId = String.Concat(this.initParam.ApplicationInstanceId, "_", this.initParam.SectionName);
            this.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            this.configReader = new ConfigReader(initParam.ApplicationInstanceId);
            this.azureUtility = new AzureUtility(this.traceSource, this.logSourceId);
            this.effectiveDeploymentId = String.Empty;
            this.fmmEventTimestampSnapshot = DateTime.MinValue;

            // Read table-specific settings from settings.xml
            GetSettings();
            if (false == this.tableUploadSettings.Enabled)
            {
                // Upload to Azure table storage is not enabled, so return immediately
                return;
            }

            // Initialize the data types helper
            this.csvDataTypesHelper = new CsvDataTypesHelper();

            // Figure out the effective deployment id
            this.GetEffectiveDeploymentId();

            // Parse the Table config file
            this.csvConfigFileHelper = new CsvConfigFileHelper(
                eventConfigFileName,
                this.logSourceId,
                this.traceSource,
                this.effectiveDeploymentId,
                this.tableUploadSettings.TableNamePrefix);

            // Get the table names
            this.GetTableNames();

            // Create the tables
            try
            {
                CreateTables();
            }
            catch (Exception e)
            {
                const string Message =
                    "Due to an error in table creation Ltt traces will not be uploaded to Azure table storage.";
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    Message);
                throw new InvalidOperationException(Message, e);
            }

            // Create a timer to delete old logs
            string timerId = String.Concat(
                                    this.logSourceId,
                                    OldLogDeletionTimerIdSuffix);
            this.oldLogDeletionTimer = new DcaTimer(
                timerId,
                this.DeleteOldLogsHandler,
                (this.tableUploadSettings.EntityDeletionAge < TimeSpan.FromDays(1)) ?
                AzureConstants.OldLogDeletionIntervalForTest :
                AzureConstants.OldLogDeletionInterval);
            this.oldLogDeletionTimer.Start();
        }

        public void FlushData()
        {
        }
        
        private void GetEffectiveDeploymentId()
        {
            if (false == String.IsNullOrEmpty(this.tableUploadSettings.DeploymentId))
            {
                // Deployment ID was explicitly specified in the cluster manifest. That's
                // the one we'll use.
                this.effectiveDeploymentId = this.tableUploadSettings.DeploymentId;
            }
            else
            {
                if (AzureUtility.IsAzureInterfaceAvailable())
                {
                    if (this.tableUploadSettings.TableNamePrefix.Length > AzureConstants.MaxTableNamePrefixLengthForDeploymentIdInclusion)
                    {
                        // The table name prefix specified by the user is too long to accomodate
                        // the deployment ID.
                        this.effectiveDeploymentId = String.Empty;
                    }
                    else
                    {
                        // Effective deployment ID is the deployment ID that we obtain from
                        // the Azure runtime interface.

                        // If it is too long, we truncate it. Azure deployment IDs seem to be
                        // random enough that this should almost always be good enough.
                        this.effectiveDeploymentId = AzureUtility.DeploymentId.Replace("-", "");
                        if (this.effectiveDeploymentId.Length > MaxAzureRuntimeDeploymentIDLength)
                        {
                            this.effectiveDeploymentId = this.effectiveDeploymentId.Remove(MaxAzureRuntimeDeploymentIDLength);
                        }

                        // The deployment ID on the Azure compute emulator has brackets, which
                        // are not valid characters in Azure table names. So let's replace the 
                        // opening and closing brackets with the string "OB" and "CB".
                        this.effectiveDeploymentId = this.effectiveDeploymentId.Replace("(", "OB");
                        this.effectiveDeploymentId = this.effectiveDeploymentId.Replace(")", "CB");
                    }
                }
                else
                {
                    // Azure interface is not available, so we cannot get the deployment ID
                    // from the Azure runtime. So the effective deployment ID is an empty string.
                    this.effectiveDeploymentId = String.Empty;
                }
            }
        }

        private void GetTableNames()
        {
            // Figure out the name of the deployment ID table
            this.deploymentIdTableName = String.Concat(
                                             this.tableUploadSettings.TableNamePrefix,
                                             DeploymentIdTableShortName);
        }

        public object GetDataSink()
        {
            // Return a sink if we are enabled, otherwise return null.
            return (this.tableUploadSettings.Enabled) ? ((ICsvFileSink) this) : null;
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }
            this.disposed = true;

            // Keep track of the fact that the consumer is stopping
            this.stopping = true;

            if (null != this.oldLogDeletionTimer)
            {
                // Stop and dispose timers
                this.oldLogDeletionTimer.StopAndDispose();

                // Wait for timer callbacks to finish executing
                this.oldLogDeletionTimer.DisposedEvent.WaitOne();
            }

            GC.SuppressFinalize(this);
            return;
        }

        private bool VerifyVersionField(Match match, CsvEvent csvEvent, out bool hasVersionField)
        {
            hasVersionField = false;
            try
            {
                var version = match.Groups[VersionFieldName].Value;

                if (string.IsNullOrEmpty(version))
                {
                    return true;
                }

                CsvDataTypesHelper.FieldType type;
                var parsedValue = this.csvDataTypesHelper.GetValueFromIdentifier(VersionFieldName[0], version, out type);

                if (type != CsvDataTypesHelper.FieldType.Int32)
                {
                    throw new ArgumentException();
                }
                hasVersionField = true;
            }
            catch
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "The field named {0} in Event of type {1}.{2} should be a 64-bit unsigned integer.",
                    InstanceFieldName,
                    csvEvent.TaskName,
                    csvEvent.EventName);

                return false;
            }

            return true;
        }

        private bool VerifyInstanceField(Match match, CsvEvent csvEvent)
        {
            try
            {
                var instance = match.Groups[InstanceFieldName].Value;

                if (string.IsNullOrEmpty(instance))
                {
                    return true;
                }

                CsvDataTypesHelper.FieldType type;
                var parsedValue = this.csvDataTypesHelper.GetValueFromIdentifier(InstanceFieldName[0], instance, out type);

                if (type != CsvDataTypesHelper.FieldType.UInt64)
                {
                    throw new ArgumentException();
                }

                return true;
            }
            catch
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "The field named {0} in Event of type {1}.{2} should be a 64-bit unsigned integer.",
                    InstanceFieldName,
                    csvEvent.TaskName,
                    csvEvent.EventName);

                return false;
            }
        }

        private DynamicTableEntity ConvertEventToEntity(CsvEvent csvEvent, CsvConfigFileHelper.EventInfo eventInfo)
        {
            Match match = eventInfo.Regex.Match(csvEvent.EventText);

            // Figure out how many fields the event has
            int fieldCount = match.Groups.Count;
            if (0 == fieldCount)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "No fields found in event of type {0}.{1}.",
                    csvEvent.TaskName,
                    csvEvent.EventName);
                return null;
            }

            // If the event has a version field, verify that it is a 32-bit value
            bool hasVersionField = false;
            if (false == VerifyVersionField(match, csvEvent, out hasVersionField))
            {
                return null;
            }

            // If the event has an instance field, verify that it is a 64-bit unsigned int
            if (false == VerifyInstanceField(match, csvEvent))
            {
                return null;
            }

            // Determine if the "id" field of the event is to be used as the partition key
            bool idFieldIsPartitionKey = false;
            //if (!string.IsNullOrEmpty(match.Groups[IdFieldName].Value))
            if (!string.IsNullOrEmpty(csvEvent.EventNameId))
                {
                // If the first field of the event is named "id", then use the value of that
                // field as the partition key.
                idFieldIsPartitionKey = true;
            }

            // Get the field names and values
            // If the conversion for any value fails, throw which will log it
            // in traces as error and avoid uploading it.
            DynamicTableEntity tableEntity = new DynamicTableEntity();
            try
            {
                var groups = eventInfo.Regex.GetGroupNames();
                for (int i=1; i<groups.Length; i++)
                {
                    var group = groups[i];

                    var typeValue = match.Groups[group].Value;
                    CsvDataTypesHelper.FieldType type;

                    var parsedValue = this.csvDataTypesHelper.GetValueFromIdentifier(group[0], typeValue, out type);
                    
                    EntityProperty property = null;
                    switch (type)
                    {

                        case CsvDataTypesHelper.FieldType.AnsiString:
                            {
                                property = new EntityProperty(typeValue);
                                break;
                            }

                        case CsvDataTypesHelper.FieldType.Boolean:
                            {
                                property = new EntityProperty(bool.Parse(typeValue));
                                break;
                            }

                        case CsvDataTypesHelper.FieldType.DateTime:
                            {
                                if (String.Compare(typeValue, "Infinite") == 0)
                                {
                                    property = new EntityProperty(DateTime.MaxValue);
                                }
                                else if (String.Compare(typeValue, "0") == 0)
                                {
                                    property = new EntityProperty(new DateTime(0));
                                }
                                else
                                {
                                    property = new EntityProperty(DateTime.Parse(typeValue));
                                }
                                break;
                            }

                        case CsvDataTypesHelper.FieldType.Double:
                            {
                                property = new EntityProperty(double.Parse(typeValue, NumberStyles.Any, null));
                                break;
                            }

                        case CsvDataTypesHelper.FieldType.Float:
                            {
                                property = new EntityProperty(double.Parse(typeValue, NumberStyles.Any, null));
                                break;
                            }

                        case CsvDataTypesHelper.FieldType.Guid:
                            {
                                property = new EntityProperty(Guid.Parse(typeValue));
                                break;
                            }

                        case CsvDataTypesHelper.FieldType.HexInt32:
                            {
                                property = new EntityProperty(UInt32.Parse(typeValue, NumberStyles.HexNumber, null));
                                break;
                            }

                        case CsvDataTypesHelper.FieldType.HexInt64:
                            {
                                property = new EntityProperty(UInt64.Parse(typeValue, NumberStyles.HexNumber, null));
                                break;
                            }

                        case CsvDataTypesHelper.FieldType.Int8:
                            {
                                property = new EntityProperty(char.Parse(typeValue));
                                break;
                            }

                        case CsvDataTypesHelper.FieldType.Int16:
                            {
                                property = new EntityProperty(Int16.Parse(typeValue));
                                break;
                            }

                        case CsvDataTypesHelper.FieldType.Int32:
                            {
                                property = new EntityProperty(Int32.Parse(typeValue, NumberStyles.Any, null));
                                break;
                            }

                        case CsvDataTypesHelper.FieldType.Int64:
                            {
                                property = new EntityProperty(Int64.Parse(typeValue, NumberStyles.Any, null));
                                break;
                            }

                        case CsvDataTypesHelper.FieldType.TimeSpan:
                            {
                                if (String.Compare(typeValue, "Infinite") == 0)
                                {
                                    property = new EntityProperty(long.MaxValue);
                                }
                                else if (String.Compare(typeValue, "-Infinite") == 0)
                                {
                                    property = new EntityProperty(long.MinValue);
                                }
                                else
                                {
                                    property = new EntityProperty(long.Parse(typeValue));
                                }
                                break;
                            }

                        case CsvDataTypesHelper.FieldType.UInt8:
                            {
                                property = new EntityProperty(int.Parse(typeValue));
                                break;
                            }

                        case CsvDataTypesHelper.FieldType.UInt16:
                            {
                                property = new EntityProperty(int.Parse(typeValue));
                                break;
                            }

                        case CsvDataTypesHelper.FieldType.UInt32:
                            {
                                property = new EntityProperty(UInt32.Parse(typeValue, NumberStyles.Any, null));
                                break;
                            }

                        case CsvDataTypesHelper.FieldType.UInt64:
                            {
                                property = new EntityProperty(long.Parse(typeValue));
                                break;
                            }

                        case CsvDataTypesHelper.FieldType.UnicodeString:
                            {
                                property = new EntityProperty(typeValue);
                                break;
                            }

                        default:
                            {
                                this.traceSource.WriteError(
                                    logSourceId,
                                    "Event of type {0} has an unsupported field of type {1} value {2}.",
                                    csvEvent.EventType,
                                    type,
                                    typeValue);
                                break;
                            }
                    }
                    if (null != property)
                    {
                        // The '.' character is not allowed in property names. However, the Windows Fabric tracing
                        // infrastructure uses the '.' character in the ETW event field names for encapsulated
                        // ETW records (i.e. ones that use AddField/FillEventData). Therefore, we replace the
                        // '.' character with the '_' character.
                        string propertyName = group.Substring(2).Replace(".", "_");
                        tableEntity.Properties[propertyName] = property;
                    }
                }
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    logSourceId,
                    e,
                    "Failed to get all the fields of event {0} with regex {1}",
                    csvEvent.EventText,
                    eventInfo.Regex);
                return null;
            }
            
            tableEntity.Properties[TaskNameProperty] = new EntityProperty(csvEvent.TaskName);
            tableEntity.Properties[EventTypeProperty] = new EntityProperty(csvEvent.EventName);

            // If the event does not have a version field, supply a default version 0
            if (false == hasVersionField)
            {
                tableEntity.Properties[VersionFieldName.Substring(2)] = new EntityProperty(ReverseChronologicalRowKeyFlag);
            }
            else
            {
                tableEntity.Properties[VersionFieldName.Substring(2)].Int32Value = tableEntity.Properties[VersionFieldName].Int32Value | ReverseChronologicalRowKeyFlag;
            }

            // Initialize the partition key
            if (idFieldIsPartitionKey)
            {
                // Use the "id" field as the partition key and remove it from the
                // set of properties because we don't want another column for it in
                // the table.
                //tableEntity.PartitionKey = ConvertEntityPropertyToString(tableEntity.Properties[IdFieldName]);
                tableEntity.PartitionKey = csvEvent.EventNameId;
                //tableEntity.Properties.Remove(IdFieldName);
            }
            else
            {
                // Use <TaskName>.<EventType> as the partition key
                tableEntity.PartitionKey = String.Concat(
                                                csvEvent.TaskName,
                                                ".",
                                                csvEvent.EventName);
            }

            // Initialize the row key
            DateTime eventTimestamp = csvEvent.Timestamp.ToUniversalTime();
            long rowKeyPrefix = DateTime.MaxValue.Ticks - eventTimestamp.Ticks;
            tableEntity.RowKey = String.Concat(
                                    rowKeyPrefix.ToString("D20", CultureInfo.InvariantCulture),
                                    "_",
                                    this.initParam.FabricNodeId,
                                    "_",
                                    //eventRecord.BufferContext.ProcessorNumber.ToString(),
                                    "0",
                                    "_",
                                    csvEvent.ProcessId.ToString(),
                                    "_",
                                    csvEvent.ThreadId.ToString());
            return tableEntity;
        }

        private bool GetExistingStatusEntity(CloudTable table, DynamicTableEntity entity, out StatusEntity statusEntity)
        {
            statusEntity = null;

            // The row key of the status record is a special value that will 
            // not match any of the row keys of the non-status records.
            TableOperation queryStatusOperation = TableOperation.Retrieve<StatusEntity>(entity.PartitionKey, StatusEntityRowKey);
            TableResult queryResult;
            try
            {
                queryResult = table.ExecuteAsync(queryStatusOperation).Result;
            }
            catch (Exception e)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Failed to query status record from table {0}. Partition key: {1}, row key: {2}. Exception: {3}",
                    table.Name,
                    entity.PartitionKey,
                    entity.RowKey,
                    e);
                return false;
            }

            if ((queryResult.HttpStatusCode < AzureTableCommon.HttpSuccessCodeMin) ||
                (queryResult.HttpStatusCode > AzureTableCommon.HttpSuccessCodeMax))
            {
                if (queryResult.HttpStatusCode == AzureTableCommon.HttpCodeResourceNotFound)
                {
                    // Status record not present. This is not an error.
                    return true;
                }
                else
                {
                    // Error occurred while retrieving status record
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Failed to query status record from table {0}. Partition key: {1}, row key: {2}. HTTP status code {3}.",
                        table.Name,
                        entity.PartitionKey,
                        entity.RowKey,
                        queryResult.HttpStatusCode);
                    return false;
                }
            }

            statusEntity = queryResult.Result as StatusEntity;
            return true;
        }


        public void OnCsvEventProcessingPeriodStart()
        {
        }

        public void OnCsvEventProcessingPeriodStop()
        {
        }

        public void OnCsvEventsAvailable(IEnumerable<CsvEvent> csvEvents)
        {
            foreach (var csvEvent in csvEvents)
            {
                if (this.stopping)
                {
                    // The consumer is being stopped. As far as possible, we don't want 
                    // the saving of changes to be interrupted midway due to any timeout 
                    // that may be enforced on consumer stop. Therefore, let's not save 
                    // the entities at this time. 
                    //
                    // Note that if a consumer stop is initiated while an ETL file is being 
                    // processed, that file gets processed again when the consumer is restarted.
                    // So we should get another opportunity to upload events from this ETL file 
                    // when we are restarted.
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "The consumer is being stopped. Therefore, traces will not be sent for upload to table storage.");
                    break;
                }

                CsvConfigFileHelper.EventInfo eventInfo = this.csvConfigFileHelper.GetEventInfo(csvEvent.EventTypeWithoutId);

                if (eventInfo == null)
                {
                    this.traceSource.WriteWarning(
                        this.logSourceId,
                        "Couldn't get event details for {0}. Skipping event upload to azure table.",
                        csvEvent.EventTypeWithoutId);
                    continue;
                }

                var storageAccount = this.tableUploadSettings.StorageAccountFactory.GetStorageAccount();
                Debug.Assert(null != storageAccount);

                var tableClientForUpload = AzureTableCommon.CreateNewTableClient(storageAccount);

                // Get a reference to the table that the event needs to go to
                CloudTable table = tableClientForUpload.GetTableReference(eventInfo.TableInfo);

                // Create an entity from this event.
                DynamicTableEntity entity = ConvertEventToEntity(csvEvent, eventInfo);
                if (null == entity)
                {
                    // Failed to convert event to entity
                    continue;
                }

                // If the partition key might contains some characters that are not allowed for
                // Azure partition keys, replace it with an alternate.
                if (CharReplaceMap.ContainsKey(eventInfo.ShortTableInfo))
                {
                    CharacterReplacement replacement = CharReplaceMap[eventInfo.ShortTableInfo];
                    entity.PartitionKey = entity.PartitionKey.Replace(replacement.Original, replacement.Substitute);
                }

                // Push the entity to the table
                WriteEntityToTable(table, entity, eventInfo.IsDeleteEvent, csvEvent);
            }
        }

        private void WriteEntityToTable(
                CloudTable table,
                DynamicTableEntity entity,
                bool entityIsDeletionEvent,
                CsvEvent csvEvent)
        {
            bool statusEntityConflict;
            do
            {
                statusEntityConflict = false;

                // Get the status record
                TableEntity statusEntity;
                bool isNewStatusEntity;
                if (false == GetStatusEntity(table, entity, entityIsDeletionEvent, out statusEntity, out isNewStatusEntity))
                {
                    // Error occurred while retrieving the status record. Skip this entity.
                    return;
                }

                // The batch can contain the following entities:
                // 1. [REQUIRED] The entity representing the ETW event that we're processing
                // 2. [OPTIONAL] The entity that creates or updates the status record for this partition
                TableBatchOperation operation = new TableBatchOperation();

                // Add the entity representing the ETW event that we're processing
                operation.InsertOrReplace(entity);

                if (null != statusEntity)
                {
                    // We need to create or update the status record
                    if (isNewStatusEntity)
                    {
                        // Create new status record
                        operation.Insert(statusEntity);
                    }
                    else
                    {
                        // Update existing status record
                        operation.Merge(statusEntity);
                    }
                }

                IList<TableResult> results = null;
                try
                {
                    results = table.ExecuteBatchAsync(operation).Result;
                }
                catch (Exception e)
                {
                    StorageException eSpecific = e as StorageException;
                    if (null != eSpecific)
                    {
                        // Check if the error was due to a conflicting update to the status record
                        //   #1 We thought we were inserting a new status entity into the table, but
                        //      someone else inserted the entity before we could.
                        //   #2 We were trying to update the existing status record, but someone else updated
                        //      or deleted it before we could. 
                        if ((isNewStatusEntity &&
                             (AzureTableCommon.HttpCodeEntityAlreadyExists == eSpecific.RequestInformation.HttpStatusCode)) // #1 above
                                ||
                            ((false == isNewStatusEntity) &&
                             ((AzureTableCommon.HttpCodePreconditionFailed == eSpecific.RequestInformation.HttpStatusCode) ||
                              (AzureTableCommon.HttpCodeResourceNotFound == eSpecific.RequestInformation.HttpStatusCode)))) // #2 above
                        {
                            statusEntityConflict = true;
                            continue;
                        }
                    }

                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Failed to upload event of type {0}.{1} to Azure table. Exception information: {2}",
                        csvEvent.TaskName,
                        csvEvent.EventName,
                        e);
                    return;
                }

                int i = 0;
                foreach (TableResult result in results)
                {
                    if ((result.HttpStatusCode < AzureTableCommon.HttpSuccessCodeMin) ||
                        (result.HttpStatusCode > AzureTableCommon.HttpSuccessCodeMax))
                    {
                        // Error occurred while uploading the entity corresponding to the ETW event
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Failed to upload event of type {0}.{1} to Azure table. HTTP status code {2} was returned while uploading entity representing {3}.",
                            csvEvent.TaskName,
                            csvEvent.EventName,
                            result.HttpStatusCode,
                            (i == 0) ? "ETW event" : "status record");

                        if (i != 0)
                        {
                            // Check if the error was due to a conflicting update to the status record
                            if (isNewStatusEntity)
                            {
                                if (AzureTableCommon.HttpCodeEntityAlreadyExists == result.HttpStatusCode)
                                {
                                    // We thought we were inserting a new status entity into the table, but
                                    // someone else inserted the entity before we could.
                                    statusEntityConflict = true;
                                }
                            }
                            else
                            {
                                if (AzureTableCommon.HttpCodePreconditionFailed == result.HttpStatusCode)
                                {
                                    // We were trying to update the existing status record, but someone else updated
                                    // it before we could. Hence the ETag on the status record was invalidated.
                                    statusEntityConflict = true;
                                }
                            }
                        }
                    }
                }
            }
            while (statusEntityConflict);
        }

        private bool GetStatusEntity(
                CloudTable table,
                DynamicTableEntity entity,
                bool entityIsDeletionEvent,
                out TableEntity statusEntity,
                out bool isNewStatusEntity)
        {
            statusEntity = null;
            isNewStatusEntity = false;

            // Check if the record representing the event contains an instance field
            long instance;
            if (false == entity.Properties.ContainsKey(InstanceFieldName))
            {
                // Instance field not present
                if (false == entityIsDeletionEvent)
                {
                    // Neither instance field is present nor is it a deletion event, so no 
                    // need for a status record.
                    return true;
                }
                else
                {
                    // It is a deletion event, so assign a dummy instance value that
                    // is large enough to always override the existing instance value in
                    // the status record.
                    unchecked
                    {
                        instance = (long)UInt64.MaxValue;
                    }
                }
            }
            else
            {
                // Instance field present
                instance = (long)entity.Properties[InstanceFieldName].Int64Value;
            }

            // If a status record already exists in the table, retrieve it
            StatusEntity existingStatusEntity;
            if (false == GetExistingStatusEntity(table, entity, out existingStatusEntity))
            {
                // Error occurred while trying to retrieve status record. Note that if the
                // status record is not present, that is NOT considered an error.
                return false;
            }

            if (null == existingStatusEntity)
            {
                // Status record doesn't exist yet. Let's create a new one.
                isNewStatusEntity = true;
                StatusEntity newStatusEntity = new StatusEntity();
                newStatusEntity.PartitionKey = entity.PartitionKey;
                newStatusEntity.RowKey = StatusEntityRowKey;

                newStatusEntity.Instance = instance;

                // If necessary, update the field that indicates deletion
                newStatusEntity.IsDeleted = entityIsDeletionEvent;

                statusEntity = newStatusEntity;
                return true;
            }
            else
            {
                UInt64 entityInstance;
                UInt64 existingStatusInstance;
                unchecked
                {
                    entityInstance = (UInt64)instance;
                    existingStatusInstance = (UInt64)existingStatusEntity.Instance;
                }
                if (entityInstance < existingStatusInstance)
                {
                    // This event does not describe the latest instance of the object.
                    // No need to update the status record in this case.
                    return true;
                }
                else if (entityInstance == existingStatusInstance)
                {
                    // This event describes the latest instance of the object. Check if
                    // we need to update the deletion status of the object.
                    if (entityIsDeletionEvent)
                    {
                        // Update the deletion status of this object
                        existingStatusEntity.IsDeleted = true;
                    }
                    else
                    {
                        // No need to update deletion status
                        return true;
                    }
                }
                else
                {
                    // We have a new instance of the object. We need to update the
                    // status record to reflect this.
                    existingStatusEntity.Instance = instance;
                    existingStatusEntity.IsDeleted = entityIsDeletionEvent;
                }
                statusEntity = existingStatusEntity;
                return true;
            }
        }

        private void GetSettings()
        {
            // Check for values in settings.xml
            this.tableUploadSettings.Enabled = this.configReader.GetUnencryptedConfigValue(
                                                   this.initParam.SectionName,
                                                   AzureConstants.EnabledParamName,
                                                   AzureConstants.TableUploadEnabledByDefault);
            if (this.tableUploadSettings.Enabled)
            {
                this.tableUploadSettings.StorageAccountFactory = this.azureUtility.GetStorageAccountFactory(
                                    this.configReader,
                                    this.initParam.SectionName,
                                    AzureConstants.ConnectionStringParamName);
                if (this.tableUploadSettings.StorageAccountFactory == null)
                {
                    this.tableUploadSettings.Enabled = false;
                    return;
                }

                this.tableUploadSettings.TableNamePrefix = this.configReader.GetUnencryptedConfigValue(
                                                         this.initParam.SectionName,
                                                         AzureConstants.EtwTraceTablePrefixParamName,
                                                         AzureConstants.DefaultTableNamePrefix);
                this.tableUploadSettings.DeploymentId = this.configReader.GetUnencryptedConfigValue(
                                                            this.initParam.SectionName,
                                                            AzureConstants.DeploymentId,
                                                            AzureConstants.DefaultDeploymentId);
                var entityDeletionAge = TimeSpan.FromDays(this.configReader.GetUnencryptedConfigValue(
                                                this.initParam.SectionName,
                                                AzureConstants.DataDeletionAgeParamName,
                                                AzureConstants.DefaultDataDeletionAge.TotalDays));
                if (entityDeletionAge > AzureConstants.MaxDataDeletionAge)
                {
                    this.traceSource.WriteWarning(
                        this.logSourceId,
                        "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                        entityDeletionAge,
                        this.initParam.SectionName,
                        AzureConstants.DataDeletionAgeParamName,
                        AzureConstants.MaxDataDeletionAge);
                    entityDeletionAge = AzureConstants.MaxDataDeletionAge;
                }
                this.tableUploadSettings.EntityDeletionAge = entityDeletionAge;

                // Check for test settings
                var logDeletionAgeTestValue = TimeSpan.FromMinutes(this.configReader.GetUnencryptedConfigValue(
                                                  this.initParam.SectionName,
                                                  AzureConstants.TestDataDeletionAgeParamName,
                                                  0));
                if (logDeletionAgeTestValue != TimeSpan.Zero)
                {
                    if (logDeletionAgeTestValue > AzureConstants.MaxDataDeletionAge)
                    {
                        this.traceSource.WriteWarning(
                            this.logSourceId,
                            "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                            logDeletionAgeTestValue,
                            this.initParam.SectionName,
                            AzureConstants.TestDataDeletionAgeParamName,
                            AzureConstants.MaxDataDeletionAge);
                        logDeletionAgeTestValue = AzureConstants.MaxDataDeletionAge;
                    }
                    this.tableUploadSettings.EntityDeletionAge = logDeletionAgeTestValue;
                }

                // Write settings to log
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Table storage upload enabled: ETW trace table prefix: {0}, Entity deletion age ({1}): {2}.",
                    this.tableUploadSettings.TableNamePrefix,
                    (logDeletionAgeTestValue == TimeSpan.Zero) ? "days" : "minutes",
                    (logDeletionAgeTestValue == TimeSpan.Zero) ? entityDeletionAge : logDeletionAgeTestValue);
            }
            else
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Table storage upload not enabled");
            }
        }

        private void CreateTablesWorker(object context)
        {
            CloudStorageAccount storageAccount = (CloudStorageAccount) context;

            // Create the table client object
            CloudTableClient cloudTableClient = AzureTableCommon.CreateNewTableClient(storageAccount);

            List<string> tablesToCreate = new List<string>(this.csvConfigFileHelper.tableNames);
            tablesToCreate.Add(this.deploymentIdTableName);
            foreach (string tableName in tablesToCreate)
            {
                // Create the table
                cloudTableClient.GetTableReference(tableName).CreateIfNotExistsAsync().Wait();
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Created table {0} in storage account {1}",
                    tableName,
                    this.tableUploadSettings.StorageAccountFactory.Connection.AccountName);
            }
        }

        private void AddEffectiveDeploymentIdToTable(object context)
        {
            if (String.IsNullOrEmpty(this.effectiveDeploymentId))
            {
                return;
            }

            // Construct the entity
            DeploymentTableEntity entity = new DeploymentTableEntity();
            entity.PartitionKey = this.effectiveDeploymentId;
            entity.RowKey = String.Empty;
            entity.StartTime = DateTime.UtcNow;

            // Construct the table operation
            TableOperation insertOperation = TableOperation.Insert(entity);

            CloudStorageAccount storageAccount = (CloudStorageAccount) context;

            // Create the table client object
            CloudTableClient cloudTableClient = AzureTableCommon.CreateNewTableClient(storageAccount);
            
            // Get the table reference
            CloudTable cloudTable = cloudTableClient.GetTableReference(this.deploymentIdTableName);

            // Add entity to the table
            try
            {
                cloudTable.ExecuteAsync(insertOperation).Wait();
            }
            catch (AggregateException ae)
            {
                ae.Handle((x) =>
                {
                    if (x is StorageException) // This we know how to handle.
                    {
                        // If the entity already exists in the table, we handle the exception and move on.
                        // It is not an error - it just means that we already added the deployment ID to
                        // the table.
                        StorageException se = x as StorageException;
                        if (AzureTableCommon.HttpCodeEntityAlreadyExists == se.RequestInformation.HttpStatusCode)
                        {
                            return true;
                        }
                    }
                    return false; // throw other exception
                });
            }
        }

        internal RetriableOperationExceptionHandler.Response TableOperationExceptionHandler(Exception e)
        {
            this.traceSource.WriteError(
                this.logSourceId,
                "Exception encountered when attempting to perform an operation on table {0} in storage account {1}. Exception information: {2}",
                this.tableUploadSettings.TableNamePrefix,
                this.tableUploadSettings.StorageAccountFactory.Connection.AccountName,
                e);
                
            if (e is StorageException)
            {
                // If this was a network error, we'll retry later. Else, we'll
                // just give up.
                StorageException eSpecific = (StorageException) e;
                if (Utility.IsNetworkError(eSpecific.RequestInformation.HttpStatusCode))
                {
                    return RetriableOperationExceptionHandler.Response.Retry;
                }
            }
            return RetriableOperationExceptionHandler.Response.Abort;
        }

        private void CreateTables()
        {
            CloudStorageAccount storageAccount = this.tableUploadSettings.StorageAccountFactory.GetStorageAccount();
            RetriableOperationExceptionHandler exceptionHandler = new RetriableOperationExceptionHandler(this.TableOperationExceptionHandler);
            Utility.PerformWithRetries(
                CreateTablesWorker,
                storageAccount,
                exceptionHandler);

            Utility.PerformWithRetries(
                AddEffectiveDeploymentIdToTable,
                storageAccount,
                exceptionHandler);
        }

        private bool ShouldDeleteOldLogs()
        {
            DateTime lastFMMEventTimestamp;
            try
            {
                // Get the timestamp of the last FMM event we saw
                lastFMMEventTimestamp = Utility.LastFmmEventTimestamp;
            }
            catch (NotImplementedException)
            {
                // Unable to determine the timestamp of the last FMM event
                this.traceSource.WriteWarning(
                    this.logSourceId,
                    "Unable to determine whether the current node is the FMM node. So multiple nodes will attempt deletion of old entities from Azure tables.");
                return true;
            }

            if (lastFMMEventTimestamp.CompareTo(this.fmmEventTimestampSnapshot) > 0)
            {
                // New FMM events were seen since the last time we checked. Therefore,
                // we're likely still on the FMM node. Hence we should attempt to delete
                // old entities.
                this.fmmEventTimestampSnapshot = lastFMMEventTimestamp;
                return true;
            }
            else
            {
                // No new FMM events were seen since the last time we checked. Therefore,
                // we're not on the FMM node and hence we should not attempt to delete
                // old entities.
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Not attempting deletion of old entities because the current node is not the FMM node.");
                return false;
            }
        }

        private void DeleteOldLogsHandler(object state)
        {
            if (false == ShouldDeleteOldLogs())
            {
                // Schedule the next pass
                this.oldLogDeletionTimer.Start();
                return;
            }

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Starting old entity deletion pass ...");

            var storageAccount = this.tableUploadSettings.StorageAccountFactory.GetStorageAccount();
            Debug.Assert(null != storageAccount);

            CloudTableClient tableClient = AzureTableCommon.CreateNewTableClient(storageAccount);

            // Figure out the timestamp before which all entities will be deleted
            DateTime cutoffTime = DateTime.UtcNow.Add(-this.tableUploadSettings.EntityDeletionAge);

            foreach (string tableName in this.csvConfigFileHelper.tableNames)
            {
                if (this.stopping)
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Deletion of old entities interrupted because the consumer is being stopped.");
                    break;
                }

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Starting old entity deletion pass for table {0} ...",
                    tableName);

                CloudTable table = tableClient.GetTableReference(tableName);

                // If we used status records to keep track of the table's partitions,
                // then perform deletion partition by partition.
                bool noStatusEntitiesFound;
                DeleteOldEntitiesByPartition(table, cutoffTime, out noStatusEntitiesFound);

                if (noStatusEntitiesFound)
                {
                    // We didn't find any status records, so just delete entites
                    // without regard to the partition to which they belong.
                    DeleteOldEntities(table, cutoffTime, null, null);
                }

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Completed old entity deletion pass for table {0}.",
                    tableName);
            }

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Completed old entity deletion pass.");

            // Schedule the next pass
            this.oldLogDeletionTimer.Start();
        }

        private void DeleteOldEntitiesByPartition(CloudTable table, DateTime cutoffTime, out bool noStatusEntitiesFound)
        {
            noStatusEntitiesFound = true;

            // Let's first get all the status records. This will tell us what
            // partitions exist in the table.
            TableQuery<StatusEntity> statusEntityQuery = (new TableQuery<StatusEntity>())
                                                         .Where(TableQuery.GenerateFilterCondition(
                                                                    "RowKey",
                                                                     QueryComparisons.Equal,
                                                                     StatusEntityRowKey));

            //IEnumerable<StatusEntity> statusEntities = table.ExecuteQuery<StatusEntity>(statusEntityQuery);
            try
            {
                List<StatusEntity> statusEntities = new List<StatusEntity>();
                TableContinuationToken tableContinuationToken = new TableContinuationToken();
                TableQuerySegment<StatusEntity> entitiesSegment = null;

                while (entitiesSegment == null)
                {
                    entitiesSegment = table.ExecuteQuerySegmentedAsync(statusEntityQuery, tableContinuationToken).Result;
                    statusEntities.AddRange(entitiesSegment);
                }

                foreach (StatusEntity statusEntity in statusEntities)
                {
                    if (this.stopping)
                    {
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Deletion of old entities from table {0} was interrupted because the consumer is being stopped.",
                            table.Name);
                        break;
                    }

                    // Delete old records for this partition
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Starting old entity deletion pass for table {0}, partition {1} ...",
                        table.Name,
                        statusEntity.PartitionKey);

                    noStatusEntitiesFound = false;
                    DeleteOldEntities(table, cutoffTime, statusEntity.PartitionKey, statusEntity);

                    // Check if the object corresponding to this partition has 
                    // already been deleted. If so, delete the status record too.
                    DeleteStatusEntityIfNeeded(table, statusEntity);

                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Completed old entity deletion pass for table {0}, partition {1}.",
                        table.Name,
                        statusEntity.PartitionKey);
                }
            }
            catch (Exception e)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Failed to query for status records from table {0}. Exception: {1}.",
                    table.Name,
                    e);
                return;
            }
        }

        private void DeleteOldEntities(CloudTable table, DateTime cutoffTime, string partitionKey, StatusEntity statusEntity)
        {
            TableQuery<DynamicTableEntity> oldEntityQuery;
            if (null != partitionKey)
            {
                // We query for entities that meet the following criteria:
                //  - partition key equals the partition key provided by the caller
                //  - event timestamp is old enough that the entity needs to be deleted
                oldEntityQuery = (new TableQuery<DynamicTableEntity>())
                                 .Where(TableQuery.CombineFilters(
                                            TableQuery.GenerateFilterCondition(
                                                "PartitionKey", 
                                                QueryComparisons.Equal, 
                                                partitionKey),
                                            TableOperators.And,
                                            this.CreateTimestampFilter(cutoffTime)));
            }
            else
            {
                // We query for entities that meet the following criteria:
                //  - event timestamp is old enough that the entity needs to be deleted
                oldEntityQuery = (new TableQuery<DynamicTableEntity>())
                                 .Where(this.CreateTimestampFilter(cutoffTime));
            }

            //IEnumerable<DynamicTableEntity> oldEntities = table.ExecuteQuery<DynamicTableEntity>(oldEntityQuery);
            try
            {
                List<DynamicTableEntity> oldEntities = new List<DynamicTableEntity>();
                TableContinuationToken tableContinuationToken = new TableContinuationToken();
                TableQuerySegment<DynamicTableEntity> entitiesSegment = null;

                while (entitiesSegment == null)
                {
                    entitiesSegment = table.ExecuteQuerySegmentedAsync(oldEntityQuery, tableContinuationToken).Result;
                    oldEntities.AddRange(entitiesSegment);
                }

                bool atLeastOneOldEntityPreserved = false;
                foreach (DynamicTableEntity oldEntity in oldEntities)
                {
                    if (this.stopping)
                    {
                        if (null != partitionKey)
                        {
                            this.traceSource.WriteInfo(
                                this.logSourceId,
                                "Deletion of old entities from table {0} partition {1} was interrupted because the consumer is being stopped.",
                                table.Name,
                                partitionKey);
                        }
                        else
                        {
                            this.traceSource.WriteInfo(
                                this.logSourceId,
                                "Deletion of old entities from table {0} was interrupted because the consumer is being stopped.",
                                table.Name);
                        }
                        break;
                    }

                    if ((null != statusEntity) &&
                        (false == statusEntity.IsDeleted))
                    {
                        if (oldEntity.Properties.ContainsKey(InstanceFieldName))
                        {
                            // We don't delete entities associated with the current
                            // instance of the object.
                            if (oldEntity.Properties[InstanceFieldName].Int64Value == statusEntity.Instance)
                            {
                                // Skip deletion of this entity.
                                atLeastOneOldEntityPreserved = true;
                                continue;
                            }
                        }

                        if ((false == atLeastOneOldEntityPreserved) &&
                            (oldEntity.Properties[VersionFieldName].Int32Value < 0))
                        {
                            // For the tables that we cleanup on a per-partition basis, we always retain one entity that meets the deletion
                            // criteria, so that the table contains full history about that partition for the retention period.
                            // For example, consider two entities with timestamps T1 and T3, where T1 < T3 and there are no other entites
                            // in between them. Let's say the retention policy is such that any entities with timestamp older than T2 should
                            // be deleted, where T1 < T2 < T3. In this example, the entity with timestamp T1 is eligible for deletion, but
                            // we won't delete it. This is because its presence will ensure that the table can be used to reconstruct the 
                            // history between T2 and T3.
                            // Note that we do this only for entities that have been written by v3.1 or later code. Entities written by v3.1
                            // and later are arranged in reverse chronological order. So when we query for old entities we receive them in
                            // reverse chronological order, which means that the newest old entity comes first and the oldest old entity comes
                            // last. Our goal is to skip the newest old entity, so we just need to skip the first old entity that was written
                            // by v3.1 or later code.
                            atLeastOneOldEntityPreserved = true;
                            continue;
                        }
                    }

                    DeleteEntity(table, oldEntity, new int[] { AzureTableCommon.HttpCodeResourceNotFound });
                }
            }
            catch (Exception e)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Failed to query for old entities from table {0}. Exception: {1}.",
                    table.Name,
                    e);
                return;
            }
        }

        private string CreateTimestampFilter(DateTime cutoffTime)
        {
            // Filter for records created by V3 and older code, where row keys were in chronological order
            string cutoffTimeTicks = cutoffTime.Ticks.ToString("D20", CultureInfo.InvariantCulture);
            string v3StyleFilter = TableQuery.CombineFilters(
                                        TableQuery.GenerateFilterCondition(
                                            "RowKey", 
                                            QueryComparisons.LessThan, 
                                            cutoffTimeTicks),
                                        TableOperators.And,
                                        TableQuery.GenerateFilterConditionForInt(
                                            VersionFieldName, 
                                            QueryComparisons.GreaterThanOrEqual, 
                                            0));

            // Filter for records created by the current code, where row keys are in reverse chronological order
            cutoffTimeTicks = (DateTime.MaxValue.Ticks - cutoffTime.Ticks).ToString("D20", CultureInfo.InvariantCulture);
            string filter = TableQuery.CombineFilters(
                                        TableQuery.GenerateFilterCondition(
                                            "RowKey",
                                            QueryComparisons.GreaterThan,
                                            cutoffTimeTicks),
                                        TableOperators.And,
                                        TableQuery.GenerateFilterConditionForInt(
                                            VersionFieldName,
                                            QueryComparisons.LessThan,
                                            0));

            return TableQuery.CombineFilters(
                        v3StyleFilter,
                        TableOperators.Or,
                        filter);
        }

        private void DeleteEntity(CloudTable table, ITableEntity entity, int[] errorCodesToIgnore)
        {
            TableOperation deleteOperation = TableOperation.Delete(entity);
            TableResult deleteResult;
            try
            {
                deleteResult = table.ExecuteAsync(deleteOperation).Result;
            }
            catch (Exception e)
            {
                StorageException eSpecific = e as StorageException;
                if (null != eSpecific)
                {
                    if (false == errorCodesToIgnore.Contains(eSpecific.RequestInformation.HttpStatusCode))
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Failed to delete entity with partition key {0}, row key {1} from table {2}. Exception: {3}.",
                            entity.PartitionKey,
                            entity.RowKey,
                            table.Name,
                            e);
                    }
                }
                return;
            }
            if ((deleteResult.HttpStatusCode < AzureTableCommon.HttpSuccessCodeMin) ||
                (deleteResult.HttpStatusCode > AzureTableCommon.HttpSuccessCodeMax))
            {
                // Error occurred while deleting entity.
                // If the error code is not something that the caller asked us to ignore,
                // then we log an error message.
                if (false == errorCodesToIgnore.Contains(deleteResult.HttpStatusCode))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Failed to delete entity with partition key {0}, row key {1} from table {2}. HTTP status code {3}.",
                        entity.PartitionKey,
                        entity.RowKey,
                        table.Name,
                        deleteResult.HttpStatusCode);
                }
            }
        }

        private void DeleteStatusEntityIfNeeded(CloudTable table, StatusEntity statusEntity)
        {
            // Check if the object corresponding to this partition has 
            // already been deleted
            if (false == statusEntity.IsDeleted)
            {
                // Object corresponding to this partition has not yet
                // been deleted. We cannot delete the status record yet.
                return;
            }

            // The object corresponding to this partition has already
            // been deleted.

            // Check if we have deleted all entities from this partition
            TableQuery<DynamicTableEntity> partitionEntityQuery = (new TableQuery<DynamicTableEntity>())
                                .Where(TableQuery.CombineFilters(
                                        TableQuery.GenerateFilterCondition(
                                            "PartitionKey",
                                            QueryComparisons.Equal,
                                            statusEntity.PartitionKey),
                                        TableOperators.And,
                                        TableQuery.GenerateFilterCondition(
                                            "RowKey",
                                            QueryComparisons.LessThan,
                                            StatusEntityRowKey)));
            //IEnumerable<DynamicTableEntity> partitionEntities = table.ExecuteQuery<DynamicTableEntity>(partitionEntityQuery);
            try
            {
                List<DynamicTableEntity> partitionEntities = new List<DynamicTableEntity>();
                TableContinuationToken tableContinuationToken = new TableContinuationToken();
                TableQuerySegment<DynamicTableEntity> entitiesSegment = null;

                while (entitiesSegment == null)
                {
                    entitiesSegment = table.ExecuteQuerySegmentedAsync(partitionEntityQuery, tableContinuationToken).Result;
                    partitionEntities.AddRange(entitiesSegment);
                }

                if (null == partitionEntities.FirstOrDefault())
                {
                    // We have deleted all entities from the partition. So
                    // we can also delete the status record now.
                    DeleteEntity(table, statusEntity, new int[] {AzureTableCommon.HttpCodePreconditionFailed});
                }
            }
            catch (Exception e)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Failed to determine whether table {0}, partition {1} has any entities other than the status record. Exception: {2}.",
                    table.Name,
                    statusEntity.PartitionKey,
                    e);
                return;
            }
        }

        // Settings related to upload of data to table storage
        private struct TableUploadSettings
        {
            internal bool Enabled;
            internal StorageAccountFactory StorageAccountFactory;
            internal string TableNamePrefix;
            internal string DeploymentId;
            internal TimeSpan EntityDeletionAge;
        }

        // Status record for a partition of the Azure table
        private class StatusEntity : TableEntity
        {
            public long Instance { get; set; }
            public bool IsDeleted { get; set; }
        }

        private class DeploymentTableEntity : TableEntity
        {
            public DateTime StartTime { get; set; }
        }

        private class CharacterReplacement
        {
            public char Original;
            public char Substitute;
        }
    }
}
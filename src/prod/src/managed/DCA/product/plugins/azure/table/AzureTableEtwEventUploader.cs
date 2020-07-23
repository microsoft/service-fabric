// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.IO;
    using System.Threading;
    using System.Linq;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Fabric.Dca.EtlConsumerHelper;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Table;

    // This class implements the logic to upload ETW traces to Azure table storage. 
    internal class AzureTableEtwEventUploader : IDcaConsumer, IBufferedEtwEventSink
    {
        // Constants
        private const string BufferedEventDirName = "Buf";
        private const string V2BufferedEventDirNamePrefix = "BufferedEvents_";

        // Default values for table-specific settings
        private static readonly TimeSpan DefaultTableUploadInterval = TimeSpan.FromMinutes(5);
        private const string defaultTableFilter = 
            "*.*:2, HealthReport.*:4, FabricNode.State:4, SiteNode.State:4, FM.Nodes:4," +
            "NamingStoreService.Open:4, NamingStoreService.ChangeRole:4, NamingStoreService.Close:4," +
            "CM.ReplicaOpen:4, CM.ReplicaChangeRole:4, CM.ReplicaClose:4, InfrastructureService.*:4";
        private const int defaultTableBatchUploadConcurrencyCount = 4;

        /// <summary>
        /// The maximum number of batches of entities that we can concurrently upload
        /// to the table store at a given time.
        /// </summary>
        private readonly int batchConcurrencyCount;

        /// <summary>
        /// Settings for uploading data to the table store.
        /// </summary>
        private TableUploadSettings tableUploadSettings;

        /// <summary>
        /// This is the azure node instance id.
        /// </summary>
        private readonly string azureNodeInstanceId;

        /// <summary>
        /// This is the index of the table service context object that is currently
        /// in use.
        /// </summary>
        private int contextIndex;

        /// <summary>
        /// This is the array of a list of batched up unsaved entities. Each element
        /// in this array corresponds to different a table service context object.
        /// </summary>
        private List<EtwEventEntity>[] unsavedEntities;

        /// <summary>
        /// This is the array of clients that are used to 
        /// upload entities to the table.
        /// </summary>
        private CloudTableClient[] tableClients;

        /// <summary>
        /// This is the array of handles we wait on until a table service context
        /// becomes available.
        /// </summary>
        private WaitHandle[] handles;

        /// <summary>
        /// This is the array of IAsyncResults for the batched operations that 
        /// we perform.
        /// </summary>
        private IAsyncResult[] asyncResult;

        /// <summary>
        /// List of available indexes in the table service context array.
        /// </summary>
        private Stack<int> availableIndexes;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Consumer initialization parameters
        ConsumerInitializationParameters initParam;

        // Configuration reader
        readonly ConfigReader configReader;

        // Object that deletes old entities from the table
        readonly AzureTableTrimmer trimmer;

        // Object that implements miscellaneous utility functions
        private readonly AzureUtility azureUtility;

        // Object that measures performance
        private readonly AzureTablePerformance perfHelper;

        // Buffered event provider interface reference
        readonly IBufferedEtwEventProvider bufferedEventProvider;

        // Whether or not we are in the process of stopping
        private bool stopping;

        // Whether or not the object has been disposed
        bool disposed;

        public AzureTableEtwEventUploader(ConsumerInitializationParameters initParam)
        {
            // Initialization
            this.stopping = false;
            this.initParam = initParam;
            this.logSourceId = String.Concat(this.initParam.ApplicationInstanceId, "_", this.initParam.SectionName);
            this.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            this.configReader = new ConfigReader(initParam.ApplicationInstanceId);
            this.azureUtility = new AzureUtility(this.traceSource, this.logSourceId);
            this.perfHelper = new AzureTablePerformance(this.traceSource, this.logSourceId);

            // Make sure that the Azure interfaces are available
            if (false == AzureUtility.IsAzureInterfaceAvailable())
            {
                const string Message = "Due to unavailability of Azure interfaces, ETW traces will not be uploaded to Azure table storage.";
                this.traceSource.WriteError(
                    this.logSourceId,
                    Message);
                throw new InvalidOperationException(Message);
            }

            this.azureNodeInstanceId = AzureUtility.RoleInstanceId;

            // Read table-specific settings from settings.xml
            GetSettings();
            if (false == this.tableUploadSettings.Enabled)
            {
                // Upload to Azure table storage is not enabled, so return immediately
                return;
            }

            // Create a sub-directory for ourselves under the log directory
            string bufferedEventFolder = GetBufferedEventSubDirectory();
            if (String.IsNullOrEmpty(bufferedEventFolder))
            {
                throw new InvalidOperationException("Unable to get buffered event subdirectory.");
            }

            // Create the helper object that buffers events delivered from the ETL
            // files into CSV files on disk. 
            this.bufferedEventProvider = new BufferedEtwEventProvider(
                new TraceEventSourceFactory(),
                this.logSourceId, 
                bufferedEventFolder, 
                this.tableUploadSettings.UploadIntervalMinutes, 
                this.tableUploadSettings.EntityDeletionAge, 
                this);
            if (null == this.bufferedEventProvider)
            {
                const string Message = "Failed to create buffered event provider helper object.";
                this.traceSource.WriteError(
                    this.logSourceId,
                    Message);
                throw new InvalidOperationException(Message);
            }

            // Set the filter for Windows Fabric events
            this.bufferedEventProvider.SetEtwEventFilter(
                this.tableUploadSettings.Filter,
                defaultTableFilter,
                WinFabSummaryFilter.StringRepresentation,
                true);

            // Initialize the batch upload concurrency count
            Debug.Assert(this.tableUploadSettings.BatchUploadConcurrencyCount <= AzureConstants.MaxBatchConcurrencyCount);
            if (this.tableUploadSettings.BatchUploadConcurrencyCount <= AzureConstants.MaxBatchConcurrencyCount)
            {
                this.batchConcurrencyCount = this.tableUploadSettings.BatchUploadConcurrencyCount;
            }
            else
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "{0} is an invalid value for table batch concurrency count. The maximum supported value is {1} and that value will be used instead.",
                    this.tableUploadSettings.BatchUploadConcurrencyCount,
                    AzureConstants.MaxBatchConcurrencyCount);
                this.batchConcurrencyCount = AzureConstants.MaxBatchConcurrencyCount;
            }

            this.perfHelper.ExternalOperationInitialize(
                ExternalOperationTime.ExternalOperationType.TableUpload,
                this.batchConcurrencyCount);

            // Create the table
            try
            {
                CreateTable();
            }
            catch (Exception e)
            {
                const string Message =
                    "Due to an error in table creation ETW traces will not be uploaded to Azure table storage.";
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    Message);
                throw new InvalidOperationException(Message, e);
            }

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Created table for uploading ETW traces. Storage account: {0}, Table name: {1}",
                this.tableUploadSettings.StorageAccountFactory.Connection.AccountName,
                this.tableUploadSettings.TableName);

            // Initialize old log deletion
            this.trimmer = new AzureTableTrimmer(
                this.traceSource,
                this.logSourceId,
                this.tableUploadSettings.StorageAccountFactory,
                this.tableUploadSettings.TableName,
                this.tableUploadSettings.EntityDeletionAge,
                this.CreateDeletionQuery,
                this.perfHelper);
        }

        public object GetDataSink()
        {
            // Return a sink if we are enabled, otherwise return null.
            return (this.tableUploadSettings.Enabled) ? ((IEtlFileSink) this.bufferedEventProvider) : null;
        }

        public void FlushData()
        {
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }
            this.disposed = true;

            if (null != this.bufferedEventProvider)
            {
                // Tell the buffered event provider to stop delivering events
                //
                // IMPORTANT: Invoke the buffered event provider's dispose callback 
                // before stopping ourselves. By doing so, we know that we will not
                // get any events from the buffered event provider while we are 
                // stopping.
                this.bufferedEventProvider.Dispose();
            }
            
            // Keep track of the fact that the consumer is stopping
            this.stopping = true;

            // Stop old log deletion
            if (null != this.trimmer)
            {
                this.trimmer.Dispose();
            }

            GC.SuppressFinalize(this);
        }

        private string GetBufferedEventSubDirectory()
        {
            string bufferedEventParentFolder = Path.Combine(this.initParam.WorkDirectory, Utility.ShortWindowsFabricIdForPaths);

            string destinationKey = String.Join("_", StandardPluginTypes.AzureTableEtwEventUploader, 
                this.tableUploadSettings.StorageAccountFactory.Connection.UseDevelopmentStorage 
                ? AzureConstants.DevelopmentStorageConnectionString 
                : this.tableUploadSettings.StorageAccountFactory.Connection.AccountName, this.tableUploadSettings.TableName);

            string bufferedEventFolder;
            bool success = Utility.CreateWorkSubDirectory(
                                this.traceSource,
                                this.logSourceId,
                                destinationKey,
                                String.Concat(Utility.EtwConsumerWorkSubFolderIdPrefix, this.logSourceId),
                                bufferedEventParentFolder,
                                out bufferedEventFolder);
            if (success)
            {
                bufferedEventFolder = Path.Combine(bufferedEventFolder, BufferedEventDirName);
                return bufferedEventFolder;
            } 

            return null;
        }

        private void GetSettings()
        {
            string dataDeletionAgeParamName = AzureConstants.DataDeletionAgeParamName;
            string dataDeletionAgeTestParamName = AzureConstants.TestDataDeletionAgeParamName;

            // Check for values in settings.xml
            this.tableUploadSettings.Enabled = this.configReader.GetUnencryptedConfigValue(
                                                   this.initParam.SectionName,
                                                   AzureConstants.EnabledParamName,
                                                   AzureConstants.TableUploadEnabledByDefault);
            if (this.tableUploadSettings.Enabled)
            {
                this.tableUploadSettings.StorageAccountFactory = 
                    this.azureUtility.GetStorageAccountFactory(this.configReader, this.initParam.SectionName, AzureConstants.ConnectionStringParamName);
                if (this.tableUploadSettings.StorageAccountFactory == null)
                {
                    this.tableUploadSettings.Enabled = false;
                    return;
                }

                this.tableUploadSettings.TableName = this.configReader.GetUnencryptedConfigValue(
                                                         this.initParam.SectionName,
                                                         AzureConstants.EtwTraceTableParamName,
                                                         AzureConstants.DefaultTableName);
                this.tableUploadSettings.UploadIntervalMinutes = TimeSpan.FromMinutes(this.configReader.GetUnencryptedConfigValue(
                                                                     this.initParam.SectionName,
                                                                     AzureConstants.UploadIntervalParamName,
                                                                     (int)DefaultTableUploadInterval.TotalMinutes));
                var entityDeletionAge = TimeSpan.FromDays(this.configReader.GetUnencryptedConfigValue(
                                                this.initParam.SectionName,
                                                dataDeletionAgeParamName,
                                                AzureConstants.DefaultDataDeletionAge.TotalDays));
                if (entityDeletionAge > AzureConstants.MaxDataDeletionAge)
                {
                    this.traceSource.WriteWarning(
                        this.logSourceId,
                        "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                        entityDeletionAge,
                        this.initParam.SectionName,
                        dataDeletionAgeParamName,
                        AzureConstants.MaxDataDeletionAge);
                    entityDeletionAge = AzureConstants.MaxDataDeletionAge;
                }
                this.tableUploadSettings.EntityDeletionAge = entityDeletionAge;

                this.tableUploadSettings.BatchUploadConcurrencyCount = this.configReader.GetUnencryptedConfigValue(
                                                                           this.initParam.SectionName,
                                                                           AzureConstants.UploadConcurrencyParamName, 
                                                                           defaultTableBatchUploadConcurrencyCount);
                this.tableUploadSettings.Filter = this.configReader.GetUnencryptedConfigValue(
                                                      this.initParam.SectionName,
                                                      AzureConstants.LogFilterParamName,
                                                      defaultTableFilter);

                // Check for test settings
                var logDeletionAgeTestValue = TimeSpan.FromMinutes(this.configReader.GetUnencryptedConfigValue(
                                                  this.initParam.SectionName,
                                                  dataDeletionAgeTestParamName,
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
                            dataDeletionAgeTestParamName,
                            AzureConstants.MaxDataDeletionAge);
                        logDeletionAgeTestValue = AzureConstants.MaxDataDeletionAge;
                    }
                    this.tableUploadSettings.EntityDeletionAge = logDeletionAgeTestValue;
                }

                // Write settings to log
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Table storage upload enabled: ETW trace table: {0}, Upload interval (minutes): {1}, Entity deletion age ({2}): {3}",
                    this.tableUploadSettings.TableName,
                    this.tableUploadSettings.UploadIntervalMinutes,
                    (logDeletionAgeTestValue == TimeSpan.Zero) ? "days" : "minutes",
                    (logDeletionAgeTestValue == TimeSpan.Zero) ? entityDeletionAge : logDeletionAgeTestValue);
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Windows Fabric event filters for table storage upload: {0}",
                    this.tableUploadSettings.Filter);
            }
            else
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Table storage upload not enabled");
            }
        }
        
        public void OnEtwEventDeliveryStart()
        {
            this.perfHelper.TableUploadPeriodBegin();

            var storageAccount = this.tableUploadSettings.StorageAccountFactory.GetStorageAccount();
            Debug.Assert(null == storageAccount);

            // Create a table client
            CloudTableClient tableClient = AzureTableCommon.CreateNewTableClient(storageAccount);

            // Create new table service contexts using the new credentials
            for (int i = 0; i < this.batchConcurrencyCount; i++)
            {
                Debug.Assert(null == this.tableClients[i]);
                this.tableClients[i] = tableClient;
            }
        }

        public void OnEtwEventDeliveryStop()
        {
            // No more events will be delivered during this event delivery period.
            // So upload any entities that we have batched up until now, even though
            // we may not have reached the limit for batch size.
            if (this.batchConcurrencyCount != this.contextIndex)
            {
                CommitBatchedChangesBegin();
            }
            
            // Wait for all in-flight batch operations to complete.
            WaitForInflightBatchedOperations();
            
            // All service contexts are now available.
            this.availableIndexes.Clear();
            for (int i=0; i < this.batchConcurrencyCount; i++)
            {
                this.availableIndexes.Push(i);
            }
            
            // Set the service context index to an invalid value, so that we try
            // to use a fresh service context next time.
            this.contextIndex = this.batchConcurrencyCount;

            // Clear out the table service contexts, so that we allocate and initialize
            // new ones in the next event delivery period.
            for (int i=0; i < this.batchConcurrencyCount; i++)
            {
                Debug.Assert(null == this.handles[i]);
                Debug.Assert(null == this.asyncResult[i]);
                this.tableClients[i] = null;
            }
            
            this.perfHelper.TableUploadPeriodEnd();
        }

        private void WaitForInflightBatchedOperations()
        {
            // Wait for all in-flight batch operations to complete
            for (int i = 0; i < this.batchConcurrencyCount; i++)
            {
                if (null == this.handles[i])
                {
                    continue;
                }

                // We have a non-null wait handle, which implies an in-flight batched
                // operation. Wait for it to complete.
                this.handles[i].WaitOne();
                this.handles[i].Dispose();
                this.handles[i] = null;

                // Verify the results of the completed operation
                CommitBatchedChangesEnd(i);
            }
        }

        private bool CommitBatchedChangesBegin()
        {
            Debug.Assert(null == this.handles[this.contextIndex]);
            Debug.Assert(null == this.asyncResult[this.contextIndex]);

            if (this.stopping)
            {
                // The consumer is being stopped. As far as possible, we don't want 
                // the saving of batched changes to be interrupted midway due to
                // any any timeout that may be enforced on consumer stop. Therefore,
                // let's not save the batched entities at this time. 
                //
                // Note that if a consumer stop is initiated while buffered event 
                // file is being processed, that file gets processed again when the
                // consumer is restarted. So we should get another opportunity to 
                // upload events from this buffered event file when we are restarted.
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "The consumer is being stopped. Therefore, filtered traces will not be sent for upload to table storage.");
                return false;
            }

            // Save batched changes
            bool success = true;
            try
            {
                this.perfHelper.ExternalOperationBegin(
                    ExternalOperationTime.ExternalOperationType.TableUpload, 
                    this.contextIndex);
                var batchOperation = new TableBatchOperation();
                foreach (var entityToSave in this.unsavedEntities[this.contextIndex])
                {
                    batchOperation.Add(TableOperation.Insert(entityToSave));
                }

                this.asyncResult[this.contextIndex] = this.tableClients[this.contextIndex]
                    .GetTableReference(this.tableUploadSettings.TableName)
                    .BeginExecuteBatch(batchOperation, state => { }, null);
            }
            catch (Exception e)
            {
                success = false;

                try
                {
                    AzureTableCommon.ProcessTableServiceRequestException(
                                 this.traceSource,
                                 this.logSourceId,
                                 e, 
                                 AzureTableCommon.TableServiceAction.SaveEntityBatchBegin);
                }
                catch (MaxRetriesException)
                {
                    // A failure that we are designed to handle did not get resolved
                    // even after the maximum number of retries. Let's abort this
                    // pass and retry on the next pass.
                    this.bufferedEventProvider.AbortEtwEventDelivery();
                }
            }

            if (false == success)
            {
                // Clear out all entities from the table service context, so that it
                // is ready for reuse.
                ClearTableServiceContext(this.contextIndex);

                Debug.Assert(null == this.handles[this.contextIndex]);
                Debug.Assert(null == this.asyncResult[this.contextIndex]);
            }
            else
            {
                // Save the handle we need to wait on in order to know when this
                // batched operation completes.
                this.handles[this.contextIndex] = this.asyncResult[this.contextIndex].AsyncWaitHandle;
            }

            return success;
        }

        private bool VerifyTableServiceResponse(IList<TableResult> results, int context)
        {
            for (var idx = 0; idx < this.unsavedEntities[context].Count; idx++)
            {
                var result = results[idx];
                var eventEntity = this.unsavedEntities[context][idx];
                if (Utility.IsNetworkError(result.HttpStatusCode))
                {
                    // We encountered a network error that wasn't resolved even
                    // after retries.
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Error {0} encountered when attempting to upload an entity to table storage. Event represented by entity: {1},{2},{3},{4}.{5},{6},{7}.",
                        result.HttpStatusCode,
                        eventEntity.EventTimestamp,
                        eventEntity.ThreadId,
                        eventEntity.ProcessId,
                        eventEntity.TaskName,
                        eventEntity.EventType,
                        this.initParam.FabricNodeId,
                        eventEntity.EventText);
                    this.bufferedEventProvider.AbortEtwEventDelivery();
                    return false;
                }

                if (result.HttpStatusCode < AzureTableCommon.HttpSuccessCodeMin ||
                    result.HttpStatusCode > AzureTableCommon.HttpSuccessCodeMax)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "FabricDCA encountered an error when attempting to upload an event to table service. Error code: {0}. Event information: {1},{2},{3},{4}.{5},{6},{7}",
                        result.HttpStatusCode,
                        eventEntity.EventTimestamp,
                        eventEntity.ThreadId,
                        eventEntity.ProcessId,
                        eventEntity.TaskName,
                        eventEntity.EventType,
                        this.initParam.FabricNodeId,
                        eventEntity.EventText);
                    return false;
                }
            }

            return true;
        }

        private void ClearTableServiceContext(int index)
        {
            // Clear the list of unsaved entities
            this.unsavedEntities[index].Clear();
        }

        private void CommitBatchedChangesEnd(int index)
        {
            IList<TableResult> results = new TableResult[0];
            try
            {
                // Get the result of the batched operation
                results = this.tableClients[this.contextIndex]
                    .GetTableReference(this.tableUploadSettings.TableName)
                    .EndExecuteBatch(this.asyncResult[this.contextIndex]);
                this.perfHelper.ExternalOperationEnd(
                    ExternalOperationTime.ExternalOperationType.TableUpload, 
                    index);
            }
            catch (Exception e)
            {
                AzureTableCommon.TableServiceExceptionAction action;
                try
                {
                    action = AzureTableCommon.ProcessTableServiceRequestException(
                                 this.traceSource,
                                 this.logSourceId,
                                 e, 
                                 AzureTableCommon.TableServiceAction.SaveEntityBatchEnd);
                }
                catch (MaxRetriesException)
                {
                    // A failure that we are designed to handle did not get resolved
                    // even after the maximum number of retries. Let's abort this
                    // pass and retry on the next pass.
                    this.bufferedEventProvider.AbortEtwEventDelivery();
                    action = AzureTableCommon.TableServiceExceptionAction.Abort;
                }
                
                if (AzureTableCommon.TableServiceExceptionAction.Abort == action)
                {
                    // Give up on this batch
                    results = null;
                }
                else
                {
                    Debug.Assert(AzureTableCommon.TableServiceExceptionAction.ProcessResponse == action);
                }
            }

            this.asyncResult[index] = null;

            // Verify the result
            if (results != null)
            {
                if (VerifyTableServiceResponse(results, index))
                {
                    this.perfHelper.BatchUploadedToAzureTable((ulong) this.unsavedEntities[index].Count);
                }
            }

            // Clear out all entities from the table service context, so that it
            // is ready for reuse.
            ClearTableServiceContext(index);
        }

        private bool ShouldSaveBatchedChanges()
        {
            return (this.unsavedEntities[this.contextIndex].Count == AzureTableCommon.MaxEntitiesInBatch);
        }

        private int GetServiceContextIndex()
        {
            int index;
            
            if (this.availableIndexes.Count > 0)
            {
                // A table service context object is available
                index = this.availableIndexes.Pop();
            }
            else
            {
                // All of the table service context objects are in use. Reuse one
                // of them as soon as it is available for reuse.
                index = WaitHandle.WaitAny(this.handles);
                this.handles[index].Dispose();
                this.handles[index] = null;

                // A batched operation that was previously started has now completed,
                // because of which its service context is now available for reuse.
                // But before reusing the service context, let's verify the results
                // of the previous batched operation and also prepare the service 
                // context object for reuse.
                CommitBatchedChangesEnd(index);
            }

            return index;
        }

        public void OnEtwEventAvailable(DecodedEtwEvent etwEvent, string nodeUniqueEventId)
        {
            // Create and initialize the entity to be written
            EtwEventEntity eventEntity = new EtwEventEntity();
            eventEntity.PartitionKey = this.initParam.FabricNodeId;
            // Prefix the row key with the event timestamp. This allows us to use the row key when 
            // querying for old events, which makes old event deletion faster.
            eventEntity.RowKey = String.Concat(
                                     etwEvent.Timestamp.Ticks.ToString(
                                         "D20", 
                                         CultureInfo.InvariantCulture),
                                     "_",
                                     nodeUniqueEventId);
            eventEntity.EventTimestamp = etwEvent.Timestamp;
            eventEntity.Level = etwEvent.Level;
            eventEntity.ThreadId = (int) etwEvent.ThreadId;
            eventEntity.ProcessId = (int) etwEvent.ProcessId;
            eventEntity.TaskName = etwEvent.TaskName;
            eventEntity.EventType = etwEvent.EventType;
            eventEntity.AzureNodeInstanceId = this.azureNodeInstanceId;
            eventEntity.EventText = etwEvent.EventText;
            eventEntity.EventId = etwEvent.EventRecord.EventHeader.EventDescriptor.Id;

            const int eventTextLengthWarningThreshold = 8192;
            if (eventEntity.EventText.Length > eventTextLengthWarningThreshold)
            {
                // The maximum transaction size for a batched transaction to Windows
                // Azure table storage is 4MB. The maximum number of entities in a
                // batch is 100. Therefore, on an average we have about 40KB per entity,
                // including the overhead incurred by headers and other formatting.
                // This should be more than enough for our log messages. However, if
                // we come across a particularly long message, we log a warning.
                this.traceSource.WriteWarning(
                    this.logSourceId,
                    "Encountered a long trace message. Several long messages in a batched transaction can cause the transaction size limit to be exceeded. Message length: {0}. Message information: {1},{2},{3},{4}.{5},{6}",
                    eventEntity.EventText.Length,
                    eventEntity.EventTimestamp,
                    eventEntity.ThreadId,
                    eventEntity.ProcessId,
                    eventEntity.TaskName,
                    eventEntity.EventType,
                    this.initParam.FabricNodeId);
            }

            if (this.batchConcurrencyCount == this.contextIndex)
            {
                // We need a service context object that we can use
                this.contextIndex = GetServiceContextIndex();
            }

            // If the entity is already being tracked in the table service context,
            // there is nothing more to be done
            if (this.unsavedEntities[this.contextIndex].Any(
                            e => 
                            {
                                return e.PartitionKey.Equals(eventEntity.PartitionKey) &&
                                        e.RowKey.Equals(eventEntity.RowKey);
                            }))
            {
                return;
            }
            
            // Add entity to batched changes
            this.unsavedEntities[this.contextIndex].Add(eventEntity);

            // Save batched changes if necessary
            if (ShouldSaveBatchedChanges())
            {
                bool result = CommitBatchedChangesBegin();
                if (result)
                {
                    // The current service context cannot be used until the batched 
                    // operation we started above is complete. So set the service
                    // context index to an invalid value, so that we try to use a 
                    // different context next time.
                    this.contextIndex = this.batchConcurrencyCount;
                }
            }
        }

        private void CreateTableWorker(CloudStorageAccount storageAccount)
        {
            // Create the table client object
            CloudTableClient cloudTableClient = AzureTableCommon.CreateNewTableClient(storageAccount);

            // Create the table
#if DotNetCoreClrLinux
            cloudTableClient.GetTableReference(this.tableUploadSettings.TableName).CreateIfNotExistsAsync().Wait();
#else
            cloudTableClient.GetTableReference(this.tableUploadSettings.TableName).CreateIfNotExists();
#endif

            // Create an array of table service context objects
            this.availableIndexes = new Stack<int>();
            this.tableClients = new CloudTableClient[this.batchConcurrencyCount];
            for (int i=0; i < this.batchConcurrencyCount; i++)
            {
                this.availableIndexes.Push(i);
            }
        }

        internal RetriableOperationExceptionHandler.Response CreateTableExceptionHandler(Exception e)
        {
            this.traceSource.WriteError(
                this.logSourceId,
                "Exception encountered when attempting to create table {0} in storage account {1}. Exception information: {2}",
                this.tableUploadSettings.TableName,
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

        private void CreateTable()
        {
            var storageAccount = this.tableUploadSettings.StorageAccountFactory.GetStorageAccount();

            Utility.PerformInitializationWithRetries(
                CreateTableWorker,
                storageAccount,
                new RetriableOperationExceptionHandler(this.CreateTableExceptionHandler));

            // Initialize the index into the table service context array to 
            // an invalid (out of range) value.
            this.contextIndex = this.batchConcurrencyCount;

            // Create an array of IAsyncResult objects that we use when performing
            // batched operations on the table
            this.asyncResult = new IAsyncResult[this.batchConcurrencyCount];

            // Create an array of handles to wait on until a table service
            // context becomes available
            this.handles = new WaitHandle[this.batchConcurrencyCount];

            // Initialize unsaved entity lists
            this.unsavedEntities = new List<EtwEventEntity>[this.batchConcurrencyCount];
            for (int i = 0; i < this.batchConcurrencyCount; i++)
            {
                this.unsavedEntities[i] = new List<EtwEventEntity>();
            }
        }

        private TableQuery<TableEntity> CreateDeletionQuery(DateTime cutoffTime)
        {
            // We query for entities that meet the following criteria:
            //  - partition key equals the partition key used by the current DCA
            //    instance 
            //  - event timestamp is old enough that the entity needs to be deleted
            string cutoffTimeTicks = cutoffTime.Ticks.ToString("D20", CultureInfo.InvariantCulture);
            return new TableQuery<TableEntity>()
                .Where(TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, this.initParam.FabricNodeId))
                .Where(TableQuery.GenerateFilterCondition("RowKey", QueryComparisons.LessThan, cutoffTimeTicks));
        }

        // Settings related to upload of data to table storage
        private struct TableUploadSettings
        {
            internal bool Enabled;
            internal StorageAccountFactory StorageAccountFactory;
            internal string TableName;
            internal TimeSpan UploadIntervalMinutes;
            internal TimeSpan EntityDeletionAge;
            internal string Filter;
            internal int BatchUploadConcurrencyCount;
        };

        // Defines the entity that represents an ETW event
        private class EtwEventEntity : TableEntity
        {
            public DateTime EventTimestamp { get; set; }

            public int Level { get; set; }

            public int ThreadId { get; set; }

            public int ProcessId { get; set; }

            public string TaskName { get; set; }

            public string EventType { get; set; }

            public string AzureNodeInstanceId { get; set; }

            public string EventText { get; set; }

            public int EventId { get; set; }
        }
    }
}
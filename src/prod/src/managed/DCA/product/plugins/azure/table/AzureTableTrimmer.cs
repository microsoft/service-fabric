// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
using System.Fabric.Dca;

namespace FabricDCA
{
    using System;
    using System.Linq;
    using System.Diagnostics;
    using System.Fabric.Common.Tracing;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Table;

    // This class implements the logic to delete old entities from Azure table storage.
    class AzureTableTrimmer
    {
        // This delegate creates a query that returns those entities that are candidates
        // for deletion from the table
        internal delegate TableQuery<TableEntity> QueryCreationMethod(DateTime cutoffTime);

        #region Private Fields
        // Constants
        private const string OldLogDeletionTimerIdSuffix = "_OldEntityDeletionTimer";

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Object that measures performance
        private readonly AzureTablePerformance perfHelper;

        // Timer to delete old logs
        private readonly DcaTimer oldLogDeletionTimer;

        private readonly StorageAccountFactory storageAccountFactory;

        // Table name
        private readonly string tableName;

        // Age above which entities should be deleted
        private readonly TimeSpan entityDeletionAge;

        // Method that creates the query to return entities to be deleted
        readonly QueryCreationMethod queryCreationMethod;

        // Whether or not the object has been disposed
        bool disposed;

        // Whether or not we are in the process of stopping
        private bool stopping;
        #endregion

        internal AzureTableTrimmer(
                     FabricEvents.ExtensionsEvents traceSource,
                     string logSourceId,
                     StorageAccountFactory storageAccountFactory,
                     string tableName,
                     TimeSpan entityDeletionAge,
                     QueryCreationMethod queryCreationMethod,
                     AzureTablePerformance perfHelper)
        {
            // Initialization
            this.stopping = false;
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;
            this.storageAccountFactory = storageAccountFactory;
            this.tableName = tableName;
            this.entityDeletionAge = entityDeletionAge;
            this.perfHelper = perfHelper;
            this.queryCreationMethod = queryCreationMethod;

            // Deletion and query are currently synchronous, so the concurrency
            // count is 1.
            this.perfHelper.ExternalOperationInitialize(
                ExternalOperationTime.ExternalOperationType.TableQuery,
                1);
            this.perfHelper.ExternalOperationInitialize(
                ExternalOperationTime.ExternalOperationType.TableDeletion,
                1);

            // Create a timer to delete old logs
            string timerId = String.Concat(
                                    this.logSourceId,
                                    OldLogDeletionTimerIdSuffix);
            this.oldLogDeletionTimer = new DcaTimer(
                                                timerId,
                                                this.DeleteOldLogsHandler,
                                                (this.entityDeletionAge < TimeSpan.FromDays(1)) ?
                                                    AzureConstants.OldLogDeletionIntervalForTest :
                                                    AzureConstants.OldLogDeletionInterval);
            this.oldLogDeletionTimer.Start();
        }

        internal void Dispose()
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
        }

        private void QueryAndDeleteEntityBatch(CloudStorageAccount storageAccount, CloudTableClient tableServiceContext, DateTime cutoffTime, ref TableContinuationToken continuationToken)
        {
            TableContinuationToken initialContinuationToken = continuationToken;
            continuationToken = null;
            this.traceSource.WriteInfo(this.logSourceId, "Deleting table entries older than {0}.", cutoffTime);

            // Create a query object
            TableQuery<TableEntity> query = this.queryCreationMethod(cutoffTime);

            TableQuerySegment<TableEntity> resultSegment;
            try
            {
                this.perfHelper.ExternalOperationBegin(
                    ExternalOperationTime.ExternalOperationType.TableQuery,
                    0);

                // Execute the query
#if DotNetCoreClr
                resultSegment = tableServiceContext.GetTableReference(this.tableName).ExecuteQuerySegmentedAsync(query, initialContinuationToken).Result;
#else
                resultSegment = tableServiceContext.GetTableReference(this.tableName).ExecuteQuerySegmented(query, initialContinuationToken);
#endif

                this.perfHelper.ExternalOperationEnd(
                    ExternalOperationTime.ExternalOperationType.TableQuery,
                    0);
            }
            catch (Exception e)
            {
                AzureTableCommon.TableServiceExceptionAction action = AzureTableCommon.ProcessTableServiceQueryException(
                                                                          this.traceSource,
                                                                          this.logSourceId,
                                                                          e,
                                                                          AzureTableCommon.TableServiceAction.QueryEntitiesForDeletion);
                // Give up on this batch
                Debug.Assert(AzureTableCommon.TableServiceExceptionAction.Abort == action);
                return;
            }

            continuationToken = resultSegment.ContinuationToken;
            if (!resultSegment.Results.Any())
            {
                // Query did not give us any entities to delete. Nothing more to
                // be done here.
                return;
            }
            this.perfHelper.BatchQueriedFromAzureTable((ulong) resultSegment.Results.Count());

            // Create a table client
            CloudTableClient tableClient = AzureTableCommon.CreateNewTableClient(storageAccount);

            // Walk through the query results
            var entitiesToDelete = new List<TableEntity>();
            string partitionKey = String.Empty;
            foreach (var entity in resultSegment.Results)
            {
                // Azure table service requires all entities in a batched transaction to
                // have the same parition key. Hence if the partition key for this entity
                // is not the same as that of the entities we already added to the batch,
                // then delete those entities first and add this one to a new batch.
                if ((false == String.IsNullOrEmpty(partitionKey)) &&
                    (false == entity.PartitionKey.Equals(partitionKey)))
                {
                    DeleteEntityBatch(tableClient, entitiesToDelete);

                    // Clear for the next batch
                    entitiesToDelete.Clear();
                }

                // Record the partition key for the current batch
                partitionKey = entity.PartitionKey;

                entitiesToDelete.Add(entity);

                // If we have reached that maximum entity count for a batch, then
                // delete those entities.
                if (entitiesToDelete.Count == AzureTableCommon.MaxEntitiesInBatch)
                {
                    DeleteEntityBatch(tableClient, entitiesToDelete);

                    // Clear for the next batch
                    entitiesToDelete.Clear();
                }
            }

            if (entitiesToDelete.Any())
            {
                DeleteEntityBatch(tableClient, entitiesToDelete);
            }
        }

        private void DeleteEntityBatch(CloudTableClient client, List<TableEntity> entitiesToDelete)
        {
            var batchOperation = new TableBatchOperation();
            foreach (var entity in entitiesToDelete)
            {
                batchOperation.Add(TableOperation.Delete(entity));
            }

            // Save batched changes
            IList<TableResult> results = new TableResult[0];
            try
            {
                this.perfHelper.ExternalOperationBegin(
                    ExternalOperationTime.ExternalOperationType.TableDeletion,
                    0);
#if DotNetCoreClr
                results = client.GetTableReference(this.tableName).ExecuteBatchAsync(batchOperation).Result;
#else
                results = client.GetTableReference(this.tableName).ExecuteBatch(batchOperation);
#endif
                this.perfHelper.ExternalOperationEnd(
                    ExternalOperationTime.ExternalOperationType.TableDeletion,
                    0);
            }
            catch (Exception e)
            {
                AzureTableCommon.TableServiceExceptionAction action = AzureTableCommon.ProcessTableServiceRequestException(
                                                                          this.traceSource,
                                                                          this.logSourceId,
                                                                          e,
                                                                          AzureTableCommon.TableServiceAction.DeleteEntityBatch);
                if (AzureTableCommon.TableServiceExceptionAction.Abort == action)
                {
                    // Give up on this batch
                    return;
                }

                Debug.Assert(AzureTableCommon.TableServiceExceptionAction.ProcessResponse == action);
            }

            for (var idx = 0; idx < batchOperation.Count; idx++)
            {
                TableEntity entity = entitiesToDelete[idx];
                var result = results[idx];

                if (Utility.IsNetworkError(result.HttpStatusCode))
                {
                    // We encountered a network error that wasn't resolved even
                    // after retries.
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Error {0} encountered when attempting to delete an entity from table storage. Entity information: {1},{2},{3}.",
                        results[idx],
                        entity.PartitionKey,
                        entity.RowKey,
                        entity.Timestamp);

                    throw new MaxRetriesException();
                }

                if (AzureTableCommon.HttpCodeResourceNotFound == result.HttpStatusCode)
                {
                    continue;
                }

                if (result.HttpStatusCode < AzureTableCommon.HttpSuccessCodeMin ||
                    result.HttpStatusCode > AzureTableCommon.HttpSuccessCodeMax)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "FabricDCA encountered an error when attempting to delete an entity from table service. Error code: {0}. Entity information: {1},{2},{3}.",
                        result.HttpStatusCode,
                        entity.PartitionKey,
                        entity.RowKey,
                        entity.Timestamp);
                    return;
                }
            }
            this.perfHelper.BatchDeletedFromAzureTable((ulong)batchOperation.Count);
        }

        private void DeleteOldEntities()
        {
            var storageAccount = this.storageAccountFactory.GetStorageAccount();
            Debug.Assert(null == storageAccount);

            // Create a table client
            CloudTableClient tableClient = AzureTableCommon.CreateNewTableClient(storageAccount);

            // Create a new table service context
            CloudTableClient tableServiceContext = tableClient;

            // Figure out the timestamp before which all entities will be deleted
            DateTime cutoffTime = DateTime.UtcNow.Add(-this.entityDeletionAge);

            // Delete the entities in batches. Do this in a loop until our query
            // for "delete-eligible" entities yields no results.
            TableContinuationToken continuationToken = null;
            do
            {
                QueryAndDeleteEntityBatch(storageAccount, tableServiceContext, cutoffTime, ref continuationToken);

                if (this.stopping)
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "The consumer is being stopped. Therefore, no more old entities will be deleted in this pass.");
                    break;
                }
            } while (null != continuationToken);
        }

        private void DeleteOldLogsHandler(object state)
        {
            try
            {
                this.perfHelper.AzureTableDeletionPassBegin();
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Starting deletion of old entities from table storage ...");

                // Delete old entities from table storage
                DeleteOldEntities();

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Finished deletion of old entities from table storage.");
                this.perfHelper.AzureTableDeletionPassEnd();
            }
            catch (MaxRetriesException)
            {
                // A failure that we are designed to handle did not get resolved
                // even after the maximum number of retries. Let's abort this
                // pass and retry on the next pass.
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Deletion of old entities from table storage aborted due to errors.");
            }

            // Schedule the next pass
            this.oldLogDeletionTimer.Start();
        }
    }
}
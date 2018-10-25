// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.Dca;

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;

    class AzureTablePerformance
    {
        // Constants
        private const string traceType = AzureConstants.PerformanceTraceType;

        // DCA controller object
        private FabricEvents.ExtensionsEvents traceSource;

        // Tag used to represent the source of the log message
        private string logSourceId;

        // The beginning tick count for table deletion pass
        private long tableDeletionPassBeginTime;

        // Counter to count the number of batches of entities uploaded to Azure table store
        private ulong azureEntityBatchesUploaded;

        // Counter to count the number of entities uploaded to Azure table store
        private ulong azureEntitiesUploaded;

        // Counter to count the number of batches of entities queried from Azure table store
        private ulong azureEntityBatchesQueried;

        // Counter to count the number of entities queried from Azure table store
        private ulong azureEntitiesQueried;

        // Counter to count the number of batches of entities deleted from Azure table store
        private ulong azureEntityBatchesDeleted;

        // Counter to count the number of entities deleted from Azure table store
        private ulong azureEntitiesDeleted;

        // Objects to keep track of how much time it takes to perform storage client operations
        private Dictionary<ExternalOperationTime.ExternalOperationType, ExternalOperationTime> externalOperationTime =
                           new Dictionary<ExternalOperationTime.ExternalOperationType, ExternalOperationTime> ();

        internal AzureTablePerformance(FabricEvents.ExtensionsEvents traceSource, string logSourceId)
        {
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;
        }
        
        internal void BatchUploadedToAzureTable(ulong entityCount)
        {
            this.azureEntityBatchesUploaded++;
            this.azureEntitiesUploaded += entityCount;
        }

        internal void ExternalOperationInitialize(ExternalOperationTime.ExternalOperationType type, int concurrencyCount)
        {
            this.externalOperationTime[type] = new ExternalOperationTime(concurrencyCount);
        }
        
        internal void ExternalOperationBegin(ExternalOperationTime.ExternalOperationType type, int index)
        {
            this.externalOperationTime[type].OperationBegin(index);
        }

        internal void ExternalOperationEnd(ExternalOperationTime.ExternalOperationType type, int index)
        {
            this.externalOperationTime[type].OperationEnd(index);
        }

        internal void TableUploadPeriodBegin()
        {
            this.azureEntityBatchesUploaded = 0;
            this.azureEntitiesUploaded = 0;
            
            if (this.externalOperationTime.ContainsKey(ExternalOperationTime.ExternalOperationType.TableUpload))
            {
                this.externalOperationTime[ExternalOperationTime.ExternalOperationType.TableUpload].Reset();
            }
        }

        internal void TableUploadPeriodEnd()
        {
            this.traceSource.WriteInfo(
                traceType,
                "AzureTableEntityBatchUploadInfo - {0}: {1},{2}", // WARNING: The performance test parses this message,
                                                                  // so changing the message can impact the test.
                this.logSourceId,
                this.azureEntityBatchesUploaded,
                this.azureEntitiesUploaded);

            if (externalOperationTime.ContainsKey(ExternalOperationTime.ExternalOperationType.TableUpload))
            {
                this.traceSource.WriteInfo(
                    traceType,
                    "ExternalOperationTableUpload - {0}: {1},{2}", // WARNING: The performance test parses this message,
                                                                        // so changing the message can impact the test.
                    this.logSourceId,
                    this.externalOperationTime[ExternalOperationTime.ExternalOperationType.TableUpload].OperationCount,
                    this.externalOperationTime[ExternalOperationTime.ExternalOperationType.TableUpload].CumulativeTimeSeconds);
            }
            return;
        }

        internal void BatchQueriedFromAzureTable(ulong entityCount)
        {
            this.azureEntityBatchesQueried++;
            this.azureEntitiesQueried += entityCount;
        }
        
        internal void BatchDeletedFromAzureTable(ulong entityCount)
        {
            this.azureEntityBatchesDeleted++;
            this.azureEntitiesDeleted += entityCount;
        }
    
        internal void AzureTableDeletionPassBegin()
        {
            this.azureEntityBatchesQueried = 0;
            this.azureEntitiesQueried = 0;
            this.azureEntityBatchesDeleted = 0;
            this.azureEntitiesDeleted = 0;
            this.tableDeletionPassBeginTime = DateTime.Now.Ticks;
            
            this.externalOperationTime[ExternalOperationTime.ExternalOperationType.TableQuery].Reset();
            this.externalOperationTime[ExternalOperationTime.ExternalOperationType.TableDeletion].Reset();
        }
        
        internal void AzureTableDeletionPassEnd()
        {
            long tableDeletionPassEndTime = DateTime.Now.Ticks;
            double tableDeletionPassTimeSeconds = ((double) (tableDeletionPassEndTime - this.tableDeletionPassBeginTime)) / (10 * 1000 * 1000);
            
            this.traceSource.WriteInfo(
                traceType,
                "AzureTableDeletionPassInfo - {0}: {1},{2},{3},{4},{5}", // WARNING: The performance test parses this message,
                                                                         // so changing the message can impact the test.
                this.logSourceId,
                tableDeletionPassTimeSeconds,
                this.azureEntitiesDeleted,
                this.azureEntityBatchesDeleted,
                this.azureEntitiesQueried,
                this.azureEntityBatchesQueried);
                
            this.traceSource.WriteInfo(
                traceType,
                "ExternalOperationTableQuery - {0}: {1},{2}", // WARNING: The performance test parses this message,
                                                                   // so changing the message can impact the test.
                this.logSourceId,
                this.externalOperationTime[ExternalOperationTime.ExternalOperationType.TableQuery].OperationCount,
                this.externalOperationTime[ExternalOperationTime.ExternalOperationType.TableQuery].CumulativeTimeSeconds);

            this.traceSource.WriteInfo(
                traceType,
                "ExternalOperationTableDeletion - {0}: {1},{2}", // WARNING: The performance test parses this message,
                                                                      // so changing the message can impact the test.
                this.logSourceId,
                this.externalOperationTime[ExternalOperationTime.ExternalOperationType.TableDeletion].OperationCount,
                this.externalOperationTime[ExternalOperationTime.ExternalOperationType.TableDeletion].CumulativeTimeSeconds);
            return;
        }
    }
}
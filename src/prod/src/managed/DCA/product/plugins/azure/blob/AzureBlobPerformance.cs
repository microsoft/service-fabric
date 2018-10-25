// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;

    internal class AzureBlobPerformance
    {
        // Constants
        private const string TraceType = AzureConstants.PerformanceTraceType;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Objects to keep track of how much time it takes to perform storage client operations
        private readonly Dictionary<ExternalOperationTime.ExternalOperationType, ExternalOperationTime> externalOperationTime =
                           new Dictionary<ExternalOperationTime.ExternalOperationType, ExternalOperationTime>();

        // Object used for tracing
        private FabricEvents.ExtensionsEvents traceSource;

        // The beginning tick count for blob deletion pass
        private long blobDeletionPassBeginTime;

        // Counter to count the number of query segments for queries to Azure blob store
        private ulong azureBlobQuerySegments;

        // Counter to count the number of blobs queried from Azure blob store
        private ulong azureBlobsQueried;

        // Counter to count the number of blobs deleted from Azure blob store
        private ulong azureBlobsDeleted;

        // The beginning tick count for blob upload pass
        private long blobUploadPassBeginTime;

        // Counter to count the number of blobs copied to Azure blob store
        private ulong azureBlobsUploaded;

        internal AzureBlobPerformance(FabricEvents.ExtensionsEvents traceSource, string logSourceId)
        {
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;
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

        internal void AzureBlobUploaded()
        {
            this.azureBlobsUploaded++;
        }

        internal void AzureBlobUploadPassBegin()
        {
            this.azureBlobsUploaded = 0;
            this.blobUploadPassBeginTime = DateTime.Now.Ticks;

            this.externalOperationTime[ExternalOperationTime.ExternalOperationType.BlobCopy].Reset();
        }

        internal void AzureBlobUploadPassEnd()
        {
            long blobUploadPassEndTime = DateTime.Now.Ticks;
            double blobUploadPassTimeSeconds = ((double)(blobUploadPassEndTime - this.blobUploadPassBeginTime)) / (10 * 1000 * 1000);

            this.traceSource.WriteInfo(
                TraceType,
                "AzureBlobUploadPassInfo - {0}: {1},{2}", // WARNING: The performance test parses this message,
                                                          // so changing the message can impact the test.
                this.logSourceId,
                blobUploadPassTimeSeconds,
                this.azureBlobsUploaded);

            this.traceSource.WriteInfo(
                TraceType,
                "ExternalOperationAzureBlobCopy - {0}: {1},{2}", // WARNING: The performance test parses this message,
                                                                 // so changing the message can impact the test.
                this.logSourceId,
                this.externalOperationTime[ExternalOperationTime.ExternalOperationType.BlobCopy].OperationCount,
                this.externalOperationTime[ExternalOperationTime.ExternalOperationType.BlobCopy].CumulativeTimeSeconds);
        }

        internal void AzureBlobsQueried(ulong blobCount)
        {
            this.azureBlobQuerySegments++;
            this.azureBlobsQueried += blobCount;
        }
        
        internal void AzureBlobDeleted()
        {
            this.azureBlobsDeleted++;
        }

        internal void AzureBlobDeletionPassBegin()
        {
            this.azureBlobQuerySegments = 0;
            this.azureBlobsQueried = 0;
            this.azureBlobsDeleted = 0;
            this.blobDeletionPassBeginTime = DateTime.Now.Ticks;
            
            this.externalOperationTime[ExternalOperationTime.ExternalOperationType.BlobQuery].Reset();
            this.externalOperationTime[ExternalOperationTime.ExternalOperationType.BlobDeletion].Reset();
        }
        
        internal void AzureBlobDeletionPassEnd()
        {
            long blobDeletionPassEndTime = DateTime.Now.Ticks;
            double blobDeletionPassTimeSeconds = ((double)(blobDeletionPassEndTime - this.blobDeletionPassBeginTime)) / (10 * 1000 * 1000);
            
            this.traceSource.WriteInfo(
                TraceType,
                "AzureBlobDeletionPassInfo - {0}: {1},{2},{3},{4}", // WARNING: The performance test parses this message,
                                                                    // so changing the message can impact the test.
                this.logSourceId,
                blobDeletionPassTimeSeconds,
                this.azureBlobsDeleted,
                this.azureBlobQuerySegments,
                this.azureBlobsQueried);
                
            this.traceSource.WriteInfo(
                TraceType,
                "ExternalOperationBlobQuery - {0}: {1},{2}", // WARNING: The performance test parses this message,
                                                                  // so changing the message can impact the test.
                this.logSourceId,
                this.externalOperationTime[ExternalOperationTime.ExternalOperationType.BlobQuery].OperationCount,
                this.externalOperationTime[ExternalOperationTime.ExternalOperationType.BlobQuery].CumulativeTimeSeconds);
        
            this.traceSource.WriteInfo(
                TraceType,
                "ExternalOperationBlobDeletion - {0}: {1},{2}", // WARNING: The performance test parses this message,
                                                                     // so changing the message can impact the test.
                this.logSourceId,
                this.externalOperationTime[ExternalOperationTime.ExternalOperationType.BlobDeletion].OperationCount,
                this.externalOperationTime[ExternalOperationTime.ExternalOperationType.BlobDeletion].CumulativeTimeSeconds);
        }
    }
}
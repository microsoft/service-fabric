// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Linq;

    internal class AzureBlobPerformance : IDisposable
    {
        private const string TraceType = "AzureBlobUploaderPerformance";

        private const double ReadCountersTimerInterval = 2.0;

        private const string AzureBlobUploaderCountersReadTimerIdSuffix = "_AzureBlobUploaderCountersReadTimer";

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Objects to keep track of how much time it takes to perform storage client operations
        private readonly Dictionary<ExternalOperationTime.ExternalOperationType, ExternalOperationTime> externalOperationTime =
                           new Dictionary<ExternalOperationTime.ExternalOperationType, ExternalOperationTime>();

        // List of counters being monitored
        private readonly List<PerformanceCounter> perfCountersList;

        // Timer to periodically query counters
        private readonly DcaTimer azureBlobUploaderCountersReadTimer;

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

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

        // Whether or not the object has been disposed
        private bool disposed;

        // Determines whether to schedule timer for another pass
        private bool scheduleNextPass;

        // Read byte count
        private long readByteCount;

        // Write byte count
        private long writeByteCount;

        internal AzureBlobPerformance(FabricEvents.ExtensionsEvents traceSource, string logSourceId)
        {
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;

            this.perfCountersList = new List<PerformanceCounter>();

            bool success = this.SetupPerformanceCounters();

            if (success)
            {
                // Timer for tracing performance counters.
                string timerId = string.Concat(
                    TraceType,
                    AzureBlobUploaderCountersReadTimerIdSuffix);
                this.azureBlobUploaderCountersReadTimer = new DcaTimer(
                    timerId,
                    this.ReadCounters,
                    TimeSpan.FromSeconds(ReadCountersTimerInterval));
            }
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            if (this.azureBlobUploaderCountersReadTimer != null)
            {
                // Stop the timer that triggers periodic read of azure blob uploader counters
                this.azureBlobUploaderCountersReadTimer.StopAndDispose();
                this.azureBlobUploaderCountersReadTimer.DisposedEvent.WaitOne();
            }

            GC.SuppressFinalize(this);
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
            this.readByteCount = 0;
            this.writeByteCount = 0;
            this.scheduleNextPass = true;
            this.blobUploadPassBeginTime = DateTime.Now.Ticks;

            if (this.azureBlobUploaderCountersReadTimer != null)
            {
                this.azureBlobUploaderCountersReadTimer.Start();
            }

            this.externalOperationTime[ExternalOperationTime.ExternalOperationType.BlobCopy].Reset();
        }

        internal void AzureBlobUploadPassEnd()
        {
            this.scheduleNextPass = false;
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

            if (blobUploadPassTimeSeconds > 0)
            {
                this.traceSource.WriteInfo(
                       TraceType,
                       "Function level counter {0}: {1}",
                       "IO Read Bytes/sec",
                       Math.Ceiling(this.readByteCount / blobUploadPassTimeSeconds));

                this.traceSource.WriteInfo(
                       TraceType,
                       "Function level counter {0}: {1}",
                       "IO Write Bytes/sec",
                       Math.Ceiling(this.writeByteCount / blobUploadPassTimeSeconds));
            }
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

        internal void BytesRead(long bytesRead)
        {
            this.readByteCount += bytesRead;
        }

        internal void BytesWritten(long bytesWritten)
        {
            this.writeByteCount += bytesWritten;
        }

        private void ReadCounters(object state)
        {
            this.ReadCounters();

            if (this.scheduleNextPass)
            {
                this.azureBlobUploaderCountersReadTimer.Start();
            }
        }

        private void ReadCounters()
        {
            foreach (var p in this.perfCountersList)
            {
                try
                {
                    this.traceSource.WriteInfo(
                        TraceType,
                        "Process level counter {0}: {1}",
                        p.CounterName,
                        Math.Ceiling(p.NextValue()));
                }
                catch (Exception ex)
                {
                    this.traceSource.WriteError(
                        TraceType,
                        "Failed to read next value of performance counter {0}. {1}",
                        p.CounterName,
                        ex);
                }
            }
        }

        private bool SetupPerformanceCounters()
        {
            bool success = false;

            try
            {
                var currentProcessName = Process.GetCurrentProcess().ProcessName;
                var pcc = new PerformanceCounterCategory("Process");
                var instanceNames = pcc.GetInstanceNames();
                var fabricDCAInstanceName = instanceNames.FirstOrDefault(i => i.StartsWith(currentProcessName));
                this.perfCountersList.Add(new PerformanceCounter("Process", "IO Read Bytes/sec", fabricDCAInstanceName, true));
                this.perfCountersList.Add(new PerformanceCounter("Process", "IO Write Bytes/sec", fabricDCAInstanceName, true));
                this.perfCountersList.Add(new PerformanceCounter("Process", "% Processor Time", fabricDCAInstanceName, true));
                this.perfCountersList.Add(new PerformanceCounter("Process", "Private Bytes", fabricDCAInstanceName, true));
                success = true;
            }
            catch (Exception ex)
            {
                this.traceSource.WriteError(
                       TraceType,
                       "Failed to set up performance counters. {0}",
                       ex);
            }

            return success;
        }
    }
}
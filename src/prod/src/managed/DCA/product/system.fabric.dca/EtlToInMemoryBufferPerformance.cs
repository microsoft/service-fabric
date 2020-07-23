// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common.Tracing;
    using System.Linq;

    // Class for gathering information related to the performance of writing
    // ETW events to an in-memory buffer
    internal class EtlToInMemoryBufferPerformance : IDisposable
    {
        #region Constants

        private const string TraceTypePerformance = "EtlToInMemoryBufferPerformance";

        private const double ReadCountersTimerInterval = 2.0;

        private const string EtlToInMemoryCountersReadTimerIdSuffix = "_EtlToInMemoryCountersReadTimer";

        #endregion

        // List of counters being monitored
        private readonly List<PerformanceCounter> perfCountersList;

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Timer to periodically query counters
        private readonly DcaTimer etlToInMemoryCountersReadTimer;

        // Counter to count the number of ETW events processed
        private ulong etwEventsProcessed;

        // Counter to count the number of ETW events written to the in-memory buffer
        private ulong etwEventsWrittenToInMemoryBuffer;

        // The beginning tick count for ETL processing pass
        private long etlPassBeginTime;

        // Whether or not the object has been disposed
        private bool disposed;

        // Determines whether to schedule timer for another pass
        private bool scheduleNextPass;

        // Read byte count
        private long readByteCount;

        // Write byte count
        private long writeByteCount;

        internal EtlToInMemoryBufferPerformance(FabricEvents.ExtensionsEvents traceSource, string logSourceId)
        {
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;
            this.perfCountersList = new List<PerformanceCounter>();

            bool success = this.SetupPerformanceCounters();

            if (success)
            {
                // Timer for tracing performance counters.
                string timerId = string.Concat(
                    this.logSourceId,
                    EtlToInMemoryCountersReadTimerIdSuffix);
                this.etlToInMemoryCountersReadTimer = new DcaTimer(
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

            if (this.etlToInMemoryCountersReadTimer != null)
            {
                // Stop the timer that triggers periodic read of Etl to in-memory writer counters
                this.etlToInMemoryCountersReadTimer.StopAndDispose();
                this.etlToInMemoryCountersReadTimer.DisposedEvent.WaitOne();
            }

            GC.SuppressFinalize(this);
        }

        internal void BytesRead(long bytesRead)
        {
            this.readByteCount += bytesRead;
        }

        internal void BytesWritten(long bytesWritten)
        {
            this.writeByteCount += bytesWritten;
        }

        internal void EtwEventProcessed()
        {
            this.etwEventsProcessed++;
        }

        internal void EtwEventWrittenToInMemoryBuffer()
        {
            this.etwEventsWrittenToInMemoryBuffer++;
        } 

        internal void EtlReadPassBegin()
        {
            this.scheduleNextPass = true;
            this.readByteCount = 0;
            this.writeByteCount = 0;
            this.etwEventsProcessed = 0;
            this.etwEventsWrittenToInMemoryBuffer = 0;
            this.etlPassBeginTime = DateTime.Now.Ticks;
            if (this.etlToInMemoryCountersReadTimer != null)
            {
                this.etlToInMemoryCountersReadTimer.Start();
            }
        }

        internal void EtlReadPassEnd()
        {
            this.scheduleNextPass = false;
            long etlPassEndTime = DateTime.Now.Ticks;
            double etlPassTimeSeconds = ((double)(etlPassEndTime - this.etlPassBeginTime)) / (10 * 1000 * 1000);

            this.traceSource.WriteInfo(
                TraceTypePerformance,
                "ETLPassInfo - {0}:{1},{2},{3}", // WARNING: The performance test parses this message,
                // so changing the message can impact the test.
                this.logSourceId,
                etlPassTimeSeconds,
                this.etwEventsProcessed,
                this.etwEventsWrittenToInMemoryBuffer);

            if (etlPassTimeSeconds > 0)
            {
                this.traceSource.WriteInfo(
                      TraceTypePerformance,
                      "Function level counter {0}: {1}",
                      "IO Read Bytes/sec",
                      Math.Ceiling(this.readByteCount / etlPassTimeSeconds));

                this.traceSource.WriteInfo(
                       TraceTypePerformance,
                       "Function level counter {0}: {1}",
                       "IO Write Bytes/sec",
                       Math.Ceiling(this.writeByteCount / etlPassTimeSeconds));
            }
        }

        private void ReadCounters(object state)
        {
            this.ReadCounters();

            if (this.scheduleNextPass)
            {
                this.etlToInMemoryCountersReadTimer.Start();
            }
        }

        private void ReadCounters()
        {
            foreach (var p in this.perfCountersList)
            {
                try
                {
                    this.traceSource.WriteInfo(
                        TraceTypePerformance,
                        "Process level counter {0}: {1}",
                        p.CounterName,
                        Math.Ceiling(p.NextValue()));
                }
                catch (Exception ex)
                {
                    this.traceSource.WriteError(
                        TraceTypePerformance,
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
                       TraceTypePerformance,
                       "Failed to set up performance counters. {0}",
                       ex);
            }

            return success;
        }
    }
}
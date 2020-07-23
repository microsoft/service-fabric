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

    internal class MdsFileProducerPerformance : IDisposable
    {
        #region Constants

        private const string TraceTypePerformance = "MdsFileProducerPerformance";

        private const double ReadCountersTimerInterval = 2.0;

        private const string MdsCountersReadTimerIdSuffix = "_MdsCountersReadTimer";

        #endregion

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Timer to periodically query counters
        private readonly DcaTimer mdsCountersReadTimer;

        // List of counters being monitored
        private readonly List<PerformanceCounter> perfCountersList;

        // Whether or not the object has been disposed
        private bool disposed;

        // Determines whether to schedule timer for another pass
        private bool scheduleNextPass;

        // Read byte count
        private long readByteCount;

        // Write byte count
        private long writeByteCount;

        // MDS write error count
        private long mdsWriteErrorCount;

        // MDS write success count
        private long mdsWriteSuccessCount;

        // Start time for pass
        private long passStartTime;

        internal MdsFileProducerPerformance()
        {
            this.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            this.perfCountersList = new List<PerformanceCounter>();

            bool success = this.SetupPerformanceCounters();

            if (success)
            {
                // Timer for tracing performance counters.
                string timerId = string.Concat(
                    TraceTypePerformance,
                    MdsCountersReadTimerIdSuffix);
                this.mdsCountersReadTimer = new DcaTimer(
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

            if (this.mdsCountersReadTimer != null)
            {
                // Stop the timer that triggers periodic read of MDS uploader counters
                this.mdsCountersReadTimer.StopAndDispose();
                this.mdsCountersReadTimer.DisposedEvent.WaitOne();
            }

            GC.SuppressFinalize(this);
        }

        internal void OnProcessingPeriodStart()
        {
            this.scheduleNextPass = true;
            this.readByteCount = 0;
            this.writeByteCount = 0;
            this.mdsWriteErrorCount = 0;
            this.passStartTime = DateTime.Now.Ticks;
            if (this.mdsCountersReadTimer != null)
            {
                this.mdsCountersReadTimer.Start();
            }
        }

        internal void OnProcessingPeriodStop()
        {
            this.scheduleNextPass = false;
            long passEndTime = DateTime.Now.Ticks;
            double etlPassTimeSeconds = ((double)(passEndTime - this.passStartTime)) / (10 * 1000 * 1000);

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

            this.traceSource.WriteInfo(
                       TraceTypePerformance,
                       "{0} records written to MDS table",
                       this.mdsWriteSuccessCount);

            this.traceSource.WriteInfo(
                       TraceTypePerformance,
                       "Failed to write {0} records to MDS table",
                       this.mdsWriteErrorCount);
        }

        internal void BytesRead(long bytesRead)
        {
            this.readByteCount += bytesRead;
        }

        internal void BytesWritten(long bytesWritten)
        {
            this.writeByteCount += bytesWritten;
        }

        internal void MdsWriteError()
        {
            this.mdsWriteErrorCount++;
        }

        internal void MdsWriteSuccess()
        {
            this.mdsWriteSuccessCount++;
        }

        private void ReadCounters(object state)
        {
            this.ReadCounters();

            if (this.scheduleNextPass)
            {
                this.mdsCountersReadTimer.Start();
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
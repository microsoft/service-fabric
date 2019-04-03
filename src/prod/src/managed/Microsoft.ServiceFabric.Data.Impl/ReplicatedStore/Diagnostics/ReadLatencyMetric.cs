// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ReplicatedStore.Diagnostics
{
    using Microsoft.ServiceFabric.Replicator.Diagnostics;
    using System;
    using System.Fabric;
    using System.Fabric.Common.Tracing;
    using System.Threading;

    internal class ReadLatencyMetric : IMetricReporter, ISamplingSuggester
    {
        private const long TicksPerMicrosecond = TimeSpan.TicksPerMillisecond / 1000;

        // totalReadLatencyInTicks and totalReads are used to calculate AvgReadLatency.
        private long totalReadLatencyInTicks = 0;
        private long totalReads = 0;

        // lastReadLatencyInTicks is used to get percentile metrics.
        private long lastReadLatencyInTicks = 0;

        private SamplingSuggester samplingSuggester = new SamplingSuggester();

        /// <summary>
        /// Updates the read latency counters.
        /// </summary>
        /// <param name="readTicks">New read latency record (ticks).</param>
        internal void Update(long readTicks)
        {
            this.lastReadLatencyInTicks = readTicks;
            Interlocked.Add(ref this.totalReadLatencyInTicks, readTicks);
            Interlocked.Increment(ref this.totalReads);
        }

        /// <summary>
        /// Reports to telemetry.
        /// </summary>
        /// <param name="context">Required trace parameters.</param>
        public void Report(StatefulServiceContext context)
        {
#if !DotNetCoreClr
            long totalReadTicks = GetTotalReadTicksAndReset(out long numberOfReads);
            if (numberOfReads > 0)
            {
                // This trace gets picked up by TelemetryEtwSink.
                FabricEvents.Events.ReadLatencyTelemetry(
                    context.PartitionId,
                    context.ReplicaId,
                    totalReadTicks / TicksPerMicrosecond,
                    numberOfReads,
                    this.lastReadLatencyInTicks / TicksPerMicrosecond);
            }
#endif
        }

        /// <summary>
        /// Gets total read latency (ticks) since this method was last called.
        /// Exposed for testability.
        /// </summary>
        /// <param name="numberOfReads">The number of reads since this method was last called.</param>
        /// <returns>Total latency in ticks since this method was last called.</returns>
        internal long GetTotalReadTicksAndReset(out long numberOfReads)
        {
            numberOfReads = Interlocked.Exchange(ref this.totalReads, 0);
            return (numberOfReads > 0) ? Interlocked.Exchange(ref this.totalReadLatencyInTicks, 0) : 0;
        }

        /// <summary>
        /// Advises this metric to return true on next suggestion requested.
        /// </summary>
        public void ResetSampling()
        {
            this.samplingSuggester.ResetSampling();
        }

        /// <summary>
        /// Returns a suggestion on whether a sample metric measurement should be taken.
        /// </summary>
        /// <returns>True if metric measurement is recommended. False otherwise.</returns>
        public bool GetSamplingSuggestion()
        {
            return this.samplingSuggester.GetSamplingSuggestion();
        }
    }
}

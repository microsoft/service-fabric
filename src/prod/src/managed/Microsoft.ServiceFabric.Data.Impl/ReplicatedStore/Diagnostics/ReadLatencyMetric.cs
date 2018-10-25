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

    internal class ReadLatencyMetric : IMetric, ISamplingSuggester
    {
        private long totalReadLatencyInTicks = 0;
        private long totalReads = 0;
        private bool samplingSuggested = true;

        /// <summary>
        /// Advises this metric to return true on next suggestion requested.
        /// </summary>
        public void ResetSampling()
        {
            samplingSuggested = true;
        }

        /// <summary>
        /// Returns a suggestion on whether a sample metric measurement should be taken.
        /// </summary>
        /// <returns>True if metric measurement is recommended. False otherwise.</returns>
        public bool GetSamplingSuggestion()
        {
            bool result = this.samplingSuggested;

            if (result == true)
            {
                this.samplingSuggested = false;
            }

            return result;
        }

        /// <summary>
        /// Updates the read latency counters.
        /// </summary>
        /// <param name="readTicks">New read latency record (ticks).</param>
        public void Update(long readTicks)
        {
            Interlocked.Add(ref this.totalReadLatencyInTicks, readTicks);
            Interlocked.Increment(ref this.totalReads);
        }

        /// <summary>
        /// Reports to telemetry.
        /// </summary>
        /// <param name="context">Required trace parameters.</param>
        public void Report(StatefulServiceContext context)
        {
            long totalReadTicks = GetTotalReadTicksAndReset(out long numberOfReads);

            if (numberOfReads > 0)
            {
                double avgReadLatencyInMs = (double)totalReadTicks / (numberOfReads * TimeSpan.TicksPerMillisecond);
#if !DotNetCoreClr
                // This trace gets picked up by TelemetryEtwSink.
                FabricEvents.Events.ReadLatency(
                    context.PartitionId,
                    context.ReplicaId,
                    avgReadLatencyInMs,
                    numberOfReads);
#endif
            }
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
    }
}

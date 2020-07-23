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

    internal class ReadWriteMetric : IMetricReporter, ISamplingSuggester
    {
        private long numberOfReads = 0;
        private long numberOfWrites = 0;
        private SamplingSuggester samplingSuggester = new SamplingSuggester();

        internal void RegisterRead()
        {
            Interlocked.Increment(ref this.numberOfReads);
        }

        internal void RegisterWrite()
        {
            Interlocked.Increment(ref this.numberOfWrites);
        }

        /// <summary>
        /// Reports to telemetry.
        /// </summary>
        /// <param name="context">Required trace parameters.</param>
        public void Report(StatefulServiceContext context)
        {
#if !DotNetCoreClr
            if (this.numberOfReads + this.numberOfWrites > 0)
            {
                var reads = Interlocked.Exchange(ref this.numberOfReads, 0);
                var writes = Interlocked.Exchange(ref this.numberOfWrites, 0);

                FabricEvents.Events.ReadWriteTelemetry(
                        context.PartitionId,
                        context.ReplicaId,
                        reads,
                        writes);
            }
#endif
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

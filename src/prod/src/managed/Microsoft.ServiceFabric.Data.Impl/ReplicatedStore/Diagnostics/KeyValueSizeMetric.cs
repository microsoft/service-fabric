// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ReplicatedStore.Diagnostics
{
    using Microsoft.ServiceFabric.Replicator;
    using Microsoft.ServiceFabric.Replicator.Diagnostics;
    using System.Fabric;
    using System.Fabric.Common.Tracing;
    using System.Threading;

    internal class KeyValueSizeMetric : IMetricReporter, ISamplingSuggester
    {
        private long totalKeyBytes = 0;
        private long totalValueBytes = 0;
        private long totalAddCount = 0;
        private long totalUpdateCount = 0;
        private SamplingSuggester samplingSuggester = new SamplingSuggester();
        private ITransactionalReplicator replicator;

        internal KeyValueSizeMetric(ITransactionalReplicator replicator)
        {
            this.replicator = replicator;
        }

        /// <summary>
        /// Adds key-value size data to the counter.
        /// </summary>
        /// <param name="keyBytes">Key size in bytes.</param>
        /// <param name="valueBytes">Value size in bytes.</param>
        internal void Add(int keyBytes, int valueBytes)
        {
            Interlocked.Add(ref this.totalKeyBytes, keyBytes);
            Interlocked.Add(ref this.totalValueBytes, valueBytes);
            Interlocked.Increment(ref this.totalAddCount);
        }

        /// <summary>
        /// Updates the key-value size counter.
        /// </summary>
        /// <param name="valueBytes">New value size in bytes.</param>
        internal void Update(int valueBytes)
        {
            Interlocked.Add(ref this.totalValueBytes, valueBytes);
            Interlocked.Increment(ref this.totalUpdateCount);
        }

        /// <summary>
        /// Reports to telemetry.
        /// </summary>
        /// <param name="context">Required trace parameters.</param>
        public void Report(StatefulServiceContext context)
        {
#if !DotNetCoreClr
            if (this.replicator.Role == ReplicaRole.Primary)
            {
                var addCount = Interlocked.Exchange(ref this.totalAddCount, 0);
                var updateCount = Interlocked.Exchange(ref this.totalUpdateCount, 0);

                if (addCount + updateCount > 0)
                {
                    var totalKeySize = (addCount > 0) ? Interlocked.Exchange(ref this.totalKeyBytes, 0) : 0;
                    var totalValueSize = Interlocked.Exchange(ref this.totalValueBytes, 0);

                    FabricEvents.Events.KeyValueSizeTelemetry(
                        context.PartitionId,
                        context.ReplicaId,
                        totalKeySize,
                        addCount,
                        totalValueSize,
                        addCount + updateCount);
                }
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

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

    internal class ItemCountMetric : IMetricReporter
    {
        private long totalItemCount = 0;
        private ITransactionalReplicator replicator;

        internal ItemCountMetric(ITransactionalReplicator replicator)
        {
            this.replicator = replicator;
        }

        /// <summary>
        /// Updates the counter for the item count.
        /// </summary>
        /// <param name="deltaItemCount">Change in item count since last update.</param>
        internal void Add(long deltaItemCount)
        {
            Interlocked.Add(ref this.totalItemCount, deltaItemCount);
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
                // This trace gets picked up by TelemetryEtwSink.
                FabricEvents.Events.ItemCountTelemetry(
                    context.PartitionId,
                    context.ReplicaId,
                    totalItemCount);
            }
#endif
        }
    }
}

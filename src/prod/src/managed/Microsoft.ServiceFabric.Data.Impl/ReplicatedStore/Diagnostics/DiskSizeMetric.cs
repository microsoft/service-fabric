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

    internal class DiskSizeMetric : IMetricReporter
    {
        private long totalDiskBytes = 0;
        private ITransactionalReplicator replicator;

        internal DiskSizeMetric(ITransactionalReplicator replicator)
        {
            this.replicator = replicator;
        }

        /// <summary>
        /// Adds to the disk size counter.
        /// </summary>
        /// <param name="deltaDiskBytes">Change in disk bytes since last update.</param>
        internal void Add(long deltaDiskBytes)
        {
            Interlocked.Add(ref this.totalDiskBytes, deltaDiskBytes);
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
                FabricEvents.Events.DiskSizeTelemetry(
                    context.PartitionId,
                    context.ReplicaId,
                    totalDiskBytes);
            }
#endif
        }
    }
}

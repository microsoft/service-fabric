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

    internal class StoreCountMetric : IMetricReporter
    {
        private int count = 0;
        private ITransactionalReplicator replicator;

        internal StoreCountMetric(ITransactionalReplicator replicator)
        {
            this.replicator = replicator;
        }

        internal void RegisterStore()
        {
            Interlocked.Increment(ref this.count);
        }

        internal void UnregisterStore()
        {
            Interlocked.Decrement(ref this.count);
        }

        /// <summary>
        /// Reports to telemetry.
        /// </summary>
        /// <param name="context">Required trace parameters.</param>
        public void Report(StatefulServiceContext context)
        {
#if !DotNetCoreClr
            if (replicator.Role == ReplicaRole.Primary)
            {
                FabricEvents.Events.StoreCountTelemetry(
                    context.PartitionId,
                    context.ReplicaId,
                    this.count);
            }
#endif
        }
    }
}

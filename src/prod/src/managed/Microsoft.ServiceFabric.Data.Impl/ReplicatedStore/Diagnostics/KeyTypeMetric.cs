// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ReplicatedStore.Diagnostics
{
    using Microsoft.ServiceFabric.Replicator;
    using Microsoft.ServiceFabric.Replicator.Diagnostics;
    using System;
    using System.Fabric;
    using System.Fabric.Common.Tracing;
    using System.Threading;

    internal class KeyTypeMetric : IMetricReporter
    {
        private int customTypeKeyStores = 0;
        private int nonCustomTypeKeyStores = 0;
        private ITransactionalReplicator replicator;

        internal KeyTypeMetric(ITransactionalReplicator replicator)
        {
            this.replicator = replicator;
        }

        internal void RegisterKeyType(Type t)
        {
            if (IsCustomKeyType(t))
            {
                Interlocked.Increment(ref customTypeKeyStores);
            }
            else
            {
                Interlocked.Increment(ref nonCustomTypeKeyStores);
            }
        }

        internal void UnregisterKeyType(Type t)
        {
            if (IsCustomKeyType(t))
            {
                Interlocked.Decrement(ref this.customTypeKeyStores);
            }
            else
            {
                Interlocked.Decrement(ref this.nonCustomTypeKeyStores);
            }
        }

        private bool IsCustomKeyType(Type t)
        {
            return !(t.IsPrimitive || t.IsEnum || t == typeof(string) || t == typeof(Guid));
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
                // These traces gets picked up by TelemetryEtwSink.
                if (this.customTypeKeyStores > 0)
                {
                    FabricEvents.Events.KeyTypeTelemetry(
                        context.PartitionId,
                        context.ReplicaId,
                        100.0);
                }
                else if (this.nonCustomTypeKeyStores > 0)
                {
                    FabricEvents.Events.KeyTypeTelemetry(
                        context.PartitionId,
                        context.ReplicaId,
                        0.0);
                }
            }
#endif
        }
    }
}

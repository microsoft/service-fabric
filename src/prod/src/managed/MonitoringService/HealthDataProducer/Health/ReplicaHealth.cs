// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Health
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Health;

    /// <summary>
    /// Extensible wrapper for System.Fabric.Health.ReplicaHealth to support testability.
    /// </summary>
    internal class ReplicaHealth : EntityHealth
    {
        internal ReplicaHealth(Guid partitionId, long replicaId, HealthState aggregatedHealthState)
            : base(aggregatedHealthState)
        {
            this.PartitionId = partitionId;
            this.ReplicaId = replicaId;
        }

        internal ReplicaHealth(
            Guid partitionId,
            long replicaId,
            HealthState aggregatedHealthState,
            IEnumerable<EntityHealthEvent> healthEvents,
            IList<System.Fabric.Health.HealthEvaluation> unhealthyEvaluations)
            : base(aggregatedHealthState, healthEvents, unhealthyEvaluations)
        {
            this.PartitionId = partitionId;
            this.ReplicaId = replicaId;
        }

        internal ReplicaHealth(System.Fabric.Health.ReplicaHealth health)
            : base(health)
        {
            this.PartitionId = health.PartitionId;
            this.ReplicaId = health.Id;
        }

        internal virtual Guid PartitionId { get; }

        internal virtual long ReplicaId { get; private set; }
    }
}
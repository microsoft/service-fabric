// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Health
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Health;
    using System.Linq;

    /// <summary>
    /// Extensible wrapper for System.Fabric.Health.PartitionHealth to support testability.
    /// </summary>
    internal class PartitionHealth : EntityHealth
    {
        internal PartitionHealth(Guid partitionId, HealthState aggregatedHealthState)
            : base(aggregatedHealthState)
        {
            this.PartitionId = partitionId;
        }

        internal PartitionHealth(
            Guid partitionId,
            HealthState aggregatedHealthState,
            IEnumerable<EntityHealthEvent> healthEvents,
            IList<System.Fabric.Health.HealthEvaluation> unhealthyEvaluations)
            : base(aggregatedHealthState, healthEvents, unhealthyEvaluations)
        {
            this.PartitionId = partitionId;
        }

        internal PartitionHealth(System.Fabric.Health.PartitionHealth health)
            : base(health)
        {
            this.PartitionId = health.PartitionId;
            this.ReplicaHealthStates = health.ReplicaHealthStates
                .Select(state => new ReplicaHealthState(
                    state.PartitionId,
                    state.Id, 
                    state.AggregatedHealthState));
        }

        internal virtual Guid PartitionId { get; private set; }

        internal virtual IEnumerable<ReplicaHealthState> ReplicaHealthStates { get; }
    }
}
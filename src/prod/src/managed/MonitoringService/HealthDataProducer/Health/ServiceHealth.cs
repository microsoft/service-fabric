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
    /// Extensible wrapper for System.Fabric.Health.ServiceHealth to support testability.
    /// </summary>
    internal class ServiceHealth : EntityHealth
    {
        internal ServiceHealth(Uri serviceName, HealthState aggregatedHealthState)
            : base(aggregatedHealthState)
        {
            this.ServiceName = serviceName;
        }

        internal ServiceHealth(
            Uri serviceName,
            HealthState aggregatedHealthState,
            IEnumerable<EntityHealthEvent> healthEvents,
            IList<System.Fabric.Health.HealthEvaluation> unhealthyEvaluations)
            : base(aggregatedHealthState, healthEvents, unhealthyEvaluations)
        {
            this.ServiceName = serviceName;
        }

        internal ServiceHealth(System.Fabric.Health.ServiceHealth health)
            : base(health)
        {
            this.ServiceName = health.ServiceName;
            this.PartitionHealthStates = health.PartitionHealthStates
                .Select(state => new PartitionHealthState(
                    state.PartitionId, 
                    state.AggregatedHealthState));
        }

        internal virtual Uri ServiceName { get; private set; }

        internal virtual IEnumerable<PartitionHealthState> PartitionHealthStates { get; private set; }
    }
}
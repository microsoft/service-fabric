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
    /// Extensible wrapper for System.Fabric.Health.ApplicationHealth to support testability.
    /// </summary>
    internal class ApplicationHealth : EntityHealth
    {
        internal ApplicationHealth(
            Uri appUri,
            HealthState aggregatedHealthState)
            : base(aggregatedHealthState)
        {
            this.ApplicationUri = appUri;
        }

        internal ApplicationHealth(
            Uri appUri,
            HealthState aggregatedHealthState,
            IEnumerable<EntityHealthEvent> healthEvents,
            IList<System.Fabric.Health.HealthEvaluation> unhealthyEvaluations)
            : base(aggregatedHealthState, healthEvents, unhealthyEvaluations)
        {
            this.ApplicationUri = appUri;
        }

        internal ApplicationHealth(System.Fabric.Health.ApplicationHealth health)
            : base(health)
        {
            this.ApplicationUri = health.ApplicationName;

            this.DeployedApplicationHealthStates = health.DeployedApplicationHealthStates
                .Select(state => new DeployedApplicationHealthState(
                    state.ApplicationName.OriginalString,
                    state.NodeName,
                    state.AggregatedHealthState));

            this.ServiceHealthStates = health.ServiceHealthStates
                .Select(state => new ServiceHealthState(
                    state.ServiceName,
                    state.AggregatedHealthState));
        }

        internal virtual Uri ApplicationUri { get; private set; }

        internal virtual IEnumerable<DeployedApplicationHealthState> DeployedApplicationHealthStates { get; private set; }

        internal virtual IEnumerable<ServiceHealthState> ServiceHealthStates { get; private set; }
    }
}
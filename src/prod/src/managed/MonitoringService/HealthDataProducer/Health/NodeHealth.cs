// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Health
{
    using System.Collections.Generic;
    using System.Fabric.Health;

    /// <summary>
    /// Extensible wrapper for System.Fabric.Health.NodeHealth to support testability.
    /// </summary>
    internal class NodeHealth : EntityHealth
    {
        internal NodeHealth(string nodeName, HealthState aggregatedHealthState)
            : base(aggregatedHealthState)
        {
            this.NodeName = nodeName;
        }

        internal NodeHealth(
            string nodeName,
            HealthState aggregatedHealthState,
            IEnumerable<EntityHealthEvent> healthEvents,
            IList<System.Fabric.Health.HealthEvaluation> unhealthyEvaluations)
            : base(aggregatedHealthState, healthEvents, unhealthyEvaluations)
        {
            this.NodeName = nodeName;
        }

        internal NodeHealth(System.Fabric.Health.NodeHealth health) : base(health)
        {
            this.NodeName = health.NodeName;
        }

        internal virtual string NodeName { get; private set; }
    }
}
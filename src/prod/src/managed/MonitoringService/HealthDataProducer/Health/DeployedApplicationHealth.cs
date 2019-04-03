// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Health
{
    using System.Collections.Generic;
    using System.Fabric.Health;
    using System.Linq;

    /// <summary>
    /// Extensible wrapper for System.Fabric.Health.DeployedApplicationHealth to support testability.
    /// </summary>
    internal class DeployedApplicationHealth : EntityHealth
    {
        internal DeployedApplicationHealth(string appName, string nodeName, HealthState aggregatedHealthState)
            : base(aggregatedHealthState)
        {
            this.ApplicationName = appName;
            this.NodeName = nodeName;
        }

        internal DeployedApplicationHealth(
            string appName,
            string nodeName,
            HealthState aggregatedHealthState,
            IEnumerable<EntityHealthEvent> healthEvents,
            IList<System.Fabric.Health.HealthEvaluation> unhealthyEvaluations)
            : base(aggregatedHealthState, healthEvents, unhealthyEvaluations)
        {
            this.ApplicationName = appName;
            this.NodeName = nodeName;
        }

        internal DeployedApplicationHealth(
            System.Fabric.Health.DeployedApplicationHealth health)
            : base(health)
        {
            this.ApplicationName = health.ApplicationName.OriginalString;
            this.NodeName = health.NodeName;
            this.DeployedServicePackageHealthStates =
                health.DeployedServicePackageHealthStates
                .Select(state => new DeployedServicePackageHealthState(
                    state.ApplicationName.OriginalString,
                    state.ServiceManifestName,
                    state.NodeName,
                    state.AggregatedHealthState));
        }

        internal string ApplicationName { get; }

        internal string NodeName { get; }

        internal IEnumerable<DeployedServicePackageHealthState> DeployedServicePackageHealthStates { get; }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Health
{
    using System.Collections.Generic;
    using System.Fabric.Health;

    /// <summary>
    /// Extensible wrapper for System.Fabric.Health.DeployedServicePackageHealth to support testability.
    /// </summary>
    internal class DeployedServicePackageHealth : EntityHealth
    {
        internal DeployedServicePackageHealth(
            string appName,
            string serviceName,
            string nodeName,
            HealthState aggregatedHealthState)
            : base(aggregatedHealthState)
        {
            this.ApplicationName = appName;
            this.ServiceManifestName = serviceName;
            this.NodeName = nodeName;
        }

        internal DeployedServicePackageHealth(
            string appName,
            string serviceName,
            string nodeName,
            HealthState aggregatedHealthState,
            IEnumerable<EntityHealthEvent> healthEvents,
            IList<System.Fabric.Health.HealthEvaluation> unhealthyEvaluations)
            : base(aggregatedHealthState, healthEvents, unhealthyEvaluations)
        {
            this.ApplicationName = appName;
            this.ServiceManifestName = serviceName;
            this.NodeName = nodeName;
        }

        internal DeployedServicePackageHealth(
            System.Fabric.Health.DeployedServicePackageHealth health)
            : base(health)
        {
            this.ApplicationName = health.ApplicationName.OriginalString;
            this.ServiceManifestName = health.ServiceManifestName;
            this.NodeName = health.NodeName;
        }

        public string ApplicationName { get; private set; }

        public string ServiceManifestName { get; private set; }

        public string NodeName { get; private set; }
    }
}
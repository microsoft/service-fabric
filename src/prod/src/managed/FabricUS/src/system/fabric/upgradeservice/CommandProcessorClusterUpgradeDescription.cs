// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    public class CommandProcessorClusterUpgradeDescription
    {
        public bool? ForceRestart { get; set; }

        public TimeSpan? UpgradeReplicaSetCheckTimeout { get; set; }

        public TimeSpan? HealthCheckWaitDuration { get; set; }

        public TimeSpan? HealthCheckStableDuration { get; set; }

        public TimeSpan? HealthCheckRetryTimeout { get; set; }

        public TimeSpan? UpgradeTimeout { get; set; }

        public TimeSpan? UpgradeDomainTimeout { get; set; }

        public CommandProcessorClusterUpgradeHealthPolicy HealthPolicy { get; set; }

        public CommandProcessorClusterUpgradeDeltaHealthPolicy DeltaHealthPolicy { get; set; }
    }
}
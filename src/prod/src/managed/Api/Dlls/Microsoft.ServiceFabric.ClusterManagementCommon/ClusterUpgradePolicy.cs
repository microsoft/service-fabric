// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;

    public class ClusterUpgradePolicy
    {
        public bool OverrideUserUpgradePolicy { get; set; }

        public bool ForceRestart { get; set; }

        public TimeSpan UpgradeReplicaSetCheckTimeout { get; set; }

        public TimeSpan HealthCheckWaitDuration { get; set; }

        public TimeSpan HealthCheckStableDuration { get; set; }

        public TimeSpan HealthCheckRetryTimeout { get; set; }

        public TimeSpan UpgradeTimeout { get; set; }

        public TimeSpan UpgradeDomainTimeout { get; set; }

        public ClusterUpgradeHealthPolicy HealthPolicy { get; set; }

        public ClusterUpgradeDeltaHealthPolicy DeltaHealthPolicy { get; set; }

        public ClusterUpgradePolicy Clone()
        {
            return (ClusterUpgradePolicy)this.MemberwiseClone();
        }
    }
}
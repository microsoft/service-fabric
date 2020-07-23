// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    public enum ClusterUpgradeFailureReason
    {
        Unknown,
        Interrupted,
        HealthCheck,
        UpgradeDomainTimeout,
        OverallUpgradeTimeout,
        UpgradeServiceDown
    }
}
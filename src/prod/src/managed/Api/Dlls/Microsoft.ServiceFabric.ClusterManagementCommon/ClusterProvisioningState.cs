// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    public enum ClusterProvisioningState
    {
        Default,

        // ClusterResource intermediate states
        WaitingForNodes,
        Deploying,
        BaselineUpgrade,
        UpdatingUserConfiguration,
        UpdatingUserCertificate,
        UpdatingInfrastructure,
        EnforcingClusterVersion,
        UpgradeServiceUnreachable,
        Deleting,
        ScaleUp,
        ScaleDown,
        AutoScale,

        // Cluster terminal state
        Ready,
        Failed
    }
}
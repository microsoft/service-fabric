// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.Collections.Generic;

    internal interface IUpgradeStateActivator
    {
        BaselineClusterUpgradeStateBase CreateBaselineUpgradeState(
            IUserConfig csmConfig,
            IAdminConfig wrpConfig,
            ClusterNodeConfig nodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger);

        SimpleClusterUpgradeState CreateSimpleUpgradeState(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger);

        ClusterUpgradeStateBase CreateCertificateClusterUpgradeState(
            IUserConfig csmConfig,
            IAdminConfig wrpConfig,
            ClusterNodeConfig nodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger,
            HashSet<string> certsAdded,
            HashSet<string> certsRemoved);

        AutoScaleClusterUpgradeStateBase CreateAutoScaleClusterUpgradeStateBase(
            bool isUserInitiated,
            IUserConfig csmConfig,
            IAdminConfig wrpConfig,
            ClusterNodeConfig nodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger);

        ScaleUpClusterUpgradeStateBase CreateScaleUpClusterUpgradeStateBase(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            Dictionary<string, int> nodeDiffList,
            ITraceLogger traceLogger);

        ScaleDownClusterUpgradeStateBase CreateScaleDownClusterUpgradeStateBase(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            Dictionary<string, int> nodeDiffList,
            ITraceLogger traceLogger);

        SeedNodeUpgradeStateBase CreateSeedNodeUpgradeStateBase(
            List<string> disabledNodes,
            List<string> removedNodes,
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger);

        GatekeepingClusterUpgradeState CreateGatekeepingClusterUpgradeState(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger);
    }
}
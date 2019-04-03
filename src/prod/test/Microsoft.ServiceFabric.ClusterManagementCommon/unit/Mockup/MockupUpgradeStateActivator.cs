// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using System;
    using System.Collections.Generic;

    internal class MockupUpgradeStateActivator : IUpgradeStateActivator
    {
        public BaselineClusterUpgradeStateBase CreateBaselineUpgradeState(
            IUserConfig csmConfig,
            IAdminConfig wrpConfig,
            ClusterNodeConfig nodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger) 
        {
            return new MockupBaselineUpgradeState(csmConfig, wrpConfig, nodeConfig, clusterResource, traceLogger);
        }

        public SimpleClusterUpgradeState CreateSimpleUpgradeState(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger) 
        {
            return new MockupSimpleClusterUpgradeState(targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, traceLogger);
        }

        public ClusterUpgradeStateBase CreateCertificateClusterUpgradeState(
            IUserConfig csmConfig,
            IAdminConfig wrpConfig,
            ClusterNodeConfig nodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger,
            HashSet<string> certsAdded,
            HashSet<string> certsRemoved) 
        {
            return new MockupCertificateClusterUpgradeState(csmConfig, wrpConfig, nodeConfig, clusterResource, certsAdded, certsRemoved, traceLogger);
        }

        public AutoScaleClusterUpgradeStateBase CreateAutoScaleClusterUpgradeStateBase(
            bool isUserInitiated,
            IUserConfig csmConfig,
            IAdminConfig wrpConfig,
            ClusterNodeConfig nodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger) 
        {
            List<string> disabledNodes = new List<String>();
            List<string> removedNodes = new List<String>();
            return new MockupAutoScaleClusterUpgradeState(disabledNodes, removedNodes, csmConfig, wrpConfig, nodeConfig, clusterResource, traceLogger);
        }

        public ScaleUpClusterUpgradeStateBase CreateScaleUpClusterUpgradeStateBase(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            Dictionary<string, int> nodeDiffList,
            ITraceLogger traceLogger) 
        {
            return new MockupScaleUpClusterUpgradeState(targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, nodeDiffList, traceLogger);
        }

        public ScaleDownClusterUpgradeStateBase CreateScaleDownClusterUpgradeStateBase(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            Dictionary<string, int> nodeDiffList,
            ITraceLogger traceLogger) 
        {
            return new MockupScaleDownClusterUpgradeState(targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, nodeDiffList, traceLogger);
        }

        public SeedNodeUpgradeStateBase CreateSeedNodeUpgradeStateBase(
            List<string> disabledNodes,
            List<string> removedNodes,
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger) 
        {
            return new MockupSeedNodeUpgradeState(disabledNodes, removedNodes, targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, traceLogger);
        }

        public GatekeepingClusterUpgradeState CreateGatekeepingClusterUpgradeState(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger) 
        {
            return new MockupGatekeepingClusterUpgradeState(targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, traceLogger);
        }
    }
}
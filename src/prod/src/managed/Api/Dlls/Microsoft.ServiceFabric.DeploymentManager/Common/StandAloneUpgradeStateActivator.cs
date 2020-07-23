// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using System;
    using System.Collections.Generic;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Serialization;

    internal class StandAloneUpgradeStateActivator : IUpgradeStateActivator
    {
        public AutoScaleClusterUpgradeStateBase CreateAutoScaleClusterUpgradeStateBase(bool isUserInitiated, IUserConfig csmConfig, IAdminConfig wrpConfig, ClusterNodeConfig nodeConfig, ICluster clusterResource, ITraceLogger traceLogger)
        {
            return new StandAloneAutoScaleClusterUpgradeState(csmConfig, wrpConfig, nodeConfig, clusterResource, traceLogger);
        }

        public BaselineClusterUpgradeStateBase CreateBaselineUpgradeState(IUserConfig csmConfig, IAdminConfig wrpConfig, ClusterNodeConfig nodeConfig, ICluster clusterResource, ITraceLogger traceLogger)
        {
            return new StandAloneBaselineUpgradeState(csmConfig, wrpConfig, nodeConfig, clusterResource, traceLogger);
        }

        public ClusterUpgradeStateBase CreateCertificateClusterUpgradeState(IUserConfig csmConfig, IAdminConfig wrpConfig, ClusterNodeConfig nodeConfig, ICluster clusterResource, ITraceLogger traceLogger, HashSet<string> certsAdded, HashSet<string> certsRemoved)
        {
            return new StandAloneCertificateClusterUpgradeState(csmConfig, wrpConfig, nodeConfig, clusterResource, certsAdded, certsRemoved, traceLogger);
        }

        public GatekeepingClusterUpgradeState CreateGatekeepingClusterUpgradeState(IUserConfig targetCsmConfig, IAdminConfig targetWrpConfig, ClusterNodeConfig targetNodeConfig, ICluster clusterResource, ITraceLogger traceLogger)
        {
            throw new NotImplementedException();
        }

        public ScaleDownClusterUpgradeStateBase CreateScaleDownClusterUpgradeStateBase(IUserConfig targetCsmConfig, IAdminConfig targetWrpConfig, ClusterNodeConfig targetNodeConfig, ICluster clusterResource, Dictionary<string, int> nodeDiffList, ITraceLogger traceLogger)
        {
            throw new NotImplementedException();
        }

        public ScaleUpClusterUpgradeStateBase CreateScaleUpClusterUpgradeStateBase(IUserConfig targetCsmConfig, IAdminConfig targetWrpConfig, ClusterNodeConfig targetNodeConfig, ICluster clusterResource, Dictionary<string, int> nodeDiffList, ITraceLogger traceLogger)
        {
            throw new NotImplementedException();
        }

        public SeedNodeUpgradeStateBase CreateSeedNodeUpgradeStateBase(List<string> disabledNodes, List<string> removedNodes, IUserConfig targetCsmConfig, IAdminConfig targetWrpConfig, ClusterNodeConfig targetNodeConfig, ICluster clusterResource, ITraceLogger traceLogger)
        {
            return new StandAloneSeedNodeUpgradeState(disabledNodes, removedNodes, targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, traceLogger);
        }

        public SimpleClusterUpgradeState CreateSimpleUpgradeState(IUserConfig targetCsmConfig, IAdminConfig targetWrpConfig, ClusterNodeConfig targetNodeConfig, ICluster clusterResource, ITraceLogger traceLogger)
        {
            return new StandAloneSimpleClusterUpgradeState(targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, traceLogger);
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using System.Collections.Generic;
    using System.Linq;

    internal class MockupJsonModel
    {
        public string ClusterConfigurationVersion
        {
            get;
            set;
        }

        public string Name
        {
            get;
            set;
        }

        public string ApiVersion
        {
            get;
            set;
        }

        public List<NodeDescription> Nodes
        {
            get;
            set;
        }

        public MockupProperty Properties
        {
            get;
            set;
        }

        public MockupUserConfig GetUserConfig()
        {
            var userConfig = new MockupUserConfig();
            userConfig.TotalNodeCount = this.Nodes != null ? this.Nodes.Count : 0;
            userConfig.ClusterName = this.Name;
            userConfig.Version = new UserConfigVersion(this.ClusterConfigurationVersion);
            userConfig.IsScaleMin = this.Nodes != null ? this.Nodes.Count != this.Nodes.GroupBy(node => node.IPAddress).Count() : true;
            userConfig.EnableTelemetry = this.Properties.EnableTelemetry;
            userConfig.FabricSettings = this.Properties.FabricSettings;
            userConfig.DiagnosticsStorageAccountConfig = this.Properties.DiagnosticsStorageAccountConfig;
            userConfig.DiagnosticsStoreInformation = this.Properties.DiagnosticsStore;
            userConfig.Security = this.Properties.Security;
            userConfig.AutoupgradeEnabled = this.Properties.FabricClusterAutoupgradeEnabled ?? !userConfig.IsScaleMin;
            userConfig.NodeTypes = this.Properties.NodeTypes;
            userConfig.PrimaryNodeTypes = userConfig.NodeTypes != null ? userConfig.NodeTypes.FindAll(nt => nt.IsPrimary) : null;
            userConfig.TotalPrimaryNodeTypeNodeCount = this.Nodes != null ? this.Nodes.Count(node => userConfig.PrimaryNodeTypes.FirstOrDefault(nt => nt.Name == node.NodeTypeRef) != null) : 0;
            userConfig.ReliabilityLevel = ReliabilityLevelExtension.GetReliabilityLevel(userConfig.TotalPrimaryNodeTypeNodeCount);
            return userConfig;
        }

        public MockupClusterTopology GetClusterTopology()
        {
            var clusterTopology = new MockupClusterTopology()
            {
                Nodes = new Dictionary<string, NodeDescription>(),
            };
            this.Nodes.ForEach(node => clusterTopology.Nodes.Add(node.NodeName, node));
            return clusterTopology;
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;

    internal class MockupCluster : ICluster
    {
        private ClusterResourceStateMachine stateMachine;
        private Lazy<IFabricSettingsActivator> fabricSettingsActivator;

        public MockupCluster(IAdminConfig adminConfig, IUserConfig userConfig, IClusterTopology topology, ITraceLogger traceLogger)
        {
            this.TargetWrpConfig = adminConfig;
            this.TargetCsmConfig = userConfig;
            this.Topology = topology;
            this.ClusterManifestVersion = 0;
            this.SeedNodeSelector = new SeedNodeSelector(new TimeSpan(0, 1, 0), new MockupTraceLogger());
            this.SetStateMachine(traceLogger);
            this.SetGeneratorSettings();

            this.fabricSettingsActivator = new Lazy<IFabricSettingsActivator>(() =>
            {
                return new MockupFabricSettingsActivator();
            });

            this.NodeTypeNodeStatusList = new Dictionary<string, SortedList<string, NodeStatus>>();
            this.TargetCsmConfig.NodeTypes.ForEach(nodeType => this.NodeTypeNodeStatusList.Add(nodeType.Name, new SortedList<string, NodeStatus>()));

            foreach (var node in this.Topology.Nodes)
            {
                this.NodeTypeNodeStatusList[node.Value.NodeTypeRef].Add(
                    node.Key,
                    new NodeStatus()
                    {
                        NodeName = node.Value.NodeName,
                        InstanceId = 0,
                        NodeDeactivationIntent = WrpNodeDeactivationIntent.Invalid,
                        NodeState = NodeState.Enabled,
                        NodeType = node.Value.NodeTypeRef
                    });
            }
        }
        
        public ClusterState Current { get; set; }

        public ClusterUpgradeStateBase Pending { get; set; }

        public IUserConfig TargetCsmConfig { get; set; }

        public IAdminConfig TargetWrpConfig { get; set; }

        public ClusterNodeConfig TargetNodeConfig { get; set; }

        public IClusterTopology Topology { get; set; }

        public string LastUpgradeFailureTimestampUtc { get; set; }

        public bool TargetCsmConfigUpgradeFailed { get; set; }

        public bool TargetWrpConfigUpgradeFailed { get; set; }

        public ClusterUpgradeFailureReason WrpConfigUpgradeFailedReason { get; set; }

        public ClusterProvisioningState ClusterState { get; set; }

        public Dictionary<string, SortedList<string, NodeStatus>> NodeTypeNodeStatusList { get; set; }

        public ClusterUpgradeStateFactory UpgradeFactory { get; set; }

        public SeedNodeSelector SeedNodeSelector { get; set; }

        public ClusterManifestGeneratorSettings ClusterManifestGeneratorSettings { get; set; }

        public IFabricSettingsActivator FabricSettingsActivator
        {
            get
            {
                return this.fabricSettingsActivator.Value;
            }
        }

        public int ClusterManifestVersion { get; set; }

        public bool RunStateMachine(){
            return this.stateMachine.RunStateMachine();
        }

        public bool EnsureCurrentVersion(string clusterManifestVersion, string msiVersion) {
            return true;
        }

        public void ClusterUpgradeStarted() {
            
        }

        public void ClusterUpgradeCompleted() {
            this.stateMachine.ClusterUpgradeCompleted();
        }

        public void ClusterUpgradeRolledBackOrFailed(string upgradeFailureTimestampUtc, ClusterUpgradeFailureReason upgradeFailureReason) {
            this.stateMachine.ClusterUpgradeRolledBackOrFailed(upgradeFailureTimestampUtc, upgradeFailureReason);
        }

        public List<NodeStatus> UpdateNodesStatus(List<NodeStatus> nodesStatusUpdate) {
            return this.stateMachine.UpdateNodesStatus(nodesStatusUpdate);
        }

        public static ClusterManifestGeneratorSettings GenerateSettings(SettingsType wrpSettings, IClusterTopology topology)
        {
            var systemServicesForScale = new Dictionary<string, string>();
            var section = wrpSettings.Section.FirstOrDefault(item => item.Name == "SystemServicesForScale");
            if (section != null)
            {
                foreach (var parameter in section.Parameter)
                {
                    systemServicesForScale.Add(parameter.Name, parameter.Value);
                }
            }

            HashSet<string> uds = new HashSet<string>();
            HashSet<string> fds = new HashSet<string>();
            foreach (var node in topology.Nodes.Values)
            {
                uds.Add(node.FaultDomain);
                fds.Add(node.FaultDomain);
            }

            return new ClusterManifestGeneratorSettings()
            {
                AllowUnprotectedDiagnosticsStorageAccountKeys = false,
                ClusterConnectionEndpointPort = GetIntConfigurationSetting(wrpSettings, "ClusterConnectionEndpointPort", 9025),
                DataDeletionAgeInDaysForDiagnosticsDisabledClusters = GetIntConfigurationSetting(wrpSettings, "DataDeletionAgeInDaysForDiagnosticsDisabledClusters", 7),
                EndReservedPortNumber = 1000,
                StartReservedPortNumber = 2000,
                FDCount = fds.Count,
                UDCount = uds.Count,
                LeaseDriverEndpointPort = GetIntConfigurationSetting(wrpSettings, "LeaseDriverEndpointPort", 9026),
                MaxAllowedPortNumber = 65535,
                MaxClusterSize = 1000,
                MinAllowedPortNumber = 1,
                MinClusterSize = GetIntConfigurationSetting(wrpSettings, "MinClusterSize", 3),
                MinDynamicPortCount = GetIntConfigurationSetting(wrpSettings, "MinDynamicPortCount", 1), // Needs to be 255 or higher to actually change ephemeral port range
                ServiceConnectionEndpointPort = GetIntConfigurationSetting(wrpSettings, "ServiceConnectionEndpointPort", 9027),
                SystemServicesForScale = systemServicesForScale
            };
        }

        private static int GetIntConfigurationSetting(SettingsType settings, string parameterName, int defaultValue)
        {
            string value = GetConfigurationSetting(settings, parameterName, null);
            return value == null ? defaultValue : int.Parse(value);
        }

        private static string GetConfigurationSetting(SettingsType settings, string parameterName, string defaultValue)
        {
            if (settings == null)
            {
                return defaultValue;
            }

            var section = settings.Section.FirstOrDefault(item => item.Name == StringConstants.ConfigPackage.WrpSettingsSection);
            if (section == null)
            {
                return defaultValue;
            }

            var parameter = section.Parameter.FirstOrDefault(item => item.Name == parameterName);
            return parameter == null ? defaultValue : parameter.Name;
        }

        private void SetGeneratorSettings()
        {
            string wrpSettingsPath = Utility.FindFabricResourceFile(Constant.WrpSettingsFilename);
            SettingsType wrpSettings = System.Fabric.Interop.Utility.ReadXml<SettingsType>(wrpSettingsPath);
            this.ClusterManifestGeneratorSettings = GenerateSettings(wrpSettings, this.Topology);
        }

        private void SetStateMachine(ITraceLogger traceLogger)
        {
            var stateActivator = new MockupUpgradeStateActivator();
            var factory = new ClusterUpgradeStateFactory(this, stateActivator, traceLogger);
            this.stateMachine = new ClusterResourceStateMachine(this, factory, traceLogger);
            this.UpgradeFactory = factory;
        }
    }
}
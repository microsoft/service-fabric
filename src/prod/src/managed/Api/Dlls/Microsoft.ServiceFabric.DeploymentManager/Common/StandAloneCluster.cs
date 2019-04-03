// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Management.ServiceModel;
    using System.IO;
    using System.Linq;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager.Model;
    using Newtonsoft.Json;

    using DMConstants = Microsoft.ServiceFabric.DeploymentManager.Constants;

    [JsonObject(IsReference = true)]
    internal class StandAloneCluster : ICluster
    {
        // This property is deprecated. No code is consuming it. Keep it for backward data compatibility.
        [SuppressMessage("StyleCop.CSharp.MaintainabilityRules", "SA1401:FieldsMustBePrivate", Justification = "Avoid any potential risk of serialization or deserialization with existing live data.")]
        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1307:AccessibleFieldsMustBeginWithUpperCaseLetter", Justification = "Avoid any potential risk of serialization or deserialization with existing live data.")]
        [JsonProperty]
        public string configurationVersion;

        private ClusterResourceStateMachine stateMachine;
        private Lazy<IFabricSettingsActivator> fabricSettingsActivator;

        [JsonProperty]
        private string clusterId;

        [JsonConstructor]
        public StandAloneCluster()
        {
            //// This allows the StandAloneCluster instance to be created first during deserialization,
            //// so that its properties (e.g. Pending) which also references back to this instance can
            //// be then instantiated (constructor invoked) with this instance as one of its parameters.
            ITraceLogger logger = new StandAloneTraceLogger("StandAloneDeploymentManager");

            // TODO: This is an workaround for bug 8874388: SimpleClusterUpgradeState shouldn't reselect seednode. It should use the exisiting ones.
            this.SeedNodeSelector = new SeedNodeSelector(new TimeSpan(0, 0, 0), logger);
            this.SetStateMachine(logger);
            this.fabricSettingsActivator = new Lazy<IFabricSettingsActivator>(() =>
            {
                this.clusterId.MustNotBeNullOrWhiteSpace("clusterId");
                return new StandAloneFabricSettingsActivator(this.clusterId);
            });
        }
        
        public StandAloneCluster(IAdminConfig adminConfig, IUserConfig userConfig, IClusterTopology topology, string clusterId, ITraceLogger traceLogger)
        {
            this.TargetWrpConfig = adminConfig;
            this.TargetCsmConfig = userConfig;
            this.Topology = topology;
            this.ClusterManifestVersion = 0;

            // TODO: This is an workaround for bug 8874388: SimpleClusterUpgradeState shouldn't reselect seednode. It should use the exisiting ones.
            this.SeedNodeSelector = new SeedNodeSelector(new TimeSpan(0, 0, 0), new StandAloneTraceLogger("StandAloneDeploymentManager"));
            this.SetStateMachine(traceLogger);
            this.SetGeneratorSettings();

            this.clusterId = clusterId;

            this.fabricSettingsActivator = new Lazy<IFabricSettingsActivator>(() =>
            {
                this.clusterId.MustNotBeNullOrWhiteSpace("clusterId");
                return new StandAloneFabricSettingsActivator(this.clusterId);
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

        /// <summary>
        /// Provided by this class.
        /// </summary>
        public ClusterManifestGeneratorSettings ClusterManifestGeneratorSettings
        {
            get;
            set;
        }

        /// <summary>
        /// Provided by state machine.
        /// </summary>
        public ClusterProvisioningState ClusterState
        {
            get;
            set;
        }

        public ClusterState Current
        {
            get;
            set;
        }

        [JsonIgnore]
        public IFabricSettingsActivator FabricSettingsActivator
        {
            get
            {
                return this.fabricSettingsActivator.Value;
            }
        }

        public string LastUpgradeFailureTimestampUtc
        {
            get;
            set;
        }

        public ClusterUpgradeStateBase Pending
        {
            get;
            set;
        }

        public SeedNodeSelector SeedNodeSelector
        {
            get;
            set;
        }

        public IUserConfig TargetCsmConfig
        {
            get;
            set;
        }

        public bool TargetCsmConfigUpgradeFailed
        {
            get;
            set;
        }

        public ClusterNodeConfig TargetNodeConfig
        {
            get;
            set;
        }

        public IAdminConfig TargetWrpConfig
        {
            get;
            set;
        }

        public bool TargetWrpConfigUpgradeFailed
        {
            get;
            set;
        }

        public IClusterTopology Topology
        {
            get;
            set;
        }

        public ClusterUpgradeStateFactory UpgradeFactory
        {
            get;
            set;
        }

        public ClusterUpgradeFailureReason WrpConfigUpgradeFailedReason
        {
            get;
            set;
        }

        public int ClusterManifestVersion
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the deserialized string that has been persisted for this instance.
        /// </summary>
        [JsonIgnore]
        public string PersistedClusterString
        {
            get;
            set;
        }

        public Dictionary<string, SortedList<string, NodeStatus>> NodeTypeNodeStatusList
        {
            get;
            set;
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

        public void ClusterUpgradeCompleted()
        {
            this.stateMachine.ClusterUpgradeCompleted();
        }

        public void ClusterUpgradeRolledBackOrFailed(string upgradeFailureTimestampUtc, ClusterUpgradeFailureReason upgradeFailureReason)
        {
            this.stateMachine.ClusterUpgradeRolledBackOrFailed(upgradeFailureTimestampUtc, upgradeFailureReason);
        }

        /// <summary>
        /// To be called by upgrade service.
        /// </summary>
        public void ClusterUpgradeStarted()
        {
            throw new System.NotImplementedException();
        }

        public bool EnsureCurrentVersion(string clusterManifestVersion, string msiVersion)
        {
            return this.stateMachine.EnsureCurrentVersion(clusterManifestVersion, msiVersion);
        }

        public bool RunStateMachine()
        {
            return this.stateMachine.RunStateMachine();
        }

        public List<NodeStatus> UpdateNodesStatus(List<NodeStatus> nodesStatusUpdate)
        {
            return this.stateMachine.UpdateNodesStatus(nodesStatusUpdate);
        }

        internal void Reset()
        {
            this.TargetCsmConfigUpgradeFailed = false;
            this.TargetWrpConfigUpgradeFailed = false;
            this.TargetCsmConfig = null;
            this.TargetNodeConfig = null;
            this.TargetWrpConfig = null;
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
            string wrpSettingsPath = StandaloneUtility.FindFabricResourceFile(DMConstants.WrpSettingsFilename);
            SettingsType wrpSettings = System.Fabric.Interop.Utility.ReadXml<SettingsType>(wrpSettingsPath);
            this.ClusterManifestGeneratorSettings = GenerateSettings(wrpSettings, this.Topology);
        }

        private void SetStateMachine(ITraceLogger traceLogger)
        {
            var stateActivator = new StandAloneUpgradeStateActivator();
            var factory = new ClusterUpgradeStateFactory(this, stateActivator, traceLogger);
            this.stateMachine = new ClusterResourceStateMachine(this, factory, traceLogger);
            this.UpgradeFactory = factory;
        }
    }
}
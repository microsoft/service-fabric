// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Net;
    using System.Net.Sockets;
    using System.Linq;

    internal class UpdateNodeStateOperation : RestartOperation

    {
        protected List<NodeSettings> nodeSettings { get; private set; }

        protected override void OnExecuteOperation(DeploymentParameters parameters, ClusterManifestType clusterManifest, Infrastructure infrastructure)
        {
            this.manifest = clusterManifest;
            this.infrastructure = infrastructure;
            this.parameters = parameters;
            this.fabricValidator = new FabricValidatorWrapper(parameters, manifest, infrastructure);
            fabricValidator.ValidateAndEnsureDefaultImageStore();
            this.nodeSettings = GetNodeSettings();

            // creating a ClusterSettings without Unreliable Transport settings
            var clusterSettingsNoUnreliableTransport = new ClusterSettings(this.manifest.FabricSettings, GetVotes(), GetSeedNodes(), manifest.Certificates != null ? this.manifest.Certificates.SecretsCertificate : null);
            clusterSettingsNoUnreliableTransport.Settings.RemoveAll(x => x.Name == FabricValidatorConstants.SectionNames.UnreliableTransport);

            var clusterSettings = new ClusterSettings(this.manifest.FabricSettings, GetVotes(), GetSeedNodes(), manifest.Certificates != null ? this.manifest.Certificates.SecretsCertificate : null);

            this.UpdateClusterSettings(clusterSettingsNoUnreliableTransport);
            var fabricHostSettings = InitializeFabricHostSettings(clusterSettingsNoUnreliableTransport, nodeSettings);
            var servicesToBeEnabled = GetServicesToBeEnabled();

            for (int i = 0; i < nodeSettings.Count; i++)
            {
                NodeSettings nodeSetting = nodeSettings[i];
                List<string> servicesToBeDeployed = GetServicesToBeDeployed(i);
                DeploymentNode node = new DeploymentNode(nodeSetting, clusterSettings, infrastructure, servicesToBeDeployed.ToArray(), servicesToBeEnabled.ToArray(), this.manifest);

                if (string.IsNullOrEmpty(parameters.NodeName) || parameters.NodeName.Equals(nodeSetting.NodeName, StringComparison.OrdinalIgnoreCase))
                {
                    if (parameters.Operation == DeploymentOperations.Update || parameters.Operation == DeploymentOperations.UpdateInstanceId)
                    {
                        node.UpgradeDeployment();
                    }
                    else if (parameters.Operation == DeploymentOperations.Create)
                    {
                        node.CreateDeployment();  //update infrastructure manifest
                    }
                }

                //  Gateway service activation is managed by Fabric.exe, and so its settings shouldn't be written to FabricHostSettings.xml
                fabricHostSettings.AddRange(node.GetHostedServices().FindAll(
                    delegate(SettingsTypeSection section)
                    {
                        string hostedServiceName = string.Format(
                            CultureInfo.InvariantCulture,
                            Constants.SectionNames.HostSettingsSectionPattern,
                            nodeSetting.NodeName,
                            Constants.HttpGatewayService);

                        return section.Name != hostedServiceName;
                    }));
            }

            WriteFabricHostSettingsFile(parameters.DeploymentSpecification.GetDataRoot(), new SettingsType() { Section = fabricHostSettings.ToArray() }, parameters.MachineName);

            var currentType = this as CreateorUpdateOperation;
            if (currentType != null)
            {
                base.OnExecuteOperation(parameters, clusterManifest, infrastructure);
            }         
        }

        private void UpdateClusterSettings(ClusterSettings clusterSettingsNoUnreliableTransport)
        {
            var fabricHostSetting = clusterSettingsNoUnreliableTransport.Settings.FirstOrDefault(
                setting =>
                    setting.Name.Equals(FabricValidatorConstants.SectionNames.FabricHost, StringComparison.OrdinalIgnoreCase));
            if (fabricHostSetting == null)
            {
                return;
            }

            var autoFabricUpdateParameterValue = fabricHostSetting.Parameter.FirstOrDefault(
                param =>
                    param.Name.Equals(FabricValidatorConstants.ParameterNames.EnableServiceFabricAutomaticUpdates, StringComparison.OrdinalIgnoreCase) &&
                    param.Value.Equals("true", StringComparison.OrdinalIgnoreCase));

            var autoFabricBaseUpgradeParameterValue = fabricHostSetting.Parameter.FirstOrDefault(
                param =>
                    param.Name.Equals(FabricValidatorConstants.ParameterNames.EnableServiceFabricBaseUpgrade, StringComparison.OrdinalIgnoreCase) &&
                    param.Value.Equals("true", StringComparison.OrdinalIgnoreCase));

            var restartManagementParameterValue = fabricHostSetting.Parameter.FirstOrDefault(
                param =>
                    param.Name.Equals(FabricValidatorConstants.ParameterNames.EnableRestartManagement, StringComparison.OrdinalIgnoreCase) &&
                    param.Value.Equals("true", StringComparison.OrdinalIgnoreCase));

            if (autoFabricUpdateParameterValue == null &&
                autoFabricBaseUpgradeParameterValue == null &&
                restartManagementParameterValue == null)
            {
                return;
            }

            string targetReplicaSetSize = this.infrastructure.InfrastructureNodes.Count() < 3 ? "1" : "3";
            string minReplicaSetSize = this.infrastructure.InfrastructureNodes.Count() < 3 ? "1" : "2";
            var targetSetSizeSetting = new SettingsTypeSectionParameter()
            {
                Name = FabricValidatorConstants.ParameterNames.TargetReplicaSetSize,
                Value = targetReplicaSetSize
            };

            var minSetSizeSetting = new SettingsTypeSectionParameter()
            {
                Name = FabricValidatorConstants.ParameterNames.MinReplicaSetSize,
                Value = minReplicaSetSize
            };

            if (autoFabricBaseUpgradeParameterValue != null || autoFabricUpdateParameterValue != null)
            {
                List<SettingsTypeSectionParameter> parameters = new List<SettingsTypeSectionParameter>
                {
                    targetSetSizeSetting,
                    minSetSizeSetting,
                    new SettingsTypeSectionParameter()
                    {
                        Name = FabricValidatorConstants.ParameterNames.CoordinatorType,
                        Value = FabricValidatorConstants.WindowsUpdateServiceCoordinatorType
                    }
                };

                if (autoFabricBaseUpgradeParameterValue != null &&
                    autoFabricUpdateParameterValue == null)
                {
                    parameters.Add(
                        new SettingsTypeSectionParameter()
                        {
                            Name = FabricValidatorConstants.ParameterNames.OnlyBaseUpgrade,
                            Value = "true"
                        });
                }

                clusterSettingsNoUnreliableTransport.Settings.Add(
                    new SettingsTypeSection()
                    {
                        Name = FabricValidatorConstants.SectionNames.UpgradeService,
                        Parameter = parameters.ToArray()
                    });
            }

            if (restartManagementParameterValue != null)
            {
                clusterSettingsNoUnreliableTransport.Settings.Add(
                    new SettingsTypeSection()
                    {
                        Name = FabricValidatorConstants.SectionNames.RepairManager,
                        Parameter = new SettingsTypeSectionParameter[]
                        {
                            targetSetSizeSetting,
                            minSetSizeSetting,
                        }
                    });

                clusterSettingsNoUnreliableTransport.Settings.Add(
                    new SettingsTypeSection()
                    {
                        Name = FabricValidatorConstants.SectionNames.InfrastructureService,
                        Parameter = new SettingsTypeSectionParameter[]
                        {
                            targetSetSizeSetting,
                            minSetSizeSetting,
                            new SettingsTypeSectionParameter()
                            {
                                Name = FabricValidatorConstants.ParameterNames.CoordinatorType,
                                Value = FabricValidatorConstants.ServerRestartCoordinatorType
                            }
                        }
                    });
            }                       
        }

        private SettingsTypeSection GetVotes()
        {
            List<SettingsTypeSectionParameter> votes = this.infrastructure.InfrastructureVotes;
            var votesSection = this.manifest.FabricSettings.FirstOrDefault(section => section.Name.Equals(Constants.SectionNames.Votes, StringComparison.OrdinalIgnoreCase));
            if (votesSection != null && votesSection.Parameter != null)
            {
                foreach (SettingsOverridesTypeSectionParameter vote in votesSection.Parameter)
                {
                    votes.Add(new SettingsTypeSectionParameter() { Name = vote.Name, Value = vote.Value, MustOverride = false });
                }
            }

            return new SettingsTypeSection() { Name = Constants.SectionNames.Votes, Parameter = votes.ToArray() };
        }

        private SettingsTypeSection GetSeedNodes()
        {
            List<SettingsTypeSectionParameter> seedNodes = this.infrastructure.SeedNodeClientConnectionAddresses;
            var seedNodeSection = this.manifest.FabricSettings.FirstOrDefault(section => section.Name.Equals(Constants.SectionNames.SeedNodeClientConnectionAddresses, StringComparison.OrdinalIgnoreCase));
            if (seedNodeSection != null && seedNodeSection.Parameter != null)
            {
                foreach (SettingsOverridesTypeSectionParameter seedNode in seedNodeSection.Parameter)
                {
                    seedNodes.Add(new SettingsTypeSectionParameter() { Name = seedNode.Name, Value = seedNode.Value, MustOverride = false });
                }
            }

            return new SettingsTypeSection() { Name = Constants.SectionNames.SeedNodeClientConnectionAddresses, Parameter = seedNodes.ToArray() };
        }


        private static List<SettingsTypeSection> InitializeFabricHostSettings(ClusterSettings clusterSettings, List<NodeSettings> nodeSettings)
        {
            List<SettingsTypeSection> fabricHostSettings = new List<SettingsTypeSection>();
            fabricHostSettings.AddRange(clusterSettings.Settings);

            if (nodeSettings.Count > 0)
            {
                fabricHostSettings.AddRange(nodeSettings[0].Settings);
            }

            return fabricHostSettings;
        }

        private List<string> GetServicesToBeEnabled()
        {
            List<string> servicesToBeEnabled = new List<string>();

            servicesToBeEnabled.Add(Constants.FabricService);

            if (this.fabricValidator.IsDCAEnabled)
            {
                servicesToBeEnabled.Add(Constants.DCAService);
            }

            return servicesToBeEnabled;
        }

        private List<string> GetServicesToBeDeployed(int i)
        {
            List<string> servicesToBeDeployed = new List<string>();

            servicesToBeDeployed.Add(Constants.FabricService);

            if (!infrastructure.IsScaleMin || i == 0)
            {
                servicesToBeDeployed.Add(Constants.DCAService);
            }

            return servicesToBeDeployed;
        }
    }
}
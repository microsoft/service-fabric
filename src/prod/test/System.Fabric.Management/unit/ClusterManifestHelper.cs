// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.IO;
    using System.Linq;
    using System.Security.Cryptography.X509Certificates;

    public class ClusterManifestHelper
    {
        public enum InfrastructureType
        {
            WindowsServer,
            PaaS
        }

        public ClusterManifestType ClusterManifest;

        public InfrastructureInformationType InfrastructureManfiest;

        public ClusterManifestHelper(bool isWindowsServer, bool hasAllParameters)
        {
            GenerateValidClusterManifest(isWindowsServer ? InfrastructureType.WindowsServer : InfrastructureType.PaaS);
        }

        public ClusterManifestHelper(InfrastructureType infraType)
        {
            GenerateValidClusterManifest(infraType);
        }

        private void GenerateValidClusterManifest(InfrastructureType infaType)
        {
            this.ClusterManifest = new ClusterManifestType();
            this.ClusterManifest.Name = "TestClusterManifestAllSettings";
            this.ClusterManifest.Version = "1.0";
            this.ClusterManifest.Infrastructure = new ClusterManifestTypeInfrastructure();

            if (infaType == InfrastructureType.WindowsServer)
            {
                ClusterManifestTypeInfrastructureWindowsServer infra =
                    new ClusterManifestTypeInfrastructureWindowsServer();
                infra.IsScaleMin = false;
                infra.NodeList = new FabricNodeType[1];
                infra.NodeList[0] = new FabricNodeType()
                {
                    FaultDomain = "fd:/RACK1",
                    UpgradeDomain = "MYUD1",
                    NodeName = "Node1",
                    NodeTypeRef = "NodeType1",
                    IPAddressOrFQDN = "localhost",
                    IsSeedNode = true
                };

                this.ClusterManifest.Infrastructure.Item = infra;
            }
            else
            {
                ClusterManifestTypeInfrastructurePaaS infra = new ClusterManifestTypeInfrastructurePaaS();
                infra.Roles = new PaaSRoleType[]
                {
                    new PaaSRoleType() {NodeTypeRef = "NodeType1", RoleName = "NodeType1", RoleNodeCount = 5}
                };

                infra.Votes = new PaaSVoteType[]
                {
                    new PaaSVoteType() {IPAddressOrFQDN = "10.0.0.1", NodeName = "_NodeType1_0", Port = 19005},
                    new PaaSVoteType() {IPAddressOrFQDN = "10.0.0.2", NodeName = "_NodeType1_1", Port = 19005},
                    new PaaSVoteType() {IPAddressOrFQDN = "10.0.0.3", NodeName = "_NodeType1_2", Port = 19005}
                };

                this.ClusterManifest.Infrastructure.Item = infra;
            }

            
            this.ClusterManifest.NodeTypes = new ClusterManifestTypeNodeType[1];
            this.ClusterManifest.NodeTypes[0] = new ClusterManifestTypeNodeType();
            this.ClusterManifest.NodeTypes[0].Name = "NodeType1";
            this.ClusterManifest.NodeTypes[0].Endpoints = new FabricEndpointsType()
            {
                ClientConnectionEndpoint = new InputEndpointType() { Port = "19000" },
                LeaseDriverEndpoint = new InternalEndpointType() { Port = "19001" },
                ClusterConnectionEndpoint = new InternalEndpointType() { Port = "19002" },
                ServiceConnectionEndpoint = new InternalEndpointType() { Port = "19003" },
                HttpGatewayEndpoint = new InputEndpointType() { Port = "19004" },
                ApplicationEndpoints = new FabricEndpointsTypeApplicationEndpoints() { StartPort = 30001, EndPort = 31000 },
            };

            this.ClusterManifest.FabricSettings = new SettingsOverridesTypeSection[3];
            List<SettingsOverridesTypeSectionParameter> parameters = new List<SettingsOverridesTypeSectionParameter>();
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ClusterCredentialType, Value = "None", IsEncrypted = false });
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ServerAuthCredentialType, Value = "None", IsEncrypted = false });
            this.ClusterManifest.FabricSettings[0] = new SettingsOverridesTypeSection() { Name = FabricValidatorConstants.SectionNames.Security, Parameter = parameters.ToArray() };
            parameters = new List<SettingsOverridesTypeSectionParameter>();
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = "ExpectedClustersize", Value = "1", IsEncrypted = false });
            this.ClusterManifest.FabricSettings[1] = new SettingsOverridesTypeSection() { Name = "FailoverManager", Parameter = parameters.ToArray() };
            parameters = new List<SettingsOverridesTypeSectionParameter>();
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = "ImageStoreConnectionString", Value = "_default_", IsEncrypted = false });
            this.ClusterManifest.FabricSettings[2] = new SettingsOverridesTypeSection() { Name = "Management", Parameter = parameters.ToArray() };
        }

        public void AddToOrModifyFabricSettings(string sectionName, string parameterName, string parameterValue, bool isEncrypted)
        {
            SettingsOverridesTypeSection matchingSection = this.ClusterManifest.FabricSettings.FirstOrDefault(section => section.Name == sectionName);
            if (matchingSection == null)
            {
                Collection<SettingsOverridesTypeSection> sections = new Collection<SettingsOverridesTypeSection>();
                foreach (var section in this.ClusterManifest.FabricSettings)
                {
                    sections.Add(section);
                }

                matchingSection = new SettingsOverridesTypeSection() { Name = sectionName, Parameter = new SettingsOverridesTypeSectionParameter[0] };
                sections.Add(matchingSection);

                this.ClusterManifest.FabricSettings = sections.ToArray();
            }

            SettingsOverridesTypeSectionParameter matchingParameter = matchingSection.Parameter.FirstOrDefault(parameter => parameter.Name == parameterName);
            if (matchingParameter == null)
            {
                Collection<SettingsOverridesTypeSectionParameter> parameters = new Collection<SettingsOverridesTypeSectionParameter>();
                foreach (var parameter in matchingSection.Parameter)
                {
                    parameters.Add(parameter);
                }

                matchingParameter = new SettingsOverridesTypeSectionParameter() { Name = parameterName, Value = parameterValue, IsEncrypted = isEncrypted };
                parameters.Add(matchingParameter);

                matchingSection.Parameter = parameters.ToArray();
            }
            else
            {
                matchingParameter.Value = parameterValue;
                matchingParameter.IsEncrypted = isEncrypted;
            }
        }

        private void AddAllParameters()
        {
            var lines = File.ReadAllLines(Environment.ExpandEnvironmentVariables("%_NTTREE%\\configurations.csv"));
            var sections = new List<SettingsOverridesTypeSection>();
            var parameters = new List<SettingsOverridesTypeSectionParameter>();
            var prevSection = "";
            foreach (var line in lines)
            {
                var configInfo = line.Split(',');
                var sectionName = configInfo[0].Trim();
                var parameterName = configInfo[1].Trim();
                var value = configInfo[2].Trim();
                if (sectionName != prevSection)
                {
                    if (!string.IsNullOrEmpty(prevSection))
                    {
                        sections.Add(new SettingsOverridesTypeSection() { Name = prevSection, Parameter = parameters.ToArray() });
                    }
                    parameters = new List<SettingsOverridesTypeSectionParameter>();
                    prevSection = sectionName;
                }
                parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = parameterName, Value = value, IsEncrypted = false });
            }
            sections.Add(new SettingsOverridesTypeSection() { Name = prevSection, Parameter = parameters.ToArray() });
        }
    }
}
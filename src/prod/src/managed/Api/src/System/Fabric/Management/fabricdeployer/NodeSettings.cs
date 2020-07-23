// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using Strings;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Globalization;

    /// <summary>
    /// This class stores the settings for a single node.
    /// </summary>
    internal class NodeSettings
    {
        #region Public properties
        public string NodeName { get; private set; }

        public string IPAddressOrFQDN { get; private set; }

        public List<SettingsTypeSection> Settings { get; private set; }

        public DeploymentFolders DeploymentFoldersInfo { get; private set; }
        #endregion

        #region Constructors
        public NodeSettings(FabricNodeType fabricNode, ClusterManifestTypeNodeType nodeType, DeploymentFolders deploymentFolders, bool isScaleMin, FabricEndpointsType endpoints)
        {
            this.Settings = new List<SettingsTypeSection>();
            this.NodeName = fabricNode.NodeName;
            this.IPAddressOrFQDN = fabricNode.IPAddressOrFQDN;
            this.DeploymentFoldersInfo = deploymentFolders;
            AddFabricNodeSection(fabricNode, endpoints, nodeType, isScaleMin);
            AddDomainIds(fabricNode);
            AddNodeProperties(nodeType);
        }
        #endregion

        #region Private Functions
        private void AddFabricNodeSection(FabricNodeType fabricNode, FabricEndpointsType endpoints, ClusterManifestTypeNodeType nodeType, bool isScaleMin)
        {
            CertificatesType certificates = nodeType.Certificates;

            List<SettingsTypeSectionParameter> parameters = new List<SettingsTypeSectionParameter>();
            parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.InstanceName, Value = this.NodeName, MustOverride = false });

            if (Helpers.isIPV6AddressAndNoBracket(this.IPAddressOrFQDN))
            {
                parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.IPAddressOrFQDN, Value = Helpers.AddBracketsAroundIPV6(this.IPAddressOrFQDN), MustOverride = false });
            }
            else
            {
                parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.IPAddressOrFQDN, Value = this.IPAddressOrFQDN, MustOverride = false });
            }

            parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.WorkingDir, Value = this.DeploymentFoldersInfo.WorkFolder, MustOverride = false });
            parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.NodeVersion, Value = this.DeploymentFoldersInfo.versions.FabricInstanceVersion, MustOverride = false });
            parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.NodeType, Value = fabricNode.NodeTypeRef, MustOverride = false });
            parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.IsScaleMin, Value = isScaleMin.ToString(), MustOverride = false });

            //
            // Fill in any KTLLogger values from the NodeTypes
            //
            if (nodeType.KtlLoggerSettings != null)
            {
                if (nodeType.KtlLoggerSettings.SharedLogFilePath != null)
                {
                    parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.SharedLogFilePath, Value = nodeType.KtlLoggerSettings.SharedLogFilePath.Value, MustOverride = false });
                }

                if (nodeType.KtlLoggerSettings.SharedLogFileId != null)
                {
                    parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.SharedLogFileId, Value = nodeType.KtlLoggerSettings.SharedLogFileId.Value, MustOverride = false });
                }

                if (nodeType.KtlLoggerSettings.SharedLogFileSizeInMB != null)
                {
                    parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.SharedLogFileSizeInMB, Value = nodeType.KtlLoggerSettings.SharedLogFileSizeInMB.Value.ToString(), MustOverride = false });
                }
            }

            AddEndpoints(parameters, endpoints);
            AddCertifcateInformation(parameters, certificates);
            this.Settings.Add(new SettingsTypeSection() { Name = Constants.SectionNames.FabricNode, Parameter = parameters.ToArray() });
        }

        private void AddCertifcateInformation(List<SettingsTypeSectionParameter> parameters, CertificatesType certificates)
        {
            if (certificates == null)
            {
                return;
            }

            AddSingleCertificateInformation(parameters, certificates.ClientCertificate, Constants.ParameterNames.ClientCertType);
            AddSingleCertificateInformation(parameters, certificates.UserRoleClientCertificate, Constants.ParameterNames.UserRoleClientCertType);
            AddSingleCertificateInformation(parameters, certificates.ServerCertificate, Constants.ParameterNames.ServerCertType);
            AddSingleCertificateInformation(parameters, certificates.ClusterCertificate, Constants.ParameterNames.ClusterCertType);
        }

        private void AddSingleCertificateInformation(List<SettingsTypeSectionParameter> parameters, FabricCertificateType certificate, string certType)
        {
            if (certificate == null)
            {
                return;
            }

            string storeNameTag = string.Format(CultureInfo.InvariantCulture, Constants.ParameterNames.X509StoreNamePattern, certType);
            string findTypeTag = string.Format(CultureInfo.InvariantCulture, Constants.ParameterNames.X509FindTypePattern, certType);
            string findValueTag = string.Format(CultureInfo.InvariantCulture, Constants.ParameterNames.X509FindValuePattern, certType);
            string findValueSecondaryTag = string.Format(CultureInfo.InvariantCulture, Constants.ParameterNames.X509FindValueSecondaryPattern, certType);

            if (certificate.X509StoreName != null)
            {
                parameters.Add(new SettingsTypeSectionParameter() { Name = storeNameTag, Value = certificate.X509StoreName, MustOverride = false });
            }

            parameters.Add(new SettingsTypeSectionParameter() { Name = findTypeTag, Value = certificate.X509FindType.ToString(), MustOverride = false });
            parameters.Add(new SettingsTypeSectionParameter() { Name = findValueTag, Value = certificate.X509FindValue, MustOverride = false });
            parameters.Add(new SettingsTypeSectionParameter() { Name = findValueSecondaryTag, Value = certificate.X509FindValueSecondary, MustOverride = false });
        }

        private void AddEndpoints(List<SettingsTypeSectionParameter> parameters, FabricEndpointsType endpoints)
        {
            AddEndPoint(parameters, FabricValidatorConstants.ParameterNames.NodeAddress, endpoints.ClusterConnectionEndpoint.Port.ToString());
            AddEndPoint(parameters, FabricValidatorConstants.ParameterNames.LeaseAgentAddress, endpoints.LeaseDriverEndpoint.Port.ToString());
            AddEndPoint(parameters, FabricValidatorConstants.ParameterNames.ClientConnectionAddress, endpoints.ClientConnectionEndpoint.Port.ToString());
            AddOptionalEndPoint(parameters, FabricValidatorConstants.ParameterNames.HttpGatewayListenAddress, endpoints.HttpGatewayEndpoint);
            AddOptionalEndpointProtocol(parameters, FabricValidatorConstants.ParameterNames.HttpGatewayProtocol, endpoints.HttpGatewayEndpoint);
            AddOptionalEndPoint(parameters, FabricValidatorConstants.ParameterNames.HttpApplicationGatewayListenAddress, endpoints.HttpApplicationGatewayEndpoint);
            AddOptionalEndpointProtocol(parameters, FabricValidatorConstants.ParameterNames.HttpApplicationGatewayProtocol, endpoints.HttpApplicationGatewayEndpoint);
            AddOptionalEndPoint(parameters, FabricValidatorConstants.ParameterNames.ClusterManagerReplicatorAddress, endpoints.ClusterManagerReplicatorEndpoint);
            AddOptionalEndPoint(parameters, FabricValidatorConstants.ParameterNames.RepairManagerReplicatorAddress, endpoints.RepairManagerReplicatorEndpoint);
            AddOptionalEndPoint(parameters, FabricValidatorConstants.ParameterNames.ImageStoreServiceReplicatorAddress, endpoints.ImageStoreServiceReplicatorEndpoint);
            AddOptionalEndPoint(parameters, FabricValidatorConstants.ParameterNames.UpgradeServiceReplicatorAddress, endpoints.UpgradeServiceReplicatorEndpoint);
            AddOptionalEndPoint(parameters, FabricValidatorConstants.ParameterNames.NamingReplicatorAddress, endpoints.NamingReplicatorEndpoint);
            AddOptionalEndPoint(parameters, FabricValidatorConstants.ParameterNames.FailoverManagerReplicatorAddress, endpoints.FailoverManagerReplicatorEndpoint);
            AddOptionalEndPoint(parameters, FabricValidatorConstants.ParameterNames.RuntimeServiceAddress, endpoints.ServiceConnectionEndpoint);
            AddOptionalEndPoint(parameters, FabricValidatorConstants.ParameterNames.FaultAnalysisServiceReplicatorAddress, endpoints.FaultAnalysisServiceReplicatorEndpoint);
            AddOptionalEndPoint(parameters, FabricValidatorConstants.ParameterNames.BackupRestoreServiceReplicatorAddress, endpoints.BackupRestoreServiceReplicatorEndpoint);
            AddOptionalEndPoint(parameters, FabricValidatorConstants.ParameterNames.UpgradeOrchestrationServiceReplicatorAddress, endpoints.UpgradeOrchestrationServiceReplicatorEndpoint);
            AddOptionalEndPoint(parameters, FabricValidatorConstants.ParameterNames.CentralSecretServiceReplicatorAddress, endpoints.CentralSecretServiceReplicatorEndpoint);
            AddOptionalEndPoint(parameters, FabricValidatorConstants.ParameterNames.EventStoreServiceReplicatorAddress, endpoints.EventStoreServiceReplicatorEndpoint);
            AddOptionalEndPoint(parameters, FabricValidatorConstants.ParameterNames.GatewayResourceManagerReplicatorAddress, endpoints.GatewayResourceManagerReplicatorEndpoint);
            if (endpoints.ApplicationEndpoints != null)
            {
                AddEndPoint(parameters, FabricValidatorConstants.ParameterNames.StartApplicationPortRange, endpoints.ApplicationEndpoints.StartPort.ToString(CultureInfo.InvariantCulture));
                AddEndPoint(parameters, FabricValidatorConstants.ParameterNames.EndApplicationPortRange, endpoints.ApplicationEndpoints.EndPort.ToString(CultureInfo.InvariantCulture));
            }

            if (endpoints.EphemeralEndpoints != null)
            {
                AddEndPoint(parameters, FabricValidatorConstants.ParameterNames.StartDynamicPortRange, endpoints.EphemeralEndpoints.StartPort.ToString(CultureInfo.InvariantCulture));
                AddEndPoint(parameters, FabricValidatorConstants.ParameterNames.EndDynamicPortRange,
                    endpoints.EphemeralEndpoints.EndPort.ToString(CultureInfo.InvariantCulture));
            }
        }

        private void AddOptionalEndPoint(List<SettingsTypeSectionParameter> parameters, string name, InternalEndpointType endpoint)
        {
            AddEndPoint(parameters, name, endpoint != null ? endpoint.Port.ToString() : "0");
        }

        private void AddOptionalEndPoint(List<SettingsTypeSectionParameter> parameters, string name, InputEndpointType endpoint)
        {
            AddEndPoint(parameters, name, endpoint != null ? endpoint.Port.ToString() : "0");
        }

        private void AddOptionalEndpointProtocol(List<SettingsTypeSectionParameter> parameters, string name, InputEndpointType endpoint)
        {
            AddEndPoint(parameters, name, endpoint != null ? endpoint.Protocol.ToString() : "");
        }

        private void AddEndPoint(List<SettingsTypeSectionParameter> parameters, string name, string port)
        {
            parameters.Add(new SettingsTypeSectionParameter() { Name = name, Value = port.ToString(), MustOverride = false });
        }

        private void AddDomainIds(FabricNodeType fabricNode)
        {
            string nodeUpgradeDomain = fabricNode.UpgradeDomain;
            string nodeFaultDomain = fabricNode.FaultDomain;

            List<SettingsTypeSectionParameter> parameters = new List<SettingsTypeSectionParameter>();
            if (!string.IsNullOrEmpty(nodeUpgradeDomain))
            {
                parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.UpgradeDomainId, Value = nodeUpgradeDomain, MustOverride = false });
            }
            if (!string.IsNullOrEmpty(nodeFaultDomain))
            {
                parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.NodeFaultDomainId, Value = nodeFaultDomain, MustOverride = false });
            }
            if (parameters.Count > 0)
            {
                this.Settings.Add(new SettingsTypeSection() { Name = Constants.SectionNames.NodeDomainIds, Parameter = parameters.ToArray() });
            }
        }

        private void AddNodeProperties(ClusterManifestTypeNodeType nodeType)
        {
            List<SettingsTypeSectionParameter> parameters = GetSectionProperties(nodeType.PlacementProperties);
            if (parameters != null)
            {
                this.Settings.Add(new SettingsTypeSection() { Name = Constants.SectionNames.NodeProperties, Parameter = parameters.ToArray() });
            }
            parameters = GetSectionProperties(nodeType.Capacities);
            if (parameters != null)
            {
                this.Settings.Add(new SettingsTypeSection() { Name = Constants.SectionNames.NodeCapacities, Parameter = parameters.ToArray() });
            }
            parameters = GetSectionProperties(nodeType.SfssRgPolicies);
            if (parameters != null)
            {
                this.Settings.Add(new SettingsTypeSection() { Name = Constants.SectionNames.NodeSfssRgPolicies, Parameter = parameters.ToArray() });
            }

            if (nodeType.LogicalDirectories != null)
            {
                List<KeyValuePairType> logicalApplicationDirectories = new List<KeyValuePairType>();
                List<KeyValuePairType> logicalNodeDirectories = new List<KeyValuePairType>();

                foreach (var dir in nodeType.LogicalDirectories)
                {
                    KeyValuePairType pair = new KeyValuePairType
                    {
                        Name = dir.LogicalDirectoryName,
                        Value = dir.MappedTo
                    };

                    if (dir.Context == LogicalDirectoryTypeContext.application)
                    {
                        logicalApplicationDirectories.Add(pair);
                    }
                    else
                    {
                        logicalNodeDirectories.Add(pair);
                    }
                }

                if (logicalApplicationDirectories.Count > 0)
                {
                    parameters = GetSectionProperties(logicalApplicationDirectories.ToArray());
                    if (parameters != null && parameters.Count > 0)
                    {
                        this.Settings.Add(new SettingsTypeSection() { Name = Constants.SectionNames.LogicalApplicationDirectories, Parameter = parameters.ToArray() });
                    }
                }

                if (logicalNodeDirectories.Count > 0)
                {
                    parameters = GetSectionProperties(logicalNodeDirectories.ToArray());
                    if (parameters != null && parameters.Count > 0)
                    {
                        this.Settings.Add(new SettingsTypeSection() { Name = Constants.SectionNames.LogicalNodeDirectories, Parameter = parameters.ToArray() });
                    }
                }
            }

#if false
            parameters = GetSectionProperties(nodeType.CapacityRatios);
            if (parameters != null)
            {
                this.Settings.Add(new SettingsTypeSection() { Name = FabricValidatorConstants.SectionNames.NodeCapacityRatios, Parameter = parameters.ToArray() });
            }
#endif
        }

        private List<SettingsTypeSectionParameter> GetSectionProperties(KeyValuePairType[] properties)
        {
            if (properties == null)
            {
                return null;
            }
            List<SettingsTypeSectionParameter> sectionProperties = new List<SettingsTypeSectionParameter>();
            foreach (KeyValuePairType property in properties)
            {
                sectionProperties.Add(new SettingsTypeSectionParameter() { Name = property.Name, Value = property.Value, MustOverride = false });
            }
            return sectionProperties;
        }
        #endregion
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.FabricDeployer
{
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Globalization;

    internal class PaasInfrastructure : Infrastructure
    {
        #region Public Constructors
        public PaasInfrastructure(ClusterManifestTypeInfrastructure cmInfrastructure, InfrastructureNodeType[] infrastructureNodes, Dictionary<string, ClusterManifestTypeNodeType> nameToNodeTypeMap)
        {
            this.votes = new List<SettingsTypeSectionParameter>();
            this.nodes = new List<InfrastructureNodeType>();
            this.seedNodeClientConnectionAddresses = new List<SettingsTypeSectionParameter>();

            ClusterManifestTypeInfrastructurePaaS infrastructure = cmInfrastructure.Item as ClusterManifestTypeInfrastructurePaaS;
            Dictionary<string, string> roleToTypeMap = new Dictionary<string, string>();
            foreach (var role in infrastructure.Roles)
            {
                try
                {
                    roleToTypeMap.Add(role.RoleName, role.NodeTypeRef);
                }
                catch (ArgumentException)
                {
                    throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Role name {0} is duplicate", role.RoleName));
                }
            }

            foreach (var nodeInfo in infrastructureNodes)
            {
                ClusterManifestTypeNodeType nodeType = null;
                if (!nameToNodeTypeMap.TryGetValue(nodeInfo.NodeTypeRef, out nodeType) || nodeType == null)
                {
                    continue;
                }      

                var node = (new InfrastructureNodeType()
                {
                    Certificates = nodeType.Certificates,
                    Endpoints = nodeType.Endpoints,
                    FaultDomain = nodeInfo.FaultDomain,
                    IPAddressOrFQDN = nodeInfo.IPAddressOrFQDN,
                    IsSeedNode = nodeInfo.IsSeedNode,
                    NodeName = nodeInfo.NodeName,
                    NodeTypeRef = nodeInfo.NodeTypeRef,
                    RoleOrTierName = nodeInfo.NodeTypeRef,
                    UpgradeDomain = nodeInfo.UpgradeDomain
                });
                
                this.nodes.Add(node);
                
                if (node.IsSeedNode)
                {
                    string key = node.NodeName;
                    string clientConnectionAddress = string.Format(CultureInfo.InvariantCulture, "{0}:{1}", node.IPAddressOrFQDN, node.Endpoints.ClientConnectionEndpoint.Port);
                    this.seedNodeClientConnectionAddresses.Add(new SettingsTypeSectionParameter() { Name = key, Value = clientConnectionAddress, MustOverride = false });
                    FabricValidator.TraceSource.WriteNoise(FabricValidatorUtility.TraceTag, "Seed Nodes {0}-{1} added", key, clientConnectionAddress);
                }              
            }

            foreach (var vote in infrastructure.Votes)
            {
                string key = vote.NodeName;
                string connectionAddress = string.Format(CultureInfo.InvariantCulture, "{0}:{1}", vote.IPAddressOrFQDN, vote.Port);
                string value = string.Format(CultureInfo.InvariantCulture, "{0},{1}", "SeedNode", connectionAddress);
                this.votes.Add(new SettingsTypeSectionParameter() { Name = key, Value = value, MustOverride = false });
                FabricValidator.TraceSource.WriteNoise(FabricValidatorUtility.TraceTag, "Vote {0}-{1} added", key, value);
            }            
        }
        #endregion

        public override bool IsDefaultSystem
        {
            get
            {
                return false;
            }
        }

        public override bool IsScaleMin
        {
            get
            {
                return false;
            }
        }

        public override List<SettingsTypeSectionParameter> InfrastructureVotes
        {
            get
            {
                return this.votes;
            }
        }

        public override Dictionary<string, string> VoteMappings
        {
            get
            {
                return null;
            }
        }

        public override List<SettingsTypeSectionParameter> SeedNodeClientConnectionAddresses
        {
            get
            {
                return this.seedNodeClientConnectionAddresses;
            }
        }

        private List<SettingsTypeSectionParameter> votes;
        private readonly List<SettingsTypeSectionParameter> seedNodeClientConnectionAddresses;
    }
}
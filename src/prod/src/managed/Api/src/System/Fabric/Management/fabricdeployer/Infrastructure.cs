// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.FabricDeployer
{
    using Microsoft.Win32;
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Linq;

    internal abstract class Infrastructure
    {
        #region Public Properties
        public abstract bool IsDefaultSystem { get; }

        public abstract bool IsScaleMin { get; }

        public abstract List<SettingsTypeSectionParameter> InfrastructureVotes { get; }

        public abstract Dictionary<string, string> VoteMappings { get; }

        public abstract List<SettingsTypeSectionParameter> SeedNodeClientConnectionAddresses { get; }

        public List<InfrastructureNodeType> InfrastructureNodes { get { return this.nodes; } }
        #endregion Public Properties

        #region Public Functions
        public static Infrastructure Create(ClusterManifestTypeInfrastructure cmInfrastructure, InfrastructureNodeType[] infrastructureNodes, ClusterManifestTypeNodeType[] nodeTypes)
        {
            Dictionary<string, ClusterManifestTypeNodeType> nameToNodeTypeMap = new Dictionary<string, ClusterManifestTypeNodeType>();
            foreach (var nodeType in nodeTypes)
            {
                try
                {
                    nameToNodeTypeMap.Add(nodeType.Name, nodeType);
                }
                catch (ArgumentException)
                {
                    throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Node type name {0} is duplicate", nodeType.Name));
                }
            }
#if DotNetCoreClrLinux
            if (cmInfrastructure.Item is ClusterManifestTypeInfrastructureLinux)
#else
            if (cmInfrastructure.Item is ClusterManifestTypeInfrastructureWindowsServer)
#endif
            {
                return new ServerInfrastructure(cmInfrastructure, infrastructureNodes, nameToNodeTypeMap);
            }
            else if (cmInfrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzure)
            {
                return new AzureInfrastructure(cmInfrastructure, infrastructureNodes, nameToNodeTypeMap);
            }
            else if (cmInfrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzureStaticTopology)
            {
                return new AzureInfrastructure(cmInfrastructure, infrastructureNodes, nameToNodeTypeMap, GetDynamicTopologyKindIntValue());
            }
            else if (cmInfrastructure.Item is ClusterManifestTypeInfrastructurePaaS)
            {
                return new PaasInfrastructure(cmInfrastructure, infrastructureNodes, nameToNodeTypeMap);
            }
            else
            {
                return null;
            }
        }

        public InfrastructureInformationType GetInfrastructureManifest()
        {
            return new InfrastructureInformationType() { NodeList = this.nodes.ToArray() };
        }
        #endregion Public Functions

        protected List<InfrastructureNodeType> nodes;

        private static int GetDynamicTopologyKindIntValue()
        {
            using (RegistryKey registryKey = Registry.LocalMachine.OpenSubKey(FabricConstants.FabricRegistryKeyPath))
            {
                if (registryKey == null)
                {
                    return 0;
                }

                int value = (int)registryKey.GetValue(Fabric.FabricDeployer.Constants.Registry.DynamicTopologyKindValue, 0);

                return value;
            }
        }
    }
}
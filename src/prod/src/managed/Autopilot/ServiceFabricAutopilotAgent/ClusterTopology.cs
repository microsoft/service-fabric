// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Autopilot
{
    using Microsoft.Search.Autopilot;
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.IO;
    using System.Linq;

    [Flags]
    internal enum DynamicTopologyUpdateMode
    {
        None = 0x00,

        OnNodeConfigurationOnly = 0x01,
    }

    internal class ClusterTopology
    {
        private ClusterTopology()
        {
        }

        internal List<InfrastructureNodeType> ServiceFabricNodes { get; private set; }

        // Current node name is only needed when there is a need to perform node configuration update (to get current cluster manifest path in Fabric data root)
        internal string CurrentNodeName { get; private set; }

        internal static ClusterTopology GetClusterTopology()
        {
            // Note that this is temporary.
            // Eventually we would have two config parameters controlling
            // 1) The scope of topology info: environment or cross-environment.
            // 2) When to update topology info (i.e. what DynamicTopologyUpdateMode was actually for): only during node configuration or whenever there is a change in topology.
            if (ConfigurationManager.DynamicTopologyUpdateMode == DynamicTopologyUpdateMode.None)
            {
                return GetClusterTopologyFromStaticTopologyProvider();
            }

            return GetEnvironmentTopology();
        }

        internal void LogTopology()
        {
            if (this.ServiceFabricNodes.Count == 0)
            {
                TextLogger.LogInfo("Static topology defined in cluster manifest + dynamic topology update disabled.");
            }
            else
            {
                TextLogger.LogInfo("Static topology defined in cluster manifest + dynamic topology update with {0} nodes", this.ServiceFabricNodes.Count);
                TextLogger.LogInfo("NodeName, NodeType, UpgradeDomain, FaultDomain, IPAddressOrFQDN");

                foreach (InfrastructureNodeType node in this.ServiceFabricNodes)
                {
                    TextLogger.LogInfo(
                        "{0}, {1}, {2}, {3}, {4}",
                        node.NodeName,
                        node.NodeTypeRef,
                        node.UpgradeDomain,
                        node.FaultDomain,
                        node.IPAddressOrFQDN
                    );
                }
            }
        }

        // Whether topology of the cluster has changed.
        // Topology is defined as the set of nodes. Each node is defined by name, UD, FD, IP, role, etc.
        internal bool IsClusterTopologyChanged(ClusterTopology clusterTopology)
        {
            if (clusterTopology == null || ServiceFabricNodes.Count != clusterTopology.ServiceFabricNodes.Count)
            {
                return true;
            }

            // note that node list is always ordered by node name(role.nodeID), which uniquely identifies a node
            for (int i = 0; i < clusterTopology.ServiceFabricNodes.Count; i++)
            {
                if (!NodeEquals(clusterTopology.ServiceFabricNodes[i], ServiceFabricNodes[i]))
                {
                    return true;
                }
            }

            return false;
        }

        private static ClusterTopology GetClusterTopologyFromStaticTopologyProvider()
        {
            // An empty node list indicates that cluster topology is statically defined in cluster manifest.
            return new ClusterTopology
            {
                ServiceFabricNodes = new List<InfrastructureNodeType>(0),

                CurrentNodeName = string.Empty
            };
        }

        private static ClusterTopology GetEnvironmentTopology()
        {
            string machineInfoFile = Path.Combine(APRuntime.DataDirectory, StringConstants.MachineInfoFileName);

            List<InfrastructureNodeType> serviceFabricNodes = new List<InfrastructureNodeType>();
            using (CsvReader reader = new CsvReader(machineInfoFile))
            {
                while (reader.Read())
                {
                    var serviceFabricNode = new InfrastructureNodeType()
                    {
                        NodeName = reader["MachineName"],
                        // Note that NodeTypeRef and RoleOrTierName are provided due to XML schema of infrastructure manifest.
                        NodeTypeRef = reader["MachineFunction"],
                        RoleOrTierName = reader["MachineFunction"],
                        IPAddressOrFQDN = reader["MachineName"],
                        UpgradeDomain = reader["ScaleUnit"],
                        FaultDomain = string.Format(CultureInfo.InvariantCulture, "fd:/{0}", reader["PodName"])
                    };

                    serviceFabricNodes.Add(serviceFabricNode);
                }
            };

            return new ClusterTopology
            {
                ServiceFabricNodes = serviceFabricNodes,

                CurrentNodeName = string.Empty
            };
        }

        private static int CompareNodeByName(InfrastructureNodeType nodeA, InfrastructureNodeType nodeB)
        {
            return string.Compare(nodeA.NodeName, nodeB.NodeName, StringComparison.OrdinalIgnoreCase);
        }

        private static bool NodeEquals(InfrastructureNodeType nodeA, InfrastructureNodeType nodeB)
        {
            // TODO: On Autopilot, endpoint set is not part of service model so would not be modeled as part of topology.
            if (string.Compare(nodeA.NodeName, nodeB.NodeName, StringComparison.Ordinal) != 0 ||
                string.Compare(nodeA.UpgradeDomain, nodeB.UpgradeDomain, StringComparison.OrdinalIgnoreCase) != 0 ||
                string.Compare(nodeA.FaultDomain, nodeB.FaultDomain, StringComparison.OrdinalIgnoreCase) != 0 ||
                string.Compare(nodeA.IPAddressOrFQDN, nodeB.IPAddressOrFQDN, StringComparison.OrdinalIgnoreCase) != 0 ||
                string.Compare(nodeA.RoleOrTierName, nodeB.RoleOrTierName, StringComparison.OrdinalIgnoreCase) != 0)
            {
                return false;
            }

            return true;
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Fabric.InfrastructureWrapper
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.Linq;
    using System.Net;
    
    using Microsoft.WindowsAzure.ServiceRuntime;

    internal class ClusterTopology
    {
        private ClusterTopology()
        {
        }

        internal List<InfrastructureNodeType> WindowsFabricNodes { get; private set; }

        internal string CurrentNodeName { get; private set; }

        internal string CurrentNodeIPAddressOrFQDN { get; private set; }

        internal static ClusterTopology GetClusterTopologyFromServiceRuntime(LocalNodeConfiguration localWindowsFabricNode)
        {
            List<InfrastructureNodeType> windowsFabricNodes = new List<InfrastructureNodeType>();

            int applicationPortCount = localWindowsFabricNode.ApplicationEndPort - localWindowsFabricNode.ApplicationStartPort + 1;
            Dictionary<string, WindowsFabricEndpointSet> roleNameToEndpointSetMap = GetRoleNameToEndpointSetMap(applicationPortCount);
            WindowsFabricEndpointSet currentRoleEndpointSet = roleNameToEndpointSetMap[RoleEnvironment.CurrentRoleInstance.Role.Name];

            foreach (KeyValuePair<string, Role> role in RoleEnvironment.Roles)
            {
                foreach (RoleInstance roleInstance in role.Value.Instances.Where(instance => IsWindowsFabricRoleInstance(instance)))
                {
                    var roleEndpointSet = roleNameToEndpointSetMap[roleInstance.Role.Name];
                    InfrastructureNodeType node = new InfrastructureNodeType()
                    {
                        NodeName = GetNodeName(roleInstance),

                        UpgradeDomain = roleInstance.UpdateDomain.ToString(CultureInfo.InvariantCulture),

                        FaultDomain = string.Format(CultureInfo.InvariantCulture, WindowsFabricAzureWrapperServiceCommon.FaultDomainTemplate, roleInstance.FaultDomain),

                        IPAddressOrFQDN = GetRoleInstanceHostIPAddress(roleInstance).ToString(),

                        RoleOrTierName = roleInstance.Role.Name,

                        // The following two entries are unknown at this time and will be populated by FabricDeployer based on cluster manifest
                        NodeTypeRef = string.Empty,

                        IsSeedNode = false,

                        Endpoints = new FabricEndpointsType()
                        {
                            ClientConnectionEndpoint = roleEndpointSet.ClientConnectionEndpoint ?? currentRoleEndpointSet.ClientConnectionEndpoint,
                            HttpGatewayEndpoint = roleEndpointSet.HttpGatewayEndpoint ?? currentRoleEndpointSet.HttpGatewayEndpoint,
                            HttpApplicationGatewayEndpoint = roleEndpointSet.HttpApplicationGatewayEndpoint ?? currentRoleEndpointSet.HttpApplicationGatewayEndpoint,
                            ClusterConnectionEndpoint = roleEndpointSet.ClusterConnectionEndpoint,
                            LeaseDriverEndpoint = roleEndpointSet.LeaseDriverEndpoint,
                            ApplicationEndpoints = roleEndpointSet.ApplicationEndpoints,
                        }
                    };

                    windowsFabricNodes.Add(node);
                }
            }

            // node list is always ordered by node name
            windowsFabricNodes.Sort(CompareNodeByName);

            return new ClusterTopology 
            { 
                WindowsFabricNodes = windowsFabricNodes,

                CurrentNodeName = GetNodeName(RoleEnvironment.CurrentRoleInstance),

                CurrentNodeIPAddressOrFQDN = GetRoleInstanceHostIPAddress(RoleEnvironment.CurrentRoleInstance).ToString(),
            };            
        }

        internal static ClusterTopology GetClusterTopologyFromStaticTopologyProvider()
        {
            // An empty node list indicates that cluster topology is statically defined in cluster manifest.
            return new ClusterTopology
            {
                WindowsFabricNodes = new List<InfrastructureNodeType>(0),

                CurrentNodeName = string.Empty,

                CurrentNodeIPAddressOrFQDN = GetRoleInstanceHostIPAddress(RoleEnvironment.CurrentRoleInstance).ToString(),
            };            
        }
        
        // Whether topology of the cluster has changed.
        // Topology is defined as the set of nodes. Each node is defined by name, UD, FD, IP, role, etc. 
        internal bool IsClusterTopologyChanged(ClusterTopology clusterTopology)
        {
            if (clusterTopology == null || WindowsFabricNodes.Count != clusterTopology.WindowsFabricNodes.Count)
            {
                return true;
            }

            // note that node list is always ordered by node name(role.nodeID), which uniquely identifies a node
            for (int i = 0; i < clusterTopology.WindowsFabricNodes.Count; i++)
            {
                if (!NodeEquals(clusterTopology.WindowsFabricNodes[i], WindowsFabricNodes[i]))
                {
                    return true;
                }
            }

            return false;
        }

        private static Dictionary<string, WindowsFabricEndpointSet> GetRoleNameToEndpointSetMap(int applicationPortCount)
        {
            Dictionary<string, WindowsFabricEndpointSet> roleNameToEndpointSetMap = new Dictionary<string, WindowsFabricEndpointSet>(StringComparer.OrdinalIgnoreCase);

            foreach (KeyValuePair<string, Role> role in RoleEnvironment.Roles)
            {
                // If it is the role of current role instance, get endpoints from current role instance; otherwise, use the first role instance in the collection
                if (RoleEnvironment.CurrentRoleInstance.Role.Name.Equals(role.Value.Name, StringComparison.OrdinalIgnoreCase))
                {
                    roleNameToEndpointSetMap[role.Value.Name] = WindowsFabricEndpointSet.GetWindowsFabricEndpointSet(RoleEnvironment.CurrentRoleInstance.InstanceEndpoints, applicationPortCount);
                }
                else
                {
                    foreach (RoleInstance roleInstance in role.Value.Instances.Where(instance => IsWindowsFabricRoleInstance(instance)))
                    {
                        roleNameToEndpointSetMap[role.Value.Name] = WindowsFabricEndpointSet.GetWindowsFabricEndpointSet(roleInstance.InstanceEndpoints, applicationPortCount);
                        break;
                    }
                }
            }

            return roleNameToEndpointSetMap;
        }

        private static string GetNodeName(RoleInstance roleInstance)
        {
            int roleInstanceIndex = GetRoleInstanceIndex(roleInstance);

            return string.Format(CultureInfo.InvariantCulture, WindowsFabricAzureWrapperServiceCommon.NodeNameTemplate, roleInstance.Role.Name, roleInstanceIndex);
        }

        private static int GetRoleInstanceIndex(RoleInstance roleInstance)
        {
            // Get the index from role instance id e.g. workerRole1_IN_0 -> 0
            string[] tokens = roleInstance.Id.Split('_');
            return int.Parse(tokens[tokens.Length - 1], CultureInfo.InvariantCulture);
        }

        private static IPAddress GetRoleInstanceHostIPAddress(RoleInstance roleInstance)
        {
            if (roleInstance.InstanceEndpoints != null && roleInstance.InstanceEndpoints.Count() > 0)
            {
                return roleInstance.InstanceEndpoints.First().Value.IPEndpoint.Address;
            }

            return IPAddress.Any;
        }

        private static bool IsWindowsFabricRoleInstance(RoleInstance roleInstance)
        {
            // Whether a role instance is a WindowsFabric role instance is determined by whether it defines ClientConnectionEndpoint (i.e. 'NodeAddress' endpoint)
            return roleInstance.InstanceEndpoints.ContainsKey(WindowsFabricAzureWrapperServiceCommon.NodeAddressEndpointName);
        }

        private static int CompareNodeByName(InfrastructureNodeType nodeA, InfrastructureNodeType nodeB)
        {
            return string.Compare(nodeA.NodeName, nodeB.NodeName, StringComparison.OrdinalIgnoreCase);
        }

        private static bool NodeEquals(InfrastructureNodeType nodeA, InfrastructureNodeType nodeB)
        {
            if (string.Compare(nodeA.NodeName, nodeB.NodeName, StringComparison.OrdinalIgnoreCase) != 0 ||
                string.Compare(nodeA.UpgradeDomain, nodeB.UpgradeDomain, StringComparison.OrdinalIgnoreCase) != 0 ||
                string.Compare(nodeA.FaultDomain, nodeB.FaultDomain, StringComparison.OrdinalIgnoreCase) != 0 ||
                string.Compare(nodeA.IPAddressOrFQDN, nodeB.IPAddressOrFQDN, StringComparison.OrdinalIgnoreCase) != 0 ||
                string.Compare(nodeA.RoleOrTierName, nodeB.RoleOrTierName, StringComparison.OrdinalIgnoreCase) != 0)
            {
                return false;
            }

            return FabricEndpointsEquals(nodeA.Endpoints, nodeB.Endpoints);
        }

        private static bool FabricEndpointsEquals(FabricEndpointsType endpointSetA, FabricEndpointsType endpointSetB)
        {
            if (endpointSetA == null && endpointSetB == null)
            {
                return true;
            }

            if (endpointSetA != null && endpointSetA != null)
            {
                return
                    WindowsFabricEndpointSet.InputEndpointEquals(endpointSetA.ClientConnectionEndpoint, endpointSetB.ClientConnectionEndpoint) &&
                    WindowsFabricEndpointSet.InputEndpointEquals(endpointSetA.HttpGatewayEndpoint, endpointSetB.HttpGatewayEndpoint) &&
                    WindowsFabricEndpointSet.InputEndpointEquals(endpointSetA.HttpApplicationGatewayEndpoint, endpointSetB.HttpApplicationGatewayEndpoint) &&
                    WindowsFabricEndpointSet.InternalEndpointEquals(endpointSetA.ClusterConnectionEndpoint, endpointSetB.ClusterConnectionEndpoint) &&
                    WindowsFabricEndpointSet.InternalEndpointEquals(endpointSetA.LeaseDriverEndpoint, endpointSetB.LeaseDriverEndpoint) &&
                    WindowsFabricEndpointSet.EndpointRangeEquals(endpointSetA.ApplicationEndpoints, endpointSetB.ApplicationEndpoints);
            }

            return false;
        }
    }
}
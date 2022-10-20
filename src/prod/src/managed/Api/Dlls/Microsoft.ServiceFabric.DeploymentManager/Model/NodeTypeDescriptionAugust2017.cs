// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Model
{
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager.BPA;

    public class NodeTypeDescriptionAugust2017 : NodeTypeDescriptionGA
    {
        public LogicalDirectory[] LogicalDirectories
        {
            get;
            set;
        }

        internal static new NodeTypeDescriptionAugust2017 ReadFromInternal(NodeTypeDescription nodeTypeDesc)
        {
            return new NodeTypeDescriptionAugust2017
            {
                Name = nodeTypeDesc.Name,
                PlacementProperties = nodeTypeDesc.PlacementProperties,
                Capacities = nodeTypeDesc.Capacities,
                ClientConnectionEndpointPort = nodeTypeDesc.ClientConnectionEndpointPort,
                HttpGatewayEndpointPort = nodeTypeDesc.HttpGatewayEndpointPort,
                ApplicationPorts = nodeTypeDesc.ApplicationPorts,
                EphemeralPorts = nodeTypeDesc.EphemeralPorts,
                ReverseProxyEndpointPort = nodeTypeDesc.HttpApplicationGatewayEndpointPort,
                LeaseDriverEndpointPort = nodeTypeDesc.LeaseDriverEndpointPort,
                ClusterConnectionEndpointPort = nodeTypeDesc.ClusterConnectionEndpointPort,
                ServiceConnectionEndpointPort = nodeTypeDesc.ServiceConnectionEndpointPort,
                LogicalDirectories = nodeTypeDesc.LogicalDirectories,
                KtlLogger = nodeTypeDesc.KtlLogger,
                IsPrimary = nodeTypeDesc.IsPrimary,
            };
        }

        internal override NodeTypeDescription ConvertToInternal()
        {
            NodeTypeDescription nodeTypeDesc = new NodeTypeDescription()
            {
                Name = this.Name,
                PlacementProperties = this.PlacementProperties,
                Capacities = this.Capacities,
                ClientConnectionEndpointPort = this.ClientConnectionEndpointPort,
                HttpGatewayEndpointPort = this.HttpGatewayEndpointPort,
                ApplicationPorts = this.ApplicationPorts,
                EphemeralPorts = this.EphemeralPorts,
                HttpApplicationGatewayEndpointPort = this.ReverseProxyEndpointPort,
                LeaseDriverEndpointPort = this.LeaseDriverEndpointPort,
                ClusterConnectionEndpointPort = this.ClusterConnectionEndpointPort,
                ServiceConnectionEndpointPort = this.ServiceConnectionEndpointPort,
                LogicalDirectories = this.LogicalDirectories,
                KtlLogger = this.KtlLogger,
                IsPrimary = this.IsPrimary
            };

            return nodeTypeDesc;
        }
    }
}
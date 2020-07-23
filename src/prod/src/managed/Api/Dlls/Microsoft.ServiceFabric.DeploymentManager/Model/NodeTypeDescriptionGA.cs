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

    public class NodeTypeDescriptionGA
    {
        public string Name
        {
            get;
            set;
        }

        public Dictionary<string, string> PlacementProperties
        {
            get;
            set;
        }

        public Dictionary<string, string> Capacities
        {
            get;
            set;
        }

        public int ClientConnectionEndpointPort
        {
            get;
            set;
        }

        public int HttpGatewayEndpointPort
        {
            get;
            set;
        }

        public int? ReverseProxyEndpointPort
        {
            get;
            set;
        }

        public int LeaseDriverEndpointPort
        {
            get;
            set;
        }

        public int ClusterConnectionEndpointPort
        {
            get;
            set;
        }

        public int ServiceConnectionEndpointPort
        {
            get;
            set;
        }

        public EndpointRangeDescription ApplicationPorts
        {
            get;
            set;
        }

        public EndpointRangeDescription EphemeralPorts
        {
            get;
            set;
        }

        public KtlLogger KtlLogger
        {
            get;
            set;
        }

        public bool IsPrimary
        {
            get;
            set;
        }

        internal static NodeTypeDescriptionGA ReadFromInternal(NodeTypeDescription nodeTypeDesc)
        {
            return new NodeTypeDescriptionGA
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
                KtlLogger = nodeTypeDesc.KtlLogger,
                IsPrimary = nodeTypeDesc.IsPrimary,
            };
        }

        internal virtual NodeTypeDescription ConvertToInternal()
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
                KtlLogger = this.KtlLogger,
                IsPrimary = this.IsPrimary
            };

            return nodeTypeDesc;
        }

        internal void Verify()
        {
            this.Name.ThrowValidationExceptionIfNullOrEmpty("NodeType/NodeName");
        }
    }
}
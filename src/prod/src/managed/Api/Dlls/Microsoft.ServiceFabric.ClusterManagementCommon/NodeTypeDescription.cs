// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;

    public class NodeTypeDescription
    {
        public NodeTypeDescription()
        {
            this.DurabilityLevel = DurabilityLevel.Bronze;
            this.LeaseDriverEndpointPort = 0;
            this.ClusterConnectionEndpointPort = 0;
            this.ServiceConnectionEndpointPort = 0;
        }

        [Required(AllowEmptyStrings = false)]
        public string Name { get; set; }

        public Dictionary<string, string> PlacementProperties { get; set; }

        public Dictionary<string, string> Capacities { get; set; }

        public Dictionary<string, string> SfssRgPolicies { get; set; }

        public int ClientConnectionEndpointPort { get; set; }

        public int HttpGatewayEndpointPort { get; set; }

        public int? HttpApplicationGatewayEndpointPort { get; set; }

        public int LeaseDriverEndpointPort { get; set; }

        public int ClusterConnectionEndpointPort { get; set; }

        public int ServiceConnectionEndpointPort { get; set; }

        public EndpointRangeDescription ApplicationPorts { get; set; }

        public EndpointRangeDescription EphemeralPorts { get; set; }

        public bool IsPrimary { get; set; }
        
        public int VMInstanceCount { get; set; }

        public string VMResourceName { get; set; }

        public KtlLogger KtlLogger { get; set; }

        public LogicalDirectory[] LogicalDirectories { get; set; }

        public DurabilityLevel DurabilityLevel { get; set; }
        
        public ClusterManifestTypeNodeType ToClusterManifestTypeNodeType()
        {
            var endpoints = new FabricEndpointsType()
            {
                ClientConnectionEndpoint =
                    new InputEndpointType()
                    {
                        Port = this.ClientConnectionEndpointPort.ToString(),
                        Protocol = InputEndpointTypeProtocol.tcp
                    },
                HttpGatewayEndpoint =
                    new InputEndpointType()
                    {
                        Port = this.HttpGatewayEndpointPort.ToString(),
                        Protocol = InputEndpointTypeProtocol.http
                    },                
            };

            if (this.HttpApplicationGatewayEndpointPort != null)
            {
                endpoints.HttpApplicationGatewayEndpoint =
                        new InputEndpointType()
                        {
                            Port = this.HttpApplicationGatewayEndpointPort.ToString(),
                            Protocol = InputEndpointTypeProtocol.http
                        };
            }

            LogicalDirectoryType[] logicalDirectories = null;
            if (this.LogicalDirectories != null && this.LogicalDirectories.Length > 0)
            {
                logicalDirectories = new LogicalDirectoryType[this.LogicalDirectories.Count()];
                var i = 0;
                foreach (var dir in this.LogicalDirectories)
                {
                    logicalDirectories[i++] = dir.ToLogicalDirectoryType();
                }
            }

            return new ClusterManifestTypeNodeType()
            {
                Name = this.Name,
                PlacementProperties =
                    (this.PlacementProperties == null)
                        ? null
                        : this.PlacementProperties.Select(
                            property => new KeyValuePairType()
                            {
                                Name = property.Key,
                                Value = property.Value
                            }).ToArray(),
                Capacities =
                    (this.Capacities == null)
                        ? null
                        : this.Capacities.Select(
                            capacity => new KeyValuePairType()
                            {
                                Name = capacity.Key,
                                Value = capacity.Value
                            }).ToArray(),
                SfssRgPolicies = (this.SfssRgPolicies == null)
                        ? null
                        : this.SfssRgPolicies.Select(
                            rgPolicy => new KeyValuePairType()
                            {
                                Name = rgPolicy.Key,
                                Value = rgPolicy.Value
                            }).ToArray(),
                Endpoints = endpoints,
                KtlLoggerSettings = (this.KtlLogger == null) ? null : this.KtlLogger.ToFabricKtlLoggerSettingsType(),
                LogicalDirectories = logicalDirectories
            };
        }

        public PaaSRoleType ToPaaSRoleType()
        {
            return new PaaSRoleType()
            {
                RoleName = this.Name,
                NodeTypeRef = this.Name,
                RoleNodeCount = this.VMInstanceCount
            };
        }
    }
}
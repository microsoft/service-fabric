// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Fabric.InfrastructureWrapper
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Management.ServiceModel;   
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Net;   
          
    using Microsoft.WindowsAzure;
    using Microsoft.WindowsAzure.ServiceRuntime;

    // The set of Windows Fabric endpoints for a Windows Fabric node on Windows Azure.
    // Note that WindowsFabricEndpointSet objects are used as immutable objects.
    internal class WindowsFabricEndpointSet
    {
        private WindowsFabricEndpointSet()
        {            
        }

        internal InputEndpointType ClientConnectionEndpoint { get; private set; }

        internal InputEndpointType HttpGatewayEndpoint { get; private set; }

        internal InputEndpointType HttpApplicationGatewayEndpoint { get; private set; }

        internal InternalEndpointType ClusterConnectionEndpoint { get; private set; }

        internal InternalEndpointType LeaseDriverEndpoint { get; private set; }

        internal FabricEndpointsTypeApplicationEndpoints ApplicationEndpoints { get; private set; }        

        public override string ToString()
        {
            return
                string.Format(
                    CultureInfo.InvariantCulture,
                    "ClientConnectionEndpoint <-> {0}, HttpGatewayEndpoint <-> {1}, HttpApplicationGatewayEndpoint <-> {2}, ClusterConnectionEndpoint <-> {3}, LeaseDriverEndpoint <-> {4}, ApplicationEndpoints <-> {5}",
                    InputEndpointToString(this.ClientConnectionEndpoint),
                    InputEndpointToString(this.HttpGatewayEndpoint),
                    InputEndpointToString(this.HttpApplicationGatewayEndpoint),
                    InternalEndpointToString(this.ClusterConnectionEndpoint),                    
                    InternalEndpointToString(this.LeaseDriverEndpoint),
                    EndpointRangeToString(this.ApplicationEndpoints));
        }

        internal static WindowsFabricEndpointSet GetWindowsFabricEndpointSet(IDictionary<string, RoleInstanceEndpoint> roleInstanceEndpointMap, int applicationPortCount)
        {
            Dictionary<string, RoleInstanceEndpoint> roleInstanceEndpoints = new Dictionary<string, RoleInstanceEndpoint>(roleInstanceEndpointMap, StringComparer.OrdinalIgnoreCase);

            return new WindowsFabricEndpointSet
            {
                ClientConnectionEndpoint = GetWindowsFabricInputEndpoint(roleInstanceEndpoints, WindowsFabricAzureWrapperServiceCommon.ClientConnectionEndpointName),

                HttpGatewayEndpoint = GetWindowsFabricInputEndpoint(roleInstanceEndpoints, WindowsFabricAzureWrapperServiceCommon.HttpGatewayEndpointName),

                HttpApplicationGatewayEndpoint = GetWindowsFabricInputEndpoint(roleInstanceEndpoints, WindowsFabricAzureWrapperServiceCommon.HttpApplicationGatewayEndpointName),

                ClusterConnectionEndpoint = GetWindowsFabricInternalEndpoint(roleInstanceEndpoints, WindowsFabricAzureWrapperServiceCommon.NodeAddressEndpointName),

                LeaseDriverEndpoint = GetWindowsFabricInternalEndpoint(roleInstanceEndpoints, WindowsFabricAzureWrapperServiceCommon.LeaseDriverEndpointName),

                ApplicationEndpoints = GetWindowsFabricEndpointRange(roleInstanceEndpoints, WindowsFabricAzureWrapperServiceCommon.ApplicationPortsEndpointName, applicationPortCount),                
            };
        }

        internal static bool InputEndpointEquals(InputEndpointType endpointA, InputEndpointType endpointB)
        {
            if (endpointA == null && endpointB == null)
            {
                return true;
            }

            if (endpointA == null || endpointB == null)
            {
                return false;
            }

            return endpointA.Protocol == endpointB.Protocol && string.Compare(endpointA.Port, endpointB.Port, StringComparison.OrdinalIgnoreCase) == 0;
        }

        internal static bool InternalEndpointEquals(InternalEndpointType endpointA, InternalEndpointType endpointB)
        {
            if (endpointA == null && endpointB == null)
            {
                return true;
            }

            if (endpointA == null || endpointB == null)
            {
                return false;
            }

            return endpointA.Protocol == endpointB.Protocol && string.Compare(endpointA.Port, endpointB.Port, StringComparison.OrdinalIgnoreCase) == 0;
        }

        internal static bool EndpointRangeEquals(FabricEndpointsTypeApplicationEndpoints endpointRangeA, FabricEndpointsTypeApplicationEndpoints endpointRangeB)
        {
            if (endpointRangeA == null && endpointRangeB == null)
            {
                return true;
            }

            if (endpointRangeA == null || endpointRangeB == null)
            {
                return false;
            }

            return endpointRangeA.StartPort == endpointRangeB.StartPort && endpointRangeA.EndPort == endpointRangeB.EndPort;
        }

        internal bool Equals(WindowsFabricEndpointSet windowsFabricEndpointSet)
        {
            if (windowsFabricEndpointSet == null)
            {
                return false;
            }

            return
                InputEndpointEquals(this.ClientConnectionEndpoint, windowsFabricEndpointSet.ClientConnectionEndpoint) &&
                InputEndpointEquals(this.HttpGatewayEndpoint, windowsFabricEndpointSet.HttpGatewayEndpoint) &&
                InputEndpointEquals(this.HttpApplicationGatewayEndpoint, windowsFabricEndpointSet.HttpApplicationGatewayEndpoint) &&
                InternalEndpointEquals(this.ClusterConnectionEndpoint, windowsFabricEndpointSet.ClusterConnectionEndpoint) &&
                InternalEndpointEquals(this.LeaseDriverEndpoint, windowsFabricEndpointSet.LeaseDriverEndpoint) &&
                EndpointRangeEquals(this.ApplicationEndpoints, windowsFabricEndpointSet.ApplicationEndpoints);                
        }
        
        private static InputEndpointType GetWindowsFabricInputEndpoint(IDictionary<string, RoleInstanceEndpoint> roleInstanceEndpoints, string endpointName)
        {
            if (!roleInstanceEndpoints.ContainsKey(endpointName))
            {
                return null;
            }

            RoleInstanceEndpoint roleInstanceEndpoint = roleInstanceEndpoints[endpointName];

            return new InputEndpointType
            {
                Protocol = (InputEndpointTypeProtocol)Enum.Parse(typeof(InputEndpointTypeProtocol), roleInstanceEndpoint.Protocol, true),
                Port = roleInstanceEndpoint.IPEndpoint.Port.ToString(CultureInfo.InvariantCulture),
            };
        }
        
        private static InternalEndpointType GetWindowsFabricInternalEndpoint(IDictionary<string, RoleInstanceEndpoint> roleInstanceEndpoints, string endpointName)
        {
            if (!roleInstanceEndpoints.ContainsKey(endpointName))
            {
                return null;
            }

            RoleInstanceEndpoint roleInstanceEndpoint = roleInstanceEndpoints[endpointName];

            return new InternalEndpointType
            {
                Protocol = (InternalEndpointTypeProtocol)Enum.Parse(typeof(InternalEndpointTypeProtocol), roleInstanceEndpoint.Protocol, true),
                Port = roleInstanceEndpoint.IPEndpoint.Port.ToString(CultureInfo.InvariantCulture),
            };
        }
        
        private static FabricEndpointsTypeApplicationEndpoints GetWindowsFabricEndpointRange(IDictionary<string, RoleInstanceEndpoint> roleInstanceEndpoints, string endpointName, int applicationPortCount)
        {
            if (!roleInstanceEndpoints.ContainsKey(endpointName))
            {
                return null;
            }

            RoleInstanceEndpoint roleInstanceEndpoint = roleInstanceEndpoints[endpointName];            

            return new FabricEndpointsTypeApplicationEndpoints
            {
                StartPort = roleInstanceEndpoint.IPEndpoint.Port,
                EndPort = roleInstanceEndpoint.IPEndpoint.Port + applicationPortCount - 1,
            };
        }       

        private static string InputEndpointToString(InputEndpointType endpoint)
        {
            return endpoint == null ? string.Empty : string.Format(CultureInfo.InvariantCulture, "< {0}, {1} >", endpoint.Protocol, endpoint.Port);
        }

        private static string InternalEndpointToString(InternalEndpointType endpoint)
        {
            return endpoint == null ? string.Empty : string.Format(CultureInfo.InvariantCulture, "< {0}, {1} >", endpoint.Protocol, endpoint.Port);
        }

        private static string EndpointRangeToString(FabricEndpointsTypeApplicationEndpoints endpointRange)
        {
            return endpointRange == null ? string.Empty : string.Format(CultureInfo.InvariantCulture, "< {0} - {1} >", endpointRange.StartPort, endpointRange.EndPort);
        }
    }
}
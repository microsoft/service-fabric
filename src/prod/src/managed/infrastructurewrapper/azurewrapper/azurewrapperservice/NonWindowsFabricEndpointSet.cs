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
    
    internal class NonWindowsFabricEndpointSet
    {
        private NonWindowsFabricEndpointSet()
        {
        }

        internal Dictionary<string, InputEndpointType> NonWindowsFabricPorts { get; private set; }
        internal Dictionary<string, FabricEndpointsTypeApplicationEndpoints> NonWindowsFabricPortRanges { get; private set; }        

        internal static NonWindowsFabricEndpointSet GetNonWindowsFabricEndpointSet(IDictionary<string, RoleInstanceEndpoint> roleInstanceEndpointMap)
        {
            Dictionary<string, InputEndpointType> nonWindowsFabricPorts = new Dictionary<string, InputEndpointType>(StringComparer.OrdinalIgnoreCase);
            Dictionary<string, FabricEndpointsTypeApplicationEndpoints> nonWindowsFabricPortRanges = new Dictionary<string, FabricEndpointsTypeApplicationEndpoints>(StringComparer.OrdinalIgnoreCase);

            foreach (KeyValuePair<string, RoleInstanceEndpoint> roleInstanceEndpoint in roleInstanceEndpointMap)
            {
                string roleInstanceEndpointName = roleInstanceEndpoint.Key;                

                if (WindowsFabricAzureWrapperServiceCommon.FabricEndpointNames.FirstOrDefault(name => string.Compare(name, roleInstanceEndpointName, StringComparison.OrdinalIgnoreCase) == 0) == null)
                {
                    string portRangeConfigurationSettingName = string.Format(CultureInfo.InvariantCulture, WindowsFabricAzureWrapperServiceCommon.NonWindowsFabricPortRangeConfigurationSettingNameTemplate, roleInstanceEndpointName);

                    int portCount;
                    if (TryGetConfigurationSettingValueAsInt(portRangeConfigurationSettingName, out portCount))
                    {
                        int startPort = roleInstanceEndpoint.Value.IPEndpoint.Port;
                        int endPort = startPort + portCount - 1;
                        nonWindowsFabricPortRanges[roleInstanceEndpointName] = new FabricEndpointsTypeApplicationEndpoints { StartPort = startPort, EndPort = endPort };
                    }
                    else
                    {
                        int port = roleInstanceEndpoint.Value.IPEndpoint.Port;
                        nonWindowsFabricPorts[roleInstanceEndpointName] = new InputEndpointType { Port = port.ToString(CultureInfo.InvariantCulture), Protocol = InputEndpointTypeProtocol.tcp };
                    }
                }
            }

            return new NonWindowsFabricEndpointSet { NonWindowsFabricPorts = nonWindowsFabricPorts, NonWindowsFabricPortRanges = nonWindowsFabricPortRanges };
        }

        internal bool IsNonWindowsFabricEndpointSetChanged(NonWindowsFabricEndpointSet nonWindowsFabricEndpointSet)
        {
            if (nonWindowsFabricEndpointSet == null || 
                nonWindowsFabricEndpointSet.NonWindowsFabricPorts.Count != this.NonWindowsFabricPorts.Count || 
                nonWindowsFabricEndpointSet.NonWindowsFabricPortRanges.Count != this.NonWindowsFabricPortRanges.Count)
            {
                return true;
            }

            // Note that the name of endpoints are unique
            foreach (KeyValuePair<string, InputEndpointType> nonWindowsFabricPort in nonWindowsFabricEndpointSet.NonWindowsFabricPorts)
            {
                if (!this.NonWindowsFabricPorts.ContainsKey(nonWindowsFabricPort.Key) ||
                    string.Compare(nonWindowsFabricPort.Value.Port, this.NonWindowsFabricPorts[nonWindowsFabricPort.Key].Port, StringComparison.OrdinalIgnoreCase) != 0 ||
                    nonWindowsFabricPort.Value.Protocol != this.NonWindowsFabricPorts[nonWindowsFabricPort.Key].Protocol)
                {
                    return true;
                }
            }

            foreach (KeyValuePair<string, FabricEndpointsTypeApplicationEndpoints> nonWindowsFabricPortRange in nonWindowsFabricEndpointSet.NonWindowsFabricPortRanges)
            {
                if (!this.NonWindowsFabricPortRanges.ContainsKey(nonWindowsFabricPortRange.Key) ||
                    nonWindowsFabricPortRange.Value.StartPort != this.NonWindowsFabricPortRanges[nonWindowsFabricPortRange.Key].StartPort ||
                    nonWindowsFabricPortRange.Value.EndPort != this.NonWindowsFabricPortRanges[nonWindowsFabricPortRange.Key].EndPort)
                {
                    return true;
                }
            }

            return false;
        }
        
        private static bool TryGetConfigurationSettingValueAsInt(string configurationSettingName, out int configurationSettingValue)
        {
            configurationSettingValue = 0;

            try
            {
                string configurationSettingValueStr = RoleEnvironment.GetConfigurationSettingValue(configurationSettingName);

                return int.TryParse(configurationSettingValueStr, out configurationSettingValue);
            }
            catch (Exception e)
            {
                if (WindowsFabricAzureWrapperServiceCommon.IsFatalException(e))
                {
                    throw;
                }

                return false;
            }
        }        
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#define TRACE

namespace Microsoft.Fabric.InfrastructureWrapper
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.Net.Sockets;

    using Microsoft.WindowsAzure.ServiceRuntime;

    internal class NetworkConfigManager
    {
        public static bool ReduceDynamicPortRange(int dynamicStartPort, int dynamicEndPort)
        {                        
            int dynamicPortCount = dynamicEndPort - dynamicStartPort + 1;

            TextLogger.LogInfo("Reducing dynamic port range. Start port : {0}. Dynamic port count : {1}.", dynamicStartPort, dynamicPortCount);

            if (Socket.OSSupportsIPv4)
            {
                if (!ReduceDynamicPortRange(dynamicStartPort, dynamicPortCount, "ipv4", "tcp"))
                {
                    TextLogger.LogError("Failed to reduce dynamic port range for ipv4:tcp. Start port : {0}. Dynamic port count : {1}.", dynamicStartPort, dynamicPortCount);

                    return false;
                }

                if (!ReduceDynamicPortRange(dynamicStartPort, dynamicPortCount, "ipv4", "udp"))
                {
                    TextLogger.LogError("Failed to reduce dynamic port range for ipv4:udp. Start port : {0}. Dynamic port count : {1}.", dynamicStartPort, dynamicPortCount);

                    return false;
                }
            }

            if (Socket.OSSupportsIPv6)
            {
                if (!ReduceDynamicPortRange(dynamicStartPort, dynamicPortCount, "ipv6", "tcp"))
                {
                    TextLogger.LogError("Failed to reduce dynamic port range for ipv6:tcp. Start port : {0}. Dynamic port count : {1}.", dynamicStartPort, dynamicPortCount);

                    return false;
                }

                if (!ReduceDynamicPortRange(dynamicStartPort, dynamicPortCount, "ipv6", "udp"))
                {
                    TextLogger.LogError("Failed to reduce dynamic port range for ipv6:udp. Start port : {0}. Dynamic port count : {1}.", dynamicStartPort, dynamicPortCount);

                    return false;
                }
            }

            return true;
        }

        public static bool AllowNonWindowsFabricPortsTraffic(NonWindowsFabricEndpointSet nonWindowsFabricEndpointSet)
        {
            foreach (KeyValuePair<string, InputEndpointType> nonWindowsFabricPort in nonWindowsFabricEndpointSet.NonWindowsFabricPorts)
            {
                int port = int.Parse(nonWindowsFabricPort.Value.Port, CultureInfo.InvariantCulture);
                if (!AllowNonWindowsFabricPortsTraffic(nonWindowsFabricPort.Key, port))
                {
                    TextLogger.LogError("Failed to allow traffic to non Windows Fabric port. Endpoint name : {0}. Port : {1}.", nonWindowsFabricPort.Key, port);

                    return false;
                }
            }

            foreach (KeyValuePair<string, FabricEndpointsTypeApplicationEndpoints> nonWindowsFabricPortRange in nonWindowsFabricEndpointSet.NonWindowsFabricPortRanges)
            {
                if (!AllowNonWindowsFabricPortsTraffic(nonWindowsFabricPortRange.Key, nonWindowsFabricPortRange.Value.StartPort, nonWindowsFabricPortRange.Value.EndPort))
                {
                    TextLogger.LogError(
                        "Failed to allow traffic to non Windows Fabric port range. Endpoint name : {0}. Start Port : {1}. End Port : {2}.",
                        nonWindowsFabricPortRange.Key,
                        nonWindowsFabricPortRange.Value.StartPort,
                        nonWindowsFabricPortRange.Value.EndPort);

                    return false;
                }
            }

            return true;
        }

        public static bool AllowDynamicPortsTraffic(int dynamicStartPort, int dynamicEndPort)
        {            
            if (!AllowDynamicPortsTraffic("tcp", dynamicStartPort, dynamicEndPort))
            {
                TextLogger.LogError("Failed to allow traffic to dynamic port range for tcp. Start port : {0}. End port : {1}.", dynamicStartPort, dynamicEndPort);

                return false;
            }

            if (!AllowDynamicPortsTraffic("udp", dynamicStartPort, dynamicEndPort))
            {
                TextLogger.LogError("Failed to allow traffic to dynamic port range for udp. Start port : {0}. End port : {1}.", dynamicStartPort, dynamicEndPort);

                return false;
            }

            return true;
        }

        private static bool ReduceDynamicPortRange(int startPort, int portCount, string networkInterface, string transportProtocol)
        {
            int exitCode = WindowsFabricAzureWrapperServiceCommon.ExecuteNetshCommand(
                string.Format(
                    CultureInfo.InvariantCulture,
                    "int {0} set dynamicport {1} start={2} num={3}",
                    networkInterface,
                    transportProtocol,
                    startPort,
                    portCount));

            return exitCode == 0;
        }

        private static bool AllowNonWindowsFabricPortsTraffic(string name, int startPort)
        {
            return AllowNonWindowsFabricPortsTraffic(name, startPort, startPort);
        }

        private static bool AllowNonWindowsFabricPortsTraffic(string name, int startPort, int endPort)
        {
            string firewallRuleName = string.Format(
                CultureInfo.InvariantCulture,
                "Windows Fabric Endpoint - {0} ({1})",
                name,
                RoleEnvironment.CurrentRoleInstance.Id);

            string deleteExistingRule = string.Format(
                CultureInfo.InvariantCulture,
                "advfirewall firewall delete rule name=\"{0}\"",
                firewallRuleName);

            string addAllowRule;
            if (startPort == endPort)
            {
                addAllowRule = string.Format(
                    CultureInfo.InvariantCulture,
                    "advfirewall firewall add rule name=\"{0}\" action=allow dir=in protocol=tcp localport={1}",
                    firewallRuleName,
                    startPort);
            }
            else
            {
                addAllowRule = string.Format(
                  CultureInfo.InvariantCulture,
                  "advfirewall firewall add rule name=\"{0}\" action=allow dir=in protocol=tcp localport={1}-{2}",
                  firewallRuleName,
                  startPort,
                  endPort);
            }

            WindowsFabricAzureWrapperServiceCommon.ExecuteNetshCommand(deleteExistingRule);

            int exitCode = WindowsFabricAzureWrapperServiceCommon.ExecuteNetshCommand(addAllowRule);

            return exitCode == 0;
        }   

        private static bool AllowDynamicPortsTraffic(string protocol, int startPort, int endPort)
        {
            string firewallRuleName = string.Format(
                CultureInfo.InvariantCulture,
                "Windows Fabric Dynamic Ports - {0} ({1})",
                protocol,
                RoleEnvironment.CurrentRoleInstance.Id);

            string deleteExistingRule = string.Format(
                CultureInfo.InvariantCulture,
                "advfirewall firewall delete rule name=\"{0}\"",
                firewallRuleName);

            string addAllowRule = string.Format(
                CultureInfo.InvariantCulture,
                "advfirewall firewall add rule name=\"{0}\" action=allow dir=in protocol={1} localport={2}-{3}",
                firewallRuleName,
                protocol,
                startPort,
                endPort);

            WindowsFabricAzureWrapperServiceCommon.ExecuteNetshCommand(deleteExistingRule);

            int exitCode = WindowsFabricAzureWrapperServiceCommon.ExecuteNetshCommand(addAllowRule);

            return exitCode == 0;
        }
    }
}
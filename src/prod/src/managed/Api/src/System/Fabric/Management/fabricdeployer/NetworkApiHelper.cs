// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using Microsoft.Win32;
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Net;
#if DotNetCoreClrLinux	
    using System.Net.NetworkInformation;
#endif	
    using System.Net.Sockets;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Threading.Tasks;

    public static class NetworkApiHelper
    {
        internal class HostNameComparer : IEqualityComparer<string>
        {

            public bool Equals(string hostName1, string hostName2)
            {
                IdnMapping mapping = new IdnMapping();
                string hostName1Ascii = mapping.GetAscii(hostName1);
                string hostName2Ascii = mapping.GetAscii(hostName2);
                return hostName1Ascii.Equals(hostName2Ascii, StringComparison.OrdinalIgnoreCase);
            }

            public int GetHashCode(string hostName)
            {
                if (hostName == null)
                {
                    throw new ArgumentNullException("hostName");
                }
                return hostName.ToUpperInvariant().GetHashCode();
            }
        }

        internal static HashSet<string> CurrentMachineNames = NetworkApiHelper.GetCurrentMachineNames();

        internal static HashSet<IPAddress> CurrentMachineAddresses = NetworkApiHelper.GetCurrentMachineAddresses();

        private static HashSet<string> GetCurrentMachineNames()
        {
            HashSet<string> currentMachineNames = new HashSet<string>(new HostNameComparer());
            IPHostEntry entry = Helpers.GetHostEntryFromName(Dns.GetHostName()).Result;
            string domainName = System.Net.NetworkInformation.IPGlobalProperties.GetIPGlobalProperties().DomainName;
            if (!entry.HostName.Contains(domainName))
            {
                currentMachineNames.Add(entry.HostName);  //add computername
                currentMachineNames.Add(entry.HostName + "." + domainName);  //add FQDN
            }
            else
            {
                currentMachineNames.Add(entry.HostName);  //addFQDN
                string[] strs = entry.HostName.Split('.');
                if (strs.Length > 0)
                {
                    currentMachineNames.Add(strs[0]);   //add computername
                }
            }

            currentMachineNames.Add(Constants.LocalHostMachineName);

            return currentMachineNames;
        }

        internal static HashSet<IPAddress> GetCurrentMachineAddresses()
        { 
#if !DotNetCoreClrLinux
            IPHostEntry entry = Helpers.GetHostEntryFromName(Dns.GetHostName()).Result;
            HashSet<IPAddress> currentMachineAddresses = new HashSet<IPAddress>(entry.AddressList);
            currentMachineAddresses.Add(IPAddress.Loopback);
            currentMachineAddresses.Add(IPAddress.IPv6Loopback);
#else
            HashSet<IPAddress> currentMachineAddresses = new HashSet<IPAddress>();

            foreach (NetworkInterface ni in NetworkInterface.GetAllNetworkInterfaces())
            {
                foreach (UnicastIPAddressInformation ip in ni.GetIPProperties().UnicastAddresses)
                {
                    currentMachineAddresses.Add(ip.Address);
                }
            }
#endif
            DeployerTrace.WriteInfo("The current machine IP addresses are: {0}", string.Join(", ", currentMachineAddresses.ToArray().Select(IP => IP.ToString())));
            return currentMachineAddresses;
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        internal static bool IsNodeForThisMachine(InfrastructureNodeType node)
        {
            return IsAddressForThisMachine(node.IPAddressOrFQDN);
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        internal static bool IsAddressForThisMachine(string address)
        {
            IPAddress ipAddress;
            if (IPAddress.TryParse(address, out ipAddress))
            {
                return NetworkApiHelper.CurrentMachineAddresses.Contains(ipAddress) ||
                         IsAddressLoopback(address);
            }
            else
            {
                return NetworkApiHelper.CurrentMachineNames.Contains(address);
            }
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        internal static bool IsAddressLoopback(string address)
        {
            IPAddress ipAddress;
            if (IPAddress.TryParse(address, out ipAddress))
            {
                return IPAddress.IsLoopback(ipAddress);
            }
            else
            {
                IdnMapping mapping = new IdnMapping();
                string inputHostName = mapping.GetAscii(address);
                string localhost = mapping.GetAscii(Constants.LocalHostMachineName);
                return inputHostName.Equals(localhost, StringComparison.OrdinalIgnoreCase);
            }
        }

        internal static bool ReduceDynamicPortRange(List<NodeSettings> nodes, bool isScaleMin)
        {
            if (isScaleMin && NetworkApiHelper.IsAddressLoopback(nodes[0].IPAddressOrFQDN))
            {
                return true;
            }

            List<Tuple<int, int>> portRanges = GetDynamicPortRangeForNodes(nodes);
            foreach(Tuple<int, int> portRange in portRanges)
            {
                ReduceDynamicPortRange(portRange.Item1, portRange.Item2);
            }

            return true;
        }

        internal static bool IsUriReachable(Uri uri, string requestMethod)
        {
            return IsUriReachableAsync(uri, requestMethod, Constants.UriReachableTimeoutInMs, Constants.UriRequestTimeoutInMs, Constants.UriReachableRetryIntervalInMs).Result;
        }

        internal static async Task<bool> IsUriReachableAsync(Uri uri, string requestMethod, int operationTimeoutInMs, int requestTimeoutInMs, int retryIntervalInMs)
        {
            ReleaseAssert.AssertIf(uri == null, "uri cannot be null for IsUriReachableAsync.");
            if (uri.IsFile && !uri.IsUnc)
            {
                FileInfo fi = new FileInfo(uri.LocalPath);
                return fi.Exists;
            }
            else
            {
                if (string.IsNullOrWhiteSpace(uri.Host))
                {
                    return false;
                }

                var timeout = new System.Fabric.Common.TimeoutHelper(TimeSpan.FromMilliseconds(operationTimeoutInMs));
                while (!System.Fabric.Common.TimeoutHelper.HasExpired(timeout))
                {
                    WebRequest request = WebRequest.Create(uri);
#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
                    request.Timeout = requestTimeoutInMs;
#endif
                    request.Method = requestMethod;
                    try
                    {
                        using (WebResponse response = await request.GetResponseAsync().ConfigureAwait(false))
                        {
                            if (response is HttpWebResponse)
                            {
                                if (((HttpWebResponse)response).StatusCode == HttpStatusCode.OK)
                                {
                                    return true;
                                }

                                return false;
                            }
                            else
                            {
                                return response.ContentLength > 0;
                            }
                        }
                    }
                    catch (WebException ex)
                    {
                        DeployerTrace.WriteNoise("Unable to reach uri {0} becaue {1}.", uri, ex.Message);
                    }

                    System.Threading.Thread.Sleep(retryIntervalInMs);
                }
            }

            return false;
        }


        private static List<Tuple<int, int>> GetDynamicPortRangeForNodes(List<NodeSettings> nodes)
        {
            List<Tuple<int, int>> dynamicPortRanges = new List<Tuple<int, int>>(); 
            foreach(NodeSettings node in nodes)
            {
                int startPort, endPort;
                GetPorts(node, out startPort, out endPort);
                dynamicPortRanges.Add(new Tuple<int, int>(startPort, endPort)); 
            }

            return dynamicPortRanges; 
        }

        private static void GetPorts(NodeSettings setting, out int startPort, out int endPort)
        {
            SettingsTypeSection fabricNodeSection = setting.Settings.FirstOrDefault(section => section.Name.Equals(Constants.SectionNames.FabricNode, StringComparison.OrdinalIgnoreCase));
            
            var startDynamicPortParamter = fabricNodeSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.StartDynamicPortRange, StringComparison.OrdinalIgnoreCase));
            var endDynamicnPortParamter = fabricNodeSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.EndDynamicPortRange, StringComparison.OrdinalIgnoreCase));

            if (startDynamicPortParamter != null && endDynamicnPortParamter != null)
            {
                int.TryParse(startDynamicPortParamter.Value, out startPort);
                int.TryParse(endDynamicnPortParamter.Value, out endPort); 
            }
            else
            {
                startPort = 0;
                endPort = 0;
            }
        }

        private static bool ReduceDynamicPortRange(int dynamicStartPort, int dynamicEndPort)
        {
            if (dynamicStartPort == 0 && dynamicEndPort == 0)
            {
                DeployerTrace.WriteInfo("Dynamic port range not specified in cluster manifest. Start port : {0}. Using default dynamic port range for server.", dynamicStartPort);
                return true;
            }

#if DotNetCoreClrLinux
            DeployerTrace.WriteInfo("Reducing dynamic port range. Start port : {0}. End port  : {1}.", dynamicStartPort, dynamicEndPort);
            if (! ReduceDynamicPortRangeForLinux(dynamicStartPort, dynamicEndPort))
            {
                DeployerTrace.WriteError("Failed to reduce dynamic port range for  Start port : {0}. End port {1}.", dynamicStartPort, dynamicEndPort);
                return false;
            }
#else
            int dynamicPortCount = dynamicEndPort - dynamicStartPort + 1;

            DeployerTrace.WriteInfo("Reducing dynamic port range. Start port : {0}. Dynamic port count : {1}.", dynamicStartPort, dynamicPortCount);

            if (Socket.OSSupportsIPv4)
            {
                if (!ReduceDynamicPortRange(dynamicStartPort, dynamicPortCount, "ipv4", "tcp"))
                {
                    DeployerTrace.WriteError("Failed to reduce dynamic port range for ipv4:tcp. Start port : {0}. Dynamic port count : {1}.", dynamicStartPort, dynamicPortCount);

                    return false;
                }

                if (!ReduceDynamicPortRange(dynamicStartPort, dynamicPortCount, "ipv4", "udp"))
                {
                    DeployerTrace.WriteError("Failed to reduce dynamic port range for ipv4:udp. Start port : {0}. Dynamic port count : {1}.", dynamicStartPort, dynamicPortCount);

                    return false;
                }
            }

            if (Socket.OSSupportsIPv6)
            {
                if (!ReduceDynamicPortRange(dynamicStartPort, dynamicPortCount, "ipv6", "tcp"))
                {
                    DeployerTrace.WriteError("Failed to reduce dynamic port range for ipv6:tcp. Start port : {0}. Dynamic port count : {1}.", dynamicStartPort, dynamicPortCount);

                    return false;
                }

                if (!ReduceDynamicPortRange(dynamicStartPort, dynamicPortCount, "ipv6", "udp"))
                {
                    DeployerTrace.WriteError("Failed to reduce dynamic port range for ipv6:udp. Start port : {0}. Dynamic port count : {1}.", dynamicStartPort, dynamicPortCount);

                    return false;
                }
            }
#endif

            return true;
        }

        private static bool ReduceDynamicPortRange(int startPort, int portCount, string networkInterface, string transportProtocol)
        {
            int exitCode = Utility.ExecuteNetshCommand(
                string.Format(
                    CultureInfo.InvariantCulture,
                    "int {0} set dynamicport {1} start={2} num={3}",
                    networkInterface,
                    transportProtocol,
                    startPort,
                    portCount));

            return exitCode == 0;
        }

#if DotNetCoreClrLinux
        private static bool ReduceDynamicPortRangeForLinux(int dynamicStartPort, int dynamicEndPort)
        {
            string updatePortCommand = String.Format("echo {0} {1} > /proc/sys/net/ipv4/ip_local_port_range ", dynamicStartPort, dynamicEndPort);
            return  System.Fabric.Management.Common.NativeHelper.system(updatePortCommand) ==0;
        }
#endif
    }
}
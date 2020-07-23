// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Net;
    using System.Net.NetworkInformation;
    using System.Threading.Tasks;
    using Microsoft.Win32;
    using NetFwTypeLib;
    using System.Runtime.InteropServices;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Net.Sockets;

    /// <summary>
    /// Helper class for setting up a Service Fabric specific DNS server (primarily for Docker scenarios).
    /// Background:
    /// Shortly, SF services can be hosted in Docker containers. Also, SF services (and guest EXEs) can communicate with
    /// each other with a DNS name (e.g. myservice.myapp.com instead of the SF specific name like fabric:/myapp/myservice).
    /// The DNS name to SF name resolution is being done by a system service called DnsService, an instance of which
    /// resides on each SF node.
    /// When a Docker container is created, we can provide a --dns parameter to the 'docker run' command.
    /// Docker daemon uses this parameter to redirect DNS requests to an alternate DNS server in case the
    /// built-in Docker DNS server cannot fulfill the request.
    /// In this class, we go through some setup steps to create an alternate DNS server.
    /// The SF services residing inside separate Docker containers communicate with each other via
    /// DnsService which in turn uses the alternate DNS server.
    /// </summary>
    internal sealed class DockerDnsHelper
    {
        /// <summary>
        /// DNS for Service Fabric.
        /// </summary>

#if DotNetCoreClr
        private const string DockerDnsNetworkAdapterName = "docker";
#else
        private const string DockerDnsNetworkAdapterName = "HNS Internal NIC";
        private const string SFNetworkAdapterName = "DNSFORSF";
#endif

        private const string SwitchName = "nat";

        private const string FirewallGroupName = FabricNodeFirewallRules.WindowsFabricGrouping;
        private const string FirewallRuleName = FirewallGroupName + ".Dns.v1";
        private const string FirewallRuleDescription = "Inbound rule for ServiceFabric DNS operations";

        /// <summary>
        /// Registry value to persist the DNS server IP address. This is used by hosting subsystem when
        /// creating the Docker container with the --dns parameter
        /// </summary>
        private const string DnsServerIPAddressRegValue = "FabricDnsServerIPAddress";

        /// <summary>
        /// Firewall specific constant for adding firewall rules.
        /// </summary>
        private const string FirewallPolicy2TypeProgID = "HNetCfg.FwPolicy2";

        /// <summary>
        /// The feature name as reported by the OS. If this feature exists, then the DNS setup will continue.
        /// </summary>
        private const string ContainersFeatureName = "Containers";

        /// <summary>
        /// The fixed port on which the DnsService communicates
        /// </summary>
        private const int PortNumber = 53;

        private const int MaxRetries = 3;
        private const int RetryDelayInSeconds = 15;

        private readonly DeploymentParameters deploymentParameters;
        private readonly string nodeIPAddressOrFQDN;

        public DockerDnsHelper(DeploymentParameters parameters, string ipAddressOrFQDN)
        {
            DeployerTrace.WriteInfo("DockerDnsHelper current node IPAddressOrFQDN <{0}>", ipAddressOrFQDN);

            this.deploymentParameters = parameters ?? new DeploymentParameters();
            this.nodeIPAddressOrFQDN = string.IsNullOrEmpty(ipAddressOrFQDN) ? string.Empty : ipAddressOrFQDN;
        }

        /// <summary>
        /// Does the following
        /// a. Gets the Service Fabric specific DNS network adapter (creates one if it doesn't exist)
        /// b. Adds the IP addresses that adapter can handle (and are assigned to Docker containers)
        /// c. Adds firewall rules to enable DNS
        /// d. Persists relevant data so that the Service Fabric hosting layer can create Docker
        ///    containers with the required parameters (e.g. an IP address for the container)
        /// </summary>
        /// <remarks>
        /// 1. This currently works only on Windows.
        ///    When this code is merged with the code in the Linux branch, care has to be taken to suitably ifdef this
        /// 2. Please ensure that all operations are idempotent
        /// 3. Please use PowerShell scripts only if there is no simple .NET alternative.
        /// </remarks>
        /// <example>
        /// To use this for standalone testing, use the below commands on a VM
        /// FabricDeployer.exe /operation:DockerDnsSetup
        /// FabricDeployer.exe /operation:DockerDnsCleanup
        /// </example>
        public async Task SetupAsync()
        {
            DeployerTrace.WriteInfo("Starting DockerDnsHelper.SetupAsync");

            try
            {
                await ExecuteAsync(SetupInternal, "DockerDnsHelper.SetupAsync");
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteWarning(StringResources.Warning_FabricDeployer_DockerDnsSetup_SetupError, ex);
                throw;
            }

            DeployerTrace.WriteInfo("Completed DockerDnsHelper.SetupAsync");
        }

        public async Task CleanupAsync()
        {
            DeployerTrace.WriteInfo("Starting DockerDnsHelper.CleanupAsync");

            try
            {
                await ExecuteAsync(CleanupInternal, "DockerDnsHelper.CleanupAsync");
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteWarning(StringResources.Warning_FabricDeployer_DockerDnsSetup_CleanupError, ex);
            }

            DeployerTrace.WriteInfo("Completed DockerDnsHelper.CleanupAsync");
        }

        private static async Task ExecuteAsync(Action func, string funcName)
        {
            for (int i = 0; i < MaxRetries; i++)
            {
                DeployerTrace.WriteInfo("Executing {0}, attempt {1} of {2}", funcName, i + 1, MaxRetries);

                try
                {
                    // all operations are idempotent, so retry from beginning is okay
                    func();
                    DeployerTrace.WriteInfo("Successfully completed {0}", funcName);
                    break;
                }
                catch (Exception ex)
                {
                    string message = string.Format(
                        CultureInfo.CurrentUICulture,
                        StringResources.Warning_FabricDeployer_DockerDnsSetup_RetryMessage1, funcName, i + 1, MaxRetries, ex);

                    if (i == MaxRetries - 1)
                    {
                        message += string.Format(CultureInfo.CurrentUICulture, StringResources.Warning_FabricDeployer_DockerDnsSetup_RetryExhausted, Environment.NewLine);
                        DeployerTrace.WriteWarning(message);
                        throw;
                    }

                    message += string.Format(CultureInfo.CurrentUICulture, StringResources.Warning_FabricDeployer_DockerDnsSetup_RetryContinue, Environment.NewLine);
                    DeployerTrace.WriteWarning(message);
                }

                await Task.Delay(TimeSpan.FromSeconds(RetryDelayInSeconds));
            }
        }

        private void SetupInternal()
        {
#if !DotNetCoreClr // Disable compiling on windows for now. Need to correct when porting FabricDeployer for windows.
            bool proceed = ValidateContainersFeature();
            if (!proceed)
            {
                return;
            }
#endif

            string dnsIpAddressString = string.Empty;
            NetworkInterface dockerDnsNic = GetNetworkAdapter(DockerDnsNetworkAdapterName);
            if (dockerDnsNic == null)
            {
                // nodeIPAddressOrFQDN is a hostname (not an IP address) which means that nodes in the cluster manifest
                // are actuall DNS names. This is usually the case for on-prem clusters or dev-box clusters.
                if (!IPAddress.TryParse(this.nodeIPAddressOrFQDN, out IPAddress address))
                {
                    try
                    {
                        using (Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, 0))
                        {
                            socket.Connect("1.2.3.4", 12345); // doesnt matter what it connects to
                            IPEndPoint endPoint = socket.LocalEndPoint as IPEndPoint;
                            DeployerTrace.WriteInfo("Found outgoing interface <{0}>", endPoint.Address.ToString());
                            dnsIpAddressString = endPoint.Address.ToString();
                        }
                    }
                    catch (Exception ex)
                    {
                        DeployerTrace.WriteWarning("Failed to find the outgoing interface, exception {0}", ex.ToString());
                    }
                }
                // Cluster is created using IP adddresses, use the IP address of the node as the preferred DNS address.
                else
                {
                    dnsIpAddressString = this.nodeIPAddressOrFQDN;
                }

                CreateFirewallRule();
            }
            else
            {
#if !DotNetCoreClr // Disable compiling on windows for now. Need to correct when porting FabricDeployer for windows.
                int dockerDnsNicInterfaceIndex = GetInterfaceIndex(dockerDnsNic, DockerDnsNetworkAdapterName);
                IPAddress dockerDnsIPAddress = GetIPAddress(dockerDnsNicInterfaceIndex);
#else
                IPAddress dockerDnsIPAddress = GetFirstUnicastAddress(dockerDnsNic, DockerDnsNetworkAdapterName);
#endif
                IPAddress dockerDnsSubnetMask = GetSubnetMask(dockerDnsNic, dockerDnsIPAddress, DockerDnsNetworkAdapterName);
                int dockerDnsPrefixLength = GetPrefixLength(dockerDnsSubnetMask, DockerDnsNetworkAdapterName);

                IPAddress dnsIpAddress = dockerDnsIPAddress;
#if !DotNetCoreClr // Disable compiling on windows for now. Need to correct when porting FabricDeployer for windows.
                NetworkInterface sfNic = GetNetworkAdapter(SFNetworkAdapterName);

                if (sfNic == null)
                {
                    AddNetworkAdapter(SFNetworkAdapterName);
                    sfNic = GetNetworkAdapter(SFNetworkAdapterName);
                    if (sfNic == null)
                    {
                        var message = string.Format(
                            CultureInfo.CurrentUICulture,
                            StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorGettingNetworkAdapter2, SFNetworkAdapterName);
                        DeployerTrace.WriteWarning(message);
                        throw new InvalidOperationException(message);
                    }
                }

                int sfNicInterfaceIndex = GetInterfaceIndex(sfNic, SFNetworkAdapterName);

                IPAddress newIPAddressForSFNic = GetNewIPAddressForSFNic(dockerDnsIPAddress);
                AssignNewIPAddress(newIPAddressForSFNic, dockerDnsPrefixLength, sfNicInterfaceIndex);

                dnsIpAddress = newIPAddressForSFNic;
#endif

                CreateFirewallRule();

                dnsIpAddressString = dnsIpAddress.ToString();
            }

            var persistData = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
                {
                    { DnsServerIPAddressRegValue, dnsIpAddressString },
                };

            PersistNetworkDetailForContainerStart(persistData);
        }

        private void CleanupInternal()
        {
#if !DotNetCoreClr // Disable compiling on windows for now. Need to correct when porting FabricDeployer for windows.
            bool proceed = ValidateContainersFeature();
            if (!proceed)
            {
                return;
            }
#endif

            CleanupPersistedNetworkDetail(new List<string> { DnsServerIPAddressRegValue });
            RemoveFirewallRule();

#if !DotNetCoreClr // Disable compiling on windows for now. Need to correct when porting FabricDeployer for windows.
            NetworkInterface nic = GetNetworkAdapter(SFNetworkAdapterName);

            if (nic != null)
            {
                int interfaceIndex = GetInterfaceIndex(nic, SFNetworkAdapterName, false);
                if (interfaceIndex == -1)
                {
                    DeployerTrace.WriteWarning(StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorGettingInterfaceIndex, SFNetworkAdapterName);
                    return;
                }

                IPAddress ipAddress = GetIPAddress(interfaceIndex, false);
                if (ipAddress == null)
                {
                    DeployerTrace.WriteWarning(StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorGettingIPAddress, SFNetworkAdapterName);
                    return;
                }

                RemoveIPAddress(ipAddress, interfaceIndex);
                RemoveNetworkAdapter(SFNetworkAdapterName);
            }
            else
            {
                DeployerTrace.WriteWarning(StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorGettingNetworkAdapter3, SFNetworkAdapterName);
            }
#endif
        }

#if !DotNetCoreClr // Disable compiling on windows for now. Need to correct when porting FabricDeployer for windows.
        internal bool ValidateContainersFeature()
        {
            if (IsContainersFeaturePresent())
            {
                DeployerTrace.WriteInfo("{0} feature present", ContainersFeatureName);
                return true;
            }

            // on a system where the above checks fail for some reason, then we can at least move forward
            if (deploymentParameters.ContinueIfContainersFeatureNotInstalled)
            {
                DeployerTrace.WriteWarning(StringResources.Warning_FabricDeployer_DockerDnsSetup_FeatureNotPresentContinuing, ContainersFeatureName);
                return true;
            }

            DeployerTrace.WriteInfo("{0} feature not present. Not continuing further", ContainersFeatureName);
            return false;
        }

        public static bool IsContainersFeaturePresent()
        {
            try
            {
                DismPackageFeatureState featureState = DismHelper.GetFeatureState(ContainersFeatureName);

                switch (featureState)
                {
                    case DismPackageFeatureState.DismStateInstallPending:
                    case DismPackageFeatureState.DismStateInstalled:
                    case DismPackageFeatureState.DismStateSuperseded:
                        return true;
                }
            }
            catch (COMException ex)
            {
                // Dism COM errors aren't well understood yet across different OSes. TODO Handle specific error codes and throw the rest
                DeployerTrace.WriteWarning(StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorGettingFeatureState1, ContainersFeatureName, ex);
            }
            catch (DllNotFoundException ex)
            {
                // This happens on platforms that don't have dismapi.dll
                // https://technet.microsoft.com/en-us/library/hh825186.aspx
                DeployerTrace.WriteWarning(
                    StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorGettingFeatureState2,
                    ContainersFeatureName,
                    ex);
            }
            catch (Exception ex)
            {
                // Swallowing all!
                // Setup for Docker isn't a mainline scenario currently. Hence try and continue.
                // If an exception is thrown on an OS that has Docker, and setup doesn't continue, we have mitigation
                // steps on the VM by setting ContinueIfContainersFeatureNotInstalled.
                // E.g. FabricDeployer.exe /operation:DockerDnsSetup /continueIfContainersFeatureNotInstalled:true
                DeployerTrace.WriteWarning(
                    StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorGettingFeatureState3,
                    ContainersFeatureName,
                    ex);
            }

            return false;
        }
#endif

        internal static NetworkInterface GetNetworkAdapter(string networkAdapterName)
        {
            DeployerTrace.WriteInfo("Getting network adapter {0}...", networkAdapterName);

            foreach (NetworkInterface nic in NetworkInterface.GetAllNetworkInterfaces())
            {
                if (nic.Name.IndexOf(networkAdapterName, StringComparison.OrdinalIgnoreCase) != -1)
                {
                    DeployerTrace.WriteInfo("Network adapter {0} successfully obtained, name: {1}", networkAdapterName, nic.Name);
                    return nic;
                }
            }

            DeployerTrace.WriteInfo("Unable to detect network adapter {0}, returning <null>", networkAdapterName);
            return null;
        }

#if !DotNetCoreClr // Disable compiling on windows for now. Need to correct when porting FabricDeployer.
        // TODO, convert to real C# calls
        internal static void AddNetworkAdapter(string networkAdapterName)
        {
            DeployerTrace.WriteInfo("Adding network adapter {0}...", networkAdapterName);

            string script = "Add-VMNetworkAdapter -Name {0} -ManagementOS -SwitchName {1}"
                .ToFormat(networkAdapterName, SwitchName);

            var psObjects = Utility.ExecutePowerShellScript(script, false);
            if (psObjects != null)
            {
                DeployerTrace.WriteInfo("PowerShell Invoke completed and returned {0} objects", psObjects.Count);

                foreach (var psObject in psObjects)
                {
                    DeployerTrace.WriteInfo("{0}", psObject);
                }
            }
            else
            {
                DeployerTrace.WriteInfo("PowerShell Invoke completed and returned null");
            }

            DeployerTrace.WriteInfo("Network adapter {0} added successfully", networkAdapterName);
        }

        internal static void RemoveNetworkAdapter(string networkAdapterName)
        {
            DeployerTrace.WriteInfo("Removing network adapter {0}...", networkAdapterName);

            string script = "Remove-VMNetworkAdapter -Name {0} -ManagementOS -SwitchName {1}"
                .ToFormat(networkAdapterName, SwitchName);

            var psObjects = Utility.ExecutePowerShellScript(script, false);
            if (psObjects != null)
            {
                DeployerTrace.WriteInfo("PowerShell Invoke completed and returned {0} objects", psObjects.Count);

                foreach (var psObject in psObjects)
                {
                    DeployerTrace.WriteInfo("{0}", psObject);
                }
            }
            else
            {
                DeployerTrace.WriteInfo("PowerShell Invoke completed and returned null");
            }

            DeployerTrace.WriteInfo("Network adapter {0} removed successfully", networkAdapterName);
        }

        internal static int GetInterfaceIndex(NetworkInterface nic, string networkAdapterName, bool throwOnError = true)
        {
            DeployerTrace.WriteInfo("Calling GetIPProperties() for network adapter {0}", networkAdapterName);
            IPInterfaceProperties ipProperties = nic.GetIPProperties();
            if (ipProperties == null)
            {
                var message = string.Format(
                    CultureInfo.CurrentUICulture,
                    StringResources.Warning_FabricDeployer_DockerDnsSetup_GetIPPropertiesError, networkAdapterName);

                DeployerTrace.WriteWarning(message);

                if (throwOnError)
                {
                    throw new InvalidOperationException(message);
                }

                return -1;
            }

            DeployerTrace.WriteInfo("Calling GetIPv4Properties() for network adapter {0}", networkAdapterName);
            IPv4InterfaceProperties ipv4Properties = ipProperties.GetIPv4Properties();
            if (ipv4Properties == null)
            {
                var message = string.Format(
                    CultureInfo.CurrentUICulture,
                    StringResources.Warning_FabricDeployer_DockerDnsSetup_GetIPv4PropertiesError, networkAdapterName);

                DeployerTrace.WriteWarning(message);

                if (throwOnError)
                {
                    throw new InvalidOperationException(message);
                }

                return -1;
            }

            DeployerTrace.WriteInfo("Returning InterfaceIndex: {0} for network adapter {1}", ipv4Properties.Index, networkAdapterName);
            return ipv4Properties.Index;
        }

        internal static IPAddress GetIPAddress(int interfaceIndex, bool throwOnError = true)
        {
            DeployerTrace.WriteInfo("Getting IP address for InterfaceIndex: {0}", interfaceIndex);

            string script = "(Get-NetIPConfiguration -InterfaceIndex {0}).IPv4Address.IPAddress".ToFormat(interfaceIndex);
            var psObjects = Utility.ExecutePowerShellScript(script, true);

            if (psObjects == null || psObjects.Count != 1 || psObjects[0] == null)
            {
                string message = string.Format(
                    CultureInfo.CurrentUICulture,
                    StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorGettingIPAddressForInterfaceIndex, interfaceIndex);
                DeployerTrace.WriteWarning(message);

                if (throwOnError)
                {
                    throw new InvalidOperationException(message);
                }

                return null;
            }

            string ipAddress = psObjects[0].ToString();

            DeployerTrace.WriteInfo("Successfully obtained IP address: {0}", ipAddress);

            IPAddress ip;
            bool status = IPAddress.TryParse(ipAddress, out ip);
            if (!status)
            {
                string message = string.Format(
                    CultureInfo.CurrentUICulture,
                    StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorParsingIPAddressForInterfaceIndex, ipAddress, interfaceIndex);
                DeployerTrace.WriteWarning(message);

                if (throwOnError)
                {
                    throw new InvalidOperationException(message);
                }

                return null;
            }

            return ip;
        }
#endif

        internal static IPAddress GetFirstUnicastAddress(NetworkInterface nic, string networkAdapterName)
        {
            DeployerTrace.WriteInfo("Getting first unicast address for network adapter {0}", networkAdapterName);

            IPInterfaceProperties ipProperties = nic.GetIPProperties();
            if (ipProperties == null)
            {
                var message = string.Format(
                    CultureInfo.CurrentUICulture,
                    StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorGettingSubnetMask1, networkAdapterName);

                DeployerTrace.WriteWarning(message);
                throw new InvalidOperationException(message);

            }

            UnicastIPAddressInformationCollection ipInfos = ipProperties.UnicastAddresses;
            foreach (UnicastIPAddressInformation ipInfo in ipInfos)
            {
                return ipInfo.Address;
            }

            var message2 = string.Format(
                CultureInfo.CurrentUICulture,
                StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorGettingSubnetMask2, networkAdapterName);

            DeployerTrace.WriteWarning(message2);
            throw new InvalidOperationException(message2);
        }

        internal static IPAddress GetSubnetMask(NetworkInterface nic, IPAddress ipAddress, string networkAdapterName)
        {
            DeployerTrace.WriteInfo("Getting subnet mask for IP address {0} and network adapter {1}", ipAddress, networkAdapterName);

            IPInterfaceProperties ipProperties = nic.GetIPProperties();
            if (ipProperties == null)
            {
                var message = string.Format(
                    CultureInfo.CurrentUICulture,
                    StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorGettingSubnetMask1, networkAdapterName);

                DeployerTrace.WriteWarning(message);
                throw new InvalidOperationException(message);

            }

            UnicastIPAddressInformationCollection ipInfos = ipProperties.UnicastAddresses;
            foreach (UnicastIPAddressInformation ipInfo in ipInfos)
            {
                // ToString() is the only valid way to get the IP address. There is no other explicit property
                if (string.Equals(ipInfo.Address.ToString(), ipAddress.ToString(), StringComparison.OrdinalIgnoreCase))
                {
                    DeployerTrace.WriteInfo("Successfully obtained subnet mask {0} for IP address {1} and network adapter {2}", ipInfo.IPv4Mask, ipAddress, networkAdapterName);
                    return ipInfo.IPv4Mask;
                }
            }

            var message2 = string.Format(
                CultureInfo.CurrentUICulture,
                StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorGettingSubnetMask2, ipAddress, networkAdapterName);

            DeployerTrace.WriteWarning(message2);
            throw new InvalidOperationException(message2);
        }

        internal static int GetPrefixLength(IPAddress subnetMask, string networkAdapterName)
        {
            subnetMask.Validate("subnetMask");

            DeployerTrace.WriteInfo("Getting prefix length of subnet mask {0} and network adapter {1}", subnetMask, networkAdapterName);

            byte[] bytes = subnetMask.GetAddressBytes();

            int count = 0;

            foreach (var b in bytes)
            {
                // preferring over right-shift till b is 0, since right-shift may pad with 1 in some cases
                byte mask = 1;
                for (byte i = 0; i < sizeof(byte) * 8; i++, mask <<= 1)
                {
                    if ((b & mask) != 0) count++;
                }
            }

            DeployerTrace.WriteInfo("Successfully obtained prefix length of subnet mask {0} and network adapter {1}. Prefix length: {2}", subnetMask, networkAdapterName, count);
            return count;
        }

#if !DotNetCoreClr // Disable compiling on windows for now. Need to correct when porting FabricDeployer.
        /// <summary>
        /// Assigns a new IP address to <see cref="SFNetworkAdapterName"/> since it would have been
        /// assigned a random IP address when it was first created.
        /// </summary>
        /// <param name="ipAddress"></param>
        /// <param name="prefixLength"></param>
        /// <param name="interfaceIndex"></param>
        internal static void AssignNewIPAddress(IPAddress ipAddress, int prefixLength, int interfaceIndex)
        {
            DeployerTrace.WriteInfo("Assigning new IP address {0} InterfaceIndex: {1}", ipAddress, interfaceIndex);

            string script = "New-NetIPAddress -IPAddress {0} -AddressFamily ipv4 -PrefixLength {1} -InterfaceIndex {2}"
                .ToFormat(ipAddress, prefixLength, interfaceIndex);

            var psObjects = Utility.ExecutePowerShellScript(script, false);

            if (psObjects != null)
            {
                DeployerTrace.WriteInfo("PowerShell Invoke completed and returned {0} objects", psObjects.Count);

                foreach (var psObject in psObjects)
                {
                    DeployerTrace.WriteInfo("{0}", psObject);
                }
            }
            else
            {
                DeployerTrace.WriteInfo("PowerShell Invoke completed and returned null");
            }
        }

        internal static void RemoveIPAddress(IPAddress ipAddress, int interfaceIndex)
        {
            DeployerTrace.WriteInfo("Removing IP address {0} InterfaceIndex: {1}", ipAddress, interfaceIndex);

            string script = "Remove-NetIPAddress -IPAddress {0} -InterfaceIndex {1} -Confirm:$false"
                .ToFormat(ipAddress, interfaceIndex);

            var psObjects = Utility.ExecutePowerShellScript(script, false);

            if (psObjects != null)
            {
                DeployerTrace.WriteInfo("PowerShell Invoke completed and returned {0} objects", psObjects.Count);

                foreach (var psObject in psObjects)
                {
                    DeployerTrace.WriteInfo("{0}", psObject);
                }
            }
            else
            {
                DeployerTrace.WriteInfo("PowerShell Invoke completed and returned null");
            }
        }
#endif

        internal static void CreateFirewallRule()
        {
            DeployerTrace.WriteInfo("Creating firewall rule {0} if required...", FirewallRuleName);

#if !DotNetCoreClrLinux
            INetFwPolicy2 fwPolicy2 = GetFirewallPolicy();
            if (fwPolicy2 == null)
            {
                string message = StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorGettingFirewallPolicy1;
                DeployerTrace.WriteWarning(message);
                throw new InvalidOperationException(message);
            }

            bool exists = DoesFirewallRuleExist(fwPolicy2);

            if (exists)
            {
                DeployerTrace.WriteInfo("Firewall rule {0} already exists", FirewallRuleName);
                return;
            }

            DeployerTrace.WriteInfo("Firewall rule {0} doesn't exist. Creating it...", FirewallRuleName);

            NetFwRule rule = new NetFwRuleClass
            {
                Name = FirewallRuleName,
                Grouping = FirewallGroupName,
                Protocol = (int)NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_UDP,
                Direction = NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
                LocalPorts = PortNumber.ToString(),
                Profiles = (int)NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_ALL,
                Description = FirewallRuleDescription,
                LocalAddresses = "*",
                RemoteAddresses = "*",
                Action = NET_FW_ACTION_.NET_FW_ACTION_ALLOW,
                Enabled = true,
            };

            fwPolicy2.Rules.Add(rule);

            string details = "Name: {0}, Grouping: {1}, Protocol: {2}, Direction: {3}, LocalPorts: {4}, Profiles: {5}, LocalAddresses: {6}, RemoteAddresses: {7}, Action: {8}, Enabled: {9}"
                .ToFormat(rule.Name, rule.Grouping, rule.Protocol, rule.Direction, rule.LocalPorts, rule.Profiles,
                    rule.LocalAddresses, rule.RemoteAddresses, rule.Action, rule.Enabled);
#else
            Firewall fw = new Firewall();
            FirewallRule rule = new FirewallRule()
            {
                Name = FirewallRuleName,
                Ports = PortNumber.ToString(),
                Protocol = NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_UDP,
                Direction = NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
                Grouping = FirewallGroupName,
                Profile = NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_ALL
            };
            List<FirewallRule> newRules = new List<FirewallRule>() { rule };
            fw.UpdateRules(newRules, false);

            string details = "Name: {0}, Grouping: {1}, Protocol: {2}, Direction: {3}, Ports: {4}, Profile: {5}"
                .ToFormat(rule.Name, rule.Grouping, rule.Protocol, rule.Direction, rule.Ports, rule.Profile);
#endif


            DeployerTrace.WriteInfo("Firewall rule {0} created.{1}Rule details: {2}", FirewallRuleName, Environment.NewLine, details);
        }

#if !DotNetCoreClrLinux
        /// <summary>
        /// Gets the firewall policy. Using this approach versus reusing <see cref="Firewall"/> class
        /// because the class doesn't have certain required properties like Action, RemoteAddresses etc.
        /// We don't want to go through extensive testing on existing firewall rules if we add properties
        /// as part of this feature.
        /// Tracking bug 8976855. Once that is fixed, this code could be removed.
        /// </summary>
        /// <returns></returns>
        private static INetFwPolicy2 GetFirewallPolicy()
        {
            Type policyType = Type.GetTypeFromProgID(FirewallPolicy2TypeProgID);

            if (policyType == null)
            {
                return null;
            }

            INetFwPolicy2 fwPolicy2 = (INetFwPolicy2)Activator.CreateInstance(policyType);
            return fwPolicy2;
        }

        private static bool DoesFirewallRuleExist(INetFwPolicy2 fwPolicy2)
        {
            if (fwPolicy2.Rules == null)
            {
                return false;
            }

            // Comparing by looping preferred over using Rules.Items(FirewallRuleName) and
            // catching a FileNotFoundException (or other exceptions) if the name doesn't
            // exist. The list of possible exceptions isn't clearly documented.
            foreach (NetFwRule r in fwPolicy2.Rules)
            {
                if (string.Equals(r.Name, FirewallRuleName, StringComparison.OrdinalIgnoreCase) &&
                    string.Equals(r.Grouping, FirewallGroupName, StringComparison.OrdinalIgnoreCase))
                {
                    // we don't do a deep comparison.
                    // In case the firewall rule has been explicitly changed (say to mitigate livesite),
                    // we don't want to overwrite it. Shallow name and group comparison is sufficient.
                    return true;
                }
            }

            return false;
        }
#endif

        internal static void RemoveFirewallRule()
        {
            DeployerTrace.WriteInfo("Removing firewall rule {0} if it exists...", FirewallRuleName);

            try
            {
#if !DotNetCoreClrLinux
                INetFwPolicy2 fwPolicy2 = GetFirewallPolicy();
                if (fwPolicy2 == null)
                {
                    DeployerTrace.WriteWarning(StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorGettingFirewallPolicy2);
                    return;
                }

                bool exists = DoesFirewallRuleExist(fwPolicy2);
                if (!exists)
                {
                    DeployerTrace.WriteInfo("Firewall rule {0} doesn't exist. Nothing to remove", FirewallRuleName);
                    return;
                }

                fwPolicy2.Rules.Remove(FirewallRuleName);
#else
                INetFwRules rules = NetFwRules.GetAllRules();
                rules.Remove(FirewallRuleName);
#endif
                DeployerTrace.WriteInfo("Firewall rule {0} removed", FirewallRuleName);
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteWarning(StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorRemovingFirewallRule, FirewallRuleName, ex);
            }
        }

        internal static IPAddress GetIpRangeStart(IPAddress ipAddress, IPAddress subnetMask)
        {
            var ipBytes = ipAddress.GetAddressBytes();
            var subnetBytes = subnetMask.GetAddressBytes();
            var rangeStartBytes = new byte[ipBytes.Length];

            for (int i = 0; i < ipBytes.Length; i++)
            {
                rangeStartBytes[i] = (byte)(ipBytes[i] & subnetBytes[i]);
            }

            var rangeStartAddress = new IPAddress(rangeStartBytes);

            DeployerTrace.WriteInfo("IPAddress {0} bytes: {1}", ipAddress, ipBytes.GetBytesAsBinaryString());
            DeployerTrace.WriteInfo("Subnet mask {0} bytes: {1}", subnetMask, subnetBytes.GetBytesAsBinaryString());
            DeployerTrace.WriteInfo("IP range start {0} bytes: {1}", rangeStartAddress, rangeStartBytes.GetBytesAsBinaryString());

            return rangeStartAddress;
        }

        internal static void PersistNetworkDetailForContainerStart(IDictionary<string, string> persistData)
        {
#if !DotNetCoreClr
            Utility.AddRegistryKeyValues(Registry.LocalMachine, FabricConstants.FabricRegistryKeyPath, persistData);
#else
            foreach (var kv in persistData)
            {
                string path = Path.Combine(Constants.FabricEtcConfigPath, kv.Key);
                try
                {
                    File.WriteAllText(path, kv.Value);
                }
                catch (Exception ex)
                {
                    string message =
                        "Error persisting DNS configuration setting. Setting path: {0}, setting name: {1}, setting value: {2}, exception: {3}".ToFormat(
                        FabricConstants.FabricRegistryKeyPath, kv.Key, kv.Value, ex);

                    DeployerTrace.WriteError(message);
                    throw new InvalidOperationException(message);
                }
            }
#endif
        }

        internal static void CleanupPersistedNetworkDetail(IList<string> valueNames)
        {
            try
            {
#if !DotNetCoreClr
                Utility.RemoveRegistryKeyValues(Registry.LocalMachine, FabricConstants.FabricRegistryKeyPath, valueNames);
#else
                foreach (var name in valueNames)
                {
                    string path = Path.Combine(Constants.FabricEtcConfigPath, name);
                    File.Delete(path);
                }
#endif
                DeployerTrace.WriteInfo("Persisted network details cleaned up");
            }
            catch (Exception ex)
            {
                // best effort
                DeployerTrace.WriteWarning(StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorCleaningNetworkDetails, ex);
            }
        }

#if !DotNetCoreClr
        /// <summary>
        /// Gets a new IP address to be reassigned to <see cref="SFNetworkAdapterName"/>.
        /// The new IP address is 1 more than Docker's DNS IP address.
        /// </summary>
        internal static IPAddress GetNewIPAddressForSFNic(IPAddress dockerDnsIPAddress)
        {
            var bytes = dockerDnsIPAddress.GetAddressBytes();

            try
            {
                checked
                {
                    bytes[3] += 1;
                }

                var newIP = new IPAddress(bytes);

                var message =
                    "New IP address for network adapter {0} is: {1}".ToFormat(SFNetworkAdapterName, newIP);
                DeployerTrace.WriteInfo(message);

                return newIP;
            }
            catch (OverflowException ex)
            {
                var message = string.Format(
                    CultureInfo.CurrentUICulture,
                    StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorGettingNewIPAddress, SFNetworkAdapterName, dockerDnsIPAddress, ex);
                DeployerTrace.WriteWarning(message);
                throw;
            }
        }
#endif
    }
}
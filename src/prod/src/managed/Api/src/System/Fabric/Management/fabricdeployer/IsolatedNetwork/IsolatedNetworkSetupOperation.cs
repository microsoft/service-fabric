// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using Microsoft.Win32;
using Newtonsoft.Json;
using System.Collections.Generic;
using System.Dynamic;
using System.Fabric.Management.FabricDeployer;
using System.Fabric.Management.ServiceModel;
using System.IO;
using System.Net;
using System.Net.Http;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Text;
using System.Threading;

namespace System.Fabric.FabricDeployer
{
    internal class IsolatedNetworkSetupOperation : IsolatedNetworkDeploymentOperation
    {
        /// <summary>
        /// Setting up an isolated network needs the following steps:
        /// 1) Start the cns plugin.
        /// 2) Check if a secondary NIC is available on Azure or use the provided interface on prem.
        /// 3) Set up the isolated network using the cns plugin.
        /// 4) Record the network interface used to set up the isolated network.
        /// 5) Stop the cns plugin.
        /// </summary>
        /// <param name="parameters"></param>
        /// <param name="clusterManifest"></param>
        /// <param name="infrastructure"></param>
        protected override void OnExecuteOperation(DeploymentParameters parameters, ClusterManifestType clusterManifest, Infrastructure infrastructure)
        {
            ExecuteOperation(parameters, clusterManifest, infrastructure);
        }

        internal void ExecuteOperation(DeploymentParameters parameters, ClusterManifestType clusterManifest, Infrastructure infrastructure)
        {
            if (!parameters.IsolatedNetworkSetup)
            {
                DeployerTrace.WriteInfo("Skipping isolated network set up since it has been disabled.");
                return;
            }

            if (!Utility.IsContainersFeaturePresent())
            {
                DeployerTrace.WriteInfo("Skipping isolated network set up since containers feature has been disabled.");
                return;
            }

#if !DotNetCoreClrLinux
            if (!Utility.IsContainerNTServicePresent())
            {
                DeployerTrace.WriteInfo("Skipping isolated network set up since containers NT service is not installed.");
                return;
            }
#endif
            var networkName = (!string.IsNullOrEmpty(parameters.IsolatedNetworkName)) ? (parameters.IsolatedNetworkName)
                : (IsolatedNetworkConstants.NetworkName);

            DeployerTrace.WriteInfo("Isolated network set up invoked in context: {0}.", parameters.Operation);

            var networkInterfaceName = string.Empty;
            if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzure ||
                clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructurePaaS)
            {
                networkInterfaceName = parameters.IsolatedNetworkInterfaceName;
            }
            else
            {
                if (string.IsNullOrEmpty(parameters.IsolatedNetworkInterfaceName))
                {
                    string message = "Failed to set up isolated network on-prem because isolated network interface name not provided.";
                    DeployerTrace.WriteError(message);
                    throw new InvalidDeploymentException(message);
                }
                else
                {
                    networkInterfaceName = parameters.IsolatedNetworkInterfaceName;
                }
            }

#if !DotNetCoreClr
            bool networkPluginRunning = false;
            if (parameters.Operation == DeploymentOperations.Update || parameters.Operation == DeploymentOperations.UpdateInstanceId)
            {
                networkPluginRunning = IsNetworkPluginRunning();
                DeployerTrace.WriteInfo("Isolated network plugin running: {0}.", networkPluginRunning);
            }

            if (SetupIsolatedNetwork(infrastructure, networkName, parameters.IsolatedNetworkInterfaceName, parameters.FabricBinRoot, networkPluginRunning))
#else
            if (SetupIsolatedNetwork(parameters.IsolatedNetworkInterfaceName))
#endif
            {
                DeployerTrace.WriteInfo("Successfully set up isolated network.");
            }
            else
            {
                string message = "Failed to set up isolated network.";
                DeployerTrace.WriteError(message);
                throw new InvalidDeploymentException(message);
            }
        }

#if !DotNetCoreClr
        private bool SetupIsolatedNetwork(Infrastructure infrastructure, string networkName, string networkInterfaceName, string fabricBinRoot, bool networkPluginRunning)
        {
            bool success = false;

            if (StartNetworkPlugin(fabricBinRoot, networkPluginRunning))
            {
                string hostIp = Utility.GetHostIp(infrastructure);
                if (!string.IsNullOrEmpty(hostIp))
                {
                    if (CreateNetwork(networkName, hostIp, networkInterfaceName))
                    {
                        success = true;
                    }
                }

                if (!success)
                {
                    DeployerTrace.WriteInfo("Rolling back operations performed since isolated network set up failed.");

                    RemoveNetwork(networkName);
                }
            }

            // if network plugin was already running, shutting it down will be skipped since it
            // is being managed by hosting
            bool cleanUpSuccess = false;
            if (networkPluginRunning || Utility.StopProcess(IsolatedNetworkConstants.NetworkPluginProcessName))
            {
                cleanUpSuccess = true;
            }

            success = success && cleanUpSuccess;

            return success;
        }

        private bool EnableForwarding(NetworkInterface networkInterface, IPAddress expectedAddress)
        {
            bool success = false;

            DeployerTrace.WriteInfo("Enable forwarding on network adapter {0}.", networkInterface.Name);

            var isIPAddressMatching = Utility.IsIPAddressMatching(networkInterface, expectedAddress);

            if (!isIPAddressMatching)
            {
                DeployerTrace.WriteInfo("Failed to get ip address for network interface {0} that matches {1}.", networkInterface.Name, expectedAddress.ToString());
                return success;
            }

            int networkInterfaceIndex = -1;
            string networkInterfaceName = string.Empty;
            Utility.GetNetworkInterfaceIndexAndNameWithRetry(expectedAddress, out networkInterfaceIndex, out networkInterfaceName);

            if (networkInterfaceIndex == -1)
            {
                DeployerTrace.WriteInfo("Failed to get index for ip address {0}.", expectedAddress);
                return success;
            }

            string script = string.Format("Set-NetIPInterface -InterfaceIndex {0} -Forwarding Enabled", networkInterfaceIndex);
            var psObjects = Utility.ExecutePowerShellScript(script, false);
            if (psObjects != null)
            {
                DeployerTrace.WriteInfo("PowerShell Invoke completed and returned {0} objects.", psObjects.Count);
                foreach (var psObject in psObjects)
                {
                    DeployerTrace.WriteInfo("{0}", psObject);
                }
            }
            else
            {
                DeployerTrace.WriteInfo("PowerShell Invoke completed and returned null.");
            }

            success = true;
            DeployerTrace.WriteInfo("Enabled forwarding on network adapter {0} successfully.", networkInterface.Name);

            return success;
        }

        private bool CreateNetwork(string networkName, string hostIp, string networkInterfaceName)
        {
            bool success = false;

            DeployerTrace.WriteInfo("Creating network Name:{0} Network Interface:{1}.", networkName, networkInterfaceName);

            try
            {
                string networkId = string.Empty;

                if (GetNetwork(networkName, out networkId) && !string.IsNullOrEmpty(networkId))
                {
                    DeployerTrace.WriteInfo("Skipping network creation since network {0} already exists.", networkName);
                    return true;
                }

                var networkInterface = GetNetworkInterfaceForIsolatedNetwork(networkInterfaceName);
                if (networkInterface == null)
                {
                    DeployerTrace.WriteWarning("Failed to find network interface to set up isolated network.");
                    return success;
                }

                // Capture ip address for comparison later
                IPAddress ipv4Address;
                IPAddress ipv4Mask;
                Utility.GetDefaultIPV4AddressAndMask(networkInterface, out ipv4Address, out ipv4Mask);
                
                // Enable forwarding if interface status is "Up" and not in APIPA address range.
                // This is done so that we do not spend time setting up forwarding on a disconnected, APIPA nic
                // which is not used for communication.
                var apipaNic = Utility.GetNetworkInterfaceOnSubnet(Constants.ApipaSubnet);
                var operationalStatus = networkInterface.OperationalStatus;
                bool enableForwarding = (apipaNic != null && string.Equals(apipaNic.Name, networkInterfaceName, StringComparison.OrdinalIgnoreCase) && operationalStatus != OperationalStatus.Up) 
                ? (false) : (true);
                DeployerTrace.WriteInfo("Forwarding should be enabled:{0}", enableForwarding);

                var isolatedNetworkConfig = new IsolatedNetworkConfig();
                isolatedNetworkConfig.Name = networkName;
                isolatedNetworkConfig.Type = IsolatedNetworkConstants.NetworkType;
                isolatedNetworkConfig.ManagementIP = hostIp;
                isolatedNetworkConfig.NetworkAdapterName = networkInterface.Name;

                var isolatedNetworkPolicyConfig = new IsolateNetworkPolicyConfig();
                isolatedNetworkPolicyConfig.VSID = IsolatedNetworkConstants.DefaultVxlanId;
                isolatedNetworkPolicyConfig.Type = IsolatedNetworkConstants.NetworkPolicyType;

                var isolatedNetworkSubnetConfig = new IsolatedNetworkSubnetConfig();
                isolatedNetworkSubnetConfig.AddressPrefix = IsolatedNetworkConstants.DefaultAddressPrefix;
                isolatedNetworkSubnetConfig.GatewayAddress = IsolatedNetworkConstants.DefaultGatewayAddress;
                isolatedNetworkSubnetConfig.Policies = new IsolateNetworkPolicyConfig[] { isolatedNetworkPolicyConfig };

                isolatedNetworkConfig.Subnets = new IsolatedNetworkSubnetConfig[] { isolatedNetworkSubnetConfig };

                var isolatedNetworkConfigJsonString = JsonConvert.SerializeObject(isolatedNetworkConfig);
                var content = new StringContent(isolatedNetworkConfigJsonString, Encoding.UTF8, "application/json");

                HttpClient httpClient = new HttpClient();
                var task = httpClient.PostAsync(IsolatedNetworkConstants.NetworkCreateUrl, content);
                var httpResponse = task.GetAwaiter().GetResult().EnsureSuccessStatusCode();
                var response = httpResponse.Content.ReadAsStringAsync().GetAwaiter().GetResult();

                // check status code on response to know if operation was really successful
                dynamic createNetworkResponse = JsonConvert.DeserializeObject<ExpandoObject>(response);
                if (createNetworkResponse.ReturnCode != 0)
                {
                    DeployerTrace.WriteInfo("Create network plugin call failed with exception {0}.", createNetworkResponse.Message);
                    return success;
                }

                var persistData = new Dictionary<string, string>();
                persistData.Add(IsolatedNetworkConstants.IsolatedNetworkInterfaceNameRegValue, networkInterface.Name);
                Utility.AddRegistryKeyValues(Registry.LocalMachine, FabricConstants.FabricRegistryKeyPath, persistData);

                // skip enabling fowarding on the network interface if we determined before that it is not needed.
                if (!enableForwarding)
                {
                    success = true;
                    DeployerTrace.WriteInfo("Skip enabling forwarding on Network Interface:{0}.", networkInterface.Name);
                    return success;
                }
                
                if (EnableForwarding(networkInterface, ipv4Address))
                {
                    success = true;
                    DeployerTrace.WriteInfo("Network Name:{0} Network Interface:{1} created successfully.", networkName, networkInterface.Name);
                }
                else
                {
                    DeployerTrace.WriteInfo("Failed to enable forwarding on Network Interface:{0}.", networkInterface.Name);
                }
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to create network Name:{0} Network Interface:{1} exception {2}.",
                                         networkName,
                                         networkInterfaceName,
                                         ex);
            }

            return success;
        }
#else
        private bool SetupIsolatedNetwork(string networkInterfaceName)
        {
            bool success = false;

            var networkInterface = GetNetworkInterfaceForIsolatedNetwork(networkInterfaceName);
            if (networkInterface != null)
            {
                string path = Path.Combine(Constants.FabricEtcConfigPath, IsolatedNetworkConstants.IsolatedNetworkInterfaceNameRegValue);
                try
                {
                    File.WriteAllText(path, networkInterface.Name);
                    success = true;
                }
                catch (Exception ex)
                {
                    DeployerTrace.WriteError("Failed to write isolated network interace name to path {0} exception {1}.", path, ex);
                }
            }
        
            return success;
        }
#endif

        /// <summary>
        /// Gets the network interface matching the interface name, if provided.
        /// If network interface name is not provided then - 
        /// a) On Windows - the second NIC is retrieved from NM agent. The adapter is refreshed so that the ip plumbs correctly.
        /// b) On Linux - the primary NIC is retrieved from NM agent.
        /// </summary>
        /// <param name="networkInterfaceName"></param>
        /// <returns></returns>
        private NetworkInterface GetNetworkInterfaceForIsolatedNetwork(string networkInterfaceName)
        {
            NetworkInterface networkInterface = null;

            if (!string.IsNullOrEmpty(networkInterfaceName))
            {
                DeployerTrace.WriteInfo("Get network interface for isolated network set up: {0}.", networkInterfaceName);

                foreach (NetworkInterface nic in NetworkInterface.GetAllNetworkInterfaces())
                {
                    if (string.Equals(nic.Name, networkInterfaceName, StringComparison.OrdinalIgnoreCase))
                    {
                        networkInterface = nic;
                    }
                }

                DeployerTrace.WriteInfo("Network interface found for {0}:{1}.", networkInterfaceName, (networkInterface != null) ? (true) : (false));
            }
            else
            {
                DeployerTrace.WriteInfo("Get network interface for isolated network set up from nm agent.");

#if !DotNetCoreClr
                string subnet = string.Empty;
                string gateway = string.Empty;
                string macAddress = string.Empty;
                if (Utility.RetrieveNMAgentInterfaceInfo(false, out subnet, out gateway, out macAddress))
                {
                    networkInterface = Utility.GetNetworkInterfaceByMacAddress(macAddress);

                    string script = string.Format("Disable-NetAdapter -Name \"{0}\" -Confirm:$false", networkInterface.Name);
                    var psObjects = Utility.ExecutePowerShellScript(script, false);
                    script = string.Format("Enable-NetAdapter -Name \"{0}\" -Confirm:$false", networkInterface.Name);
                    psObjects = Utility.ExecutePowerShellScript(script, false);

                    // Wait for ip to be configured, after enabling adapter.
                    uint subnetIp;
                    uint subnetMask;
                    if (Utility.ParseSubnetIpAndMask(subnet, out subnetIp, out subnetMask))
                    {
                        var subnetIpAddress = Utility.ConvertToIpAddress(subnetIp, false);
                        networkInterface = Utility.GetNetworkInterfaceOnSubnetWithRetry(subnetIpAddress);
                    }
                }
#else
                string subnet = string.Empty;
                string gateway = string.Empty;
                string macAddress = string.Empty;
                if (Utility.RetrieveNMAgentInterfaceInfo(true, out subnet, out gateway, out macAddress))
                {
                    networkInterface = Utility.GetNetworkInterfaceByMacAddress(macAddress);
                }
#endif

                DeployerTrace.WriteInfo("Network interface from nm agent found:{0}.", (networkInterface != null) ? (true) : (false));
            }

            return networkInterface;
        }
    }
}
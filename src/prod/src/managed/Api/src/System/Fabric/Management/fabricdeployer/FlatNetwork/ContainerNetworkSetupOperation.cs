// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Collections.Generic;
    using System.Dynamic;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.Common;
    using System.Fabric.Management.FabricDeployer.FlatNetwork.CNSApi;
    using System.Linq;
    using System.Threading;
    using System.IO;
    using System.Net;
    using System.Net.NetworkInformation;
    using System.Net.Sockets;
    using Net.Http;
    using NetFwTypeLib;
    using Newtonsoft.Json;
    using Text;

    internal class ContainerNetworkSetupOperation : ContainerNetworkDeploymentOperation
    {
        /// <summary>
        /// Setting up a docker network needs the following steps:
        /// 1) Stopping the Docker NT service
        /// 2) Stopping the dockerd process
        /// 3) Starting the dockerd process on localhost:2375
        /// 4) Saving routing table information
        /// 5) Getting subnet and gateway information from nm agent
        /// 6) Creating docker network
        /// 7) Restoring routing table information
        /// 8) Applies firewall rule to allow multi-ip containers to communicate
        /// 9) Stopping the dockerd process
        /// 10) Starting the Docker NT service if stopped in step 1)
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
#if !DotNetCoreClrLinux

            if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzure ||
                clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructurePaaS)
            {
                if (parameters.ContainerNetworkSetup)
                {
                    if (Utility.IsContainersFeaturePresent())
                    {
                        if (IsContainerServiceInstalled())
                        {
                            if (IsWireServerAvailable())
                            {
                                var networkName = (!string.IsNullOrEmpty(parameters.ContainerNetworkName)) ? (parameters.ContainerNetworkName) : (FlatNetworkConstants.NetworkName);

                                DeployerTrace.WriteInfo("Container network set up invoked in context: {0}", parameters.Operation);

                                bool containerServiceRunningOnCustomPort = false;
                                if (parameters.Operation == DeploymentOperations.Update || parameters.Operation == DeploymentOperations.UpdateInstanceId)
                                {
                                    containerServiceRunningOnCustomPort = IsContainerServiceRunning();
                                    DeployerTrace.WriteInfo("Container service running on custom port: {0}", containerServiceRunningOnCustomPort);
                                }

                                if (SetupContainerNetwork(networkName, containerServiceRunningOnCustomPort))
                                {
                                    DeployerTrace.WriteInfo("Successfully set up container network.");
                                }
                                else
                                {
                                    DeployerTrace.WriteError("Failed to set up container network.");
                                    throw new InvalidDeploymentException("Failed to set up container network.");
                                }
                            }
                            else
                            {
                                DeployerTrace.WriteInfo("Skipping container network set up since wire server is not available.");
                            }
                        }
                        else
                        {
                            DeployerTrace.WriteInfo("Skipping container network set up since containers service is not installed.");
                        }
                    }
                    else
                    {
                        DeployerTrace.WriteInfo("Skipping container network set up since containers feature has been disabled.");
                    }
                }
                else
                {
                    DeployerTrace.WriteInfo("Skipping container network set up since it has been disabled.");
                }
            }
            else
            {
                DeployerTrace.WriteInfo("Skipping container network set up since we are not on Azure.");
            }
#else
            if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructurePaaS)
            {
                if (parameters.ContainerNetworkSetup)
                {
                    var networkName = (!string.IsNullOrEmpty(parameters.ContainerNetworkName)) ? (parameters.ContainerNetworkName) : (FlatNetworkConstants.NetworkName);

                    DeployerTrace.WriteInfo("Container network set up invoked in context: {0}", parameters.Operation);

                    bool containerServiceRunningOnCustomPort = false;
                    if (parameters.Operation == DeploymentOperations.Update || parameters.Operation == DeploymentOperations.UpdateInstanceId)
                    {
                        containerServiceRunningOnCustomPort = IsContainerServiceRunning();
                        DeployerTrace.WriteInfo("Container service running on custom port: {0}", containerServiceRunningOnCustomPort);
                    }

                    if (SetupContainerNetwork(networkName, parameters.FabricBinRoot, containerServiceRunningOnCustomPort))
                    {
                        DeployerTrace.WriteInfo("Successfully set up container network.");
                    }
                    else
                    {
                        DeployerTrace.WriteError("Failed to set up container network.");
                        throw new InvalidDeploymentException("Failed to set up container network.");
                    }
                }
                else
                {
                    DeployerTrace.WriteInfo("Skipping container network set up since it has been disabled.");
                }
            }
            else
            {
                DeployerTrace.WriteInfo("Skipping container network set up since we are not on Azure.");
            }
#endif
        }

#if DotNetCoreClrLinux
        private bool SetupContainerNetwork(string networkName, string fabricBinRoot, bool containerServiceRunningOnCustomPort)
        {
            bool success = false;

            bool containerServiceStopped = false;
            if (InstallNetworkDriver(fabricBinRoot))
            {
                if (StopContainerService(containerServiceRunningOnCustomPort, out containerServiceStopped))
                {
                    if (StartContainerProcess(containerServiceRunningOnCustomPort))
                    {
                        if (CreateNetwork(networkName))
                        {
                            if (CreateIptableRule())
                            {
                                success = true;
                            }
                        }

                        // if we failed to complete the set up process, remove the network, uninstall the network driver and remove iptable.
                        if (!success)
                        {
                            DeployerTrace.WriteInfo("Rolling back operations performed since set up container network failed.");

                            RemoveNetwork(networkName);
                            UninstallNetworkDriver();
                            RemoveIptableRule();
                        }
                    }
                }
            }
            else
            {
                DeployerTrace.WriteInfo("Rolling back operations performed since set up container network failed.");

                UninstallNetworkDriver();
            }

            bool cleanUpSuccess = false;
            if (StopContainerProcess(containerServiceRunningOnCustomPort) && StartContainerService(containerServiceRunningOnCustomPort, containerServiceStopped))
            {
                cleanUpSuccess = true;
            }

            success = success && cleanUpSuccess;

            return success;
        }

        /// <summary>
        /// Installs the azure network driver. This is the driver used to create the 
        /// flat network to which containers with static IPs are added.
        /// </summary>
        private bool InstallNetworkDriver(string fabricBinRoot)
        {
            bool success = false;

            DeployerTrace.WriteInfo("Installing the network driver.");

            try
            {
                var pluginFullPath = Path.Combine(Path.Combine(fabricBinRoot, @"Fabric/Fabric.Code"), FlatNetworkConstants.NetworkDriverPlugin);
                DeployerTrace.WriteInfo("Network driver plugin path is {0}.", pluginFullPath);

                bool running;
                int processId = 0;
                IsProcessRunning(FlatNetworkConstants.NetworkDriverPlugin, out running, out processId);
                if (!running)
                {
                    // Remove the azure vnet sock file if it exists.
                    var command = string.Format("sudo rm -f {0}", FlatNetworkConstants.AzureVnetPluginSockPath);
                    int returnvalue = NativeHelper.system(command);
                    if (returnvalue != 0)
                    {
                        DeployerTrace.WriteInfo("Failed to execute command {0} return value {1}", command, returnvalue);
                    }
                    else
                    {
                        DeployerTrace.WriteInfo("Successfully executed command \"{0}\".", command);

                        // check ebtables dependency
                        bool ebtablesInstalled = false;
                        if (IsPackageInstalled(FlatNetworkConstants.EbtablesPackageName))
                        {
                            DeployerTrace.WriteInfo("Ebtables package installed.");
                            ebtablesInstalled = true;
                        }
                        else
                        {
                            DeployerTrace.WriteInfo("Ebtables package not installed.");
                        }

                        // start up cns plugin
                        if (ebtablesInstalled)
                        {
                            command = string.Format("{0} {1}&", "sudo", pluginFullPath);
                            returnvalue = NativeHelper.system(command);
                            if (returnvalue != 0)
                            {
                                DeployerTrace.WriteInfo("Failed to execute command {0} with return value {1}.", command, returnvalue);
                            }
                            else
                            {
                                DeployerTrace.WriteInfo("Successfully executed command \"{0}\".", command);
                                // Check if network driver is installed
                                for (int i=0; i<3; i++)
                                {
                                   bool processRunning = false;
                                   int pid = -1;
                                   IsProcessRunning(FlatNetworkConstants.NetworkDriverPlugin, out processRunning, out pid);  
                                   if (processRunning)
                                   {
                                      DeployerTrace.WriteInfo("Successfully installed network driver.");
                                      success = true;
                                      break;
                                   }
                                   else
                                   {
                                      Thread.Sleep(5000);
                                   }
                                }
                            }
                        }
                    }
                }
                else
                {
                    DeployerTrace.WriteInfo("Skipping installation of network driver since it is already installed.");
                    success = true;
                }
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to install network driver exception {0}", ex);
            }

            return success;
        }

        /// <summary>
        /// Creates docker network using the cns plugin.
        /// </summary>
        /// <param name="networkName"></param>
        /// <returns></returns>
        private bool CreateNetwork(string networkName)
        {
            bool success = false;

            DeployerTrace.WriteInfo("Creating network Name:{0} using cns plugin.", networkName);

            try
            {
                string dockerNetworkId = string.Empty;
                string dockerNetworkName = string.Empty;
                if (GetNetwork(networkName, out dockerNetworkName, out dockerNetworkId))
                {
                    if (string.IsNullOrEmpty(dockerNetworkId))
                    {
                        // Initialize network configuration
                        var initReq = new InitializeRequest
                        {
                            Location = FlatNetworkConstants.NetworkEnvAzure,
                            NetworkType = FlatNetworkConstants.UnderlayNetworkType
                        };

                        var initRes = httpClient.PostAsync(
                                        FlatNetworkConstants.NetworkEnvInitializationUrl, 
                                        new StringContent(JsonConvert.SerializeObject(initReq), Encoding.UTF8, "application/json")
                                      ).GetAwaiter().GetResult();

                        string response = initRes.Content.ReadAsStringAsync().GetAwaiter().GetResult();

                        if (!initRes.IsSuccessStatusCode)
                        {
                            DeployerTrace.WriteError("Failed to initialize network environment Name:{0} error details {1}.", networkName, response);
                        }
                        else
                        {
                            var initResult = JsonConvert.DeserializeObject<CnsResponse>(response);
                            if(initResult.ReturnCode != 0)
                            {
                                DeployerTrace.WriteWarning(
                                    "Network Name:{0} environment initialize returned error code {1}, message {2}",
                                    networkName, 
                                    initResult.ReturnCode, 
                                    initResult.Message);
                                return success;
                            }

                            // Create network
                            var createNetworkReq = new CreateNetworkRequest
                            {
                                NetworkName = networkName
                            };

                            var createNetworkRes = httpClient.PostAsync(
                                                    FlatNetworkConstants.NetworkCreateUrl, 
                                                    new StringContent(JsonConvert.SerializeObject(createNetworkReq), Encoding.UTF8, "application/json")
                                                   ).GetAwaiter().GetResult();
                         
                            response = createNetworkRes.Content.ReadAsStringAsync().GetAwaiter().GetResult();

                            if (!createNetworkRes.IsSuccessStatusCode)
                            {
                                DeployerTrace.WriteError("Failed to create network Name:{0} error details {1}.", networkName, response);
                            }
                            else
                            {
                                var createNetworkResult = JsonConvert.DeserializeObject<CnsResponse>(response);
                                if(createNetworkResult.ReturnCode != 0)
                                {
                                    DeployerTrace.WriteWarning(
                                        "Network Name:{0} network create returned error code {1}, message {2}", 
                                        networkName, 
                                        createNetworkResult.ReturnCode, 
                                        createNetworkResult.Message);
                                    return success;
                                }

                                success = true;
                                DeployerTrace.WriteInfo("Network Name:{0} created successfully.", networkName);
                            }
                        }
                    }
                    else
                    {
                        success = true;
                        DeployerTrace.WriteInfo("Skipping network creation since network {0} already exists.", networkName);
                    }
                }
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to create network Name:{0} exception {1}.", networkName, ex);
            }

            return success;
        }

        /// <summary>
        /// Creates iptable rule to allow multi-ip containers
        /// to communicate with each other.
        /// </summary>
        /// <returns></returns>
        private bool CreateIptableRule()
        {
            string subnet = string.Empty;
            string gateway = string.Empty;
            if (Utility.RetrieveGatewayAndSubnet(true, out subnet, out gateway))
            {
                return (CreateIptableRule(subnet, ConnectivityDirection.Inbound) && CreateIptableRule(subnet, ConnectivityDirection.Outbound));
            }

            return false;
        }

        /// <summary>
        /// Creates inbound or outbound forward iptable rule to allow multi-ip containers
        /// to communicate with each other.
        /// </summary>
        /// <returns></returns>
        private bool CreateIptableRule(string subnet, ConnectivityDirection connectivityDir)
        {
           bool success = false;

           DeployerTrace.WriteInfo("Creating iptable rule for {0} connectivity.", connectivityDir.ToString());

           try
           {
              if (!DoesIptableRuleExist(subnet, connectivityDir))
              {
                  string command = string.Empty;
                  if (connectivityDir == ConnectivityDirection.Inbound)
                  {
                      command = string.Format("sudo iptables -w 30 -I FORWARD 1 -d {0} -j ACCEPT", subnet); 
                  }
                  else if (connectivityDir == ConnectivityDirection.Outbound)
                  {
                      command = string.Format("sudo iptables -w 30 -I FORWARD 1 -s {0} -j ACCEPT", subnet);
                  }

                  int returnvalue = NativeHelper.system(command);
                  if (returnvalue == 0)
                  {
                      DeployerTrace.WriteInfo("Successfully executed command \"{0}\".", command);
                      success = true;  
                  }
                  else
                  {
                     DeployerTrace.WriteInfo("Failed to create forward iptable rule for {0} connectivity.", connectivityDir);
                  }
              }
              else
              {
                  DeployerTrace.WriteInfo("Skipping creation of forward iptable rule for {0} connectivity since it already exists.", connectivityDir);
                  success = true;
              }
           }
           catch (Exception ex)
           {
               DeployerTrace.WriteError("Failed to create forward iptable rule for {0} connectivity exception {1}", connectivityDir, ex);
           }

           return success;
        }
#else
        private bool SetupContainerNetwork(string networkName, bool containerServiceRunningOnCustomPort)
        {
            bool success = false;

            bool containerNTServiceStopped;
            if (StopContainerNTService(containerServiceRunningOnCustomPort, out containerNTServiceStopped))
            {
                if (StartContainerProviderService(containerServiceRunningOnCustomPort))
                {
                    var oldIpForwardTable = default(NativeMethods.IPFORWARDTABLE);
                    if (RetrieveIPv4RoutingTable(out oldIpForwardTable))
                    {
                        string subnet = string.Empty;
                        if (CreateNetwork(networkName, out subnet))
                        {
                            var newIpForwardTable = default(NativeMethods.IPFORWARDTABLE);
                            if (RetrieveIPv4RoutingTable(out newIpForwardTable))
                            {
                                if (RestoreIPv4RoutingTable(oldIpForwardTable, newIpForwardTable, subnet))
                                {
                                    if (CreateFirewallRule())
                                    {
                                        success = true;
                                    }
                                }
                            }
                        }
                    }

                    // if we failed to complete the set up process, remove the network and firewall rule.
                    if (!success)
                    {
                        DeployerTrace.WriteInfo("Rolling back operations performed since set up container network failed.");

                        RemoveNetwork(networkName);
                        RemoveFirewallRule();
                    }
                }
            }

            bool cleanUpSuccess = false;
            if (StopContainerProviderService(containerServiceRunningOnCustomPort) && StartContainerNTService(containerServiceRunningOnCustomPort, containerNTServiceStopped))
            {
                cleanUpSuccess = true;
            }

            success = success && cleanUpSuccess;

            return success;
        }

        /// <summary>
        /// Creates firewall rule to allow multi-ip containers
        /// to communicate with each other.
        /// </summary>
        /// <returns></returns>
        private bool CreateFirewallRule()
        {
            DeployerTrace.WriteInfo("Creating firewall rule {0}.", FlatNetworkConstants.FirewallRuleName);

            INetFwPolicy2 fwPolicy2 = GetFirewallPolicy();
            if (fwPolicy2 == null)
            {
                DeployerTrace.WriteError("Unable to get firewall policy.");
                return false;
            }

            bool exists = DoesFirewallRuleExist(fwPolicy2);
            if (exists)
            {
                DeployerTrace.WriteInfo("Firewall rule {0} already exists.", FlatNetworkConstants.FirewallRuleName);
                return true;
            }

            NetFwRule rule = new NetFwRuleClass
            {
                Name = FlatNetworkConstants.FirewallRuleName,
                Grouping = FlatNetworkConstants.FirewallGroupName,
                Protocol = (int)NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_UDP,
                Direction = NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
                LocalPorts = FlatNetworkConstants.PortNumber.ToString(),
                Profiles = (int)NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_ALL,
                Description = FlatNetworkConstants.FirewallRuleDescription,
                Action = NET_FW_ACTION_.NET_FW_ACTION_ALLOW,
                Enabled = true,
            };

            fwPolicy2.Rules.Add(rule);

            string details = string.Format("Name: {0}, Grouping: {1}, Protocol: {2}, Direction: {3}, LocalPorts: {4}, Profiles: {5}, LocalAddresses: {6}, RemoteAddresses: {7}, Action: {8}, Enabled: {9}",
                                           rule.Name, 
                                           rule.Grouping, 
                                           rule.Protocol, 
                                           rule.Direction, 
                                           rule.LocalPorts,
                                           rule.Profiles,
                                           rule.LocalAddresses, 
                                           rule.RemoteAddresses, 
                                           rule.Action, 
                                           rule.Enabled);

            DeployerTrace.WriteInfo("Firewall rule {0} created.\nRule details: {1}.", FlatNetworkConstants.FirewallRuleName, details);
            return true;
        }

        /// <summary>
        /// Retrieves subnet and gateway information
        /// and then invokes the create network command.
        /// </summary>
        /// <returns></returns>
        private bool CreateNetwork(string networkName, out string subnet)
        {
            subnet = string.Empty;
            string gateway = string.Empty;
            if (Utility.RetrieveGatewayAndSubnet(true, out subnet, out gateway))
            {
                uint subnetIp;
                uint subnetMask;
                if (Utility.ParseSubnetIpAndMask(subnet, out subnetIp, out subnetMask))
                {
                    var subnetIpAddress = Utility.ConvertToIpAddress(subnetIp, false);
                    var nic = Utility.GetNetworkInterfaceOnSubnet(subnetIpAddress);
                    if (nic != null)
                    {
                        return CreateNetwork(networkName, subnet, gateway, nic);
                    }
                }
            }

            return false;
        }

        /// <summary>
        /// Creates docker network using network details provided.
        /// </summary>
        /// <param name="networkName"></param>
        /// <param name="subnet"></param>
        /// <param name="gateway"></param>
        /// <param name="networkInterface"></param>
        /// <returns></returns>
        private bool CreateNetwork(string networkName, string subnet, string gateway, NetworkInterface networkInterface)
        {
            bool success = false;

            DeployerTrace.WriteInfo("Creating network Name:{0} Subnet:{1} Gateway:{2} Network Interface:{3}", networkName, subnet, gateway, networkInterface.Name);

            try
            {
                string networkId = string.Empty;

                string dockerNetworkId = string.Empty;
                string dockerNetworkName = string.Empty;
                if (GetNetwork(networkName, out dockerNetworkName, out dockerNetworkId))
                {
                    if (string.IsNullOrEmpty(dockerNetworkId))
                    {
                        var networkConfig = new NetworkConfig();
                        networkConfig.Name = networkName;
                        networkConfig.Driver = FlatNetworkConstants.NetworkDriver;
                        networkConfig.CheckDuplicate = true;
                        networkConfig.Ipam = new IpamConfig();
                        networkConfig.Ipam.Config = new Config[] { new Config() { Subnet = subnet, Gateway = gateway } };

                        // Add parameter that specifies the network interface to create the flat network on.
                        networkConfig.Options = new ExpandoObject();
                        ((IDictionary<String, Object>)networkConfig.Options).Add(FlatNetworkConstants.NetworkInterfaceParameterName, networkInterface.Name);

                        var networkConfigJsonString = JsonConvert.SerializeObject(networkConfig);
                        var content = new StringContent(networkConfigJsonString, Encoding.UTF8, "application/json");
                        var networkCreateUrl = this.dockerClient.GetRequestUri(FlatNetworkConstants.NetworkCreateRequestPath);

                        var task = this.dockerClient.HttpClient.PostAsync(networkCreateUrl, content);
                        var httpResponseMessage = task.GetAwaiter().GetResult();
                        string response = httpResponseMessage.Content.ReadAsStringAsync().GetAwaiter().GetResult();
                        if (!httpResponseMessage.IsSuccessStatusCode)
                        {
                            DeployerTrace.WriteError("Failed to get network details for network Name:{0} error details {1}.", networkName, response);
                        }
                        else
                        {
                            IPAddress ipv4Address;
                            IPAddress ipv4Mask;
                            Utility.GetDefaultIPV4AddressAndMask(networkInterface, out ipv4Address, out ipv4Mask);
                            if (ipv4Address == null)
                            {
                                DeployerTrace.WriteWarning("Failed to get default ip address for network interface {0}.", networkInterface.Name);
                                return success;
                            }

                            var isIPAddressMatching = Utility.IsIPAddressMatching(networkInterface, ipv4Address);
                            if (!isIPAddressMatching)
                            {
                                DeployerTrace.WriteWarning("Failed to get ip address for network interface {0} that matches {1}.", networkInterface.Name, ipv4Address.ToString());
                                return success;
                            }

                            success = true;
                            DeployerTrace.WriteInfo("Network Name:{0} Subnet:{1} Gateway:{2} created successfully.",
                                                    networkName,
                                                    subnet,
                                                    gateway);
                        }
                    }
                    else
                    {
                        if (string.Equals(dockerNetworkName, networkName, StringComparison.OrdinalIgnoreCase))
                        {
                            success = true;
                            DeployerTrace.WriteInfo("Skipping network creation since network {0} already exists.", networkName);
                        }
                        else
                        {
                            DeployerTrace.WriteError("Removing network since it is not created with name {0}.", networkName);
                            RemoveNetwork(dockerNetworkName);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to create network Name:{0} Subnet:{1} Gateway:{2} exception {3}.",
                                         networkName,
                                         subnet,
                                         gateway,
                                         ex);
            }

            return success;
        }

        /// <summary>
        /// Checks if docker container service is installed.
        /// </summary>
        /// <returns></returns>
        private bool IsContainerServiceInstalled()
        {
            bool installed = false;

            var programFilesPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), FlatNetworkConstants.ContainerServiceName);
            var containerServiceFullPath = Path.Combine(programFilesPath, FlatNetworkConstants.ContainerProviderProcessNameWithExtension);
            if (File.Exists(containerServiceFullPath))
            {
                installed = true;
            }
            
            DeployerTrace.WriteInfo("Container service installed {0}.", installed);

            return installed;
        }
#endif
    }
}
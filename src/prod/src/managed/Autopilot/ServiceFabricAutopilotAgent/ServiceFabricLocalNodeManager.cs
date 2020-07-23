// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Autopilot
{
    using System;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Net.Sockets;
    using System.ServiceProcess;
    using System.Text.RegularExpressions;
    using System.Xml;
    using System.Xml.Serialization;

    using Microsoft.Fabric.InfrastructureWrapper;
    using Microsoft.Search.Autopilot;

    internal class ServiceFabricLocalNodeManager : InfrastructureWrapper
    {
        private readonly string agentApplicationDirectory;
        private readonly bool useTestMode;

        public string DataRootDirectory { get; private set; }

        public string LocalMachineSku { get; private set; }

        public string LocalMachineIpAddress { get; private set; }

        private ServiceFabricLocalNodeManager(string agentApplicationDirectory, bool useTestMode)
        {
            this.agentApplicationDirectory = agentApplicationDirectory;
            this.useTestMode = useTestMode;
        }  

        public static bool Create(string agentApplicationDirectory, bool useTestMode, out ServiceFabricLocalNodeManager serviceFabricLocalNodeManager)
        {
            serviceFabricLocalNodeManager = null;

            ServiceFabricLocalNodeManager localNodeManager = new ServiceFabricLocalNodeManager(agentApplicationDirectory, useTestMode);            
            if (!localNodeManager.Initialize())
            {
                return false;
            }

            serviceFabricLocalNodeManager = localNodeManager;

            return true;
        }

        public bool ManageNetworkConfiguration()
        {
            // Note that portproxy creation operations are idempotent.
            if (ConfigurationManager.PortFowardingRules != null)
            {
                foreach (PortFowardingRule portForwardingRule in ConfigurationManager.PortFowardingRules)
                {
                    if (!this.ApplyPortForwardingRule(portForwardingRule))
                    {
                        return false;
                    }
                }
            }

            // Currently Fabric Deployer only supports topology info update for nodes defined in static topology.
            // In the future, more options would be added at deployer as well as agent end.
            if (ConfigurationManager.DynamicTopologyUpdateMode != DynamicTopologyUpdateMode.None)
            {
                Utilities.SetDynamicTopologyKind((int)DynamicTopologyUpdateMode.OnNodeConfigurationOnly);
            }

            return this.SetDynamicPortRange(ConfigurationManager.DynamicPortRangeStart, ConfigurationManager.DynamicPortRangeEnd);            
        }

        public bool InstallProduct()
        {
            TextLogger.LogInfo("Installing Service Fabric product.");

            try
            {
                string installedServiceFabricProductVersion = Utilities.GetCurrentWindowsFabricVersion();          

                if (installedServiceFabricProductVersion == null)
                {
                    /*
                     * For Service Fabric on Autopilot, xcopyable deployment is the only supported mechanism for installing Service Fabric product.
                     * MSI is not supported.
                     */
                    string serviceFabricCabFile = this.GetServiceFabricCabFile();

                    if (File.Exists(serviceFabricCabFile))
                    {
                        TextLogger.LogInfo("Installing Service Fabric product with cab file {0}.", serviceFabricCabFile);

                        string localInstallationPath = Environment.ExpandEnvironmentVariables(Constants.FabricRootPath);
                        if (!Directory.Exists(localInstallationPath))
                        {
                            Directory.CreateDirectory(localInstallationPath);
                        }

                        Utilities.UnCabFile(serviceFabricCabFile, localInstallationPath);

                        Utilities.SetFabricRoot(localInstallationPath);

                        Utilities.SetFabricCodePath(Path.Combine(localInstallationPath, Constants.PathToFabricCode));

                        // TODO: registry key not available yet              
                        installedServiceFabricProductVersion = Utilities.GetCurrentWindowsFabricVersion();

                        TextLogger.LogInfo("Successfully installed Service Fabric product version {0} to local path {1} and set Service Fabric registry keys.", installedServiceFabricProductVersion, localInstallationPath);                        
                    }
                    else
                    {
                        // TODO: [General] surface fatal errors outside the agent as AP health report.
                        TextLogger.LogError("Failed to install Service Fabric product : Service Fabric product cab file does not exist in agent application directory as {0}.", serviceFabricCabFile);

                        return false;
                    }
                }          
                else
                {
                    TextLogger.LogInfo("Service Fabric product version {0} is already installed locally. Skip product installation.", installedServiceFabricProductVersion);
                }

                return true;
            }
            catch (Exception e)
            {
                TextLogger.LogError("Failed to install Service Fabric product : {0}", e);             

                return false;
            }            
        }

        public bool DiscoverTopology(out ClusterTopology expectedTopology)
        {
            expectedTopology = null;

            try
            {
                TextLogger.LogInfo("Discovering cluster topology from bootstrap cluster manifest file.");

                if (ConfigurationManager.BootstrapClusterManifestLocation == BootstrapClusterManifestLocationType.NotReady)
                {
                    TextLogger.LogWarning("Bootstrap cluster manifest is not ready. Cluster topology would not be discovered until bootstrap cluster manifest is ready.");

                    return false;
                }

                ClusterManifestType bootstrapClusterManifest;
                if (!this.ReadBootstrapClusterManifestFile(out bootstrapClusterManifest))
                {
                    return false;
                }

                TopologyProviderType topologyProviderType = TopologyProvider.GetTopologyProviderTypeFromClusterManifest(bootstrapClusterManifest);

                TextLogger.LogInfo("Topology provider type : {0}.", topologyProviderType);

                if (topologyProviderType != TopologyProviderType.StaticTopologyProvider)
                {
                    TextLogger.LogInfo("Topology provider type {0} is not supported to provide static topology.", topologyProviderType);

                    return false;
                }
                else
                {
                    /*
                     * Static topology provider defines the authoritative cluster topology in the cluster manifest.                   
                     * If the local machine (physical or virtual) is not part of the cluster topology specified in static topology provider section, a Service Fabric node will not be started locally.
                     * During scale-up, bootstrap cluster manifest has to be upgraded to allow new Service Fabric nodes to be started on the new machines and join the ring. 
                     * A scale-up is normally a two-phase process with allocation of new machines (and their IPs) and generation of a new bootstrap cluster manifest with updated topology.
                    */
                    ClusterManifestTypeInfrastructureWindowsAzureStaticTopology staticTopologyProviderElement = TopologyProvider.GetStaticTopologyProviderElementFromClusterManifest(bootstrapClusterManifest);

                    if (staticTopologyProviderElement.NodeList == null || staticTopologyProviderElement.NodeList.Length == 0)
                    {
                        TextLogger.LogError("Static topology provider section of bootstrap cluster manifest does not specify topology of the Service Fabric cluster.");

                        return false;
                    }

                    LogClusterTopology(staticTopologyProviderElement);

                    ClusterTopology staticTopologyProviderClusterTopology = ClusterTopology.GetClusterTopology();

                    staticTopologyProviderClusterTopology.LogTopology();

                    expectedTopology = staticTopologyProviderClusterTopology;

                    TextLogger.LogInfo("Successfully discovered cluster topology.");

                    return true;
                }                
            }
            catch (Exception e)
            {
                TextLogger.LogError("Failed to discover cluster topology : {0}", e);

                return false;
            }
        }

        public bool ConfigureLocalNode(ClusterTopology topologyToDeploy)
        {
            try
            {
                TextLogger.LogInfo("Configuring local Service Fabric node based on bootstrap cluster manifest.");                

                if (!this.InvokeFabricDeployer(topologyToDeploy, DeploymentType.Create, false))
                {
                    TextLogger.LogError("Failed to configure local Service Fabric node based on bootstrap cluster manifest.");

                    return false;
                }

                Utilities.SetWindowsFabricNodeConfigurationCompleted(false);

                TextLogger.LogInfo("Successfully configured local Service Fabric node based on bootstrap cluster manifest.");

                return true;
            }
            catch (Exception e)
            {
                TextLogger.LogError("Failed to configure local Service Fabric node based on bootstrap cluster manifest : {0}", e);

                return false;
            }
        }

        public bool StartLocalNode()
        {
            try
            {
                TextLogger.LogInfo("Starting local Service Fabric node.");

                // TODO: After current xcopyable issue is fixed, update the pattern here.

                // TODO: [General] disposal of ServiceController
                ServiceController fabricHostService = this.GetInstalledService(Constants.FabricHostServiceName);

                if (fabricHostService != null)
                {
                    if (fabricHostService.Status == ServiceControllerStatus.StartPending)
                    {
                        TextLogger.LogWarning("Fabric host service is starting.");

                        return false;
                    }
                    else if (fabricHostService.Status == ServiceControllerStatus.Running)
                    {
                        TextLogger.LogInfo("Successfully started local Service Fabric node.");

                        return true;
                    }
                    else if (!this.StartFabricHostService())
                    {
                        TextLogger.LogError("Failed to start local Service Fabric node ");

                        return false;
                    }
                }
                else
                {
                    ServiceController fabricInstallerService = this.GetInstalledService(Constants.FabricInstallerServiceName);

                    // Install and configure Fabric installer service if not installed.
                    if (fabricInstallerService == null)
                    {
                        TextLogger.LogInfo("Installing and configuring Fabric installer service.");

                        if (!this.InstallAndConfigureFabricInstallerService(this.agentApplicationDirectory))
                        {
                            TextLogger.LogError("Failed to install and configure Fabric installer service");

                            return false;
                        }

                        fabricInstallerService = this.GetInstalledService(Constants.FabricInstallerServiceName);
                    }

                    // If Fabric installer service is not in stopped state, stop it.
                    if (fabricInstallerService.Status != ServiceControllerStatus.Stopped)
                    {
                        TextLogger.LogInfo("Fabric installer service is in {0} state. Stopping it.", fabricInstallerService.Status);

                        if (!this.StopFabricInstallerService())
                        {
                            TextLogger.LogError("Failed to stop Fabric installer service.");

                            return false;
                        }
                    }

                    // Start Fabric installer service to start Fabric host service.                
                    TextLogger.LogInfo("Starting Fabric installer service to start Fabric host service.");
                    if (!this.StartFabricInstallerService())
                    {
                        TextLogger.LogError("Failed to start Fabric installer service.");

                        return false;
                    }
                }

                TextLogger.LogInfo("Successfully started local Service Fabric node.");

                return true;                
            }
            catch (Exception e)
            {
                TextLogger.LogError("Failed to start local Service Fabric node : {0}", e);

                return false;
            }
        }

        public void StopLocalNode()
        {
            // AP service manager waits for ShutdownTimeoutInMilliseconds (by default 5s) before forcefully terminating the service host.
            // For now we are not waiting for Fabric host service to stop on stopping path. 
            ServiceController fabricInstallerService = this.GetInstalledService(Constants.FabricInstallerServiceName);

            if (fabricInstallerService != null)
            {
                if (fabricInstallerService.Status != ServiceControllerStatus.StopPending && fabricInstallerService.Status != ServiceControllerStatus.Stopped)
                {
                    TextLogger.LogInfo("Stopping Fabric installer serivce.");

                    fabricInstallerService.Stop();
                }
            }

            ServiceController fabricHostService = this.GetInstalledService(Constants.FabricHostServiceName);

            if (fabricHostService != null)
            {
                if (fabricHostService.Status != ServiceControllerStatus.StopPending && fabricHostService.Status != ServiceControllerStatus.Stopped)
                {
                    TextLogger.LogInfo("Stopping Fabric host serivce.");

                    fabricHostService.Stop();
                }                
            }            
        }

        public ServiceController GetInstalledService(string serviceName)
        {
            ServiceController[] allServices = ServiceController.GetServices();

            return allServices.FirstOrDefault(service => service.ServiceName.Equals(serviceName, StringComparison.OrdinalIgnoreCase));
        }

        private bool Initialize()
        {
            string localMachineSku;
            if (!this.GetLocalMachineSku(out localMachineSku))
            {
                return false;
            }

            this.LocalMachineSku = localMachineSku;

            string dataRootDirectory;
            if (!this.GetAndCreateDataRootDirectory(out dataRootDirectory))
            {
                return false;
            }

            this.DataRootDirectory = dataRootDirectory;

            this.LocalMachineIpAddress = this.GetLocalMachineIpAddress();

            return true;
        }

        private bool GetLocalMachineSku(out string localMachineSku)
        {
            localMachineSku = null;

            if (this.useTestMode)
            {
                localMachineSku = "TestModeMachineSku";

                return true;
            }

            // When server pool is enabled, logical machine name would be used. Otherwise, physical machine name would be used.
            // Note that in a server pool disabled environment, logical machine names would be the same as physical machine names.
            string machineInfoFile = Path.Combine(APRuntime.DataDirectory, StringConstants.MachineInfoFileName);
            string localMachineName = APRuntime.MachineName;

            try
            {
                using (CsvReader reader = new CsvReader(machineInfoFile))
                {
                    while (reader.Read())
                    {
                        if (string.Compare(reader["MachineName"], localMachineName, StringComparison.OrdinalIgnoreCase) == 0)
                        {
                            localMachineSku = reader["SKU"];

                            TextLogger.LogInfo("Local machine {0} is using machine SKU {1}.", localMachineName, localMachineSku);

                            return true;
                        }
                    }
                }

                TextLogger.LogError("Local machine {0} is not in machine info file {1}.", localMachineName, machineInfoFile);

                return false;
            }
            catch (Exception e)
            {
                TextLogger.LogError("Failed to get machine SKU of local machine {0} from machine info file {1} : {2}", localMachineName, machineInfoFile, e);

                return false;
            }
        }

        private bool GetAndCreateDataRootDirectory(out string dataRootDirectory)
        {
            dataRootDirectory = null;

            try
            {
                // On AP machines, data root would depend on machine SKU (specifically the SKU's disk layout). For test mode testing, use C:\SFRoot.
                if (this.useTestMode)
                {
                    dataRootDirectory = Path.Combine(Path.GetPathRoot("C:\\"), StringConstants.ServiceFabricRootDirectoryName);

                    TextLogger.LogInfo("Using {0} as data root directory in test mode.", dataRootDirectory);
                }
                else
                {
                    string diskLayoutConfigurationFile = Path.Combine(this.agentApplicationDirectory, StringConstants.DiskLayoutConfigurationFileName);

                    TextLogger.LogInfo("Data root directory would be determined based on local machine SKU {0} and disk layout configuration file {1}.", this.LocalMachineSku, diskLayoutConfigurationFile);

                    string dataRootHint = null;
                    using (CsvReader reader = new CsvReader(diskLayoutConfigurationFile))
                    {
                        while (reader.Read())
                        {
                            if (Regex.IsMatch(this.LocalMachineSku, reader["SKU"], RegexOptions.IgnoreCase | RegexOptions.CultureInvariant))
                            {
                                dataRootHint = Path.GetFullPath(Environment.ExpandEnvironmentVariables(reader["DataRootHint"]));

                                TextLogger.LogInfo(
                                    "Local machine SKU {0} matched disk layout SKU rule '{1}' -> '{2}' which expands to data root '{3}'",
                                    this.LocalMachineSku,
                                    reader["SKU"],
                                    reader["DataRootHint"],
                                    dataRootHint);

                                break;
                            }
                        }
                    }

                    if (string.IsNullOrEmpty(dataRootHint))
                    {
                        TextLogger.LogError("Disk layout configuration file {0} does not contain a valid entry for local machine SKU {1}.", diskLayoutConfigurationFile, this.LocalMachineSku);

                        return false;
                    }                    

                    dataRootDirectory = Path.Combine(Path.GetPathRoot(dataRootHint), StringConstants.ServiceFabricRootDirectoryName);

                    TextLogger.LogInfo("Using {0} as data root directory based on local machine SKU {1} and disk layout configuration file {2}.", dataRootDirectory, this.LocalMachineSku, diskLayoutConfigurationFile);
                }

                TextLogger.LogInfo("Creating data root directory {0}.", dataRootDirectory);

                Directory.CreateDirectory(dataRootDirectory);

                return true;
            }
            catch (Exception e)
            {
                TextLogger.LogError("Failed to get and create data root directory : {0}", e);

                return false;
            }
        }

        private string GetLocalMachineIpAddress()
        {            
            if (this.useTestMode)
            {                
                IPHostEntry hostEntry = Dns.GetHostEntry(Dns.GetHostName());
                foreach (IPAddress ipAddress in hostEntry.AddressList)
                {
                    if (ipAddress.AddressFamily == AddressFamily.InterNetwork)
                    {
                        return ipAddress.ToString();
                    }
                }

                return IPAddress.None.ToString();
            }

            // When server pool is enabled, logical IP address would be used, otherwise physical IP address would be used.
            // Note that in a server pool disabled environment, logical IP addresses would be the same as physical IP addresses.
            return APRuntime.StaticIpAddress;
        }

        private bool SetDynamicPortRange(int startPort, int endPort)
        {
            int portCount = endPort - startPort + 1;

            TextLogger.LogInfo("Setting dynamic port range. Start port : {0}. Port count : {1}.", startPort, portCount);

            if (Socket.OSSupportsIPv4)
            {
                if (!SetDynamicPortRange(startPort, portCount, "ipv4", "tcp"))
                {
                    TextLogger.LogError("Failed to set dynamic port range for ipv4:tcp. Start port : {0}. Port count : {1}.", startPort, portCount);

                    return false;
                }

                if (!SetDynamicPortRange(startPort, portCount, "ipv4", "udp"))
                {
                    TextLogger.LogError("Failed to set dynamic port range for ipv4:udp. Start port : {0}. Port count : {1}.", startPort, portCount);

                    return false;
                }
            }

            if (Socket.OSSupportsIPv6)
            {
                if (!SetDynamicPortRange(startPort, portCount, "ipv6", "tcp"))
                {
                    TextLogger.LogError("Failed to set dynamic port range for ipv6:tcp. Start port : {0}. Port count : {1}.", startPort, portCount);

                    return false;
                }

                if (!SetDynamicPortRange(startPort, portCount, "ipv6", "udp"))
                {
                    TextLogger.LogError("Failed to set dynamic port range for ipv6:udp. Start port : {0}. Port count : {1}.", startPort, portCount);

                    return false;
                }
            }

            return true;
        }

        private bool SetDynamicPortRange(int startPort, int portCount, string networkInterface, string transportProtocol)
        {
            int exitCode = CommandLineUtility.ExecuteNetshCommand(
                string.Format(
                    CultureInfo.InvariantCulture,
                    "int {0} set dynamicport {1} start={2} num={3}",
                    networkInterface,
                    transportProtocol,
                    startPort,
                    portCount));

            return exitCode == 0;
        }
        
        private bool ApplyPortForwardingRule(PortFowardingRule portForwardingRule)
        {
            if (string.Compare(portForwardingRule.MachineFunction, "*", StringComparison.OrdinalIgnoreCase) == 0 || string.Compare(portForwardingRule.MachineFunction, APRuntime.MachineFunction, StringComparison.OrdinalIgnoreCase) == 0)
            {
                TextLogger.LogInfo("Applying port forwarding rule {0} from port {1} to port {2}.", portForwardingRule.RuleName, portForwardingRule.ListenPort, portForwardingRule.ConnectPort);

                return this.CreatePortProxy(IPAddress.Any.ToString(), portForwardingRule.ListenPort, this.LocalMachineIpAddress, portForwardingRule.ConnectPort);
            }
            else
            {
                TextLogger.LogInfo("Port forwarding rule {0} does not apply to machine function {1}.", portForwardingRule.RuleName, APRuntime.MachineFunction);

                return true;
            }
        }

        private bool CreatePortProxy(string listenAddress, int listenPort, string connectAddress, int connectPort)
        {
            int exitCode = CommandLineUtility.ExecuteNetshCommand(
                string.Format(
                    CultureInfo.InvariantCulture,
                    "interface portproxy add v4tov4 listenport={0} listenaddress={1} connectport={2} connectaddress={3}",
                    listenPort,
                    listenAddress,
                    connectPort,
                    connectAddress));

            return exitCode == 0;
        }

        private bool ReadBootstrapClusterManifestFile(out ClusterManifestType bootstrapClusterManifest)
        {
            bootstrapClusterManifest = null;

            string bootstrapClusterManifestFile = this.GetBootstrapClusterManifestFile();
            try
            {
                if (!File.Exists(bootstrapClusterManifestFile))
                {
                    TextLogger.LogError("Bootstrap cluster manifest does not exist in application directory as {0}.", bootstrapClusterManifestFile);

                    return false;
                }

                /*
                 * As part of application directory, the bootstrap cluster manifest would only be updated via application upgrades where the agent host would be stopped prior to the update.                 
                 */
                    using (FileStream fileStream = File.Open(bootstrapClusterManifestFile, FileMode.Open, FileAccess.Read, FileShare.Read))
                {
                    // TODO: validation against schema
                    XmlReaderSettings xmlReaderSettings = new XmlReaderSettings();
                    xmlReaderSettings.ValidationType = ValidationType.None;
                    xmlReaderSettings.XmlResolver = null;

                    using (XmlReader xmlReader = XmlReader.Create(fileStream, xmlReaderSettings))
                    {
                        XmlSerializer serializer = new XmlSerializer(typeof(ClusterManifestType));

                        bootstrapClusterManifest = (ClusterManifestType)serializer.Deserialize(xmlReader);
                    }
                }

                return true;
            }
            catch (Exception e)
            {
                TextLogger.LogError("Failed to read bootstrap cluster manifest file {0} : {1}", bootstrapClusterManifestFile, e);

                return false;
            }
        }

        private void LogClusterTopology(ClusterManifestTypeInfrastructureWindowsAzureStaticTopology clusterTopology)
        {
            TextLogger.LogInfo("Cluster topology from static topology provider:");

            TextLogger.LogInfo("NodeName, NodeType, IsSeedNode, UpgradeDomain, FaultDomain, IPAddressOrFQDN");

            foreach (FabricNodeType node in clusterTopology.NodeList)
            {
                TextLogger.LogInfo(
                    "{0}, {1}, {2}, {3}, {4}, {5}",
                    node.NodeName,
                    node.NodeTypeRef,
                    node.IsSeedNode,
                    node.UpgradeDomain,
                    node.FaultDomain,
                    node.IPAddressOrFQDN
                );
            }
        }

        private bool InvokeFabricDeployer(ClusterTopology topologyToDeploy, DeploymentType deploymentType, bool toUseCurrentClusterManifestForDeployment)
        {
            TextLogger.LogInfo("Invoking FabricDeployer with deployment type = {0} and toUseCurrentClusterManifestForDeployment = {1}.", deploymentType, toUseCurrentClusterManifestForDeployment);

            string clusterManifestFileToDeploy;
            if (toUseCurrentClusterManifestForDeployment)
            {
                string currentClusterManifestFile = this.GetCurrentClusterManifestFile(topologyToDeploy.CurrentNodeName);

                if (File.Exists(currentClusterManifestFile))
                {
                    TextLogger.LogInfo("Using current cluster manifest file {0} for deployment.", currentClusterManifestFile);

                    clusterManifestFileToDeploy = currentClusterManifestFile;
                }
                else
                {
                    TextLogger.LogError("Current cluster manifest file does not exist as {0} when toUseCurrentClusterManifestForDeployment = true.", currentClusterManifestFile);

                    return false;
                }
            }
            else
            {
                clusterManifestFileToDeploy = this.GetBootstrapClusterManifestFile();
            }
           
            if (!this.FabricDeploy(deploymentType, clusterManifestFileToDeploy, this.DataRootDirectory, topologyToDeploy.ServiceFabricNodes, true))
            {
                TextLogger.LogError(
                    "Failed to invoke FabricDeployer with deployment type = {0}, cluster manifest file = {1}, data root directory = {2}.",
                    deploymentType,                    
                    clusterManifestFileToDeploy,
                    this.DataRootDirectory);

                return false;
            }

            return true;
        }

        private string GetServiceFabricCabFile()
        {
            return Path.Combine(this.agentApplicationDirectory, Constants.FabricPackageCabinetFile);
        }

        private string GetBootstrapClusterManifestFile()
        {
            string bootstrapClusterManifestFile;

            if (ConfigurationManager.BootstrapClusterManifestLocation == BootstrapClusterManifestLocationType.Default || this.useTestMode)
            {
                bootstrapClusterManifestFile = Path.Combine(this.agentApplicationDirectory, StringConstants.DefaultBootstrapClusterManifestFileName);
            }
            else if (ConfigurationManager.BootstrapClusterManifestLocation == BootstrapClusterManifestLocationType.EnvironmentDefault)
            {
                bootstrapClusterManifestFile = Path.Combine(this.agentApplicationDirectory, StringConstants.EnvironmentDefaultConfigDirectoryName, APRuntime.ClusterName, APRuntime.EnvironmentName, StringConstants.DefaultBootstrapClusterManifestFileName);
            }
            else
            {
                throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Bootstrap cluster manifest file is not available based on current BootstrapClusterManifestLocation {0}.", ConfigurationManager.BootstrapClusterManifestLocation));
            }

            TextLogger.LogInfo("Based on bootstrapClusterManifestLocation {0} and useTestMode {1}, bootstrapClusterManifestFile = {2}", ConfigurationManager.BootstrapClusterManifestLocation, this.useTestMode, bootstrapClusterManifestFile);

            return bootstrapClusterManifestFile;
        }      
    }    
}
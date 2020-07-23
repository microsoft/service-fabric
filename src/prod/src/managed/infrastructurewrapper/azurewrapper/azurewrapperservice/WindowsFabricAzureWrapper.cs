// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#define TRACE

namespace Microsoft.Fabric.InfrastructureWrapper
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.ServiceProcess;
    using System.Threading;
    using System.Xml;
    using System.Xml.Serialization;

    using Microsoft.WindowsAzure;
    using Microsoft.WindowsAzure.ServiceRuntime;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Blob;
    using Microsoft.WindowsAzure.Storage.RetryPolicies;
    using Win32;

    // State of WindowsFabric AzureWrapper Service.
    // Note that the order of the values matters.
    internal enum WindowsFabricAzureWrapperState
    {
        None,

        ServiceStopped,

        ServiceStopping,

        ServiceStarted,

        ManagingNetworkConfiguration,

        InstallingProduct,

        DiscoveringTopology,

        ConfiguringNode,

        StartingNode,

        NodeStarted
    }

    internal class WindowsFabricAzureWrapper : InfrastructureWrapper
    {
        private static volatile LocalNodeConfiguration localNodeConfiguration = null;

        // Callback to stop the AzureWrapper service.
        private readonly Action stopServiceCallback;

        private readonly object serviceStateLock = new object();

        private WindowsFabricAzureWrapperState serviceState = WindowsFabricAzureWrapperState.ServiceStarted;

        // The topology that is currently running.
        private ClusterTopology deployedTopology = null;

        // The topology that is expected to be running.
        private ClusterTopology expectedTopology = null;

        // Whether the expected topology is running.
        private bool isExpectedTopologyRunning = false;

        private bool computingStateTransition = false;

        // The last failed deployment.
        private DateTime lastFailedDeploymentAction = DateTime.MinValue;

        // Path of currently running cluster manifest in xstore and in local file system.
        private string clusterManifestLocation = null;

        private string clusterManifestFile = null;

        private string pluginDirectory = string.Empty;

        private bool emulatorEnvironment = false;

        // The data root directory before emulator starts. It is only used in emulator environment.
        private string emulatorEnvironmentLastDataRootDirectory = null;

        private bool multipleCloudServiceDeployment = false;

        private bool subscribeBlastNotification = false;

        public WindowsFabricAzureWrapper(Action stopServiceCallback)
        {
            this.stopServiceCallback = stopServiceCallback;
        }

        // Initialize the AzureWrapper service.
        // In case of failures during initialization, throw directly and the service will be restarted.      
        public void OnStart(string[] args)
        {
            try
            {
                localNodeConfiguration = LocalNodeConfiguration.GetLocalNodeConfiguration();

                this.InitializeDirectories(localNodeConfiguration);

                TextLogger.InitializeLogging(localNodeConfiguration.DeploymentRootDirectory, localNodeConfiguration.TextLogTraceLevel);

                this.emulatorEnvironment = RoleEnvironment.IsEmulated;

                this.multipleCloudServiceDeployment = localNodeConfiguration.MultipleCloudServiceDeployment;

                this.subscribeBlastNotification = localNodeConfiguration.SubscribeBlastNotification;

                this.pluginDirectory = Path.GetDirectoryName(Assembly.GetEntryAssembly().Location);

                this.clusterManifestLocation = null;
                this.clusterManifestFile = null;

                LogPluginVersion();

                LogLocalNodeConfiguration(localNodeConfiguration);

                TextLogger.LogInfo("Running under emulator environment : {0}", this.emulatorEnvironment);

                if (this.emulatorEnvironment)
                {
                    this.emulatorEnvironmentLastDataRootDirectory = Utilities.GetWindowsFabricDataRootRegKey();

                    TextLogger.LogInfo("Last value of FabricDataRoot : {0}", this.emulatorEnvironmentLastDataRootDirectory ?? "null");
                }

                WindowsFabricAzureWrapperState initialState;

                if (!this.TryComputeInitialState(out initialState))
                {
                    TextLogger.LogError("Failed to compute the initial service state when initializing the service. Exiting from the service.");

                    this.OnStop();

                    this.stopServiceCallback();
                }
                else
                {
                    TextLogger.LogInfo("Initial service state : {0}.", initialState);

                    lock (this.serviceStateLock)
                    {
                        this.serviceState = initialState;

                        this.deployedTopology = null;

                        this.expectedTopology = this.multipleCloudServiceDeployment ? null : ClusterTopology.GetClusterTopologyFromServiceRuntime(localNodeConfiguration);

                        this.isExpectedTopologyRunning = false;

                        this.lastFailedDeploymentAction = DateTime.MinValue;

                        this.computingStateTransition = false;
                    }
                }

                this.RegisterRoleEnvironmentNotifications();
            }
            catch (Exception e)
            {
                TextLogger.LogErrorToEventLog("Exception occurred when initializing the service : {0}", e);

                this.OnStop();

                throw;
            }
        }

        public void OnStop()
        {
            lock (this.serviceStateLock)
            {
                if (this.serviceState < WindowsFabricAzureWrapperState.ServiceStarted)
                {
                    return;
                }

                this.serviceState = WindowsFabricAzureWrapperState.ServiceStopping;

                this.UnregisterRoleEnvironmentNotifications();

                if (this.emulatorEnvironment)
                {
                    this.StopFabricHostProcess();

                    if (!string.IsNullOrEmpty(this.emulatorEnvironmentLastDataRootDirectory))
                    {
                        try
                        {
                            Utilities.SetWindowsFabricDataRootRegKey(this.emulatorEnvironmentLastDataRootDirectory);

                            TextLogger.LogInfo("Successfully set FabricDataRoot registry key to its last value {0}.", this.emulatorEnvironmentLastDataRootDirectory);
                        }
                        catch (Exception e)
                        {
                            TextLogger.LogWarning("Exception occurred when setting FabricDataRoot registry key to its last value {0} : {1}", this.emulatorEnvironmentLastDataRootDirectory, e);
                        }
                    }
                }
                else
                {
                    this.StopFabricHostService();
                    TextLogger.LogInfo("Fabric Host Service Stopped Successfully.");
                }

                this.serviceState = WindowsFabricAzureWrapperState.ServiceStopped;
            }
        }

        public void OnShutdown()
        {
            this.OnStop();
        }

        private static void LogClusterTopology(ClusterTopology clusterTopology)
        {
            TextLogger.LogInfo("Cluster Topology:");

            if (clusterTopology.WindowsFabricNodes.Count == 0)
            {
                // An empty node list indicates that cluster topology is statically defined in cluster manifest.
                TextLogger.LogInfo("Cluster topology is statically defined in cluster manifest.");

                return;
            }

            TextLogger.LogInfo("NodeName, RoleOrTierName, UpgradeDomain, FaultDomain, IPAddressOrFQDN, ClientConnectionEndpoint, HttpGatewayEndpoint, HttpApplicationGatewayEndpoint, ClusterConnectionEndpoint, LeaseDriverEndpoint, ApplicationEndpoints");

            foreach (InfrastructureNodeType node in clusterTopology.WindowsFabricNodes)
            {
                TextLogger.LogInfo(
                    "{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}",
                    node.NodeName,
                    node.RoleOrTierName,
                    node.UpgradeDomain,
                    node.FaultDomain,
                    node.IPAddressOrFQDN,
                    node.Endpoints.ClientConnectionEndpoint == null ? string.Empty : string.Format(CultureInfo.InvariantCulture, "{0}/{1}", node.Endpoints.ClientConnectionEndpoint.Protocol, node.Endpoints.ClientConnectionEndpoint.Port),
                    node.Endpoints.HttpGatewayEndpoint == null ? string.Empty : string.Format(CultureInfo.InvariantCulture, "{0}/{1}", node.Endpoints.HttpGatewayEndpoint.Protocol, node.Endpoints.HttpGatewayEndpoint.Port),
                    node.Endpoints.HttpApplicationGatewayEndpoint == null ? string.Empty : string.Format(CultureInfo.InvariantCulture, "{0}/{1}", node.Endpoints.HttpApplicationGatewayEndpoint.Protocol, node.Endpoints.HttpApplicationGatewayEndpoint.Port),
                    node.Endpoints.ClusterConnectionEndpoint == null ? string.Empty : string.Format(CultureInfo.InvariantCulture, "{0}/{1}", node.Endpoints.ClusterConnectionEndpoint.Protocol, node.Endpoints.ClusterConnectionEndpoint.Port),
                    node.Endpoints.LeaseDriverEndpoint == null ? string.Empty : string.Format(CultureInfo.InvariantCulture, "{0}/{1}", node.Endpoints.LeaseDriverEndpoint.Protocol, node.Endpoints.LeaseDriverEndpoint.Port),
                    node.Endpoints.ApplicationEndpoints == null ? string.Empty : string.Format(CultureInfo.InvariantCulture, "{0}-{1}", node.Endpoints.ApplicationEndpoints.StartPort, node.Endpoints.ApplicationEndpoints.EndPort)
                );
            }
        }

        private static void LogClusterTopology(ClusterManifestTypeInfrastructureWindowsAzureStaticTopology clusterTopology)
        {
            TextLogger.LogInfo("Cluster Topology From Static Topology Provider:");

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

        private static void LogPluginVersion()
        {
            string fileVersion = Process.GetCurrentProcess().MainModule?.FileVersionInfo?.FileVersion ?? string.Empty;
            TextLogger.LogInfo("Executable version: {0}", fileVersion);
        }

        private static void LogLocalNodeConfiguration(LocalNodeConfiguration localNodeConfiguration)
        {
            TextLogger.LogInfo("Local Node Configuration:");
            TextLogger.LogInfo("{0} : {1}", WindowsFabricAzureWrapperServiceCommon.ApplicationPortCountConfigurationSetting, localNodeConfiguration.ApplicationPortCountConfigurationSettingValue);
            TextLogger.LogInfo("{0} : {1}", WindowsFabricAzureWrapperServiceCommon.EphemeralPortCountConfigurationSetting, localNodeConfiguration.EphemeralPortCountConfigurationSettingValue);
            TextLogger.LogInfo("ApplicationStartPort : {0}", localNodeConfiguration.ApplicationStartPort);
            TextLogger.LogInfo("ApplicationEndPort : {0}", localNodeConfiguration.ApplicationEndPort);
            TextLogger.LogInfo("EphemeralStartPort : {0}", localNodeConfiguration.EphemeralStartPort);
            TextLogger.LogInfo("EphemeralEndPort : {0}", localNodeConfiguration.EphemeralEndPort);
            TextLogger.LogInfo("RootDirectory : {0}", localNodeConfiguration.RootDirectory);
            TextLogger.LogInfo("DataRootDirectory : {0}", localNodeConfiguration.DataRootDirectory);
            TextLogger.LogInfo("DeploymentRootDirectory : {0}", localNodeConfiguration.DeploymentRootDirectory);
            TextLogger.LogInfo("DeploymentRetryIntervalSeconds : {0}", localNodeConfiguration.DeploymentRetryIntervalSeconds);
            TextLogger.LogInfo("TextLogTraceLevel : {0}", localNodeConfiguration.TextLogTraceLevel);
            TextLogger.LogInfo("MultipleCloudServiceDeployment : {0}", localNodeConfiguration.MultipleCloudServiceDeployment);
            TextLogger.LogInfo("SubscribeBlastNotification : {0}", localNodeConfiguration.SubscribeBlastNotification);
        }

        private bool TryComputeInitialState(out WindowsFabricAzureWrapperState initialState)
        {
            initialState = WindowsFabricAzureWrapperState.None;

            if (this.emulatorEnvironment)
            {
                try
                {
                    string installedWindowsFabricVersion = Utilities.GetCurrentWindowsFabricVersion();

                    if (installedWindowsFabricVersion == null || this.IsWindowsFabricV1Installed(installedWindowsFabricVersion) || string.IsNullOrEmpty(this.emulatorEnvironmentLastDataRootDirectory))
                    {
                        TextLogger.LogError("Emulator environment: Windows Fabric V2.0 or a higher version has not been installed locally or the installation is corrupted.");

                        return false;
                    }
                    else
                    {
                        TextLogger.LogInfo("Emulator environment: Windows Fabric version {0} is installed locally.", installedWindowsFabricVersion);
                    }

                    TextLogger.LogInfo("Skip network configuration management and product installation in emulator environment.");

                    initialState = this.multipleCloudServiceDeployment ? WindowsFabricAzureWrapperState.DiscoveringTopology : WindowsFabricAzureWrapperState.ConfiguringNode;
                }
                catch (Exception e)
                {
                    TextLogger.LogError("Exception occurred when computing the initial state in emulator environment : {0}", e);

                    if (WindowsFabricAzureWrapperServiceCommon.IsFatalException(e))
                    {
                        throw;
                    }

                    return false;
                }
            }
            else
            {
                initialState = WindowsFabricAzureWrapperState.ManagingNetworkConfiguration;
            }

            return true;
        }

        private void StateTransitionCallback(object state)
        {
            WindowsFabricAzureWrapperState currentState = this.GetCurrentState();

            TextLogger.LogVerbose("StateTransitionCallback : Current service state {0}.", currentState);

            if (currentState > WindowsFabricAzureWrapperState.ServiceStarted)
            {
                bool computeStateTransitionResult;

                do
                {
                    computeStateTransitionResult = this.ComputeStateTransition(currentState);

                    TextLogger.LogInfo("StateTransitionCallback: ComputeStateTransition returns {0} with current service state {1}.", computeStateTransitionResult, currentState);

                    if (!computeStateTransitionResult)
                    {
                        lock (this.serviceStateLock)
                        {
                            this.lastFailedDeploymentAction = DateTime.UtcNow;
                        }
                    }
                    else
                    {
                        lock (this.serviceStateLock)
                        {
                            this.lastFailedDeploymentAction = DateTime.MinValue;
                        }
                    }

                    currentState = this.GetCurrentState();
                }
                while (currentState > WindowsFabricAzureWrapperState.ServiceStarted && currentState < WindowsFabricAzureWrapperState.NodeStarted && computeStateTransitionResult);
            }

            lock (this.serviceStateLock)
            {
                this.computingStateTransition = false;
            }
        }

        private bool ComputeStateTransition(WindowsFabricAzureWrapperState currentState)
        {
            if (currentState >= WindowsFabricAzureWrapperState.ManagingNetworkConfiguration && currentState < WindowsFabricAzureWrapperState.NodeStarted)
            {
                TextLogger.LogInfo("ComputeStateTransition : Current service state {0}", currentState);
            }
            else
            {
                TextLogger.LogVerbose("ComputeStateTransition : Current service state {0}", currentState);
            }

            switch (currentState)
            {
                case WindowsFabricAzureWrapperState.ManagingNetworkConfiguration:
                    {
                        if (!this.ManageNetworkConfiguration())
                        {
                            return false;
                        }

                        WindowsFabricAzureWrapperState nextState;

                        if (!this.TryComputeStateTransitionFromManagingNetworkConfigurationState(out nextState))
                        {
                            return false;
                        }

                        this.SetNextState(nextState);

                        break;
                    }

                case WindowsFabricAzureWrapperState.InstallingProduct:
                    {
                        if (!this.InstallProduct())
                        {
                            return false;
                        }

                        if (this.multipleCloudServiceDeployment)
                        {
                            this.SetNextState(WindowsFabricAzureWrapperState.DiscoveringTopology);
                        }
                        else
                        {
                            this.SetNextState(WindowsFabricAzureWrapperState.ConfiguringNode);
                        }

                        break;
                    }

                case WindowsFabricAzureWrapperState.DiscoveringTopology:
                    {
                        if (!this.DiscoverTopology())
                        {
                            return false;
                        }

                        this.SetNextState(WindowsFabricAzureWrapperState.ConfiguringNode);

                        break;
                    }

                case WindowsFabricAzureWrapperState.ConfiguringNode:
                    {
                        ClusterTopology topologyToDeploy;

                        lock (this.serviceStateLock)
                        {
                            topologyToDeploy = this.expectedTopology;
                        }

                        if (!this.ConfigureNode(topologyToDeploy))
                        {
                            return false;
                        }

                        lock (this.serviceStateLock)
                        {
                            this.deployedTopology = topologyToDeploy;

                            this.isExpectedTopologyRunning = !this.deployedTopology.IsClusterTopologyChanged(this.expectedTopology);
                        }

                        this.SetNextState(WindowsFabricAzureWrapperState.StartingNode);

                        break;
                    }

                case WindowsFabricAzureWrapperState.StartingNode:
                    {
                        bool isNodeStarted;

                        lock (this.serviceStateLock)
                        {
                            isNodeStarted = this.StartNode();
                        }

                        if (!isNodeStarted)
                        {
                            return false;
                        }

                        this.SetNextState(WindowsFabricAzureWrapperState.NodeStarted);

                        break;
                    }

                case WindowsFabricAzureWrapperState.NodeStarted:
                    {
                        if (this.multipleCloudServiceDeployment)
                        {
                            lock (this.serviceStateLock)
                            {
                                this.isExpectedTopologyRunning = true;
                                this.deployedTopology = ClusterTopology.GetClusterTopologyFromStaticTopologyProvider();
                                this.expectedTopology = this.deployedTopology;
                            }

                            break;
                        }

                        bool isExpectedTopologyRunning;
                        ClusterTopology topologyToDeploy;

                        lock (this.serviceStateLock)
                        {
                            isExpectedTopologyRunning = this.isExpectedTopologyRunning;
                            topologyToDeploy = this.expectedTopology;
                        }

                        if (!isExpectedTopologyRunning)
                        {
                            if (!this.UpdateNode(topologyToDeploy))
                            {
                                return false;
                            }

                            lock (this.serviceStateLock)
                            {
                                this.deployedTopology = topologyToDeploy;

                                this.isExpectedTopologyRunning = !this.deployedTopology.IsClusterTopologyChanged(this.expectedTopology);
                            }
                        }

                        break;
                    }
            }

            return true;
        }

        private bool ManageNetworkConfiguration()
        {
            try
            {
                TextLogger.LogInfo("Managing network configuration.");

                // Configure dynamic port range, firewalls, etc.                
                if (!NetworkConfigManager.ReduceDynamicPortRange(localNodeConfiguration.EphemeralStartPort, localNodeConfiguration.EphemeralEndPort))
                {
                    TextLogger.LogError("Failed to reduce dynamic port range.");

                    return false;
                }

                if (!NetworkConfigManager.AllowDynamicPortsTraffic(localNodeConfiguration.EphemeralStartPort, localNodeConfiguration.EphemeralEndPort))
                {
                    TextLogger.LogError("Failed to allow traffic to dynamic port range.");

                    return false;
                }

                if (!NetworkConfigManager.AllowNonWindowsFabricPortsTraffic(localNodeConfiguration.NonWindowsFabricEndpointSet))
                {
                    TextLogger.LogError("Failed to allow traffic to non Windows Fabric ports and port ranges.");

                    return false;
                }

                TextLogger.LogInfo("Successfully completed network configuration management.");
            }
            catch (Exception e)
            {
                TextLogger.LogError("Exception occurred when managing network configuration : {0}", e);

                if (WindowsFabricAzureWrapperServiceCommon.IsFatalException(e))
                {
                    throw;
                }

                return false;
            }

            return true;
        }

        private bool TryComputeStateTransitionFromManagingNetworkConfigurationState(out WindowsFabricAzureWrapperState nextState)
        {
            nextState = WindowsFabricAzureWrapperState.None;

            try
            {
                bool isWindowsFabricNodeConfigurationCompleted = Utilities.IsWindowsFabricNodeConfigurationCompleted(false);

                if (!isWindowsFabricNodeConfigurationCompleted)
                {
                    bool isWindowsFabricNodeConfigurationCompletedWithDepreciatedKey = Utilities.IsWindowsFabricNodeConfigurationCompleted(true);

                    if (isWindowsFabricNodeConfigurationCompletedWithDepreciatedKey)
                    {
                        TextLogger.LogInfo("Windows Fabric node configuration has been completed based on depreciated key.");

                        Utilities.SetWindowsFabricNodeConfigurationCompleted(false);

                        isWindowsFabricNodeConfigurationCompleted = true;
                    }
                }

                TextLogger.LogInfo("Windows Fabric node configuration {0} been completed.", isWindowsFabricNodeConfigurationCompleted ? "has" : "has not");

                if (!isWindowsFabricNodeConfigurationCompleted)
                {
                    nextState = WindowsFabricAzureWrapperState.InstallingProduct;
                }
                else
                {
                    ServiceController fabricHostService = WindowsFabricAzureWrapperServiceCommon.GetInstalledService(Constants.FabricHostServiceName);

                    if (fabricHostService == null || fabricHostService.Status != ServiceControllerStatus.Running)
                    {
                        nextState = WindowsFabricAzureWrapperState.StartingNode;
                    }
                    else
                    {
                        nextState = WindowsFabricAzureWrapperState.NodeStarted;
                    }
                }
            }
            catch (Exception e)
            {
                TextLogger.LogError("Exception occurred when computing state transition from service state {0} : {1}", WindowsFabricAzureWrapperState.ManagingNetworkConfiguration, e);

                if (WindowsFabricAzureWrapperServiceCommon.IsFatalException(e))
                {
                    throw;
                }

                return false;
            }

            return true;
        }

        private bool InstallProduct()
        {
            TextLogger.LogInfo("Installing Service Fabric");
            try
            {
                string installedWindowsFabricVersion = Utilities.GetCurrentWindowsFabricVersion();

                // Install the product on a VM with no installation or with V1 installation
                if (this.IsWindowsFabricV1Installed(installedWindowsFabricVersion))
                {
                    TextLogger.LogInfo(
                        "Windows Fabric V1 version {0} is installed locally. Cleaning the regsitry for upgrade.",
                        installedWindowsFabricVersion);

                    Utilities.DeleteWindowsFabricV1VersionRegKey();

                    installedWindowsFabricVersion = Utilities.GetCurrentWindowsFabricVersion();
                }

                if (installedWindowsFabricVersion == null)
                {
                    string microsoftFabricCabinetFilePath = Path.Combine(this.pluginDirectory, Constants.FabricPackageCabinetFile);
                    TextLogger.LogInfo("Checking for Existence of Service Fabric Package Cabinet at Location : {0}", microsoftFabricCabinetFilePath);
                    if (File.Exists(microsoftFabricCabinetFilePath))
                    {
                        TextLogger.LogInfo("Fabric Package exists, Installing Service Fabric product using Service Fabric Package xcopy path.");

                        ServiceController fabricInstallerService = WindowsFabricAzureWrapperServiceCommon.GetInstalledService(Constants.FabricInstallerServiceName);
                        if (fabricInstallerService != null && fabricInstallerService.Status != ServiceControllerStatus.Stopped)
                        {
                            TextLogger.LogWarning("FabricInstallerSvc is already running. Stopping to stabilize scenario.");
                            if (!this.StopFabricInstallerService())
                            {
                                TextLogger.LogWarning("FabricInstallerSvc could not be stopped.");
                                return false;
                            }
                        }

                        string localInstallationPath = Environment.ExpandEnvironmentVariables(Constants.FabricRootPath);
                        if (!Directory.Exists(localInstallationPath))
                        {
                            Directory.CreateDirectory(localInstallationPath);
                        }

                        Utilities.UnCabFile(microsoftFabricCabinetFilePath, localInstallationPath);
                        Utilities.SetFabricRoot(localInstallationPath);
                        Utilities.SetFabricCodePath(Path.Combine(localInstallationPath, Constants.PathToFabricCode));
                        string fabricVersion = Utilities.GetCurrentFabricFileVersion();
                        TextLogger.LogInfo("Local Installation Path is successful at {0} and registry entries are made. Fabric version: {1}.",
                            localInstallationPath, fabricVersion ?? string.Empty);

                        TextLogger.LogInfo("Configuring FabricInstallerSvc.");
                        if (!this.InstallAndConfigureFabricInstallerService(this.pluginDirectory))
                        {
                            TextLogger.LogError("Failed to install fabric installer service");
                            return false;
                        }
                    }
                    else
                    {
                        string msiInstallationArguments = this.GetMsiInstallationArguments(localNodeConfiguration.DeploymentRootDirectory);
                        if (!this.FabricInstall(msiInstallationArguments))
                        {
                            TextLogger.LogError(
                                "Failed to install Windows Fabric through MSI installation with arguments {0}.",
                                msiInstallationArguments);

                            return false;
                        }
                    }

                    installedWindowsFabricVersion = Utilities.GetCurrentWindowsFabricVersion();
                    TextLogger.LogInfo("Service Fabric version {0} has been installed locally.", installedWindowsFabricVersion);
                }
                else if (this.IsWindowsFabricV1Installed(installedWindowsFabricVersion))
                {
                    TextLogger.LogError(
                        "Windows Fabric V1 version {0} is installed locally. Failed to clean the registry for upgrade.",
                        installedWindowsFabricVersion);

                    return false;
                }
                else
                {
                    TextLogger.LogInfo("Windows Fabric version {0} is installed locally. Skip product installation.", installedWindowsFabricVersion);
                }
            }
            catch (Exception e)
            {
                TextLogger.LogError("Exception occurred when installing product : {0}", e);

                if (WindowsFabricAzureWrapperServiceCommon.IsFatalException(e))
                {
                    throw;
                }

                return false;
            }

            return true;
        }

        private bool DiscoverTopology()
        {
            try
            {
                TextLogger.LogInfo("Discovering cluster topology.");

                if (string.Compare(localNodeConfiguration.BootstrapClusterManifestLocation, WindowsFabricAzureWrapperServiceCommon.NotReadyClusterManifestLocation) == 0)
                {
                    TextLogger.LogWarning("Bootstrap cluster manifest is not ready. Continue to discovery cluster topology after bootstrap cluster manifest is updated.");

                    return false;
                }

                if (!this.FetchBootstrapClusterManifestFile())
                {
                    return false;
                }

                ClusterManifestType bootstrapClusterManifest;
                if (!this.TryReadClusterManifestFile(this.clusterManifestFile, out bootstrapClusterManifest))
                {
                    TextLogger.LogError("Failed to read bootstrap cluster manifest file.");

                    return false;
                }

                TopologyProviderType topologyProviderType = TopologyProvider.GetTopologyProviderTypeFromClusterManifest(bootstrapClusterManifest);

                TextLogger.LogInfo("Topology provider type : {0}.", topologyProviderType);

                if (topologyProviderType <= TopologyProviderType.ServiceRuntimeTopologyProvider)
                {
                    TextLogger.LogError("Topology provider type {0} is not supported for a Windows Fabric cluster that spans multiple Windows Azure cloud services.", topologyProviderType);

                    return false;
                }

                if (topologyProviderType == TopologyProviderType.StaticTopologyProvider)
                {
                    // Static topology provider defines the authoritative cluster topology in the cluster manifest.
                    // For emulator environment, IP addresses of all nodes have to be a loopback address.
                    // For non-emulator environment, if local VM is not part of the cluster topology specified in static topology provider section, a Windows Fabric node will not be started locally.
                    // During scale-up, bootstrap cluster manifest has to be upgraded to allow new Windows Fabric nodes to be started on the new VMs and join the ring. 
                    ClusterTopology staticTopologyProviderClusterTopology = ClusterTopology.GetClusterTopologyFromStaticTopologyProvider();

                    ClusterManifestTypeInfrastructureWindowsAzureStaticTopology staticTopologyProviderElement = TopologyProvider.GetStaticTopologyProviderElementFromClusterManifest(bootstrapClusterManifest);

                    if (staticTopologyProviderElement.NodeList == null || staticTopologyProviderElement.NodeList.Length == 0)
                    {
                        TextLogger.LogError("Static topology provider section of bootstrap cluster manifest does not specify topology of the Windows Fabric cluster.");

                        return false;
                    }

                    LogClusterTopology(staticTopologyProviderElement);

                    if (this.emulatorEnvironment)
                    {
                        FabricNodeType nonLocalNode = staticTopologyProviderElement.NodeList.FirstOrDefault(node => !Utilities.IsAddressLoopback(node.IPAddressOrFQDN));

                        if (nonLocalNode != null)
                        {
                            TextLogger.LogError("Static topology provider section of bootstrap cluster manifest indicates that a non-local machine is part of the Windows Fabric cluster. This is not supported in emulator environment.");

                            return false;
                        }
                    }
                    else
                    {
                        FabricNodeType localNode = staticTopologyProviderElement.NodeList.FirstOrDefault(node => string.Compare(node.IPAddressOrFQDN, staticTopologyProviderClusterTopology.CurrentNodeIPAddressOrFQDN, StringComparison.OrdinalIgnoreCase) == 0);

                        if (localNode == null)
                        {
                            TextLogger.LogWarning("Static topology provider section of bootstrap cluster manifest indicates that local machine is not part of the Windows Fabric cluster. A Windows Fabric node will not be configured and started locally.");

                            return false;
                        }
                    }

                    lock (this.serviceStateLock)
                    {
                        this.expectedTopology = staticTopologyProviderClusterTopology;

                        this.isExpectedTopologyRunning = false;
                    }
                }

                TextLogger.LogInfo("Successfully discovered cluster topology.");
            }
            catch (Exception e)
            {
                TextLogger.LogError("Exception occurred when discovering cluster topology : {0}", e);

                if (WindowsFabricAzureWrapperServiceCommon.IsFatalException(e))
                {
                    throw;
                }

                return false;
            }

            return true;
        }

        private bool ConfigureNode(ClusterTopology topologyToDeploy)
        {
            try
            {
                TextLogger.LogInfo("Configuring local Service Fabric node based on bootstrap cluster manifest.");

                LogClusterTopology(topologyToDeploy);

                if (!this.InvokeFabricDeployer(topologyToDeploy, DeploymentType.Create, false))
                {
                    TextLogger.LogError("Failed to configure local Service Fabric node based on bootstrap cluster manifest.");

                    return false;
                }

                if (!this.emulatorEnvironment)
                {
                    // Set NodeConfigurationCompleted registry key.
                    Utilities.SetWindowsFabricNodeConfigurationCompleted(false);
                }

                TextLogger.LogInfo("Successfully configured local Service Fabric node based on bootstrap cluster manifest.");
            }
            catch (Exception e)
            {
                TextLogger.LogError("Exception occurred when configuring local Service Fabric node based on bootstrap cluster manifest : {0}", e);

                if (WindowsFabricAzureWrapperServiceCommon.IsFatalException(e))
                {
                    throw;
                }

                return false;
            }

            return true;
        }

        private bool StartNode()
        {
            try
            {
                TextLogger.LogInfo("Starting Fabric host to start local Windows Fabric node.");

                if (this.serviceState < WindowsFabricAzureWrapperState.ServiceStarted)
                {
                    return true;
                }

                ServiceController fabricInstallerSvc;
                if (this.emulatorEnvironment)
                {
                    if (!this.StartFabricHostProcess(this.FabricHostProcessExited))
                    {
                        TextLogger.LogError("Failed to start Fabric Host process.");

                        return false;
                    }
                }
                else if ((fabricInstallerSvc = WindowsFabricAzureWrapperServiceCommon.GetInstalledService(Constants.FabricInstallerServiceName)) != null)
                {
                    TextLogger.LogInfo("FabricInstallerService is present; starting node via XCopy path.");
                    if (!this.StopFabricInstallerService())
                    {
                        TextLogger.LogError("FabricInstallerSvc could not be stopped.");
                        return false;
                    }

                    TextLogger.LogInfo("Starting Fabric Installer service.");

                    if (!this.StartAndValidateInstallerServiceCompletion())
                    {
                        TextLogger.LogError("Running FabricInstallerSvc didn't bring up FabricHost.");

                        ServiceController fabricHost = WindowsFabricAzureWrapperServiceCommon.GetInstalledService(Constants.FabricHostServiceName);

                        if (fabricHost == null)
                        {
                            TextLogger.LogError("FabricInstallerService failed to install FabricHostService.");
                            return false;
                        }
                        else if (!this.StartFabricHostService())
                        {
                            TextLogger.LogError("Failed to start Fabric Host service.");
                            return false;
                        }
                    }
                }
                else
                {
                    ServiceController fabricHostService = WindowsFabricAzureWrapperServiceCommon.GetInstalledService(Constants.FabricHostServiceName);

                    if (fabricHostService.Status == ServiceControllerStatus.StartPending)
                    {
                        TextLogger.LogWarning("Fabric host service is starting.");

                        return false;
                    }
                    else if (fabricHostService.Status == ServiceControllerStatus.Running)
                    {
                        return true;
                    }

                    if (!this.StartFabricHostService())
                    {
                        TextLogger.LogError("Failed to start Fabric Host service.");

                        return false;
                    }
                }

                TextLogger.LogInfo("Successfully started Fabric host and local Windows Fabric node.");
            }
            catch (Exception e)
            {
                TextLogger.LogError("Exception occurred when starting Fabric host to start local Windows Fabric node : {0}", e);

                if (WindowsFabricAzureWrapperServiceCommon.IsFatalException(e))
                {
                    throw;
                }

                return false;
            }

            return true;
        }

        public bool UpdateNode(ClusterTopology topologyToDeploy)
        {
            try
            {
                TextLogger.LogInfo("Updating local Windows Fabric node with latest topology.");

                LogClusterTopology(topologyToDeploy);

                if (this.emulatorEnvironment)
                {
                    TextLogger.LogInfo("Updating local Windows Fabric node based on latest topology and bootstrap cluster manifest.");

                    if (!this.InvokeFabricDeployer(topologyToDeploy, DeploymentType.Update, false))
                    {
                        TextLogger.LogError("Failed to update local Windows Fabric node based on latest topology and bootstrap cluster manifest.");

                        return false;
                    }
                }
                else
                {
                    if (CurrentClusterManifestFileExists(topologyToDeploy.CurrentNodeName))
                    {
                        TextLogger.LogInfo("Updating local Windows Fabric node based on latest topology and current cluster manifest.");

                        if (!this.InvokeFabricDeployer(topologyToDeploy, DeploymentType.Update, true))
                        {
                            TextLogger.LogError("Failed to update local Windows Fabric node based on latest topology and current cluster manifest.");

                            return false;
                        }
                    }
                    else
                    {
                        TextLogger.LogWarning("Local Windows Fabric node has been configured and Fabric host has been started while current cluster manifest has not been generated yet. Retry later.");

                        return false;
                    }
                }

                TextLogger.LogInfo("Successfully updated local Windows Fabric node with latest topology.");
            }
            catch (Exception e)
            {
                TextLogger.LogError("Exception occurred when updating local Windows Fabric node with latest topology : {0}", e);

                if (WindowsFabricAzureWrapperServiceCommon.IsFatalException(e))
                {
                    throw;
                }

                return false;
            }

            return true;
        }

        // Generate SeedInfo and NodeList. Download the latest cluster manifest file if necessary. Then invoke Fabric Deployer.         
        private bool InvokeFabricDeployer(ClusterTopology topologyToDeploy, DeploymentType deploymentType, bool toUseCurrentClusterManifestForDeployment)
        {
            TextLogger.LogInfo("Invoke FabricDeployer with deployment type {0} and toUseCurrentClusterManifestForDeployment = {1}.", deploymentType, toUseCurrentClusterManifestForDeployment);

            if (toUseCurrentClusterManifestForDeployment)
            {
                string currentClusterManifestFile = GetCurrentClusterManifestFile(topologyToDeploy.CurrentNodeName);

                if (File.Exists(currentClusterManifestFile))
                {
                    TextLogger.LogInfo("Using current cluster manifest at {0} for deployment.", currentClusterManifestFile);

                    this.clusterManifestLocation = currentClusterManifestFile;
                    this.clusterManifestFile = currentClusterManifestFile;
                }
                else
                {
                    TextLogger.LogError("Current cluster manifest does not exist when toUseCurrentClusterManifestForDeployment = true.");

                    return false;
                }
            }
            else
            {
                if (!this.FetchBootstrapClusterManifestFile())
                {
                    return false;
                }
            }

            if (!this.FabricDeploy(deploymentType, this.clusterManifestFile, localNodeConfiguration.DataRootDirectory, topologyToDeploy.WindowsFabricNodes, true))
            {
                TextLogger.LogError(
                    "Failed when invoking FabricDeployer with deployment type = {0}, cluster manifest location = {1}, cluster manifest file = {2}, data root directory = {3}.",
                    deploymentType,
                    this.clusterManifestLocation,
                    this.clusterManifestFile,
                    localNodeConfiguration.DataRootDirectory);

                return false;
            }

            return true;
        }

        private void InitializeDirectories(LocalNodeConfiguration localNodeConfiguration)
        {
            Directory.CreateDirectory(localNodeConfiguration.DataRootDirectory);
            Directory.CreateDirectory(localNodeConfiguration.DeploymentRootDirectory);
        }

        private bool IsWindowsFabricV1Installed(string installedWindowsFabricVersion)
        {
            return !string.IsNullOrEmpty(installedWindowsFabricVersion) && installedWindowsFabricVersion.StartsWith(Constants.RegKeyV1Prefix, StringComparison.OrdinalIgnoreCase);
        }

        private string GetMsiInstallationArguments(string msiInstallationLogDirectory)
        {
            string msiInstallationLogFileName =
                string.Format(
                    CultureInfo.InvariantCulture,
                    WindowsFabricAzureWrapperServiceCommon.MsiInstallationLogFileNameTemplate,
                    DateTime.Now.Ticks);

            string msiInstallationLogFileFullPath =
                Path.Combine(
                    msiInstallationLogDirectory,
                    msiInstallationLogFileName);

            string msiPackageFile =
                Path.Combine(
                    this.pluginDirectory,
                    WindowsFabricAzureWrapperServiceCommon.DefaultWindowsFabricInstallationPackageName);

            string msiInstallationArguments =
                string.Format(
                    CultureInfo.InvariantCulture,
                    WindowsFabricAzureWrapperServiceCommon.MsiInstallationArgumentTemplate,
                    msiPackageFile,
                    msiInstallationLogFileFullPath);

            TextLogger.LogInfo("MSI installation arguments : {0}", msiInstallationArguments);

            return msiInstallationArguments;
        }


        private bool FetchBootstrapClusterManifestFile()
        {
            if (string.Compare(this.clusterManifestLocation, localNodeConfiguration.BootstrapClusterManifestLocation, StringComparison.OrdinalIgnoreCase) != 0 ||
                this.clusterManifestFile == null ||
                !File.Exists(this.clusterManifestFile))
            {
                string clusterManifestLocationToFetch = localNodeConfiguration.BootstrapClusterManifestLocation;
                string clusterManifestFileToFetch;

                if (!this.FetchClusterManifestFile(clusterManifestLocationToFetch, out clusterManifestFileToFetch))
                {
                    TextLogger.LogError("Failed to fetch bootstrap cluster manifest file.");

                    return false;
                }

                this.clusterManifestLocation = clusterManifestLocationToFetch;
                this.clusterManifestFile = clusterManifestFileToFetch;
            }

            return true;
        }

        // Fetch cluster manifest file from the location specified in configuration and add role endpoints information.
        // If the location is the default location, use the file in service binary directory.
        // If the location is in XStore, download it to a temporary local file in deployment root directory. 
        private bool FetchClusterManifestFile(string clusterManifestLocation, out string clusterManifestFileToReturn)
        {
            clusterManifestFileToReturn = null;

            try
            {
                string temporaryClusterManifestFileName =
                     string.Format(
                         CultureInfo.InvariantCulture,
                         WindowsFabricAzureWrapperServiceCommon.DownloadedClusterManifestFileNameTemplate,
                         DateTime.Now.Ticks);

                clusterManifestFileToReturn =
                    Path.Combine(
                        localNodeConfiguration.DeploymentRootDirectory,
                        temporaryClusterManifestFileName);

                if (clusterManifestLocation.Equals(WindowsFabricAzureWrapperServiceCommon.DefaultClusterManifestLocation, StringComparison.OrdinalIgnoreCase))
                {
                    string clusterManifestFileToRead = Path.Combine(this.pluginDirectory, WindowsFabricAzureWrapperServiceCommon.DefaultClusterManifestFileName);

                    TextLogger.LogInfo("Using default cluster manifest file : {0}", clusterManifestFileToRead);
                    File.Copy(clusterManifestFileToRead, clusterManifestFileToReturn, true);
                    if (!File.Exists(clusterManifestFileToRead))
                    {
                        TextLogger.LogError("Cluster manifest file {0} does not exist", clusterManifestFileToRead);

                        return false;
                    }
                }
                else
                {
                    string[] clusterManifestInfo = clusterManifestLocation.Split(';');
                    int clusterManifestInfoCount = clusterManifestInfo.Length;
                    string connectionString = String.Join(";", clusterManifestInfo, 0, clusterManifestInfoCount - 2);
                    string container = clusterManifestInfo[clusterManifestInfoCount - 2].Split('=')[1];
                    string file = clusterManifestInfo[clusterManifestInfoCount - 1].Split('=')[1];

                    TextLogger.LogInfo("Downloading cluster manifest file from Windows Azure Storage to {0}.", clusterManifestFileToReturn);

                    CloudBlobClient blobClient = CloudStorageAccount.Parse(connectionString).CreateCloudBlobClient();
                    blobClient.DefaultRequestOptions.RetryPolicy = new ExponentialRetry();
                    CloudBlobContainer blobContainer = blobClient.GetContainerReference(container);
                    CloudBlob blob = blobContainer.GetBlobReference(file);
                    blob.DownloadToFile(clusterManifestFileToReturn, FileMode.Create);
                }
            }
            catch (Exception e)
            {
                TextLogger.LogError("Exception occurred when fetching cluster manifest file : {0}", e);

                if (WindowsFabricAzureWrapperServiceCommon.IsFatalException(e))
                {
                    throw;
                }

                return false;
            }
            return true;
        }

        private bool TryReadClusterManifestFile(string clusterManifestFile, out ClusterManifestType clusterManifest)
        {
            clusterManifest = null;

            try
            {
                using (FileStream fileStream = File.Open(clusterManifestFile, FileMode.Open, FileAccess.Read, FileShare.Read))
                {
                    XmlReaderSettings xmlReaderSettings = new XmlReaderSettings();
                    xmlReaderSettings.ValidationType = ValidationType.None;
                    xmlReaderSettings.XmlResolver = null;

                    using (XmlReader xmlReader = XmlReader.Create(fileStream, xmlReaderSettings))
                    {
                        XmlSerializer serializer = new XmlSerializer(typeof(ClusterManifestType));

                        clusterManifest = (ClusterManifestType)serializer.Deserialize(xmlReader);
                    }
                }

                return true;
            }
            catch (Exception ex)
            {
                TextLogger.LogError("Exception occurred when reading cluster manifest file {0} : {1}", clusterManifestFile, ex);

                if (WindowsFabricAzureWrapperServiceCommon.IsFatalException(ex))
                {
                    throw;
                }
            }

            return false;
        }


        private void SetNextState(WindowsFabricAzureWrapperState nextState)
        {
            lock (this.serviceStateLock)
            {
                if (this.serviceState < WindowsFabricAzureWrapperState.ServiceStarted)
                {
                    TextLogger.LogVerbose("Service is stopping. Ignore state transition request to {0}", nextState);

                    return;
                }

                TextLogger.LogInfo("State transition: {0} -> {1}", this.serviceState, nextState);

                this.serviceState = nextState;
            }
        }

        private WindowsFabricAzureWrapperState GetCurrentState()
        {
            lock (this.serviceStateLock)
            {
                return this.serviceState;
            }
        }

        private void RegisterRoleEnvironmentNotifications()
        {
            if (this.subscribeBlastNotification)
            {
                RoleEnvironment.SimultaneousChanged += this.RoleEnvironmentChangedBlastNotification;
            }

            RoleEnvironment.Changed += this.RoleEnvironmentChangedRollingNotification;
            RoleEnvironment.StatusCheck += this.RoleEnvironmentStatusCheck;
            RoleEnvironment.Stopping += this.RoleEnvironmentStopping;
        }

        private void UnregisterRoleEnvironmentNotifications()
        {
            if (this.subscribeBlastNotification)
            {
                RoleEnvironment.SimultaneousChanged -= this.RoleEnvironmentChangedBlastNotification;
            }

            RoleEnvironment.Changed -= this.RoleEnvironmentChangedRollingNotification;
            RoleEnvironment.StatusCheck -= this.RoleEnvironmentStatusCheck;
            RoleEnvironment.Stopping -= this.RoleEnvironmentStopping;
        }

        private void RoleEnvironmentChangedRollingNotification(object sender, RoleEnvironmentChangedEventArgs e)
        {
            RoleEnvironmentChanged(false);
        }

        private void RoleEnvironmentChangedBlastNotification(object sender, SimultaneousChangedEventArgs e)
        {
            RoleEnvironmentChanged(true);
        }

        // In case of configuration changes, update the configuration expected to be running if latest configuration is valid.
        // The expected configuration would be picked up asynchronously.
        private void RoleEnvironmentChanged(bool isBlastNotification)
        {
            TextLogger.LogInfo("Windows Azure cloud service configuration has changed. Notification mode : {0}.", isBlastNotification ? "Blast" : "Rolling");

            LocalNodeConfiguration updatedLocalNodeConfiguration = LocalNodeConfiguration.GetLocalNodeConfiguration();

            Thread.MemoryBarrier();

            localNodeConfiguration = updatedLocalNodeConfiguration;

            LogLocalNodeConfiguration(localNodeConfiguration);

            TextLogger.SetTraceLevel(localNodeConfiguration.TextLogTraceLevel);

            if (!this.multipleCloudServiceDeployment)
            {
                ClusterTopology updatedTopology = null;

                try
                {
                    updatedTopology = ClusterTopology.GetClusterTopologyFromServiceRuntime(localNodeConfiguration);
                }
                catch (Exception ex)
                {
                    TextLogger.LogError("Exception occurred when retrieving updated topology : {0}", ex);

                    if (WindowsFabricAzureWrapperServiceCommon.IsFatalException(ex))
                    {
                        throw;
                    }

                    TextLogger.LogError("Updated topology is invalid. Keep running current topology.");

                    return;
                }

                lock (this.serviceStateLock)
                {
                    if (this.serviceState < WindowsFabricAzureWrapperState.ServiceStarted)
                    {
                        TextLogger.LogWarning("Windows Azure cloud service configuration has changed. Ignore it since Azure wrapper service is in stopping process.");

                        return;
                    }

                    this.expectedTopology = updatedTopology;

                    this.isExpectedTopologyRunning = this.deployedTopology == null ? false : !this.deployedTopology.IsClusterTopologyChanged(this.expectedTopology);

                    TextLogger.LogInfo(
                        "Cluster topology {0} changed and expected topology {1} deployed topology.",
                        this.isExpectedTopologyRunning ? "has not" : "has",
                        this.isExpectedTopologyRunning ? "is" : "is not");
                }
            }
        }

        // Report the role instance as busy if nothing has been deployed yet or the service(and probably the role instance) is shutting down.
        // Schedule a deployment creation/update if nothing is running or the running configuration is not the expected configuration. 
        // If another deployment is in progress, hold scheduling for this check.  
        // If last deployment failed, hold scheduling until a retry interval has passed.
        // TODO: Refer to #1282639, this will be updated once we have a way to query Fabric Host about whether hosted application/services are actually ready.
        private void RoleEnvironmentStatusCheck(object sender, RoleInstanceStatusCheckEventArgs e)
        {
            bool toStartStateTransition = false;

            lock (this.serviceStateLock)
            {
                if (this.serviceState < WindowsFabricAzureWrapperState.NodeStarted)
                {
                    e.SetBusy();

                    if (this.serviceState < WindowsFabricAzureWrapperState.ServiceStarted)
                    {
                        return;
                    }
                }

                double retryIntervalSeconds = localNodeConfiguration.DeploymentRetryIntervalSeconds;

                TextLogger.LogVerbose("RoleEnvironmentStatusCheck : Current service state : {0}. Last deployment action failure : {1}. Retry interval in seconds : {2}. ComputingStateTransition : {3}. IsExpectedTopologyRunning : {4}.",
                    this.serviceState,
                    this.lastFailedDeploymentAction,
                    retryIntervalSeconds,
                    this.computingStateTransition,
                    this.isExpectedTopologyRunning);

                if (!this.computingStateTransition &&
                    (((this.serviceState > WindowsFabricAzureWrapperState.ServiceStarted && this.serviceState < WindowsFabricAzureWrapperState.DiscoveringTopology) || (this.serviceState > WindowsFabricAzureWrapperState.DiscoveringTopology && this.serviceState < WindowsFabricAzureWrapperState.NodeStarted)) ||
                     (this.serviceState == WindowsFabricAzureWrapperState.DiscoveringTopology && string.Compare(localNodeConfiguration.BootstrapClusterManifestLocation, WindowsFabricAzureWrapperServiceCommon.NotReadyClusterManifestLocation, StringComparison.OrdinalIgnoreCase) != 0 && string.Compare(localNodeConfiguration.BootstrapClusterManifestLocation, this.clusterManifestLocation, StringComparison.OrdinalIgnoreCase) != 0) ||
                     (this.serviceState == WindowsFabricAzureWrapperState.NodeStarted && !this.isExpectedTopologyRunning)) &&
                    (DateTime.UtcNow - this.lastFailedDeploymentAction).TotalSeconds > retryIntervalSeconds)
                {
                    TextLogger.LogInfo(
                        "Start asynchronous deployment state transition. Current service state : {0}. Last deployment action failure : {1}. Retry interval in seconds : {2}.",
                        this.serviceState,
                        this.lastFailedDeploymentAction,
                        retryIntervalSeconds);

                    this.computingStateTransition = true;

                    toStartStateTransition = true;
                }
            }

            if (toStartStateTransition)
            {
                ThreadPool.QueueUserWorkItem(StateTransitionCallback);
            }
        }

        private void RoleEnvironmentStopping(object sender, RoleEnvironmentStoppingEventArgs e)
        {
            TextLogger.LogInfo("The role instance is stopping. Stopping the service.");

            if (this.emulatorEnvironment)
            {
                this.OnStop();
            }

            this.stopServiceCallback();
        }

        private void FabricHostProcessExited(object sender, EventArgs e)
        {
            TextLogger.LogInfo("Fabric host process has exited.");

            this.StopFabricHostProcess();

            lock (this.serviceStateLock)
            {
                this.serviceState = WindowsFabricAzureWrapperState.StartingNode;

                this.lastFailedDeploymentAction = DateTime.UtcNow;
            }
        }
    }
}
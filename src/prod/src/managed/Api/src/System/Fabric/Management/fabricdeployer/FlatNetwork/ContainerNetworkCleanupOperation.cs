// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using Management.FabricDeployer;
    using Management.ServiceModel;
    using NetFwTypeLib;
    using Newtonsoft.Json;
    using System.Fabric.Management.Common;
    using System.Net.Http;

    internal class ContainerNetworkCleanupOperation : ContainerNetworkDeploymentOperation
    {
        protected override void OnExecuteOperation(DeploymentParameters parameters, ClusterManifestType clusterManifest, Infrastructure infrastructure)
        {
            var containerNetworkName = (parameters != null) ? (parameters.ContainerNetworkName) : (FlatNetworkConstants.NetworkName);
            var operation = (parameters != null) ? (parameters.Operation) : (DeploymentOperations.None);

            var containerServiceArguments = (parameters != null && parameters.UseContainerServiceArguments) 
                ? (parameters.ContainerServiceArguments) : (FlatNetworkConstants.ContainerServiceArguments);
#if !DotNetCoreClrLinux
            containerServiceArguments = (parameters != null && parameters.EnableContainerServiceDebugMode) 
                ? (string.Format("{0} {1}", containerServiceArguments, FlatNetworkConstants.ContainerProviderServiceDebugModeArg))
                : containerServiceArguments;
#endif

            var fabricDataRoot = (parameters != null) ? (parameters.FabricDataRoot) : (string.Empty);

            ExecuteOperation(containerNetworkName, containerServiceArguments, fabricDataRoot, operation);
        }

        internal void ExecuteOperation(string containerNetworkName, string containerServiceArguments, string fabricDataRoot, DeploymentOperations operation = DeploymentOperations.None)
        {
            if (operation == DeploymentOperations.UpdateInstanceId)
            {
                DeployerTrace.WriteInfo("Skipping Container network clean up invoked in context: {0}", operation);
                return;
            }

            if (!Utility.IsContainersFeaturePresent())
            {
                DeployerTrace.WriteInfo("Skipping container network clean up since containers feature has been disabled.");
                return;
            }

#if !DotNetCoreClrLinux
           if(!Utility.IsContainerNTServicePresent())
           {
                DeployerTrace.WriteInfo("Skipping container network clean up since containers NT service is not installed.");
                return;
           }
#endif

            // Wire server check is being made for the edge cluster scenario,
            // so that clean up is a noop. Wire server is not explicitly needed
            // for the clean up process.
            if (!IsWireServerAvailable())
            {
                DeployerTrace.WriteInfo("Skipping container network clean up since wire server is not available.");
                return;
            }

            containerNetworkName = (!string.IsNullOrEmpty(containerNetworkName)) ? (containerNetworkName) : (FlatNetworkConstants.NetworkName);
            containerServiceArguments = (!string.IsNullOrEmpty(containerServiceArguments)) ? (containerServiceArguments) : (FlatNetworkConstants.ContainerServiceArguments);

            if (string.IsNullOrEmpty(fabricDataRoot))
            {
                fabricDataRoot = Utility.GetFabricDataRoot();
                DeployerTrace.WriteInfo("Fabric data root passed in was empty, using environment fabric data root instead: {0}.", fabricDataRoot);
            }

            DeployerTrace.WriteInfo("Container network clean up invoked in context: {0}", operation);

            bool containerServiceRunning = false;
            if (operation == DeploymentOperations.Update || operation == DeploymentOperations.UpdateInstanceId)
            {
                containerServiceRunning = IsContainerServiceRunning();
                DeployerTrace.WriteInfo("Container service running: {0}", containerServiceRunning);
            }

            if (CleanupContainerNetwork(containerNetworkName, containerServiceArguments, fabricDataRoot, containerServiceRunning))
            {
                DeployerTrace.WriteInfo("Successfully cleaned up container network.");
            }
            else
            {
                string message = "Failed to clean up container network.";
                DeployerTrace.WriteError(message);
                throw new InvalidDeploymentException(message);
            }
        }

#if DotNetCoreClrLinux
        private bool CleanupContainerNetwork(string networkName, string containerServiceArguments, string fabricDataRoot, bool containerServiceRunning)
        {
            bool success = false;

            bool containerServiceStopped;
            if (StopContainerService(containerServiceRunning, out containerServiceStopped))
            {
                if (StartContainerProcess(containerServiceArguments, fabricDataRoot, containerServiceRunning))
                {
                    if (RemoveNetwork(networkName))
                    {
                        if (UninstallNetworkDriver())
                        {
                            if (RemoveIptableRule())
                            {
                                success = true;
                            }
                        }
                    }
                }
            }

            bool cleanUpSuccess = false;
            if (StopContainerProcess(fabricDataRoot, containerServiceRunning) && StartContainerService(containerServiceRunning, containerServiceStopped))
            {
                cleanUpSuccess = true;
            }

            success = success && cleanUpSuccess;

            return success;
        }
#else
        private bool CleanupContainerNetwork(string networkName, string containerServiceArguments, string fabricDataRoot, bool containerServiceRunning)
        {
            bool success = false;

            bool containerNTServiceStopped;
            if (StopContainerNTService(containerServiceRunning, out containerNTServiceStopped))
            {
                if (StartContainerProviderService(containerServiceArguments, fabricDataRoot, containerServiceRunning))
                {
                    string subnet = string.Empty;
                    string gateway = string.Empty;
                    string macAddress = string.Empty;
                    if (Utility.RetrieveNMAgentInterfaceInfo(true, out subnet, out gateway, out macAddress))
                    {
                        var beforeIpForwardTable = default(NativeMethods.IPFORWARDTABLE);
                        if (RetrieveIPv4RoutingTable(out beforeIpForwardTable))
                        {
                            if (RemoveNetwork(networkName))
                            {
                                var afterIpForwardTable = default(NativeMethods.IPFORWARDTABLE);
                                if (RetrieveIPv4RoutingTable(out afterIpForwardTable))
                                {
                                    if (RestoreIPv4RoutingTable(beforeIpForwardTable, afterIpForwardTable, subnet))
                                    {
                                        if (RemoveFirewallRule())
                                        {
                                            success = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            bool cleanUpSuccess = false;
            if (StopContainerProviderService(fabricDataRoot, containerServiceRunning) && StartContainerNTService(containerServiceRunning, containerNTServiceStopped))
            {
                cleanUpSuccess = true;
            }

            success = success && cleanUpSuccess;

            return success;
        }
#endif
    }
}
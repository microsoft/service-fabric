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
            ExecuteOperation(containerNetworkName, operation);
        }

        internal void ExecuteOperation(string containerNetworkName, DeploymentOperations operation = DeploymentOperations.None)
        {
            if (operation == DeploymentOperations.UpdateInstanceId)
            {
                DeployerTrace.WriteInfo("Skipping Container network clean up invoked in context: {0}", operation);
                return;
            }

            if (Utility.IsContainersFeaturePresent())
            {
                // Wire server check is being made for the edge cluster scenario,
                // so that clean up is a noop. Wire server is not explicitly needed
                // for the clean up process.
                if (IsWireServerAvailable())
                {
                    var networkName = (!string.IsNullOrEmpty(containerNetworkName)) ? (containerNetworkName) : (FlatNetworkConstants.NetworkName);

                    DeployerTrace.WriteInfo("Container network clean up invoked in context: {0}", operation);

                    bool containerServiceRunningOnCustomPort = false;
                    if (operation == DeploymentOperations.Update || operation == DeploymentOperations.UpdateInstanceId)
                    {
                        containerServiceRunningOnCustomPort = IsContainerServiceRunning();
                        DeployerTrace.WriteInfo("Container service running on custom port: {0}", containerServiceRunningOnCustomPort);
                    }

                    if (CleanupContainerNetwork(networkName, containerServiceRunningOnCustomPort))
                    {
                        DeployerTrace.WriteInfo("Successfully cleaned up container network.");
                    }
                    else
                    {
                        DeployerTrace.WriteError("Failed to clean up container network.");
                        throw new InvalidDeploymentException("Failed to clean up container network.");
                    }
                }
                else
                {
                    DeployerTrace.WriteInfo("Skipping container network clean up since wire server is not available.");
                }
            }
            else
            {
                DeployerTrace.WriteInfo("Skipping container network clean up since containers feature has been disabled.");
            }
        }

#if DotNetCoreClrLinux
        private bool CleanupContainerNetwork(string networkName, bool containerServiceRunningOnCustomPort)
        {
            bool success = false;

            bool containerServiceStopped;
            if (StopContainerService(containerServiceRunningOnCustomPort, out containerServiceStopped))
            {
                if (StartContainerProcess(containerServiceRunningOnCustomPort))
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
            if (StopContainerProcess(containerServiceRunningOnCustomPort) && StartContainerService(containerServiceRunningOnCustomPort, containerServiceStopped))
            {
                cleanUpSuccess = true;
            }

            success = success && cleanUpSuccess;

            return success;
        }
#else
        private bool CleanupContainerNetwork(string networkName, bool containerServiceRunningOnCustomPort)
        {
            bool success = false;

            bool containerNTServiceStopped;
            if (StopContainerNTService(containerServiceRunningOnCustomPort, out containerNTServiceStopped))
            {
                if (StartContainerProviderService(containerServiceRunningOnCustomPort))
                {
                    string subnet = string.Empty;
                    string gateway = string.Empty;
                    if (Utility.RetrieveGatewayAndSubnet(true, out subnet, out gateway))
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
            if (StopContainerProviderService(containerServiceRunningOnCustomPort) && StartContainerNTService(containerServiceRunningOnCustomPort, containerNTServiceStopped))
            {
                cleanUpSuccess = true;
            }

            success = success && cleanUpSuccess;

            return success;
        }
#endif
    }
}
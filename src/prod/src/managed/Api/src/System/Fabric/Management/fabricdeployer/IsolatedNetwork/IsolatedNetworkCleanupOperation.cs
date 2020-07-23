// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using Microsoft.Win32;
using System.Collections.Generic;
using System.Fabric.Management.FabricDeployer;
using System.Fabric.Management.ServiceModel;
using System.IO;

namespace System.Fabric.FabricDeployer
{
    /// <summary>
    /// Clean up of isolated networks needs the following steps -
    /// 1) Start the cns plugin.
    /// 3) Remove all isolated networks on the node using the cns plugin.
    /// 4) Remove the recorded network interface used to set up the service fabric isolated network.
    /// 5) Stop the cns plugin. 
    /// </summary>
    internal class IsolatedNetworkCleanupOperation : IsolatedNetworkDeploymentOperation
    {
        protected override void OnExecuteOperation(DeploymentParameters parameters, ClusterManifestType clusteManifest, Infrastructure infrastructure)
        {
            var isolatedNetworkName = (parameters != null) ? (parameters.IsolatedNetworkName) : (IsolatedNetworkConstants.NetworkName);
            var operation = (parameters != null) ? (parameters.Operation) : (DeploymentOperations.None);
            var fabricBinRoot = (parameters != null) ? (parameters.FabricBinRoot) : (string.Empty);
            ExecuteOperation(isolatedNetworkName, fabricBinRoot, operation);
        }

        internal void ExecuteOperation(string isolatedNetworkName, string fabricBinRoot, DeploymentOperations operation = DeploymentOperations.None)
        {
            if (operation == DeploymentOperations.UpdateInstanceId)
            {
                DeployerTrace.WriteInfo("Skipping isolated network clean up invoked in context: {0}.", operation);
                return;
            }

            if (!Utility.IsContainersFeaturePresent())
            {
                DeployerTrace.WriteInfo("Skipping isolated network clean up since containers feature has been disabled.");
                return;
            }

#if !DotNetCoreClrLinux
            if (!Utility.IsContainerNTServicePresent())
            {
                DeployerTrace.WriteInfo("Skipping isolated network clean up since containers NT service is not installed.");
                return;
            }
#endif
            var networkName = (!string.IsNullOrEmpty(isolatedNetworkName)) ? (isolatedNetworkName) : (IsolatedNetworkConstants.NetworkName);

            if (string.IsNullOrEmpty(fabricBinRoot))
            {
                fabricBinRoot = Utility.GetFabricCodePath();
                DeployerTrace.WriteInfo("Fabric bin root was empty, using fabric code path instead: {0}.", fabricBinRoot);
            }

            DeployerTrace.WriteInfo("Isolated network clean up invoked in context: {0}.", operation);

#if !DotNetCoreClr
                bool networkPluginRunning = false;
                if (operation == DeploymentOperations.Update)
                {
                    networkPluginRunning = IsNetworkPluginRunning();
                    DeployerTrace.WriteInfo("Isolated network plugin running: {0}.", networkPluginRunning);
                }

                if (CleanupIsolatedNetwork(networkName, fabricBinRoot, networkPluginRunning))
#else
            if (CleanupIsolatedNetwork())
#endif
            {
                DeployerTrace.WriteInfo("Successfully cleaned up isolated network.");
            }
            else
            {
                string message = "Failed to clean up isolated network.";
                DeployerTrace.WriteError(message);
                throw new InvalidDeploymentException(message);
            }
        }

#if !DotNetCoreClr
        private bool CleanupIsolatedNetwork(string networkName, string fabricBinRoot, bool networkPluginRunning)
        {
            bool success = false;

            if (StartNetworkPlugin(fabricBinRoot, networkPluginRunning))
            {
                if (RemoveNetwork(networkName))
                {
                    var dataToBeRemoved = new List<string>();
                    dataToBeRemoved.Add(IsolatedNetworkConstants.IsolatedNetworkInterfaceNameRegValue);
                    Utility.RemoveRegistryKeyValues(Registry.LocalMachine, FabricConstants.FabricRegistryKeyPath, dataToBeRemoved);

                    success = true;
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
#else
        private bool CleanupIsolatedNetwork()
        {
            bool success = false;

            string path = Path.Combine(Constants.FabricEtcConfigPath, IsolatedNetworkConstants.IsolatedNetworkInterfaceNameRegValue);

            try
            {
                File.Delete(path);
                success = true;
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to delete the isolated network interface name file at path {0} exception {1}.", path, ex);
            }

            return success;
        }
#endif
    }
}

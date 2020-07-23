// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using Newtonsoft.Json;
using System.Diagnostics;
using System.Dynamic;
using System.Linq;
using System.Net.Http;
using System.Text;

namespace System.Fabric.FabricDeployer
{
    internal abstract class IsolatedNetworkDeploymentOperation : DeploymentOperation
    {
        #region Protected Members

        protected HttpClient httpClient = new HttpClient();

        #endregion

        /// <summary>
        /// Get network details, if it exists.
        /// </summary>
        /// <param name="networkName"></param>
        /// <param name="networkId"></param>
        /// <returns></returns>
        protected internal bool GetNetwork(string networkName, out string networkId)
        {
            bool success = false;
            networkId = string.Empty;

            DeployerTrace.WriteInfo("Getting network id for network Name:{0}.", networkName);

            try
            {
                var task = httpClient.GetAsync(IsolatedNetworkConstants.NetworkGetUrl);
                var httpResponse = task.GetAwaiter().GetResult().EnsureSuccessStatusCode();
                var response = httpResponse.Content.ReadAsStringAsync().GetAwaiter().GetResult();

                dynamic res = JsonConvert.DeserializeObject<ExpandoObject>(response);
                string dataStr = JsonConvert.SerializeObject(res.Data);
                var networks = JsonConvert.DeserializeObject<IsolatedNetworkConfig[]>(dataStr);
                if (networks.Length > 0)
                {
                    var network = networks.SingleOrDefault(n => n.Name == networkName);
                    if (network != null)
                    {
                        networkId = network.Id;
                    }
                }

                success = true;
                DeployerTrace.WriteInfo("Retrieved network id:{0}.", networkId);
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to get network details for network Name:{0} exception {1}.", networkName, ex);
            }

            return success;
        }

        /// <summary>
        /// Removes isolated network.
        /// </summary>
        /// <param name="networkName"></param>
        /// <returns></returns>
        protected internal bool RemoveNetwork(string networkName)
        {
            bool success = false;

            DeployerTrace.WriteInfo("Removing network Name:{0}.", networkName);

            try
            {
                string networkId = string.Empty;
                if (GetNetwork(networkName, out networkId))
                {
                    if (!string.IsNullOrEmpty(networkId))
                    {
                        dynamic isolatedNetworkConfig = new ExpandoObject();
                        isolatedNetworkConfig.NetworkName = networkName;
                        isolatedNetworkConfig.CleanUpAllNetworks = true;

                        var isolatedNetworkConfigJsonString = JsonConvert.SerializeObject(isolatedNetworkConfig);
                        var content = new StringContent(isolatedNetworkConfigJsonString, Encoding.UTF8, "application/json");
                        var task = httpClient.PostAsync(IsolatedNetworkConstants.NetworkDeleteUrl, content);
                        var httpResponse = task.GetAwaiter().GetResult().EnsureSuccessStatusCode();
                        var response = httpResponse.Content.ReadAsStringAsync().GetAwaiter().GetResult();

                        // check status code on response to know if operation was really successful
                        dynamic removeNetworkResponse = JsonConvert.DeserializeObject<ExpandoObject>(response);
                        if (removeNetworkResponse.ReturnCode != 0)
                        {
                            DeployerTrace.WriteInfo("Delete network plugin call failed with exception {0}.", removeNetworkResponse.Message);
                            return success;
                        }
                    }
                }
                else
                {
                    DeployerTrace.WriteError("Failed to retrieve network id for network Name:{0}.", networkName);
                }

                success = true;
                DeployerTrace.WriteInfo("Removed network Name:{0}.", networkName);
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to remove network Name:{0} exception {1}.", networkName, ex);
            }

            return success;
        }

        /// <summary>
        /// Starts the isolated network plugin. This is the plugin used to create the 
        /// isolated network to which containers with IP and MAC are added. 
        /// If network plugin is already running, the call to start is a noop.
        /// </summary>
        /// <param name="fabricBinRoot"></param>
        /// <param name="networkPluginRunning"></param>
        /// <returns></returns>
        protected internal bool StartNetworkPlugin(string fabricBinRoot, bool networkPluginRunning)
        {
            bool success = false;

            if (networkPluginRunning)
            {
                return true;
            }

            DeployerTrace.WriteInfo("Starting the network plugin.");

            if (Utility.ExecuteCommand(IsolatedNetworkConstants.NetworkPluginProcessName, string.Empty, fabricBinRoot, false, null))
            {
                success = true;
            }
            else
            {
                DeployerTrace.WriteError("Failed to start network plugin {0}.", IsolatedNetworkConstants.NetworkPluginProcessName);
            }

            return success;
        }

        /// <summary>
        /// Checks if the network plugin is running.
        /// </summary>
        /// <returns></returns>
        protected bool IsNetworkPluginRunning()
        {
            bool isRunning = false;
            int processId = 0;
            if (Utility.IsProcessRunning(IsolatedNetworkConstants.NetworkPluginProcessName, out isRunning, out processId))
            {
                return isRunning;
            }
            else
            {
                DeployerTrace.WriteInfo("Failed to check if network plugin is running.");
                return false;
            }
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Fabric.VMMWrapper
{
    using System;
    using System.Net;
    using System.Linq;
    using System.Diagnostics;
    using System.Collections;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using InfrastructureWrapper;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Fabric.Query;
    using System.Net.Sockets;
    using System.Threading.Tasks;
    using System.Numerics;
    using System.ServiceProcess;

    internal class VMMWrapper : InfrastructureWrapper
    {
        private const string LogTag = "VMMInstaller";
        private const string InstallParameters = "/i WindowsFabric.msi /q IACCEPTEULA=Yes /lv install.log";
        private readonly string clusterManifestLocation;
        private string serviceVMComputerName;

        public VMMWrapper(string ServiceVMComputerName, string clusterManifest)
        {
            this.serviceVMComputerName = ServiceVMComputerName;
            this.clusterManifestLocation = clusterManifest;
        }

        public int OnStart()
        {
            Logger.LogInfo(LogTag, "VMMWrapper OnStart");
            var nodeList = VMMNodeList.Create(this.serviceVMComputerName);

            if (nodeList == null || nodeList.Count == 0)
            {
                Logger.LogError(LogTag, "The node list provided is invalid.");
                return (int)ErrorCode.InvalidNodeList;
            }

            if (!this.FabricInstall(InstallParameters))
            {
                Logger.LogError(LogTag, "Unable to install fabric.");
                return (int)ErrorCode.FabricInstallFailed;
            }

            if (!this.FabricDeploy(DeploymentType.Create, this.clusterManifestLocation, null, nodeList))
            {
                Logger.LogError(LogTag, "Unable to deploy fabric");
                return (int)ErrorCode.FabricDeploymentFailed;
            }

            if (!this.StartFabricHostService())
            {
                Logger.LogError(LogTag, "Unable to start fabric host service.");
                return (int)ErrorCode.ServiceStartFailed;
            }

            return (int)ErrorCode.Success;
        }

        public static int OnVMRemove()
        {
            //step 1: create fabircClientLocal that connects to local node                        
            Logger.LogInfo(LogTag, "OnVMRemove: creating local FabricClient");
            FabricClient fabircClientLocal = null;
            for (int i = 1; i <= 5; i++)
            {
                try
                {
                    fabircClientLocal = new FabricClient();
                }
                catch (Exception e)
                {
                    if (i < 5)
                    {
                        Logger.LogInfo(LogTag, "creating local client throw execption: {0}. Retrying: {1}", e.ToString(), i);
                    }
                }
            }
            if (fabircClientLocal == null)
            {
                return (int)ErrorCode.Unexpected;
            }


            //step 2: query nodes in the cluster that is up, and use them to create fabricClient that actually sends infraStructureService Command.            
            var nodeResult = fabircClientLocal.QueryManager.GetNodeListAsync().Result;

            IPHostEntry host = Dns.GetHostEntry(Dns.GetHostName());
            string ipAddressLocal = (host.AddressList.First(ip => ip.AddressFamily == AddressFamily.InterNetwork)).ToString();
            string localNodeName = "";
            List<string> ipAddressesList = new List<string>();
            for (int idx = 0; idx < nodeResult.Count; idx++)
            {
                if (nodeResult[idx].NodeStatus != NodeStatus.Up)
                {
                    continue;
                }

                string[] result = nodeResult[idx].IpAddressOrFQDN.Split(':');
                string IpAddressOrFQDN = result[0];

                Logger.LogInfo(LogTag, "FQDN is {0}, ipAddressLocal is {1}, IpAddressOrFQDN is {2}", VMMNodeList.GetFQDN(), ipAddressLocal, IpAddressOrFQDN);
                if (VMMNodeList.GetFQDN().Equals(IpAddressOrFQDN, StringComparison.CurrentCultureIgnoreCase)
                    ||
                    ipAddressLocal.Equals(IpAddressOrFQDN, StringComparison.CurrentCultureIgnoreCase))
                {
                    localNodeName = nodeResult[idx].NodeName;
                }
                else
                {
                    Logger.LogInfo(LogTag, "node {0} address is {1}", idx, IpAddressOrFQDN);
                    ipAddressesList.Add(IpAddressOrFQDN + ":19000");  //todo:  query clientconnectionendpoints when available.
                }
            }
            Logger.LogInfo(LogTag, "localNodeName: {0}", localNodeName);

            string[] ipAddressesUp = new string[ipAddressesList.Count];
            ipAddressesList.CopyTo(ipAddressesUp, 0);
            FabricClient fabricClient = null;
            for (int i = 1; i <= 5; i++)
            {
                try
                {
                    fabricClient = new FabricClient(ipAddressesUp);
                }
                catch (Exception e)
                {
                    if (i < 5)
                    {
                        Logger.LogInfo(LogTag, "creating client (connecting to cluster) throw execption: {0}. Retrying: {1}", e.ToString(), i);
                    }
                }
            }
            if (fabricClient == null)
            {
                return (int)ErrorCode.Unexpected;
            }


            //step 3: send StartInfrastructureTask
            NodeTaskDescription nodeTaskDescription = new NodeTaskDescription();
            nodeTaskDescription.NodeName = localNodeName;
            nodeTaskDescription.TaskType = NodeTask.Remove;

            string taskId = "remove" + localNodeName;
            Int64 instanceId = DateTime.Now.Ticks;
            List<NodeTaskDescription> nodeTasks = new List<NodeTaskDescription>();
            nodeTasks.Add(nodeTaskDescription);
            ReadOnlyCollection<NodeTaskDescription> readonlyNodeTasks = new ReadOnlyCollection<NodeTaskDescription>(nodeTasks);
            InfrastructureTaskDescription infrastructureTaskDesp = new InfrastructureTaskDescription(taskId, instanceId, readonlyNodeTasks);

            bool taskCompleted = false;
            while (taskCompleted == false)
            {
                try
                {
                    Task<bool> taskStart = fabricClient.ClusterManager.StartInfrastructureTaskAsync(infrastructureTaskDesp);
                    taskCompleted = taskStart.Result;
                    if (taskCompleted == false)
                    {
                        Logger.LogInfo(LogTag, "StartInfrastructureTaskAsync returned false, sleep for 3 second and retry");
                        System.Threading.Thread.Sleep(3000);
                    }
                }
                catch (Exception e)
                {
                    Logger.LogInfo(LogTag, "StartInfrastructureTaskAsync throw execption: {0}. Retrying...", e.ToString());
                }
            }

            Logger.LogInfo(LogTag, "StartInfrastructureTaskAsync returned true");


            //step 4: kill local fabric.exe by stoping fabrichostsvc 
            ServiceController service = new ServiceController("FabricHostSvc");
            try
            {
                TimeSpan timeout = TimeSpan.FromMilliseconds(10000);
                service.Stop();
                service.WaitForStatus(ServiceControllerStatus.Stopped, timeout);
            }
            catch (Exception e)
            {
                Logger.LogInfo(LogTag, "failed to stop service with exception {0}", e.ToString());
            }


            //step 5: send FinishInfrastructureTask
            taskCompleted = false;
            while (taskCompleted == false)
            {
                try
                {
                    Task<bool> taskFinish = fabricClient.ClusterManager.FinishInfrastructureTaskAsync(taskId, instanceId);
                    taskCompleted = taskFinish.Result;
                    if (taskCompleted == false)
                    {
                        Logger.LogInfo(LogTag, "FinishInfrastructureTaskAsync returned false, sleep for 3 second and retry");
                        System.Threading.Thread.Sleep(3000);
                    }
                }
                catch (Exception e)
                {
                    Logger.LogInfo(LogTag, "StartInfrastructureTaskAsync throw execption: {0}. Retrying...", e.ToString());
                }
            }

            Logger.LogInfo(LogTag, "FinishInfrastructureTaskAsync returned true");

            return (int)ErrorCode.Success;
        }
    }
}
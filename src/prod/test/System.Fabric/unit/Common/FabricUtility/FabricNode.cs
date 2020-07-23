// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Common.FabricUtility
{
    using System;
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.Diagnostics;
    using System.IO;
    using System.Net;
    using System.Text;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    /// <summary>
    /// Class encapsulating a fabric node
    /// </summary>
    public class FabricNode
    {
        private const int PortsReservedForFabric = 10;

        private readonly FabricNodeParameters parameters;
        private Process fabricExeProcess;

        public FabricNode(FabricNodeParameters parameters)
        {
            Debug.Assert(parameters != null);
            Debug.Assert(!string.IsNullOrWhiteSpace(parameters.NodeId));
            Debug.Assert(!string.IsNullOrWhiteSpace(parameters.NodeRootPath));
            Debug.Assert(!string.IsNullOrWhiteSpace(parameters.ImageStoreConnectionString));
            
            this.parameters = parameters;

            using (this.GetLogContext("Constructor"))
            {
                LogHelper.Log("Creating directories");
                this.CreateDirectories();

                LogHelper.Log("Creating configuration");
                this.CreateConfiguration();                
            }
        }

        /// <summary>
        /// Root path for this node
        /// </summary>
        public string NodeRootPath
        {
            get { return this.parameters.NodeRootPath; }
        }

        /// <summary>
        /// The address to which a fabric client should connect
        /// </summary>
        public IPEndPoint ClientConnectionAddress { get; private set; }

        /// <summary>
        /// the work folder
        /// </summary>
        public string FabricWorkingDirectory
        {
            get
            {
                Debug.Assert(!string.IsNullOrWhiteSpace(this.NodeRootPath));
                return Path.Combine(this.NodeRootPath, "data");
            }
        }

        public string NodeId
        {
            get { return this.parameters.NodeId; }
        }

        public string NodeName
        {
            get { return this.parameters.NodeName; }
        }

        public Uri FaultDomain
        {
            get { return this.parameters.FaultDomainId; }
        }

        public string UpgradeDomain
        {
            get { return this.parameters.UpgradeDomainId; }
        }

        public int Pid
        {
            get
            {
                if (!this.IsFabricRunning)
                {
                    return -1;
                }

                return this.fabricExeProcess.Id;
            }
        }

        /// <summary>
        /// the folder in which the binaries are kept
        /// </summary>
        public string BinariesPath
        {
            get
            {
                Debug.Assert(!string.IsNullOrWhiteSpace(this.NodeRootPath));
                return Path.Combine(this.NodeRootPath, "binaries");
            }
        }

        public string ConfigPath
        {
            get
            {
                return Path.Combine(this.BinariesPath, "node.cfg");
            }
        }

        public bool IsFabricRunning
        {
            get
            {
                if (this.fabricExeProcess == null)
                {
                    return false;
                }

                return !this.fabricExeProcess.HasExited;
            }
        }

        /// <summary>
        /// Deploys windows fabric to this node
        /// </summary>
        /// <param name="binaries">all binaries, exe etc</param>
        public void DeployFabric(IEnumerable<string> binaries, IEnumerable<string> filesToDeployToNodeRoot, IEnumerable<string> directories)
        {
            using (this.GetLogContext("DeployFabric"))
            {
                FileUtility.CopyFilesToDirectory(binaries, this.BinariesPath);
                FileUtility.CopyFilesToDirectory(filesToDeployToNodeRoot, this.NodeRootPath);

                foreach (var dir in directories)
                {
                    FileUtility.CopyDirectory(dir, Path.Combine(this.BinariesPath, Path.GetFileName(dir)));
                }
            }
        }

        /// <summary>
        /// Starts fabric.exe 
        /// </summary>
        public void StartNode()
        {
            Debug.Assert(this.fabricExeProcess == null, "can't start more than once");

            using (this.GetLogContext("StartNode"))
            {
                ProcessStartInfo psi = new ProcessStartInfo()
                {
                    Arguments = "-c",
                    FileName = Path.Combine(this.BinariesPath, "fabric.exe"),
                    WorkingDirectory = this.BinariesPath,
                    UseShellExecute = false,
                };

                psi.EnvironmentVariables["FabricConfigFileName"] = this.ConfigPath;
                // Any random path should work.
                psi.EnvironmentVariables["FabricDataRoot"] = this.FabricWorkingDirectory;

                LogHelper.Log("Starting ...");
                this.fabricExeProcess = Process.Start(psi);
                LogHelper.Log("fabric.exe pid {0}", this.fabricExeProcess.Id);

            }
        }

        private static IPEndPoint GetNextAddress(IPEndPoint ep)
        {
            return new IPEndPoint(ep.Address, ep.Port + 1);
        }

        private static string GetVoters(IEnumerable<IPEndPoint> voters)
        {
            StringBuilder sb = new StringBuilder();
            int nodeId = 1;
            foreach (var item in voters)
            {
                sb.AppendLine(string.Format("{0} = SeedNode,{1}", nodeId++, item.ToString()));
            }

            return sb.ToString();
        }

        private static void AddRemoveFirewallException(string exePath, bool add)
        {
            ProcessStartInfo netsh = new ProcessStartInfo
            {
                FileName = Path.Combine(Environment.SystemDirectory, "netsh.exe"),
                WorkingDirectory = Environment.SystemDirectory,
                UseShellExecute = false,
            };

            if (add)
            {
                netsh.Arguments = String.Format("firewall add allowedprogram {0} {1} ENABLE", exePath, Path.GetFileNameWithoutExtension(exePath));
            }
            else
            {
                netsh.Arguments = String.Format("firewall delete allowedprogram {0}", exePath);
            }

            Process process = Process.Start(netsh);
            process.WaitForExit();

            Assert.AreEqual<int>(0, process.ExitCode);
        }

        private void CreateDirectories()
        {
            FileUtility.CreateDirectoryIfNotExists(this.NodeRootPath);
            FileUtility.CreateDirectoryIfNotExists(this.BinariesPath);
            FileUtility.CreateDirectoryIfNotExists(this.FabricWorkingDirectory);
        }

        private void CreateConfiguration()
        {
            var nodeAddress = this.parameters.Address;
            var leaseAgentAddress = FabricNode.GetNextAddress(nodeAddress);
            var runtimeServiceAddress = FabricNode.GetNextAddress(leaseAgentAddress);
            var clientConnectionAddress = FabricNode.GetNextAddress(runtimeServiceAddress);
            var gatewayIpcAddress = FabricNode.GetNextAddress(clientConnectionAddress);
            this.ClientConnectionAddress = clientConnectionAddress;

            string voters = FabricNode.GetVoters(this.parameters.Voters);

            var fabricConfigData = new NameValueCollection
            {
                {"WORKING_DIR", this.FabricWorkingDirectory},
                {"NODE_ID", this.parameters.NodeId},
                {"NODE_NAME", this.parameters.NodeName},
                {"NODE_ADDRESS", nodeAddress.ToString()},
                {"LEASE_AGENT_ADDRESS", leaseAgentAddress.ToString()},
                {"RUNTIME_SERVICE_ADDRESS", runtimeServiceAddress.ToString()},
                {"CLIENT_CONNECTION_ADDRESS", clientConnectionAddress.ToString()},
                {"GATEWAY_IPC_ADDRESS", gatewayIpcAddress.ToString()},
                {"IMAGE_STORE_CONNECTION_STRING", this.parameters.ImageStoreConnectionString},
                {"FEDERATION_DIR", Path.Combine(this.FabricWorkingDirectory, "federation")},
                {"VOTERS", voters},
                {"NAMING_SERVICE_DIR", Path.Combine(this.FabricWorkingDirectory, "communication")},
                {"FM_STORE_DIR", this.parameters.ReliabilityPath},
                {"RA_STORE_DIR", this.parameters.ReliabilityPath},
                {"ACTIVATION_CONFIG_DIR", this.BinariesPath},
                {"START_PORT", (this.parameters.StartPort + FabricNode.PortsReservedForFabric).ToString()},
                {"END_PORT", this.parameters.EndPort.ToString()},
                {"FAULT_DOMAIN", this.parameters.FaultDomainId.ToString()},
                {"UPGRADE_DOMAIN", this.parameters.UpgradeDomainId.ToString()},
            };

            string configText = EmbeddedResourceUtility.ReadTemplateAndApplyVariables("FabricConfig.cfg.template", fabricConfigData);
            File.WriteAllText(this.ConfigPath, configText);

            var hostConfigData = new NameValueCollection
            {
                {"NODE_ID", this.parameters.NodeId},
                {"CLIENT_CONNECTION_ADDRESS", clientConnectionAddress.ToString()},
                {"LEASE_AGENT_ADDRESS", leaseAgentAddress.ToString()},
                {"NODE_ADDRESS", nodeAddress.ToString()},
                {"RUNTIME_SERVICE_ADDRESS", runtimeServiceAddress.ToString()}
            };

            string hostConfigText = EmbeddedResourceUtility.ReadTemplateAndApplyVariables("HostConfig.ini.template", hostConfigData);
            File.WriteAllText(Path.Combine(this.BinariesPath, "hostconfig.ini"), hostConfigText);

            var clusterConfigData = new NameValueCollection
            {
                {"ACTIVATION_CONFIG_DIR", this.BinariesPath},
                {"IMAGE_STORE_CONNECTION_STRING", this.parameters.ImageStoreConnectionString},
                {"VOTERS", voters},
                {"WORKING_DIR", this.FabricWorkingDirectory},
            };

            string clusterConfigText = EmbeddedResourceUtility.ReadTemplateAndApplyVariables("ClusterConfig.ini.template", clusterConfigData);
            File.WriteAllText(Path.Combine(this.BinariesPath, "ClusterConfig.ini"), clusterConfigText);

            var activationConfigData = new NameValueCollection
            {
                
            };

            var activationConfigText = EmbeddedResourceUtility.ReadTemplateAndApplyVariables("cfg.template", activationConfigData);
            File.WriteAllText(Path.Combine(this.BinariesPath, ".cfg"), activationConfigText);
        }
        
        private IDisposable GetLogContext(string operationName)
        {
            Debug.Assert(this.parameters != null);
            return new LogHelper(string.Format("FabricNode: {0} - {1}", this.parameters.NodeId, operationName));
        }

        #region IDisposable Members

        public void Dispose()
        {
            if (this.fabricExeProcess != null)
            {
                LogHelper.Log("Cleaning up fabricExe: {0}", this.fabricExeProcess.Id);
                if (!this.fabricExeProcess.HasExited)
                {
                    LogHelper.Log("Cleaning up force close {0}", this.fabricExeProcess.Id);
                    this.fabricExeProcess.Kill();

                    LogHelper.Log("WaitForExit: {0}", this.fabricExeProcess.Id);
                    this.fabricExeProcess.WaitForExit();
                    LogHelper.Log("Completed kill of fabric.exe {0}", this.fabricExeProcess.Id);
                }

                this.fabricExeProcess.Close();
                this.fabricExeProcess = null;
            }
        }

        #endregion
    }
}
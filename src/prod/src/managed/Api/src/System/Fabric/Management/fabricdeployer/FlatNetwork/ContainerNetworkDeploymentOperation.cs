// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using Net.Http;
    using NetFwTypeLib;
    using Newtonsoft.Json;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.ServiceProcess;
    using System.Text;
    using System.Fabric.Management.Common;
    using System.Threading;
    using Xml.Serialization;
    using Xml;

#if !DotNetCoreClr
    using System.Management;
    using Net.NetworkInformation;
    using System.Net.Sockets;
    using System.Net;
    using System.Collections.Generic;
#endif

    using Microsoft.ServiceFabric.ContainerServiceClient;

    internal abstract class ContainerNetworkDeploymentOperation : DeploymentOperation
    {
        #region Protected Members

        protected ContainerServiceClient dockerClient;

#if DotNetCoreClrLinux
        protected HttpClient httpClient = new HttpClient();
#endif

        #endregion

        internal ContainerNetworkDeploymentOperation()
        {
#if DotNetCoreClrLinux
            var containerServiceAddress = "unix:///var/run/docker.sock";
#else
            var containerServiceAddress = "npipe://./pipe/docker_engine";
#endif

            this.dockerClient = new ContainerServiceClient(containerServiceAddress);
        }

#if DotNetCoreClrLinux

        protected enum ConnectivityDirection
        {
            Inbound = 1,
            Outbound = 2
        }

        /// <summary>
        /// Stops the docker service. This is done so that we can bring up docker on the provided container service arguments
        /// since this is the endpoint that Fabric Hosting is using for setting up containers.
        /// If docker container service is already running on the right container service arguments, return true.
        /// </summary>
        /// <param name="stopped"></param>
        /// <returns></returns>
        protected internal bool StopContainerService(bool containerServiceRunning, out bool stopped)
        {
            stopped = false;

            if (containerServiceRunning)
            {
                return true;
            }

            bool success = false;

            DeployerTrace.WriteInfo("Stopping docker container service.");

            try
            {
                int dockerSetup = NativeHelper.system(" [ -f /usr/bin/docker ]");
                if (dockerSetup != 0)
                {
                    DeployerTrace.WriteInfo("Docker not set up.");
                    return success;
                }

                var command = " [ -f /var/run/docker.pid ] && sudo service docker stop";
                int returnvalue = NativeHelper.system(command);
                if (returnvalue != 0)
                {
                    DeployerTrace.WriteInfo("Failed to execute command {0} return value {1}.", command, returnvalue);
                }
                else
                {
                    DeployerTrace.WriteInfo("Successfully executed command \"{0}\".", command);
                    stopped = true;
                }

                success = true;
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to stop docker container service exception {0}", ex);
            }

            return success;
        }

        /// <summary>
        /// Stops the docker process. This is done so that we can bring up docker on the provided container service arguments
        /// since this is the endpoint that Fabric Hosting is using for setting up containers.
        /// If docker container service is already running on the right container service arguments, return true.
        /// </summary>
        protected internal bool StopContainerProcess(string fabricDataRoot, bool containerServiceRunning)
        {
            if (containerServiceRunning)
            {
                return true;
            }

            bool success = false;

            DeployerTrace.WriteInfo("Stopping docker container process.");

            try
            {
                bool running = false;
                int processId = -1;
                IsProcessRunning(FlatNetworkConstants.ContainerProviderProcessName, out running, out processId);
                if (!running)
                {
                    DeployerTrace.WriteInfo("Skipping stop docker container process since it is not running.");
                    return true;
                }

                var command = string.Format("sudo kill {0}", processId);
                int returnvalue = NativeHelper.system(command);
                if (returnvalue != 0)
                {
                    DeployerTrace.WriteInfo("Failed to execute command {0} return value {1}", command, returnvalue);
                    return success;
                }

                DeployerTrace.WriteInfo("Successfully executed command \"{0}\".", command);

                // Allow the system to catch up with docker service not running
                for (int i = 0; i < Constants.ApiRetryAttempts; i++)
                {
                    bool processRunning = false;
                    int pid = -1;
                    IsProcessRunning(FlatNetworkConstants.ContainerProviderProcessName, out processRunning, out pid);
                    if (!processRunning)
                    {
                        DeployerTrace.WriteInfo("Successfully stopped container process.");
                        success = true;
                        break;
                    }
                    else
                    {
                        Thread.Sleep(Constants.ApiRetryIntervalMilliSeconds);
                    }
                }

                // Remove the pid file if it exists.
                if (success)
                {
                    // reset value to take pid file deletion result into account
                    success = false;
        
                    // docker process pid file directory
                    var dockerPidFileDirPath = Path.Combine(fabricDataRoot, FlatNetworkConstants.DockerProcessIdFileDirectory);

                    command = string.Format("sudo rm -rf {0}/*", dockerPidFileDirPath);
                    returnvalue = NativeHelper.system(command);
                    if (returnvalue != 0)
                    {
                        DeployerTrace.WriteInfo("Failed to execute command {0} return value {1}", command, returnvalue);
                    }
                    else
                    {
                        DeployerTrace.WriteInfo("Successfully executed command \"{0}\".", command);
                        success = true;
                    }
                }
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to stop docker container process exception {0}", ex);
            }

            return success;
        }

        /// <summary>
        /// Starts the docker container process using the provided container service arguments.
        /// This is the endpoint used to create the docker network.
        /// If docker container service is already running on the right container service arguments, return true.
        /// </summary>
        /// <returns></returns>
        protected internal bool StartContainerProcess(string containerServiceArguments, string fabricDataRoot, bool containerServiceRunning)
        {
            if (containerServiceRunning)
            {
                return true;
            }

            bool success = false;

            DeployerTrace.WriteInfo("Clean up docker process, if running.");

            if (!StopContainerProcess(fabricDataRoot, containerServiceRunning))
            {
                DeployerTrace.WriteInfo("Failed to clean up docker process.");
                return success;
            }

            DeployerTrace.WriteInfo("Starting docker process.");

            try
            {
                var dockerPidFileDirectory = Path.Combine(fabricDataRoot, FlatNetworkConstants.DockerProcessIdFileDirectory);
                if (!Directory.Exists(dockerPidFileDirectory))
                {
                    Directory.CreateDirectory(dockerPidFileDirectory);
                }

                var dockerPidFilePath = Path.Combine(dockerPidFileDirectory, string.Format("{0}_{1}", DateTime.Now.Ticks, FlatNetworkConstants.DockerProcessFile));

                var command = string.Format("sudo dockerd {0} --pidfile {1}&", containerServiceArguments, dockerPidFilePath);
                int returnvalue = NativeHelper.system(command);
                if (returnvalue != 0)
                {
                    DeployerTrace.WriteInfo("Failed to execute command {0} return value {1}.", command, returnvalue);
                    return success;  
                }

                DeployerTrace.WriteInfo("Successfully executed command \"{0}\".", command);

                // Allow the system to catch up after starting up docker service
                for (int i = 0; i < Constants.ApiRetryAttempts; i++)
                {
                    Thread.Sleep(Constants.ApiRetryIntervalMilliSeconds);

                    bool containerServiceReady = IsContainerServiceRunning();
                    if (containerServiceReady)
                    {
                        DeployerTrace.WriteInfo("Successfully started container process.");
                        success = true;
                        break;
                    }
                }
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to start container process exception {0}", ex);
            }

            return success;
        }

        /// <summary>
        /// Starts the default docker service if stopped by the deployer.
        /// This is simply restoring state on the machine after docker network has been set up.
        /// If docker container service is already running on the right container service arguments, return true.
        /// </summary>
        /// <returns></returns>
        protected internal bool StartContainerService(bool containerServiceRunning, bool containerServiceStopped)
        {
            if (containerServiceRunning)
            {
                return true;
            }

            bool success = false;

            DeployerTrace.WriteInfo("Starting docker container service.");

            try
            {
                if (containerServiceStopped)
                {
                    var command = "sudo service docker start";
                    int returnvalue = NativeHelper.system(command);
                    if (returnvalue != 0)
                    {
                        DeployerTrace.WriteInfo("Failed to execute command {0} return value {1}.", command, returnvalue);
                    }
                    else
                    {
                        DeployerTrace.WriteInfo("Successfully executed command \"{0}\".", command);
                        success = true;
                    }
                }
                else
                {
                    DeployerTrace.WriteInfo("Skipping start of docker container service since it was not stopped by deployer.");
                    success = true;
                }
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to start docker container service exception {0}.", ex);
            }

            return success;
        }

        protected bool IsPackageInstalled(string packageName)
        {
            var packageManagerType = System.Fabric.Common.FabricEnvironment.GetLinuxPackageManagerType();
            switch (packageManagerType)
            {
                case System.Fabric.Common.LinuxPackageManagerType.Deb:
                    return DoesDebPackageStatusMatch(packageName, FlatNetworkConstants.DpkgInstallStatus);
                case System.Fabric.Common.LinuxPackageManagerType.Rpm:
                    return IsRpmPackageInstalled(packageName);
                default:
                    string errorMessage = string.Format("Package checking not supported for this distro: {0}", packageManagerType);
                    DeployerTrace.WriteError(errorMessage);
                    throw new InvalidDeploymentException(errorMessage);
            }
        }

        protected bool DoesDebPackageStatusMatch(string packageName, string packageStatus)
        {
            bool statusMatch = false;

            DeployerTrace.WriteInfo("Checking if package status {0} for package {1} matches.", packageStatus, packageName);

            IntPtr pipePtr = IntPtr.Zero;
            try
            {
                var command = string.Format("dpkg --get-selections {0}", packageName);
                pipePtr = NativeHelper.popen(command, "r");
                if (pipePtr == IntPtr.Zero)
                {
                    DeployerTrace.WriteInfo("Failed to open stream for command {0}.", command);
                }
                else
                {
                    StringBuilder outputBuilder = new StringBuilder(1024);
                    IntPtr fPtr = NativeHelper.fgets(outputBuilder, 1024, pipePtr);
                    if (fPtr != IntPtr.Zero)
                    {
                        string sData = outputBuilder.ToString();
                        var sDataArray = sData.Split(new char[] { ' ', '\t' }, StringSplitOptions.RemoveEmptyEntries);
                        if (sDataArray.Length == 2)
                        {
                            if (string.Equals(sDataArray[0].Trim(), packageName, StringComparison.OrdinalIgnoreCase) &&
                                string.Equals(sDataArray[1].Trim(), packageStatus, StringComparison.OrdinalIgnoreCase))
                            {
                                statusMatch = true;
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to check status for package {0} status {1} exception {2}", packageName, packageStatus, ex);
            }
            finally
            {
                if (pipePtr != IntPtr.Zero)
                {
                    NativeHelper.pclose(pipePtr);
                }
            }

            return statusMatch;
        }

        protected bool IsRpmPackageInstalled(string packageName)
        {
            string command = string.Format("rpm -q {0}", packageName);
            int errorCode = NativeHelper.system(command);
            return errorCode == 0;
        }

        /// <summary>
        /// Determines if a process is running given the command used to start the process.
        /// </summary>
        protected internal void IsProcessRunning(string processCommand, out bool running, out int processId)
        {
            running = false;
            processId = -1;
            IntPtr pipePtr = IntPtr.Zero;

            try
            {
                var isProcessRunningCommand = string.Format("ps -C {0} --no-headers", processCommand);
                pipePtr = NativeHelper.popen(isProcessRunningCommand, "r");
                if (pipePtr == IntPtr.Zero)
                {
                    DeployerTrace.WriteInfo("Failed to open stream for command {0}.", isProcessRunningCommand);
                }
                else
                {
                    StringBuilder outputBuilder = new StringBuilder(1024);
                    IntPtr fPtr = NativeHelper.fgets(outputBuilder, 1024, pipePtr);
                    if (fPtr != IntPtr.Zero)
                    {
                        string sData = outputBuilder.ToString();
                        var sDataArray = sData.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
                        if (sDataArray.Length == 4)
                        {
                            running = true;
                            if (!Int32.TryParse(sDataArray[0], out processId))
                            {
                                DeployerTrace.WriteInfo("Failed to parse process id {0}.", sDataArray[0]);
                            }
                            else
                            {
                                DeployerTrace.WriteInfo("Process for command {0} running {1} process id {2}.", processCommand, running, processId);
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to check if process is running exception {0}", ex);
            }
            finally
            {
                if (pipePtr != IntPtr.Zero)
                {
                    NativeHelper.pclose(pipePtr);
                }
            }
        }

        /// <summary>
        /// Determines if the inbound or outbound forward iptable rule already exists.
        /// The inbound rule looks like below -
        /// target   prot  opt  source        destination
        /// ACCEPT   all   --   anywhere      10.0.0.0/24
        /// The outbound rule looks like below -
        /// target   prot  opt  source        destination
        /// ACCEPT   all   --   10.0.0.0/24   anywhere
        /// </summary>
        protected bool DoesIptableRuleExist(string subnet, ConnectivityDirection connectivityDir)
        {
            bool exists = false;
            IntPtr pipePtr = IntPtr.Zero;

            try
            {
                var doesIptableRuleExistCommand = "sudo iptables -L FORWARD | grep ACCEPT";
                pipePtr = NativeHelper.popen(doesIptableRuleExistCommand, "r");
                if (pipePtr == IntPtr.Zero)
                {
                    DeployerTrace.WriteInfo("Failed to open stream for command {0}.", doesIptableRuleExistCommand);
                }
                else
                {
                    StringBuilder outputBuilder = new StringBuilder(1024);
                    while (NativeHelper.fgets(outputBuilder, 1024, pipePtr) != IntPtr.Zero)
                    {
                        string sData = outputBuilder.ToString();
                        var sDataArray = sData.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
                        if (sDataArray.Length >= 5)
                        {
                            if (string.Equals(sDataArray[0], FlatNetworkConstants.AcceptAction, StringComparison.OrdinalIgnoreCase) &&
                                string.Equals(sDataArray[1], FlatNetworkConstants.AllProtocols, StringComparison.OrdinalIgnoreCase) &&
                                string.Equals(sDataArray[2], FlatNetworkConstants.NoOptions, StringComparison.OrdinalIgnoreCase))
                            {
                                if (connectivityDir == ConnectivityDirection.Inbound)
                                {
                                    if (string.Equals(sDataArray[3], FlatNetworkConstants.AnyIp, StringComparison.OrdinalIgnoreCase) &&
                                        string.Equals(sDataArray[4], subnet, StringComparison.OrdinalIgnoreCase))
                                    {
                                        exists = true;
                                        break;
                                    }
                                }
                                else if (connectivityDir == ConnectivityDirection.Outbound)
                                {
                                    if (string.Equals(sDataArray[3], subnet, StringComparison.OrdinalIgnoreCase) &&
                                        string.Equals(sDataArray[4], FlatNetworkConstants.AnyIp, StringComparison.OrdinalIgnoreCase))
                                    {
                                        exists = true;
                                        break;
                                    }
                                }
                            }
                        }
                        outputBuilder.Clear();
                    }
                }
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to check if forward iptable rule exists for {0} connectivity exception {1}", connectivityDir, ex);
            }
            finally
            {
                if (pipePtr != IntPtr.Zero)
                {
                    NativeHelper.pclose(pipePtr);
                }
            }

            return exists;
        }

        /// <summary>
        /// Uninstalls the azure network driver. This is the driver used to create the
        /// flat network to which containers with static IPs are added.
        /// </summary>
        /// <returns></returns>
        protected bool UninstallNetworkDriver()
        {
            bool success = false;

            DeployerTrace.WriteInfo("Uninstalling the network driver.");

            try
            {
                bool networkDriverKilled = false;
                bool running;
                int processId = 0;
                IsProcessRunning(FlatNetworkConstants.NetworkDriverPlugin, out running, out processId);
                if (running)
                {
                    var command = string.Format("sudo kill {0}", processId);
                    int returnvalue = NativeHelper.system(command);
                    if (returnvalue != 0)
                    {
                        DeployerTrace.WriteInfo("Failed to execute command {0} return value {1}", command, returnvalue);
                    }
                    else
                    {
                        DeployerTrace.WriteInfo("Successfully executed command \"{0}\".", command);
                        networkDriverKilled = true;
                    }
                }
                else
                {
                    DeployerTrace.WriteInfo("Skipping kill of network driver process since it is not running.");
                    networkDriverKilled = true;
                }

                if (networkDriverKilled)
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
                        success = true;
                    }
                }
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to uninstall network driver exception {0}", ex);
            }

            return success;
        }

        /// <summary>
        /// Removes iptable rule that allows multi-ip containers
        /// to communicate with each other.
        /// </summary>
        /// <returns></returns>
        protected bool RemoveIptableRule()
        {
            string subnet = string.Empty;
            string gateway = string.Empty;
            string macAddress = string.Empty;
            if (Utility.RetrieveNMAgentInterfaceInfo(true, out subnet, out gateway, out macAddress))
            {
                return (RemoveIptableRule(subnet, ConnectivityDirection.Inbound) && RemoveIptableRule(subnet, ConnectivityDirection.Outbound));
            }

            return false;
        }

        /// <summary>
        /// Removes forward iptable rule that allows multi-ip containers
        /// to communicate with each other and externally.
        /// </summary>
        /// <returns></returns>
        protected bool RemoveIptableRule(string subnet, ConnectivityDirection connectivityDir)
        {
           bool success = false;

           DeployerTrace.WriteInfo("Removing iptable rule for {0} connectivity.", connectivityDir);

           try
           {
              if (DoesIptableRuleExist(subnet, connectivityDir))
              {
                  string command = string.Empty;
                  if (connectivityDir == ConnectivityDirection.Inbound)
                  {
                      command = string.Format("sudo iptables -D FORWARD -d {0} -j ACCEPT", subnet);
                  }
                  else if (connectivityDir == ConnectivityDirection.Outbound)
                  {
                      command = string.Format("sudo iptables -D FORWARD -s {0} -j ACCEPT", subnet);
                  }

                  int returnvalue = NativeHelper.system(command);
                  if (returnvalue == 0)
                  {
                      DeployerTrace.WriteInfo("Successfully executed command \"{0}\".", command);
                      success = true;
                  }
                  else
                  {
                     DeployerTrace.WriteInfo("Failed to remove forward iptable rule for {0} connectivity.", connectivityDir);
                  }
              }
              else
              {
                  DeployerTrace.WriteInfo("Skipping removal of forward iptable rule for {0} connectivity since it does not exist.");
                  success = true;
              }
           }
           catch (Exception ex)
           {
               DeployerTrace.WriteError("Failed to remove forward iptable rule for {0} connectivity exception {1}", ex);
           }

           return success;
        }
#else

        /// <summary>
        /// Retrieves firewall policy.
        /// </summary>
        /// <returns></returns>
        protected internal INetFwPolicy2 GetFirewallPolicy()
        {
            Type policyType = Type.GetTypeFromProgID(FlatNetworkConstants.FirewallPolicy2TypeProgID);

            if (policyType == null)
            {
                return null;
            }

            INetFwPolicy2 fwPolicy2 = (INetFwPolicy2)Activator.CreateInstance(policyType);
            return fwPolicy2;
        }

        /// <summary>
        /// Checks if multi-ip firewall rule exists.
        /// </summary>
        /// <param name="fwPolicy2"></param>
        /// <returns></returns>
        protected internal bool DoesFirewallRuleExist(INetFwPolicy2 fwPolicy2)
        {
            if (fwPolicy2.Rules == null)
            {
                return false;
            }

            foreach (NetFwRule r in fwPolicy2.Rules)
            {
                if (string.Equals(r.Name, FlatNetworkConstants.FirewallRuleName, StringComparison.OrdinalIgnoreCase) &&
                    string.Equals(r.Grouping, FlatNetworkConstants.FirewallGroupName, StringComparison.OrdinalIgnoreCase))
                {
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Removes firewall rule
        /// </summary>
        /// <returns></returns>
        protected bool RemoveFirewallRule()
        {
            INetFwPolicy2 fwPolicy2 = GetFirewallPolicy();
            if (fwPolicy2 == null)
            {
                string message = "Unable to get firewall policy.";
                throw new InvalidOperationException(message);
            }

            bool exists = DoesFirewallRuleExist(fwPolicy2);

            if (!exists)
            {
                return true;
            }

            fwPolicy2.Rules.Remove(FlatNetworkConstants.FirewallRuleName);

            return true;
        }

        /// <summary>
        /// Starts the default Docker NT service if stopped by the deployer.
        /// This is simply restoring state on the machine after docker network has been set up.
        /// If docker container service is already running on the right container service arguments, return true.
        /// </summary>
        /// <returns></returns>
        protected internal bool StartContainerNTService(bool containerServiceRunning, bool containerNTServiceStopped)
        {
            if (containerServiceRunning)
            {
                return true;
            }

            bool success = false;

            DeployerTrace.WriteInfo("Starting container NT service {0}.", FlatNetworkConstants.ContainerServiceName);

            try
            {
                if (containerNTServiceStopped)
                {
                    var service = ServiceController.GetServices().SingleOrDefault(s => string.Equals(s.ServiceName, FlatNetworkConstants.ContainerServiceName, StringComparison.OrdinalIgnoreCase));

                    if (service != null)
                    {
                        if (service.Status != ServiceControllerStatus.Running && service.Status != ServiceControllerStatus.StartPending)
                        {
                            service.Start();
                            service.WaitForStatus(ServiceControllerStatus.Running, new TimeSpan(0, 0, Constants.ServiceManagementTimeoutInSeconds));
                            success = true;
                            DeployerTrace.WriteInfo("Successfully started container NT service {0}.", FlatNetworkConstants.ContainerServiceName);
                        }
                        else
                        {
                            success = true;
                            DeployerTrace.WriteInfo("Container NT service {0} is already running or pending start.", FlatNetworkConstants.ContainerServiceName);
                        }
                    }
                    else
                    {
                        DeployerTrace.WriteWarning("Container NT service {0} not found.", FlatNetworkConstants.ContainerServiceName);
                    }
                }
                else
                {
                    success = true;
                    DeployerTrace.WriteInfo("Skipping start of container NT service since it was not stopped by deployer.");
                }
            }
            catch (TimeoutException)
            {
                DeployerTrace.WriteError("Start container NT service {0} timed out.", FlatNetworkConstants.ContainerServiceName);
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to start container NT service {0} exception {1}.", FlatNetworkConstants.ContainerServiceName, ex);
            }

            return success;
        }

        /// <summary>
        /// Stops the Docker NT service.This is done so that we can bring up docker on the provided container service arguments
        /// since this is the endpoint that Fabric Hosting is using for setting up containers.
        /// If docker container service is already running on the right container service arguments, return true.
        /// </summary>
        /// <param name="stopped"></param>
        /// <returns></returns>
        protected internal bool StopContainerNTService(bool containerServiceRunning, out bool stopped)
        {
            stopped = false;

            if (containerServiceRunning)
            {
                return true;
            }

            bool success = false;

            DeployerTrace.WriteInfo("Stopping container NT service {0}.", FlatNetworkConstants.ContainerServiceName);

            try
            {
                var service = ServiceController.GetServices().SingleOrDefault(s => string.Equals(s.ServiceName, FlatNetworkConstants.ContainerServiceName, StringComparison.OrdinalIgnoreCase));

                if (service != null)
                {
                    // stop service if not already stopped
                    if (service.Status != ServiceControllerStatus.Stopped && service.Status != ServiceControllerStatus.StopPending)
                    {
                        try
                        {
                            service.Stop();
                            service.WaitForStatus(ServiceControllerStatus.Stopped, new TimeSpan(0, 0, Constants.ServiceManagementTimeoutInSeconds));
                            stopped = true;
                            success = true;
                            DeployerTrace.WriteInfo("Successfully stopped container NT service {0}.", FlatNetworkConstants.ContainerServiceName);
                        }
                        catch (TimeoutException)
                        {
                            DeployerTrace.WriteError("Stop container NT service {0} timed out.", FlatNetworkConstants.ContainerServiceName);
                        }
                        catch (Exception ex)
                        {
                            DeployerTrace.WriteError("Failed to stop container NT service {0} exception {1}.", FlatNetworkConstants.ContainerServiceName, ex);
                        }
                    }
                    else
                    {
                        success = true;
                        DeployerTrace.WriteInfo("Container NT service {0} already stopped or pending stop.", FlatNetworkConstants.ContainerServiceName);
                    }
                }
                else
                {
                    DeployerTrace.WriteWarning("Container NT service {0} not found.", FlatNetworkConstants.ContainerServiceName);
                }
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to stop container NT service {0} exception {1}.", FlatNetworkConstants.ContainerServiceName, ex);
            }

            return success;
        }

        /// <summary>
        /// Starts the docker container service using the provided container service arguments.
        /// This is the endpoint used to create the docker network.
        /// If the docker container service is already running using the right container service arguments, return true.
        /// </summary>
        /// <returns></returns>
        protected internal bool StartContainerProviderService(string containerServiceArguments, string fabricDataRoot, bool containerServiceRunning)
        {
            bool success = false;

            if (containerServiceRunning)
            {
                return true;
            }

            if (StopContainerProviderService(fabricDataRoot, containerServiceRunning))
            {
                DeployerTrace.WriteInfo("Starting service {0}.", FlatNetworkConstants.ContainerProviderProcessName);

                var programFilesPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), FlatNetworkConstants.ContainerServiceName);

                var dockerPidFileDirectory = Path.Combine(fabricDataRoot, FlatNetworkConstants.DockerProcessIdFileDirectory);
                if (!Directory.Exists(dockerPidFileDirectory))
                {
                    Directory.CreateDirectory(dockerPidFileDirectory);
                }

                var dockerPidFilePath = Path.Combine(dockerPidFileDirectory, string.Format("{0}_{1}", DateTime.Now.Ticks, FlatNetworkConstants.DockerProcessFile));

                containerServiceArguments = string.Format("{0} --pidfile={1}", containerServiceArguments, dockerPidFilePath);

                if (Utility.ExecuteCommand(FlatNetworkConstants.ContainerProviderProcessName, containerServiceArguments, programFilesPath, false, null))
                {
                    // Allow the system to catch up after starting up docker service
                    for (int i = 0; i < Constants.ApiRetryAttempts; i++)
                    {
                        Thread.Sleep(Constants.ApiRetryIntervalMilliSeconds);

                        bool containerServiceReady = IsContainerServiceRunning();
                        if (containerServiceReady)
                        {
                            DeployerTrace.WriteInfo("Successfully started container process.");
                            success = true;
                            break;
                        }
                    }
                }
                else
                {
                    DeployerTrace.WriteError("Failed to start service {0}.", FlatNetworkConstants.ContainerProviderProcessName);
                }
            }

            return success;
        }

        /// <summary>
        /// Stops the dockerd service. This is done so that we can bring it up on the provided container service arguments. 
        /// If docker container service is already running on the right container service arguments, return true.
        /// </summary>
        /// <returns></returns>
        protected internal bool StopContainerProviderService(string fabricDataRoot, bool containerServiceRunning)
        {
            if (containerServiceRunning)
            {
                return true;
            }

            bool success = false;

            DeployerTrace.WriteInfo("Stopping service {0}.", FlatNetworkConstants.ContainerProviderProcessName);

            try
            {
                if (Utility.StopProcess(FlatNetworkConstants.ContainerProviderProcessName))
                {
                    var dockerPidFileDirectory = Path.Combine(fabricDataRoot, FlatNetworkConstants.DockerProcessIdFileDirectory);
                    var dockerPidFiles = Directory.Exists(dockerPidFileDirectory) ? Directory.GetFiles(dockerPidFileDirectory) : new string[0];
                    if (dockerPidFiles.Count() > 0)
                    {
                        DeployerTrace.WriteInfo("Deleting docker pid files.");
                        foreach (var dockerPidFile in dockerPidFiles)
                        {
                            File.Delete(dockerPidFile);
                        }
                    }
                    else
                    {
                        DeployerTrace.WriteInfo("Docker pid files do not exist in directory {0}.", dockerPidFileDirectory);
                    }

                    success = true;
                }
                else
                {
                    DeployerTrace.WriteError("Failed to stop container provider service {0}.", FlatNetworkConstants.ContainerProviderProcessName);
                }
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to stop container provider service {0} exception {1}.", FlatNetworkConstants.ContainerProviderProcessName, ex);
            }

            return success;
        }

        /// <summary>
        /// Retrieves the IpV4 routing table. This is cached and restored
        /// after the docker create network call. The create network call
        /// has been known to drop entries from the ip routing table. If we
        /// drop the wire server entry, then calls to nm agent will fail.
        /// </summary>
        /// <param name="ipForwardTable"></param>
        /// <returns></returns>
        protected bool RetrieveIPv4RoutingTable(out NativeMethods.IPFORWARDTABLE ipForwardTable)
        {
            bool success = false;
            ipForwardTable = default(NativeMethods.IPFORWARDTABLE);

            DeployerTrace.WriteInfo("Retrieving IPv4 routing table.");

            try
            {
                int size = 0;
                var result = NativeMethods.GetIpForwardTable(new SafeInteropPtrToStructureMemoryHandle<NativeMethods.IPFORWARDTABLE>(), ref size, true);
                if (result == NativeMethods.ERROR_INSUFFICIENT_BUFFER)
                {
                    using (var tablePtr = new SafeInteropPtrToStructureMemoryHandle<NativeMethods.IPFORWARDTABLE>(size))
                    {
                        result = NativeMethods.GetIpForwardTable(tablePtr, ref size, true);
                        if (result == NativeMethods.ERROR_SUCCESS)
                        {
                            ipForwardTable = tablePtr.Struct;

                            NativeMethods.IPFORWARDROW[] rows = new NativeMethods.IPFORWARDROW[ipForwardTable.Size];
                            IntPtr rowPtr = new IntPtr(tablePtr.ToInt64() + Marshal.SizeOf(ipForwardTable.Size));
                            for (int i = 0; i < ipForwardTable.Size; ++i)
                            {
                                rows[i] = (NativeMethods.IPFORWARDROW)Marshal.PtrToStructure(rowPtr, typeof(NativeMethods.IPFORWARDROW));
                                rowPtr = new IntPtr(rowPtr.ToInt64() + Marshal.SizeOf(typeof(NativeMethods.IPFORWARDROW)));
                            }

                            ipForwardTable.Table = rows;
                        }
                        else
                        {
                            DeployerTrace.WriteError("Failed to get the IPv4 forward table details.");
                        }
                    }
                }
                else
                {
                    DeployerTrace.WriteError("Failed to get the IPv4 forward table size information.");
                }

                success = true;
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to get IPv4 routing table exception {0}.", ex);
            }

            return success;
        }

        /// <summary>
        /// Prints routing table row details.
        /// </summary>
        /// <param name="i"></param>
        /// <param name="row"></param>
        private void PrintRoutingTableRow(int i, NativeMethods.IPFORWARDROW row)
        {
            DeployerTrace.WriteInfo("Route[{0}] Dest IP: {1}\n", i, Utility.ConvertToIpAddress(row.dwForwardDest, true));
            DeployerTrace.WriteInfo("Route[{0}] Subnet Mask: {1}\n", i, Utility.ConvertToIpAddress(row.dwForwardMask, true));
            DeployerTrace.WriteInfo("Route[{0}] Next Hop: {1}\n", i, Utility.ConvertToIpAddress(row.dwForwardNextHop, true));
            DeployerTrace.WriteInfo("Route[{0}] Forward Policy: {1}\n", i, row.dwForwardPolicy);
            DeployerTrace.WriteInfo("Route[{0}] If Index: {1}\n", i, row.dwForwardIfIndex);
            DeployerTrace.WriteInfo("Route[{0}] Type: {1}\n", i, row.dwForwardType);
            DeployerTrace.WriteInfo("Route[{0}] Proto: {1}\n", i, row.dwForwardProto);
            DeployerTrace.WriteInfo("Route[{0}] Age: {1}\n", i, row.dwForwardAge);
            DeployerTrace.WriteInfo("Route[{0}] Metric1: {1}\n", i, row.dwForwardMetric1);
        }

        protected int GetHNSTransparentNetworkInterfaceIndex(string subnet)
        {
            int interfaceIndex = -1;

            uint subnetIp;
            uint subnetMask;
            if (Utility.ParseSubnetIpAndMask(subnet, out subnetIp, out subnetMask))
            {
                var subnetIpAddress = Utility.ConvertToIpAddress(subnetIp, false);
                var nic = Utility.GetNetworkInterfaceOnSubnetWithRetry(subnetIpAddress);
                if (nic != null)
                {
                    var ipProperties = nic.GetIPProperties();
                    if (ipProperties != null)
                    {
                        var ipv4Properties = ipProperties.GetIPv4Properties();
                        if (ipv4Properties != null)
                        {
                            interfaceIndex = ipv4Properties.Index;
                        }
                    }
                }
            }

            return interfaceIndex;
        }

        /// <summary>
        /// Compares static routes in old and new ip routing tables.
        /// </summary>
        /// <param name="oldIpForwardTable"></param>
        /// <param name="newIpForwardTable"></param>
        /// <returns></returns>
        protected bool CompareIPv4RoutingTables(NativeMethods.IPFORWARDTABLE oldIpForwardTable, NativeMethods.IPFORWARDTABLE newIpForwardTable)
        {
            bool match = true;

            DeployerTrace.WriteInfo("Comparing old and new IPv4 routing tables.");

            for (int i = 0; i < oldIpForwardTable.Table.Length && match; i++)
            {
                if (oldIpForwardTable.Table[i].dwForwardProto != 3)
                {
                    continue;
                }

                match = false;
                for (int j = 0; j < newIpForwardTable.Table.Length && !match; j++)
                {
                    if (newIpForwardTable.Table[i].dwForwardProto != 3)
                    {
                        continue;
                    }

                    if (oldIpForwardTable.Table[i].dwForwardDest == newIpForwardTable.Table[j].dwForwardDest &&
                        oldIpForwardTable.Table[i].dwForwardMask == newIpForwardTable.Table[j].dwForwardMask &&
                        oldIpForwardTable.Table[i].dwForwardNextHop == newIpForwardTable.Table[j].dwForwardNextHop)
                    {
                        match = true;
                    }
                }

                // if there is no match in the new table for the old row, then print out the old row
                if (!match)
                {
                    PrintRoutingTableRow(i, oldIpForwardTable.Table[i]);
                }
            }

            DeployerTrace.WriteInfo("Old and new IPv4 routing tables match:{0}.", match);

            return match;
        }

        /// <summary>
        /// Docker network create can result in loss of routing table entries.
        /// This api restores the routing table only for static routing entries.
        /// </summary>
        /// <param name="oldIpForwardTable"></param>
        /// <param name="newIpForwardTable"></param>
        /// <param name="subnet"></param>
        /// <returns></returns>
        protected bool RestoreIPv4RoutingTable(NativeMethods.IPFORWARDTABLE oldIpForwardTable, NativeMethods.IPFORWARDTABLE newIpForwardTable, string subnet)
        {
            bool success = false;

            DeployerTrace.WriteInfo("Restoring IPv4 routing table.");

            if (CompareIPv4RoutingTables(oldIpForwardTable, newIpForwardTable))
            {
                success = true;
                DeployerTrace.WriteInfo("Skipping restoring routing table since it is intact.");
                return success;
            }

            try
            {
                var strb = new StringBuilder();
                bool allRoutesCreated = true;

                for (int i = 0; i < oldIpForwardTable.Table.Length; ++i)
                {
                    if (oldIpForwardTable.Table[i].dwForwardProto == 3)
                    {
                        using (var rowPtr = new SafeInteropStructureToPtrMemoryHandle(oldIpForwardTable.Table[i]))
                        {
                            NativeMethods.IPFORWARDROW rowPtrNew = new NativeMethods.IPFORWARDROW();
                            rowPtrNew.dwForwardDest = oldIpForwardTable.Table[i].dwForwardDest;
                            rowPtrNew.dwForwardMask = oldIpForwardTable.Table[i].dwForwardMask;
                            rowPtrNew.dwForwardNextHop = oldIpForwardTable.Table[i].dwForwardNextHop;
                            rowPtrNew.dwForwardPolicy = oldIpForwardTable.Table[i].dwForwardPolicy;
                            rowPtrNew.dwForwardProto = oldIpForwardTable.Table[i].dwForwardProto;
                            rowPtrNew.dwForwardType = oldIpForwardTable.Table[i].dwForwardType;
                            rowPtrNew.dwForwardMetric1 = oldIpForwardTable.Table[i].dwForwardMetric1;
                            var hnsTransparentInterfaceIndex = GetHNSTransparentNetworkInterfaceIndex(subnet);
                            rowPtrNew.dwForwardIfIndex = (hnsTransparentInterfaceIndex == -1) ? (oldIpForwardTable.Table[i].dwForwardIfIndex) : ((uint)hnsTransparentInterfaceIndex);

                            using (var rowPtrNewSafe = new SafeInteropStructureToPtrMemoryHandle(rowPtrNew))
                            {
                                int retVal = NativeMethods.CreateIpForwardEntry(rowPtrNewSafe);

                                strb.AppendFormat("Route[{0}] Dest IP: {1}\n", i, Utility.ConvertToIpAddress(rowPtrNew.dwForwardDest, true));
                                strb.AppendFormat("Route[{0}] Subnet Mask: {1}\n", i, Utility.ConvertToIpAddress(rowPtrNew.dwForwardMask, true));
                                strb.AppendFormat("Route[{0}] Next Hop: {1}\n", i, Utility.ConvertToIpAddress(rowPtrNew.dwForwardNextHop, true));
                                strb.AppendFormat("Route[{0}] Forward Policy: {1}\n", i, rowPtrNew.dwForwardPolicy);
                                strb.AppendFormat("Route[{0}] If Index: {1}\n", i, rowPtrNew.dwForwardIfIndex);
                                strb.AppendFormat("Route[{0}] Type: {1}\n", i, rowPtrNew.dwForwardType);
                                strb.AppendFormat("Route[{0}] Proto: {1}\n", i, rowPtrNew.dwForwardProto);
                                strb.AppendFormat("Route[{0}] Age: {1}\n", i, rowPtrNew.dwForwardAge);
                                strb.AppendFormat("Route[{0}] Metric1: {1}\n", i, rowPtrNew.dwForwardMetric1);

                                if (retVal == NativeMethods.ERROR_OBJECT_ALREADY_EXISTS)
                                {
                                    DeployerTrace.WriteInfo("Route already exists.\nRoute details: {0}.", strb.ToString());
                                }
                                else if (retVal != NativeMethods.ERROR_SUCCESS)
                                {
                                    var ex = new Win32Exception(retVal);
                                    DeployerTrace.WriteError("Unable to create route exception {0}\nRoute details: {1}.", ex, strb.ToString());
                                    allRoutesCreated = false;
                                }
                                else
                                {
                                    DeployerTrace.WriteInfo("Route added.\nRoute details: {0}.", strb.ToString());
                                }

                                strb.Clear();
                            }
                        }
                    }
                }

                success = allRoutesCreated;
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to restore routing table exception {0}.", ex);
            }

            return success;
        }
#endif

        /// <summary>
        /// Get network details, if it exists.
        /// </summary>
        /// <param name="networkName"></param>
        /// <param name="dockerNetworkName"></param>
        /// <param name="dockerNetworkId"></param>
        /// <returns></returns>
        protected internal bool GetNetwork(string networkName, out string dockerNetworkName, out string dockerNetworkId)
        {
            bool success = false;
            dockerNetworkName = string.Empty;
            dockerNetworkId = string.Empty;

            DeployerTrace.WriteInfo("Getting network id for network Name:{0}.", networkName);

            try
            {
                var networkGetByTypeRequestPath = string.Format(
                    FlatNetworkConstants.NetworkGetByTypeRequestPath,
                    FlatNetworkConstants.NetworkDriver);

                var networkGetByTypeUrl = this.dockerClient.GetRequestUri(networkGetByTypeRequestPath);
                var task = dockerClient.HttpClient.GetAsync(networkGetByTypeUrl);
                var httpResponseMessage = task.GetAwaiter().GetResult();
                string response = httpResponseMessage.Content.ReadAsStringAsync().GetAwaiter().GetResult();
                if (!httpResponseMessage.IsSuccessStatusCode)
                {
                    DeployerTrace.WriteError("Failed to get network details for network name:{0} error details {1}.", networkName, response);
                }
                else
                {
                    var networks = JsonConvert.DeserializeObject<NetworkConfig[]>(response);
                    if (networks.Length > 0)
                    {
                        dockerNetworkId = networks[0].Id;
                        dockerNetworkName = networks[0].Name;
                    }

                    success = true;
                    DeployerTrace.WriteInfo("Retrieved network id:{0} network name:{1}.", dockerNetworkId, dockerNetworkName);
                }
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to get network details for network Name:{0} exception {1}.", networkName, ex);
            }

            return success;
        }

        protected bool IsContainerServiceRunning()
        {
            bool isRunning = false;

            try
            {
                this.dockerClient.SystemOperation.PingAsync(TimeSpan.FromSeconds(30)).GetAwaiter().GetResult();
                isRunning = true;
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteInfo("Failed to check if container service is running, exception {0}", ex);
            }

            return isRunning;
        }

        /// <summary>
        /// Removes flat network
        /// </summary>
        /// <param name="networkName"></param>
        /// <returns></returns>
        protected bool RemoveNetwork(string networkName)
        {
            bool success = false;

            DeployerTrace.WriteInfo("Removing network Name:{0}.", networkName);

            try
            {
                string dockerNetworkId = string.Empty;
                string dockerNetworkName = string.Empty;
                if (GetNetwork(networkName, out dockerNetworkName, out dockerNetworkId))
                {
                    if (!string.IsNullOrEmpty(dockerNetworkId))
                    {
                        var deleterequestPath = string.Format(FlatNetworkConstants.NetworkDeleteRequestPath, dockerNetworkId);
                        var deleteUrl = this.dockerClient.GetRequestUri(deleterequestPath);
                        var task = dockerClient.HttpClient.DeleteAsync(deleteUrl);
                        var httpResponseMessage = task.GetAwaiter().GetResult();

                        string response = httpResponseMessage.Content.ReadAsStringAsync().GetAwaiter().GetResult();
                        if (!httpResponseMessage.IsSuccessStatusCode)
                        {
                            DeployerTrace.WriteError("Failed to delete network Name:{0} error details {1}.", networkName, response);
                        }
                    }
                }
                else
                {
                    DeployerTrace.WriteError("Failed to retrieve network id for network Name:{0}.", networkName);
                }

                success = true;
                DeployerTrace.WriteInfo("Removed network Name:{0}", networkName);
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to remove network Name:{0} exception {1}.", networkName, ex);
            }

            return success;
        }

        /// <summary>
        /// Checks if the wire server is available.
        /// This check was added for the edge cluster scenario where the
        /// infrastructure type is "PAAS" but there is no wire server available.
        /// </summary>
        /// <returns></returns>
        protected bool IsWireServerAvailable()
        {
            bool success = false;

            TimeSpan retryInterval = TimeSpan.FromMilliseconds(Constants.ApiRetryIntervalMilliSeconds);
            int retryCount = Constants.ApiRetryAttempts;
            Type[] exceptionTypes = { typeof(Exception) };

            try
            {
                System.Fabric.Common.Helpers.PerformWithRetry(() =>
                {
                    HttpClient httpClient = new HttpClient();
                    var task = httpClient.GetAsync(FlatNetworkConstants.NetworkInterfaceUrl);
                    task.GetAwaiter().GetResult().EnsureSuccessStatusCode();
                    success = true;
                },
                exceptionTypes,
                retryInterval,
                retryCount);
            }
            catch (Exception)
            {
                DeployerTrace.WriteInfo("Wire server is not available.");
            }

            return success;
        }
    }
}
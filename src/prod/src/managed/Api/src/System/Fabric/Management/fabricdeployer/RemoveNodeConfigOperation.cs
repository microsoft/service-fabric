// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using Collections.Generic;
    using Linq;
    using Microsoft.Win32;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.ServiceProcess;
    using System.Threading;
    using System.Threading.Tasks;

    internal class RemoveNodeConfigOperation : DeploymentOperation
    {
        public static readonly string DeleteLogTrue = "DeleteLogTrue";
        public static readonly string DeleteLogFalse = "DeleteLogFalse";

        public static void DeleteRemoveNodeConfigurationRegistryValue(string machineName)
        {
            RegistryKey hklm = Utility.GetHklm(machineName);

            using (RegistryKey regKey = hklm.OpenSubKey(FabricConstants.FabricRegistryKeyPath, true))
            {
                if (regKey == null)
                {
                    string error = string.Format(CultureInfo.InvariantCulture, "Could not open registry key '{0}', is product installed?", FabricConstants.FabricRegistryKeyPath);
                    DeployerTrace.WriteError(error);
                    throw new InvalidOperationException(error);
                }

                regKey.DeleteValue(Constants.Registry.RemoveNodeConfigurationValue, false);
                object valueAfterDelete = regKey.GetValue(Constants.Registry.RemoveNodeConfigurationValue); 
                if (valueAfterDelete != null)
                {
                    string error = string.Format(CultureInfo.InvariantCulture, "Could not delete {0} value under {1}", Constants.Registry.RemoveNodeConfigurationValue, FabricConstants.FabricRegistryKeyPath);
                    DeployerTrace.WriteError(error);
                    throw new InvalidOperationException(error);
                }
            }
        }

        protected override void OnExecuteOperation(DeploymentParameters parameters, ClusterManifestType clusterManifest, Infrastructure infrastructure)
        {
            RemoveLogicalDirectoriesIfExist(parameters.FabricDataRoot, parameters.MachineName);

            if (parameters.DeploymentPackageType == FabricPackageType.XCopyPackage)
            {
                RemoveNodeConfigurationXCopy(parameters.MachineName, parameters.DeleteLog, parameters.NodeName);
            }
            else
            {
                if (string.IsNullOrEmpty(parameters.MachineName)
                    || parameters.MachineName == Constants.LocalHostMachineName
                    || parameters.MachineName.Equals(Helpers.GetMachine()))
                {
                    this.RemoveNodeConfigurationMsi(parameters.DeleteLog);
                }
                else
                {
                    throw new ArgumentException(StringResources.Error_RemoveNodeConfigOperationNotSupportedRemovelyWithMSI);
                }
            }
        }

        private void RemoveLogicalDirectoriesIfExist(string fabricDataRoot, string machineName)
        {
            if (machineName == null)
            {
                machineName = "";
            }

            ClusterManifestType clusterManifest = null;

            var clusterManifestLocation = Helpers.GetCurrentClusterManifestPath(fabricDataRoot);
            if (clusterManifestLocation == null)
            {
                clusterManifestLocation = Directory.EnumerateFiles(
                    fabricDataRoot, 
                    "clusterManifest.xml",
                    SearchOption.AllDirectories).FirstOrDefault();
            }

            if (clusterManifestLocation != null)
            {
                clusterManifest = XMLHelper.ReadClusterManifest(clusterManifestLocation);
            }
            else
            {
                DeployerTrace.WriteWarning(string.Format(StringResources.Warning_clusterManifestFileMissingInDataRoot, fabricDataRoot, machineName));
            }

            if (clusterManifest != null)
            {
                InfrastructureInformationType infrastructure = null;
                FabricNodeType[] nodeList;

                if ((clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsServer &&
                ((ClusterManifestTypeInfrastructureWindowsServer)clusterManifest.Infrastructure.Item).IsScaleMin)
                || (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureLinux &&
                ((ClusterManifestTypeInfrastructureLinux)clusterManifest.Infrastructure.Item).IsScaleMin))
                {
                    DeployerTrace.WriteInfo(string.Format("Checking to remove logicalDirectories for one box scale min deployment. Machine name: {0}.", machineName));

                    // Get all the nodes on this IP's LogicalDirectories and remove the all directoies.
                    nodeList = clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsServer ?
                        ((ClusterManifestTypeInfrastructureWindowsServer)clusterManifest.Infrastructure.Item).NodeList :
                        ((ClusterManifestTypeInfrastructureLinux)clusterManifest.Infrastructure.Item).NodeList;

                    RemoveLogicalDirectoies(clusterManifest, nodeList, machineName);
                }
                else if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructurePaaS) // PaaS doesn't support scale min.
                {
                    var infrastructureManifestLocation = Helpers.GetInfrastructureManifestPath(fabricDataRoot);
                    if (infrastructureManifestLocation == null)
                    {
                        DeployerTrace.WriteWarning(string.Format(StringResources.Warning_infrastructureManifestFileMissingInDataRoot, fabricDataRoot, machineName));
                        return;
                    }

                    infrastructure = XMLHelper.ReadInfrastructureManifest(infrastructureManifestLocation);

                    // Get all the nodes on this IP's LogicalDirectories and remove the all directories.
                    var len = infrastructure.NodeList.Length;
                    nodeList = new FabricNodeType[len];
                    for (var i = 0; i < len; i++)
                    {
                        FabricNodeType fabricNode = new FabricNodeType()
                        {
                            NodeName = infrastructure.NodeList[i].NodeName,
                            FaultDomain = infrastructure.NodeList[i].FaultDomain,
                            IPAddressOrFQDN = infrastructure.NodeList[i].IPAddressOrFQDN,
                            IsSeedNode = infrastructure.NodeList[i].IsSeedNode,
                            NodeTypeRef = infrastructure.NodeList[i].NodeTypeRef,
                            UpgradeDomain = infrastructure.NodeList[i].UpgradeDomain
                        };

                        nodeList[i] = fabricNode;
                    }

                    RemoveLogicalDirectoies(clusterManifest, nodeList, machineName);
                }
                else if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsServer 
                    || clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureLinux)
                {
                    nodeList = clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsServer ?
                        ((ClusterManifestTypeInfrastructureWindowsServer)clusterManifest.Infrastructure.Item).NodeList :
                        ((ClusterManifestTypeInfrastructureLinux)clusterManifest.Infrastructure.Item).NodeList;
                    if (!string.IsNullOrEmpty(machineName)) //For cab deployment
                    {
                        DeployerTrace.WriteInfo(string.Format("Checking to remove logicalDirectories for WindowsServer/Linux deployment with cab. Machine name: {0}.", machineName));
                        RemoveLogicalDirectoies(clusterManifest, nodeList, machineName);
                    }
                    else
                    {
                        DeployerTrace.WriteInfo(string.Format("Checking to remove logicalDirectories for WindowsServer/Linux deployment with MSI. Machine name: {0}.", machineName));
                        foreach (var node in nodeList)
                        {
                            InfrastructureNodeType infraNode = new InfrastructureNodeType()
                            {
                                NodeName = node.NodeName,
                                NodeTypeRef = node.NodeTypeRef,
                                IPAddressOrFQDN = node.IPAddressOrFQDN,
                                IsSeedNode = node.IsSeedNode,
                                FaultDomain = node.FaultDomain,
                                UpgradeDomain = node.UpgradeDomain
                            };

                            bool isNodeForThisMachine = NetworkApiHelper.IsNodeForThisMachine(infraNode);
                            if (isNodeForThisMachine)
                            {
                                FabricNodeType[] fabricNodes = new FabricNodeType[1];
                                FabricNodeType fabricNode = new FabricNodeType()
                                {
                                    NodeName = node.NodeName,
                                    FaultDomain = node.FaultDomain,
                                    IPAddressOrFQDN = node.IPAddressOrFQDN,
                                    IsSeedNode = node.IsSeedNode,
                                    NodeTypeRef = node.NodeTypeRef,
                                    UpgradeDomain = node.UpgradeDomain
                                };

                                fabricNodes[0] = fabricNode;
                                RemoveLogicalDirectoies(clusterManifest, fabricNodes, machineName);
                            }
                        }
                    }
                }          
            }
        }

        private static void RemoveLogicalDirectoies(ClusterManifestType clusterManifest, FabricNodeType[] nodelist, string machineName)
        {
            List<LogicalDirectoryType[]> LogicalDirectoriesSetForThisMachine = new List<LogicalDirectoryType[]>();
            foreach (var node in nodelist)
            {
                var LogicalDirectory = clusterManifest.NodeTypes.Where(nodeType => nodeType.Name == node.NodeTypeRef).First().LogicalDirectories;
                if (LogicalDirectory != null)
                {
                    LogicalDirectoriesSetForThisMachine.Add(LogicalDirectory);
                }
            }

            if (LogicalDirectoriesSetForThisMachine.Any())
            {
                foreach (LogicalDirectoryType[] LogicalDirectories in LogicalDirectoriesSetForThisMachine)
                {
                    foreach (LogicalDirectoryType dir in LogicalDirectories)
                    {
                        try
                        {
                            Helpers.DeleteDirectoryIfExist(dir.MappedTo, machineName);
                        }
                        catch (Exception e)
                        {
                            DeployerTrace.WriteError(StringResources.Error_FailedToRemoveLogicalDirectory, dir.MappedTo, machineName, e);
                        }
                    }
                }
            }
        }

        private static void RemoveNodeConfigurationXCopy(string machineName, bool deleteLog, string nodeName)
        {
            try
            {               
                string fabricDataRoot = Utility.GetFabricDataRoot(machineName);
                if (string.IsNullOrEmpty(fabricDataRoot))
                {
                    DeployerTrace.WriteWarning("FabricDataRoot on machine {0} is empty or not present; no current installation likely exists", machineName);
                }
                else
                {
                    DeployerTrace.WriteInfo("FabricDataRoot on machine {0} is {1}", machineName, fabricDataRoot);
                    FabricDeployerServiceController.GetServiceInStoppedState(machineName, Constants.FabricInstallerServiceName);
                    WriteTargetInformationFile(fabricDataRoot, machineName, deleteLog, nodeName);
                    RunInstallerService(machineName);
                }
                FabricDeployerServiceController.DeleteFabricInstallerService(machineName);
                Utility.DeleteRegistryKeyTree(machineName);
            }
            catch (TimeoutException)
            {
                DeployerTrace.WriteError("FabricInstallerSvc timeout on machine {0}. Cleaning up to avoid environment corruption.", machineName);
                SuppressExceptions(() => { FabricDeployerServiceController.StopHostSvc(machineName); });
                SuppressExceptions(() => { FabricDeployerServiceController.DeleteFabricHostService(machineName); });
                SuppressExceptions(() => { FabricDeployerServiceController.StopInstallerSvc(machineName); });
                SuppressExceptions(() => { FabricDeployerServiceController.DeleteFabricInstallerService(machineName); });
                SuppressExceptions(() => { Utility.DeleteRegistryKeyTree(machineName); });
                throw;
            }
        }

        private void RemoveNodeConfigurationMsi(bool deleteLog)
        {
            DeployerTrace.WriteInfo("Checking if fabric host is running");
            if (FabricDeployerServiceController.IsRunning(Helpers.GetMachine()))
            {
                DeployerTrace.WriteInfo("Stopping fabric host");
                FabricDeployerServiceController.StopHostSvc(Helpers.GetMachine());
            }

            string fabricCodePath = String.Empty;
            try
            {
                fabricCodePath = FabricEnvironment.GetCodePath();
            }
            catch (FabricException)
            {
                DeployerTrace.WriteError(StringResources.Error_FabricCodePathNotFound);
            }

            string fabricSetupFilepath = Path.Combine(fabricCodePath, "FabricSetup.exe");

            ProcessStartInfo FabricSetupExeStartInfo = new ProcessStartInfo();
            FabricSetupExeStartInfo.FileName = fabricSetupFilepath;
            FabricSetupExeStartInfo.Arguments = string.Format("/operation:removenodestate");
            FabricSetupExeStartInfo.WorkingDirectory = fabricCodePath == String.Empty ? Directory.GetCurrentDirectory() : fabricCodePath;

            try
            {
                using (Process exeProcess = Process.Start(FabricSetupExeStartInfo))
                {
                    DeployerTrace.WriteInfo("Starting FabricSetup.exe");
                    exeProcess.WaitForExit();
                }
            }
            catch (Exception e)
            {
                DeployerTrace.WriteError("Starting FabricSetup.exe failed with exception {0}.", e);
            }

            string value = deleteLog ? DeleteLogTrue : DeleteLogFalse;
            using (RegistryKey regKey = Registry.LocalMachine.OpenSubKey(FabricConstants.FabricRegistryKeyPath, true))
            {
                regKey.SetValue(Constants.Registry.RemoveNodeConfigurationValue, value);
            }

            DeployerTrace.WriteInfo("Starting fabric host");
            FabricDeployerServiceController.StartHostSvc();

            DeployerTrace.WriteInfo("Stopping fabric host");
            FabricDeployerServiceController.StopHostSvc();

            DeployerTrace.WriteInfo("Cleaning registry value");
            DeleteRemoveNodeConfigurationRegistryValue(string.Empty);

            DeployerTrace.WriteInfo("Done cleaning registry value");
        }

        private static void WriteTargetInformationFile(string fabricDataRoot, string machineName, bool deleteLog, string nodeName)
        {
            string targetInformationFileName = Path.Combine(fabricDataRoot, Constants.FileNames.TargetInformation);
            DeployerTrace.WriteNoise("targetInformationFileName is {0}", targetInformationFileName);
            if (!string.IsNullOrEmpty(machineName) && !machineName.Equals(Helpers.GetMachine()))
            {
                targetInformationFileName = Helpers.GetRemotePath(targetInformationFileName, machineName);
            }

            DeployerTrace.WriteInfo("Creating target information file {0} to be written on machine {1}", targetInformationFileName, machineName);
            TargetInformationType targetInformation = new TargetInformationType();
            targetInformation.CurrentInstallation = new WindowsFabricDeploymentInformation();
            targetInformation.CurrentInstallation.UndoUpgradeEntryPointExe = "FabricSetup.exe";
            targetInformation.CurrentInstallation.UndoUpgradeEntryPointExeParameters = string.Format("/operation:Uninstall {0}", deleteLog ? "/deleteLog:true" : "");
            targetInformation.CurrentInstallation.NodeName = nodeName;

            DeployerTrace.WriteInfo("Writing Target information file {0} on machine {1}", targetInformationFileName, machineName);
            XmlHelper.WriteXmlExclusive<TargetInformationType>(targetInformationFileName, targetInformation);
        }

        private static void RunInstallerService(string machineName)
        {
            var installerSvcStartedTimeout = TimeSpan.FromMinutes(Constants.FabricInstallerServiceStartTimeoutInMinutes);
            TimeSpan retryInterval = TimeSpan.FromSeconds(5);
            int retryCount = 20;

            bool isLocalIp = System.Fabric.Common.Helpers.IsLocalIpAddress(machineName);

            Task startInstallerTask = Task.Run(() =>
            {
                try
                {
                    Type[] exceptionTypes = { typeof(InvalidOperationException), typeof(ComponentModel.Win32Exception) };
                    Helpers.PerformWithRetry(() =>
                    {
                        // Each installerSvc handle/manager should be kept in its own thread context. Tossing this around leads to race condition failures in .NET.
                        ServiceController installerSvc = FabricDeployerServiceController.GetService(Constants.FabricInstallerServiceName, machineName);
                        installerSvc.WaitForStatus(ServiceControllerStatus.Running, installerSvcStartedTimeout);
                    },
                    exceptionTypes,
                    retryInterval,
                    retryCount);

                    DeployerTrace.WriteInfo("FabricInstallerSvc started on machine {0}", machineName);
                }
                catch (ServiceProcess.TimeoutException)
                {
                    throw new ServiceProcess.TimeoutException(string.Format("Timed out waiting for Installer Service to start for machine {0}.", machineName));
                }
            });

            {
                ServiceController installerSvc = FabricDeployerServiceController.GetService(Constants.FabricInstallerServiceName, machineName);
                if (isLocalIp)
                {
                    installerSvc.Start();
                }
                else
                {
                    installerSvc.Start(new string[] { Constants.FabricInstallerServiceAutocleanArg });
                }
            }
            startInstallerTask.Wait(installerSvcStartedTimeout); // Locks until service is started or reaches timeout

            DeployerTrace.WriteInfo("Waiting for FabricInstallerService to stop after completing Fabric Uninstallation");
            var waitTimeLimit = TimeSpan.FromMinutes(Constants.FabricUninstallTimeoutInMinutes);
            try
            {
                Type[] exceptionTypes = { typeof(InvalidOperationException), typeof(ComponentModel.Win32Exception) };
                Helpers.PerformWithRetry(() =>
                {
                    ServiceController installerSvc = FabricDeployerServiceController.GetService(Constants.FabricInstallerServiceName, machineName);
                    installerSvc.WaitForStatus(ServiceControllerStatus.Stopped, waitTimeLimit);
                },
                exceptionTypes,
                retryInterval,
                retryCount);
            }
            catch (System.ServiceProcess.TimeoutException)
            {
                DeployerTrace.WriteError("FabricInstallerService Stop timed out after {0}.", waitTimeLimit.ToString());
            }

            {
                ServiceController installerSvc = FabricDeployerServiceController.GetService(Constants.FabricInstallerServiceName, machineName);
                ServiceControllerStatus serviceStatus = installerSvc.Status;
                if (serviceStatus != ServiceControllerStatus.Stopped)
                {
                    string errorMessage = string.Format(CultureInfo.InvariantCulture,
                        "FabricInstallerService is in {0} state and hasn't returned to STOPPED state after uninstall timeout period",
                        serviceStatus.ToString());
                    DeployerTrace.WriteError(errorMessage);
                    throw new System.TimeoutException(errorMessage);
                }
            }
        }

        private delegate void ExceptionExpectedMethod();

        private static void SuppressExceptions(ExceptionExpectedMethod method)
        {
            try
            {
                method();
            }
            catch (Exception ex)
            {
                // Suppress any exception
                DeployerTrace.WriteWarning(String.Format("RemoveNodeConfigOperation Suppressed exception: {0}", ex));
            }
        }
    }
}
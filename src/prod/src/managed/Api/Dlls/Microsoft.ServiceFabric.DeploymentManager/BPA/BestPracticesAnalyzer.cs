// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.BPA
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.FabricDeployer;
    using System.Fabric.Strings;
    using System.IO;
    using System.Linq;
    using System.ServiceProcess;
    using System.Text.RegularExpressions;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Microsoft.ServiceFabric.DeploymentManager.Model;
    using Microsoft.Win32;

    using DMConstants = ServiceFabric.DeploymentManager.Constants;

    internal static class BestPracticesAnalyzer
    {
        public static bool ValidateClusterSetup(string jsonConfigPath)
        {
            AnalysisSummary result = AnalyzeClusterSetup(jsonConfigPath, null, null);
            return result.Passed;
        }

        public static Task<AnalysisSummary> AnalyzeClusterSetupAsync(string jsonConfigPath, string oldJsonConfigPath, string cabPath, bool isForcedRun, int maxPercentFailedNodes)
        {
            return Task.Run<AnalysisSummary>(
                       () => AnalyzeClusterSetup(jsonConfigPath, oldJsonConfigPath, cabPath, usingClusterManifest: false, isForcedRun: isForcedRun, maxPercentFailedNodes: maxPercentFailedNodes));
        }

        public static bool ValidateClusterRemovalXmlManifest(string xmlManifestPath)
        {
            return ValidateClusterRemoval(xmlManifestPath, usingClusterManifest: true);
        }

        internal static bool ValidateClusterSetupXmlManifest(string xmlManifestPath, FabricPackageType fabricPackageType)
        {
            AnalysisSummary result = AnalyzeClusterSetup(xmlManifestPath, oldConfigPath: null, cabPath: null, usingClusterManifest: true, fabricPackageType: fabricPackageType);
            return result.Passed;
        }

        internal static AnalysisSummary AnalyzeClusterSetup(
            string configPath,
            string oldConfigPath,
            string cabPath,
            bool usingClusterManifest = false,
            FabricPackageType fabricPackageType = FabricPackageType.XCopyPackage,
            bool isForcedRun = false,
            int maxPercentFailedNodes = 0)
        {
            SFDeployerTrace.WriteInfo(StringResources.Info_BPAStart);
            var summary = new AnalysisSummary();

            // Check user has local admin privilege
            summary.LocalAdminPrivilege = CheckLocalAdminPrivilege();

            // Validate JSON config
            StandAloneInstallerJsonModelBase standAloneModel = null;
            StandAloneInstallerJsonModelBase oldStandAloneModel = null;
            if (!usingClusterManifest)
            {
                standAloneModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(configPath);

                if (!string.IsNullOrEmpty(oldConfigPath))
                {
                    oldStandAloneModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(oldConfigPath);
                }

                summary.IsJsonValid = IsJsonConfigModelValid(standAloneModel, oldStandAloneModel, validateDownNodes: true, throwIfError: false) && ValidateCodeVersionExists(standAloneModel);
            }

            // Deliberately not checking empty. Some invocations aren't intended to test cabPath.
            if (cabPath != null)
            {
                summary.IsCabValid = CheckIsCabFile(cabPath);
            }

            // Below depends on JSON being valid
            if (!summary.Passed)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPABailing);
                return summary;
            }

            // Get machine names from JSON config
            IEnumerable<string> healthyMachineNames = usingClusterManifest
                                        ? StandaloneUtility.GetMachineNamesFromClusterManifest(configPath)
                                        : standAloneModel.GetClusterTopology().Machines;

            MachineHealthContainer machineHealthContainer = new MachineHealthContainer(healthyMachineNames, maxPercentFailedNodes);

            // Validate machine FQDNs, Check SMB ports opened
            summary.RequiredPortsOpen = StandaloneUtility.CheckRequiredPorts(machineHealthContainer);

            // Below depends on machines being reachable
            if (!summary.Passed)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPABailing);
                return summary;
            }

            // Validate Remote Registry Service is not disabled on all machines
            summary.RemoteRegistryAvailable = CheckRemoteRegistryEnabled(machineHealthContainer);

            // Below depends on remote registry service
            if (!summary.Passed)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPABailing);
                return summary;
            }

            summary.FirewallAvailable = CheckFirewallEnabled(machineHealthContainer);

            // Below depends on firewall service
            if (!summary.Passed)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPABailing);
                return summary;
            }

            // Run RPC check to validate end-to-end registry access to all machines
            summary.RpcCheckPassed = CheckRPCAccess(machineHealthContainer);

            // Below depend on remote registry access
            if (!summary.Passed)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPABailing);
                return summary;
            }

            summary.NoDomainController = CheckNoMachineIsDomainController(machineHealthContainer);

            // Below depends on machines not being domain controller
            if (!summary.Passed)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPABailing);
                return summary;
            }

            if (fabricPackageType == FabricPackageType.XCopyPackage)
            {
                // Validate that Fabric runtime MSI is not installed since this will be conflicting with Standalone
                summary.NoConflictingInstallations = !StandaloneUtility.IsMsiInstalled(machineHealthContainer);
            }

            string fabricDataRoot = null;
            string fabricLogRoot = null;
            if (!usingClusterManifest)
            {
                if (string.IsNullOrEmpty(oldConfigPath))
                {
                    summary.FabricInstallable = CheckForCleanInstall(standAloneModel, machineHealthContainer, isForcedRun); // Fabric is not installed on target machines
                }

                var importantSettings = standAloneModel.GetFabricSystemSettings();
                fabricDataRoot = importantSettings.ContainsKey(DMConstants.FabricDataRootString) ?
                    importantSettings[DMConstants.FabricDataRootString] :
                    null;
                fabricLogRoot = importantSettings.ContainsKey(DMConstants.FabricLogRootString) ?
                    importantSettings[DMConstants.FabricLogRootString] :
                    null;

                summary.DataDrivesAvailable = CheckDataSystemDrives(machineHealthContainer, fabricDataRoot, fabricLogRoot); // System drives for path-based settings, exist on target machines

                // Below depend on data system drives
                if (!summary.Passed)
                {
                    SFDeployerTrace.WriteError(StringResources.Error_BPABailing);
                    return summary;
                }
            }

            summary.DrivesEnoughAvailableSpace = CheckDrivesAvailableSpace(machineHealthContainer, fabricDataRoot, fabricLogRoot);

            // Below depend on root drives having enough space available
            if (!summary.Passed)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPABailing);
                return summary;
            }

            summary.IsAllOrNoneIOTDevice = CheckAllIOTDevices(machineHealthContainer);

            // Below depend on all or none machines being IOT Devices
            if (!summary.Passed)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPABailing);
                return summary;
            }

            // Check dotnet.exe exists in %Path%
            // Currently we only need to check for IOTCore environment.
            summary.DotNetExeInPath = IsDotNetExeInPath(machineHealthContainer);

            // Below depend on IOT Devices having dotnet.exe in path
            if (!summary.Passed)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPABailing);
                return summary;
            }

            summary.MachineHealthContainer = machineHealthContainer;
            LogResult(summary);

            return summary;
        }

        internal static bool ValidateClusterRemoval(
            string configPath,
            bool usingClusterManifest = false)
        {
            SFDeployerTrace.WriteInfo(StringResources.Info_BPAStart);
            var summary = new AnalysisSummary();

            // Check user has local admin privilege
            summary.LocalAdminPrivilege = CheckLocalAdminPrivilege();
            StandAloneInstallerJsonModelBase standAloneModel = null;
            if (!usingClusterManifest)
            {
                standAloneModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(configPath);
                summary.IsJsonValid = IsJsonConfigModelValid(standAloneModel, null, validateDownNodes: false, throwIfError: false);
            }

            // Below depends on JSON being valid
            if (!summary.Passed)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPABailing);
                return false;
            }

            // Get machine names from JSON config
            IEnumerable<string> machineNames = usingClusterManifest
                                        ? StandaloneUtility.GetMachineNamesFromClusterManifest(configPath)
                                        : standAloneModel.GetClusterTopology().Machines;

            MachineHealthContainer machineHealthContainer = new MachineHealthContainer(machineNames);

            // Log validations but don't fail
            StandaloneUtility.CheckRequiredPorts(machineHealthContainer);
            CheckRPCAccess(machineHealthContainer);

            // At least one machine should be removable
            bool anyMachinesRemovable = CheckAnyMachinesRemovable(machineNames);

            LogResult(summary, anyMachinesRemovable);

            return summary.Passed;
        }

        internal static bool ValidateAddNode(string machineName, string nodeName, string cabPath, string fabricDataRoot, string fabricLogRoot)
        {
            SFDeployerTrace.WriteInfo(StringResources.Info_BPAStart);

            //// TODO: (maburlik) Validate FabricClient connection

            var summary = new AnalysisSummary();

            // Check user has local admin privilege
            summary.LocalAdminPrivilege = CheckLocalAdminPrivilege();
            summary.IsCabValid = CheckIsCabFile(cabPath);

            // Below depends on JSON being valid
            if (!summary.Passed)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPABailing);
                return false;
            }

            IEnumerable<string> machineContainer = new List<string>()
            {
                machineName
            };
            MachineHealthContainer machineHealthContainer = new MachineHealthContainer(machineContainer);

            summary.RequiredPortsOpen = StandaloneUtility.CheckRequiredPorts(machineHealthContainer);

            // Below depends on machines being reachable
            if (!summary.Passed)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPABailing);
                return false;
            }

            summary.RpcCheckPassed = CheckRPCAccess(machineHealthContainer);

            // Below depend on remote registry access
            if (!summary.Passed)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPABailing);
                return false;
            }

            summary.NoDomainController = CheckNoMachineIsDomainController(machineHealthContainer);

            // Below depends on machine not being domain controller
            if (!summary.Passed)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPABailing);
                return false;
            }

            summary.NoConflictingInstallations = !StandaloneUtility.IsMsiInstalled(machineHealthContainer);
            summary.FabricInstallable = StandaloneUtility.CheckForCleanInstall(machineName, nodeName, fabricDataRoot);

            summary.DataDrivesAvailable = CheckDataSystemDrives(machineHealthContainer, fabricDataRoot, fabricLogRoot);

            // Below depend on data system drives
            if (!summary.Passed)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPABailing);
                return false;
            }

            summary.DrivesEnoughAvailableSpace = CheckDrivesAvailableSpace(machineHealthContainer, fabricDataRoot, fabricLogRoot);

            // Below depend on root drives having enough space available
            if (!summary.Passed)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPABailing);
                return false;
            }

            LogResult(summary);

            return summary.Passed;
        }     

        internal static bool IsJsonConfigModelValid(StandAloneInstallerJsonModelBase config, StandAloneInstallerJsonModelBase oldConfig, bool validateDownNodes, bool throwIfError = false)
        {
            try
            {
                config.ThrowValidationExceptionIfNull(StringResources.Error_BPAJsonModelInvalid);
                config.ValidateModel();
                var settingsValidator = new StandaloneSettingsValidator(config);
                settingsValidator.Validate(validateDownNodes);

                // UOS validates upgrade config diffs by calling ValidateUpdateFrom() method directly. Test-Configuration invokes IsJsonConfigModelValid() and it calls ValidateUpdateFrom inside.
                // The reason is from Test-Configuration, there is no cluster connection, so that we ignore ValidateTopologyAsync(). Therefore in the ValidateUpdateFrom here there is no need 
                // to await the async call. However in UOS, we should call ValidateUpdateFrom in an async manner. That's why I am not trying to have the UOS/TestConfiguration going down the same path.
                if (oldConfig != null)
                {
                    settingsValidator.ValidateUpdateFrom(oldConfig.GetUserConfig(), oldConfig.GetClusterTopology(), connectedToCluster: false).GetAwaiter().GetResult();
                }
            }
            catch (FileNotFoundException ex)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPAPackageFileNotFound, ex.ToString());
                return false;
            }
            catch (ValidationException ex)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPAModelSettingsValidationFailed, ex.GetMessage(System.Globalization.CultureInfo.InvariantCulture));
                if (throwIfError)
                {
                    throw;
                }

                return false;
            }

            return true;
        }

        internal static bool CheckAllIOTDevices(MachineHealthContainer machineHealthContainer)
        {
            int numOfIOTCores = StandaloneUtility.IOTDevicesCount(machineHealthContainer.GetHealthyMachineNames());
            return numOfIOTCores == 0 || numOfIOTCores == machineHealthContainer.GetHealthyMachineNames().Count();
        }

        // This function check whether dotnet exe exists in %PATH% on IOT device.
        // It will read %PATH% from registry and expand %systemroot% if found
        // Currently it doesn't support expanding other variables.
        internal static bool IsDotNetExeInPath(MachineHealthContainer machineHealthContainer)
        {
            bool result = true;

            Parallel.ForEach( 
                machineHealthContainer.GetHealthyMachineNames(),
                (string machineName) =>
                {
                    // Currently we only need to check for IOTCore environment
                    if (StandaloneUtility.IsIOTCore(machineName))
                    {
                        string remotePath = string.Empty;
                        string remoteSystemRoot = string.Empty;

                        // Read %Path% variable
                        using (RegistryKey regKey = StandaloneUtility.GetHklm(machineName))
                        {
                            RegistryKey envKey = regKey.OpenSubKey(DMConstants.EnvironmentRegKeyPath);
                            if (envKey != null)
                            {
                                remotePath = (string)envKey.GetValue(DMConstants.Path, string.Empty, RegistryValueOptions.DoNotExpandEnvironmentNames);
                            }

                            RegistryKey versionKey = regKey.OpenSubKey(DMConstants.IOTCurrentVersionRegKeyPath);
                            if (versionKey != null)
                            {
                                remoteSystemRoot = (string)versionKey.GetValue(DMConstants.SystemRoot, string.Empty);
                            }
                        }

                        bool found = false;
                        string[] paths = remotePath.Split(';');
                        foreach (string path in paths)
                        {
                            // Replace %SystemRoot%
                            string absolutePath = Regex.Replace(path, "%SYSTEMROOT%", remoteSystemRoot, RegexOptions.IgnoreCase);
                            absolutePath = Path.Combine(absolutePath, "dotnet.exe");
                            string dotNetExePath = Helpers.GetRemotePathIfNotLocalMachine(absolutePath, machineName);
                            if (File.Exists(dotNetExePath))
                            {
                                found = true;
                                break;
                            }
                        }

                        if (!found)
                        {
                            result = false;         
                        }
                    }                                       
                });

            return result;
        }

        internal static bool ValidateRemoveNode(string machineName)
        {
            SFDeployerTrace.WriteInfo(StringResources.Info_BPAStart);

            //// TODO: (maburlik) Validate FabricClient connection

            var summary = new AnalysisSummary();
            summary.LocalAdminPrivilege = CheckLocalAdminPrivilege();

            IEnumerable<string> machineContainer = new List<string>()
            {
                machineName
            };
            MachineHealthContainer machineHealthContainer = new MachineHealthContainer(machineContainer);
            summary.RequiredPortsOpen = StandaloneUtility.CheckRequiredPorts(machineHealthContainer);

            // Below depends on machines being reachable
            if (!summary.Passed)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPABailing);
                return false;
            }

            summary.RpcCheckPassed = CheckRPCAccess(machineHealthContainer);

            // Below depend on remote registry access
            if (!summary.Passed)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPABailing);
                return false;
            }

            bool hasFabricInstalled = StandaloneUtility.IsFabricInstalled(machineName);
            if (!hasFabricInstalled)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPARemoveNodeNeedsFabric, machineName);
            }

            LogResult(summary, hasFabricInstalled);

            return summary.Passed && hasFabricInstalled;
        }

        private static bool CheckLocalAdminPrivilege()
        {
            SFDeployerTrace.WriteNoise(StringResources.Info_BPAValidatingAdmin);
            bool result = AccountHelper.IsAdminUser();
            if (!result)
            {
                SFDeployerTrace.WriteError(StringResources.Error_SFPreconditionsAdminUserRequired);
            }

            return result;
        }

        private static bool CheckIsCabFile(string cabPath)
        {
            SFDeployerTrace.WriteNoise(StringResources.Info_BPAValidatingCab, cabPath);
            bool result = CabFileOperations.IsCabFile(cabPath);
            if (!result)
            {
                SFDeployerTrace.WriteError(StringResources.Error_SFCabInvalid, cabPath);
                return result;
            }

            result = FileSignatureVerifier.IsSignatureValid(cabPath);
            if (!result)
            {
                SFDeployerTrace.WriteError(StringResources.Error_InvalidCodePackage);
            }

            // Add Signature Validation.
            return result;
        }

        private static bool CheckRemoteRegistryEnabled(MachineHealthContainer machineHealthContainer)
        {
            SFDeployerTrace.WriteNoise(StringResources.Info_BPARemoteRegistry);
            Parallel.ForEach(
                machineHealthContainer.GetHealthyMachineNames(),
                (string machineName) =>
                {
                    bool result = true;
                    int retryCount = 5;
                    for (int i = 0; i < retryCount; i++)
                    {
                        try
                        {
                            FabricDeployerServiceController.ServiceStartupType type =
                            FabricDeployerServiceController.GetServiceStartupType(machineName, DMConstants.RemoteRegistryServiceName);
                            if (type == FabricDeployerServiceController.ServiceStartupType.Disabled)
                            {
                                SFDeployerTrace.WriteError(StringResources.Error_BPARemoteRegistryServiceDisabled, machineName);
                                result = false;
                            }

                            break;
                        }
                        catch (Exception ex)
                        {
                            SFDeployerTrace.WriteError(StringResources.Error_BPARemoteRegistryQueryException, machineName, ex.Message, i);

                            if (i < retryCount - 1)
                            {
                                Thread.Sleep(TimeSpan.FromSeconds(10));
                            }
                            else
                            {
                                result = false;
                            }
                        }
                    }

                    if (!result)
                    {
                        machineHealthContainer.MarkMachineAsUnhealthy(machineName);
                    }
                });

            return machineHealthContainer.EnoughHealthyMachines();
        }

        private static bool CheckFirewallEnabled(MachineHealthContainer machineHealthContainer)
        {
            SFDeployerTrace.WriteNoise(StringResources.Info_BPAWindowsFirewall);
            Parallel.ForEach(
                machineHealthContainer.GetHealthyMachineNames(),
                (string machineName) =>
                {
                    bool result = true;
                    try
                    {
                        FabricDeployerServiceController.ServiceStartupType type =
                        FabricDeployerServiceController.GetServiceStartupType(machineName, DMConstants.FirewallServiceName);
                        if (type == FabricDeployerServiceController.ServiceStartupType.Disabled)
                        {
                            SFDeployerTrace.WriteError(StringResources.Error_BPAFirewallServiceDisabled, machineName);
                            result = false;
                        }
                    }
                    catch (Exception ex)
                    {
                        SFDeployerTrace.WriteError(StringResources.Error_BPAFirewallServiceQueryException, machineName, ex.Message);
                        result = false;
                    }

                    try
                    {
                        ServiceController firewallSvc = FabricDeployerServiceController.GetService(DMConstants.FirewallServiceName, machineName);
                        ServiceControllerStatus status = firewallSvc.Status;
                        if (status == ServiceControllerStatus.Stopped || status == ServiceControllerStatus.StopPending)
                        {
                            SFDeployerTrace.WriteError(StringResources.Error_BPAFirewallServiceNotRunning, machineName, status.ToString());
                            result = false;
                        }
                    }
                    catch (Exception ex)
                    {
                        SFDeployerTrace.WriteError(StringResources.Error_BPAFirewallServiceStatusException, machineName, ex.Message);
                        result = false;
                    }

                    if (!result)
                    {
                        machineHealthContainer.MarkMachineAsUnhealthy(machineName);
                    }
                });

            return machineHealthContainer.EnoughHealthyMachines();
        }

        private static bool CheckRPCAccess(MachineHealthContainer machineHealthContainer)
        {
            var retryTimeout = new System.Fabric.Common.TimeoutHelper(DMConstants.BpaRpcRetryTimeout);
            SFDeployerTrace.WriteNoise(StringResources.Info_SFRpcInfo);

            Parallel.ForEach<string>(
                machineHealthContainer.GetHealthyMachineNames(),
                (string machine) =>
                {
                    bool result = true;
                    bool willRetry;

                    do
                    {
                        willRetry = false;

                        try
                        {
                            Utility.GetTempPath(machine);
                        }
                        catch (Exception ex)
                        {
                            string message;
                            if (ex is System.IO.IOException)
                            {
                                switch (ex.HResult)
                                {
                                    // If new failures are discovered: https://msdn.microsoft.com/en-us/library/windows/desktop/ms681382(v=vs.85).aspx
                                    case 53: // ERROR_BAD_NETPATH
                                        message = string.Format(StringResources.Error_SFRpcIoNetpath, machine, ex.HResult);
                                        willRetry = true;
                                        break;
                                    case 1723: // RPC_S_SERVER_TOO_BUSY
                                        message = string.Format(StringResources.Error_SFRpcIoTooBusy, machine, ex.HResult);
                                        willRetry = true;
                                        break;
                                    case 1727: // RPC_S_CALL_FAILED_DNE
                                        message = string.Format(StringResources.Error_SFRpcIoFailedDne, machine, ex.HResult);
                                        break;
                                    default:
                                        message = string.Format(StringResources.Error_SFRpcIoGeneric, machine, ex.HResult);
                                        break;
                                }
                            }
                            else if (ex is System.Security.SecurityException)
                            {
                                switch (ex.HResult)
                                {
                                    case -2146233078: // COR_E_SECURITY
                                        message = string.Format(StringResources.Error_SFRpcSecAccess, machine, ex.HResult);
                                        break;
                                    default:
                                        message = string.Format(StringResources.Error_SFRpcSecGeneric, machine, ex.HResult);
                                        break;
                                }
                            }
                            else if (ex is NullReferenceException)
                            {
                                switch (ex.HResult)
                                {
                                    case -2146232828: // COR_E_TARGETINVOCATION
                                        message = string.Format(StringResources.Error_SFRpcNullRegAccess, machine, ex.HResult);
                                        break;
                                    default:
                                        message = string.Format(StringResources.Error_SFRpcNullGeneric, machine, ex.HResult);
                                        break;
                                }
                            }
                            else
                            {
                                // This is to catch coding errors.
                                message = string.Format(StringResources.Error_SFRpcGeneric, machine, ex.HResult);
                            }

                            willRetry &= !System.Fabric.Common.TimeoutHelper.HasExpired(retryTimeout);

                            if (willRetry)
                            {
                                SFDeployerTrace.WriteWarning(message);

                                StandaloneUtility.OpenRemoteRegistryNamedPipe(machine, retryTimeout.GetRemainingTime());

                                Thread.Sleep(TimeSpan.FromSeconds(5));
                            }
                            else
                            {
                                SFDeployerTrace.WriteError(message);

                                result = false;
                            }
                        }
                    }
                    while (willRetry);

                    if (!result)
                    {
                        machineHealthContainer.MarkMachineAsUnhealthy(machine);
                    }
                });

            return machineHealthContainer.EnoughHealthyMachines();
        }

        // Clean install requires machine be free of previous Fabric installations
        private static bool CheckForCleanInstall(StandAloneInstallerJsonModelBase config, MachineHealthContainer machineHealthContainer, bool isForcedRun = false)
        {
            SFDeployerTrace.WriteNoise(StringResources.Info_BPANoFabric);

            List<string> machineNamesTemp = StandaloneUtility.GetMachineNamesIncludingClient(machineHealthContainer.GetHealthyMachineNames());
            var importantSettings = config.GetFabricSystemSettings();
            string fabricDataRoot = importantSettings.ContainsKey(DMConstants.FabricDataRootString) ?
                importantSettings[DMConstants.FabricDataRootString] :
                null;

            bool localMachineFailed = false;
            Parallel.ForEach(
                machineNamesTemp,
                (string machineName) =>
                {
                    bool result = true;
                    if (StandaloneUtility.IsFabricInstalled(machineName))
                    {
                        SFDeployerTrace.WriteError(StringResources.Error_BPAPreviousFabricExists, machineName);
                        result = false;
                    }

                    if (!isForcedRun)
                    {
                        if (fabricDataRoot != null)
                        {
                            IEnumerable<string> machineNodes = config.Nodes.Where(n => n.IPAddress == machineName).Select(n => n.NodeName);
                            foreach (string node in machineNodes)
                            {
                                string nodeDirectory;
                                if (StandaloneUtility.DataRootNodeExists(machineName, node, fabricDataRoot, out nodeDirectory))
                                {
                                    SFDeployerTrace.WriteError(StringResources.Error_BPADataRootNodeExists, node, machineName, nodeDirectory);
                                    result = false;
                                }
                            }
                        }
                    }

                    if (!result)
                    {
                        if (Helpers.IsLocalIpAddress(machineName))
                        {
                            localMachineFailed = true;
                        }
                        else
                        {
                            machineHealthContainer.MarkMachineAsUnhealthy(machineName);
                        }
                    }
                });

            if (localMachineFailed)
            {
                return false;
            }

            return machineHealthContainer.EnoughHealthyMachines();
        }

        private static bool CheckDataSystemDrives(MachineHealthContainer machineHealthContainer, string fabricDataRoot, string fabricLogRoot)
        {
            bool systemDataDriveExists = true;
            bool systemLogDriveExists = true;
            if (fabricDataRoot != null)
            {
                // Verify path system drive exists on each machine to be deployed
                Parallel.ForEach(
                    machineHealthContainer.GetHealthyMachineNames(),
                    (string machineName) =>
                    {
                        try
                        {
                            string remotePath = Helpers.GetRemotePathIfNotLocalMachine(fabricDataRoot, machineName);
                            var info = new DirectoryInfo(remotePath);

                            if (!info.Root.Exists)
                            {
                                SFDeployerTrace.WriteError(StringResources.Error_BPAJsonDataRootRootDriveDoesNotExist, DMConstants.FabricDataRootString, machineName);
                                systemDataDriveExists = false;
                                machineHealthContainer.MarkMachineAsUnhealthy(machineName);
                            }
                        }
                        catch (Exception ex)
                        {
                            SFDeployerTrace.WriteNoise(StringResources.Error_BPAJsonDataRootRootDriveQueryException, DMConstants.FabricDataRootString, ex);
                            systemDataDriveExists = false;
                            machineHealthContainer.MarkMachineAsUnhealthy(machineName);
                        }
                    });

                if (!systemDataDriveExists)
                {
                    SFDeployerTrace.WriteError(StringResources.Error_BPAJsonDataRootRootDriveDoesNotExistGeneric, DMConstants.FabricDataRootString);
                }
            }

            if (fabricLogRoot != null)
            {
                Parallel.ForEach(
                    machineHealthContainer.GetHealthyMachineNames(),
                    (string machineName) =>
                    {
                        try
                        {
                            string remotePath = Helpers.GetRemotePathIfNotLocalMachine(fabricLogRoot, machineName);
                            var info = new DirectoryInfo(remotePath);
                            if (!info.Root.Exists)
                            {
                                SFDeployerTrace.WriteError(StringResources.Error_BPAJsonDataRootRootDriveDoesNotExist, DMConstants.FabricLogRootString, machineName);
                                systemLogDriveExists = false;
                                machineHealthContainer.MarkMachineAsUnhealthy(machineName);
                            }
                        }
                        catch (Exception ex)
                        {
                            SFDeployerTrace.WriteError(StringResources.Error_BPAJsonDataRootRootDriveQueryException, DMConstants.FabricLogRootString, ex);
                            systemLogDriveExists = false;
                            machineHealthContainer.MarkMachineAsUnhealthy(machineName);
                        }
                    });

                if (!systemLogDriveExists)
                {
                    SFDeployerTrace.WriteError(StringResources.Error_BPAJsonDataRootRootDriveDoesNotExistGeneric, DMConstants.FabricLogRootString);
                }
            }

            return machineHealthContainer.EnoughHealthyMachines();
        }

        private static bool CheckDrivesAvailableSpace(MachineHealthContainer machineHealthContainer, string fabricDataRoot, string fabricLogRoot)
        {
            List<string> drives = new List<string>();
            string candidateMachine = machineHealthContainer.GetHealthyMachineNames().ElementAt(0);
            string fabricPackageDestinationPath = Utility.GetDefaultPackageDestination(candidateMachine);
            string fabricRootDrive = Path.GetPathRoot(fabricPackageDestinationPath);
            drives.Add(fabricRootDrive);

            if (fabricDataRoot != null)
            {
                drives.Add(Path.GetPathRoot(fabricDataRoot));
            }

            if (fabricLogRoot != null)
            {
                drives.Add(Path.GetPathRoot(fabricLogRoot));
            }

            foreach (string drive in drives.Distinct())
            {
                Parallel.ForEach(
                    machineHealthContainer.GetHealthyMachineNames(),
                    (string machineName) =>
                    {
                        string remoteRootDrive = Helpers.GetRemotePathIfNotLocalMachine(drive, machineName);
                        if (!StandaloneUtility.EnoughAvailableSpaceOnDrive(remoteRootDrive))
                        {
                            SFDeployerTrace.WriteError(StringResources.Error_BPANotEnoughSpaceOnDrive, drive, machineName);
                            machineHealthContainer.MarkMachineAsUnhealthy(machineName);
                        }
                    });
            }

            return machineHealthContainer.EnoughHealthyMachines();
        }

        private static bool CheckAnyMachinesRemovable(IEnumerable<string> machineNames)
        {
            bool result = false;
            Parallel.ForEach(
                machineNames,
                (string machineName) =>
                {
                    if (StandaloneUtility.IsFabricInstalled(machineName))
                    {
                        result = true;
                    }
                });

            if (!result)
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPANoMachinesHaveFabricInstalled);
            }

            return result;
        }

        private static bool CheckNoMachineIsDomainController(MachineHealthContainer machineHealthContainer)
        {
            Parallel.ForEach(
               machineHealthContainer.GetHealthyMachineNames(),
               (string machineName) =>
               {
                   bool result = true;
                   try
                   {
                       if (StandaloneUtility.IsMachineDomainController(machineName))
                       {
                           SFDeployerTrace.WriteError(StringResources.Error_BPAMachineIsDomainController, machineName);
                           result = false;
                       }
                   }
                   catch (System.ComponentModel.Win32Exception ex)
                   {
                       SFDeployerTrace.WriteError(StringResources.Error_BPADomainControllerQueryException, machineName, ex.NativeErrorCode, ex.Message);
                       result = false;
                   }

                   if (!result)
                   {
                       machineHealthContainer.MarkMachineAsUnhealthy(machineName);
                   }
               });

            return machineHealthContainer.EnoughHealthyMachines();
        }

        private static void LogResult(AnalysisSummary summary, bool additionalPredicate = true)
        {
            if (summary.Passed && additionalPredicate)
            {
                SFDeployerTrace.WriteInfo(StringResources.Info_BPAEndSuccess);
            }
            else
            {
                SFDeployerTrace.WriteError(StringResources.Info_BPAEndFail);
            }
        }

        private static bool ValidateCodeVersionExists(StandAloneInstallerJsonModelBase config)
        {
            var userConfig = config.GetUserConfig();
            if (!string.IsNullOrEmpty(userConfig.CodeVersion))
            {
                Version ver;
                if (userConfig.CodeVersion != DMConstants.AutoupgradeCodeVersion && !Version.TryParse(userConfig.CodeVersion, out ver))
                {
                    SFDeployerTrace.WriteError(StringResources.Error_SFCodeVersionUnsupportedForDeployment);
                    return false;
                }
            }

            return true;
        }
    }
}
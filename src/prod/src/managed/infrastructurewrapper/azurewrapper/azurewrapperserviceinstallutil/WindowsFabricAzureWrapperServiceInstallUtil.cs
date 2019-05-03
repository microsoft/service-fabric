// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Fabric.InfrastructureWrapper
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Reflection;
    using System.ServiceProcess;
    using AzureUploaderCommon;
    using Microsoft.WindowsAzure.ServiceRuntime;
    using Win32;
    
    public class Program
    {
        private const int WaitForServiceRunningTimeoutMinutes = 3;
        private const int WaitForInstallationTimeoutMinutes = 5;
        private const int SuccessExitCode = 0;
        private const int FailureExitCode = 1;

        // utility to install, configure and start WindowsFabric AzureWrapper service.
        // it is executed as a simple startup task. In case of failures during installation, it will directly exit and get restarted. 
        public static int Main(string[] args)
        {
            InitializeLogging();
            
            try
            {
                string pluginDirectory = Path.GetDirectoryName(Assembly.GetEntryAssembly().Location);

                if (RoleEnvironment.IsEmulated)
                {
                    if (!TryStartProcess(pluginDirectory))
                    {
                        return FailureExitCode;
                    }
                }
                else
                {
                    // Write azure environment details to registry 
                    if (!TryWriteAzureEnvironmentToRegistry())
                    {
                        return FailureExitCode;
                    }

                    // install VC 11 runtime
                    if (!TryInstallVcRuntime(pluginDirectory))
                    {
                        return FailureExitCode;
                    }

                    // uninstall any previously installed version before installing to enable upgrade of AzureWrapper service bits
                    if (!UninstallService(pluginDirectory))
                    {
                        return FailureExitCode;
                    }

                    if (!InstallAndStartService(pluginDirectory))
                    {
                        return FailureExitCode;
                    }
                }
            }
            catch (Exception e)
            {
                LogError("Exception occurred when installing and starting Windows Fabric AzureWrapper Service : {0}", e);

                return FailureExitCode;
            }

            return SuccessExitCode;
        }

        // installation utility would only log to Event Log (Log : Application, Event Source: WindowsFabricAzureWrapperServiceInstallationUtility)
        // since other logging dependencies have not been initialized at this point 
        private static void InitializeLogging()
        {
            if (!EventLog.SourceExists(WindowsFabricAzureWrapperServiceCommon.WindowsFabricAzureWrapperServiceInstallUtilEventSource))
            {
                EventLog.CreateEventSource(WindowsFabricAzureWrapperServiceCommon.WindowsFabricAzureWrapperServiceInstallUtilEventSource, null);
            }

            if (!EventLog.SourceExists(WindowsFabricAzureWrapperServiceCommon.WindowsFabricAzureWrapperServiceName))
            {
                EventLog.CreateEventSource(WindowsFabricAzureWrapperServiceCommon.WindowsFabricAzureWrapperServiceName, null);
            }
        }

        private static bool UninstallService(string pluginDirectory)
        {
            LogInfo("Uninstalling any previously installed version of Windows Fabric AzureWrapper Service.");

            ServiceController service = GetInstalledService();

            if (service == null)
            {
                LogInfo("Windows Fabric AzureWrapper Service is not installed. Skip uninstalling.");

                return true;
            }

            string serviceBinaryPath = Path.Combine(pluginDirectory, WindowsFabricAzureWrapperServiceCommon.WindowsFabricAzureWrapperBinaryName);

            LogInfo("Uninstalling Windows Fabric AzureWrapper Service from {0}", serviceBinaryPath);

            int exitCode = WindowsFabricAzureWrapperServiceCommon.UninstallService(serviceBinaryPath);

            LogInfo("Uninstallation returned with exit code {0}.", exitCode);

            service = GetInstalledService();

            if (service == null)
            {
                LogInfo("Successfully uninstalled Windows Fabric AzureWrapper Service.");

                return true;
            }

            LogError("Failed to uninstall Windows Fabric AzureWrapper Service.");

            return false;
        }

        private static bool InstallAndStartService(string pluginDirectory)
        {
            string serviceBinaryPath = Path.Combine(pluginDirectory, WindowsFabricAzureWrapperServiceCommon.WindowsFabricAzureWrapperBinaryName);

            LogInfo("Installing Windows Fabric AzureWrapper Service from {0}", serviceBinaryPath);

            int exitCode = WindowsFabricAzureWrapperServiceCommon.InstallService(serviceBinaryPath);

            LogInfo("Installation returned with exit code {0}.", exitCode);

            ServiceController service = GetInstalledService();

            if (service == null || !SetServiceRecoveryOptions())
            {
                LogError("Failed to install Windows Fabric AzureWrapper Service and set recovery options for it.");

                return false;
            }

            if (service.Status != ServiceControllerStatus.Running)
            {
                if (service.Status != ServiceControllerStatus.StartPending)
                {
                    LogInfo("Windows Fabric AzureWrapper Service is not started yet. Starting it.");

                    service.Start();
                }

                service.WaitForStatus(ServiceControllerStatus.Running, TimeSpan.FromMinutes(WaitForServiceRunningTimeoutMinutes));
            }

            if (service.Status == ServiceControllerStatus.Running)
            {
                LogInfo("Successfully started Windows Fabric AzureWrapper Service.");

                return true;
            }

            LogError("Failed to start Windows Fabric AzureWrapper Service.");

            return false;
        }

        // WindowsFabric AzureWrapper service should be restarted immediately upon failures 
        private static bool SetServiceRecoveryOptions()
        {
            string command = Environment.ExpandEnvironmentVariables(@"%WINDIR%\System32\Sc.exe");
            string arguments = string.Format(CultureInfo.InvariantCulture, "Failure {0} Reset= 0 Actions= Restart/0/Restart/0/Restart/0", WindowsFabricAzureWrapperServiceCommon.WindowsFabricAzureWrapperServiceName);

            LogInfo("Setting recovery options for WindowsFabricAzureWrapperService with command {0} {1}", command, arguments);

            int exitCode = WindowsFabricAzureWrapperServiceCommon.ExecuteCommand(command, arguments);

            LogInfo("Setting recovery options for WindowsFabricAzureWrapperService returns with exit code {0}", exitCode);

            return exitCode == 0;
        }

        private static ServiceController GetInstalledService()
        {
            return WindowsFabricAzureWrapperServiceCommon.GetInstalledService(WindowsFabricAzureWrapperServiceCommon.WindowsFabricAzureWrapperServiceName);
        }
        
        private static bool TryWriteAzureEnvironmentToRegistry()
        {
            return TryWriteEnvironmentVariableToRegistry("RoleInstanceID", (id) => AzureRegistryStore.RoleInstanceId = id)
                && TryWriteEnvironmentVariableToRegistry("RoleName", (name) => AzureRegistryStore.RoleName = name)
                && TryWriteEnvironmentVariableToRegistry("RoleDeploymentID", (id) => AzureRegistryStore.DeploymentId = id);
        }

        private static bool TryWriteEnvironmentVariableToRegistry(string envVariableName, Action<string> registryWriter)
        {
            try
            {
                string envValue = Environment.GetEnvironmentVariable(envVariableName);

                if (string.IsNullOrEmpty(envValue))
                {
                    LogWarning(
                        "Environment variable {0} was null or empty.",
                        envVariableName);
                    return false;
                }

                LogInfo(
                    "Writing {0} {1} to registry",
                    envVariableName,
                    envValue);

                registryWriter(envValue);
            }
            catch (Exception e)
            {
                LogWarning(
                    "Exception occurred when writing {0} to registry : {1}",
                    envVariableName,
                    e);

                return false;
            }

            return true;
        }

        private static bool TryInstallVcRuntime(string pluginDirectory)
        {
            try
            {
                string installationCommand = Path.Combine(pluginDirectory, WindowsFabricAzureWrapperServiceCommon.Vc11RuntimeInstallerBinaryName);
                string arguments = "/q /norestart";

                LogInfo("Installing VC 11 runtime with command {0} {1}", installationCommand, arguments);

                int exitCode = WindowsFabricAzureWrapperServiceCommon.ExecuteCommand(installationCommand, arguments, TimeSpan.FromMinutes(WaitForInstallationTimeoutMinutes));

                LogInfo("Installing VC 11 runtime returns with exit code {0}", exitCode);

                return exitCode == 0;
            }
            catch (Exception e)
            {
                LogError("Exception occurred when installing VC 11 runtime : {0}", e);

                return false;
            }
        }

        private static bool TryStartProcess(string pluginDirectory)
        {
            try
            {
                int currentRoleInstanceIndex = GetRoleInstanceIndex(RoleEnvironment.CurrentRoleInstance);
                string currentNodeName = string.Format(CultureInfo.InvariantCulture, "{0}.{1}", RoleEnvironment.CurrentRoleInstance.Role.Name, currentRoleInstanceIndex);
                string minNodeName = GetMinNodeName();

                if (string.IsNullOrEmpty(minNodeName))
                {
                    LogError("No roles have defined Windows Fabric endpoints.");

                    return false;
                }

                // only the role instance with the minimum node name in form of roleName.roleInstanceIndex will run WindowsFabric AzureWrapper Service as a process to start a scale-min WindowsFabric cluster
                if (string.Compare(currentNodeName, minNodeName, StringComparison.OrdinalIgnoreCase) == 0)
                {
                    string serviceBinaryPath = Path.Combine(pluginDirectory, WindowsFabricAzureWrapperServiceCommon.WindowsFabricAzureWrapperBinaryName);
                    string arguments = string.Empty;

                    LogInfo("Starting Windows Fabric AzureWrapper Service as a process from {0}", serviceBinaryPath);

                    bool succeeded = WindowsFabricAzureWrapperServiceCommon.TryStartProcess(serviceBinaryPath, arguments);

                    LogInfo("Starting Windows Fabric AzureWrapper Service as a process {0}", succeeded ? "succeeded" : "failed");

                    return succeeded;
                }

                return true;
            }
            catch (Exception e)
            {
                LogError("Running occurred when starting Windows Fabric AzureWrapper Service as a process : {0}", e);

                return false;
            }
        }

        private static string GetMinNodeName()
        {
            string minNodeName = null;
            foreach (KeyValuePair<string, Role> role in RoleEnvironment.Roles)
            {
                foreach (RoleInstance roleInstance in role.Value.Instances)
                {
                     // Only count Windows Fabric roles when trying to determine min node name 
                     // Whether a role is a WindowsFabric role is determined by whether it has an endpoint named 'NodeAddress'
                     if (!roleInstance.InstanceEndpoints.ContainsKey(WindowsFabricAzureWrapperServiceCommon.NodeAddressEndpointName))
                     {
                         continue;
                     }

                     int roleInstanceIndex = GetRoleInstanceIndex(roleInstance);
                     string nodeName = string.Format(CultureInfo.InvariantCulture, "{0}.{1}", roleInstance.Role.Name, roleInstanceIndex);
                     
                     // Any string, including the empty string (""), compares greater than a null reference
                     if (string.IsNullOrEmpty(minNodeName) || string.Compare(nodeName, minNodeName, StringComparison.OrdinalIgnoreCase) < 0)
                     {
                         minNodeName = nodeName;
                     }
                }
            }

            return minNodeName;
        }

        private static int GetRoleInstanceIndex(RoleInstance roleInstance)
        {
            // Get the index from role instance id e.g. workerRole1_IN_0 -> 0
            string[] tokens = roleInstance.Id.Split('_');
            return int.Parse(tokens[tokens.Length - 1], CultureInfo.InvariantCulture);
        }

        private static void LogInfo(string format, params object[] args)
        {
            string message = string.Format(CultureInfo.InvariantCulture, format, args);
            EventLog.WriteEntry(WindowsFabricAzureWrapperServiceCommon.WindowsFabricAzureWrapperServiceInstallUtilEventSource, message, EventLogEntryType.Information);
        }

        private static void LogWarning(string format, params object[] args)
        {
            string message = string.Format(CultureInfo.InvariantCulture, format, args);
            EventLog.WriteEntry(WindowsFabricAzureWrapperServiceCommon.WindowsFabricAzureWrapperServiceInstallUtilEventSource, message, EventLogEntryType.Warning);
        }

        private static void LogError(string format, params object[] args)
        {
            string message = string.Format(CultureInfo.InvariantCulture, format, args);
            EventLog.WriteEntry(WindowsFabricAzureWrapperServiceCommon.WindowsFabricAzureWrapperServiceInstallUtilEventSource, message, EventLogEntryType.Error);
        }
    }
}
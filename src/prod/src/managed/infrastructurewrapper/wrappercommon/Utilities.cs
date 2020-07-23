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
    using System.Linq;
    using System.Net;
    using System.ServiceProcess;
    using Win32;

    internal class Utilities
    {
        private const string LogTag = "Utilities";
        
        public static int ExecuteCommand(string command, string arguments)
        {
            int ret = ExecuteCommand(command, arguments, Directory.GetCurrentDirectory());

            if (ret == 0)
            {
                Logger.LogInfo(LogTag, "Successfully executed {0} with params {1}", command, arguments);
            }
            else
            {
                Logger.LogError(LogTag, "Running {0} failed with params {1} and error code {2}", command, arguments, ret);
            }

            return ret;
        }

	[System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.WebSecurity", "CA3006:ProcessCommandExecutionRule")] 
        public static int ExecuteCommand(string command, string arguments, string workingDir)
        {
            Logger.LogInfo(LogTag, "Starting Process {0} {1} in {2}", command, arguments, workingDir);
            using (Process p = new Process())
            {
                p.StartInfo.FileName = command;
                p.StartInfo.Arguments = arguments;
                p.StartInfo.UseShellExecute = false;
                p.StartInfo.CreateNoWindow = true;
                p.StartInfo.WorkingDirectory = workingDir;
                p.Start();
                p.WaitForExit();
                return p.ExitCode;
            }
        }

        public static string GetFabricDeployerPath()
        {
            string codePath = GetFabricCodePath();
            return codePath == null ? null : Path.Combine(codePath, Constants.FabricDeployerName);
        }

        public static void SetFabricRoot(string path)
        {
            Registry.SetValue(Constants.FabricInstalledRegPath, Constants.FabricRootRegKey, path);
        }

        public static string GetFabricRoot()
        {
            string fabricRoot = (string)Registry.GetValue(Constants.FabricInstalledRegPath, Constants.FabricRootRegKey, string.Empty);

            if (string.IsNullOrEmpty(fabricRoot))
            {
                fabricRoot = (string)Registry.GetValue(Constants.FabricInstalledRegPathDepreciated, Constants.FabricRootRegKey, string.Empty);
            }

            return fabricRoot;
        }

        public static string GetFabricBinRoot()
        {
            string fabricBinRoot = (string)Registry.GetValue(Constants.FabricInstalledRegPath, Constants.FabricBinRootRegKey, string.Empty);

            if (string.IsNullOrEmpty(fabricBinRoot))
            {
                fabricBinRoot = (string)Registry.GetValue(Constants.FabricInstalledRegPathDepreciated, Constants.FabricBinRootRegKey, string.Empty);
            }

            return fabricBinRoot;
        }

        public static void SetFabricCodePath(string path)
        {
            Registry.SetValue(Constants.FabricInstalledRegPath,Constants.FabricCodePathRegKey,path);
        }

        public static string GetFabricCodePath()
        {
            string fabricCodePath = (string)Registry.GetValue(Constants.FabricInstalledRegPath, Constants.FabricCodePathRegKey, string.Empty);

            if (string.IsNullOrEmpty(fabricCodePath))
            {
                fabricCodePath = (string)Registry.GetValue(Constants.FabricInstalledRegPathDepreciated, Constants.FabricCodePathRegKey, string.Empty);
            }

            return fabricCodePath;
        }

        public static int MSIExec(string param)
        {
            return ExecuteCommand("msiexec.exe", param);
        }

        public static int UnCabFile(string filepath, string destinationDirectory)
        {
            string decabAllFileParams = string.Format(
                    CultureInfo.InvariantCulture,
                    Constants.UnCabOptionForAllFiles, filepath,destinationDirectory);
            return ExecuteCommand("expand.exe", decabAllFileParams);
        }

        public static bool IsWindowsFabricInstalled()
        {            
            string version = GetCurrentWindowsFabricVersion();
            if (version != null)
            {
                Logger.LogInfo(LogTag, "Windows Fabric version {0} already installed on the system.", version);
                return true;
            }

            return false;
        }

        public static string GetCurrentWindowsFabricVersion()
        {
            string version = (string)Registry.GetValue(Constants.FabricInstalledRegPath, Constants.FabricInstalledVersionRegKey, null);

            if (string.IsNullOrEmpty(version))
            {
                // Check if depreciated registry key before renaming specified
                version = (string)Registry.GetValue(Constants.FabricInstalledRegPathDepreciated, Constants.FabricInstalledVersionRegKey, null);
            }

            if (string.IsNullOrEmpty(version))
            {
                // Check if V1 registry key specified
                version = (string)Registry.GetValue(Constants.FabricInstalledRegPathDepreciated, Constants.FabricV1InstalledVersionRegKey, null);
            }

            return version;
        }

        /// <summary>
        /// To be called by deployment code, to determine version of Fabric bits prior to configuration.
        /// </summary>
        /// <returns></returns>
        public static string GetCurrentFabricFileVersion()
        {
            string fabricCodePath = GetFabricCodePath();
            if (string.IsNullOrEmpty(fabricCodePath))
            {
                return null;
            }

            string fabricExePath = Path.Combine(fabricCodePath, Constants.FabricExeName);
            if (!File.Exists(fabricExePath))
            {
                return null;
            }

            FileVersionInfo versionInfo = FileVersionInfo.GetVersionInfo(fabricExePath);

            return string.Format(CultureInfo.InvariantCulture, Constants.FabricVersions.CodeVersionPattern, versionInfo.ProductMajorPart, versionInfo.ProductMinorPart, versionInfo.ProductBuildPart, versionInfo.ProductPrivatePart); ;
        }

        public static void DeleteWindowsFabricV1VersionRegKey()
        {
            DeleteWindowsFabricRelatedRegKey(Constants.FabricV1InstalledVersionRegKey, true);
        }

        public static void DeleteWindowsFabricRelatedRegKey(string keyName, bool isDepreciatedKey = false)
        {
            using (RegistryKey registryKey = Registry.LocalMachine.OpenSubKey(isDepreciatedKey ? Constants.FabricInstalledRegSubPathDepreciated : Constants.FabricInstalledRegSubPath, true))
            {
                // the registry path does not exist
                if (registryKey == null)
                {
                    return;
                }

                registryKey.DeleteValue(keyName, false);
            }
        }

        public static void SetWindowsFabricDataRootRegKey(string value, bool isDepreciatedKey = false)
        {
            Registry.SetValue(
                isDepreciatedKey ? Constants.FabricInstalledRegPathDepreciated : Constants.FabricInstalledRegPath,
                Constants.FabricDataRootRegKey,
                value);
        }

        public static string GetWindowsFabricDataRootRegKey()
        {
            string dataRoot = Registry.GetValue(Constants.FabricInstalledRegPath, Constants.FabricDataRootRegKey, null) as string;

            if (string.IsNullOrEmpty(dataRoot))
            {
                dataRoot = Registry.GetValue(Constants.FabricInstalledRegPathDepreciated, Constants.FabricDataRootRegKey, null) as string;
            }

            return dataRoot;
        }

        public static bool IsWindowsFabricNodeConfigurationCompleted(bool isDepreciatedKey = false)
        {
            string value = Registry.GetValue(isDepreciatedKey ? Constants.FabricInstalledRegPathDepreciated : Constants.FabricInstalledRegPath, Constants.NodeConfigurationCompletedRegKey, null) as string;

            return string.Compare(value, bool.TrueString, StringComparison.OrdinalIgnoreCase) == 0;
        }

        public static void SetWindowsFabricNodeConfigurationCompleted(bool isDepreciatedKey = false)
        {
            Registry.SetValue(isDepreciatedKey ? Constants.FabricInstalledRegPathDepreciated : Constants.FabricInstalledRegPath, Constants.NodeConfigurationCompletedRegKey, bool.TrueString);
        }

        public static void SetDynamicTopologyKind(int dynamicTopologyKindIntValue)
        {
            Registry.SetValue(Constants.FabricInstalledRegPath, Constants.DynamicTopologyKindRegKey, dynamicTopologyKindIntValue);
        }

        public static bool IsAddressLoopback(string address)
        {
            IPAddress ipAddress;
            if (IPAddress.TryParse(address, out ipAddress))
            {
                return IPAddress.IsLoopback(ipAddress);
            }
            else
            {
                IdnMapping mapping = new IdnMapping();
                string inputHostName = mapping.GetAscii(address);
                string localhost = mapping.GetAscii("localhost");
                return inputHostName.Equals(localhost, StringComparison.OrdinalIgnoreCase);
            }
        }

        public static ServiceController GetInstalledService(string serviceName)
        {
            ServiceController[] allServices = ServiceController.GetServices();

            return allServices.FirstOrDefault(service => service.ServiceName.Equals(serviceName, StringComparison.OrdinalIgnoreCase));
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using Linq;
    using Microsoft.Win32;
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.IO;
    using System.Net;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Text;
    using Strings;
    using System.Net.Http;
    using System.Xml.Serialization;
    using System.Xml;
    using System.Net.NetworkInformation;
    using System.Net.Sockets;
    using System.ServiceProcess;
    using System.Threading;

#if !DotNetCoreClr
    using System.Management.Automation;
    using System.Management;
    using Management.FabricDeployer;
#endif

#if !DotNetCoreClr // Disable compiling on windows for now. Need to correct when porting FabricDeployer for windows.
    [CLSCompliant(true)]
#endif
    public static class Utility
    {
#if DotNetCoreClrLinux
        private static readonly TimeSpan ProcessOutputWaitTimeout = TimeSpan.FromSeconds(5);
        private static readonly TimeSpan InstallTimeout = TimeSpan.FromMinutes(20);
#endif

        internal static char[] SecureStringToCharArray(SecureString secureString)
        {
            if (secureString == null)
            {
                return null;
            }

            char[] charArray = new char[secureString.Length];
#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
            IntPtr ptr = Marshal.SecureStringToGlobalAllocUnicode(secureString);
#else
            IntPtr ptr = SecureStringMarshal.SecureStringToGlobalAllocUnicode(secureString);
#endif
            try
            {
                Marshal.Copy(ptr, charArray, 0, secureString.Length);
            }
            finally
            {
#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
                Marshal.ZeroFreeGlobalAllocUnicode(ptr);
#else
                Marshal.FreeHGlobal(ptr);
#endif
            }

            return charArray;
        }

        internal static void SetFabricRegistrySettings(DeploymentParameters parameters)
        {
            string machineNameForRegistry = string.IsNullOrEmpty(parameters.MachineName) ? null : @"\\" + parameters.MachineName;

            if (!string.IsNullOrEmpty(parameters.FabricPackageRoot))
            {
                DeployerTrace.WriteInfo("Setting FabricRoot to {0} on machine {1}", parameters.FabricPackageRoot, parameters.MachineName);
                FabricEnvironment.SetFabricRoot(parameters.FabricPackageRoot, machineNameForRegistry);

                string fabricBinRoot = Path.Combine(parameters.FabricPackageRoot, Constants.FabricBinRootRelativePath);
                DeployerTrace.WriteInfo("Setting FabricBinRoot to {0} on machine {1}", fabricBinRoot, parameters.MachineName);
                FabricEnvironment.SetFabricBinRoot(fabricBinRoot, machineNameForRegistry);

                string fabricCodePath = Path.Combine(parameters.FabricPackageRoot, Constants.FabricCodePathRelativePath);
                DeployerTrace.WriteInfo("Setting FabricCodePath to {0} on machine {1}", fabricCodePath, parameters.MachineName);
                FabricEnvironment.SetFabricCodePath(fabricCodePath, machineNameForRegistry);
            }

            if (!string.IsNullOrEmpty(parameters.FabricDataRoot))
            {
                DeployerTrace.WriteInfo("Setting FabricDataRoot to {0} on machine {1}", parameters.FabricDataRoot, parameters.MachineName);
                FabricEnvironment.SetDataRoot(parameters.FabricDataRoot, machineNameForRegistry);
            }

            if (!string.IsNullOrEmpty(parameters.FabricLogRoot))
            {
                DeployerTrace.WriteInfo("Setting FabricLogRoot to {0} on machine {1}", parameters.FabricLogRoot, parameters.MachineName);
                FabricEnvironment.SetLogRoot(parameters.FabricLogRoot, machineNameForRegistry);
            }

            DeployerTrace.WriteInfo("Setting EnableCircularTraceSession to {0} on machine {1}", parameters.EnableCircularTraceSession, parameters.MachineName);
            FabricEnvironment.SetEnableCircularTraceSession(parameters.EnableCircularTraceSession, machineNameForRegistry);

            // Set the state for preview features that need to lightup at runtime
            DeployerTrace.WriteInfo("Setting EnableUnsupportedPreviewFeatures to {0} on machine {1}", parameters.EnableUnsupportedPreviewFeatures, parameters.MachineName);
            FabricEnvironment.SetEnableUnsupportedPreviewFeatures(parameters.EnableUnsupportedPreviewFeatures, machineNameForRegistry);

            DeployerTrace.WriteInfo("Setting IsSFVolumeDiskServiceEnabled to {0} on machine {1}", parameters.IsSFVolumeDiskServiceEnabled, parameters.MachineName);
            FabricEnvironment.SetIsSFVolumeDiskServiceEnabled(parameters.IsSFVolumeDiskServiceEnabled, machineNameForRegistry);
        }

        internal static string GetFabricRoot()
        {
            return GetFabricRoot(null);
        }

        internal static string GetFabricRoot(string machineName)
        {
            try
            {
                return FabricEnvironment.GetRoot(machineName);
            }
            catch (System.Fabric.FabricException)
            {
                return null;
            }
        }

        internal static string GetFabricBinRoot()
        {
            return GetFabricBinRoot(null);
        }

        internal static string GetFabricBinRoot(string machineName)
        {
            try
            {
                return FabricEnvironment.GetBinRoot(machineName);
            }
            catch (System.Fabric.FabricException)
            {
                return null;
            }
        }

        internal static string GetFabricCodePath()
        {
            return GetFabricCodePath(null);
        }

        internal static string GetFabricCodePath(string machineName)
        {
            try
            {
                return FabricEnvironment.GetCodePath(machineName);
            }
            catch (System.Fabric.FabricException)
            {
                return null;
            }
        }

        internal static string GetFabricDataRoot()
        {
            return GetFabricDataRoot(null);
        }

        internal static string GetFabricDataRoot(string machineName)
        {
            try
            {
                return FabricEnvironment.GetDataRoot(machineName);
            }
            catch (System.Fabric.FabricException)
            {
                return null;
            }
        }

        internal static string GetFabricLogRoot()
        {
            return GetFabricLogRoot(null);
        }

        internal static string GetFabricLogRoot(string machineName)
        {
            try
            {
                return FabricEnvironment.GetLogRoot(machineName);
            }
            catch (System.Fabric.FabricException)
            {
                return null;
            }
        }

        internal static string GetCurrentCodeVersion(string packageLocation)
        {
#if DotNetCoreClr
            string currentAssembly = typeof(Utility).GetTypeInfo().Assembly.Location;
            string versionFile = Path.Combine(Path.GetDirectoryName(currentAssembly), "ClusterVersion");
            string codeVersion = File.ReadAllText(versionFile);
            return codeVersion;
#else
            if (string.IsNullOrEmpty(packageLocation))
            {
                string currentAssembly = typeof(Utility).GetTypeInfo().Assembly.Location;
                string fabricPath = Path.Combine(Path.GetDirectoryName(currentAssembly), Constants.FabricExe);

                if (!File.Exists(fabricPath))
                {
                    DeployerTrace.WriteWarning("{0} Fabric Path doesn't exist", fabricPath);

                    string codePath = GetFabricCodePath();
                    if (!string.IsNullOrEmpty(codePath))
                    {
                        fabricPath = Path.Combine(codePath, Constants.FabricExe);
                        if (!File.Exists(fabricPath))
                        {
                            DeployerTrace.WriteWarning("{0} Code Path doesn't exist", fabricPath);
                            codePath = null;
                        }
                    }

                    if (string.IsNullOrEmpty(codePath))
                    {
                        string errorMessage = string.Format("Fabric.exe not found in current execution assembly directory. CurrentAssembly : {0}, Fabric Path : {1}", currentAssembly, fabricPath);
                        throw new InvalidOperationException(errorMessage);
                    }

                }
                var versionInfo = System.Diagnostics.FileVersionInfo.GetVersionInfo(fabricPath);
                return string.Format(CultureInfo.InvariantCulture, Constants.FabricVersions.CodeVersionPattern, versionInfo.ProductMajorPart, versionInfo.ProductMinorPart, versionInfo.ProductBuildPart, versionInfo.ProductPrivatePart);
            }
            else
            {
                return CabFileOperations.GetCabVersion(packageLocation);
            }
#endif
        }

#if DotNetCoreClrLinux || DotNetCoreClrIOT
        internal static bool GetTestFailDeployer()
        {
            return false;
        }

        internal static void DeleteTestFailDeployer()
        {
            return;
        }

        internal static bool GetSkipDeleteFabricDataRoot()
        {
             return false;
        }

        internal static void DeleteRegistryKeyTree(string machineName)
        {
            return;
        }
#else
        internal static bool GetTestFailDeployer()
        {
            using (RegistryKey key = Registry.LocalMachine.OpenSubKey(FabricConstants.FabricRegistryKeyPath, false))
            {
                if (key != null)
                {
                    return bool.Parse((string)key.GetValue(Constants.Registry.TestFailDeployerKey, "false"));
                }
            }
            return false;
        }

        internal static void DeleteTestFailDeployer()
        {
            using (RegistryKey key = Registry.LocalMachine.OpenSubKey(FabricConstants.FabricRegistryKeyPath, true))
            {
                if (key != null)
                {
                    key.DeleteValue(Constants.Registry.TestFailDeployerKey);
                }
            }
        }

        internal static bool GetSkipDeleteFabricDataRoot()
        {
            using (RegistryKey key = Registry.LocalMachine.OpenSubKey(FabricConstants.FabricRegistryKeyPath, false))
            {
                if (key != null)
                {
                    return bool.Parse((string)key.GetValue(Constants.Registry.SkipDeleteFabricDataRoot, "false"));
                }
            }
            return false;
        }

        internal static void DeleteRegistryKeyTree(string machineName)
        {
            try
            {
                DeployerTrace.WriteInfo("Deleting Fabric registry key tree for machine {0}", machineName);
                RegistryKey baseKey = GetHklm(machineName);

                if (baseKey.OpenSubKey(FabricConstants.FabricRegistryKeyPath) != null)
                {
                    baseKey.DeleteSubKeyTree(FabricConstants.FabricRegistryKeyPath);
                }
            }
            catch (ArgumentException e)
            {
                DeployerTrace.WriteWarning("Trying to delete registry key tree threw ArgumentException: {0}", e.ToString());
            }
        }
#endif
        internal static string GetNodeTypeFromMachineName(ClusterManifestType clusterManifest, InfrastructureInformationType infrastructure, string machineName)
        {
            if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsServer)
            {
                return ((ClusterManifestTypeInfrastructureWindowsServer)clusterManifest.Infrastructure.Item).NodeList.Where(node => node.IPAddressOrFQDN == machineName).First().NodeTypeRef;
            }
            else if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructurePaaS || clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzure)
            {
                return infrastructure.NodeList.Where(node => node.IPAddressOrFQDN == machineName).First().NodeTypeRef;
            }
            else if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzureStaticTopology)
            {
                return ((ClusterManifestTypeInfrastructureWindowsAzureStaticTopology)clusterManifest.Infrastructure.Item).NodeList.Where(node => node.IPAddressOrFQDN == machineName).First().NodeTypeRef;
            }
            else if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureLinux)
            {
                return ((ClusterManifestTypeInfrastructureLinux)clusterManifest.Infrastructure.Item).NodeList.Where(node => node.IPAddressOrFQDN == machineName).First().NodeTypeRef;
            }
            else
            {
                throw new ArgumentException(StringResources.Error_ClusterManifestTypeNotSupported);
            }
        }

        internal static int ExecuteNetshCommand(string arguments)
        {
            return ExecuteCommand("netsh", arguments);
        }

        internal static int ExecuteCommand(string command, string arguments)
        {
            return ExecuteCommand(command, arguments, TimeSpan.MaxValue);
        }

        internal static int ExecuteCommand(string command, string arguments, TimeSpan timeout)
        {
            string workingDir = Directory.GetCurrentDirectory();
            DeployerTrace.WriteInfo("Executing command: {0} {1}", command, arguments);
            using (Process p = new Process())
            {
                p.StartInfo.FileName = command;
                p.StartInfo.Arguments = arguments;
                p.StartInfo.UseShellExecute = false;
                p.StartInfo.CreateNoWindow = true;
                p.StartInfo.WorkingDirectory = workingDir;
                p.Start();

                if (timeout == TimeSpan.MaxValue)
                {
                    p.WaitForExit();
                }
                else
                {
                    if (!p.WaitForExit((int)timeout.TotalMilliseconds))
                    {
                        p.Kill();
                        DeployerTrace.WriteInfo("Execution of command {0} {1} timedout after {2} milliseconds", command, arguments, timeout.TotalMilliseconds);
                        return Constants.ErrorCode_Failure;
                    }
                }

                return p.ExitCode;
            }
        }

        internal static bool ExecuteCommand(string command, string arguments, string workingDir, bool waitForExit, TimeSpan? timeout)
        {
            bool success = false;
            DeployerTrace.WriteInfo("Executing command: {0} {1} from working dir {2} wait for exit {3}", command, arguments, workingDir, waitForExit);

            try
            {
                var p = new Process();
                p.StartInfo.FileName = command;
                p.StartInfo.Arguments = arguments;
                p.StartInfo.UseShellExecute = false;
                p.StartInfo.CreateNoWindow = false;
                p.StartInfo.WorkingDirectory = workingDir;
                p.Start();

                success = true;

                if (waitForExit)
                {
                    if (timeout.HasValue)
                    {
                        if (timeout.Value == TimeSpan.MaxValue)
                        {
                            p.WaitForExit();
                        }
                        else
                        {
                            if (!p.WaitForExit((int)timeout.Value.TotalMilliseconds))
                            {
                                p.Kill();
                                DeployerTrace.WriteInfo("Execution of command {0} {1} from working dir {2} timed out after {3} milliseconds",
                                                        command,
                                                        arguments,
                                                        workingDir,
                                                        timeout.Value.TotalMilliseconds);
                                success = false;
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to execute command {0} {1} from working dir {2} wait for exit {3} exception {4}",
                                         command,
                                         arguments,
                                         workingDir,
                                         waitForExit,
                                         ex);
            }

            return success;
        }

        /// <summary>
        /// Formats a string using <see cref="string.Format(IFormatProvider, string, object[])"/>
        /// where IFormatProvider is <see cref="CultureInfo.InvariantCulture"/>
        /// </summary>
        /// <param name="format">The format to use.</param>
        /// <param name="args">The arguments to be passed in.</param>
        /// <returns>
        /// A copy of format in which the format items have been replaced by the string representations of the corresponding arguments.
        /// </returns>
        /// <remarks>TODO, replace this method with $ style formatting once we move to C# 6.0</remarks>
        internal static string ToFormat(this string format, params object[] args)
        {
            format.Validate("format");
            string text = string.Format(CultureInfo.InvariantCulture, format, args);
            return text;
        }

        internal static T Validate<T>(this T argument, string argumentName) where T : class
        {
            if (argumentName == null)
            {
                throw new ArgumentNullException("argumentName");
            }

            if (string.IsNullOrWhiteSpace(argumentName))
            {
                throw new ArgumentException("The argument name is empty or contains only white spaces.");
            }

            if (argument == null)
            {
                throw new ArgumentNullException(argumentName);
            }

            return argument;
        }

        internal static string Validate(this string argument, string argumentName, bool allowEmptyOrWhiteSpace = false)
        {
            Validate<string>(argument, argumentName);

            if (!allowEmptyOrWhiteSpace && string.IsNullOrWhiteSpace(argument))
            {
                throw new ArgumentException(
                    string.Format(CultureInfo.InvariantCulture, "Argument '{0}' is empty or contains only white spaces.", argumentName),
                    argumentName);
            }

            return argument;
        }

        internal static string GetBytesAsBinaryString(this byte[] bytes)
        {
            bytes.Validate("bytes");

            var builder = new StringBuilder();

            foreach (var b in bytes)
            {
                builder.AppendFormat(CultureInfo.InvariantCulture, "{0:8} ", Convert.ToString(b, 2).PadLeft(8, '0'));
            }

            return builder.ToString();
        }

        internal static bool IsIOTCore()
        {
            using (RegistryKey regKey = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"))
            {
                if (regKey != null)
                {
                    return string.Equals((string)regKey.GetValue(@"EditionID", string.Empty), @"IotUAP");
                }
            }

            return false;
        }

        internal static string GetWin32ErrorMessage(int win32Error)
        {
            return string.Format("Error {0}: {1}", win32Error, new Win32Exception(win32Error).Message);
        }

        /// <summary>
        /// Checks if docker container service is installed.
        /// </summary>
        /// <returns></returns>
        internal static bool IsContainerServiceInstalled()
        {
#if DotNetCoreClrLinux
            return true;
#else
            bool installed = false;

            var programFilesPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), FlatNetworkConstants.ContainerServiceName);
            var containerServiceFullPath = Path.Combine(programFilesPath, FlatNetworkConstants.ContainerProviderProcessNameWithExtension);
            if (File.Exists(containerServiceFullPath))
            {
                installed = true;
            }

            DeployerTrace.WriteInfo("Container service installed {0}.", installed);
            return installed;
#endif
        }

#if !DotNetCoreClrLinux
        /// <summary>
        /// Checks if docker container NT service is present or not .
        /// </summary>
        /// <returns></returns>
        internal static bool IsContainerNTServicePresent()
        {
            var service = ServiceController.GetServices().SingleOrDefault(
                s => string.Equals(s.ServiceName, FlatNetworkConstants.ContainerServiceName, StringComparison.OrdinalIgnoreCase));
            return ((service != null) ? true : false);
        }
#endif

#if DotNetCoreClr // Need to correct when porting FabricDeployer for windows.
        internal static bool IsContainersFeaturePresent()
        {
            return true;
        }
#else
        internal static bool IsContainersFeaturePresent()
        {
            for (int i = 0; i < Constants.ApiRetryAttempts; i++)
            {
                DeployerTrace.WriteInfo("Waiting to query dism to get container feature state.");

                Thread.Sleep(Constants.ApiRetryIntervalMilliSeconds);

                try
                {
                    DismPackageFeatureState featureState = DismHelper.GetFeatureState(Constants.ContainersFeatureName);

                    switch (featureState)
                    {
                        case DismPackageFeatureState.DismStateInstallPending:
                        case DismPackageFeatureState.DismStateInstalled:
                        case DismPackageFeatureState.DismStateSuperseded:
                            return true;
                    }
                }
                catch (COMException ex)
                {
                    // Dism COM errors aren't well understood yet across different OSes.
                    DeployerTrace.WriteWarning("Error getting feature state for feature {0}. Exception: {1}", Constants.ContainersFeatureName, ex);
                }
                catch (DllNotFoundException ex)
                {
                    // This happens on platforms that don't have dismapi.dll
                    // https://technet.microsoft.com/en-us/library/hh825186.aspx
                    DeployerTrace.WriteWarning(
                        "Error getting feature state for feature {0}. This usually means that the platform doesn't support DISM APIs. Exception: {1}",
                        Constants.ContainersFeatureName,
                        ex);
                }
                catch (Exception ex)
                {
                    // Swallowing all!
                    DeployerTrace.WriteWarning(
                        "Unexpected error getting feature state for feature {0}. Exception: {1}",
                        Constants.ContainersFeatureName,
                        ex);
                }
            }

            DeployerTrace.WriteWarning("Treating feature as not present and continuing.");
        
            return false;
        }
#endif

        internal static RegistryKey GetHklm(string machineName)
        {
            RegistryKey hklm;
            if (string.IsNullOrWhiteSpace(machineName)
                || string.Equals(machineName.Trim(), Constants.LocalHostMachineName, StringComparison.OrdinalIgnoreCase))
            {
                hklm = Registry.LocalMachine;
            }
            else
            {
#if DotNetCoreClr
                throw new Exception("CoreCLR doesn't support RegistryKey.OpenRemoteBaseKey");
#else
                hklm = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, machineName);
#endif
            }

            return hklm;
        }

#if !DotNetCoreClr
        internal static T GetRegistryKeyValue<T>(
            RegistryKey baseRegistryKey,
            string keyName,
            string valueName,
            T defaultValue)
        {
            baseRegistryKey.Validate("baseRegistryKey");
            keyName.Validate("keyName");
            valueName.Validate("valueName");

            DeployerTrace.WriteInfo("Getting key-value data for key name: {0}, value name: {1}", keyName, valueName);

            using (RegistryKey registryKey = baseRegistryKey.OpenSubKey(keyName))
            {
                if (registryKey == null)
                {
                    string message = "Error opening registry sub key: {0}, using default value: {1}".ToFormat(keyName, defaultValue);
                    DeployerTrace.WriteWarning(message);
                    return defaultValue;
                }

                T value = (T)Registry.GetValue(registryKey.Name, valueName, defaultValue);

                DeployerTrace.WriteInfo(
                    "Value successfully obtained. Key name: {0}, reg value name: {1}, reg value: {2}", keyName,
                    valueName, value);
                return value;
            }
        }

        internal static void AddRegistryKeyValues(RegistryKey baseRegistryKey, string keyName, IDictionary<string, string> keyValueMap)
        {
            baseRegistryKey.Validate("baseRegistryKey");
            keyName.Validate("keyName");
            keyValueMap.Validate("keyValueMap");

            DeployerTrace.WriteInfo("Adding key-value data for key name {0}", keyName);

            using (RegistryKey registryKey = baseRegistryKey.OpenSubKey(keyName, true))
            {
                if (registryKey == null)
                {
                    string message = "Error opening registry sub key: {0}".ToFormat(keyName);
                    DeployerTrace.WriteError(message);
                    throw new InvalidOperationException(message);
                }

                foreach (var kv in keyValueMap)
                {
                    try
                    {
                        Registry.SetValue(
                            registryKey.Name,
                            kv.Key,
                            kv.Value);

                        DeployerTrace.WriteInfo(
                            "Successfully created reg value. Reg key: {0}, reg value name: {1}, value: {2}",
                            keyName,
                            kv.Key,
                            kv.Value);
                    }
                    catch (Exception ex)
                    {
                        string message =
                            "Error creating reg value. Reg key: {0}, reg value name: {1}, value: {2}, exception: {3}".ToFormat(
                            keyName,
                            kv.Key,
                            kv.Value,
                            ex);

                        DeployerTrace.WriteError(message);
                        throw new InvalidOperationException(message);
                    }
                }
            }

            DeployerTrace.WriteInfo("Key-value(s) successfully added for key name {0}", keyName);
        }

        internal static void RemoveRegistryKeyValues(RegistryKey baseRegistryKey, string keyName, IList<string> valueNames)
        {
            baseRegistryKey.Validate("baseRegistryKey");
            keyName.Validate("keyName");
            valueNames.Validate("valueNames");

            DeployerTrace.WriteInfo("Removing key-value data for key name {0}", keyName);

            using (RegistryKey registryKey = baseRegistryKey.OpenSubKey(keyName, true))
            {
                if (registryKey == null)
                {
                    string message = "Error opening registry sub key. Continuing. Registry values have to be cleaned up manually. Reg key: {0}".ToFormat(keyName);
                    DeployerTrace.WriteWarning(message);
                    return;
                }

                foreach (var v in valueNames)
                {
                    try
                    {
                        registryKey.DeleteValue(v, false);

                        DeployerTrace.WriteInfo(
                            "Successfully created reg value. Reg key: {0}, reg value name: {1}",
                            keyName,
                            v);
                    }
                    catch (Exception ex)
                    {
                        string message =
                            "Error creating reg value. Continuing. This needs to be cleaned up manually. Reg key: {0}, reg value name: {1}, exception: {2}".ToFormat(
                            keyName,
                            v,
                            ex);

                        DeployerTrace.WriteWarning(message);
                    }
                }
            }

            DeployerTrace.WriteInfo("Key-value(s) successfully deleted for key name {0}", keyName);
        }

        /// <summary>
        /// Executes a PowerShell script and captures any error in a temp file.
        /// If <see cref="throwOnScriptError"/> is set, then an exception is thrown if the script returns an error.
        /// </summary>
        internal static System.Collections.ObjectModel.Collection<PSObject> ExecutePowerShellScript(
            string script,
            bool throwOnScriptError)
        {
            script.Validate("script");

            string tempFile = Path.GetTempFileName();

            // Callers need to be aware that this is getting suffixed.
            script += " 2>{0}".ToFormat(tempFile);

            using (var ps = PowerShell.Create())
            {
                DeployerTrace.WriteInfo("Invoking PowerShell script: {0}", script);

                try
                {
                    ps.AddScript(script);

                    // TODO, suffix the -ErrorVariable myVar and check if that is not null
                    var psObjects = ps.Invoke();

                    if (File.Exists(tempFile))
                    {
                        var errorText = File.ReadAllText(tempFile).Trim();
                        if (!string.IsNullOrWhiteSpace(errorText))
                        {
                            string message = "Error executing script. Script: {0}{1}{2}Error: {3}{4}".ToFormat(
                                Environment.NewLine, script, Environment.NewLine, Environment.NewLine, errorText);

                            Trace.TraceWarning(message);

                            if (throwOnScriptError)
                            {
                                throw new InvalidOperationException(message);
                            }
                        }

                        File.Delete(tempFile);
                    }

                    return psObjects;
                }
                catch (Exception ex)
                {
                    var message =
                        "Error invoking PowerShell script. Exception: {0}".ToFormat(ex);
                    DeployerTrace.WriteError(message);
                    throw;
                }
            }
        }

        internal static string GetDefaultPackageDestination(string machineName)
        {
            try
            {
                return GetDefaultPackageDestinationInner(machineName);
            }
            catch (Exception exception)
            {
                DeployerTrace.WriteError(exception.ToString());
                throw;
            }
        }

        internal static string GetTempPath(string machineName)
        {
            try
            {
                return GetTempPathInner(machineName);
            }
            catch (Exception exception)
            {
                DeployerTrace.WriteWarning(exception.ToString());
                throw;
            }
        }

        private static string GetDefaultPackageDestinationInner(string machineName)
        {
            const string defaultPathEnding = @"Program Files\Microsoft Service Fabric";
            var remoteSystemRoot = GetRemoteSystemRoot(machineName);
            string systemDrive = Path.GetPathRoot(remoteSystemRoot);
            string destination = Path.Combine(systemDrive, defaultPathEnding);
            return destination;
        }

        private static string GetRemoteSystemRoot(string machineName)
        {
            var ntCurrentVersionKey = GetHklm(machineName).OpenSubKey(@"SOFTWARE\Microsoft\Windows NT\CurrentVersion");
            return (string)ntCurrentVersionKey.GetValue("SystemRoot");
        }

        private static string GetTempPathInner(string machineName)
        {
            RegistryKey key = GetHklm(machineName).OpenSubKey("System\\CurrentControlSet\\Control\\Session Manager\\Environment");
            var systemRoot = GetRemoteSystemRoot(machineName);
            string path = (string)key.GetValue("TEMP", Path.Combine(systemRoot, "Temp"), RegistryValueOptions.DoNotExpandEnvironmentNames);
            path = path.ToLowerInvariant().Replace("%systemroot%", systemRoot);
            return path;
        }
#endif

        /// <summary>
        /// Retrieves network interface that has the given mac address.
        /// </summary>
        public static NetworkInterface GetNetworkInterfaceByMacAddress(string macAddress)
        {
            DeployerTrace.WriteInfo("Getting network interface with macAddress: {0}.", macAddress);
            macAddress = macAddress.Replace("-", "").Replace(":", "").ToUpper();

            foreach (NetworkInterface nic in NetworkInterface.GetAllNetworkInterfaces())
            {
                var nicMacAddress = nic.GetPhysicalAddress().ToString().Replace("-", "").Replace(":", "").ToUpper();

                if (string.Equals(nicMacAddress, macAddress))
                {
                    DeployerTrace.WriteInfo("Network interface with macAddress {0} successfully obtained, name: {1}", macAddress, nic.Name);
                    return nic;
                }
            }

            DeployerTrace.WriteInfo("Unable to detect network interface with macAddress {0}.", macAddress);
            return null;
        }

        /// <summary>
        /// Retrieves the subnet, gateway and mac address information from nm agent,
        /// for the network interface specified.
        /// </summary>
        /// <param name="primaryNetworkInterface"></param>
        /// <param name="subnet"></param>
        /// <param name="gateway"></param>
        /// <param name="macAddress"></param>
        /// <returns></returns>
        public static bool RetrieveNMAgentInterfaceInfo(bool primaryNetworkInterface, out string subnet, out string gateway, out string macAddress)
        {
            bool success = false;
            subnet = string.Empty;
            gateway = string.Empty;
            macAddress = string.Empty;

            var networkInterface = (primaryNetworkInterface) ? ("primary") : ("secondary");
            DeployerTrace.WriteInfo("Retrieving gateway, subnet and mac address information from nm agent for the {0} network interface.", networkInterface);

            try
            {
                HttpClient httpClient = new HttpClient();
                var task = httpClient.GetAsync(FlatNetworkConstants.NetworkInterfaceUrl);
                var httpResponse = task.GetAwaiter().GetResult().EnsureSuccessStatusCode();
                var content = httpResponse.Content.ReadAsStringAsync().GetAwaiter().GetResult();

                var xmlSerializer = new XmlSerializer(typeof(Interfaces));
                var encoding = Encoding.ASCII;
                byte[] sourceBytes = encoding.GetBytes(content);

                using (var memoryStream = new MemoryStream(sourceBytes))
                {
#if DotNetCoreClr
                    using (var xmlTextReader = XmlReader.Create(memoryStream))
#else
                    using (var xmlTextReader = new XmlTextReader(memoryStream))
#endif
                    {
                        var interfaces = xmlSerializer.Deserialize(xmlTextReader) as Interfaces;
                        if (interfaces != null)
                        {
                            var primaryInterface = interfaces.Items.FirstOrDefault(i => string.Equals(i.IsPrimary, (primaryNetworkInterface ? Boolean.TrueString : Boolean.FalseString), StringComparison.OrdinalIgnoreCase));
                            if (primaryInterface != null && primaryInterface.IPSubnet.Length > 0)
                            {
                                macAddress = primaryInterface.MacAddress;

                                subnet = primaryInterface.IPSubnet[0].Prefix;

                                if (!string.IsNullOrEmpty(subnet))
                                {
                                    uint ip = 0;
                                    uint mask = 0;
                                    if (ParseSubnetIpAndMask(subnet, out ip, out mask))
                                    {
                                        var gt = (ip & mask) + 1;
                                        gateway = ConvertToIpAddress(gt, false);
                                    }
                                }
                            }
                        }
                    }
                }

                success = true;
                DeployerTrace.WriteInfo("Retrieved gateway, subnet and mac address information from nm agent for the {0} network interface. Subnet:{1} Gateway:{2} Mac Address:{3}.", networkInterface, subnet, gateway, macAddress);
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to retrieve gateway, subnet and mac address information from nm agent for the {0} network interface, exception {1}.", networkInterface, ex);
            }

            return success;
        }

        /// <summary>
        /// Parses subnet in CIDR format to network base ip and subnet mask.
        /// </summary>
        /// <param name="subnet"></param>
        /// <param name="networkIp"></param>
        /// <param name="subnetMask"></param>
        /// <returns></returns>
#if !DotNetCoreClr // Disable compiling on windows for now. Need to correct when porting FabricDeployer for windows.
        [CLSCompliantAttribute(false)]
#endif
        public static bool ParseSubnetIpAndMask(string subnet, out uint networkIp, out uint subnetMask)
        {
            bool success = false;
            networkIp = 0;
            subnetMask = 0;

            try
            {
                int index = subnet.IndexOf(FlatNetworkConstants.SubnetMaskSeparator);
                if (index >= 0)
                {
                    var ipAddressStr = subnet.Substring(0, index);
                    var maskStr = subnet.Substring(index + 1);

                    var ipAddress = System.Net.IPAddress.Parse(ipAddressStr);
                    var maskUInt = Convert.ToUInt32(maskStr);

                    var ipBytes = ipAddress.GetAddressBytes();
                    Array.Reverse(ipBytes);
                    networkIp = BitConverter.ToUInt32(ipBytes, 0);

                    subnetMask = 0xFFFFFFFF;
                    UInt32 size = (sizeof(UInt32) * 8) - (maskUInt);
                    for (int i = 0; i < size; i++)
                    {
                        subnetMask <<= 1;
                    }
                }

                success = true;
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to parse network ip and subnet mask from subnet information exception {0}.", ex);
            }

            return success;
        }

        /// <summary>
        /// Converts IPv4 address to dot format.
        /// </summary>
        /// <param name="ipAddress"></param>
        /// <param name="networkToHostOrderConversionNeeded"></param>
        /// <returns></returns>
#if !DotNetCoreClr // Disable compiling on windows for now. Need to correct when porting FabricDeployer for windows.
        [CLSCompliantAttribute(false)]
#endif
        public static string ConvertToIpAddress(uint ipAddress, bool networkToHostOrderConversionNeeded)
        {
            int adjustedIpAddress = (int)ipAddress;
            if (networkToHostOrderConversionNeeded)
            {
                adjustedIpAddress = System.Net.IPAddress.NetworkToHostOrder(adjustedIpAddress);
            }

            return String.Format("{0}.{1}.{2}.{3}",
            (adjustedIpAddress >> 24) & 0xFF,
            (adjustedIpAddress >> 16) & 0xFF,
            (adjustedIpAddress >> 8) & 0xFF,
            adjustedIpAddress & 0xFF);
        }

        /// <summary>
        /// Retrieves network interface that is on the given subnet.
        /// </summary>
        public static NetworkInterface GetNetworkInterfaceOnSubnetWithRetry(string subnet)
        {
            NetworkInterface networkInterface = null;

            for (int i = 0; i < Constants.ApiRetryAttempts; i++)
            {
                DeployerTrace.WriteInfo("Waiting to get network interface on subnet {0}.", subnet);

                Thread.Sleep(Constants.ApiRetryIntervalMilliSeconds);

                networkInterface = GetNetworkInterfaceOnSubnet(subnet);

                if (networkInterface != null)
                {
                    break;
                }
            }

            return networkInterface;
        }

        /// <summary>
        /// Retrieves network interface that is on the given subnet.
        /// </summary>
        public static NetworkInterface GetNetworkInterfaceOnSubnet(string subnet)
        {
            DeployerTrace.WriteInfo("Getting network interface with ip address on subnet {0}.", subnet);

            foreach (NetworkInterface nic in NetworkInterface.GetAllNetworkInterfaces())
            {
                IPAddress ipv4Address;
                IPAddress ipv4Mask;
                GetDefaultIPV4AddressAndMask(nic, out ipv4Address, out ipv4Mask);

                // Calculate subnet by ANDing ip address and mask
                var networkAddress = GetNetworkAddress(ipv4Address, ipv4Mask);
                var networkAddressString = (networkAddress != null) ? (networkAddress.ToString()) : (string.Empty);

                DeployerTrace.WriteInfo("Network broadcast ip {0}", networkAddressString);

                if (string.Equals(networkAddressString, subnet))
                {
                    DeployerTrace.WriteInfo("Network interface with ip address on subnet {0} successfully obtained, name: {1}", subnet, nic.Name);
                    return nic;
                }
            }

            DeployerTrace.WriteInfo("Unable to detect network interface with ip address on subnet {0}.", subnet);
            return null;
        }

        /// <summary>
        /// Retrieves the broadcast address.
        /// </summary>
        /// <param name="ipv4Address"></param>
        /// <param name="ipv4Mask"></param>
        /// <returns></returns>
        public static IPAddress GetNetworkAddress(IPAddress ipv4Address, IPAddress ipv4Mask)
        {
            if (ipv4Address == null || ipv4Mask == null)
            {
                return null;
            }

            byte[] ipv4AddressBytes = ipv4Address.GetAddressBytes();
            byte[] ipv4MaskBytes = ipv4Mask.GetAddressBytes();

            if (ipv4AddressBytes.Length == ipv4MaskBytes.Length)
            {
                byte[] broadcastAddress = new byte[ipv4AddressBytes.Length];
                for (int i = 0; i < broadcastAddress.Length; i++)
                {
                    broadcastAddress[i] = (byte)(ipv4AddressBytes[i] & (ipv4MaskBytes[i]));
                }
                return new IPAddress(broadcastAddress);
            }

            return null;
        }

        /// <summary>
        /// Retrieves the IPV4 address and mask associated with the network interface.
        /// </summary>
        /// <param name="networkInterface"></param>
        /// <param name="ipv4Address"></param>
        /// <param name="ipv4Mask"></param>
        public static void GetDefaultIPV4AddressAndMask(NetworkInterface networkInterface, out IPAddress ipv4Address, out IPAddress ipv4Mask)
        {
            ipv4Address = null;
            ipv4Mask = null;

            if (networkInterface == null)
            {
                return;
            }

            foreach (var address in networkInterface.GetIPProperties().UnicastAddresses)
            {
                if (address.Address.AddressFamily == AddressFamily.InterNetwork && !address.IsTransient)
                {
                    ipv4Address = address.Address;
                    ipv4Mask = address.IPv4Mask;
                    break;
                }
            }
        }

        /// <summary>
        /// Checks that the network interface has the expected ip address.
        /// This is usually needed after the network has been set up, to allow time to plumb ip address.
        /// </summary>
        /// <param name="networkInterface"></param>
        /// <param name="expectedAddress"></param>
        /// <returns></returns>
        public static bool IsIPAddressMatching(NetworkInterface networkInterface, IPAddress expectedAddress)
        {
            bool matching = false;

            for (int i = 0; i < Constants.ApiRetryAttempts; i++)
            {
                DeployerTrace.WriteInfo("Waiting to match ip address {0} on network interface {1}.", expectedAddress.ToString(), networkInterface.Name);

                Thread.Sleep(Constants.ApiRetryIntervalMilliSeconds);

                IPAddress ipv4Address = null;
                IPAddress ipv4Mask = null;
                Utility.GetDefaultIPV4AddressAndMask(networkInterface, out ipv4Address, out ipv4Mask);

                if (ipv4Address != null && IPAddress.Equals(ipv4Address, expectedAddress))
                {
                    matching = true;
                    break;
                }
            }

            DeployerTrace.WriteInfo("Matched ip address {0} on network interface {1}:{2}.", expectedAddress.ToString(), networkInterface.Name, matching);

            return matching;
        }

        /// <summary>
        /// Kills the process, if running.
        /// </summary>
        /// <param name="processName"></param>
        /// <returns></returns>
        public static bool StopProcess(string processName)
        {
            bool success = false;

            bool running;
            int processId = 0;
            if (Utility.IsProcessRunning(processName, out running, out processId) && !running)
            {
                return true;
            }

            try
            {
                var processes = Process.GetProcessesByName(processName);
                if (processes.Length > 0)
                {
                    DeployerTrace.WriteInfo("Killing {0}:{1} process.", processes[0].ProcessName, processes[0].Id);
                    processes[0].Kill();
                }
                else
                {
                    DeployerTrace.WriteWarning("Process {0} not found.", processName);
                }

                success = true;
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to find if process {0} is running, exception {1}.", processName, ex);
            }

            return success;
        }

        /// <summary>
        /// Checks if the process is running and returns the process id.
        /// </summary>
        /// <param name="processName"></param>
        /// <param name="running"></param>
        /// <param name="processId"></param>
        /// <returns></returns>
        public static bool IsProcessRunning(string processName, out bool running, out int processId)
        {
            bool success = false;
            running = false;
            processId = -1;

            try
            {
                var processes = Process.GetProcessesByName(processName);
                if (processes.Length > 0)
                {
                    running = true;
                    processId = processes[0].Id;
                    DeployerTrace.WriteInfo("Process running {0}:{1}.", processes[0].ProcessName, processes[0].Id);
                }
                else
                {
                    DeployerTrace.WriteInfo("Process {0} not found.", processName);
                }

                success = true;
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to find if process {0} is running, exception {1}.", processName, ex);
            }

            return success;
        }

#if !DotNetCoreClr
        /// <summary>
        /// Retrieves last boot up time using WMI.
        /// </summary>
        /// <returns></returns>
        public static DateTime GetNodeLastBootUpTimeFromSystem()
        {
            DateTime lastBootUpTime = DateTime.MinValue.ToUniversalTime();

            try
            {
                var searcher = new ManagementObjectSearcher("SELECT LastBootUpTime FROM Win32_OperatingSystem WHERE Primary = 'true'");
                var nodeInfoCollection = searcher.Get();
                if (nodeInfoCollection != null)
                {
                    var nodeInfoArray = new ManagementBaseObject[nodeInfoCollection.Count];
                    nodeInfoCollection.CopyTo(nodeInfoArray, 0);
                    if (nodeInfoArray.Length == 1)
                    {
                        lastBootUpTime = ManagementDateTimeConverter.ToDateTime(nodeInfoArray[0]["LastBootUpTime"].ToString()).ToUniversalTime();
                    }
                }
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Failed to retrieve node last boot up time exception {0}", ex);
            }

            return lastBootUpTime;
        }

        /// <summary>
        /// Retrieves last boot up time from the registry, if it exists, else returns DateTime.MinValue.
        /// </summary>
        /// <returns></returns>
        public static DateTime GetNodeLastBootUpTimeFromRegistry()
        {
            using (RegistryKey key = Registry.LocalMachine.OpenSubKey(FabricConstants.FabricRegistryKeyPath, false))
            {
                if (key != null)
                {
                    return DateTime.Parse((string)key.GetValue(Constants.Registry.NodeLastBootUpTime, DateTime.MinValue.ToUniversalTime().ToString()));
                }
            }
            return DateTime.MinValue.ToUniversalTime();
        }

        /// <summary>
        /// Saves last boot up time to registry.
        /// </summary>
        /// <param name="lastBootUpTime"></param>
        public static void SaveNodeLastBootUpTimeToRegistry(DateTime lastBootUpTime)
        {
            try
            {
                var persistData = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
                {
                    {
                        Constants.Registry.NodeLastBootUpTime, lastBootUpTime.ToUniversalTime().ToString()
                    }
                };

                AddRegistryKeyValues(Registry.LocalMachine, FabricConstants.FabricRegistryKeyPath, persistData);
            }
            catch(Exception ex)
            {
                DeployerTrace.WriteError("Failed to save node last boot up time to registry exception {0}", ex);
            }
        }

        /// <summary>
        /// Gets the dns verified host ip.
        /// </summary>
        /// <param name="infrastructure"></param>
        internal static string GetHostIp(Infrastructure infrastructure)
        {
            string hostIp = string.Empty;

            var currentNodeIPAddressOrFQDN = GetNodeIPAddressOrFQDN(infrastructure);
            string hostName = string.IsNullOrEmpty(currentNodeIPAddressOrFQDN) ? (Dns.GetHostName()) : (currentNodeIPAddressOrFQDN);
            DeployerTrace.WriteInfo("Trying to resolve FQDN name <{0}>.", hostName);

            if (!IPAddress.TryParse(currentNodeIPAddressOrFQDN, out IPAddress address))
            {
                IPHostEntry entry = Dns.GetHostEntry(currentNodeIPAddressOrFQDN);
                foreach (var ip in entry.AddressList)
                {
                    if (ip.AddressFamily == AddressFamily.InterNetwork)
                    {
                        // Do a reverse lookup to confirm that hostname is indeed in DNS.

                        try
                        {
                            /// From 18.03 onwards the docker host dns name resolves to
                            /// the ip address used by the host.
                            if (string.Equals(Dns.GetHostEntry(ip).HostName, entry.HostName, StringComparison.OrdinalIgnoreCase) ||
                                string.Equals(Dns.GetHostEntry(ip).HostName, Constants.DockerHostDnsName, StringComparison.OrdinalIgnoreCase))
                            {
                                hostIp = ip.ToString();
                                break;
                            }
                        }
                        catch (Exception innerEx)
                        {
                            DeployerTrace.WriteWarning("Failed to resolve IP {0}, exception {1}", ip.ToString(), innerEx.ToString());
                        }
                    }
                }
            }
            else
            {
                hostIp = address.ToString();
            }

            DeployerTrace.WriteInfo("Resolved host name <{0}> to ip: {1}.", hostName, hostIp);

            return hostIp;
        }

        /// <summary>
        /// Gets the ip address of the current node.
        /// </summary>
        /// <param name="infrastructure"></param>
        /// <returns></returns>
        internal static string GetNodeIPAddressOrFQDN(Infrastructure infrastructure)
        {
            string currentNodeIPAddressOrFQDN = string.Empty;

            if ((infrastructure != null) && (infrastructure.InfrastructureNodes != null))
            {
                foreach (var infraNode in infrastructure.InfrastructureNodes)
                {
                    if (NetworkApiHelper.IsAddressForThisMachine(infraNode.IPAddressOrFQDN))
                    {
                        currentNodeIPAddressOrFQDN = infraNode.IPAddressOrFQDN;
                        break;
                    }
                }
            }

            return currentNodeIPAddressOrFQDN;
        }

        /// <summary>
        /// Gets the network interface index and name, given the ip address.
        /// </summary>
        /// <param name="ipv4Address"></param>
        /// <param name="networkInterfaceIndex"></param>
        /// <param name="networkInterfaceName"></param>
        public static void GetNetworkInterfaceIndexAndName(IPAddress ipv4Address, out int networkInterfaceIndex, out string networkInterfaceName)
        {
            networkInterfaceIndex = -1;
            networkInterfaceName = string.Empty;

            DeployerTrace.WriteInfo("Get network interface index for ip address {0}.", ipv4Address.ToString());

            if (ipv4Address == null)
            {
                return;
            }

            foreach (NetworkInterface nic in NetworkInterface.GetAllNetworkInterfaces())
            {
                var properties = nic.GetIPProperties();
                foreach (var address in properties.UnicastAddresses)
                {
#if DotNetCoreClrLinux
                    if (address.Address.AddressFamily == AddressFamily.InterNetwork)
#else
                    if (address.Address.AddressFamily == AddressFamily.InterNetwork && !address.IsTransient)
#endif
                    {
                        if (IPAddress.Equals(ipv4Address, address.Address))
                        {
                            networkInterfaceIndex = properties.GetIPv4Properties().Index;
                            networkInterfaceName = nic.Name;
                            break;
                        }
                    }
                }
            }

            DeployerTrace.WriteInfo("Network interface details for ip address {0} are index: {1}, name: {2}.", ipv4Address.ToString(), networkInterfaceIndex, networkInterfaceName);
        }

        /// <summary>
        /// Gets the network interface index and name, given the ip address.
        /// This api will retry logic that matches ip with available interfaces.
        /// This is needed when the ip takes a while to plumb.
        /// </summary>
        /// <param name="ipv4Address"></param>
        /// <param name="networkInterfaceIndex"></param>
        /// <param name="networkInterfaceName"></param>
        public static void GetNetworkInterfaceIndexAndNameWithRetry(IPAddress ipv4Address, out int networkInterfaceIndex, out string networkInterfaceName)
        {
            networkInterfaceIndex = -1;
            networkInterfaceName = string.Empty;
            // Using MaxApiRetryAttempts since more retries are needed to ensure ip address is plumbed correctly.
            for (int i = 0; i < Constants.MaxApiRetryAttempts; i++)
            {
                DeployerTrace.WriteInfo("Waiting to get network interface details for ip address {0}.", ipv4Address.ToString());

                Thread.Sleep(Constants.ApiRetryIntervalMilliSeconds);

                int nwiIndex = -1;
                string nwiName = string.Empty;
                GetNetworkInterfaceIndexAndName(ipv4Address, out nwiIndex, out nwiName);
                if (nwiIndex != -1)
                {
                    networkInterfaceIndex = nwiIndex;
                    networkInterfaceName = nwiName;
                    break;
                }
            }

            DeployerTrace.WriteInfo("Network interface details for ip address {0} are index: {1}, name: {2}.", ipv4Address.ToString(), networkInterfaceIndex, networkInterfaceName);
        }
#endif // !DotNetCoreClr

#if DotNetCoreClrLinux
        internal static void ForceAptPackageInstall(string packageName, ref bool installedInThisRun)
        {
            ProcessResult result;
            installedInThisRun = false;

            string packageCleanlyInstalledCmd = string.Format(Constants.DpkgPackageCleanlyInstalledCommandFormat, packageName);
            result = Utility.RunLinuxBashCommand(packageCleanlyInstalledCmd, Environment.CurrentDirectory, TimeSpan.FromSeconds(30), true);

            if (result.ExitCode == 0)
            {
                DeployerTrace.WriteInfo("Package {0} is already installed.", packageName);

                string upgradePackageCmd = string.Format(Constants.AptUpgradePackageCommandFormat, packageName);
                Utility.RunAptLockLinuxBashCommand(upgradePackageCmd, Environment.CurrentDirectory, TimeSpan.FromMinutes(15), true);

                return;
            }

            string packageStateVisibleCmd = string.Format(Constants.DpkgPackageStateVisibleCommandFormat, packageName);
            result = Utility.RunLinuxBashCommand(packageStateVisibleCmd, Environment.CurrentDirectory, TimeSpan.FromSeconds(30), true);
            if (result.ExitCode == 0)
            {
                DeployerTrace.WriteInfo("Package {0} is in state: {1}.", packageName, result.Output);
            }
            else
            {
                DeployerTrace.WriteInfo("Package {0} is not installed.", packageName, result.Output);
            }

            string installPackageCmd = string.Format(Constants.AptPackageInstallCommand, packageName);
            result = Utility.RunAptLockLinuxBashCommand(installPackageCmd, Environment.CurrentDirectory, TimeSpan.FromMinutes(15), true);
            if (result.ExitCode == 0)
            {
                result = Utility.RunLinuxBashCommand(packageCleanlyInstalledCmd, Environment.CurrentDirectory, TimeSpan.FromSeconds(30), true);
                if (result.ExitCode == 0)
                {
                    DeployerTrace.WriteInfo("Package {0} has been successfully installed.", packageName);
                    installedInThisRun = true;
                    return;
                }
            }

            Utility.RunAptLockLinuxBashCommand(Constants.AptUpdateCommand, Environment.CurrentDirectory, TimeSpan.FromMinutes(15), true);
            Utility.RunAptLockLinuxBashCommand(Constants.DpkgConfigureCommand, Environment.CurrentDirectory, TimeSpan.FromMinutes(15), true);

            result = Utility.RunAptLockLinuxBashCommand(installPackageCmd, Environment.CurrentDirectory, TimeSpan.FromMinutes(15), true);
            if (result.ExitCode != 0)
            {
                string message = string.Format("Command '{0}' hit error {1}.", installPackageCmd, result.ExitCode);
                DeployerTrace.WriteError(message);
            }

            result = Utility.RunLinuxBashCommand(packageCleanlyInstalledCmd, Environment.CurrentDirectory, TimeSpan.FromSeconds(30), true);
            if (result.ExitCode == 0)
            {
                DeployerTrace.WriteInfo("Package {0} has been successfully installed (second attempt).", packageName);
                installedInThisRun = true;
                return;
            }

            // If Moby is still not installed, state is corrupted. Attempt full purge & reinstall as automitigation.
            Utility.PurgeAptPackageIfPresent(packageName);
            Utility.RunAptLockLinuxBashCommand(Constants.AptAutoremoveCommand, Environment.CurrentDirectory, TimeSpan.FromMinutes(15), true);
            result = Utility.RunAptLockLinuxBashCommand(installPackageCmd, Environment.CurrentDirectory, TimeSpan.FromMinutes(15), true);
            if (result.ExitCode == 0)
            {
                DeployerTrace.WriteInfo("Package {0} has been successfully installed (third attempt).", packageName);
                installedInThisRun = true;
                return;
            }
            else
            {
                throw new InvalidDeploymentException(string.Format("Check machine state for {0}. State is wedged.", packageName));
            }
        }

        internal static ProcessResult RunAptLockLinuxBashCommand(string dpkgCommand, string workingDirectory, TimeSpan timeout, bool trace = false)
        {
            ProcessResult result;
            var timeoutHelper = new TimeoutHelper(timeout);
            int retryAttempt = 0;
            do
            {
                if (retryAttempt != 0)
                {
                    DeployerTrace.WriteInfo("Executing attempt: {0}", retryAttempt);
                }

                result = Utility.RunLinuxBashCommand(dpkgCommand, workingDirectory, InstallTimeout, trace: true);

                if (result.ExitCode == 100 || result.Output.Contains("Could not get lock /var/lib/dpkg/lock"))
                {
                    DeployerTrace.WriteInfo("Encountered apt lock. Attempt: {0}", retryAttempt);
                    Utility.RunLinuxBashCommand("ps aux | grep apt", workingDirectory, TimeSpan.FromMinutes(10), trace: true);
                    Thread.Sleep(TimeSpan.FromSeconds(10));
                }
            } while (result.ExitCode != 0 && !TimeoutHelper.HasExpired(timeoutHelper));

            return result;
        }

        internal static ProcessResult RunLinuxBashCommand(string bashCmd, string workingDirectory, TimeSpan timeout, int[] expectedErrorCodes, int sleepTimeInSecOnRetry = 10, int retryCount = 5, bool trace = false)
        {
            ProcessResult result;
            int retryAttempt = 0;

            if (expectedErrorCodes == null)
            {
                expectedErrorCodes = new int[] { 0 };
            }

            do
            {
                if (retryAttempt != 0)
                {
                    DeployerTrace.WriteInfo("Attempt: {0}", retryAttempt);
                }

                result = Utility.RunLinuxBashCommand(bashCmd, workingDirectory, timeout, trace: trace);
                retryAttempt++;
                
                if (!IsExitCodeExpected(result, expectedErrorCodes))
                {
                    Thread.Sleep(TimeSpan.FromSeconds(sleepTimeInSecOnRetry));
                }

            } while (!IsExitCodeExpected(result, expectedErrorCodes) && retryAttempt < retryCount);

            return result;
        }

        internal static ProcessResult RunLinuxBashCommand(string bashCmd, string workingDirectory, TimeSpan timeout, bool trace = false)
        {
            return Utility.RunProcessOnLinux("sh", string.Format(CultureInfo.InvariantCulture, "-c \"{0}\"", bashCmd), workingDirectory, timeout, trace: trace);
        }

        internal static ProcessResult RunProcessOnLinux(string command, string arguments, string workingDirectory, TimeSpan timeout, bool trace = false)
        {
            if (trace)
            {
                DeployerTrace.WriteInfo("Running process {0} {1} with timeout {2}...\nWorkingDirectory: {3}", command, arguments, timeout, workingDirectory);
            }

            var processResult = RunProcessOnLinux(command, arguments, workingDirectory, timeout);

            if (trace)
            {
                DeployerTrace.WriteInfo("Process {0} completed with exit code {1}. Output: \n{2}", command, processResult.ExitCode, processResult.Output);
            }

            return processResult;
        }

        /// <summary>
        /// Runs the specified process and saves the output to specifid log file.
        /// </summary>
        /// <returns>The exit code from the process.</returns>
        internal static ProcessResult RunProcessOnLinux(string command, string arguments, string workingDirectory, TimeSpan timeout)
        {
            int outputStreamEnded = 0;
            var result = new ProcessResult();
            var startInfo = new ProcessStartInfo(command, arguments);
            startInfo.UseShellExecute = false;
            startInfo.CreateNoWindow = true;
            startInfo.RedirectStandardOutput = true;
            startInfo.RedirectStandardError = true;
            startInfo.WorkingDirectory = workingDirectory;

            var process = Process.Start(startInfo);
            var sb = new StringBuilder();
            DataReceivedEventHandler dataReceivedHandler = (sender, e) =>
            {
                if (e.Data != null)
                {
                    sb.AppendLine(e.Data);
                }
                else
                {
                    Interlocked.Increment(ref outputStreamEnded);
                }
            };

            process.OutputDataReceived += dataReceivedHandler;
            process.ErrorDataReceived += dataReceivedHandler;
            process.BeginOutputReadLine();
            process.BeginErrorReadLine();

            bool completed = process.WaitForExit((int)timeout.TotalMilliseconds);
            if (!completed)
            {
                DeployerTrace.WriteWarning("Timeout {0} exceeded, terminating process..", timeout);
                process.Kill();
            }

            if (outputStreamEnded < 2)
            {
                SpinWait.SpinUntil(() => outputStreamEnded == 2, ProcessOutputWaitTimeout);
            }

            result.ExitCode = process.ExitCode;
            result.Output = sb.ToString();

            return result;
        }

        internal static bool IsExitCodeExpected(ProcessResult result, int[] expectedExitCodes)
        {
            return expectedExitCodes.Any(code => code == result.ExitCode);
        }

        internal static void PurgeAptPackageIfPresent(string packageName)
        {
            string packageVisibleInDpkgState = string.Format(Constants.DpkgPackageStateVisibleCommandFormat, packageName);
            ProcessResult result = Utility.RunLinuxBashCommand(packageVisibleInDpkgState, Environment.CurrentDirectory, TimeSpan.FromSeconds(30), true);
            if (result.ExitCode == 0)
            {
                string purgePackageCmd = string.Format(Constants.AptPurgePackageCommandFormat, packageName);
                result = Utility.RunAptLockLinuxBashCommand(purgePackageCmd, Environment.CurrentDirectory, TimeSpan.FromMinutes(15), true);

                if (result.ExitCode != 0)
                {
                    throw new InvalidDeploymentException(string.Format("Purging package {0} was not successful. Exit code: {1}, Output: {2}", packageName, result.ExitCode, result.Output));
                }
            }
        }
#endif
    }

    internal class ProcessResult
    {
        public int ExitCode { get; set; }
        public string Output { get; set; }
    }
}
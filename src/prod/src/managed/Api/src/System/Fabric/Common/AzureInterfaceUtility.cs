// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using FabricAzureInterface;
    using Microsoft.Win32;
    using System;
    using System.Collections.Generic;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.ServiceProcess;
    
    // This class enables Windows Fabric components to use Azure APIs 
    // in a SDK version agnostic manner
    internal class AzureInterfaceUtility
    {
        private const string AzureWrapperServiceName = "WindowsFabricAzureWrapperService";
        private const string PluginPathRelativeToRoleRoot = "%RoleRoot%\\plugins\\WindowsFabric";
        private const string RegistryPath = @"SYSTEM\CurrentControlSet\Services\";

        private static string sdkWrapperCodeBase;
        private static Exception sdkWrapperCodeBaseInitException;

        private static bool AzureWrapperServiceExists()
        {
            return (0 != ServiceController.GetServices()
                         .Where(
                             s => s.ServiceName.Equals(AzureWrapperServiceName, StringComparison.OrdinalIgnoreCase))
                         .Count());
        }

#if !DotNetCoreClrLinux
        private static string GetPluginDirectoryFromServicePath()
        {
            string mgmtPathAsString = RegistryPath + AzureWrapperServiceName;
            var localMachine = Registry.LocalMachine;

            RegistryKey key = localMachine.OpenSubKey(mgmtPathAsString);
            if (key == null)
            {
                throw new ArgumentNullException("Null path returned for service:{0}", AzureWrapperServiceName);
            }
              
            String serviceExePath = key.GetValue("ImagePath").ToString();

            // If the path is enclosed in quotes, remove them
            if (serviceExePath.StartsWith("\"", StringComparison.Ordinal))
            {
                serviceExePath = serviceExePath.Remove(0, 1);
            }
            if (serviceExePath.EndsWith("\"", StringComparison.Ordinal))
            {
                serviceExePath = serviceExePath.Remove((serviceExePath.Length-1), 1);
            }

            // Expand any environment variables that the path might contain
            string serviceExePathExpanded = Environment.ExpandEnvironmentVariables(serviceExePath);
            return Path.GetDirectoryName(serviceExePathExpanded);
        }
#endif

        private static string GetPluginDirectoryFromAppRoot()
        {
            return Environment.ExpandEnvironmentVariables(PluginPathRelativeToRoleRoot);
        }

        static AzureInterfaceUtility()
        {
            // Initialize the Azure SDK wrapper path
            //
            // We load the Azure SDK wrapper from our Azure plugin directory.
            // The WindowsFabricAzureWrapperService ships with the plugin and 
            // is installed as a Windows Service. So we query for the path to 
            // the service's executable in order to determine where the plugin
            // directory is located.
            //
            // The above does not work when we run on the Azure Compute Emulator
            // because the WindowsFabricAzureWrapper is not installed as a 
            // Windows Service in that case. For that case only, we determine
            // the plugin directory by starting at the approot and appending the
            // path to the plugin folder. While this is not as robust as determining
            // it via the service path, it should be sufficient for the emulator.
            sdkWrapperCodeBase = String.Empty;
            sdkWrapperCodeBaseInitException = null;

            // Check if the Azure wrapper service exists
            bool azureWrapperServiceExists;
            try
            {
                azureWrapperServiceExists = AzureWrapperServiceExists();
            }
            catch (Exception e)
            {
                // We could not even determine whether the Azure wrapper service exists.
                sdkWrapperCodeBaseInitException = e;
                return;
            }

            string pluginDir;
#if !DotNetCoreClrLinux
            if (azureWrapperServiceExists)
            {
                // Wrapper service exists, query its exe path to determine where the
                // plugin directory is located.
                try
                {
                    pluginDir = GetPluginDirectoryFromServicePath();
                }
                catch (Exception e)
                {
                    // We were unable to determine the plugin directory from the
                    // service path
                    sdkWrapperCodeBaseInitException = e;
                    return;
                }
            }
            else
#endif
            {
                // Wrapper service does not exist. Determine the plugin directory via
                // the approot
                try
                {
                    pluginDir = GetPluginDirectoryFromAppRoot();
                }
                catch (Exception e)
                {
                    // We were unable to determine the plugin directory from the approot
                    sdkWrapperCodeBaseInitException = e;
                    return;
                }
            }

            // From the plugin directory, compute the codeBase
            try
            {
                AssemblyName assemblyName = new AssemblyName(AzureWrapper.WrapperModule);
                string sdkWrapperName = Path.ChangeExtension(assemblyName.Name, ".dll");
                string sdkWrapperPath = Path.Combine(pluginDir, sdkWrapperName);
                Uri sdkWrapperUri = new Uri(sdkWrapperPath);
                sdkWrapperCodeBase = sdkWrapperUri.AbsoluteUri;
            }
            catch (Exception e)
            {
                sdkWrapperCodeBaseInitException = e;
                return;
            }
        }

        private static string GetSdkWrapperPath()
        {
            if (String.IsNullOrEmpty(sdkWrapperCodeBase))
            {
                throw new InvalidOperationException(
                              String.Format(
                                "Unable to determine path to Azure plugin directory.{0}{1}",
                                (null == sdkWrapperCodeBaseInitException) ? String.Empty : " Exception: ",
                                (null == sdkWrapperCodeBaseInitException) ? String.Empty : sdkWrapperCodeBaseInitException.ToString()));
            }
            return sdkWrapperCodeBase;
        }

        internal static T GetInterface<T>(string typeName)
        {
            // Create the assembly name
            AssemblyName assemblyName = new AssemblyName(AzureWrapper.WrapperModule);
#if !DotNetCoreClrLinux
            assemblyName.CodeBase = GetSdkWrapperPath();
#endif

            // Load the consumer assembly
            Assembly assembly = Assembly.Load(assemblyName);
  
            // Get the type of the consumer object from the assembly
            Type type = assembly.GetType(typeName);
            if (null == type)
            {
                throw new TypeLoadException(String.Format(CultureInfo.CurrentCulture, StringResources.Error_TypeLoadAssembly,
                                             typeName, assembly));
            }
            // Create an object of the specified type
            object azureApiObject = Activator.CreateInstance(type);

            // Cast the object to the interface desired by the caller
            T itf = (T)azureApiObject;

            if (null == itf)
            {
                throw new InvalidOperationException(
                              String.Format(
                                "Interface {0} could not be obtained from assembly {1}, type {2}.",
                                typeof(T),
                                AzureWrapper.WrapperModule,
                                typeName));
            }

            return itf;
        }
    }
}
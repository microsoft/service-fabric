//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace AzureFilesVolumePluginSetup
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Description;
    using System.IO;
    using System.Linq;
    using System.Threading.Tasks;
    using AzureFilesVolumePlugin;
    using Microsoft.WindowsAzure.Storage.Blob;

    class Program
    {
        private const string PluginDirectoryName = "plugins";
        private const string PluginRegistrationFileName = "sfazurefile.json";
        private const string DockerDirectoryWindows = "%PROGRAMDATA%\\docker";
        private const string DockerDirectoryLinux = "/etc/docker";
        private const string PluginRegistrationFileTemplate = "{{\"Name\":\"sfazurefile\", \"Addr\": \"http://{0}:{1}\"}}";

        private static ConfigurationSection configSection;

        static void Main(string[] args)
        {
            var cpac = FabricRuntime.GetActivationContext();

            // Get config section
            configSection = Utilities.GetConfigSection(cpac);

            // Get OS
            var os = Utilities.GetOperatingSystem(configSection);

            // Figure out the docker directory
            var dockerDirectory = (AzureFilesVolumePluginSupportedOs.Linux == os) ?
                DockerDirectoryLinux :
                DockerDirectoryWindows;
            dockerDirectory = Environment.ExpandEnvironmentVariables(dockerDirectory);

            // Register the plugin with Docker
            WritePluginRegistrationFile(
                cpac,
                dockerDirectory,
                Constants.HttpEndpointName);
        }

        private static void WritePluginRegistrationFile(
            CodePackageActivationContext cpac,
            string dockerDirectory,
            string endpointName)
        {
            var pluginRegistrationDirectory = Path.Combine(dockerDirectory, PluginDirectoryName);
            Utilities.EnsureFolder(pluginRegistrationDirectory);

            var listenAddress = cpac.ServiceListenAddress;
            var endpointPort = cpac.GetEndpoint(endpointName).Port.ToString();

            var pluginRegistrationFilePath = Path.Combine(pluginRegistrationDirectory, PluginRegistrationFileName);
            File.WriteAllText(
                pluginRegistrationFilePath,
                String.Format(PluginRegistrationFileTemplate, listenAddress, endpointPort));
        }
    }
}

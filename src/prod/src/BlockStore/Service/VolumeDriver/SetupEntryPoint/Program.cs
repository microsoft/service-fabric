// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace BSDockerVolumePluginSetup
{
    using System;
    using System.Fabric;
    using System.IO;
    using System.Runtime.InteropServices;
    using BSDockerVolumePlugin;

    class Program
    {
        // Make sure DockerPluginName("sfvolumedisk") match the name defined in the following files:
        // src/prod/src/managed/Api/src/System/Fabric/Management/ImageBuilder/SingleInstance/VolumeParametersVolumeDisk.cs
        // src/prod/src/BlockStore/Service/Native/inc/common.h
        // src/prod/src/BlockStore/Service/VolumeDriver/SetupEntryPoint/Program.cs
        private const string DockerPluginName = "sfvolumedisk";
        private static string EndpointName = "ServiceEndpoint";

        private static string PluginRegistrationFilePathWindows = Environment.ExpandEnvironmentVariables("%PROGRAMDATA%\\docker\\plugins");
        private static string PluginRegistrationFileNameWindows = Environment.ExpandEnvironmentVariables("%PROGRAMDATA%\\docker\\plugins\\" + DockerPluginName + ".json");
        private static string PluginRegistrationFilePathForServiceWindows = Environment.ExpandEnvironmentVariables("%PROGRAMDATA%\\SF\\VolumeDriver");
        private static string PluginRegistrationFileNameForServiceWindows = Environment.ExpandEnvironmentVariables("%PROGRAMDATA%\\SF\\VolumeDriver\\" + DockerPluginName + ".json");
        private static string PluginRegistrationFilePathForServiceLinux = "VolumeDriver/";
        private static string ServiceFabricDataRootFileLinux = "/etc/servicefabric/FabricDataRoot";
        private static string PluginRegistrationFileNameForServiceLinux = DockerPluginName + ".json";
        private static string PluginRegistrationFilePathLinux = "/etc/docker/plugins";
        private static string PluginRegistrationFileNameLinux = "/etc/docker/plugins/" + DockerPluginName + ".json";
        private static string PluginRegistrationFileContents = "{{\"Name\":\"" + DockerPluginName + "\", \"Addr\": \"http://{0}:{1}\"}}";

        static void Main(string[] args)
        {
            var activationContext = FabricRuntime.GetActivationContext();
            var listenAddress = activationContext.ServiceListenAddress;
            var endpointPort = activationContext.GetEndpoint(EndpointName).Port.ToString();

            var os = Utilities.GetOperatingSystem(activationContext);

            EnsureFolder(
                (os == BSDockerVolumePluginSupportedOs.Linux) ?
                  PluginRegistrationFilePathLinux :
                  PluginRegistrationFilePathWindows);

            File.WriteAllText(
                (os == BSDockerVolumePluginSupportedOs.Linux) ?
                  PluginRegistrationFileNameLinux :
                  PluginRegistrationFileNameWindows,
                String.Format(PluginRegistrationFileContents, listenAddress, endpointPort));

            // On Windows, ensure that the secondary path to read this information from is ready as well.
            if (os == BSDockerVolumePluginSupportedOs.Windows)
            {
                EnsureFolder(PluginRegistrationFilePathForServiceWindows);

                File.WriteAllText(PluginRegistrationFileNameForServiceWindows,
                String.Format(PluginRegistrationFileContents, listenAddress, endpointPort));
            }
            else if (os == BSDockerVolumePluginSupportedOs.Linux)
            {
                try
                {
                    var fabricDataRoot = File.ReadAllText(ServiceFabricDataRootFileLinux);
                    PluginRegistrationFilePathForServiceLinux = Path.Combine(fabricDataRoot,
                                                                             PluginRegistrationFilePathForServiceLinux);
                    EnsureFolder(PluginRegistrationFilePathForServiceLinux);

                    File.WriteAllText(Path.Combine(PluginRegistrationFilePathForServiceLinux,
                                                   PluginRegistrationFileNameForServiceLinux),
                                      String.Format(PluginRegistrationFileContents,
                                                    listenAddress,
                                                    endpointPort));
                 
                    const int defaultFilePermission = 0x1ed;
                    Chmod(PluginRegistrationFileNameForServiceLinux, defaultFilePermission);
                }
                catch
                {
                }
            }
        }

        //Linux Specific PInvoke methods.
        [DllImport("libc.so.6", EntryPoint="chmod")]
        private static extern int Chmod(string path, int mode);

        private static void EnsureFolder(string folderPath)
        {
            if (!Directory.Exists(folderPath))
            {
                Directory.CreateDirectory(folderPath);
            }
        }
    }
}

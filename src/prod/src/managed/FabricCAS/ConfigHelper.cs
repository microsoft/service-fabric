// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Hosting.ContainerActivatorService
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.IO;
    using System.Linq;
    using Microsoft.ServiceFabric.ContainerServiceClient.Config;

    internal class ConfigHelper
    {
        internal static readonly string TraceType = "ConfigHelper";

        internal static readonly string FabricPackageFileNameEnvironment = "FabricPackageFileName";
        internal static readonly string ContainerNameEnvironmentVariable = "Fabric_ContainerName";
        internal static readonly string RuntimeSslConnectionCertFilePath = "Fabric_RuntimeSslConnectionCertFilePath";
        internal static readonly string RootNamingUriString = "fabric:";

        internal const double DockerVersionThreshold = 17.06;

        // When using docker we translate cores to nano cpus so that 10^9 of nano cpus = 1 core
        internal const ulong DockerNanoCpuMultiplier = 1000000000;

        internal const ulong MegaBytesToBytesMultiplier = 1024 * 1024;

        //
        // Container label key names/values. These names must match with key names specified
        // in Constants.cpp file under src/Hosting2 folder.
        //
        internal static readonly string ApplicationNameLabelKeyName = "ApplicationName";
        internal static readonly string ApplicationIdLabelKeyName = "ApplicationId";
        internal static readonly string ServiceNameLabelKeyName = "ServiceName";
        internal static readonly string CodePackageNameLabelKeyName = "CodePackageName";
        internal static readonly string CodePackageInstanceLabelKeyName = "CodePackageInstance";
        internal static readonly string DigestedApplicationNameLabelKeyName = "DigestedApplicationName";
        internal static readonly string DigestedApplicationNameDockerSuffix = "_docker";
        internal static readonly string ServicePackageActivationIdLabelKeyName = "ServicePackageActivationId";
        internal static readonly string SimpleApplicationNameLabelKeyName = "SimpleApplicationName";
        internal static readonly string PlatformLabelKeyName = "Platform";
        internal static readonly string PlatformLabelKeyValue = "ServiceFabric";
        internal static readonly string PartitionIdLabelKeyName = "PartitionId";

        internal static readonly string CodePackageInstanceIdFileName = "containerid";

        internal static readonly string UserLogsDirectoryEnvVarName = "UserLogsDirectory";
        //
        // Make sure the following value matched:
        // PartitionIdToken at /src/prod/src/managed/FabricCAS/ConfigHelper.cs
        // servicePartitionId at /src/prod/src/managed/Api/src/System/Fabric/Management/ImageBuilder/SingleInstance/VolumeDescriptionVolumeDisk.cs
        //
        internal static readonly string PartitionIdToken = "@PartitionId393d4d7b-69cd-4561-9052-29f3685bf629@";
        internal static readonly string QueryFilterLabelKeyName = "ServiceName_CodePackageName_PartitionId";
        internal static readonly string SrppQueryFilterLabelKeyName = "ServiceName_CodePackageName_PartitionId";
        internal static readonly string MrppQueryFilterLabelKeyName = "ServiceName_CodePackageName_ApplicationId";

        public static void ReplaceWellKnownParameters(VolumeConfig volumeConfig, ContainerDescription containerDesc)
        {
            // Replace the PartitionId token in Docker Volume Driver options
            List<string> keysforPartition = new List<string>();
            foreach(var key in volumeConfig.DriverOpts.Keys)
            {
                if(volumeConfig.DriverOpts[key] == PartitionIdToken)
                {
                    keysforPartition.Add(key);
                }
            }
            foreach(var key in keysforPartition)
            {
                volumeConfig.DriverOpts[key] = containerDesc.PartitionId;
            }

            // Replace the PartitionId token in Source, if specified.
            if (volumeConfig.Name.Contains(PartitionIdToken))
            {
                volumeConfig.Name = volumeConfig.Name.Replace(PartitionIdToken, containerDesc.PartitionId);
            }
        }

        public static IList<VolumeConfig> GetVolumeConfig(
            ContainerDescription containerDesc)
        {
            var volumeConfigs = new List<VolumeConfig>();

            foreach (var volume in containerDesc.Volumes)
            {
                if(!string.IsNullOrEmpty(volume.Driver))
                {
                    var driverOpts = ParseDriverOptions(volume.DriverOpts);

                    var volumeCreateParam = new VolumeConfig
                    {
                        Name = volume.Source,
                        Driver = volume.Driver,
                        DriverOpts = driverOpts
                    };

                    ReplaceWellKnownParameters(volumeCreateParam, containerDesc);

                    // Update the Volume source incase it was updated due to token replacement.
                    volume.Source = volumeCreateParam.Name;
                    volumeConfigs.Add(volumeCreateParam);
                }
            }

            return volumeConfigs;
        }

        public static ContainerConfig GetContainerConfig(
            ProcessDescription processDesc,
            ContainerDescription containerDesc,
            string dockerVersion)
        {
            var containerConfig = new ContainerConfig
            {
                Image = processDesc.ExePath,
                Hostname = containerDesc.HostName,
                Labels = new Dictionary<string, string>(),
                Env = new List<string>(),
                ExposedPorts = new Dictionary<string, EmptyStruct>(),
                Tty = containerDesc.RunInteractive,
                OpenStdin = containerDesc.RunInteractive,
                HostConfig = new HostConfig()
                {
                    Binds = new List<string>(),
                    PortBindings = new Dictionary<string, IList<PortBinding>>(),
                    DNSSearch = new List<string>(),
                    SecurityOpt = containerDesc.SecurityOptions,
                    AutoRemove = containerDesc.AutoRemove,
                    LogConfig = new LogConfig()
                }
            };

            #if !DotNetCoreClrLinux
                containerConfig.HostConfig.Isolation = containerDesc.IsolationMode.ToString().ToLower();
            #endif

            PopulateLabels(containerConfig, processDesc, containerDesc);

            PopulateProcessEnvironment(containerConfig, processDesc, containerDesc);

            if (!containerDesc.RemoveServiceFabricRuntimeAccess)
            {
                PopulateFabricRuntimeEnvironment(containerConfig, processDesc, containerDesc);
            }

            PopulateVolumes(containerConfig, containerDesc);

            PopulateDebugParameters(containerConfig, processDesc, containerDesc);

            PopulateNetworkSettings(containerConfig, containerDesc);

            PopulateResourceGovernanceSettings(containerConfig, processDesc, dockerVersion);

            PopulateLogConfig(containerConfig, containerDesc);

            AddContainerLogMount(containerConfig, containerDesc);

            return containerConfig;
        }

        private static ulong GetNewContainerInstanceIdAtomic(
            ContainerDescription containerDesc)
        {
#if DotNetCoreClrLinux
            // There is a file called containerid in
            // /log/application/partitionid/servicepackageactivationid/codepackagename/
            // This file contains the number corresponding to the latest folder for logs
            // /mnt/log/application/partitionid/servicepackageactivationid/codepackagename/number/application.log
            //
            // For the first instance of thecontainer, create a new file and start at 0,
            // For every container afterwards, check the file exists, read from the file
            // and add 1 to the value in the file in order to get the new current instance id
            // and save the value to the file.
            var applicationNamePart = containerDesc.ApplicationName;
            var applicationNameExtraPart = string.Format("{0}/", RootNamingUriString);
            if (applicationNamePart.StartsWith(applicationNameExtraPart))
            {
                applicationNamePart = applicationNamePart.Substring(applicationNameExtraPart.Length);
            }

            applicationNamePart = applicationNamePart.Replace('/', '_');
            var digestedApplicationName = applicationNamePart + DigestedApplicationNameDockerSuffix;

            var logRoots = "/mnt/logs";

            var logPath = Path.Combine(logRoots, digestedApplicationName, containerDesc.PartitionId, containerDesc.ServicePackageActivationId, containerDesc.CodePackageName);

            if (!FabricDirectory.Exists(logPath))
            {
                FabricDirectory.CreateDirectory(logPath);
            }

            ulong containerId = 0;
            var containerInstanceIdFilePath = Path.Combine(logPath, CodePackageInstanceIdFileName);
            if (!FabricFile.Exists(containerInstanceIdFilePath))
            {
                using (var containerIdFile = FabricFile.Create(containerInstanceIdFilePath))
                {
                    containerId = 0;
                }
            }
            else
            {
                containerId = ReadContainerIdFromFile(containerInstanceIdFilePath) + 1;
            }

            WriteContainerIdToFile(containerInstanceIdFilePath, containerId);

            return containerId;
#else
            return 0;
#endif
        }

        private static ulong ReadContainerIdFromFile(string filePath)
        {
            ulong containerId = 0;

            try
            {
                using (var containerIdFile = FabricFile.Open(filePath, FileMode.Open))
                {
                    using (StreamReader reader = new StreamReader(containerIdFile))
                    {
                        var line = reader.ReadLine();
                        containerId = Convert.ToUInt64(line);
                    }
                }
            }
            catch (Exception ex)
            {
                HostingTrace.Source.WriteWarning(
                    TraceType,
                    "ReadContainerIdFromFile() encountered error. filePath={0} Exception={1}.",
                    filePath,
                    ex.ToString());
                throw;
            }

            return containerId;
        }

        private static void WriteContainerIdToFile(string filePath, ulong containerId)
        {
            try
            {
                using (var containerIdFile = FabricFile.Open(filePath, FileMode.Open))
                {
                    using (var writer = new StreamWriter(containerIdFile))
                    {
                        writer.WriteLine(containerId);
                    }
                }
            }
            catch (Exception ex)
            {
                HostingTrace.Source.WriteWarning(
                    TraceType,
                    "WriteContainerIdToFile() encountered error. filePath={0}, containerId={1}, Exception={2}.",
                    filePath,
                    containerId,
                    ex.ToString());
                throw;
            }
        }

        private static void AddContainerLogMount(
            ContainerConfig containerConfig,
            ContainerDescription containerDesc)
        {
            // Create mounting Path
            // Pattern is Root/ApplicationName/PartitionId/ServicePackageActivationId/CodePackageName,
            // we will mount up to Root/ApplicationName/PartitionId/ServicePackageActivationId
            // ApplicationName might come as fabric:/MyAppName. User really only cares about the MyAppName portion.
            // The / character in the app name will be converted to _ for the directory
            // so we can keep only one directory instead of multiple (1 for each / in the name)
            // If the name is fabric:/MyAppName/AnotherPartOfTheName, we will convert it to MyAppName_AnotherPartOfTheName

            // On linux the mount root is /mnt/logs/
#if DotNetCoreClrLinux
            var applicationNamePart = containerDesc.ApplicationName;
            var applicationNameExtraPart = string.Format("{0}/", RootNamingUriString);
            if (applicationNamePart.StartsWith(applicationNameExtraPart))
            {
                applicationNamePart = applicationNamePart.Substring(applicationNameExtraPart.Length);
            }

            applicationNamePart = applicationNamePart.Replace('/', '_');
            containerConfig.Labels.Add(SimpleApplicationNameLabelKeyName, applicationNamePart);

            var digestedApplicationName = applicationNamePart + DigestedApplicationNameDockerSuffix;
            containerConfig.Labels.Add(DigestedApplicationNameLabelKeyName, digestedApplicationName);

            var logRoots = "/mnt/logs";

            var logPath = Path.Combine(logRoots, digestedApplicationName, containerDesc.PartitionId, containerDesc.ServicePackageActivationId);

            if (Path.GetFullPath(logPath).Equals(Path.GetFullPath(logRoots)))
            {
                // We do no want to mount the root path to a container
                return;
            }

            // Create path
            FabricDirectory.CreateDirectory(logPath);

            // Add to environment label
            containerConfig.Env.Add(string.Format("{0}={1}", UserLogsDirectoryEnvVarName, logPath));

            // Add to mount options
            containerConfig.HostConfig.Binds.Add(string.Format("{0}:{0}", logPath));
#endif
        }

        private static void PopulateLabels(
            ContainerConfig containerConfig,
            ProcessDescription processDesc,
            ContainerDescription containerDesc)
        {
            //
            // Add labels from debug params if any
            //
            var containerLabels = processDesc.DebugParameters.ContainerLabels;
            if (containerLabels != null && containerLabels.Count > 0)
            {
                foreach(var label in containerLabels)
                {
                    var tokens = label.Split(new[] { "=" }, StringSplitOptions.RemoveEmptyEntries);
                    if(tokens != null && tokens.Length !=2)
                    {
                        throw new FabricException(
                            $"Container label '{label}' specified in debug parameters has incorrect format. Expected format:'<label-key>=<label-value>'",
                            FabricErrorCode.InvalidOperation);
                    }

                    containerConfig.Labels.Add(tokens[0], tokens[1]);
                }
            }

            // Add the labels present in container description
            if (containerDesc.Labels != null)
            {
                foreach(var label in containerDesc.Labels)
                {
                    containerConfig.Labels.Add(label.Name, label.Value);
                }
            }

            //
            // Add labels from hosting subsystem
            //
            containerConfig.Labels.Add(ApplicationNameLabelKeyName, containerDesc.ApplicationName);
            containerConfig.Labels.Add(ApplicationIdLabelKeyName, containerDesc.ApplicationId);
            containerConfig.Labels.Add(ServiceNameLabelKeyName, containerDesc.ServiceName);
            containerConfig.Labels.Add(CodePackageNameLabelKeyName, containerDesc.CodePackageName);
            containerConfig.Labels.Add(ServicePackageActivationIdLabelKeyName, containerDesc.ServicePackageActivationId);
            containerConfig.Labels.Add(PlatformLabelKeyName, PlatformLabelKeyValue);
            containerConfig.Labels.Add(PartitionIdLabelKeyName, containerDesc.PartitionId);

#if DotNetCoreClrLinux
            containerConfig.Labels.Add(CodePackageInstanceLabelKeyName, Convert.ToString(GetNewContainerInstanceIdAtomic(containerDesc)));
#endif

            var srppQueryFilterValue = $"{containerDesc.ServiceName}_{containerDesc.CodePackageName}_{containerDesc.PartitionId}";
            containerConfig.Labels.Add(SrppQueryFilterLabelKeyName, srppQueryFilterValue);

            var mrppQueryFilterValue = $"{containerDesc.ServiceName}_{containerDesc.CodePackageName}_{containerDesc.ApplicationId}";
            containerConfig.Labels.Add(MrppQueryFilterLabelKeyName, mrppQueryFilterValue);
        }

        //
        // Fabric Runtime environment is not made visible inside the container if RemoveServiceFabricRuntimeAccess is specified.
        //
        private static void PopulateFabricRuntimeEnvironment(
            ContainerConfig containerConfig,
            ProcessDescription processDesc,
            ContainerDescription containerDesc)
        {
            var packageFilePath = string.Empty;
            if(processDesc.EnvVars.ContainsKey(FabricPackageFileNameEnvironment))
            {
                packageFilePath = processDesc.EnvVars[FabricPackageFileNameEnvironment];
            }

            var fabricContainerLogRoot = Path.Combine(Utility.FabricLogRoot, "Containers");
            var fabricContainerRoot = Path.Combine(fabricContainerLogRoot, containerDesc.ContainerName);

            // Set path to UT file
            // TODO: Bug#9728016 - Disable the bind until windows supports mounting file onto container

#if DotNetCoreClrLinux
            PopulateFabricRuntimeEnvironmentForLinux(
                containerConfig,
                containerDesc,
                processDesc,
                packageFilePath,
                fabricContainerRoot);
#else
            PopulateFabricRuntimeEnvironmentForWindows(
                containerConfig,
                containerDesc,
                processDesc,
                packageFilePath,
                fabricContainerRoot);
#endif
        }

        private static void PopulateFabricRuntimeEnvironmentForWindows(
            ContainerConfig containerConfig,
            ContainerDescription containerDesc,
            ProcessDescription processDesc,
            string packageFilePath,
            string fabricContainerRoot)
        {
            containerConfig.Env.Add(string.Format("FabricCodePath={0}", HostingConfig.Config.ContainerFabricBinRootFolder));
            containerConfig.Env.Add(string.Format("FabricLogRoot={0}", HostingConfig.Config.ContainerFabricLogRootFolder));

            var appDir = Path.Combine(HostingConfig.Config.ContainerAppDeploymentRootFolder, containerDesc.ApplicationId);
            containerConfig.HostConfig.Binds.Add(
                string.Format("{0}:{1}", Path.GetFullPath(processDesc.AppDirectory), appDir));

            if (!string.IsNullOrWhiteSpace(packageFilePath))
            {
                var configDir = Path.GetDirectoryName(packageFilePath);
                containerConfig.HostConfig.Binds.Add(
                    string.Format(
                        "{0}:{1}:ro",
                        Path.GetFullPath(configDir),
                        HostingConfig.Config.ContainerPackageRootFolder));
            }

            if (!string.IsNullOrWhiteSpace(Utility.FabricCodePath))
            {
                containerConfig.HostConfig.Binds.Add(
                    string.Format(
                        "{0}:{1}:ro",
                        Path.GetFullPath(Utility.FabricCodePath),
                        HostingConfig.Config.ContainerFabricBinRootFolder));
            }

            if (!string.IsNullOrWhiteSpace(fabricContainerRoot))
            {
                containerConfig.HostConfig.Binds.Add(
                    string.Format(
                        "{0}:{1}", // Log folder should be writable.
                        Path.GetFullPath(fabricContainerRoot),
                        HostingConfig.Config.ContainerFabricLogRootFolder));
            }

            // Mount the UT settings file
            // TODO: Bug#9728016 - Disable the bind until windows supports mounting file onto container
        }

        private static void PopulateFabricRuntimeEnvironmentForLinux(
            ContainerConfig containerConfig,
            ContainerDescription containerDesc,
            ProcessDescription processDesc,
            string packageFilePath,
            string fabricContainerRoot)
        {
            containerConfig.Env.Add(string.Format("FabricCodePath={0}", Path.GetFullPath(Utility.FabricCodePath)));
            containerConfig.Env.Add(string.Format("FabricLogRoot={0}", fabricContainerRoot));
            containerConfig.Env.Add(string.Format("FabricDataRoot={0}", Path.GetFullPath(Utility.FabricDataRoot)));

            containerConfig.HostConfig.Binds.Add(
                string.Format("{0}:{1}", processDesc.AppDirectory, processDesc.AppDirectory));

            if (!string.IsNullOrWhiteSpace(packageFilePath))
            {
                var configDir = Path.GetDirectoryName(packageFilePath);
                containerConfig.HostConfig.Binds.Add(string.Format("{0}:{1}:ro", configDir, configDir));
            }

            if (!string.IsNullOrWhiteSpace(Utility.FabricCodePath))
            {
                containerConfig.HostConfig.Binds.Add(
                    string.Format(
                        "{0}:{1}:ro",
                        Utility.FabricCodePath,
                        Utility.FabricCodePath));
            }

            if (!string.IsNullOrWhiteSpace(fabricContainerRoot))
            {
                containerConfig.HostConfig.Binds.Add(
                    string.Format("{0}:{1}", fabricContainerRoot, fabricContainerRoot));
            }

            if (processDesc.EnvVars.ContainsKey(RuntimeSslConnectionCertFilePath))
            {
                var certDir = Path.GetDirectoryName(processDesc.EnvVars[RuntimeSslConnectionCertFilePath]);
                containerConfig.HostConfig.Binds.Add(
                    string.Format("{0}:{1}:ro", certDir, certDir));
            }

            // Mount the UT settings file
            // TODO: Bug#9728016 - Disable the bind until windows supports mounting file onto container
        }

        private static void PopulateProcessEnvironment(
            ContainerConfig containerConfig,
            ProcessDescription processDesc,
            ContainerDescription containerDesc)
        {
            foreach (var kvPair in processDesc.EnvVars)
            {
                var envVarName = kvPair.Key;
                var envVarValue = kvPair.Value;

#if !DotNetCoreClrLinux
                if (envVarName.Equals(FabricPackageFileNameEnvironment, StringComparison.OrdinalIgnoreCase))
                {
                    var packageFilePath = kvPair.Value;
                    var packageFileName = Path.GetFileName(packageFilePath);
                    var containerPackageFilePath = Path.Combine(HostingConfig.Config.ContainerPackageRootFolder, packageFileName);

                    envVarValue = containerPackageFilePath;
                }
#endif
                containerConfig.Env.Add(string.Format("{0}={1}", envVarName, envVarValue));
            }

            foreach (var setting in processDesc.EncryptedEnvironmentVariables)
            {
                var value = setting.Value;
                string decryptedValue = Utility.GetDecryptedValue(value);
                containerConfig.Env.Add(string.Format("{0}={1}", setting.Key, decryptedValue));
            }

            containerConfig.Env.Add(
                string.Format("{0}={1}", ContainerNameEnvironmentVariable, containerDesc.ContainerName));

            containerConfig.Cmd =
                processDesc.Arguments.Split(new string[] { "," }, StringSplitOptions.RemoveEmptyEntries);
        }

        private static void PopulateVolumes(
            ContainerConfig containerConfig,
            ContainerDescription containerDesc)
        {
            foreach (var volume in containerDesc.Volumes)
            {
#if !DotNetCoreClrLinux
                if (string.IsNullOrEmpty(volume.Driver) && !FabricDirectory.Exists(volume.Source))
                {
                    throw new FabricException(
                        string.Format(
                            "The source location specified for container volume does not exist. Source:{0}",
                            volume.Source),
                        FabricErrorCode.DirectoryNotFound);
                }
#endif
                var bindFormat = volume.IsReadOnly ? "{0}:{1}:ro" : "{0}:{1}";

                containerConfig.HostConfig.Binds.Add(string.Format(bindFormat, volume.Source, volume.Destination));
            }

            foreach (var bind in containerDesc.BindMounts)
            {
                containerConfig.HostConfig.Binds.Add(
                    string.Format("{0}:{1}:ro", bind.Value, bind.Key));
            }
        }

        private static void PopulateNetworkSettings(
            ContainerConfig containerConfig,
            ContainerDescription containerDesc)
        {
            containerConfig.HostConfig.NetworkMode =  HostingConfig.Config.ContainerDefaultNetwork;

#if DotNetCoreClrLinux
            if (containerDesc.PortBindings.Count != 0)
            {
                containerConfig.HostConfig.NetworkMode =  HostingConfig.Config.DefaultNatNetwork;
            }
#endif

            var namespaceId = GetContainerNamespace(containerDesc.GroupContainerName);
            if (!string.IsNullOrEmpty(namespaceId))
            {
                containerConfig.HostConfig.IpcMode = namespaceId;
                containerConfig.HostConfig.UsernsMode = namespaceId;
                containerConfig.HostConfig.PidMode = namespaceId;
                containerConfig.HostConfig.UTSMode = namespaceId;
            }

            bool sharingNetworkNamespace = false;

            // network namespace can be shared for open network config and
            // for isolated network config.
            if (!string.IsNullOrEmpty(namespaceId))
            {
                // Open and/or Isolated
                if (((containerDesc.ContainerNetworkConfig.NetworkType & ContainerNetworkType.Open) == ContainerNetworkType.Open && 
                    !string.IsNullOrEmpty(containerDesc.ContainerNetworkConfig.OpenNetworkAssignedIp)) ||
                    ((containerDesc.ContainerNetworkConfig.NetworkType & ContainerNetworkType.Isolated) == ContainerNetworkType.Isolated &&
                    containerDesc.ContainerNetworkConfig.OverlayNetworkResources.Count > 0))
                {
                    containerConfig.HostConfig.NetworkMode = namespaceId;
                    containerConfig.HostConfig.PublishAllPorts = false;
                    sharingNetworkNamespace = true;
                }
            }
            else if (containerDesc.PortBindings.Count > 0)
            {
                foreach (var portBinding in containerDesc.PortBindings)
                {
                    containerConfig.HostConfig.PortBindings.Add(
                       portBinding.Key,
                       new List<PortBinding>
                       {
                            new PortBinding()
                            {
                                HostPort = portBinding.Value
                            }
                       });

                    containerConfig.ExposedPorts.Add(portBinding.Key, new EmptyStruct());
                }

                containerConfig.HostConfig.PublishAllPorts = false;
            }
            else if ((containerDesc.ContainerNetworkConfig.NetworkType & ContainerNetworkType.Open) == ContainerNetworkType.Open && 
                !string.IsNullOrEmpty(containerDesc.ContainerNetworkConfig.OpenNetworkAssignedIp))
            {
                var endPointSettings = new EndpointSettings()
                {
                    IPAMConfig = new EndpointIPAMConfig()
                    {
                        IPv4Address = containerDesc.ContainerNetworkConfig.OpenNetworkAssignedIp
                    }
                };

                containerConfig.HostConfig.NetworkMode = HostingConfig.Config.LocalNatIpProviderEnabled
                    ? HostingConfig.Config.LocalNatIpProviderNetworkName
                    : "servicefabric_network";

                containerConfig.NetworkingConfig = new NetworkingConfig
                {
                    EndpointsConfig = new Dictionary<string, EndpointSettings>()
                    {
                        { containerConfig.HostConfig.NetworkMode, endPointSettings}
                    }
                };
               
                containerConfig.HostConfig.PublishAllPorts = false;
            }
            else if ((containerDesc.ContainerNetworkConfig.NetworkType & ContainerNetworkType.Isolated) == ContainerNetworkType.Isolated && 
                containerDesc.ContainerNetworkConfig.OverlayNetworkResources.Count > 0)
            {
                containerConfig.HostConfig.PublishAllPorts = false;
            }
            else
            {
                containerConfig.HostConfig.PublishAllPorts = true;
            }

            if (sharingNetworkNamespace == false)
            {
                containerConfig.HostConfig.DNS = containerDesc.DnsServers;

                // On Linux, we explicitly pass the ndots value to 1. This is needed to override the default docker behavior.
                // Docker by default, sets the value to 0 for the cases when custom network is used for the container.
                // Value 0 means domains listed in the search list won't be tried.
                // In SF Mesh environment, we rely on DNS suffixes to uniquely identify a service in a given domain.
#if DotNetCoreClrLinux
                containerConfig.HostConfig.DNSOptions = new List<string>();
                containerConfig.HostConfig.DNSOptions.Add("ndots:1");
#endif

                var searchOptions = GetDnsSearchOptions(containerDesc.ApplicationName);
                if (!string.IsNullOrEmpty(searchOptions))
                {
                    containerConfig.HostConfig.DNSSearch.Add(searchOptions);
                }
            }
        }

        private static void PopulateLogConfig(
            ContainerConfig containerConfig,
            ContainerDescription containerDesc)
        {
            if(!string.IsNullOrEmpty(containerDesc.LogConfig.Driver))
            {
                containerConfig.HostConfig.LogConfig = new LogConfig
                {
                    Type = containerDesc.LogConfig.Driver,
                    Config = ParseDriverOptions(containerDesc.LogConfig.DriverOpts)
                };
            }
        }

        private static void PopulateResourceGovernanceSettings(
            ContainerConfig containerConfig,
            ProcessDescription processDesc,
            string dockerVersion)
        {
            var resourceGovPolicy = processDesc.ResourceGovernancePolicy;

            if (resourceGovPolicy.MemoryInMB > 0)
            {
                containerConfig.HostConfig.Memory = resourceGovPolicy.MemoryInMB * MegaBytesToBytesMultiplier;
            }

            if (resourceGovPolicy.MemorySwapInMB > 0)
            {
                var swapMemory = (long)(resourceGovPolicy.MemorySwapInMB) * 1024 * 1024;
                containerConfig.HostConfig.MemorySwap = Math.Min(swapMemory, Int64.MaxValue);
            }

            if (resourceGovPolicy.MemoryReservationInMB > 0)
            {
                containerConfig.HostConfig.MemoryReservation = (resourceGovPolicy.MemoryReservationInMB) * MegaBytesToBytesMultiplier;
            }

            if (resourceGovPolicy.CpuShares > 0)
            {
                containerConfig.HostConfig.CPUShares = resourceGovPolicy.CpuShares;
            }

            if (resourceGovPolicy.DiskQuotaInMB > 0)
            {
                containerConfig.HostConfig.DiskQuota = resourceGovPolicy.DiskQuotaInMB * MegaBytesToBytesMultiplier;
            }
#if DotNetCoreClrLinux
            // Note this is Linux specific and docker on windows will error out if this is set.
            if (resourceGovPolicy.KernelMemoryInMB > 0)
            {
                containerConfig.HostConfig.KernelMemory = resourceGovPolicy.KernelMemoryInMB * MegaBytesToBytesMultiplier;
            }

            // Note this is Linux specific, however, docker will NOT error out if specified.  It is simply ignored.
            if (resourceGovPolicy.ShmSizeInMB > 0)
            {
                containerConfig.HostConfig.ShmSize = resourceGovPolicy.ShmSizeInMB * MegaBytesToBytesMultiplier;
            }
#endif
            if (!string.IsNullOrEmpty(resourceGovPolicy.CpusetCpus))
            {
                containerConfig.HostConfig.CpusetCpus = resourceGovPolicy.CpusetCpus;
            }

            if (resourceGovPolicy.NanoCpus > 0)
            {
                var nanoCpus = resourceGovPolicy.NanoCpus;

#if !DotNetCoreClrLinux
                if (!IsNewDockerVersion(dockerVersion))
                {
                    //
                    // For Windows OS we are going to consider all the active processors from the system
                    // in order to scale the minimum number of NanoCpus to at least 1% of total cpu for
                    // older docker versions
                    //
                    var minNumOfNanoCpus = (ulong)(Environment.ProcessorCount) * DockerNanoCpuMultiplier / 100;
                    nanoCpus = Math.Max(minNumOfNanoCpus, nanoCpus);
                }
#endif
                containerConfig.HostConfig.NanoCPUs = nanoCpus;
            }

            containerConfig.HostConfig.CPUPercent = resourceGovPolicy.CpuPercent;
            containerConfig.HostConfig.IOMaximumIOps = resourceGovPolicy.MaximumIOps;
            containerConfig.HostConfig.IOMaximumBandwidth = resourceGovPolicy.MaximumIOBytesps;
            containerConfig.HostConfig.BlkioWeight = resourceGovPolicy.BlockIOWeight;

            // Adjust conflicting fields for JSON serialization
            if(containerConfig.HostConfig.NanoCPUs > 0)
            {
                containerConfig.HostConfig.CPUShares = 0;
                containerConfig.HostConfig.CPUPercent = 0;
                containerConfig.HostConfig.CpusetCpus = null;
            }
        }

        private static bool IsNewDockerVersion(string dockerVersion)
        {
            if(string.IsNullOrEmpty(dockerVersion))
            {
                return false;
            }

            //docker version are of type MajorVersion.MinorVersion.Edition
            //we want to parse up to the second dot in order to get the version number
            var truncatedVersion = dockerVersion.Substring(0, dockerVersion.LastIndexOf('.'));

            var currentVersion = 0.0;
            if (double.TryParse(truncatedVersion, out currentVersion))
            {
                return (currentVersion >= DockerVersionThreshold);
            }

            return false;
        }

        private static void PopulateDebugParameters(
            ContainerConfig containerConfig,
            ProcessDescription processDesc,
            ContainerDescription containerDesc)
        {
            var debugParams = processDesc.DebugParameters;

            containerConfig.HostConfig.Binds.AddRange(debugParams.ContainerMountedVolumes);
            containerConfig.Env.AddRange(debugParams.ContainerEnvironmentBlock);

            if (debugParams.ContainerEntryPoints != null &&
                debugParams.ContainerEntryPoints.Count > 0)
            {
                containerConfig.Entrypoint = debugParams.ContainerEntryPoints;
            }
            else if (!string.IsNullOrEmpty(containerDesc.EntryPoint))
            {
                containerConfig.Entrypoint =
                    containerDesc.EntryPoint.Split(new string[] { "," }, StringSplitOptions.RemoveEmptyEntries);
            }
        }

        private static string GetContainerNamespace(string containerName)
        {
            if(string.IsNullOrEmpty(containerName))
            {
                return containerName;
            }

            return string.Format("container:{0}", containerName);
        }

        private static IDictionary<string, string> ParseDriverOptions(
            IList<ContainerDriverOptionDescription> driverOpts)
        {
            var logDriverOpts = new Dictionary<string, string>();

            foreach (var driverOpt in driverOpts)
            {
                var decryptedValue = driverOpt.IsEncrypted ? Utility.GetDecryptedValue(driverOpt.Value) : driverOpt.Value;

                logDriverOpts.Add(driverOpt.Name, decryptedValue);
            }

            return logDriverOpts;
        }

        private static string GetDnsSearchOptions(string appName)
        {
            if (string.IsNullOrEmpty(appName))
            {
                return string.Empty;
            }

            var searchOptions = appName;
            var rootName = string.Format("{0}/", RootNamingUriString);

            if (appName.StartsWith(rootName, StringComparison.OrdinalIgnoreCase))
            {
                searchOptions = searchOptions.Substring(rootName.Length);

                var tokens = searchOptions.Split(new string[] { "/" }, StringSplitOptions.RemoveEmptyEntries);

                if(tokens.Length == 1)
                {
                    return searchOptions;
                }

                foreach (var token in tokens)
                {
                    searchOptions = string.Format("{0}.{1}", searchOptions, token);
                }
            }

            return searchOptions;
        }
    }
}

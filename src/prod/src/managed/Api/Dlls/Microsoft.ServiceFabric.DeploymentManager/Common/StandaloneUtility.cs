// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.IO.Pipes;
    using System.Linq;
    using System.Net;
    using System.Net.Http;
    using System.Net.Sockets;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager.Model;
    using Microsoft.Win32;
    using Newtonsoft.Json;
    using DMConstants = Microsoft.ServiceFabric.DeploymentManager.Constants;

    public class StandaloneUtility
    {      
        private static HashSet<char> invalidFileNameChars = new HashSet<char>(Path.GetInvalidFileNameChars().ToList<char>());

        public static FileInfo GetClusterManifestFromJsonConfig(string clusterConfigPath, string clusterName, string version)
        {
            var clusterResource = DeploymentManagerInternal.GetClusterResource(clusterConfigPath, System.Guid.NewGuid().ToString());
            return StandaloneUtility.ClusterManifestToFile(clusterResource.Current.ExternalState.ClusterManifest, clusterName, version);
        }

        public static FileInfo ClusterManifestToFile(ClusterManifestType cm, string clusterName, string version)
        {
            string clusterManifestName = string.Format(Microsoft.ServiceFabric.DeploymentManager.Constants.ClusterManifestNameFormat, clusterName, version);
            string clusterManifestPath = Path.Combine(Path.GetTempPath(), clusterManifestName);
            SFDeployerTrace.WriteNoise(StringResources.Info_SFWritingClusterManifest, clusterManifestPath);
            XMLHelper.WriteXmlExclusive<ClusterManifestType>(clusterManifestPath, cm);
            return new FileInfo(clusterManifestPath);
        }

        public static JsonSerializerSettings GetStandAloneClusterSerializerSettings()
        {
            return new JsonSerializerSettings()
            {
                Formatting = Formatting.Indented,
                ReferenceLoopHandling = ReferenceLoopHandling.Serialize,
                NullValueHandling = NullValueHandling.Ignore,
                TypeNameHandling = TypeNameHandling.Objects,
                PreserveReferencesHandling = PreserveReferencesHandling.None,
                TypeNameAssemblyFormat = System.Runtime.Serialization.Formatters.FormatterAssemblyStyle.Full
            };
        }

        public static JsonSerializerSettings GetStandAloneClusterDeserializerSettings()
        {
            JsonSerializerSettings settings = new JsonSerializerSettings
            {
                ReferenceLoopHandling = ReferenceLoopHandling.Serialize,
                NullValueHandling = NullValueHandling.Ignore,
                TypeNameHandling = TypeNameHandling.None
            };
            settings.Converters.Add(new StandAloneClusterJsonDeserializer());
            return settings;
        }

        public static void OpenRemoteRegistryNamedPipe(string machineName, TimeSpan timeout)
        {
            SFDeployerTrace.WriteNoise(StringResources.Info_SFOpenRegNamedPipe, machineName);
            using (var namedPipeClientStream = new NamedPipeClientStream(machineName, DMConstants.RemoteRegistryNamedPipeName, System.IO.Pipes.PipeDirection.In))
            {
                int timeoutInMs = (int)Math.Max((double)DMConstants.NamedPipeConnectTimeoutInMs, timeout.TotalMilliseconds);
                try
                {
                    namedPipeClientStream.Connect(timeoutInMs);
                    SFDeployerTrace.WriteInfo(StringResources.Info_SFOpenRegNamedPipeSuccess, machineName);
                }
                catch (TimeoutException ex)
                {
                    SFDeployerTrace.WriteWarning(StringResources.Error_SFOpenRegNamedPipeTimeout, machineName, ex.Message);
                }
                catch (InvalidOperationException ex)
                {
                    SFDeployerTrace.WriteWarning(StringResources.Error_SFOpenRegNamedPipeAlreadyConnected, machineName, ex.Message);
                }
                catch (IOException ex)
                {
                    SFDeployerTrace.WriteWarning(StringResources.Error_SFOpenRegNamedPipeAnotherClientAlreadyConnected, machineName, ex.Message);
                }
            }
        }

        internal static List<string> GetMachineNamesFromClusterManifest(string clusterManifestPath)
        {
            List<string> machineAddresses = new List<string>();
            ClusterManifestTypeInfrastructureWindowsServer serverCm = GetInfrastructureWindowsServerFromClusterManifest(clusterManifestPath);
            foreach (FabricNodeType n in serverCm.NodeList)
            {
                machineAddresses.Add(n.IPAddressOrFQDN);
            }

            machineAddresses = machineAddresses.Distinct().ToList();
            if (machineAddresses.Count == 0)
            {
                throw new InvalidOperationException("Could not find machines in cluster manifest");
            }

            return machineAddresses;
        }

        internal static ClusterManifestTypeInfrastructureWindowsServer GetInfrastructureWindowsServerFromClusterManifest(string clusterManifestPath)
        {
            ClusterManifestType cm = XMLHelper.ReadClusterManifest(clusterManifestPath);
            ClusterManifestTypeInfrastructureWindowsServer infrastructure = cm.Infrastructure.Item as ClusterManifestTypeInfrastructureWindowsServer;
            if (infrastructure == null)
            {
                throw new InvalidOperationException("The ClusterConfig file is invalid. Only Server deployments are supported by this api");
            }

            return infrastructure;
        }

        internal static string GetFabricDataRootFromClusterManifest(ClusterManifestType clusterManifest)
        {
            var setupSection = clusterManifest.FabricSettings.FirstOrDefault(section => section.Name == DMConstants.SetupString);
            if (setupSection == null)
            {
                throw new InvalidOperationException("Cluster manifest did not have a Setup section.");
            }

            var fabricDataRootParam = setupSection.Parameter.FirstOrDefault(p => p.Name == DMConstants.FabricDataRootString);
            if (fabricDataRootParam == null)
            {
                throw new InvalidOperationException("Cluster manifest FabricDataRoot parameter did not exist in Setup.");
            }

            if (string.IsNullOrWhiteSpace(fabricDataRootParam.Value))
            {
                throw new InvalidOperationException("Cluster manifest FabricDataRoot was empty.");
            }

            return fabricDataRootParam.Value;
        }

        internal static string GetFabricLogRootFromClusterManifest(ClusterManifestType clusterManifest)
        {
            var setupSection = clusterManifest.FabricSettings.FirstOrDefault(section => section.Name == DMConstants.SetupString);
            if (setupSection == null)
            {
                throw new InvalidOperationException("Cluster manifest did not have a Setup section.");
            }

            var fabricLogRootParam = setupSection.Parameter.FirstOrDefault(p => p.Name == DMConstants.FabricLogRootString);
            if (fabricLogRootParam == null || string.IsNullOrWhiteSpace(fabricLogRootParam.Value))
            {
                return null;
            }

            return fabricLogRootParam.Value;
        }

        internal static string FindFabricResourceFile(string filename)
        {
            string filePath = filename;
            string fabricCodePath = null;
            string assemblyCommonPath = null;
            if (File.Exists(filePath))
            {
                return filePath;
            }
            else
            {
                fabricCodePath = System.Fabric.FabricDeployer.Utility.GetFabricCodePath();
                if (fabricCodePath != null)
                {
                    filePath = Path.Combine(fabricCodePath, filename);
                    if (File.Exists(filePath))
                    {
                        return filePath;
                    }
                }

                assemblyCommonPath = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
                filePath = Path.Combine(assemblyCommonPath, filename);
                if (File.Exists(filePath))
                {
                    return filePath;
                }
            }

            throw new FileNotFoundException(string.Format(
                                                StringResources.Error_CannotFindFabricResource,
                                                string.Format("{0} in {1} {2}", filename, fabricCodePath ?? string.Empty, assemblyCommonPath)));
        }

        internal static bool IsUriReachable(
            Uri uri,
            string requestMethod = DMConstants.HttpMethodHead,
            int operationTimeoutInMs = DMConstants.UriReachableTimeoutInMs,
            int requestTimeoutInMs = DMConstants.UriRequestTimeoutInMs,
            int retryIntervalInMs = DMConstants.UriReachableRetryIntervalInMs)
        {
            return IsUriReachableAsync(uri, requestMethod, operationTimeoutInMs, requestTimeoutInMs, retryIntervalInMs).Result;
        }

        internal static async Task<bool> IsUriReachableAsync(
            Uri uri,
            string requestMethod = DMConstants.HttpMethodHead,
            int operationTimeoutInMs = DMConstants.UriReachableTimeoutInMs,
            int requestTimeoutInMs = DMConstants.UriRequestTimeoutInMs,
            int retryIntervalInMs = DMConstants.UriReachableRetryIntervalInMs)
        {
            ReleaseAssert.AssertIf(uri == null, "uri cannot be null for IsUriReachableAsync.");
            if (uri.IsFile)
            {
                FileInfo fi = new FileInfo(uri.LocalPath);
                return fi.Exists;
            }
            else
            {
                if (string.IsNullOrWhiteSpace(uri.Host))
                {
                    return false;
                }

                var timeout = new System.Fabric.Common.TimeoutHelper(TimeSpan.FromMilliseconds(operationTimeoutInMs));
                while (!System.Fabric.Common.TimeoutHelper.HasExpired(timeout))
                {
                    WebRequest request = WebRequest.Create(uri);
#if !DotNetCoreClrLinux
                    request.Timeout = requestTimeoutInMs;
#endif
                    request.Method = requestMethod;
                    try
                    {
                        using (WebResponse response = await request.GetResponseAsync().ConfigureAwait(false))
                        {
                            if (response is HttpWebResponse)
                            {
                                if (((HttpWebResponse)response).StatusCode == HttpStatusCode.OK)
                                {
                                    return true;
                                }

                                return false;
                            }
                            else
                            {
                                return response.ContentLength > 0;
                            }
                        }
                    }
                    catch (WebException ex)
                    {
                        SFDeployerTrace.WriteNoise(StringResources.Error_SFUriUnreachable_Formatted, uri, ex.Message);
                    }

                    System.Threading.Thread.Sleep(retryIntervalInMs);
                }
            }

            return false;
        }

        internal static bool CheckRequiredPorts(MachineHealthContainer machineHealthContainer)
        {
            SFDeployerTrace.WriteNoise(StringResources.Info_BPARequiredPorts);
            int[] requiredPorts = { DMConstants.SmbFileSharePort };
            bool result = true;
            foreach (string machineFqdn in machineHealthContainer.GetHealthyMachineNames())
            {
                if (Uri.CheckHostName(machineFqdn) == UriHostNameType.Unknown)
                {
                    result = false;
                    SFDeployerTrace.WriteError(StringResources.Error_BPAMachineNotPingable, machineFqdn);
                }
            }

            if (!result)
            {
                return false;
            }

            foreach (int port in requiredPorts)
            {
                Parallel.ForEach(
                    machineHealthContainer.GetHealthyMachineNames(),
                    (string machineName) =>
                    {
                        result = true;
                        IPAddress address = null;
                        bool isParseableAsIPAddress = IPAddress.TryParse(machineName, out address);

                        if (isParseableAsIPAddress)
                        {
                            if (address.AddressFamily == AddressFamily.InterNetwork)
                            {
                                SFDeployerTrace.WriteNoise("Using IPv4 address for {0}", machineName);
                            }
                            else if (address.AddressFamily == AddressFamily.InterNetworkV6)
                            {
                                SFDeployerTrace.WriteNoise("Using IPv6 address for {0}", machineName);
                            }
                            else
                            {
                                SFDeployerTrace.WriteError(StringResources.Error_BPAPortConnectFailed, machineName, port, "Expected IPv4 or IPv6 address");
                                result = false;
                                return;
                            }

                            try
                            {
                                using (TcpClient client = new TcpClient(address.AddressFamily))
                                {
                                    client.Connect(address, port);
                                }
                            }
                            catch (SocketException ex)
                            {
                                SFDeployerTrace.WriteError(StringResources.Error_BPAPortConnectFailed, machineName, port, ex.Message);
                                result = false;
                            }
                        }
                        else
                        {
                            try
                            {
                                using (new TcpClient(machineName, port))
                                {
                                    // Intentionally empty.  The constructor already establishes a connection                                    
                                }
                            }
                            catch (SocketException ex)
                            {
                                SFDeployerTrace.WriteError(StringResources.Error_BPAPortConnectFailed, machineName, port, ex.Message);
                                result = false;
                            }
                        }

                        if (!result)
                        {
                            machineHealthContainer.MarkMachineAsUnhealthy(machineName);
                        }
                    });
            }

            return machineHealthContainer.EnoughHealthyMachines();
        }

        internal static string GetGoalStateUri()
        {
            string goalStateUriStr = null;

            /* Test code relies on the settings present in ClusterSettings.json for deployment of a specific version.
               We need this check for the test code because certain APIs will be invoked before the cluster is even up. */
            string clusterSettingsFilepath = StandaloneUtility.FindFabricResourceFile(DMConstants.ClusterSettingsFileName);
            if (!string.IsNullOrEmpty(clusterSettingsFilepath))
            {
                StandAloneClusterManifestSettings standAloneClusterManifestSettings = JsonConvert.DeserializeObject<StandAloneClusterManifestSettings>(File.ReadAllText(clusterSettingsFilepath));
                if (standAloneClusterManifestSettings.CommonClusterSettings != null && standAloneClusterManifestSettings.CommonClusterSettings.Section != null)
                {
                    SettingsTypeSection settings = standAloneClusterManifestSettings.CommonClusterSettings.Section.ToList().SingleOrDefault(
                           section => section.Name == DMConstants.UpgradeOrchestrationServiceConfigSectionName);
                    if (settings != null)
                    {
                        SettingsTypeSectionParameter goalStatePathParam = settings.Parameter.ToList().SingleOrDefault(param => param.Name == DMConstants.GoalStateFileUrlName);
                        if (goalStatePathParam != null)
                        {
                            goalStateUriStr = goalStatePathParam.Value;
                        }
                    }
                }
            }

            // Check the native config store before using default location. The GoalStateFileUrl may have been overridden by the user.
            if (string.IsNullOrEmpty(goalStateUriStr))
            {
                NativeConfigStore configStore = NativeConfigStore.FabricGetConfigStore();
                goalStateUriStr = configStore.ReadUnencryptedString(DMConstants.UpgradeOrchestrationServiceConfigSectionName, DMConstants.GoalStateFileUrlName);
            }

            if (string.IsNullOrEmpty(goalStateUriStr))
            {
                goalStateUriStr = DMConstants.DefaultGoalStateFileUrl;
            }

            return goalStateUriStr;
        }

        internal static bool IsUpgradeCompleted(FabricUpgradeState upgradeState)
        {
            return upgradeState == FabricUpgradeState.RollingBackCompleted ||
                   upgradeState == FabricUpgradeState.RollingForwardCompleted;
        }

        internal static async Task<string> GetContentsFromUriAsyncWithRetry(Uri uri, TimeSpan operationTimeout, CancellationToken cancellationToken)
        {
            return await GetContentsFromUriAsyncWithRetry(uri, DMConstants.DefaultRetryInterval, operationTimeout, cancellationToken);
        }

        internal static async Task<string> GetContentsFromUriAsyncWithRetry(Uri uri, TimeSpan retryInterval, TimeSpan operationTimeout, CancellationToken cancellationToken)
        {
            string downloadedContent = null;
            var timeoutHelper = new System.Fabric.Common.TimeoutHelper(operationTimeout);
            while (!System.Fabric.Common.TimeoutHelper.HasExpired(timeoutHelper))
            {
                cancellationToken.ThrowIfCancellationRequested();

                try
                {
                    if (uri.IsFile)
                    {
                        downloadedContent = File.ReadAllText(uri.LocalPath);
                    }
                    else
                    {
                        using (var wc = new WebClient())
                        {
                            downloadedContent = await wc.DownloadStringTaskAsync(uri).ConfigureAwait(false);
                        }
                    }

                    return downloadedContent;
                }
                catch (Exception e)
                {
                   SFDeployerTrace.WriteWarning(StringResources.Error_SFUriNotDownloaded, uri, e.ToString());                   
                }   
            
                await Task.Delay(retryInterval, cancellationToken).ConfigureAwait(false);
            }

            SFDeployerTrace.WriteError(StringResources.Error_SFTimedOut, operationTimeout);
            throw new FabricValidationException(string.Format(StringResources.Error_SFTimedOut, operationTimeout), FabricErrorCode.OperationCanceled);
        }

        internal static Task<NodeList> GetNodesFromFMAsync(FabricClient fabricClient, CancellationToken cancellationToken)
        {
            return FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                         () =>
                                         fabricClient.QueryManager.GetNodeListAsync(
                                                 null,
                                                 DMConstants.FabricQueryNodeListTimeout,
                                                 cancellationToken),
                                             DMConstants.FabricQueryNodeListRetryTimeout);
        }

        internal static bool CheckFabricRunningAsGMSA(IUserConfig currentConfig)
        {
            return currentConfig.Security != null &&
                currentConfig.Security.WindowsIdentities != null &&
                currentConfig.Security.WindowsIdentities.ClustergMSAIdentity != null;
        }

        internal static bool IsMsiInstalled(MachineHealthContainer machineHealthContainer)
        {
            SFDeployerTrace.WriteNoise(StringResources.Info_BPAConflictingInstallations);
            var fabricProductNames = new string[] { "Windows Fabric", "Microsoft Service Fabric", "Microsoft Azure Service Fabric" };

            bool localMachineFailed = false;
            List<string> machineNamesTemp = GetMachineNamesIncludingClient(machineHealthContainer.GetHealthyMachineNames());
            Parallel.ForEach(
                machineNamesTemp,
                (string machineName) =>
                {
                    bool result = false;
                    using (RegistryKey uninstallKeys = GetHklm(machineName).OpenSubKey(DMConstants.UninstallProgramsBaseKey))
                    {
                        if (uninstallKeys != null)
                        {
                            Parallel.ForEach(
                                uninstallKeys.GetSubKeyNames(),
                                (string guid) =>
                                {
                                    using (RegistryKey uninstallKey = uninstallKeys.OpenSubKey(guid))
                                    {
                                        string uninstallString = (string)uninstallKey.GetValue("UninstallString", null);
                                        if (uninstallString != null)
                                        {
                                            string displayNameValue = (string)uninstallKey.GetValue("DisplayName", null);
                                            if (displayNameValue != null)
                                            {
                                                displayNameValue = displayNameValue.Trim();
                                                if (uninstallString.IndexOf("msiexec.exe", StringComparison.OrdinalIgnoreCase) >= 0)
                                                {
                                                    foreach (string productName in fabricProductNames)
                                                    {
                                                        if (string.Equals(displayNameValue, productName, StringComparison.OrdinalIgnoreCase))
                                                        {
                                                            SFDeployerTrace.WriteError(StringResources.Error_BPAMsiIsInstalled, displayNameValue, machineName);
                                                            result = true;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                });
                        }
                    }

                    if (result)
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
                return true;
            }

            return !machineHealthContainer.EnoughHealthyMachines();
        }

        // Single machine/node override, for AddNode
        internal static bool CheckForCleanInstall(string machineName, string nodeName, string fabricDataRoot)
        {
            SFDeployerTrace.WriteNoise(StringResources.Info_BPANoFabric);

            bool result = true;
            if (IsFabricInstalled(machineName))
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPAPreviousFabricExists, machineName);
                result = false;
            }

            string nodeDirectory;
            if (DataRootNodeExists(machineName, nodeName, fabricDataRoot, out nodeDirectory))
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPADataRootNodeExists, nodeName, machineName, nodeDirectory);
                result = false;
            }

            return result;
        }

        internal static bool IsFabricInstalled(string machineName)
        {
            bool result = true;
            try
            {
                if (NodeConfiguration.GetNodeConfiguration(machineName) == null
                    && !FabricDeployerServiceController.ServiceExists(System.Fabric.FabricDeployer.Constants.FabricHostServiceName, machineName))
                {
                    SFDeployerTrace.WriteNoise(StringResources.Info_BPAFabricNotInstalledOnMachine, machineName);
                    result = false;
                }
            }
            catch (System.Fabric.FabricException ex)
            {
                SFDeployerTrace.WriteNoise(StringResources.Info_BPAFabricInstalledHitFabricException_Formatted, machineName, ex);
                result = false;
            }
            catch (System.InvalidOperationException ex /*Internal exception: System.Xml.Schema.InvalidOperationException*/)
            {
                SFDeployerTrace.WriteNoise(StringResources.Info_BPAFabricInstalledHitIOX_Formatted, machineName, ex);
                result = false;
            }

            SFDeployerTrace.WriteNoise(StringResources.Info_BPAFabricInstalledOnMachine, machineName, result);

            return result;
        }

        internal static bool DataRootNodeExists(string machineName, string nodeName, string fabricDataRoot, out string nodeDirectory)
        {
            nodeDirectory = Path.Combine(
                Helpers.GetRemotePathIfNotLocalMachine(fabricDataRoot, machineName),
                nodeName);

            return new DirectoryInfo(nodeDirectory).Exists;
        }

        internal static List<string> GetMachineNamesIncludingClient(IEnumerable<string> machineNames)
        {
            List<string> machineNamesTemp = new List<string>(machineNames);

            bool isLocalhostPresent = false;
            foreach (string machine in machineNamesTemp)
            {
                if (Helpers.IsLocalIpAddress(machine))
                {
                    isLocalhostPresent = true;
                    break;
                }
            }

            if (!isLocalhostPresent)
            {
                machineNamesTemp.Add("localhost");
            }

            return machineNamesTemp;
        }

        internal static string GetNextClusterManifestVersionHelper(ICluster clusterResource, ITraceLogger tracerLogger, string upgradeStateClassName)
        {
            int version = ++clusterResource.ClusterManifestVersion;
            tracerLogger.WriteInfo(new TraceType(upgradeStateClassName), "GetNextClusterManifestVersion: new version: {0}", version);
            return version.ToString(CultureInfo.InvariantCulture);
        }

        internal static bool IsValidFileName(string value)
        {
            foreach (char c in value)
            {
                if (invalidFileNameChars.Contains(c))
                {
                    return false;
                }
            }

            return true;
        }

        internal static bool IsValidIPAddressOrFQDN(string ip)
        {
            IPAddress address;
            if (IPAddress.TryParse(ip, out address))
            {
                return true;
            }

            IdnMapping maping = new IdnMapping();
            string asciiHostName = maping.GetAscii(ip);
            return System.Uri.CheckHostName(asciiHostName) != System.UriHostNameType.Unknown;
        }

        internal static bool IsMachineDomainController(string machineName)
        {
            const uint SV_TYPE_SERVER = 0x00000002;
            const uint SV_TYPE_DOMAIN_CTRL = 0x00000008;
            const uint SV_TYPE_DOMAIN_BAKCTRL = 0x00000010;
            IntPtr ptrSI = IntPtr.Zero;
            try
            {
                int ret = NetServerGetInfo(machineName, 101, out ptrSI);
                if (ret != 0)
                {
                    throw new System.ComponentModel.Win32Exception(ret);
                }

                SERVER_INFO_101 serverInfo = (SERVER_INFO_101)Marshal.PtrToStructure(ptrSI, typeof(SERVER_INFO_101));

                return (serverInfo.Type & SV_TYPE_SERVER) == SV_TYPE_SERVER &&
                       (serverInfo.Type & (SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL)) != 0;
            }
            finally
            {
                if (ptrSI != IntPtr.Zero)
                {
                    NetApiBufferFree(ptrSI);
                }
            }
        }

        internal static async Task<T> ExecuteActionWithTimeoutAsync<T>(Func<Task<T>> function, TimeSpan timeout)
        {
            Task<T> actionTask = function();
            if (timeout != TimeSpan.MaxValue && !actionTask.Wait(timeout))
            {
                SFDeployerTrace.WriteError(StringResources.Error_CreateClusterTimeout);
                throw new System.TimeoutException(StringResources.Error_CreateClusterTimeout);
            }

            return await actionTask.ConfigureAwait(false);
        }

        internal static async Task ExecuteActionWithTimeoutAsync(Func<Task> function, TimeSpan timeout)
        {
            Task actionTask = function();
            if (timeout != TimeSpan.MaxValue && !actionTask.Wait(timeout))
            {
                SFDeployerTrace.WriteError(StringResources.Error_CreateClusterTimeout);
                throw new System.TimeoutException(StringResources.Error_CreateClusterTimeout);
            }

            await actionTask.ConfigureAwait(false);
        }
        
        internal static int IOTDevicesCount(IEnumerable<string> machineNames)
        {
            int numOfIOTCores = 0;
            object countLock = new object();

            Parallel.ForEach(
                machineNames,
                (string machineName) =>
                {
                    if (StandaloneUtility.IsIOTCore(machineName))
                    {
                        lock (countLock)
                        {
                            numOfIOTCores++;
                        }
                    } 
                });

            return numOfIOTCores;
        }

        internal static bool IsIOTCore(string machineName)
        {
            try
            {
                using (RegistryKey regKey = GetHklm(machineName).OpenSubKey(DMConstants.IOTCurrentVersionRegKeyPath))
                {
                    if (regKey != null)
                    {
                        return string.Equals((string)regKey.GetValue(DMConstants.IOTEditionIDKeyName, string.Empty), DMConstants.IOTUAP);
                    }
                }
            }
            catch (IOException)
            {
                SFDeployerTrace.WriteInfo(StringResources.Info_IOTCheckFailedToOpenRegistryKey, machineName);
            }

            return false;
        }

        internal static List<SettingsOverridesTypeSection> RemoveFabricSettingsSectionForIOTCore(ClusterManifestType clustermanifest)
        {
            var targetFabricSettings = clustermanifest.FabricSettings.ToList();

            List<string> blackListIOT = new List<string>   
            {
                StringConstants.SectionName.UpgradeOrchestrationService,                
                StringConstants.SectionName.FaultAnalysisService,
                StringConstants.SectionName.DnsService,
            };

            foreach (var sectionName in blackListIOT)
            {
                var section = targetFabricSettings.FirstOrDefault(x => x.Name == sectionName);
                targetFabricSettings.Remove(section);
            }

            return targetFabricSettings;
        }

        internal static bool EnoughAvailableSpaceOnDrive(string rootDrive)
        {
            if (StandaloneUtility.DriveFreeBytes(rootDrive, out ulong availableSpace))
            {
                return availableSpace >= DMConstants.DriveMinAvailableSpace;
            }
            else
            {
                SFDeployerTrace.WriteError(StringResources.Error_CouldNotQuerySpaceOnDrive, rootDrive);
                return false;
            }
        }

        internal static async Task<RuntimePackageDetails> ValidateCodeVersionAsync(string targetCodeVersion)
        {
            RuntimePackageDetails targetCodePackage = null;
            if (targetCodeVersion == DMConstants.AutoupgradeCodeVersion)
            {
                try
                {
                    targetCodePackage = DeploymentManager.GetDownloadableRuntimeVersions(DownloadableRuntimeVersionReturnSet.Latest).First<RuntimePackageDetails>();
                }
                catch (Exception ex)
                {
                    SFDeployerTrace.WriteError(
                    "Failed trying to get latest downloadable versions for auto upgrade version. Exception: {0}",
                    ex);
                }
            }
            else
            {
                var currentCodeVersion = await StandaloneUtility.GetCurrentCodeVersion().ConfigureAwait(false);
                var upgradeableVersions = new List<RuntimePackageDetails>();
                try
                {
                    upgradeableVersions = DeploymentManager.GetUpgradeableRuntimeVersions(currentCodeVersion);
                }
                catch (Exception ex)
                {
                    SFDeployerTrace.WriteError(
                        "Failed trying to load upgradeableVersions for the given version. Exception: {0}",
                        ex);

                    return null;
                }

                targetCodePackage = upgradeableVersions.SingleOrDefault(version => version.Version.Equals(targetCodeVersion));
                if (targetCodePackage == null)
                {
                    // Todo: If no upgradeable version is found, checks for all supported versions. 
                    // Need to change this to check for only supported downgradable versions once the goal state file is modified.
                    var allSupportedVersions = DeploymentManager.GetDownloadableRuntimeVersions();
                    if (allSupportedVersions != null && allSupportedVersions.Any(version => version.Version.Equals(targetCodeVersion)))
                    {
                        targetCodePackage = allSupportedVersions.SingleOrDefault(version => version.Version.Equals(targetCodeVersion));
                    }
                }
            }

            return targetCodePackage;
        }

        internal static async Task<string> DownloadCodeVersionAsync(string codeVersion)
        {
            var targetPackage = await StandaloneUtility.ValidateCodeVersionAsync(codeVersion);
            if (targetPackage == null)
            {
                throw new FabricException(string.Format("Failed to validate code version {0}", codeVersion));
            }

            string packageDropDir = Helpers.GetNewTempPath();
            string packageLocalDownloadPath = Path.Combine(packageDropDir, string.Format(DMConstants.SFPackageDropNameFormat, targetPackage.Version));

            bool result = await StandaloneUtility.DownloadPackageAsync(targetPackage.Version, targetPackage.TargetPackageLocation, packageLocalDownloadPath).ConfigureAwait(false);
            if (!result)
            {
                throw new FabricException(string.Format("Failed to download package from {0}", targetPackage.TargetPackageLocation));
            }

            return packageLocalDownloadPath;
        }

        internal static async Task<bool> DownloadPackageAsync(string targetPackageVersion, string packageDownloadUriStr, string packageLocalDownloadFilePath)
        {
            Uri downloadUri = null;
            bool result = false;
            string packageDropDir = Path.GetDirectoryName(packageLocalDownloadFilePath);
            if (string.IsNullOrEmpty(packageDownloadUriStr))
            {
                SFDeployerTrace.WriteError("Download URI was empty for {0}", targetPackageVersion);
                return false;
            }

            if (!Uri.TryCreate(packageDownloadUriStr, UriKind.Absolute, out downloadUri))
            {
                SFDeployerTrace.WriteError("Cannot parse uri {0}", packageDownloadUriStr);
                return false;
            }

            if (!(await StandaloneUtility.IsUriReachableAsync(downloadUri).ConfigureAwait(false)))
            {
                SFDeployerTrace.WriteError("Cannot reach download uri for CAB: {0}", downloadUri.AbsoluteUri);
                return false;
            }

            if (!Directory.Exists(packageDropDir))
            {
                Directory.CreateDirectory(packageDropDir);
            }

            try
            {
                SFDeployerTrace.WriteInfo("Package {0} downloading to: {1}", targetPackageVersion, packageLocalDownloadFilePath);
                await StandaloneUtility.DownloadFileAsync(downloadUri, packageLocalDownloadFilePath).ConfigureAwait(false);
            }
            catch (Exception ex)
            {
                SFDeployerTrace.WriteError("Package {0} download threw: {1}", targetPackageVersion, ex.ToString());
                result = false;
            }

            if (File.Exists(packageLocalDownloadFilePath))
            {
                SFDeployerTrace.WriteInfo("Package {0} downloaded: {1}", targetPackageVersion, packageLocalDownloadFilePath);
                result = true;
            }
            else
            {
                SFDeployerTrace.WriteError("Package {0} failed to download: {1}", targetPackageVersion, packageLocalDownloadFilePath);
                result = false;
            }

            return result;
        }

        internal static async Task<string> GetCurrentCodeVersion()
        {
            FabricUpgradeProgress fabricUpgradeProgress = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                                                     () =>
                                                                     new FabricClient().ClusterManager.GetFabricUpgradeProgressAsync()).ConfigureAwait(false);
            return fabricUpgradeProgress.TargetCodeVersion;
        }

        internal static RegistryKey GetHklm(string machineName)
        {
            RegistryKey hklm;
            if (string.IsNullOrWhiteSpace(machineName)
                || string.Equals(machineName.Trim(), DMConstants.LocalHostMachineName, StringComparison.OrdinalIgnoreCase))
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

        private static async Task DownloadFileAsync(Uri fileUri, string destinationPath)
        {
            if (fileUri.IsFile)
            {
                using (FileStream sourceStream = File.Open(fileUri.LocalPath, FileMode.Open, FileAccess.Read, FileShare.Read))
                {
                    using (FileStream destStream = File.Create(destinationPath))
                    {
                        sourceStream.CopyTo(destStream);
                    }
                }
            }
            else
            {
                var webClient = new HttpClient()
                {
                    Timeout = TimeSpan.FromHours(ServiceFabric.DeploymentManager.Constants.FabricDownloadFileTimeoutInHours)
                };

                using (Stream srcStream = await webClient.GetStreamAsync(fileUri))
                using (Stream dstStream = File.Open(destinationPath, FileMode.Create, FileAccess.ReadWrite, FileShare.None))
                {
                    await srcStream.CopyToAsync(dstStream);
                }
            }
        }

        private static bool DriveFreeBytes(string folderName, out ulong freeBytesCount)
        {
            if (!folderName.EndsWith("\\"))
            {
                folderName += '\\';
            }

            freeBytesCount = 0;
            if (GetDiskFreeSpaceEx(folderName, out ulong FreeBytesAvailable, out ulong TotalNumberOfBytes, out ulong TotalNumberOfFreeBytes))
            {
                freeBytesCount = FreeBytesAvailable;
                return true;
            }
            else
            {
                return false;
            }
        }

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool GetDiskFreeSpaceEx(
            string directoryName,
            out ulong freeBytesAvailable,
            out ulong totalNumberOfBytes,
            out ulong totalNumberOfFreeBytes);

        [DllImport("Netapi32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        private static extern int NetServerGetInfo(string serverName, int level, out IntPtr ptrServerInfo);

        [DllImport("Netapi32.dll", SetLastError = true)]
        private static extern int NetApiBufferFree(IntPtr buffer);

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct SERVER_INFO_101
        {
            public int PlatformId;
            [MarshalAs(UnmanagedType.LPTStr)]
            public string Name;
            public int VersionMajor;
            public int VersionMinor;
            public int Type;
            [MarshalAs(UnmanagedType.LPTStr)]
            public string Comment;
        }
    }
}
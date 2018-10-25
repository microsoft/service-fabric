// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Health;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.WRP.Model;
    using System.IO;
    using System.Linq;
    using System.Net.Http;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Xml;
    using System.Xml.Serialization;

    internal class CommandParameterGenerator
    {
        private static readonly TraceType TraceType = new TraceType("CommandParameterGenerator");
        
        private IFabricClientWrapper fabricClientWrapper;
        private XmlSerializer clusterManifestSerializer;

        private int continousNullClusterHealthCount;

        public CommandParameterGenerator(IFabricClientWrapper fabricClientWrapper)
        {
            this.fabricClientWrapper = fabricClientWrapper;
            this.clusterManifestSerializer = new XmlSerializer(typeof(ClusterManifestType));
            this.continousNullClusterHealthCount = 0;
        }

        public async Task<ClusterUpgradeCommandParameter> GetCommandParameterAsync(
            FabricUpgradeProgress currentUpgradeProgress,
            ClusterHealth currentClusterHealth,
            PaasClusterUpgradeDescription response,
            CancellationToken token)
        {
            if (response == null ||
                (response.TargetClusterManifestBase64 == null && response.TargetMsiUri == null))
            {
                Trace.WriteInfo(TraceType, "GetCommandParameterAsync: Null response");
                return null;
            }

            if(currentClusterHealth == null)
            {
                Trace.WriteWarning(TraceType, "currentClusterHealth is null. ContinousNullClusterHealthCount = {0}", this.continousNullClusterHealthCount);
                if (++continousNullClusterHealthCount < 5)
                {
                    Trace.WriteWarning(TraceType, "Returning null since upgrade will not be initiated");
                    return null;
                }
                
                Trace.WriteWarning(TraceType, "Upgrading even though currentClusterHealth is null.");
                
            }
            else
            {
                this.continousNullClusterHealthCount = 0;
            }

            Trace.WriteInfo(TraceType, "GetCommandParameterAsync: Processing WRP response");
            ClusterManifestType targetClusterManifest = FromBase64ClusterManifest(response.TargetClusterManifestBase64);

            Trace.WriteInfo(TraceType, "GetCommandParameterAsync: response.TargetMsiVersion = {0}, currentUpgradeProgress.TargetCodeVersion = {1}, targetClusterManifest.Version = {2}, currentUpgradeProgress.UpgradeState = {3}",
                response.TargetMsiVersion,
                currentUpgradeProgress != null ? currentUpgradeProgress.TargetCodeVersion : "NA",
                targetClusterManifest != null ? targetClusterManifest.Version : "NA",
                currentUpgradeProgress != null ? currentUpgradeProgress.UpgradeState.ToString() : "NA");

            string currentCodeVersion = string.Empty;
            string currentConfigVersion = string.Empty;
            bool isUpgradeStateFailed = false;
            if (currentUpgradeProgress != null)
            {
                currentCodeVersion = currentUpgradeProgress.TargetCodeVersion;
                currentConfigVersion = currentUpgradeProgress.TargetConfigVersion;
                isUpgradeStateFailed = currentUpgradeProgress.UpgradeState == FabricUpgradeState.Failed;
            }

            if (response.TargetMsiVersion == currentCodeVersion &&
                (targetClusterManifest != null && targetClusterManifest.Version == currentConfigVersion) &&
                !isUpgradeStateFailed)
            {
                return null;
            }

            string clusterConfigFilePath = null;
            string clusterMsiPath = null;
            string clusterConfigVersion = null;
            var info = Directory.CreateDirectory(Path.Combine(Path.GetTempPath(), Path.GetRandomFileName()));
            var tempPath = info.FullName;
            if (isUpgradeStateFailed || (targetClusterManifest != null && targetClusterManifest.Version != currentConfigVersion))
            {
                clusterConfigFilePath = Path.Combine(tempPath, "ClusterManifest.xml");
                using (var fs = new FileStream(clusterConfigFilePath, FileMode.Create))
                {
                    using (TextWriter writer = new StreamWriter(fs, new UTF8Encoding()))
                    {
                        // Serialize using the XmlTextWriter.
                        this.clusterManifestSerializer.Serialize(writer, targetClusterManifest);
                    }
                }

                clusterConfigVersion = targetClusterManifest.Version;
            }

            if (isUpgradeStateFailed || (response.TargetMsiUri != null && response.TargetMsiVersion != currentCodeVersion))
            {
                clusterMsiPath = await DownloadMsiAsync(response.TargetMsiUri, response.TargetMsiVersion, tempPath);
            }

            Trace.WriteInfo(TraceType, "ClusterConfigFile {0}, ClusterMsiFile {1}", clusterConfigFilePath, clusterMsiPath);
            var parameter = new ClusterUpgradeCommandParameter()
            {
                CodeVersion = response.TargetMsiVersion,
                CodeFilePath = clusterMsiPath,
                ConfigFilePath = clusterConfigFilePath,
                ConfigVersion = clusterConfigVersion
            };

            parameter.UpgradeDescription = await this.GetClusterUpgradeDescriptionAsync(response.UpgradePolicy, currentClusterHealth, token);

            return parameter;
        }

        private async Task<string> DownloadMsiAsync(Uri targetMsiUri, string targetMsiVersion, string tempPath)
        {
            string clusterMsiPath = null;

#if DotNetCoreClrLinux
                string extension = String.Empty;
                var packageManagerType = FabricEnvironment.GetLinuxPackageManagerType();
                switch (packageManagerType)
                {
                    case LinuxPackageManagerType.Deb:
                        extension = ".deb";
                        break;
                    case LinuxPackageManagerType.Rpm:
                        extension = ".rpm";
                        break;
                    default:
                        Trace.WriteError(TraceType, "Unable to to determine Linux package manager. Type: {0}", packageManagerType);
                        extension = ".unknown";
                        break;
                }
                
                clusterMsiPath = Path.Combine(tempPath, string.Format("servicefabric_{0}{1}", targetMsiVersion, extension));
#else
            var pattern = @"(?<Protocol>\w+):\/\/(?<Domain>[\w@][\w.:@]+)\/(?<Container>[\w-_\W]+\/)*(?<SFFileName>(MicrosoftServiceFabric.Internal|ServiceFabric|WindowsFabric)\.(?<SFVersion>(\d+\.\d+\.\d+\.\d+))\.(?<SFFileExtn>(msi|cab)))[\w\.?=%&=\-@/$,]*";

            var downloadCenterGroup = MatchPattern(pattern, targetMsiUri.ToString());

            if (downloadCenterGroup == null || downloadCenterGroup["SFFileName"] == null || string.IsNullOrEmpty(downloadCenterGroup["SFFileName"].Value))
            {    
                throw new ArgumentException($"Invalid target MSI uri format: {targetMsiUri}! Should contain either ServiceFabric or windowsFabric and file extension should be either cab or msi.", nameof(targetMsiUri));
            }

            clusterMsiPath = Path.Combine(tempPath, downloadCenterGroup["SFFileName"].Value);
#endif      

#if !DotNetCoreClrLinux
            Trace.WriteInfo(TraceType, "Downloading fabric {2} from {0} to {1}", targetMsiUri, clusterMsiPath, downloadCenterGroup[2].Value);
#endif

            var webClient = new HttpClient();

            using (Stream srcStream = await webClient.GetStreamAsync(targetMsiUri))
            using (Stream dstStream = File.Open(clusterMsiPath, FileMode.Create, FileAccess.ReadWrite, FileShare.None))
            {
                await srcStream.CopyToAsync(dstStream);
            }

            Trace.WriteInfo(TraceType, "Download completed");

            return clusterMsiPath;
        }

        public async Task<CommandProcessorClusterUpgradeDescription> GetClusterUpgradeDescriptionAsync(PaasClusterUpgradePolicy paasUpgradePolicy, ClusterHealth currentClusterHealth, CancellationToken token)
        {
            if (paasUpgradePolicy == null)
            {
                return null;
            }

            CommandProcessorClusterUpgradeDescription upgradeDescription = new CommandProcessorClusterUpgradeDescription()
            {
                ForceRestart = paasUpgradePolicy.ForceRestart,
                HealthCheckRetryTimeout = paasUpgradePolicy.HealthCheckRetryTimeout,
                HealthCheckStableDuration = paasUpgradePolicy.HealthCheckStableDuration,
                HealthCheckWaitDuration = paasUpgradePolicy.HealthCheckWaitDuration,
                UpgradeDomainTimeout = paasUpgradePolicy.UpgradeDomainTimeout,
                UpgradeReplicaSetCheckTimeout = paasUpgradePolicy.UpgradeReplicaSetCheckTimeout,
                UpgradeTimeout = paasUpgradePolicy.UpgradeTimeout,
            };

            if (paasUpgradePolicy.HealthPolicy == null && paasUpgradePolicy.DeltaHealthPolicy == null)
            {
                return upgradeDescription;
            }

            upgradeDescription.HealthPolicy = new CommandProcessorClusterUpgradeHealthPolicy();

            var paasHealthPolicy = paasUpgradePolicy.HealthPolicy;
            if (paasHealthPolicy != null)
            {
                upgradeDescription.HealthPolicy.MaxPercentUnhealthyApplications = paasHealthPolicy.MaxPercentUnhealthyApplications;
                upgradeDescription.HealthPolicy.MaxPercentUnhealthyNodes = paasHealthPolicy.MaxPercentUnhealthyNodes;

                if (paasHealthPolicy.ApplicationHealthPolicies != null)
                {
                    upgradeDescription.HealthPolicy.ApplicationHealthPolicies = paasHealthPolicy.ApplicationHealthPolicies.ToDictionary(
                        keyValuePair => keyValuePair.Key,
                        KeyValuePair => KeyValuePair.Value.ToCommondProcessorServiceTypeHealthPolicy());
                }
            }            

            var paasDeltaHealthPolicy = paasUpgradePolicy.DeltaHealthPolicy;
            if (paasDeltaHealthPolicy != null)
            {
                upgradeDescription.DeltaHealthPolicy = new CommandProcessorClusterUpgradeDeltaHealthPolicy()
                {
                    MaxPercentDeltaUnhealthyNodes = paasDeltaHealthPolicy.MaxPercentDeltaUnhealthyNodes,
                    MaxPercentUpgradeDomainDeltaUnhealthyNodes = paasDeltaHealthPolicy.MaxPercentUpgradeDomainDeltaUnhealthyNodes
                };
                
                if (paasDeltaHealthPolicy.MaxPercentDeltaUnhealthyApplications == 100)
                {
                    upgradeDescription.HealthPolicy.MaxPercentUnhealthyApplications = paasDeltaHealthPolicy.MaxPercentDeltaUnhealthyApplications;
                }
                else
                {
                    int totalAppCount = 0, unhealthyAppCount = 0;

                    if(currentClusterHealth == null)
                    {
                        upgradeDescription.HealthPolicy.MaxPercentUnhealthyApplications = 0;
                        Trace.WriteWarning(
                            TraceType,
                            "currentClusterHealth is null. Setting MaxPercentUnhealthyApplications conservatively to 0");
                    }
                    else if (currentClusterHealth.ApplicationHealthStates != null)
                    {
                        var filteredAppHealthStates = currentClusterHealth.ApplicationHealthStates.Where(
                            appHealthState =>
                        {
                            if (appHealthState.ApplicationName.OriginalString.Equals("fabric:/System", StringComparison.OrdinalIgnoreCase))
                            {
                                return false;
                            }

                            if (paasHealthPolicy != null && paasHealthPolicy.ApplicationHealthPolicies != null &&
                                paasHealthPolicy.ApplicationHealthPolicies.ContainsKey(appHealthState.ApplicationName.OriginalString))
                            {
                                return false;
                            }

                            if (paasDeltaHealthPolicy.ApplicationDeltaHealthPolicies != null &&
                                paasDeltaHealthPolicy.ApplicationDeltaHealthPolicies.ContainsKey(appHealthState.ApplicationName.OriginalString))
                            {
                                return false;
                            }

                            return true;
                        });

                        unhealthyAppCount = filteredAppHealthStates.Count(health => health.AggregatedHealthState == HealthState.Error);
                        totalAppCount = filteredAppHealthStates.Count();

                        upgradeDescription.HealthPolicy.MaxPercentUnhealthyApplications = CommandParameterGenerator.GetMaxUnhealthyPercentage(
                            unhealthyAppCount,
                            totalAppCount,
                            paasDeltaHealthPolicy.MaxPercentDeltaUnhealthyApplications);
                    }

                    Trace.WriteInfo(
                        TraceType,
                        "Delta health policy is specified. MaxPercentUnhealthyApplications is overwritten to {0}. TotalApps={1}, UnhealthyApps={2}, MaxPercentDeltaUnhealthyApplications={3}.",
                        upgradeDescription.HealthPolicy.MaxPercentUnhealthyApplications,
                        totalAppCount,
                        unhealthyAppCount,
                        paasDeltaHealthPolicy.MaxPercentDeltaUnhealthyApplications);
                }

                if (paasDeltaHealthPolicy.ApplicationDeltaHealthPolicies != null)
                {
                    foreach (var applicationDeltaHealthPolicy in paasDeltaHealthPolicy.ApplicationDeltaHealthPolicies)
                    {
                        var applicationName = applicationDeltaHealthPolicy.Key;
                        var paasApplicationDeltaHealthPolicy = applicationDeltaHealthPolicy.Value;

                        if (paasApplicationDeltaHealthPolicy.DefaultServiceTypeDeltaHealthPolicy == null && paasApplicationDeltaHealthPolicy.SerivceTypeDeltaHealthPolicies == null)
                        {
                            // no policy provided
                            continue;
                        }

                        ApplicationHealthState matchingHealthState = null;
                        if (currentClusterHealth != null)
                        {
                            matchingHealthState = currentClusterHealth.ApplicationHealthStates.FirstOrDefault(
                               appHealthState => appHealthState.ApplicationName.OriginalString.Equals(applicationName, StringComparison.OrdinalIgnoreCase));
                        }

                        if (matchingHealthState == null)
                        {
                            Trace.WriteWarning(
                                TraceType,
                                "Application {0} is not found in the current cluster health. Ignoring the application since delta policy cannot be computed.",
                                applicationName);

                            // the application is not found in the cluster
                            continue;
                        }

                        Dictionary<string, CommandProcessorServiceTypeHealthPolicy> commandProcessorServiceTypeHealthPolicies = new Dictionary<string, CommandProcessorServiceTypeHealthPolicy>();

                        var serviceList = await this.fabricClientWrapper.GetServicesAsync(matchingHealthState.ApplicationName, Constants.MaxOperationTimeout, token);

                        // Compute the total and unhealthy services by ServiceType for this application
                        Dictionary<string, HealthStats> serviceTypeHealthStatsDictionary = new Dictionary<string, HealthStats>();
                        foreach (var service in serviceList)
                        {
                            HealthStats serviceTypeHealthstats;
                            if (!serviceTypeHealthStatsDictionary.TryGetValue(service.ServiceTypeName, out serviceTypeHealthstats))
                            {
                                serviceTypeHealthstats = new HealthStats();
                                serviceTypeHealthStatsDictionary.Add(service.ServiceTypeName, serviceTypeHealthstats);
                            }

                            if (service.HealthState == HealthState.Error)
                            {
                                serviceTypeHealthstats.UnhealthyCount++;
                            }

                            serviceTypeHealthstats.TotalCount++;
                        }

                        // For each service type specific healthy policy provided, compute the delta health policy
                        if (paasApplicationDeltaHealthPolicy.SerivceTypeDeltaHealthPolicies != null)
                        {
                            foreach (var serviceTypeHealthPolicyKeyValue in paasApplicationDeltaHealthPolicy.SerivceTypeDeltaHealthPolicies)
                            {
                                var serviceTypeName = serviceTypeHealthPolicyKeyValue.Key;
                                var serviceTypeDeltaPolicy = serviceTypeHealthPolicyKeyValue.Value;

                                HealthStats stats;
                                if (serviceTypeHealthStatsDictionary.TryGetValue(serviceTypeName, out stats))
                                {
                                    byte maxUnhealthyPercentage =
                                        CommandParameterGenerator.GetMaxUnhealthyPercentage(stats.UnhealthyCount,
                                            stats.TotalCount, serviceTypeDeltaPolicy.MaxPercentDeltaUnhealthyServices);

                                    commandProcessorServiceTypeHealthPolicies.Add(
                                        serviceTypeName,
                                        new CommandProcessorServiceTypeHealthPolicy()
                                        {
                                            MaxPercentUnhealthyServices = maxUnhealthyPercentage
                                        });

                                    Trace.WriteInfo(
                                        TraceType,
                                        "Delta health policy is specified for ServiceType {0} in Application {1}. MaxPercentUnhealthyServices is overwritten to {2}. TotalCount={3}, UnhealthyCount={4}, MaxPercentDeltaUnhealthyServices={5}.",
                                        serviceTypeName,
                                        applicationName,
                                        maxUnhealthyPercentage,
                                        stats.TotalCount,
                                        stats.UnhealthyCount,
                                        serviceTypeDeltaPolicy.MaxPercentDeltaUnhealthyServices);
                                }
                                else
                                {
                                    Trace.WriteWarning(
                                        TraceType,
                                        "ServiceType {0} in Application {1} is not found in the current application. Ignoring the ServiceType since delta policy cannot be computed.",
                                        serviceTypeName,
                                        applicationName);

                                    continue;
                                }
                            }
                        }

                        // If default service type delta policy is specified, compute the delta health policy for ServiceType
                        // which does not have an explicit policy
                        if (paasApplicationDeltaHealthPolicy.DefaultServiceTypeDeltaHealthPolicy != null)
                        {
                            foreach (var serviceTypeHealthStatsKeyValue in serviceTypeHealthStatsDictionary)
                            {
                                var serviceTypeName = serviceTypeHealthStatsKeyValue.Key;
                                var serviceTypeHealthStats = serviceTypeHealthStatsKeyValue.Value;

                                if (commandProcessorServiceTypeHealthPolicies.ContainsKey(serviceTypeName))
                                {
                                    // Explicit policy has been specified
                                    continue;
                                }

                                byte maxUnhealthyPercentage = CommandParameterGenerator.GetMaxUnhealthyPercentage(
                                    serviceTypeHealthStats.UnhealthyCount,
                                    serviceTypeHealthStats.TotalCount,
                                    paasApplicationDeltaHealthPolicy.DefaultServiceTypeDeltaHealthPolicy.MaxPercentDeltaUnhealthyServices);

                                commandProcessorServiceTypeHealthPolicies.Add(
                                    serviceTypeName,
                                    new CommandProcessorServiceTypeHealthPolicy() { MaxPercentUnhealthyServices = maxUnhealthyPercentage });

                                Trace.WriteInfo(
                                    TraceType,
                                    "Using default delta health policy for ServiceType {0} in Application {1}. MaxPercentUnhealthyServices is overwritten to {2}. TotalCount={3}, UnhealthyCount={4}, MaxPercentDeltaUnhealthyServices={5}.",
                                    serviceTypeName,
                                    applicationName,
                                    maxUnhealthyPercentage,
                                    serviceTypeHealthStats.UnhealthyCount,
                                    serviceTypeHealthStats.UnhealthyCount,
                                    paasApplicationDeltaHealthPolicy.DefaultServiceTypeDeltaHealthPolicy.MaxPercentDeltaUnhealthyServices);
                            }
                        }

                        if (commandProcessorServiceTypeHealthPolicies.Any())
                        {
                            if (upgradeDescription.HealthPolicy.ApplicationHealthPolicies == null)
                            {
                                upgradeDescription.HealthPolicy.ApplicationHealthPolicies = new Dictionary<string, CommandProcessorApplicationHealthPolicy>();
                            }

                            CommandProcessorApplicationHealthPolicy applicationHealthPolicy;
                            if (!upgradeDescription.HealthPolicy.ApplicationHealthPolicies.TryGetValue(applicationName, out applicationHealthPolicy))
                            {
                                applicationHealthPolicy = new CommandProcessorApplicationHealthPolicy() { SerivceTypeHealthPolicies = new Dictionary<string, CommandProcessorServiceTypeHealthPolicy>() };
                                upgradeDescription.HealthPolicy.ApplicationHealthPolicies.Add(
                                    applicationName,
                                    applicationHealthPolicy);
                            }

                            foreach (var commandProcessorServiceTypeHealthPolicy in commandProcessorServiceTypeHealthPolicies)
                            {
                                if (applicationHealthPolicy.SerivceTypeHealthPolicies == null)
                                {
                                    applicationHealthPolicy.SerivceTypeHealthPolicies = new Dictionary<string, CommandProcessorServiceTypeHealthPolicy>();
                                }

                                applicationHealthPolicy.SerivceTypeHealthPolicies[commandProcessorServiceTypeHealthPolicy.Key] = commandProcessorServiceTypeHealthPolicy.Value;
                            }
                        }
                    }
                }
            }

            return upgradeDescription;
        }

        private static byte GetMaxUnhealthyPercentage(int totalUnhealthyCount, int totalCount, byte deltaMaxUnhealthy)
        {
            byte unhealthyPercentage = 0;
            if (totalCount != 0)
            {
                unhealthyPercentage = (byte) ((totalUnhealthyCount * 100) / totalCount);
            }

            return (byte)Math.Min(unhealthyPercentage + deltaMaxUnhealthy, 100);
        }

        private static ClusterManifestType FromBase64ClusterManifest(string clusterManifestBase64)
        {
            if (string.IsNullOrEmpty(clusterManifestBase64))
            {
                return null;
            }

            byte[] clusterManifestBytes = Convert.FromBase64String(clusterManifestBase64);

            var cmSerializer = new XmlSerializer(typeof(ClusterManifestType));
            using (MemoryStream memoryStream = new MemoryStream(clusterManifestBytes))
            {
                XmlReaderSettings settings = new XmlReaderSettings();
#if !DotNetCoreClrLinux
                settings.XmlResolver = null;
#endif
                using (var reader = XmlReader.Create(memoryStream, settings))
                {
                    return (ClusterManifestType)cmSerializer.Deserialize(reader);
                }
            }
        }

        private static GroupCollection MatchPattern(string pattern, string inputstring)
        {
            if (string.IsNullOrEmpty(pattern) || string.IsNullOrEmpty(inputstring))
            {
                Trace.WriteWarning(
                            TraceType, "Either pattern : {0} or Uri :{1} is empty", pattern, inputstring);
                return null;
            }

            Regex regex = new Regex(pattern);
            var match = regex.Match(inputstring);

            if (match.Success)
            {
                return match.Groups;

            }

            return null;
        }

        private class HealthStats
        {
            public int TotalCount { get; set; }

            public int UnhealthyCount { get; set; }
        }
    }
}
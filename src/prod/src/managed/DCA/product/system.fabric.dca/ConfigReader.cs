// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System;
    using System.Collections.Generic;

    // Class that helps in reading configuration
    internal class ConfigReader : IConfigReader
    {
        // Names of DCA-specific sections in settings.xml
        public const string DiagnosticsSectionName = "Diagnostics";
        public const string PaasSectionName = "Paas";
        public const string FabricNodeSectionName = "FabricNode";
        public const string IsScaleMinParamName = "IsScaleMin";
        public const string InstanceNameParamName = "InstanceName";
        public const string ConsumerInstancesParamName = "ConsumerInstances";
        public const string ProducerInstancesParamName = "ProducerInstances";
        public const string ProducerTypeParamName = "ProducerType";
        public const string ConsumerTypeParamName = "ConsumerType";
        public const string ProducerInstanceParamName = "ProducerInstance";
        public const string ClusterIdParamName = "ClusterId";
        public const string EnableTelemetryParamName = "EnableTelemetry";
        public const string TestOnlyTelemetryPushTimeParamName = "TestOnlyTelemetryPushTime";
        public const string TestOnlyTelemetryDailyPushesParamName = "TestOnlyTelemetryDailyPushes";
        public const string FileShareWinFabSectionName = "FileShareWinFabEtw";
        public const string UpgradeOrchestrationServiceConfigSectionName = "UpgradeOrchestrationService";
        public const string GoalStateFileUrlParamName = "GoalStateFileUrl";
        public const string HostingSectionName = "Hosting";
        public const string FabricContainerAppsEnabledParameterName = "FabricContainerAppsEnabled";

        // Application instances for which configuration is available
        private static readonly Dictionary<string, AppConfig> ApplicationConfigurations = new Dictionary<string, AppConfig>();

        // Service packages for which configuration is available
        private static readonly Dictionary<string, Dictionary<string, ServiceConfig>> ServiceConfigurations =
            new Dictionary<string, Dictionary<string, ServiceConfig>>();

        // Application instance for which we need to read configuration
        private readonly string applicationInstanceId;

        // Object containing configuration for the current application instance
        private readonly AppConfig appConfig;

        public ConfigReader(string applicationInstanceId)
        {
            this.applicationInstanceId = applicationInstanceId;
            lock (ApplicationConfigurations)
            {
                this.appConfig = ApplicationConfigurations[this.applicationInstanceId];
            }
        }

        // Whether we're reading from cluster manifest or application manifest
        public bool IsReadingFromApplicationManifest
        {
            get
            {
                return null != this.appConfig;
            }
        }

        public static void AddAppConfig(string applicationInstanceId, AppConfig applicationConfiguration)
        {
            lock (ApplicationConfigurations)
            {
                ApplicationConfigurations[applicationInstanceId] = applicationConfiguration;
            }
        }

        public static void RemoveAppConfig(string applicationInstanceId)
        {
            lock (ApplicationConfigurations)
            {
                ApplicationConfigurations.Remove(applicationInstanceId);
            }
        }

        public static AppConfig GetAppConfig(string applicationInstanceId)
        {
            AppConfig appConfig;
            lock (ApplicationConfigurations)
            {
                appConfig = ApplicationConfigurations[applicationInstanceId];
            }

            return appConfig;
        }

        public static void AddServiceConfig(string applicationInstanceId, string servicePackageName, ServiceConfig serviceConfiguration)
        {
            lock (ServiceConfigurations)
            {
                if (ServiceConfigurations.ContainsKey(applicationInstanceId))
                {
                    ServiceConfigurations[applicationInstanceId][servicePackageName] = serviceConfiguration;
                }
                else
                {
                    Dictionary<string, ServiceConfig> newServiceConfigurations = new Dictionary<string, ServiceConfig>();
                    newServiceConfigurations[servicePackageName] = serviceConfiguration;
                    ServiceConfigurations[applicationInstanceId] = newServiceConfigurations;
                }
            }
        }

        public static void RemoveServiceConfig(string applicationInstanceId, string servicePackageName)
        {
            lock (ServiceConfigurations)
            {
                if (ServiceConfigurations.ContainsKey(applicationInstanceId))
                {
                    ServiceConfigurations[applicationInstanceId].Remove(servicePackageName);
                }
            }
        }

        public static ServiceConfig GetServiceConfig(string applicationInstanceId, string servicePackageName)
        {
            ServiceConfig serviceConfig = null;
            lock (ServiceConfigurations)
            {
                if (ServiceConfigurations.ContainsKey(applicationInstanceId) &&
                    ServiceConfigurations[applicationInstanceId].ContainsKey(servicePackageName))
                {
                    serviceConfig = ServiceConfigurations[applicationInstanceId][servicePackageName];
                }
            }

            return serviceConfig;
        }

        public static Dictionary<string, ServiceConfig> GetServiceConfigsForApp(string applicationInstanceId)
        {
            Dictionary<string, ServiceConfig> serviceConfigs = null;
            lock (ServiceConfigurations)
            {
                if (ServiceConfigurations.ContainsKey(applicationInstanceId))
                {
                    serviceConfigs = ServiceConfigurations[applicationInstanceId];
                }
            }

            return serviceConfigs;
        }

        public static void RemoveServiceConfigsForApp(string applicationInstanceId)
        {
            lock (ServiceConfigurations)
            {
                ServiceConfigurations.Remove(applicationInstanceId);
            }
        }

        public T GetUnencryptedConfigValue<T>(string sectionName, string valueName, T defaultValue)
        {
            if (null == this.appConfig)
            {
                return Utility.GetUnencryptedConfigValue(sectionName, valueName, defaultValue);
            }
            else
            {
                return this.appConfig.GetUnencryptedConfigValue(sectionName, valueName, defaultValue);
            }
        }

        public string ReadString(string sectionName, string valueName, out bool isEncrypted)
        {
            if (null == this.appConfig)
            {
                return Utility.ReadConfigValue(sectionName, valueName, out isEncrypted);
            }
            else
            {
                return this.appConfig.ReadString(sectionName, valueName, out isEncrypted);
            }
        }

        public string GetApplicationLogFolder()
        {
            return (null == this.appConfig) ? null : this.appConfig.GetApplicationLogFolder();
        }

#if !DotNetCoreClr
        public string GetApplicationEtlFilePath()
        {
            return (null == this.appConfig) ? null : this.appConfig.GetApplicationEtwTraceFolder();
        }
#endif

        public string GetApplicationType()
        {
            return (null == this.appConfig) ? null : this.appConfig.ApplicationType;
        }

        public IEnumerable<Guid> GetApplicationEtwProviderGuids()
        {
            if (null == this.appConfig)
            {
                return null;
            }

            HashSet<Guid> guids = new HashSet<Guid>();
            Dictionary<string, ServiceConfig> serviceConfigs = GetServiceConfigsForApp(this.applicationInstanceId);
            foreach (ServiceConfig serviceConfig in serviceConfigs.Values)
            {
                IEnumerable<Guid> guidsForService = serviceConfig.EtwProviderGuids;
                foreach (Guid guid in guidsForService)
                {
                    guids.Add(guid);
                }
            }

            return guids;
        }

        public Dictionary<string, List<ServiceEtwManifestInfo>> GetApplicationEtwManifests()
        {
            if (null == this.appConfig)
            {
                return null;
            }

            Dictionary<string, List<ServiceEtwManifestInfo>> manifests = new Dictionary<string, List<ServiceEtwManifestInfo>>();
            Dictionary<string, ServiceConfig> serviceConfigs = GetServiceConfigsForApp(this.applicationInstanceId);
            foreach (string servicePackageName in serviceConfigs.Keys)
            {
                ServiceConfig serviceConfig = serviceConfigs[servicePackageName];

                List<ServiceEtwManifestInfo> etwManifestPaths = serviceConfig.EtwManifests;
                if (null == etwManifestPaths)
                {
                    continue;
                }

                manifests[servicePackageName] = etwManifestPaths;
            }

            return manifests;
        }

        public IEnumerable<string> GetConsumersForProducer(string producerInstance)
        {
            if (null == this.appConfig)
            {
                // Currently this method is not implemented for the Windows Fabric application
                // because it is not needed. It can be added in the future if needed. For now,
                // just return null.
                return null;
            }

            return this.appConfig.GetConsumersForProducer(producerInstance);
        }
    }
}
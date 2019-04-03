// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using Newtonsoft.Json;

    [JsonObject]
    public class FabricSettingsMetadata
    {
        private static readonly IEnumerable<string> SectionPrefixesWithValidationDisabled =
            new[]
        {
            StringConstants.SectionName.InfrastructureService + "/",
            StringConstants.SectionName.TokenValidationService + "/",
        };

        // private static readonly TraceType TraceType = new TraceType("FabricSettingsMetadata");
        private static readonly HashSet<string> SectionsWithValidationDisabled = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
        {
            StringConstants.SectionName.UnreliableTransport,
            StringConstants.SectionName.Votes,
            StringConstants.SectionName.MetricActivityThresholds,
            StringConstants.SectionName.MetricBalancingThresholds,
            StringConstants.SectionName.GlobalMetricWeights,
            StringConstants.SectionName.DefragmentationMetrics,
            StringConstants.SectionName.NodeBufferPercentage,
            StringConstants.SectionName.FabricTestQueryWeights,
            StringConstants.SectionName.RunAs,
            StringConstants.SectionName.RunAsFabric,
            StringConstants.SectionName.RunAsDca,
            StringConstants.SectionName.RunAsHttpGateway,
            StringConstants.SectionName.InfrastructureService,
            StringConstants.SectionName.NodeCapacities,
            StringConstants.SectionName.NodeProperties,
            StringConstants.SectionName.NodeSfssRgPolicies,
            StringConstants.SectionName.SecurityClusterX509Names,
            StringConstants.SectionName.SecurityServerX509Names,
            StringConstants.SectionName.SecurityClientX509Names,
            StringConstants.SectionName.SecurityAdminClientX509Names,
            StringConstants.SectionName.HealthManagerClusterHealthPolicy,

            // Skip all DCA sections
            StringConstants.SectionName.WinFabEtlFile,
            StringConstants.SectionName.WinFabEtlFileQueryable,
            StringConstants.SectionName.WinFabCrashDump,
            StringConstants.SectionName.WinFabPerfCounter,
            StringConstants.SectionName.WinFabLttProducer,
            StringConstants.SectionName.AzureWinFabCsv,
            StringConstants.SectionName.AzureBlobWinFabEtw,
            StringConstants.SectionName.AzureTableWinFabEtwQueryable,
            StringConstants.SectionName.AzureBlobWinFabCrashDump,
            StringConstants.SectionName.AzureBlobWinFabPerfCounter,
            StringConstants.SectionName.ServiceFabricEtlFile,
            StringConstants.SectionName.ServiceFabricEtlFileQueryable,
            StringConstants.SectionName.ServiceFabricCrashDump,
            StringConstants.SectionName.ServiceFabricPerfCounter,
            StringConstants.SectionName.AzureBlobServiceFabricEtw,
            StringConstants.SectionName.AzureTableServiceFabricEtwQueryable,
            StringConstants.SectionName.AzureBlobServiceFabricCrashDump,
            StringConstants.SectionName.AzureBlobServiceFabricPerfCounter,
            StringConstants.SectionName.FileShareWinFabEtw,
            StringConstants.SectionName.FileShareWinFabCrashDump,
            StringConstants.SectionName.FileShareWinFabPerfCtr,
            StringConstants.SectionName.WinFabPerfCtrFolder,
            StringConstants.SectionName.ServiceFabricPerfCtrFolder
        };

        /* Below sections are treated as NotAllowed because Standalone enables these services by default during deployment. These services should
         * never be disabled, hence treat them as NotAllowed */
        private static readonly string[] SectionsToBeTreatedAsNotAllowed = new[]
        {
            StringConstants.SectionName.ImageStoreService,
            StringConstants.SectionName.FaultAnalysisService,
            StringConstants.SectionName.UpgradeOrchestrationService
        };

        /* Some properties of below services are treated as SingleChange by Standalone to ensure these properties dont change by an upgrade after being enabled.
         * These properties can only be enabled/disabled. Ideally, the product should mark them as SingleChange but to avoid any breaking change in the existing product,
         * we are setting it here. Future system services must set the properties in *Config.h files instead */
        private static readonly string[] SectionsToBeTreatedAsSingleChange = new[]
        {
            StringConstants.SectionName.RepairManager,
            StringConstants.SectionName.BackupRestoreService,
            StringConstants.SectionName.EventStoreService,
            StringConstants.SectionName.GatewayResourceManager
        };

        [JsonConstructor]
        private FabricSettingsMetadata(Dictionary<string, Dictionary<string, ConfigDescription>> settingsMetadata)
        {
            settingsMetadata.MustNotBeNull("settingsMetadata");

            this.SettingsMetadata = settingsMetadata;
            this.Initialize();
        }

        [JsonProperty]
        public Dictionary<string, Dictionary<string, ConfigDescription>> SettingsMetadata { get; private set; }

        public static FabricSettingsMetadata Create(string filePath)
        {
            //// Trace.WriteInfo(TraceType, "Creating FabricSettingsMetadata from {0}", filePath);

            var settingsMetadata = ReadConfigurations(filePath);
            return new FabricSettingsMetadata(settingsMetadata);
        }

        public bool CanConfigurationChange(string sectionName, string parameterName)
        {
            if (SectionsWithValidationDisabled.Contains(sectionName) ||
                SectionPrefixesWithValidationDisabled.Any(prefix => sectionName.StartsWith(prefix, StringComparison.OrdinalIgnoreCase)))
            {
                // In deployer we have section on which we skip validation
                return true;
            }

            ConfigDescription configDescription;
            if (this.TryGetConfigDescription(sectionName, parameterName, out configDescription))
            {
                return configDescription.UpgradePolicy != UpgradePolicy.NotAllowed;
            }

            return true;
        }

        public bool IsPolicySingleChange(string sectionName, string parameterName)
        {
            ConfigDescription configDescription;
            if (this.TryGetConfigDescription(sectionName, parameterName, out configDescription))
            {
                return configDescription.UpgradePolicy == UpgradePolicy.SingleChange;
            }

            return false;
        }

        public bool IsValidConfiguration(string sectionName, string parameterName)
        {
            if (SectionsWithValidationDisabled.Contains(sectionName) ||
                SectionPrefixesWithValidationDisabled.Any(prefix => sectionName.StartsWith(prefix, StringComparison.OrdinalIgnoreCase)))
            {
                // In deployer we have section on which we skip validation
                return true;
            }

            ConfigDescription configDescription;
            return this.TryGetConfigDescription(sectionName, parameterName, out configDescription);
        }

        internal bool TryGetConfigDescription(string sectionName, string parameterName, out ConfigDescription configDescription)
        {
            configDescription = null;

            Dictionary<string, ConfigDescription> parameters;
            if (!this.SettingsMetadata.TryGetValue(sectionName, out parameters))
            {
                // In deployer we have section on which we skip validation
                return false;
            }

            ConfigDescription matchingConfigDescription;

            // Check if the metadata is present for the parameter. If that fails, check if the section
            // has a parameter called "PropertyGroup". Section with 'PropertyGroup' parameter will allow
            // any parameter name
            if (!parameters.TryGetValue(parameterName, out matchingConfigDescription) &&
                !parameters.TryGetValue(StringConstants.PropertyGroupParameter, out matchingConfigDescription))
            {
                return false;
            }

            configDescription = matchingConfigDescription;

            return true;
        }

        private static Dictionary<string, Dictionary<string, ConfigDescription>> ReadConfigurations(string filePath)
        {
            TextReader reader = new StreamReader(filePath);
            var configurations = new Dictionary<string, Dictionary<string, ConfigDescription>>(StringComparer.OrdinalIgnoreCase);
            string line;
            while ((line = reader.ReadLine()) != null)
            {
                if (line.StartsWith("#", StringComparison.Ordinal))
                {
                    continue;
                }

                string[] configurationInfo = line.Split(',');
                ConfigDescription configDesc = new ConfigDescription()
                {
                    SectionName = configurationInfo[0].Trim(),
                    SettingName = configurationInfo[1].Trim(),
                    Type = configurationInfo[2].Trim(),
                    DefaultValue = configurationInfo[3].Trim(),
                    UpgradePolicy = (UpgradePolicy)Enum.Parse(typeof(UpgradePolicy), configurationInfo[4].Trim()),
                    SettingType = configurationInfo[5].Trim()
                };

                if (!configurations.ContainsKey(configDesc.SectionName))
                {
                    configurations.Add(configDesc.SectionName, new Dictionary<string, ConfigDescription>(StringComparer.OrdinalIgnoreCase));
                }

                configurations[configDesc.SectionName].Add(
                    configDesc.SettingName,
                    configDesc);
            }

            return configurations;
        }

        private void Initialize()
        {
            if (this.SettingsMetadata == null)
            {
                return;
            }

            // The following sections are marked as static in Configurations.csv to enable the option
            // to enable the service for live cluster. However, once the cluster is created, FM will not react to any change
            // in these values. These services are created as default during deployment and should not be removed.
            // Hence, making UpgradePolicy as NotAllowed so that SFRP/Standalone will not change the values during upgrade.
            foreach (var sectionName in SectionsToBeTreatedAsNotAllowed)
            {
                this.ChangePolicy(sectionName, UpgradePolicy.NotAllowed);
            }

            // The following sections are marked as static in Configurations.csv to enable the option
            // to enable the service for live cluster. 
            // Ideally, these should have been marked as SingleChange but to avoid any breaking changes in product we will control the policy here.
            // These policies can only be changed if service is being enabled\disabled.
            foreach (var sectionName in SectionsToBeTreatedAsSingleChange)
            {
                this.ChangePolicy(sectionName, UpgradePolicy.SingleChange);
            }
        }

        private void ChangePolicy(string sectionName, UpgradePolicy upgradePolicy)
        {
            if (!this.SettingsMetadata.ContainsKey(sectionName))
            {
                return;
            }

            var section = this.SettingsMetadata[sectionName];
            if (section == null)
            {
                return;
            }

            var minReplicaSetSizeConfigDescription = section[StringConstants.ParameterName.MinReplicaSetSize];
            if (minReplicaSetSizeConfigDescription != null)
            {
                minReplicaSetSizeConfigDescription.UpgradePolicy = upgradePolicy;
            }

            var targetReplicaSetSizeConfigDescription = section[StringConstants.ParameterName.TargetReplicaSetSize];
            if (targetReplicaSetSizeConfigDescription != null)
            {
                targetReplicaSetSizeConfigDescription.UpgradePolicy = upgradePolicy;
            }

            var placementConstraintsConfigDescription = section[StringConstants.ParameterName.PlacementConstraints];
            if (placementConstraintsConfigDescription != null)
            {
                placementConstraintsConfigDescription.UpgradePolicy = upgradePolicy;
            }
        }
    }
}
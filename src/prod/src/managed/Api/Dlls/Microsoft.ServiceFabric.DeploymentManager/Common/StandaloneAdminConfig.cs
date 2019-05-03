// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using System;
    using System.Fabric.Strings;
    using System.IO;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Newtonsoft.Json;

    using DMConstants = Microsoft.ServiceFabric.DeploymentManager.Constants;

    internal class StandaloneAdminConfig : IAdminConfig
    {
        private const string EnvironmentAdminConfigPath = "FabricTestAdminConfigPath";

        // Don't use this property. Keep it only for backward-compatibility purpose.
        [JsonProperty]
        private FabricSettingsMetadata fabricSettingsMetadata;

        // use this property for caching value. Don't need to persist.
        [JsonIgnore]
        private FabricSettingsMetadata fabricSettingsMetadata2;

        [JsonProperty]
        private StandAloneClusterManifestSettings standAloneClusterManifestSettings;

        [JsonConstructor]
        public StandaloneAdminConfig(
            FabricSettingsMetadata fabricSettingsMetadata,
            StandAloneClusterManifestSettings standAloneClusterManifestSettings,
            AdminConfigVersion version,
            bool isUserSet)
        {
            this.fabricSettingsMetadata = fabricSettingsMetadata;
            this.Version = version;
            this.standAloneClusterManifestSettings = standAloneClusterManifestSettings;
            this.IsUserSet = isUserSet;
        }

        public StandaloneAdminConfig(string configurationsFilePath = null, bool isUserSet = false)
        {
            //// Extract Configuration

            string fabricSettingFilePath = string.IsNullOrEmpty(configurationsFilePath) 
                                                 ? StandaloneUtility.FindFabricResourceFile(DMConstants.ConfigurationsFileName)
                                                 : configurationsFilePath;
            this.fabricSettingsMetadata = FabricSettingsMetadata.Create(fabricSettingFilePath);
            string clusterSettingsFilePath = this.GetClusterSettingsFilePath();

            // Extract Cluster Settings
            // ReSharper disable once AssignNullToNotNullAttribute - fabric Code Path cannot be null here.
            this.Version = new AdminConfigVersion("Baseline", "Baseline");
            this.standAloneClusterManifestSettings = JsonConvert.DeserializeObject<StandAloneClusterManifestSettings>(
                        File.ReadAllText(clusterSettingsFilePath));
            this.IsUserSet = isUserSet;
        }

        public AdminConfigVersion Version
        {
            get;
            set;
        }

        public bool IsUserSet
        {
            get;
            set;
        }

        public FabricSettingsMetadata GetFabricSettingsMetadata()
        {
            //// Extract Configuration
            if (this.fabricSettingsMetadata2 == null)
            {
                string fabricSettingFilePath = StandaloneUtility.FindFabricResourceFile(DMConstants.ConfigurationsFileName);
                this.fabricSettingsMetadata2 = FabricSettingsMetadata.Create(fabricSettingFilePath);
            }

            return this.fabricSettingsMetadata2;
        }

        IClusterManifestSettings IAdminConfig.GetAdminFabricSettings()
        {
            return this.standAloneClusterManifestSettings;
        }

        public string GetClusterSettingsFilePath()
        {
            string clusterSettingsFilePath = null;
            string newAdminConfigPath = Environment.GetEnvironmentVariable(EnvironmentAdminConfigPath);
            if (!string.IsNullOrEmpty(newAdminConfigPath))
            {
                clusterSettingsFilePath = newAdminConfigPath;
            }
            else
            {
                string fabricDataPath = System.Fabric.FabricDeployer.Utility.GetFabricDataRoot();
                if (fabricDataPath != null)
                {
                    string filePath = Path.Combine(fabricDataPath, DMConstants.ClusterSettingsFileName);
                    if (File.Exists(filePath))
                    {
                        clusterSettingsFilePath = filePath;
                    }
                }
            }

            if (string.IsNullOrEmpty(clusterSettingsFilePath))
            {
                clusterSettingsFilePath = StandaloneUtility.FindFabricResourceFile(DMConstants.ClusterSettingsFileName);
            }

            return clusterSettingsFilePath;
        }

        public override bool Equals(object other)
        {
            var otherWrpConfig = other as StandaloneAdminConfig;
            if (otherWrpConfig == null)
            {
                return false;
            }

            return this.Version.Equals(otherWrpConfig.Version) &&
                this.IsUserSet == otherWrpConfig.IsUserSet;
        }

        public override int GetHashCode()
        {
            var hash = 27;
            hash = (13 * hash) + this.Version.GetHashCode();
            hash = (13 * hash) + this.IsUserSet.GetHashCode();
            return hash;
       }
    }
}
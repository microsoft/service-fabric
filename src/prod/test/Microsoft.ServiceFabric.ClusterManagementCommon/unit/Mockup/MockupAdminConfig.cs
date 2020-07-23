// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using Newtonsoft.Json;
    using System;
    using System.Fabric.Common;
    using System.IO;

    internal class MockupAdminConfig : IAdminConfig
    {
        private const string EnvironmentAdminConfigPath = "FabricTestAdminConfigPath";

        [JsonProperty]
        private FabricSettingsMetadata fabricSettingsMetadata;

        [JsonProperty]
        private MockupClusterManifestSettings mockupClusterManifestSettings;

        public MockupAdminConfig(bool isUserSet = false)
        {
            string fabricSettingFilePath = Utility.FindFabricResourceFile(Constant.ConfigurationsFileName);
            this.fabricSettingsMetadata = FabricSettingsMetadata.Create(fabricSettingFilePath);
            string clusterSettingsFilePath = this.GetClusterSettingsFilePath();

            // Extract Cluster Settings
            // ReSharper disable once AssignNullToNotNullAttribute - fabric Code Path cannot be null here.
            this.Version = new AdminConfigVersion("Baseline", "Baseline");
            this.mockupClusterManifestSettings = JsonConvert.DeserializeObject<MockupClusterManifestSettings>(
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

        public IClusterManifestSettings GetAdminFabricSettings()
        {
            return JsonConvert.DeserializeObject<MockupClusterManifestSettings>(
                        File.ReadAllText(this.GetClusterSettingsFilePath()));
        }

        public FabricSettingsMetadata GetFabricSettingsMetadata()
        {
            string fabricSettingFilePath = Utility.FindFabricResourceFile(Constant.ConfigurationsFileName);
            return FabricSettingsMetadata.Create(fabricSettingFilePath);
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
                string fabricDataPath = GetFabricDataRoot();
                if (fabricDataPath != null)
                {
                    string filePath = Path.Combine(fabricDataPath, Constant.ClusterSettingsFileName);
                    if (File.Exists(filePath))
                    {
                        clusterSettingsFilePath = filePath;
                    }
                }
            }

            if (string.IsNullOrEmpty(clusterSettingsFilePath))
            {
                clusterSettingsFilePath = Utility.FindFabricResourceFile(Constant.ClusterSettingsFileName);
            }

            return clusterSettingsFilePath;
        }

        internal static string GetFabricDataRoot()
        {
            try
            {
                return FabricEnvironment.GetDataRoot();
            }
            catch (System.Fabric.FabricException)
            {
                return null;
            }
        }
    }
}
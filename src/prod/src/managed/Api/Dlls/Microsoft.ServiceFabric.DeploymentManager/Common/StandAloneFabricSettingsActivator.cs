// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using Microsoft.ServiceFabric.ClusterManagementCommon;

    internal class StandAloneFabricSettingsActivator : IFabricSettingsActivator
    {
        private static NativeConfigStore currentConfigStore;

        private string clusterId;
        private string currentPrimaryAccountNtlmPassword;
        private string currentSecondaryAccountNtlmPassword;
        private string currentCommonNameNtlmPassword;

        public StandAloneFabricSettingsActivator(string clusterId)
        {
            this.clusterId = clusterId;

            if (currentConfigStore != null)
            {
                this.currentPrimaryAccountNtlmPassword = currentConfigStore.ReadUnencryptedString(StringConstants.SectionName.FileStoreService, StringConstants.ParameterName.PrimaryAccountNtlmPasswordSecret);
                this.currentSecondaryAccountNtlmPassword = currentConfigStore.ReadUnencryptedString(StringConstants.SectionName.FileStoreService, StringConstants.ParameterName.SecondaryAccountNtlmPasswordSecret);
                this.currentCommonNameNtlmPassword = currentConfigStore.ReadUnencryptedString(StringConstants.SectionName.FileStoreService, StringConstants.ParameterName.CommonNameNtlmPasswordSecret);                
            }
        }

        public static void InitializeConfigStore(NativeConfigStore configStore)
        {
            currentConfigStore = configStore;
        }

        public FabricSettingsGeneratorBase CreateWindowsSettingsGenerator(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            FabricSettingsMetadata settingsMetadata,
            ClusterManifestType existingClusterManifest,
            ClusterManifestGeneratorSettings clusterManifestGeneratorSettings,
            ITraceLogger traceLogger)
        {
            return new StandAloneFabricSettingsGenerator(
                       this.clusterId,
                       targetCsmConfig,
                       targetWrpConfig,
                       settingsMetadata,
                       existingClusterManifest,
                       clusterManifestGeneratorSettings,
                       traceLogger,
                       this.currentPrimaryAccountNtlmPassword,
                       this.currentSecondaryAccountNtlmPassword,
                       this.currentCommonNameNtlmPassword);
        }

        public FabricSettingsGeneratorBase CreateLinuxSettingsGenerator(IUserConfig targetCsmConfig, IAdminConfig targetWrpConfig, FabricSettingsMetadata settingsMetadata, ClusterManifestType existingClusterManifest, ClusterManifestGeneratorSettings clusterManifestGeneratorSettings, ITraceLogger traceLogger)
        {
            throw new System.NotImplementedException();
        }
    }
}
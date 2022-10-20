// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Model
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Fabric;
    using System.Fabric.FabricDeployer;
    using System.Fabric.Strings;
    using System.Linq;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using DMConstants = Microsoft.ServiceFabric.DeploymentManager.Constants;

    internal abstract class PropertyBase
    {
        public List<SettingsSectionDescription> FabricSettings
        {
            get;
            set;
        }

        [DefaultValue(true)]
        public bool EnableTelemetry
        {
            get;
            set;
        }

        public EncryptableDiagnosticStoreInformation DiagnosticsStore
        {
            get;
            set;
        }

        public bool? FabricClusterAutoupgradeEnabled
        {
            get;
            set;
        }

        public string ClusterRegistrationId
        {
            get;
            set;
        }

        [JsonProperty]
        protected internal ReliabilityLevel? TestHookReliabilityLevel
        {
            get;
            set;
        }

        internal Dictionary<string, string> GetFabricSystemSettings()
        {
            var settings = new Dictionary<string, string>();
            var fabricSettings = this.FabricSettings;
            if (fabricSettings == null)
            {
                return settings;
            }

            var setupSection = fabricSettings.FirstOrDefault(section => section.Name == FabricValidatorConstants.SectionNames.Setup);
            if (setupSection == null)
            {
                return settings;
            }

            var fabricDataRoot = setupSection.Parameters.FirstOrDefault(
                parameter => parameter.Name == DMConstants.FabricDataRootString);
            if (fabricDataRoot != null)
            {
                settings.Add(fabricDataRoot.Name, fabricDataRoot.Value);
            }

            var fabricLogRoot = setupSection.Parameters.FirstOrDefault(
                parameter => parameter.Name == DMConstants.FabricLogRootString);
            if (fabricLogRoot != null)
            {
                settings.Add(fabricLogRoot.Name, fabricLogRoot.Value);
            }

            var isDevCluster = setupSection.Parameters.FirstOrDefault(
                parameter => parameter.Name == StringConstants.ParameterName.IsDevCluster);
            if (isDevCluster != null)
            {
                settings.Add(isDevCluster.Name, isDevCluster.Value);
            }

            var isTestCluster = setupSection.Parameters.FirstOrDefault(
                parameter => parameter.Name == StringConstants.ParameterName.IsTestCluster);
            if (isTestCluster != null)
            {
                settings.Add(isTestCluster.Name, isTestCluster.Value);
            }

            return settings;
        }

        internal virtual void UpdateUserConfig(IUserConfig userConfig, List<NodeDescriptionGA> nodes)
        {
            userConfig.EnableTelemetry = this.EnableTelemetry;
            userConfig.FabricSettings = this.FabricSettings;
            userConfig.DiagnosticsStorageAccountConfig = null;
            userConfig.DiagnosticsStoreInformation = this.DiagnosticsStore;
            userConfig.AutoupgradeEnabled = this.FabricClusterAutoupgradeEnabled ?? !userConfig.IsScaleMin;
        }

        internal virtual void UpdateFromUserConfig(IUserConfig userConfig)
        {
            this.FabricClusterAutoupgradeEnabled = userConfig.AutoupgradeEnabled;
            this.DiagnosticsStore = userConfig.DiagnosticsStoreInformation;
            this.EnableTelemetry = userConfig.EnableTelemetry;
            this.FabricSettings = userConfig.FabricSettings;
        }

        internal virtual void Validate(List<NodeDescriptionGA> nodes)
        {
            this.ValidateFabricSettings();
            if (this.DiagnosticsStore != null)
            {
                this.DiagnosticsStore.Verify();
            }
        }

        private static void ValidateFabricSetupParameter(string parameterName, string parameterValue)
        {
            // Verify path is an absolute path
            Uri uriPath;
            try
            {
                uriPath = new Uri(parameterValue);
            }
            catch (System.UriFormatException)
            {
                throw new ValidationException(
                    ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid,
                    string.Format(StringResources.Error_BPAJsonDataRootMustBeAbsolutePath, parameterName));
            }

            if (!uriPath.IsAbsoluteUri)
            {
                throw new ValidationException(
                    ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid,
                    string.Format(StringResources.Error_BPAJsonDataRootMustBeAbsolutePath, parameterName));
            }

            // Verify path is not in UNC format
            if (uriPath.IsUnc)
            {
                throw new ValidationException(
                    ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid,
                    string.Format(StringResources.Error_BPAJsonDataRootMustNotBeUnc, parameterName));
            }

            // Check Data/Log root is under set character length to prevent long path issues
            if (parameterValue.Length > Microsoft.ServiceFabric.DeploymentManager.Constants.FabricDataRootMaxPath)
            {
                var message = string.Format(
                            StringResources.Error_BPAJsonDataRootLongPath,
                            parameterName,
                            Microsoft.ServiceFabric.DeploymentManager.Constants.FabricDataRootMaxPath);
                throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, message);
            }
        }

        private void ValidateFabricSettings()
        {
            if (this.FabricSettings == null)
            {
                return;
            }

            var setupSection = this.FabricSettings.FirstOrDefault(section => section.Name == FabricValidatorConstants.SectionNames.Setup);

            if (setupSection != null)
            {
                var dataRootParam = setupSection.Parameters.FirstOrDefault(param => param.Name == DMConstants.FabricDataRootString);
                if (dataRootParam != null)
                {
                    ValidateFabricSetupParameter(DMConstants.FabricDataRootString, dataRootParam.Value);
                }

                var logRootParam = setupSection.Parameters.FirstOrDefault(param => param.Name == DMConstants.FabricDataRootString);
                if (logRootParam != null)
                {
                    ValidateFabricSetupParameter(DMConstants.FabricDataRootString, logRootParam.Value);
                }
            }
        }
    }
}
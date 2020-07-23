// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System;
    using System.Globalization;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;

    /// <summary>
    /// This is the class handles the validation of configuration for the hosting subsystem
    /// </summary>
    class UnsupportedPreviewFeatureValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.Common; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {

        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {
            // Fail if preview features are not enabled for the cluster but one of the preview features is opted-in
            var settingsForThisSection = targetWindowsFabricSettings.GetSection(this.SectionName);

            bool arePreviewFeaturesEnabledForTheCluster = settingsForThisSection[FabricValidatorConstants.ParameterNames.Common.EnableUnsupportedPreviewFeatures].GetValue<bool>();
            if (!arePreviewFeaturesEnabledForTheCluster)
            {
                // Is SFVolumeDisk enabled?
                var settingsHostingSection = targetWindowsFabricSettings.GetSection(FabricValidatorConstants.SectionNames.Hosting);
                bool isSFVolumeDiskServiceEnabled = settingsHostingSection[FabricValidatorConstants.ParameterNames.Hosting.IsSFVolumeDiskServiceEnabled].GetValue<bool>();
                if (isSFVolumeDiskServiceEnabled)
                {
                    // Fail the upgrade
                    throw new InvalidOperationException("Cannot use SFVolumeDisk service preview feature when preview features are disabled for the cluster");
                }
            }

            // Is CentralSecretService enabled?
            var settingsCentralSecretServiceSection = targetWindowsFabricSettings.GetSection(FabricValidatorConstants.SectionNames.CentralSecretService);
            bool isCentralSecretServiceEnabled = settingsCentralSecretServiceSection[FabricValidatorConstants.ParameterNames.CentralSecretService.IsEnabled].GetValue<bool>();
            if (!arePreviewFeaturesEnabledForTheCluster && isCentralSecretServiceEnabled)
            {
                // Fail the upgrade
                throw new InvalidOperationException("Cannot use Central Secret Service preview feature when preview features are disabled for the cluster");
            }

            // CentralSecretService: Is cluster cert configured?
            if (arePreviewFeaturesEnabledForTheCluster && isCentralSecretServiceEnabled)
            {
                if (string.IsNullOrWhiteSpace(settingsCentralSecretServiceSection[FabricValidatorConstants.ParameterNames.CentralSecretService.EncryptionCertificateThumbprint].GetValue<string>()))
                {
                    var securitySection = targetWindowsFabricSettings.GetSection(FabricValidatorConstants.SectionNames.Security);
                    var clusterCredentialTypeString = securitySection[FabricValidatorConstants.ParameterNames.ClusterCredentialType].GetValue<string>();
                    CredentialType clusterCredentialType;

                    if (!Enum.TryParse<CredentialType>(clusterCredentialTypeString, out clusterCredentialType)
                        || clusterCredentialType != CredentialType.X509
                        || string.IsNullOrWhiteSpace(securitySection["CertificateInformation/ClusterCertificate/Thumbprint"].GetValue<string>()))
                    {
                        throw new InvalidOperationException("Central Secret Service preview feature requires either an encryption certificate configured (CentralSecretService/EncryptionCertificateThumbprint) or the cluster must be secured with a certificate (ClusterCredentialType must be X509 with a thumbprint configured).");
                    }
                }
            }

            // Is NetworkInventoryManager enabled?
            var settingsNetworkInventoryManagerSection = targetWindowsFabricSettings.GetSection(FabricValidatorConstants.SectionNames.NetworkInventoryManager);
            bool isNetworkInventoryManagerEnabled = settingsNetworkInventoryManagerSection[FabricValidatorConstants.ParameterNames.NetworkInventoryManager.IsEnabled].GetValue<bool>();
            if (!arePreviewFeaturesEnabledForTheCluster && isNetworkInventoryManagerEnabled)
            {
                // Fail the upgrade
                throw new InvalidOperationException("Cannot use Isolated Network preview feature when preview features are disabled for the cluster");
            }

            // Is IsolatedNetworkSetup enabled?
            var settingsSetupSection = targetWindowsFabricSettings.GetSection(FabricValidatorConstants.SectionNames.Setup);
            bool isIsolatedNetworkSetupEnabled = settingsSetupSection[FabricValidatorConstants.ParameterNames.Setup.IsolatedNetworkSetup].GetValue<bool>();
            if (!arePreviewFeaturesEnabledForTheCluster && isIsolatedNetworkSetupEnabled)
            {
                // Fail the upgrade
                throw new InvalidOperationException("Cannot use Isolated Network preview feature when preview features are disabled for the cluster");
            }
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Chaos.Common;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.Linq;

    /// <summary>
    /// This class stores the settings for a single node.
    /// </summary>
    internal class ClusterSettings
    {
        #region Constructors
        public ClusterSettings(SettingsOverridesTypeSection[] otherSettings, SettingsTypeSection votes, SettingsTypeSection seedNodeClientConnectionAddresses, FabricCertificateType secretsCertificate)
        {
            this.Settings = new List<SettingsTypeSection>();
            this.Settings.Add(votes);
            this.Settings.Add(seedNodeClientConnectionAddresses);
            AddOtherSettings(otherSettings, secretsCertificate);
            AddApplicationLogCollectorSettingsIfRequired();
            DcaSettings.AddDcaSettingsIfRequired(this.Settings);
        }
        public ClusterSettings(List<SettingsTypeSection> settings)
        {
            this.Settings = settings;
        }

        #endregion

        #region Public Properties
        public List<SettingsTypeSection> Settings { get; private set; }
        #endregion

        #region Private Functions
        private void AddOtherSettings(SettingsOverridesTypeSection[] settings, FabricCertificateType secretsCertificate)
        {
            foreach (SettingsOverridesTypeSection section in settings)
            {
                if (section.Name != Constants.SectionNames.Votes && section.Name != Constants.SectionNames.SeedNodeClientConnectionAddresses)
                {
                    var settingsTypeSection = ClusterSettings.ConvertToSettingsTypeSection(section);
                    if (settingsTypeSection != null)
                    {
                        this.Settings.Add(settingsTypeSection);
                    }
                }
            }

            // Adds the "SettingsX509StoreName" parameter under the "Security" section
            // if a SettingsCertificate is specified on the ClusterManifest
            if (secretsCertificate != null)
            {
                SettingsTypeSection securitySection = this.Settings.FirstOrDefault(section => section.Name.Equals(Constants.SectionNames.Security, StringComparison.OrdinalIgnoreCase));

                if (securitySection == null)
                {
                    securitySection = new SettingsTypeSection() { Name = Constants.SectionNames.Security };
                    this.Settings.Add(securitySection);
                }

                SettingsTypeSectionParameter securityParameter = null;
                string securityParameterName = string.Format(CultureInfo.InvariantCulture, Constants.ParameterNames.X509StoreNamePattern, "Settings");
                if (securitySection.Parameter != null)
                {
                    securityParameter = securitySection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(securityParameterName, StringComparison.OrdinalIgnoreCase));
                }

                if (securityParameter == null)
                {
                    securityParameter = new SettingsTypeSectionParameter() { Name = securityParameterName };                    
                }

                securityParameter.Value = secretsCertificate.X509StoreName;
                securityParameter.IsEncrypted = false;

                List<SettingsTypeSectionParameter> parameterList = 
                    (securitySection.Parameter != null) ? new List<SettingsTypeSectionParameter>(securitySection.Parameter) : new List<SettingsTypeSectionParameter>();
                parameterList.Add(securityParameter);

                securitySection.Parameter = parameterList.ToArray();
            }

        }

        private static SettingsTypeSection ConvertToSettingsTypeSection(SettingsOverridesTypeSection overridesSection)
        {
            List<SettingsTypeSectionParameter> parameters = new List<SettingsTypeSectionParameter>();
            if (overridesSection == null)
            {
                return null;
            }

            if (overridesSection.Parameter != null)
            {
                foreach (SettingsOverridesTypeSectionParameter parameter in overridesSection.Parameter)
                {
                    parameters.Add(new SettingsTypeSectionParameter() { Name = parameter.Name, Value = parameter.Value, MustOverride = false, IsEncrypted = parameter.IsEncrypted });
                }
            }

            return new SettingsTypeSection() { Name = overridesSection.Name, Parameter = parameters.ToArray() };
        }

        private void AddApplicationLogCollectorSettingsIfRequired()
        {
            var diagnosticsFileStoreSection = GetDiagnosticsFileStoreSectionIfApplicationLogCollectionIsEnabled();
            if (diagnosticsFileStoreSection == null)
            {
                return;
            }

            List<SettingsTypeSectionParameter> managementParameters = new List<SettingsTypeSectionParameter>();
            var connectionString = diagnosticsFileStoreSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(System.Fabric.FabricValidatorConstants.ParameterNames.StoreConnectionString, StringComparison.OrdinalIgnoreCase));
            if (connectionString != null)
            {
                managementParameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.MonitoringAgentStorageAccount, Value = connectionString.Value, MustOverride = false, IsEncrypted = connectionString.IsEncrypted });
            }

            var uploadIntervalParameter = diagnosticsFileStoreSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(System.Fabric.FabricValidatorConstants.ParameterNames.UploadIntervalInMinutes, StringComparison.OrdinalIgnoreCase));
            if (uploadIntervalParameter != null)
            {
                int uploadIntervalInSeconds = Int32.Parse(uploadIntervalParameter.Value, CultureInfo.InvariantCulture) * 60;
                managementParameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.MonitoringAgentTransferInterval, Value = uploadIntervalInSeconds.ToString(CultureInfo.InvariantCulture), MustOverride = false, IsEncrypted = uploadIntervalParameter.IsEncrypted });
            }

            var appLogDirectoryQuotaParameter = diagnosticsFileStoreSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(System.Fabric.FabricValidatorConstants.ParameterNames.AppLogDirectoryQuotaInMB, StringComparison.OrdinalIgnoreCase));
            if (appLogDirectoryQuotaParameter != null)
            {
                managementParameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.MonitoringAgentDirectoryQuota, Value = appLogDirectoryQuotaParameter.Value, MustOverride = false, IsEncrypted = appLogDirectoryQuotaParameter.IsEncrypted });
            }

            var managementSection = this.Settings.FirstOrDefault(section => section.Name.Equals(Constants.SectionNames.Management, StringComparison.OrdinalIgnoreCase));
            if (managementSection == null)
            {
                this.Settings.Add(new SettingsTypeSection() { Name = Constants.SectionNames.Management, Parameter = managementParameters.ToArray() });
            }
            else if (managementSection.Parameter == null)
            {
                managementSection.Parameter = managementParameters.ToArray();
            }
            else
            {
                managementParameters.AddRange(managementSection.Parameter);
                managementSection.Parameter = managementParameters.ToArray();
            }
        }

        private SettingsTypeSection GetDiagnosticsFileStoreSectionIfApplicationLogCollectionIsEnabled()
        {
            var diagnosticsFileStoreSection = this.Settings.FirstOrDefault(section => section.Name.Equals(Constants.SectionNames.DiagnosticFileStore, StringComparison.OrdinalIgnoreCase));
            if (diagnosticsFileStoreSection == null)
            {
                return null;
            }

            var isAppLogCollectionEnabledParameter = diagnosticsFileStoreSection.Parameter.FirstOrDefault(
                parameter => parameter.Name.Equals(System.Fabric.FabricValidatorConstants.ParameterNames.IsAppLogCollectionEnabled, StringComparison.OrdinalIgnoreCase));
            if (isAppLogCollectionEnabledParameter == null)
            {
                return null;
            }

            if (Boolean.Parse(isAppLogCollectionEnabledParameter.Value) == false)
            {
                return null;
            }

            return diagnosticsFileStoreSection;
        }
        #endregion
    }
}
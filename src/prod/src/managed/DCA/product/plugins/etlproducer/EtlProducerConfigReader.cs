// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;

    internal class EtlProducerConfigReader : IEtlProducerConfigReader
    {
        private const string ApplicationTracesEtlFilePattern = "fabric_application_traces_*.etl";

        private readonly ConfigReader configReader;
        private readonly string sectionName;
        private readonly FabricEvents.ExtensionsEvents traceSource;
        private readonly string logSourceId;

        public EtlProducerConfigReader(
            ConfigReader configReader,
            string sectionName,
            FabricEvents.ExtensionsEvents traceSource,
            string logSourceId)
        {
            this.configReader = configReader;
            this.sectionName = sectionName;
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;
        }

        public EtlProducerSettings GetSettings()
        {
            if (this.configReader.IsReadingFromApplicationManifest)
            {
                return this.GetSettingsFromApplicationManifest();
            }

            return this.GetSettingsFromClusterManifest();
        }

        private EtlProducerSettings GetSettingsFromClusterManifest()
        {
            var etlProducerSettings = new EtlProducerSettingsBuilder
            {
                Enabled = this.configReader.GetUnencryptedConfigValue(
                    this.sectionName,
                    EtlProducerValidator.EnabledParamName,
                    EtlProducerConstants.EtlProducerEnabledByDefault),
                ProcessingWinFabEtlFiles = true
            };

            // Check for values in settings.xml
            if (etlProducerSettings.Enabled)
            {
                etlProducerSettings.EtlReadInterval = TimeSpan.FromMinutes(this.configReader.GetUnencryptedConfigValue(
                                                                      this.sectionName,
                                                                      EtlProducerValidator.EtlReadIntervalParamName,
                                                                      EtlProducerConstants.DefaultEtlReadInterval.TotalMinutes));

                var etlDeletionAge = TimeSpan.FromDays(this.configReader.GetUnencryptedConfigValue(
                                             this.sectionName,
                                             EtlProducerValidator.DataDeletionAgeParamName,
                                             EtlProducerConstants.DefaultDataDeletionAge.TotalDays));
                if (etlDeletionAge > EtlProducerConstants.MaxDataDeletionAge)
                {
                    this.traceSource.WriteWarning(
                        this.logSourceId,
                        "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                        etlDeletionAge,
                        this.sectionName,
                        EtlProducerValidator.DataDeletionAgeParamName,
                        EtlProducerConstants.MaxDataDeletionAge);
                    etlDeletionAge = EtlProducerConstants.MaxDataDeletionAge;
                }

                etlProducerSettings.EtlDeletionAgeMinutes = etlDeletionAge;

                // Check for test settings
                var logDeletionAgeTestValue = TimeSpan.FromMinutes(this.configReader.GetUnencryptedConfigValue(
                                                  this.sectionName,
                                                  EtlProducerValidator.TestDataDeletionAgeParamName,
                                                  0));
                if (logDeletionAgeTestValue != TimeSpan.Zero)
                {
                    if (logDeletionAgeTestValue > EtlProducerConstants.MaxDataDeletionAge)
                    {
                        this.traceSource.WriteWarning(
                            this.logSourceId,
                            "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                            logDeletionAgeTestValue,
                            this.sectionName,
                            EtlProducerValidator.TestDataDeletionAgeParamName,
                            EtlProducerConstants.MaxDataDeletionAge);
                        logDeletionAgeTestValue = EtlProducerConstants.MaxDataDeletionAge;
                    }

                    etlProducerSettings.EtlDeletionAgeMinutes = logDeletionAgeTestValue;
                }

                string etlTypeParamName = EtlProducerValidator.WindowsFabricEtlTypeParamName;
                string serviceFabricEtlTypeString = this.configReader.GetUnencryptedConfigValue(
                                                 this.sectionName,
                                                 EtlProducerValidator.WindowsFabricEtlTypeParamName,
                                                 EtlProducerValidator.DefaultEtl);
                if (serviceFabricEtlTypeString.Equals(EtlProducerValidator.DefaultEtl))
                {
                    serviceFabricEtlTypeString = this.configReader.GetUnencryptedConfigValue(
                                                 this.sectionName,
                                                 EtlProducerValidator.ServiceFabricEtlTypeParamName,
                                                 EtlProducerValidator.DefaultEtl);
                    etlTypeParamName = EtlProducerValidator.ServiceFabricEtlTypeParamName;
                }

                WinFabricEtlType parsedType;
                if (false == Enum.TryParse(serviceFabricEtlTypeString, out parsedType))
                {
                    this.traceSource.WriteWarning(
                        this.logSourceId,
                        "The value {0} specified for section {1}, parameter {2} is not recognized. Therefore the default value {3} will be used instead.",
                        serviceFabricEtlTypeString,
                        this.sectionName,
                        etlTypeParamName,
                        EtlProducerValidator.DefaultEtl);
                    parsedType = WinFabricEtlType.DefaultEtl;
                }

                etlProducerSettings.ServiceFabricEtlType = parsedType;

                etlProducerSettings.EtlPath = this.configReader.GetUnencryptedConfigValue(
                                                       this.sectionName,
                                                       EtlProducerValidator.TestEtlPathParamName,
                                                       string.Empty);
                if (false == string.IsNullOrEmpty(etlProducerSettings.EtlPath))
                {
                    etlProducerSettings.EtlFilePatterns = this.configReader.GetUnencryptedConfigValue(
                                                                   this.sectionName,
                                                                   EtlProducerValidator.TestEtlFilePatternsParamName,
                                                                   string.Empty);
                    if (string.IsNullOrEmpty(etlProducerSettings.EtlFilePatterns))
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "The ETL path {0} specified for section {1}, parameter {2} is being ignored because parameter {3} has not been specified. The default ETL path will be used instead.",
                            etlProducerSettings.EtlPath,
                            this.sectionName,
                            EtlProducerValidator.TestEtlPathParamName,
                            EtlProducerValidator.TestEtlFilePatternsParamName);
                        etlProducerSettings.EtlPath = string.Empty;
                    }
                    else
                    {
                        string customManifestPath = this.configReader.GetUnencryptedConfigValue(
                                                        this.sectionName,
                                                        EtlProducerValidator.TestManifestPathParamName,
                                                        string.Empty);
                        etlProducerSettings.CustomManifestPaths = new List<string> { customManifestPath };

                        etlProducerSettings.ProcessingWinFabEtlFiles = this.configReader.GetUnencryptedConfigValue(
                                                                                this.sectionName,
                                                                                EtlProducerValidator.TestProcessingWinFabEtlFilesParamName,
                                                                                false);
                    }
                }

                // Write settings to log
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "ETL processing enabled: ETL read interval (minutes): {0}, ETL deletion age ({1}): {2}",
                    etlProducerSettings.EtlReadInterval.TotalMinutes,
                    (logDeletionAgeTestValue == TimeSpan.Zero) ? "days" : "minutes",
                    (logDeletionAgeTestValue == TimeSpan.Zero) ? etlDeletionAge : logDeletionAgeTestValue);

                if (false == string.IsNullOrEmpty(etlProducerSettings.EtlPath))
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Custom ETL file info provided. Path: {0}, ETL file name patterns: {1}.",
                        etlProducerSettings.EtlPath,
                        etlProducerSettings.EtlFilePatterns);
                }
            }
            else
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "ETL processing not enabled");
            }

            return etlProducerSettings.ToEtlProducerSettings();
        }

        private EtlProducerSettings GetSettingsFromApplicationManifest()
        {
            // Check for values in application manifest
            var etlProducerSettings = new EtlProducerSettingsBuilder
            {
                Enabled = this.configReader.GetUnencryptedConfigValue(
                    this.sectionName,
                    EtlProducerValidator.EnabledParamName,
                    EtlProducerConstants.EtlProducerEnabledByDefault)
            };
            if (etlProducerSettings.Enabled)
            {
#if !DotNetCoreClr                
                etlProducerSettings.EtlPath = this.configReader.GetApplicationEtlFilePath();
                if (string.IsNullOrEmpty(etlProducerSettings.EtlPath))
                {
                    // We were unable to determine ETL file path
                    etlProducerSettings.Enabled = false;
                    return etlProducerSettings.ToEtlProducerSettings();
                }

                etlProducerSettings.EtlReadInterval = this.GetEtlReadIntervalFromApplicationManifest();

                // We don't delete application ETL files in the plugin because the same ETL files are shared across 
                // all applications. Therefore, the DCA hosting infrastructure does the deletion, instead of having
                // the plugins do it. So let's set the data delete age to the maximum possible value (which effectively
                // means never delete).
                etlProducerSettings.EtlDeletionAgeMinutes = EtlProducerConstants.MaxDataDeletionAge;

                etlProducerSettings.EtlFilePatterns = ApplicationTracesEtlFilePattern;
                etlProducerSettings.ServiceEtwManifests = new Dictionary<string, List<ServiceEtwManifestInfo>>(
                                                                        this.configReader.GetApplicationEtwManifests());
                etlProducerSettings.ProcessingWinFabEtlFiles = false;
                etlProducerSettings.AppEtwGuids = this.configReader.GetApplicationEtwProviderGuids();
                etlProducerSettings.ApplicationType = this.configReader.GetApplicationType();
#endif                
            }

            return etlProducerSettings.ToEtlProducerSettings();
        }

        private TimeSpan GetEtlReadIntervalFromApplicationManifest()
        {
            // Make the ETL read interval equal to the minimum upload interval among
            // all consumers.
            var etlReadInterval = EtlProducerConstants.DefaultEtlReadInterval;
            IEnumerable<string> consumers = this.configReader.GetConsumersForProducer(this.sectionName);
            foreach (string consumer in consumers)
            {
                bool consumerEnabled = this.configReader.GetUnencryptedConfigValue(
                                           consumer,
                                           EtlProducerValidator.EnabledParamName,
                                           false);
                if (false == consumerEnabled)
                {
                    continue;
                }

                var consumerUploadInterval = TimeSpan.FromMinutes(this.configReader.GetUnencryptedConfigValue(
                                                 consumer,
                                                 EtlProducerValidator.UploadIntervalParamName,
                                                 EtlProducerConstants.DefaultEtlReadInterval.TotalMinutes));
                if (consumerUploadInterval < etlReadInterval)
                {
                    etlReadInterval = consumerUploadInterval;
                }
            }

            return etlReadInterval;
        }

        internal class EtlProducerConfigReaderFactory : IEtlProducerConfigReaderFactory
        {
            private readonly ConfigReader configReader;
            private readonly string sectionName;

            public EtlProducerConfigReaderFactory(
                ConfigReader configReader,
                string sectionName)
            {
                this.configReader = configReader;
                this.sectionName = sectionName;
            }

            public IEtlProducerConfigReader CreateEtlProducerConfigReader(
                FabricEvents.ExtensionsEvents traceSource,
                string logSourceId)
            {
                return new EtlProducerConfigReader(
                    this.configReader,
                    this.sectionName,
                    traceSource,
                    logSourceId);
            }
        }

        private class EtlProducerSettingsBuilder
        {
            internal bool Enabled { get; set; }

            internal TimeSpan EtlReadInterval { get; set; }

            internal TimeSpan EtlDeletionAgeMinutes { get; set; }

            internal WinFabricEtlType ServiceFabricEtlType { get; set; }

            internal string EtlPath { get; set; }

            internal string EtlFilePatterns { get; set; }

            internal List<string> CustomManifestPaths { get; set; }

            internal Dictionary<string, List<ServiceEtwManifestInfo>> ServiceEtwManifests { get; set; }

            internal bool ProcessingWinFabEtlFiles { get; set; }

            internal IEnumerable<Guid> AppEtwGuids { get; set; }

            internal string ApplicationType { get; set; }

            public EtlProducerSettings ToEtlProducerSettings()
            {
                return new EtlProducerSettings(
                    this.Enabled,
                    this.EtlReadInterval,
                    this.EtlDeletionAgeMinutes,
                    this.ServiceFabricEtlType,
                    this.EtlPath,
                    this.EtlFilePatterns,
                    this.CustomManifestPaths != null ? this.CustomManifestPaths : new List<string>(),
                    this.ServiceEtwManifests != null ? this.ServiceEtwManifests : new Dictionary<string, List<ServiceEtwManifestInfo>>(),
                    this.ProcessingWinFabEtlFiles,
                    this.AppEtwGuids != null ? this.AppEtwGuids : new Guid[0],
                    this.ApplicationType);
            }
        }
    }
}
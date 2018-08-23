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

    internal class EtlInMemoryProducerConfigReader : IEtlInMemoryProducerConfigReader
    {
        private readonly ConfigReader configReader;
        private readonly string sectionName;
        private readonly FabricEvents.ExtensionsEvents traceSource;
        private readonly string logSourceId;

        public EtlInMemoryProducerConfigReader(
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

        public EtlInMemoryProducerSettings GetSettings()
        {
            return this.GetSettingsFromClusterManifest();
        }

        private EtlInMemoryProducerSettings GetSettingsFromClusterManifest()
        {
            var etlInMemoryProducerSettings = new EtlInMemoryProducerSettingsBuilder
            {
                Enabled = this.configReader.GetUnencryptedConfigValue(
                    this.sectionName,
                    EtlInMemoryProducerConstants.EnabledParamName,
                    EtlInMemoryProducerConstants.EtlProducerEnabledByDefault),
                ProcessingWinFabEtlFiles = true
            };

            // Check for values in settings.xml
            if (etlInMemoryProducerSettings.Enabled)
            {
                etlInMemoryProducerSettings.EtlReadInterval = TimeSpan.FromMinutes(this.configReader.GetUnencryptedConfigValue(
                                                                      this.sectionName,
                                                                      EtlInMemoryProducerConstants.EtlReadIntervalParamName,
                                                                      EtlInMemoryProducerConstants.DefaultEtlReadInterval.TotalMinutes));

                var etlDeletionAge = TimeSpan.FromDays(this.configReader.GetUnencryptedConfigValue(
                                             this.sectionName,
                                             EtlInMemoryProducerConstants.DataDeletionAgeParamName,
                                             EtlInMemoryProducerConstants.DefaultDataDeletionAge.TotalDays));
                if (etlDeletionAge > EtlInMemoryProducerConstants.MaxDataDeletionAge)
                {
                    this.traceSource.WriteWarning(
                        this.logSourceId,
                        "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                        etlDeletionAge,
                        this.sectionName,
                        EtlInMemoryProducerConstants.DataDeletionAgeParamName,
                        EtlInMemoryProducerConstants.MaxDataDeletionAge);
                    etlDeletionAge = EtlInMemoryProducerConstants.MaxDataDeletionAge;
                }

                etlInMemoryProducerSettings.EtlDeletionAgeMinutes = etlDeletionAge;

                // Check for test settings
                var logDeletionAgeTestValue = TimeSpan.FromMinutes(this.configReader.GetUnencryptedConfigValue(
                                                  this.sectionName,
                                                  EtlInMemoryProducerConstants.TestDataDeletionAgeParamName,
                                                  0));
                if (logDeletionAgeTestValue != TimeSpan.Zero)
                {
                    if (logDeletionAgeTestValue > EtlInMemoryProducerConstants.MaxDataDeletionAge)
                    {
                        this.traceSource.WriteWarning(
                            this.logSourceId,
                            "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                            logDeletionAgeTestValue,
                            this.sectionName,
                            EtlInMemoryProducerConstants.TestDataDeletionAgeParamName,
                            EtlInMemoryProducerConstants.MaxDataDeletionAge);
                        logDeletionAgeTestValue = EtlInMemoryProducerConstants.MaxDataDeletionAge;
                    }

                    etlInMemoryProducerSettings.EtlDeletionAgeMinutes = logDeletionAgeTestValue;
                }

                string etlTypeParamName = EtlInMemoryProducerConstants.WindowsFabricEtlTypeParamName;
                string serviceFabricEtlTypeString = this.configReader.GetUnencryptedConfigValue(
                                                 this.sectionName,
                                                 EtlInMemoryProducerConstants.WindowsFabricEtlTypeParamName,
                                                 EtlInMemoryProducerConstants.DefaultEtl);
                if (serviceFabricEtlTypeString.Equals(EtlInMemoryProducerConstants.DefaultEtl))
                {
                    serviceFabricEtlTypeString = this.configReader.GetUnencryptedConfigValue(
                                                 this.sectionName,
                                                 EtlInMemoryProducerConstants.ServiceFabricEtlTypeParamName,
                                                 EtlInMemoryProducerConstants.DefaultEtl);
                    etlTypeParamName = EtlInMemoryProducerConstants.ServiceFabricEtlTypeParamName;
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
                        EtlInMemoryProducerConstants.DefaultEtl);
                    parsedType = WinFabricEtlType.DefaultEtl;
                }

                etlInMemoryProducerSettings.ServiceFabricEtlType = parsedType;

                etlInMemoryProducerSettings.EtlPath = this.configReader.GetUnencryptedConfigValue(
                                                       this.sectionName,
                                                       EtlInMemoryProducerConstants.TestEtlPathParamName,
                                                       string.Empty);
                if (false == string.IsNullOrEmpty(etlInMemoryProducerSettings.EtlPath))
                {
                    etlInMemoryProducerSettings.EtlFilePatterns = this.configReader.GetUnencryptedConfigValue(
                                                                   this.sectionName,
                                                                   EtlInMemoryProducerConstants.TestEtlFilePatternsParamName,
                                                                   string.Empty);
                    if (string.IsNullOrEmpty(etlInMemoryProducerSettings.EtlFilePatterns))
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "The ETL path {0} specified for section {1}, parameter {2} is being ignored because parameter {3} has not been specified. The default ETL path will be used instead.",
                            etlInMemoryProducerSettings.EtlPath,
                            this.sectionName,
                            EtlInMemoryProducerConstants.TestEtlPathParamName,
                            EtlInMemoryProducerConstants.TestEtlFilePatternsParamName);
                        etlInMemoryProducerSettings.EtlPath = string.Empty;
                    }
                    else
                    {
                        etlInMemoryProducerSettings.ProcessingWinFabEtlFiles = this.configReader.GetUnencryptedConfigValue(
                                                                                this.sectionName,
                                                                                EtlInMemoryProducerConstants.TestProcessingWinFabEtlFilesParamName,
                                                                                false);
                    }
                }

                // Write settings to log
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "ETL processing enabled: ETL read interval (minutes): {0}, ETL deletion age ({1}): {2}",
                    etlInMemoryProducerSettings.EtlReadInterval.TotalMinutes,
                    (logDeletionAgeTestValue == TimeSpan.Zero) ? "days" : "minutes",
                    (logDeletionAgeTestValue == TimeSpan.Zero) ? etlDeletionAge : logDeletionAgeTestValue);

                if (false == string.IsNullOrEmpty(etlInMemoryProducerSettings.EtlPath))
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Custom ETL file info provided. Path: {0}, ETL file name patterns: {1}.",
                        etlInMemoryProducerSettings.EtlPath,
                        etlInMemoryProducerSettings.EtlFilePatterns);
                }
            }
            else
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "ETL processing not enabled");
            }

            return etlInMemoryProducerSettings.ToEtlInMemoryProducerSettings();
        }

        internal class EtlInMemoryProducerConfigReaderFactory : IEtlInMemoryProducerConfigReaderFactory
        {
            private readonly ConfigReader configReader;
            private readonly string sectionName;

            public EtlInMemoryProducerConfigReaderFactory(
                ConfigReader configReader,
                string sectionName)
            {
                this.configReader = configReader;
                this.sectionName = sectionName;
            }

            public IEtlInMemoryProducerConfigReader CreateEtlInMemoryProducerConfigReader(
                FabricEvents.ExtensionsEvents traceSource,
                string logSourceId)
            {
                return new EtlInMemoryProducerConfigReader(
                    this.configReader,
                    this.sectionName,
                    traceSource,
                    logSourceId);
            }
        }

        private class EtlInMemoryProducerSettingsBuilder
        {
            internal bool Enabled { get; set; }

            internal TimeSpan EtlReadInterval { get; set; }

            internal TimeSpan EtlDeletionAgeMinutes { get; set; }

            internal WinFabricEtlType ServiceFabricEtlType { get; set; }

            internal string EtlPath { get; set; }

            internal string EtlFilePatterns { get; set; }

            internal bool ProcessingWinFabEtlFiles { get; set; }

            public EtlInMemoryProducerSettings ToEtlInMemoryProducerSettings()
            {
                return new EtlInMemoryProducerSettings(
                    this.Enabled,
                    this.EtlReadInterval,
                    this.EtlDeletionAgeMinutes,
                    this.ServiceFabricEtlType,
                    this.EtlPath,
                    this.EtlFilePatterns,
                    this.ProcessingWinFabEtlFiles);
            }
        }
    }
}
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
    
    // Class that implements a producer that reads performance counter configuration from 
    // the cluster manifest and provides it to the consumers
    internal class PerfCounterConfigReader : IDcaProducer
    {
        // Constants
        private const string PerformanceCounterSectionName = "PerformanceCounterLocalStore";
        private const string IsEnabledParamName = "IsEnabled";
        private const string SamplingIntervalInSecondsParamName = "SamplingIntervalInSeconds";
        private const string CounterPathsParamName = "Counters";
        private const bool PerfCounterProducerEnabledByDefault = false;

        // Additional sections in the cluster manifest or application manifest from which
        // we read configuration information.
        private static readonly string[] PerfCounterAdditionalAppConfigSections = { PerformanceCounterSectionName };

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Configuration reader
        private readonly ConfigReader configReader;

        // Producer initialization parameters
        private ProducerInitializationParameters initParam;

        // Settings
        private ConfigReaderSettings configReaderSettings;

        // Performance counter configuration
        private PerformanceCounterConfiguration perfCounterConfig;

        public PerfCounterConfigReader(ProducerInitializationParameters initializationParameters)
        {
            // Initialization
            this.initParam = initializationParameters;
            this.logSourceId = string.Concat(this.initParam.ApplicationInstanceId, "_", this.initParam.SectionName);
            this.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            this.configReader = new ConfigReader(this.initParam.ApplicationInstanceId);

            // Read instance-specific settings from settings.xml
            this.GetSettings();
            if (false == this.configReaderSettings.Enabled)
            {
                // Producer is not enabled, so return immediately
                return;
            }

            // Read the performance counter configuration from settings.xml
            this.GetPerformanceCounterConfiguration();
            if (false == this.perfCounterConfig.IsEnabled)
            {
                // Performance counter collection is not enabled, so return immediately
                return;
            }

            if (null == initializationParameters.ConsumerSinks)
            {
                return;
            }

            foreach (object sinkAsObject in initializationParameters.ConsumerSinks)
            {
                IPerformanceCounterConfigurationSink perfCounterConfigSink = null;

                try
                {
                    perfCounterConfigSink = (IPerformanceCounterConfigurationSink)sinkAsObject;
                }
                catch (InvalidCastException e)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Exception occured while casting a sink object of type {0} to interface IPerformanceCounterConfigurationSink. Exception information: {1}.",
                        sinkAsObject.GetType(),
                        e);
                }

                if (null == perfCounterConfigSink)
                {
                    continue;
                }

                perfCounterConfigSink.RegisterPerformanceCounterConfiguration(
                    this.perfCounterConfig.SamplingInterval,
                    this.perfCounterConfig.CounterPaths);
            }
        }

        public IEnumerable<string> AdditionalAppConfigSections
        {
            get
            {
                return PerfCounterAdditionalAppConfigSections;
            }
        }

        // Sections in the service manifest from which we read configuration information.
        public IEnumerable<string> ServiceConfigSections
        {
            get
            {
                return null;
            }
        }

        public void FlushData()
        {
            // Not implemented
        }

        public void Dispose()
        {
            // Nothing to do here
            GC.SuppressFinalize(this);
        }

        private void GetSettings()
        {
            // Check for values in settings.xml
            this.configReaderSettings.Enabled = this.configReader.GetUnencryptedConfigValue(
                                                      this.initParam.SectionName,
                                                      PerfCounterProducerConstants.EnabledParamName,
                                                      PerfCounterProducerEnabledByDefault);
            if (this.configReaderSettings.Enabled)
            {
                // Write settings to log
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Performance counter configuration reader is enabled");
            }
            else
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Performance counter configuration reader is not enabled");
            }
        }

        private void GetPerformanceCounterConfiguration()
        {
            // Figure out if performance counter collection is enabled. By default
            // it is enabled and the 'IsEnabled' setting must be explicitly 
            // specified to disable it.
            this.perfCounterConfig.IsEnabled = this.configReader.GetUnencryptedConfigValue(
                                                   PerformanceCounterSectionName,
                                                   IsEnabledParamName,
                                                   true);
            if (this.perfCounterConfig.IsEnabled)
            {
                // Get the sampling interval
                this.perfCounterConfig.SamplingInterval = TimeSpan.FromSeconds(this.configReader.GetUnencryptedConfigValue(
                                                                       PerformanceCounterSectionName,
                                                                       SamplingIntervalInSecondsParamName,
                                                                       (int)PerformanceCounterCommon.DefaultSamplingInterval.TotalSeconds));

                // Get the counter paths
                string counterListAsString = this.configReader.GetUnencryptedConfigValue(
                                                 PerformanceCounterSectionName,
                                                 CounterPathsParamName,
                                                 string.Empty);

                if (string.IsNullOrEmpty(counterListAsString))
                {
                    counterListAsString = PerformanceCounterCommon.LabelDefault;
                }

                PerformanceCounterCommon.ParseCounterList(counterListAsString, out this.perfCounterConfig.CounterPaths);

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Performance counter data collection is enabled. Sampling interval: {0}.",
                    this.perfCounterConfig.SamplingInterval);
            }
            else
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Performance counter data collection has been disabled via the {0} parameter in the {1} section of the cluster manifest.",
                    IsEnabledParamName,
                    PerformanceCounterSectionName);
            }
        }

        // Settings related to perf counter config readers
        private struct ConfigReaderSettings
        {
            internal bool Enabled;
        }

        // Struct representing performance counter configuration
        private struct PerformanceCounterConfiguration
        {
            internal bool IsEnabled;
            internal TimeSpan SamplingInterval;
            internal HashSet<string> CounterPaths;
        }
    }
}
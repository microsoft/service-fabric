// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMonSvc
{
    using System;
    using System.Fabric;
    using System.Globalization;
    using Microsoft.ServiceFabric.Monitoring.Interfaces;
    using Microsoft.ServiceFabric.Monitoring.Internal;

    /// <summary>
    /// Wraps the static methods of ConfigReader to support DI
    /// </summary>
    internal class ServiceConfiguration : IInternalServiceConfiguration
    {
        private const string HealthDataProducerSectionName = "HealthDataProducer";
        private const string MdsHealthDataConsumerSectionName = "MdsHealthDataConsumer";
        private const string ConfigPackageName = "MonitoringService.Config";

        private readonly ICodePackageActivationContext activationContext;
        private readonly TraceWriterWrapper trace;

        private TimeSpan healthDataReadInterval;
        private TimeSpan healthDataReadTimeout;
        private TimeSpan healthQueryTimeout;
        private string clusterName;
        private string reportServiceHealth;
        private string reportPartitionHealth;
        private string reportReplicaHealth;
        private string reportDeployedApplicationHealth;
        private string reportServicePackageHealth;
        private string reportHealthEvents;
        private string serviceWhitelist;
        private string partitionWhitelist;
        private string replicaWhitelist;
        private string deployedApplicationWhitelist;
        private string servicePackageWhitelist;
        private string healthEventsWhitelist;
        private string mdmAccount;
        private string mdmNamespace;
        private string dataCenter;

        internal ServiceConfiguration(ICodePackageActivationContext activationContext, TraceWriterWrapper trace)
        {
            this.activationContext = Guard.IsNotNull(activationContext, nameof(activationContext));
            this.trace = Guard.IsNotNull(trace, nameof(trace));

            this.RefreshConfigurationCache();
        }

        TimeSpan IServiceConfiguration.HealthDataReadInterval
        {
            get { return this.healthDataReadInterval; }
        }

        TimeSpan IServiceConfiguration.HealthDataReadTimeout
        {
            get { return this.healthDataReadTimeout; }
        }

        TimeSpan IServiceConfiguration.HealthQueryTimeoutInSeconds
        {
            get { return this.healthQueryTimeout; }
        }

        string IServiceConfiguration.ClusterName
        {
            get { return this.clusterName; }
        }

        string IInternalServiceConfiguration.MdmAccountName
        {
            get { return this.mdmAccount; }
        }

        string IInternalServiceConfiguration.MdmNamespace
        {
            get { return this.mdmNamespace; }
        }

        string IInternalServiceConfiguration.DataCenter
        {
            get { return this.dataCenter; }
        }

        public string ReportServiceHealth
        {
            get
            {
                return this.reportServiceHealth;
            }
        }

        public string ReportPartitionHealth
        {
            get
            {
                return this.reportPartitionHealth;
            }
        }

        public string ReportReplicaHealth
        {
            get
            {
                return this.reportReplicaHealth;
            }
        }

        public string ReportDeployedApplicationHealth
        {
            get
            {
                return this.reportDeployedApplicationHealth;
            }
        }

        public string ReportServicePackageHealth
        {
            get
            {
                return this.reportServicePackageHealth;
            }
        }

        public string ReportHealthEvents
        {
            get
            {
                return this.reportHealthEvents;
            }
        }

        public string ApplicationsThatReportServiceHealth
        {
            get
            {
                return this.serviceWhitelist;
            }
        }

        public string ApplicationsThatReportPartitionHealth
        {
            get
            {
                return this.partitionWhitelist;
            }
        }

        public string ApplicationsThatReportReplicaHealth
        {
            get
            {
                return this.replicaWhitelist;
            }
        }

        public string ApplicationsThatReportDeployedApplicationHealth
        {
            get
            {
                return this.deployedApplicationWhitelist;
            }
        }

        public string ApplicationsThatReportServicePackageHealth
        {
            get
            {
                return this.servicePackageWhitelist;
            }
        }

        public string ApplicationsThatReportHealthEvents
        {
            get
            {
                return this.healthEventsWhitelist;
            }
        }

        void IServiceConfiguration.Refresh()
        {
            this.RefreshConfigurationCache();
        }

        private void RefreshConfigurationCache()
        {
            this.healthDataReadInterval = TimeSpan.FromMinutes(
                this.GetProducerConfigValue("HealthDataReadIntervalInMinutes", defaultValue: 5));
            this.healthDataReadTimeout = TimeSpan.FromSeconds(
                this.GetProducerConfigValue("HealthDataReadTimeoutInSeconds", defaultValue: 60));
            this.healthQueryTimeout = TimeSpan.FromSeconds(
                this.GetProducerConfigValue("HealthQueryTimeoutInSeconds", defaultValue: 30));
            this.clusterName = this.GetProducerConfigValue("ClusterName", defaultValue: "ClusterNameUnknown");

            this.reportServiceHealth = this.GetProducerConfigValue("ReportServiceHealth", defaultValue: string.Empty);
            this.reportPartitionHealth = this.GetProducerConfigValue("ReportPartitionHealth", defaultValue: string.Empty);
            this.reportReplicaHealth = this.GetProducerConfigValue("ReportReplicaHealth", defaultValue: string.Empty);
            this.reportDeployedApplicationHealth = this.GetProducerConfigValue("ReportDeployedApplicationHealth", defaultValue: string.Empty);
            this.reportServicePackageHealth = this.GetProducerConfigValue("ReportServicePackageHealth", defaultValue: string.Empty);
            this.reportHealthEvents = this.GetProducerConfigValue("ReportHealthEvents", defaultValue: string.Empty);

            this.serviceWhitelist = this.GetProducerConfigValue("ApplicationsThatReportServiceHealth", defaultValue: string.Empty);
            this.partitionWhitelist = this.GetProducerConfigValue("ApplicationsThatReportPartitionHealth", defaultValue: string.Empty);
            this.replicaWhitelist = this.GetProducerConfigValue("ApplicationsThatReportReplicaHealth", defaultValue: string.Empty);
            this.deployedApplicationWhitelist = this.GetProducerConfigValue("ApplicationsThatReportDeployedApplicationHealth", defaultValue: string.Empty);
            this.servicePackageWhitelist = this.GetProducerConfigValue("ApplicationsThatReportServicePackageHealth", defaultValue: string.Empty);
            this.healthEventsWhitelist = this.GetProducerConfigValue("ApplicationsThatReportHealthEvents", defaultValue: string.Empty);

            this.mdmAccount = this.GetConsumerConfigValue("MdmAccountName", defaultValue: string.Empty);
            this.mdmNamespace = this.GetConsumerConfigValue("MdmNamespace", defaultValue: string.Empty);
            this.dataCenter = this.GetConsumerConfigValue("DataCenter", defaultValue: string.Empty);
        }

        private TValue GetConsumerConfigValue<TValue>(string paramName, TValue defaultValue)
        {
            return this.GetConfigValue(MdsHealthDataConsumerSectionName, paramName, defaultValue);
        }

        private TValue GetProducerConfigValue<TValue>(string paramName, TValue defaultValue)
        {
            return this.GetConfigValue(HealthDataProducerSectionName, paramName, defaultValue);
        }

        private T GetConfigValue<T>(string sectionName, string parameterName, T defaultValue)
        {
            T value = defaultValue;

            var configPackage = this.activationContext.GetConfigurationPackageObject(ConfigPackageName);
            if (configPackage == null)
            {
                return defaultValue;
            }

            var configSettings = configPackage.Settings;
            if (configSettings == null
                || configSettings.Sections == null
                || !configSettings.Sections.Contains(sectionName))
            {
                return defaultValue;
            }

            var section = configSettings.Sections[sectionName];
            if (section.Parameters == null
                || !section.Parameters.Contains(parameterName))
            {
                return defaultValue;
            }

            var property = section.Parameters[parameterName];
            try
            {
                value = (T)Convert.ChangeType(property.Value, typeof(T), CultureInfo.InvariantCulture);
            }
            catch (Exception e)
            {
                this.trace.WriteError(
                    "Exception occurred while reading configuration from section {0}, parameter {1}. Exception: {2}",
                    sectionName,
                    parameterName,
                    e);
            }

            return value;
        }
    }
}
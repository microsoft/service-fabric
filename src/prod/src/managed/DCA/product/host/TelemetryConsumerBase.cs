// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Fabric.Dca;
    using TelemetryAggregation;

    internal abstract class TelemetryConsumerBase : IDcaConsumer
    {
        protected const string TraceType = "TelemetryConsumer";
        private readonly TelemetryEtwSink etwSink;
        private bool disposed;

        public TelemetryConsumerBase(string applicationInstanceId)
        {
            // reading from Diagnostic section the ClusterId and whether telemetry is enabled
            ConfigReader configReader = new ConfigReader(applicationInstanceId);

            bool enableTelemetry = TelemetryConsumerBase.ParseBoolString(
                configReader.GetUnencryptedConfigValue(
                          ConfigReader.DiagnosticsSectionName,
                          ConfigReader.EnableTelemetryParamName,
                          "true"),
                true);

            if (enableTelemetry)
            {
                string clusterId;
                string tenantId;
                string clusterType;

                this.GetClusterIdAndType(configReader, out clusterId, out tenantId, out clusterType);

                string serviceFabricVersion = this.GetServiceFabricVersion() ?? TelemetryConstants.Undefined;

                bool isScaleMin = TelemetryConsumerBase.ParseBoolString(
                    configReader.GetUnencryptedConfigValue(
                              ConfigReader.FabricNodeSectionName,
                              ConfigReader.IsScaleMinParamName,
                              "false"),
                    false);

                string nodeName = configReader.GetUnencryptedConfigValue(
                              ConfigReader.FabricNodeSectionName,
                              ConfigReader.InstanceNameParamName,
                              "undefined");

                try
                {
                    ITelemetryWriter telemetryWriter = TelemetryWriterFactory.CreateTelemetryWriter(configReader, clusterId, nodeName, Utility.TraceSource.WriteError);
                    this.etwSink = new TelemetryEtwSink(telemetryWriter, clusterId, tenantId, clusterType, serviceFabricVersion, nodeName, isScaleMin, new TelemetryScheduler(configReader));
                }
                catch (Exception e)
                {
                    Utility.TraceSource.WriteExceptionAsError(
                        TraceType,
                        e,
                        "Failed to create TelemetryEtwSink");
                }
            }
            
            this.disposed = false;
        }
        
        public object GetDataSink()
        {
            // if telemetry is disabled it returns null
            return this.etwSink;      
        }

        public void FlushData()
        {
            return;
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            // etwSink is null in case telemetry is disabled
            if (null != this.etwSink)
            {
                this.etwSink.Dispose();
            }
        }

        protected abstract string GetServiceFabricVersion();

        protected abstract string GetTenantId();

        private static bool ParseBoolString(string paramValue, bool defaultValue)
        {
            // parsing bool from paramValue
            bool isEnabled;
            if (!bool.TryParse(paramValue, out isEnabled))
            {
                return defaultValue;
            }

            return isEnabled;
        }

        private void GetClusterIdAndType(ConfigReader configReader, out string clusterId, out string tenantId, out string clusterType)
        {
            // first try to identify SFRP Clusters which have a ClusterId parameter under the Paas section
            string paasClusterId = configReader.GetUnencryptedConfigValue(
                ConfigReader.PaasSectionName,
                ConfigReader.ClusterIdParamName,
                (string)null);

            // Get tenantId for PaasV1 clusters or SFRP
            tenantId = this.GetTenantId() ?? TelemetryConstants.Undefined;

            if (null != paasClusterId)
            {
                clusterId = paasClusterId;
                clusterType = TelemetryConstants.ClusterTypeSfrp;
                return;
            }

            // try to find TenantId in Registry for PaasV1 clusters
            if (TelemetryConstants.Undefined != tenantId)
            {
                clusterId = tenantId;
                clusterType = TelemetryConstants.ClusterTypePaasV1;
                return;
            }

            // try to find ClusterId under Diagnostics section for Standalone Clusters
            string diagnosticsClusterId = configReader.GetUnencryptedConfigValue(
                    ConfigReader.DiagnosticsSectionName,
                    ConfigReader.ClusterIdParamName,
                    (string)null);

            if (null != diagnosticsClusterId)
            {
                clusterId = diagnosticsClusterId;
                clusterType = TelemetryConstants.ClusterTypeStandalone;
                return;
            }

            // Cluster id and type undefined in case it does not match any of the conditions above
            clusterId = TelemetryConstants.Undefined;
            clusterType = TelemetryConstants.Undefined;
        }

        private static class TelemetryConstants
        {
            public const string Undefined = "undefined";
            public const string ClusterTypeStandalone = "standalone";
            public const string ClusterTypeSfrp = "SFRP";
            public const string ClusterTypePaasV1 = "PaasV1";
        }
    }
}
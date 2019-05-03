// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace MdsHealthDataConsumer
{
    using FabricMonSvc;
    using Microsoft.Cloud.InstrumentationFramework;
    using Microsoft.ServiceFabric.Monitoring.Health;
    using Microsoft.ServiceFabric.Monitoring.Internal;

    /// <summary>
    /// Encapsulate the <see cref="Microsoft.Cloud.InstrumentationFramework"/> API to emit ServiceFabric metrics to MDM hot path.
    /// </summary>
    internal class MetricsEmitter
    {
        private const string ClusterNameDimension = "ClusterName";
        private const string AppNameDimension = "AppName";
        private const string NodeNameDimension = "NodeName";
        private const string ServiceNameDimension = "ServiceName";
        private const string PartitionIdDimension = "PartitionId";
        private const string ReplicaIdDimension = "ReplicaId";
        private const string SourceDimension = "Source";
        private const string PropertyDimension = "Property";
        private const string IsExpiredDimension = "IsExpired";
        private const string HealthStateDimension = "HealthState";

        private readonly IInternalServiceConfiguration config;
        private readonly TraceWriterWrapper trace;

        private bool isMetricsEnabled = true;

        private MeasureMetric2D clusterHealthMetric;
        private MeasureMetric3D appHealthMetric;
        private MeasureMetric3D nodeHealthMetric;
        private MeasureMetric4D serviceHealthMetric;
        private MeasureMetric5D partitionHealthMetric;
        private MeasureMetric6D replicaHealthMetric;
        private MeasureMetric4D deployedAppHealthMetric;
        private MeasureMetric5D deployedServicePackageHealthMetric;

        private MeasureMetric5D clusterHealthEventMetric;
        private MeasureMetric6D appHealthEventMetric;
        private MeasureMetric6D nodeHealthEventMetric;
        private MeasureMetric7D serviceHealthEventMetric;
        private MeasureMetric8D partitionHealthEventMetric;
        private MeasureMetric9D replicaHealthEventMetric;
        private MeasureMetric7D deployedAppHealthEventMetric;
        private MeasureMetric8D deployedServicePackageHealthEventMetric;

        internal MetricsEmitter(IInternalServiceConfiguration config, TraceWriterWrapper traceWriter)
        {
            this.config = Guard.IsNotNull(config, nameof(config));
            this.trace = Guard.IsNotNull(traceWriter, nameof(traceWriter));

            var mdmAccount = config.MdmAccountName;
            var mdmNamespace = config.MdmNamespace;

            if (string.IsNullOrEmpty(mdmAccount) || string.IsNullOrEmpty(mdmNamespace))
            {
                this.trace.WriteError("MetricsEmitter: MdmAccount or Namespace value is empty in configuration. Disabling metrics.");
                this.isMetricsEnabled = false;
                return;
            }

            ErrorContext mdmError = new ErrorContext();
            DefaultConfiguration.SetDefaultDimensionNamesValues(
                ref mdmError, 
                1, 
                new[] { "DataCenter" }, 
                new[] { this.config.DataCenter });

            this.clusterHealthMetric = MeasureMetric2D.Create(mdmAccount, mdmNamespace, "ClusterHealthState", ClusterNameDimension, HealthStateDimension);
            this.appHealthMetric = MeasureMetric3D.Create(mdmAccount, mdmNamespace, "AppHealthState", ClusterNameDimension, AppNameDimension, HealthStateDimension);
            this.nodeHealthMetric = MeasureMetric3D.Create(mdmAccount, mdmNamespace, "NodeHealthState", ClusterNameDimension, NodeNameDimension, HealthStateDimension);
            this.serviceHealthMetric = MeasureMetric4D.Create(mdmAccount, mdmNamespace, "ServiceHealthState", ClusterNameDimension, AppNameDimension, ServiceNameDimension, HealthStateDimension);
            this.partitionHealthMetric = MeasureMetric5D.Create(mdmAccount, mdmNamespace, "PartitionHealthState", ClusterNameDimension, AppNameDimension, ServiceNameDimension, PartitionIdDimension, HealthStateDimension);
            this.replicaHealthMetric = MeasureMetric6D.Create(mdmAccount, mdmNamespace, "ReplicaHealthState", ClusterNameDimension, AppNameDimension, ServiceNameDimension, PartitionIdDimension, ReplicaIdDimension, HealthStateDimension);
            this.deployedAppHealthMetric = MeasureMetric4D.Create(mdmAccount, mdmNamespace, "DeployedApplicationHealthState", ClusterNameDimension, AppNameDimension, NodeNameDimension, HealthStateDimension);
            this.deployedServicePackageHealthMetric = MeasureMetric5D.Create(mdmAccount, mdmNamespace, "DeployedServicePackageHealthState", ClusterNameDimension, AppNameDimension, ServiceNameDimension, NodeNameDimension, HealthStateDimension);

            this.clusterHealthEventMetric = MeasureMetric5D.Create(mdmAccount, mdmNamespace, "ClusterHealthEvent", ClusterNameDimension, HealthStateDimension, SourceDimension, PropertyDimension, IsExpiredDimension);
            this.appHealthEventMetric = MeasureMetric6D.Create(mdmAccount, mdmNamespace, "AppHealthEvent", ClusterNameDimension, AppNameDimension, HealthStateDimension, SourceDimension, PropertyDimension, IsExpiredDimension);
            this.nodeHealthEventMetric = MeasureMetric6D.Create(mdmAccount, mdmNamespace, "NodeHealthEvent", ClusterNameDimension, NodeNameDimension, HealthStateDimension, SourceDimension, PropertyDimension, IsExpiredDimension);
            this.serviceHealthEventMetric = MeasureMetric7D.Create(mdmAccount, mdmNamespace, "ServiceHealthEvent", ClusterNameDimension, AppNameDimension, ServiceNameDimension, HealthStateDimension, SourceDimension, PropertyDimension, IsExpiredDimension);
            this.partitionHealthEventMetric = MeasureMetric8D.Create(mdmAccount, mdmNamespace, "PartitionHealthEvent", ClusterNameDimension, AppNameDimension, ServiceNameDimension, PartitionIdDimension, HealthStateDimension, SourceDimension, PropertyDimension, IsExpiredDimension);
            this.replicaHealthEventMetric = MeasureMetric9D.Create(mdmAccount, mdmNamespace, "ReplicaHealthEvent", ClusterNameDimension, AppNameDimension, ServiceNameDimension, PartitionIdDimension, ReplicaIdDimension, HealthStateDimension, SourceDimension, PropertyDimension, IsExpiredDimension);
            this.deployedAppHealthEventMetric = MeasureMetric7D.Create(mdmAccount, mdmNamespace, "DeployedApplicationHealthEvent", ClusterNameDimension, AppNameDimension, NodeNameDimension, HealthStateDimension, SourceDimension, PropertyDimension, IsExpiredDimension);
            this.deployedServicePackageHealthEventMetric = MeasureMetric8D.Create(mdmAccount, mdmNamespace, "DeployedServicePackageHealthEvent", ClusterNameDimension, AppNameDimension, ServiceNameDimension, NodeNameDimension, HealthStateDimension, SourceDimension, PropertyDimension, IsExpiredDimension);

            this.trace.WriteInfo("MetricsEmitter: Initialized Service Fabric MDM metrics. AccountName: {0}, namespace: {1}", mdmAccount, mdmNamespace);
        }

        #region HealthStateMetrics
        public virtual void EmitClusterHealthState(ClusterEntity cluster)
        {
            if (!this.isMetricsEnabled)
            {
                return;
            }

            this.clusterHealthMetric.LogValue(
                1, 
                cluster.ClusterName, 
                cluster.Health.AggregatedHealthState.ToString());
        }

        public virtual void EmitApplicationHealthState(ApplicationEntity application)
        {
            if (!this.isMetricsEnabled)
            {
                return;
            }

            this.appHealthMetric.LogValue(
                1, 
                application.ClusterName, 
                application.ApplicationName, 
                application.Health.AggregatedHealthState.ToString());
        }

        public virtual void EmitNodeHealthState(NodeEntity node)
        {
            if (!this.isMetricsEnabled)
            {
                return;
            }

            this.nodeHealthMetric.LogValue(
                1, 
                node.ClusterName, 
                node.NodeName, 
                node.Health.AggregatedHealthState.ToString());
        }

        public virtual void EmitServiceHealthState(ServiceEntity service)
        {
            if (!this.isMetricsEnabled)
            {
                return;
            }

            this.serviceHealthMetric.LogValue(
                1, 
                service.ClusterName, 
                service.ApplicationName, 
                service.ServiceName, 
                service.Health.AggregatedHealthState.ToString());
        }

        public virtual void EmitPartitionHealthState(PartitionEntity partition)
        {
            if (!this.isMetricsEnabled)
            {
                return;
            }

            this.partitionHealthMetric.LogValue(
                1, 
                partition.ClusterName, 
                partition.ApplicationName, 
                partition.ServiceName, 
                partition.PartitionId.ToString(), 
                partition.Health.AggregatedHealthState.ToString());
        }

        public virtual void EmitReplicaHealthState(ReplicaEntity replica)
        {
            if (!this.isMetricsEnabled)
            {
                return;
            }

            this.replicaHealthMetric.LogValue(
                1, 
                replica.ClusterName, 
                replica.ApplicationName, 
                replica.ServiceName, 
                replica.PartitionId.ToString(), 
                replica.ReplicaId.ToString(), 
                replica.Health.AggregatedHealthState.ToString());
        }

        public virtual void EmitDeployedAppHealthState(
            DeployedApplicationEntity deployedApplication)
        {
            if (!this.isMetricsEnabled)
            {
                return;
            }

            this.deployedAppHealthMetric.LogValue(
                1, 
                deployedApplication.ClusterName, 
                deployedApplication.ApplicationName, 
                deployedApplication.NodeName, 
                deployedApplication.Health.AggregatedHealthState.ToString());
        }

        public virtual void EmitDeployedServicePackageHealthState(
            DeployedServicePackageEntity deployedServicePackage)
        {
            if (!this.isMetricsEnabled)
            {
                return;
            }

            this.deployedServicePackageHealthMetric.LogValue(
                1, 
                deployedServicePackage.ClusterName, 
                deployedServicePackage.ApplicationName, 
                deployedServicePackage.ServiceManifestName, 
                deployedServicePackage.NodeName, 
                deployedServicePackage.Health.AggregatedHealthState.ToString());
        }
        #endregion

        #region HealthEventMetrics
        public virtual void EmitClusterHealthEvent(
            ClusterEntity cluster, EntityHealthEvent healthEvent)
        {
            if (!this.isMetricsEnabled)
            {
                return;
            }

            this.clusterHealthEventMetric.LogValue(
                1,
                cluster.ClusterName,
                healthEvent.HealthState.ToString(),
                healthEvent.SourceId,
                healthEvent.Property,
                healthEvent.IsExpired.ToString());
        }

        public virtual void EmitAppHealthEvent(
            ApplicationEntity app, EntityHealthEvent healthEvent)
        {
            if (!this.isMetricsEnabled)
            {
                return;
            }

            this.appHealthEventMetric.LogValue(
                1,
                app.ClusterName,
                app.ApplicationName,
                healthEvent.HealthState.ToString(),
                healthEvent.SourceId,
                healthEvent.Property,
                healthEvent.IsExpired.ToString());
        }

        public virtual void EmitNodeHealthEvent(
            NodeEntity node, EntityHealthEvent healthEvent)
        {
            if (!this.isMetricsEnabled)
            {
                return;
            }

            this.nodeHealthEventMetric.LogValue(
                1,
                node.ClusterName,
                node.NodeName,
                healthEvent.HealthState.ToString(),
                healthEvent.SourceId,
                healthEvent.Property,
                healthEvent.IsExpired.ToString());
        }

        public virtual void EmitServiceHealthEvent(
            ServiceEntity service, EntityHealthEvent healthEvent)
        {
            if (!this.isMetricsEnabled)
            {
                return;
            }

            this.serviceHealthEventMetric.LogValue(
                1,
                service.ClusterName,
                service.ApplicationName,
                service.ServiceName,
                healthEvent.HealthState.ToString(),
                healthEvent.SourceId,
                healthEvent.Property,
                healthEvent.IsExpired.ToString());
        }

        public virtual void EmitPartitionHealthEvent(
            PartitionEntity partition, EntityHealthEvent healthEvent)
        {
            if (!this.isMetricsEnabled)
            {
                return;
            }

            this.partitionHealthEventMetric.LogValue(
                1,
                partition.ClusterName,
                partition.ApplicationName,
                partition.ServiceName,
                partition.PartitionId.ToString(),
                healthEvent.HealthState.ToString(),
                healthEvent.SourceId,
                healthEvent.Property,
                healthEvent.IsExpired.ToString());
        }

        public virtual void EmitReplicaHealthEvent(
            ReplicaEntity replica, EntityHealthEvent healthEvent)
        {
            if (!this.isMetricsEnabled)
            {
                return;
            }

            this.replicaHealthEventMetric.LogValue(
                1,
                replica.ClusterName,
                replica.ApplicationName,
                replica.ServiceName,
                replica.PartitionId.ToString(),
                replica.ReplicaId.ToString(),
                healthEvent.HealthState.ToString(),
                healthEvent.SourceId,
                healthEvent.Property,
                healthEvent.IsExpired.ToString());
        }

        public virtual void EmitDeployedAppHealthEvent(
            DeployedApplicationEntity deployedApp, EntityHealthEvent healthEvent)
        {
            if (!this.isMetricsEnabled)
            {
                return;
            }

            this.deployedAppHealthEventMetric.LogValue(
                1,
                deployedApp.ClusterName,
                deployedApp.ApplicationName,
                deployedApp.NodeName,
                healthEvent.HealthState.ToString(),
                healthEvent.SourceId,
                healthEvent.Property,
                healthEvent.IsExpired.ToString());
        }

        public virtual void EmitDeployedServicePackageHealthEvent(
            DeployedServicePackageEntity deployedServicePackage, EntityHealthEvent healthEvent)
        {
            if (!this.isMetricsEnabled)
            {
                return;
            }

            this.deployedServicePackageHealthEventMetric.LogValue(
                1,
                deployedServicePackage.ClusterName,
                deployedServicePackage.ApplicationName,
                deployedServicePackage.ServiceManifestName,
                deployedServicePackage.NodeName,
                healthEvent.HealthState.ToString(),
                healthEvent.SourceId,
                healthEvent.Property,
                healthEvent.IsExpired.ToString());
        }
        #endregion
    }
}
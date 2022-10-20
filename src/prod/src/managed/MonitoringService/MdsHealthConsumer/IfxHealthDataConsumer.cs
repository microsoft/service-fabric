// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace MdsHealthDataConsumer
{
    using System.Diagnostics.CodeAnalysis;
    using System.Linq;
    using System.Threading.Tasks;
    using FabricMonSvc;
    using Microsoft.ServiceFabric.Monitoring.Health;
    using Microsoft.ServiceFabric.Monitoring.Internal.EventSource;

    /// <summary>
    /// Concrete implementation of HealthDataConsumer which uses <see cref="Microsoft.Cloud.InstrumentationFramework"/> to upload health data to MDM and MDS pipelines of Geneva.
    /// </summary>
    internal class IfxHealthDataConsumer : HealthDataConsumer
    {
        private readonly Task completedTask = Task.FromResult(0);
        private readonly MonitoringEventWriter eventWriter;
        private readonly MetricsEmitter metrics;

        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "This rule breaks for nameof() operator.")]
        public IfxHealthDataConsumer(MonitoringEventWriter eventWriter, MetricsEmitter metrics)
        {
            this.eventWriter = Guard.IsNotNull(eventWriter, nameof(eventWriter));
            this.metrics = Guard.IsNotNull(metrics, nameof(metrics));
        }

        public override Task ProcessApplicationHealthAsync(ApplicationEntity application)
        {
            this.eventWriter.LogApplicationHealthState(application);
            this.metrics.EmitApplicationHealthState(application);

            if (application.IsHealthEventReportingEnabled(
                application.ApplicationName, 
                application.Health.AggregatedHealthState))
            {
                application.Health.HealthEvents
                    .ForEachHealthEvent(healthEvent =>
                        {
                            this.eventWriter.LogApplicationHealthEvent(application, healthEvent);
                        });
            }

            return this.completedTask;
        }

        public override Task ProcessClusterHealthAsync(ClusterEntity cluster)
        {
            this.eventWriter.LogClusterHealthState(cluster);
            this.metrics.EmitClusterHealthState(cluster);

            cluster.Health.HealthEvents
                .ForEachHealthEvent(healthEvent =>
                    {
                        this.eventWriter.LogClusterHealthEvent(cluster, healthEvent);
                    });

            return this.completedTask;
        }

        public override Task ProcessDeployedApplicationHealthAsync(DeployedApplicationEntity deployedApplication)
        {
            this.eventWriter.LogDeployedApplicationHealthState(deployedApplication);
            this.metrics.EmitDeployedAppHealthState(deployedApplication);

            if (deployedApplication.IsHealthEventReportingEnabled(
                deployedApplication.ApplicationName,
                deployedApplication.Health.AggregatedHealthState))
            {
                deployedApplication.Health.HealthEvents
                .ForEachHealthEvent(healthEvent =>
                    {
                        this.eventWriter.LogDeployedApplicationHealthEvent(deployedApplication, healthEvent);
                    });
            }

            return this.completedTask;
        }

        public override Task ProcessDeployedServicePackageHealthAsync(DeployedServicePackageEntity deployedServicePackage)
        {
            this.eventWriter.LogDeployedServicePackageHealthState(deployedServicePackage);
            this.metrics.EmitDeployedServicePackageHealthState(deployedServicePackage);

            if (deployedServicePackage.IsHealthEventReportingEnabled(
                deployedServicePackage.ApplicationName,
                deployedServicePackage.Health.AggregatedHealthState))
            {
                deployedServicePackage.Health.HealthEvents
                    .ForEachHealthEvent(healthEvent =>
                        {
                            this.eventWriter.LogDeployedServicePackageHealthEvent(deployedServicePackage, healthEvent);
                        });
            }

            return this.completedTask;
        }

        public override Task ProcessNodeHealthAsync(NodeEntity node)
        {
            this.eventWriter.LogNodeHealthState(node);
            this.metrics.EmitNodeHealthState(node);

            node.Health.HealthEvents
                .ForEachHealthEvent(healthEvent =>
                    {
                        this.eventWriter.LogNodeHealthEvent(node, healthEvent);
                    });

            return this.completedTask;
        }

        public override Task ProcessPartitionHealthAsync(PartitionEntity partition)
        {
            this.eventWriter.LogPartitionHealthState(partition);
            this.metrics.EmitPartitionHealthState(partition);

            if (partition.IsHealthEventReportingEnabled(
                partition.ApplicationName,
                partition.Health.AggregatedHealthState))
            {
                partition.Health.HealthEvents
                .ForEachHealthEvent(healthEvent =>
                        {
                            this.eventWriter.LogPartitionHealthEvent(partition, healthEvent);
                        });
            }

            return this.completedTask;
        }

        public override Task ProcessReplicaHealthAsync(ReplicaEntity replica)
        {
            this.eventWriter.LogReplicaHealthState(replica);
            this.metrics.EmitReplicaHealthState(replica);

            if (replica.IsHealthEventReportingEnabled(
                replica.ApplicationName,
                replica.Health.AggregatedHealthState))
            {
                replica.Health.HealthEvents
                    .ForEachHealthEvent(healthEvent =>
                        {
                            this.eventWriter.LogReplicaHealthEvent(replica, healthEvent);
                        });
            }

            return this.completedTask;
        }

        public override Task ProcessServiceHealthAsync(ServiceEntity service)
        {
            this.eventWriter.LogServiceHealthState(service);
            this.metrics.EmitServiceHealthState(service);

            if (service.IsHealthEventReportingEnabled(
                service.ApplicationName,
                service.Health.AggregatedHealthState))
            {
                service.Health.HealthEvents
                    .ForEachHealthEvent(healthEvent =>
                        {
                            this.eventWriter.LogServiceHealthEvent(service, healthEvent);
                        });
            }

            return this.completedTask;
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Internal.Test
{
    using System.IO;
    using FabricMonSvc;
    using Health;
    using MdsHealthDataConsumer;

    internal class FakeMetricsEmitter : MetricsEmitter
    {
        private const string MetricsFileName = @"e:\temp\metrics.txt";

        public FakeMetricsEmitter(IInternalServiceConfiguration config, TraceWriterWrapper traceWriter) 
            : base(config, traceWriter)
        {
        }

        #region HealthStateMetrics
        public override void EmitClusterHealthState(ClusterEntity cluster)
        {
            AppendToFile(
                string.Format(
                    "clusterHealthMetric: ClusterName: {0}, HealthState: {1}",
                    cluster.ClusterName,
                    cluster.Health.AggregatedHealthState.ToString()));
        }

        public override void EmitApplicationHealthState(ApplicationEntity application)
        {
            AppendToFile(
               string.Format(
                   "appHealthMetric: ClusterName: {0}, AppName: {2}, HealthState: {1}",
                   application.ClusterName,
                   application.Health.AggregatedHealthState.ToString(),
                   application.ApplicationName));
        }

        public override void EmitNodeHealthState(NodeEntity node)
        {
            AppendToFile(
               string.Format(
                   "nodeHealthMetric: ClusterName: {0}, NodeName: {2}, HealthState: {1}",
                   node.ClusterName,
                   node.Health.AggregatedHealthState.ToString(),
                   node.NodeName));
        }

        public override void EmitServiceHealthState(ServiceEntity service)
        {
            AppendToFile(
               string.Format(
                   "serviceHealthMetric: ClusterName: {0}, AppName: {2}, ServiceName: {3}, HealthState: {1}",
                   service.ClusterName,
                   service.Health.AggregatedHealthState.ToString(),
                   service.ApplicationName,
                   service.ServiceName));
        }

        public override void EmitPartitionHealthState(PartitionEntity partition)
        {
            AppendToFile(
               string.Format(
                   "partitionHealthMetric: ClusterName: {0}, AppName: {2}, ServiceName: {3}, PartitionId: {4}, HealthState: {1}",
                   partition.ClusterName,
                   partition.Health.AggregatedHealthState.ToString(),
                   partition.ApplicationName,
                   partition.ServiceName,
                   partition.PartitionId));
        }

        public override void EmitReplicaHealthState(ReplicaEntity replica)
        {
            AppendToFile(
               string.Format(
                   "replicaHealthMetric: ClusterName: {0}, AppName: {2}, ServiceName: {3}, PartitionId: {4}, ReplicaId: {5}, HealthState: {1}",
                   replica.ClusterName,
                   replica.Health.AggregatedHealthState.ToString(),
                   replica.ApplicationName,
                   replica.ServiceName,
                   replica.PartitionId,
                   replica.ReplicaId));
        }

        public override void EmitDeployedAppHealthState(
            DeployedApplicationEntity deployedApplication)
        {
            AppendToFile(
               string.Format(
                   "deployedAppHealthMetric: ClusterName: {0}, AppName: {2}, NodeName: {3}, HealthState: {1}",
                   deployedApplication.ClusterName,
                   deployedApplication.Health.AggregatedHealthState.ToString(),
                   deployedApplication.ApplicationName,
                   deployedApplication.NodeName));
        }

        public override void EmitDeployedServicePackageHealthState(
            DeployedServicePackageEntity deployedServicePackage)
        {
            AppendToFile(
               string.Format(
                   "deployedServicePackageHealthMetric: ClusterName: {0}, AppName: {2}, ServiceManifestName: {3}, NodeName: {4}, HealthState: {1}",
                   deployedServicePackage.ClusterName,
                   deployedServicePackage.Health.AggregatedHealthState.ToString(),
                   deployedServicePackage.ApplicationName,
                   deployedServicePackage.ServiceManifestName,
                   deployedServicePackage.NodeName));
        }
        #endregion

        #region HealthEventMetrics
        public override void EmitClusterHealthEvent(
            ClusterEntity cluster, EntityHealthEvent healthEvent)
        {
            AppendHealthEventToFile(
                string.Format(
                    "clusterHealthEventMetric: ClusterName: {0}",
                    cluster.ClusterName),
                healthEvent);
        }

        public override void EmitAppHealthEvent(
            ApplicationEntity app, EntityHealthEvent healthEvent)
        {
            AppendHealthEventToFile(
                string.Format(
                    "appHealthEventMetric: ClusterName: {0}, AppName: {1}",
                    app.ClusterName,
                    app.ApplicationName),
                healthEvent);
        }

        public override void EmitNodeHealthEvent(
            NodeEntity node, EntityHealthEvent healthEvent)
        {
            AppendHealthEventToFile(
                string.Format(
                    "nodeHealthEventMetric: ClusterName: {0}, NodeName: {1}",
                    node.ClusterName,
                    node.NodeName),
                healthEvent);
        }

        public override void EmitServiceHealthEvent(
            ServiceEntity service, EntityHealthEvent healthEvent)
        {
            AppendHealthEventToFile(
                string.Format(
                    "serviceHealthEventMetric: ClusterName: {0}, AppName: {1}, ServiceName: {2}",
                    service.ClusterName,
                    service.ApplicationName,
                    service.ServiceName),
                healthEvent);
        }

        public override void EmitPartitionHealthEvent(
            PartitionEntity partition, EntityHealthEvent healthEvent)
        {
            AppendHealthEventToFile(
                string.Format(
                    "partitionHealthEventMetric: ClusterName: {0}, AppName: {1}, ServiceName: {2}, PartitionId: {3}",
                    partition.ClusterName,
                    partition.ApplicationName,
                    partition.ServiceName,
                    partition.PartitionId.ToString()),
                healthEvent);
        }

        public override void EmitReplicaHealthEvent(
            ReplicaEntity replica, EntityHealthEvent healthEvent)
        {
            AppendHealthEventToFile(
                string.Format(
                    "replicaHealthEventMetric: ClusterName: {0}, AppName: {1}, ServiceName: {2}, PartitionId: {3}, ReplicaId: {4}",
                    replica.ClusterName,
                    replica.ApplicationName,
                    replica.ServiceName,
                    replica.PartitionId.ToString(),
                    replica.ReplicaId.ToString()),
                healthEvent);
        }

        public override void EmitDeployedAppHealthEvent(
            DeployedApplicationEntity deployedApp, EntityHealthEvent healthEvent)
        {
            AppendHealthEventToFile(
                string.Format(
                    "deployedAppHealthEventMetric: ClusterName: {0}, AppName: {1}, NodeName: {2}",
                    deployedApp.ClusterName,
                    deployedApp.ApplicationName,
                    deployedApp.NodeName),
                healthEvent);
        }

        public override void EmitDeployedServicePackageHealthEvent(
            DeployedServicePackageEntity deployedServicePackage, EntityHealthEvent healthEvent)
        {
            AppendHealthEventToFile(
                string.Format(
                    "deployedServicePackageHealthEventMetric: ClusterName: {0}, AppName: {1}, ServiceManifestName: {2}, NodeName: {3}",
                    deployedServicePackage.ClusterName,
                    deployedServicePackage.ApplicationName,
                    deployedServicePackage.ServiceManifestName,
                    deployedServicePackage.NodeName),
                healthEvent);
        }
        #endregion

        private static void AppendToFile(string stringToLog)
        {
            File.AppendAllLines(MetricsFileName, new string[] { stringToLog });
        }

        private static void AppendHealthEventToFile(string stringToLog, EntityHealthEvent healthEvent)
        {
            var eventString = string.Format(
                "HealthState: {0}, Property: {1}, SourceId: {2}, IsExpired: {3}",
                healthEvent.HealthState.ToString(),
                healthEvent.Property,
                healthEvent.SourceId,
                healthEvent.IsExpired.ToString());

            var finalString = string.Format("{0}, {1}", stringToLog, eventString);
            File.AppendAllLines(MetricsFileName, new string[] { finalString });
        }
    }
}
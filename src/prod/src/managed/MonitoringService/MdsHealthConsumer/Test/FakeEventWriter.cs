// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Internal.Test
{
    using System.IO;
    using EventSource;
    using FabricMonSvc;
    using Health;

    internal class FakeEventWriter : MonitoringEventWriter
    {
        private const string MetricsFileName = @"e:\temp\events.txt";

        public override void LogClusterHealthState(ClusterEntity cluster)
        {
            AppendToFile(
                string.Format(
                    "LogClusterHealthState: ClusterName: {0}, HealthState: {1}, EvaluationString: {2}",
                    cluster.ClusterName,
                    cluster.Health.AggregatedHealthState.ToString(),
                    cluster.Health.GetHealthEvaluationString()));
        }

        public override void LogApplicationHealthState(ApplicationEntity application)
        {
            AppendToFile(
                string.Format(
                    "LogApplicationHealthState: ClusterName: {0}, AppName:{3} HealthState: {1}, EvaluationString: {2}",
                    application.ClusterName,
                    application.Health.AggregatedHealthState.ToString(),
                    application.Health.GetHealthEvaluationString(),
                    application.ApplicationName));
        }

        public override void LogNodeHealthState(NodeEntity node)
        {
            AppendToFile(
                string.Format(
                    "LogNodeHealthState: ClusterName: {0}, NodeName:{3} HealthState: {1}, EvaluationString: {2}",
                    node.ClusterName,
                    node.Health.AggregatedHealthState.ToString(),
                    node.Health.GetHealthEvaluationString(),
                    node.NodeName));
        }

        public override void LogServiceHealthState(ServiceEntity service)
        {
            AppendToFile(
                string.Format(
                    "LogServiceHealthState: ClusterName: {0}, AppName:{3}, ServiceName:{4}, HealthState: {1}, EvaluationString: {2}",
                    service.ClusterName,
                    service.Health.AggregatedHealthState.ToString(),
                    service.Health.GetHealthEvaluationString(),
                    service.ApplicationName,
                    service.ServiceName));
        }

        public override void LogPartitionHealthState(PartitionEntity partition)
        {
            AppendToFile(
                string.Format(
                    "LogPartitionHealthState: ClusterName: {0}, AppName:{3}, ServiceName:{4}, PartitionId:{5}, HealthState: {1}, EvaluationString: {2}",
                    partition.ClusterName,
                    partition.Health.AggregatedHealthState.ToString(),
                    partition.Health.GetHealthEvaluationString(),
                    partition.ApplicationName,
                    partition.ServiceName,
                    partition.PartitionId.ToString()));
        }

        public override void LogReplicaHealthState(ReplicaEntity replica)
        {
            AppendToFile(
                string.Format(
                    "LogReplicaHealthState: ClusterName: {0}, AppName:{3}, ServiceName:{4}, PartitionId:{5}, ReplicaId:{6}, HealthState: {1}, EvaluationString: {2}",
                    replica.ClusterName,
                    replica.Health.AggregatedHealthState.ToString(),
                    replica.Health.GetHealthEvaluationString(),
                    replica.ApplicationName,
                    replica.ServiceName,
                    replica.PartitionId.ToString(),
                    replica.ReplicaId.ToString()));
        }

        public override void LogDeployedApplicationHealthState(
            DeployedApplicationEntity deployedApplication)
        {
            AppendToFile(
                string.Format(
                    "LogDeployedApplicationHealthState: ClusterName: {0}, AppName:{3}, NodeName:{4}, HealthState: {1}, EvaluationString: {2}",
                    deployedApplication.ClusterName,
                    deployedApplication.Health.AggregatedHealthState.ToString(),
                    deployedApplication.Health.GetHealthEvaluationString(),
                    deployedApplication.ApplicationName,
                    deployedApplication.NodeName));
        }

        public override void LogDeployedServicePackageHealthState(
            DeployedServicePackageEntity deployedServicePackage)
        {
            AppendToFile(
                string.Format(
                    "LogDeployedServicePackageHealthState: ClusterName: {0}, AppName:{3}, ServiceManifestName:{4} NodeName:{5}, HealthState: {1}, EvaluationString: {2}",
                    deployedServicePackage.ClusterName,
                    deployedServicePackage.Health.AggregatedHealthState.ToString(),
                    deployedServicePackage.Health.GetHealthEvaluationString(),
                    deployedServicePackage.ApplicationName,
                    deployedServicePackage.ServiceManifestName,
                    deployedServicePackage.NodeName));
        }

        public override void LogClusterHealthEvent(
            ClusterEntity cluster, EntityHealthEvent healthEvent)
        {
            AppendHealthEventToFile(
                string.Format(
                    "LogClusterHealthEvent: ClusterName: {0}",
                    cluster.ClusterName),
                healthEvent);
        }

        public override void LogApplicationHealthEvent(ApplicationEntity application, EntityHealthEvent healthEvent)
        {
            AppendHealthEventToFile(
                string.Format(
                    "LogApplicationHealthEvent: ClusterName: {0}, AppName: {1}",
                    application.ClusterName,
                    application.ApplicationName),
                healthEvent);
        }

        public override void LogNodeHealthEvent(NodeEntity node, EntityHealthEvent healthEvent)
        {
            AppendHealthEventToFile(
                string.Format(
                    "LogNodeHealthEvent: ClusterName: {0}, NodeName: {1}",
                    node.ClusterName,
                    node.NodeName),
                healthEvent);
        }

        public override void LogServiceHealthEvent(
            ServiceEntity service, EntityHealthEvent healthEvent)
        {
            AppendHealthEventToFile(
                string.Format(
                    "LogServiceHealthEvent: ClusterName: {0}, AppName: {1}, ServiceName: {2}",
                    service.ClusterName,
                    service.ApplicationName,
                    service.ServiceName),
                healthEvent);
        }

        public override void LogPartitionHealthEvent(
            PartitionEntity partition, EntityHealthEvent healthEvent)
        {
            AppendHealthEventToFile(
                string.Format(
                    "LogPartitionHealthEvent: ClusterName: {0}, AppName: {1}, ServiceName: {2}, PartitionId: {3}",
                    partition.ClusterName,
                    partition.ApplicationName,
                    partition.ServiceName,
                    partition.PartitionId.ToString()),
                healthEvent);
        }

        public override void LogReplicaHealthEvent(
            ReplicaEntity replica, EntityHealthEvent healthEvent)
        {
            AppendHealthEventToFile(
                string.Format(
                    "LogReplicaHealthEvent: ClusterName: {0}, AppName: {1}, ServiceName: {2}, PartitionId: {3}, ReplicaId: {4}",
                    replica.ClusterName,
                    replica.ApplicationName,
                    replica.ServiceName,
                    replica.PartitionId.ToString(),
                    replica.ReplicaId.ToString()),
                healthEvent);
        }

        public override void LogDeployedApplicationHealthEvent(
            DeployedApplicationEntity deployedApplication,
            EntityHealthEvent healthEvent)
        {
            AppendHealthEventToFile(
                string.Format(
                    "LogDeployedApplicationHealthEvent: ClusterName: {0}, AppName: {1}, NodeName: {2}",
                    deployedApplication.ClusterName,
                    deployedApplication.ApplicationName,
                    deployedApplication.NodeName),
                healthEvent);
        }

        public override void LogDeployedServicePackageHealthEvent(
            DeployedServicePackageEntity deployedServicePackage,
            EntityHealthEvent healthEvent)
        {
            AppendHealthEventToFile(
                string.Format(
                    "LogDeployedServicePackageHealthEvent: ClusterName: {0}, AppName: {1}, ServiceManifestName: {2}, NodeName: {3}",
                    deployedServicePackage.ClusterName,
                    deployedServicePackage.ApplicationName,
                    deployedServicePackage.ServiceManifestName,
                    deployedServicePackage.NodeName),
                healthEvent);
        }

        private static void AppendToFile(string stringToLog)
        {
            File.AppendAllLines(MetricsFileName, new string[] { stringToLog });
        }

        private static void AppendHealthEventToFile(string stringToLog, EntityHealthEvent healthEvent)
        {
            var eventString = string.Format(
                "HealthState: {0}, Desc: {1}, Property: {2}, SeqNum: {3}, SourceId: {4}, IsExpired: {5}",
                healthEvent.HealthState.ToString(),
                healthEvent.Description,
                healthEvent.Property,
                healthEvent.SequenceNumber.ToString(),
                healthEvent.SourceId,
                healthEvent.IsExpired.ToString());

            var finalString = string.Format("{0}, {1}", stringToLog, eventString);
            File.AppendAllLines(MetricsFileName, new string[] { finalString });
        }
    }
}
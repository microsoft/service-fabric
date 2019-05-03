// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Internal.EventSource
{
    using FabricMonSvc;
    using Health;

    internal class MonitoringEventWriter
    {
        public virtual void LogClusterHealthState(ClusterEntity cluster)
        {
            MonitoringEventSource.Current.LogClusterHealthState(
                cluster.ClusterName,
                cluster.Health.AggregatedHealthState.ToString(),
                cluster.Health.GetHealthEvaluationString());
        }

        public virtual void LogApplicationHealthState(ApplicationEntity application)
        {
            MonitoringEventSource.Current.LogApplicationHealthState(
                application.ClusterName,
                application.ApplicationName,
                application.Health.AggregatedHealthState.ToString(),
                application.Health.GetHealthEvaluationString());
        }

        public virtual void LogNodeHealthState(NodeEntity node)
        {
            MonitoringEventSource.Current.LogNodeHealthState(
                node.ClusterName,
                node.NodeName,
                node.Health.AggregatedHealthState.ToString(),
                node.Health.GetHealthEvaluationString());
        }

        public virtual void LogServiceHealthState(ServiceEntity service)
        {
            MonitoringEventSource.Current.LogServiceHealthState(
               service.ClusterName,
               service.ApplicationName,
               service.ServiceName,
               service.Health.AggregatedHealthState.ToString(),
               service.Health.GetHealthEvaluationString());
        }

        public virtual void LogPartitionHealthState(PartitionEntity partition)
        {
            MonitoringEventSource.Current.LogPartitionHealthState(
              partition.ClusterName,
              partition.ApplicationName,
              partition.ServiceName,
              partition.PartitionId.ToString(),
              partition.Health.AggregatedHealthState.ToString(),
              partition.Health.GetHealthEvaluationString());
        }

        public virtual void LogReplicaHealthState(ReplicaEntity replica)
        {
            MonitoringEventSource.Current.LogReplicaHealthState(
              replica.ClusterName,
              replica.ApplicationName,
              replica.ServiceName,
              replica.PartitionId.ToString(),
              replica.ReplicaId.ToString(),
              replica.Health.AggregatedHealthState.ToString(),
              replica.Health.GetHealthEvaluationString());
        }

        public virtual void LogDeployedApplicationHealthState(
            DeployedApplicationEntity deployedApplication)
        {
            MonitoringEventSource.Current.LogDeployedApplicationHealthState(
              deployedApplication.ClusterName,
              deployedApplication.ApplicationName,
              deployedApplication.NodeName,
              deployedApplication.Health.AggregatedHealthState.ToString(),
              deployedApplication.Health.GetHealthEvaluationString());
        }

        public virtual void LogDeployedServicePackageHealthState(
            DeployedServicePackageEntity deployedServicePackage)
        {
            MonitoringEventSource.Current.LogDeployedServicePackageHealthState(
              deployedServicePackage.ClusterName,
              deployedServicePackage.ApplicationName,
              deployedServicePackage.ServiceManifestName,
              deployedServicePackage.NodeName,
              deployedServicePackage.Health.AggregatedHealthState.ToString(),
              deployedServicePackage.Health.GetHealthEvaluationString());
        }

        public virtual void LogClusterHealthEvent(
            ClusterEntity cluster, EntityHealthEvent healthEvent)
        {
            MonitoringEventSource.Current.LogClusterHealthEvent(
                cluster.ClusterName,
                healthEvent.HealthState.ToString(),
                healthEvent.Description,
                healthEvent.Property,
                healthEvent.SequenceNumber.ToString(),
                healthEvent.SourceId,
                healthEvent.IsExpired.ToString());
        }

        public virtual void LogApplicationHealthEvent(ApplicationEntity application, EntityHealthEvent healthEvent)
        {
            MonitoringEventSource.Current.LogApplicationHealthEvent(
                application.ClusterName,
                application.ApplicationName,
                healthEvent.HealthState.ToString(),
                healthEvent.Description,
                healthEvent.Property,
                healthEvent.SequenceNumber.ToString(),
                healthEvent.SourceId,
                healthEvent.IsExpired.ToString());
        }

        public virtual void LogNodeHealthEvent(NodeEntity node, EntityHealthEvent healthEvent)
        {
            MonitoringEventSource.Current.LogNodeHealthEvent(
                node.ClusterName,
                node.NodeName,
                healthEvent.HealthState.ToString(),
                healthEvent.Description,
                healthEvent.Property,
                healthEvent.SequenceNumber.ToString(),
                healthEvent.SourceId,
                healthEvent.IsExpired.ToString());
        }

        public virtual void LogServiceHealthEvent(
            ServiceEntity service, EntityHealthEvent healthEvent)
        {
            MonitoringEventSource.Current.LogServiceHealthEvent(
                service.ClusterName,
                service.ApplicationName,
                service.ServiceName,
                healthEvent.HealthState.ToString(),
                healthEvent.Description,
                healthEvent.Property,
                healthEvent.SequenceNumber.ToString(),
                healthEvent.SourceId,
                healthEvent.IsExpired.ToString());
        }

        public virtual void LogPartitionHealthEvent(
            PartitionEntity partition, EntityHealthEvent healthEvent)
        {
            MonitoringEventSource.Current.LogPartitionHealthEvent(
                partition.ClusterName,
                partition.ApplicationName,
                partition.ServiceName,
                partition.PartitionId.ToString(),
                healthEvent.HealthState.ToString(),
                healthEvent.Description,
                healthEvent.Property,
                healthEvent.SequenceNumber.ToString(),
                healthEvent.SourceId,
                healthEvent.IsExpired.ToString());
        }

        public virtual void LogReplicaHealthEvent(
            ReplicaEntity replica, EntityHealthEvent healthEvent)
        {
            MonitoringEventSource.Current.LogReplicaHealthEvent(
                replica.ClusterName,
                replica.ApplicationName,
                replica.ServiceName,
                replica.PartitionId.ToString(),
                replica.ReplicaId.ToString(),
                healthEvent.HealthState.ToString(),
                healthEvent.Description,
                healthEvent.Property,
                healthEvent.SequenceNumber.ToString(),
                healthEvent.SourceId,
                healthEvent.IsExpired.ToString());
        }

        public virtual void LogDeployedApplicationHealthEvent(
            DeployedApplicationEntity deployedApplication, 
            EntityHealthEvent healthEvent)
        {
            MonitoringEventSource.Current.LogDeployedApplicationHealthEvent(
                deployedApplication.ClusterName,
                deployedApplication.ApplicationName,
                deployedApplication.NodeName,
                healthEvent.HealthState.ToString(),
                healthEvent.Description,
                healthEvent.Property,
                healthEvent.SequenceNumber.ToString(),
                healthEvent.SourceId,
                healthEvent.IsExpired.ToString());
        }

        public virtual void LogDeployedServicePackageHealthEvent(
            DeployedServicePackageEntity deployedServicePackage, 
            EntityHealthEvent healthEvent)
        {
            MonitoringEventSource.Current.LogDeployedServicePackageHealthEvent(
                deployedServicePackage.ClusterName,
                deployedServicePackage.ApplicationName,
                deployedServicePackage.ServiceManifestName,
                deployedServicePackage.NodeName,
                healthEvent.HealthState.ToString(),
                healthEvent.Description,
                healthEvent.Property,
                healthEvent.SequenceNumber.ToString(),
                healthEvent.SourceId,
                healthEvent.IsExpired.ToString());
        }
    }
}
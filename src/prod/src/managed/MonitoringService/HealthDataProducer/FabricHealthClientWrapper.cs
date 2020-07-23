// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMonSvc
{
    using System;
    using System.Fabric;
    using System.Fabric.Health;
    using System.Threading;
    using System.Threading.Tasks;
    using FabricHealth = System.Fabric.Health;
    using MonitoringHealth = Microsoft.ServiceFabric.Monitoring.Health;

    /// <summary>
    /// Wrapper around the FabricClient.HealthClient static methods to support DI.
    /// </summary>
    internal class FabricHealthClientWrapper
    {
        private FabricClient.HealthClient healthClient;
        private TraceWriterWrapper traceWriter;
        private string serviceTypeName;
        private Guid partitionId;
        private long replicaId;

        public FabricHealthClientWrapper(TraceWriterWrapper trace, string serviceTypeName, Guid partitionId, long replicaId)
        {
            this.traceWriter = Guard.IsNotNull(trace, nameof(trace));
            this.healthClient = this.GetNewHealthClientInstance();

            this.serviceTypeName = Guard.IsNotNullOrEmpty(serviceTypeName, nameof(serviceTypeName));
            this.partitionId = partitionId;
            this.replicaId = replicaId;
        }

        public virtual async Task<MonitoringHealth.ClusterHealth> GetClusterHealthAsync(
            TimeSpan timeout, CancellationToken cancellationToken)
        {
            return
                await this.GetHealthAsync<FabricHealth.ClusterHealth, MonitoringHealth.ClusterHealth>(
                    async () => await this.healthClient.GetClusterHealthAsync(timeout, cancellationToken),
                    (health) => new MonitoringHealth.ClusterHealth(health))
                    .ConfigureAwait(false);
        }

        public virtual async Task<MonitoringHealth.ApplicationHealth> GetApplicationHealthAsync(
            Uri applicationName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return
                await this.GetHealthAsync<FabricHealth.ApplicationHealth, MonitoringHealth.ApplicationHealth>(
                    async () => await this.healthClient.GetApplicationHealthAsync(applicationName, timeout, cancellationToken),
                    (health) => new MonitoringHealth.ApplicationHealth(health))
                    .ConfigureAwait(false);
        }

        public virtual async Task<MonitoringHealth.NodeHealth> GetNodeHealthAsync(
            string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return
                await this.GetHealthAsync<FabricHealth.NodeHealth, MonitoringHealth.NodeHealth>(
                    async () => await this.healthClient.GetNodeHealthAsync(nodeName, timeout, cancellationToken),
                    (health) => new MonitoringHealth.NodeHealth(health))
                    .ConfigureAwait(false);
        }

        public virtual async Task<MonitoringHealth.ServiceHealth> GetServiceHealthAsync(
            Uri serviceUri, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return
                await this.GetHealthAsync<FabricHealth.ServiceHealth, MonitoringHealth.ServiceHealth>(
                    async () => await this.healthClient.GetServiceHealthAsync(serviceUri, timeout, cancellationToken),
                    (health) => new MonitoringHealth.ServiceHealth(health))
                    .ConfigureAwait(false);
        }

        public virtual async Task<MonitoringHealth.PartitionHealth> GetPartitionHealthAsync(
            Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return
                await this.GetHealthAsync<FabricHealth.PartitionHealth, MonitoringHealth.PartitionHealth>(
                    async () => await this.healthClient.GetPartitionHealthAsync(partitionId, timeout, cancellationToken),
                    (health) => new MonitoringHealth.PartitionHealth(health))
                    .ConfigureAwait(false);
        }

        public virtual async Task<MonitoringHealth.ReplicaHealth> GetReplicaHealthAsync(
            Guid partitionId, long replicaId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return
                await this.GetHealthAsync<FabricHealth.ReplicaHealth, MonitoringHealth.ReplicaHealth>(
                    async () => await this.healthClient.GetReplicaHealthAsync(partitionId, replicaId, timeout, cancellationToken),
                    (health) => new MonitoringHealth.ReplicaHealth(health))
                    .ConfigureAwait(false);
        }

        public virtual async Task<MonitoringHealth.DeployedApplicationHealth> GetDeployedApplicationHealthAsync(
            Uri applicationName, string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return
                await this.GetHealthAsync<FabricHealth.DeployedApplicationHealth, MonitoringHealth.DeployedApplicationHealth>(
                    async () => await this.healthClient.GetDeployedApplicationHealthAsync(applicationName, nodeName, timeout, cancellationToken),
                    (health) => new MonitoringHealth.DeployedApplicationHealth(health))
                    .ConfigureAwait(false);
        }

        public virtual async Task<MonitoringHealth.DeployedServicePackageHealth> GetDeployedServicePackageHealthAsync(
            Uri applicationName, string serviceManifestName, string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
           return
                await this.GetHealthAsync<FabricHealth.DeployedServicePackageHealth, MonitoringHealth.DeployedServicePackageHealth>(
                    async () => await this.healthClient.GetDeployedServicePackageHealthAsync(
                        applicationName, serviceManifestName, nodeName, timeout, cancellationToken),
                    (health) => new MonitoringHealth.DeployedServicePackageHealth(health))
                    .ConfigureAwait(false);
        }

        public virtual void ReportHealth(HealthState healthState, string property, string description)
        {
            this.healthClient.ReportHealth(
                 new StatefulServiceReplicaHealthReport(
                     this.partitionId,
                     this.replicaId,
                     new HealthInformation(this.serviceTypeName, property, healthState) { Description = description }));
        }

        internal virtual async Task<TEntityHealth> DoGetHealthAsync<TEntityHealth>(Func<Task<TEntityHealth>> healthQuery)
            where TEntityHealth : FabricHealth.EntityHealth
        {
            return await healthQuery().ConfigureAwait(false);
        }

        private async Task<TMonitoringEntityHealth> GetHealthAsync<TEntityHealth, TMonitoringEntityHealth>(
            Func<Task<TEntityHealth>> healthQuery, Func<TEntityHealth, TMonitoringEntityHealth> initializer) 
            where TEntityHealth : FabricHealth.EntityHealth 
            where TMonitoringEntityHealth : MonitoringHealth.EntityHealth
        {
            try
            {
                var health = await this.DoGetHealthAsync(healthQuery).ConfigureAwait(false);
                return initializer(health);
            }
            catch (TimeoutException timeoutEx)
            {
                this.traceWriter.Exception(timeoutEx);
            }
            catch (FabricObjectClosedException fabricClientClosedEx)
            {
                this.traceWriter.Exception(fabricClientClosedEx);
                this.healthClient = this.GetNewHealthClientInstance();
            }
            catch (FabricTransientException fabricTransientEx)
            {
                this.traceWriter.Exception(fabricTransientEx);
            }
            catch (FabricException fabricEx)
            {
                this.traceWriter.Exception(fabricEx);
            }

            return null;
        }

        private FabricClient.HealthClient GetNewHealthClientInstance()
        {
            FabricClient fabricClient = new FabricClient();
            return fabricClient.HealthManager;
        }
    }
}
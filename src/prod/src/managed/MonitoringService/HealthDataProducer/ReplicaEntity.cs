// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMonSvc
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Monitoring.Filters;
    using Microsoft.ServiceFabric.Monitoring.Health;
    using Microsoft.ServiceFabric.Monitoring.Interfaces;

    internal class ReplicaEntity : Entity<ReplicaHealth>
    {
        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "This rule breaks for nameof() operator.")]
        internal ReplicaEntity(
            FabricHealthClientWrapper healthClient,
            HealthDataConsumer consumer, 
            string clusterName,
            string applicationName,
            string serviceName,
            Guid partitionId,
            long replicaId, 
            TraceWriterWrapper traceWriter,
            IServiceConfiguration config,
            EntityFilterRepository filterRepository)
            : base(healthClient, consumer, traceWriter, config, filterRepository)
        {
            this.ClusterName = Guard.IsNotNullOrEmpty(clusterName, nameof(clusterName));
            this.ApplicationName = Guard.IsNotNullOrEmpty(applicationName, nameof(applicationName));
            this.ServiceName = Guard.IsNotNullOrEmpty(serviceName, nameof(serviceName));
            this.PartitionId = partitionId;
            this.ReplicaId = replicaId;
        }

        public string ClusterName { get; private set; }

        public string ApplicationName { get; private set; }

        public string ServiceName { get; private set; }

        public Guid PartitionId { get; private set; }

        public long ReplicaId { get; private set; }

        protected async override Task<ReplicaHealth> GetHealthAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await this.HealthClient.GetReplicaHealthAsync(
                this.PartitionId,
                this.ReplicaId,
                timeout,
                cancellationToken)
                .ConfigureAwait(false);
        }

        protected async override Task ProcessHealthAsync()
        {
            await this.Consumer.ProcessReplicaHealthAsync(this);
        }
    }
}
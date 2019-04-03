// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMonSvc
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Monitoring.Filters;
    using Microsoft.ServiceFabric.Monitoring.Health;
    using Microsoft.ServiceFabric.Monitoring.Interfaces;

    internal class PartitionEntity : Entity<PartitionHealth>
    {
        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "This rule breaks for nameof() operator.")]
        internal PartitionEntity(
            FabricHealthClientWrapper healthClient,
            HealthDataConsumer consumer,
            string clusterName,
            string applicationName,
            string serviceName,
            Guid partitionId,
            TraceWriterWrapper traceWriter,
            IServiceConfiguration config,
            EntityFilterRepository filterRepository)
            : base(healthClient, consumer, traceWriter, config, filterRepository)
        {
            this.ClusterName = Guard.IsNotNullOrEmpty(clusterName, nameof(clusterName));
            this.ApplicationName = Guard.IsNotNullOrEmpty(applicationName, nameof(applicationName));
            this.ServiceName = Guard.IsNotNullOrEmpty(serviceName, nameof(serviceName));
            this.PartitionId = partitionId;
        }

        public string ClusterName { get; private set; }

        public string ApplicationName { get; private set; }

        public string ServiceName { get; private set; }

        public Guid PartitionId { get; private set; }
        
        protected async override Task<PartitionHealth> GetHealthAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await 
                this.HealthClient.GetPartitionHealthAsync(
                this.PartitionId,
                timeout,
                cancellationToken)
                .ConfigureAwait(false);
        }

        protected async override Task ProcessHealthAsync()
        {
            await this.Consumer.ProcessPartitionHealthAsync(this).ConfigureAwait(false);
        }

        protected override IEnumerable<IEntity> GetChildren(PartitionHealth health)
        {
            var replicaFilter = this.Filters.ReplicaHealthFilter;
            if (!replicaFilter.IsEntityEnabled(this.ApplicationName))
            {
                return Enumerable.Empty<ReplicaEntity>();
            }

            return 
                health.ReplicaHealthStates
                .Where(state => replicaFilter.IsEntityEnabled(state.AggregatedHealthState))
                .Select(state => new ReplicaEntity(
                    this.HealthClient,
                    this.Consumer,
                    this.ClusterName,
                    this.ApplicationName,
                    this.ServiceName,
                    this.PartitionId,
                    state.ReplicaId,
                    this.TraceWriter,
                    this.Config,
                    this.Filters));
        }
    }
}
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

    internal class ServiceEntity : Entity<ServiceHealth>
    {
        private readonly Uri serviceUri;

        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "This rule breaks for nameof() operator.")]
        internal ServiceEntity(
            FabricHealthClientWrapper healthClient,
            HealthDataConsumer consumer,
            string clusterName,
            string applicationName,
            Uri serviceUri,
            TraceWriterWrapper traceWriter,
            IServiceConfiguration config,
            EntityFilterRepository filterRepository)
            : base(healthClient, consumer, traceWriter, config, filterRepository)
        {
            this.ClusterName = Guard.IsNotNullOrEmpty(clusterName, nameof(clusterName));
            this.ApplicationName = Guard.IsNotNullOrEmpty(applicationName, nameof(applicationName));
            this.serviceUri = Guard.IsNotNull(serviceUri, nameof(serviceUri));
        }

        public string ClusterName { get; private set; }

        public string ApplicationName { get; private set; }

        public string ServiceName
        {
            get { return this.serviceUri.OriginalString; }
        }

        protected async override Task<ServiceHealth> GetHealthAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await this.HealthClient.GetServiceHealthAsync(this.serviceUri, timeout, cancellationToken).ConfigureAwait(false);
        }

        protected async override Task ProcessHealthAsync()
        {
            await this.Consumer.ProcessServiceHealthAsync(this).ConfigureAwait(false);
        }

        protected override IEnumerable<IEntity> GetChildren(ServiceHealth health)
        {
            var partitionFilter = this.Filters.PartitionHealthFilter;
            if (!partitionFilter.IsEntityEnabled(this.ApplicationName))
            {
                return Enumerable.Empty<PartitionEntity>();
            }

            return
                health.PartitionHealthStates
                .Where(state => partitionFilter.IsEntityEnabled(state.AggregatedHealthState))
                .Select(state => new PartitionEntity(
                    this.HealthClient,
                    this.Consumer,
                    this.ClusterName,
                    this.ApplicationName,
                    this.serviceUri.OriginalString,
                    state.PartitionId,
                    this.TraceWriter,
                    this.Config,
                    this.Filters));
        }
    }
}
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

    internal class ApplicationEntity : Entity<ApplicationHealth>
    {
        private readonly Uri applicationUri;

        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "This rule breaks for nameof() operator.")]
        internal ApplicationEntity(
            FabricHealthClientWrapper healthClient,
            HealthDataConsumer consumer, 
            string clusterName,
            Uri applicationUri, 
            TraceWriterWrapper traceWriter,
            IServiceConfiguration config,
            EntityFilterRepository filterRepository)
            : base(healthClient, consumer, traceWriter, config, filterRepository)
        {
            this.ClusterName = Guard.IsNotNullOrEmpty(clusterName, nameof(clusterName));
            this.applicationUri = Guard.IsNotNull(applicationUri, nameof(applicationUri));
        }

        public string ClusterName { get; private set; }

        public string ApplicationName
        {
            get { return this.applicationUri.OriginalString; }
        }

        protected async override Task<ApplicationHealth> GetHealthAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await this.HealthClient.GetApplicationHealthAsync(this.applicationUri, timeout, cancellationToken).ConfigureAwait(false);
        }

        protected async override Task ProcessHealthAsync()
        {
            await this.Consumer.ProcessApplicationHealthAsync(this).ConfigureAwait(false);
        }

        protected override IEnumerable<IEntity> GetChildren(ApplicationHealth health)
        {
            var children = new List<IEntity>();

            var serviceEntityFilter = this.Filters.ServiceHealthFilter;
            if (serviceEntityFilter.IsEntityEnabled(this.ApplicationName))
            {
                children.AddRange(
                    health.ServiceHealthStates
                    .Where(state => serviceEntityFilter.IsEntityEnabled(state.AggregatedHealthState))
                    .Select(state => new ServiceEntity(
                        this.HealthClient,
                        this.Consumer,
                        this.ClusterName,
                        this.ApplicationName,
                        state.ServiceName,
                        this.TraceWriter,
                        this.Config,
                        this.Filters)));
            }

            var deployedAppEntityFilter = this.Filters.DeployedApplicationHealthFilter;
            if (deployedAppEntityFilter.IsEntityEnabled(this.ApplicationName))
            {
                children.AddRange(
                    health.DeployedApplicationHealthStates
                    .Where(state => deployedAppEntityFilter.IsEntityEnabled(state.AggregatedHealthState))
                    .Select(state => new DeployedApplicationEntity(
                        this.HealthClient,
                        this.Consumer,
                        this.ClusterName,
                        this.applicationUri,
                        state.NodeName,
                        this.TraceWriter,
                        this.Config,
                        this.Filters)));
            }

            return children;
        }
    }
}
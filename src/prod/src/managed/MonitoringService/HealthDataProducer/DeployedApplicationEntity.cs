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

    internal class DeployedApplicationEntity : Entity<DeployedApplicationHealth>
    {
        private readonly Uri applicationUri;

        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "This rule breaks for nameof() operator.")]
        internal DeployedApplicationEntity(
            FabricHealthClientWrapper healthClient,
            HealthDataConsumer consumer,
            string clusterName,
            Uri applicationUri,
            string nodeName,
            TraceWriterWrapper traceWriter,
            IServiceConfiguration config,
            EntityFilterRepository filterRepository)
            : base(healthClient, consumer, traceWriter, config, filterRepository)
        {
            this.ClusterName = Guard.IsNotNullOrEmpty(clusterName, nameof(clusterName));
            this.NodeName = Guard.IsNotNullOrEmpty(nodeName, nameof(nodeName));
            this.applicationUri = Guard.IsNotNull(applicationUri, nameof(applicationUri));
        }

        public string ClusterName { get; private set; }

        public string NodeName { get; private set; }

        public string ApplicationName
        {
            get { return this.applicationUri.OriginalString; }
        }

        protected async override Task<DeployedApplicationHealth> GetHealthAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await 
                this.HealthClient.GetDeployedApplicationHealthAsync(
                this.applicationUri,
                this.NodeName,
                timeout,
                cancellationToken)
                .ConfigureAwait(false);
        }

        protected async override Task ProcessHealthAsync()
        {
            await this.Consumer.ProcessDeployedApplicationHealthAsync(this);
        }

        protected override IEnumerable<IEntity> GetChildren(DeployedApplicationHealth health)
        {
            var filter = this.Filters.ServicePackageHealthFilter;
            if (!filter.IsEntityEnabled(this.ApplicationName))
            {
                return Enumerable.Empty<DeployedServicePackageEntity>();
            }

            return
                health.DeployedServicePackageHealthStates
                .Where(state => filter.IsEntityEnabled(state.AggregatedHealthState))
                .Select(state => new DeployedServicePackageEntity(
                    this.HealthClient,
                    this.Consumer,
                    this.ClusterName,
                    this.applicationUri,
                    state.ServiceManifestName,
                    state.NodeName,
                    this.TraceWriter,
                    this.Config,
                    this.Filters));
        }
    }
}
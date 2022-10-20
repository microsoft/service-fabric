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

    internal class ClusterEntity : Entity<ClusterHealth>
    {
        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "This rule breaks for nameof() operator.")]
        internal ClusterEntity(
            FabricHealthClientWrapper healthClient,
            HealthDataConsumer consumer, 
            TraceWriterWrapper traceWriter, 
            IServiceConfiguration config,
            EntityFilterRepository filterRepository)
            : base(healthClient, consumer, traceWriter, config, filterRepository)
        {
            this.ClusterName = Guard.IsNotNullOrEmpty(this.Config.ClusterName, nameof(this.Config.ClusterName));
        }

        public string ClusterName { get; private set; }

        protected async override Task<ClusterHealth> GetHealthAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await this.HealthClient.GetClusterHealthAsync(timeout, cancellationToken).ConfigureAwait(false);
        }

        protected async override Task ProcessHealthAsync()
        {
            await this.Consumer.ProcessClusterHealthAsync(this).ConfigureAwait(false);
        }

        protected override IEnumerable<IEntity> GetChildren(ClusterHealth health)
        {
            var children = new List<IEntity>();

            children.AddRange(
                this.Health.NodeHealthStates
                .Select(nodeState => new NodeEntity(
                    this.HealthClient,
                    this.Consumer,
                    this.ClusterName,
                    nodeState.NodeName,
                    this.TraceWriter,
                    this.Config,
                    this.Filters)));

            children.AddRange(
                this.Health.ApplicationHealthStates
                .Select(appState => new ApplicationEntity(
                    this.HealthClient,
                    this.Consumer,
                    this.ClusterName,
                    appState.ApplicationName,
                    this.TraceWriter,
                    this.Config,
                    this.Filters)));

            return children;
        }
    }
}
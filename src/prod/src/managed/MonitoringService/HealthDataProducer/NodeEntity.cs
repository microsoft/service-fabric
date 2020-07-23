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

    internal class NodeEntity : Entity<NodeHealth>
    {
        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "This rule breaks for nameof() operator.")]
        internal NodeEntity(
            FabricHealthClientWrapper healthClient,
            HealthDataConsumer consumer,
            string clusterName,
            string nodeName,
            TraceWriterWrapper traceWriter,
            IServiceConfiguration config,
            EntityFilterRepository filterRepository)
            : base(healthClient, consumer, traceWriter, config, filterRepository)
        {
            this.ClusterName = Guard.IsNotNullOrEmpty(clusterName, nameof(clusterName));
            this.NodeName = Guard.IsNotNullOrEmpty(nodeName, nameof(nodeName));
        }

        public string ClusterName { get; private set; }

        public string NodeName { get; private set; }

        protected async override Task<NodeHealth> GetHealthAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await this.HealthClient.GetNodeHealthAsync(this.NodeName, timeout, cancellationToken).ConfigureAwait(false);
        }

        protected async override Task ProcessHealthAsync()
        {
            await this.Consumer.ProcessNodeHealthAsync(this).ConfigureAwait(false);
        }
    }
}
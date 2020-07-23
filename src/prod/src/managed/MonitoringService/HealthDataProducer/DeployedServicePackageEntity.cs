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

    internal class DeployedServicePackageEntity : Entity<DeployedServicePackageHealth>
    {
        private readonly Uri applicationUri;

        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "This rule breaks for nameof() operator.")]
        internal DeployedServicePackageEntity(
            FabricHealthClientWrapper healthClient,
            HealthDataConsumer consumer,
            string clusterName,
            Uri applicationUri,
            string serviceManifestName,
            string nodeName, 
            TraceWriterWrapper traceWriter,
            IServiceConfiguration config,
            EntityFilterRepository filterRepository)
            : base(healthClient, consumer, traceWriter, config, filterRepository)
        {
            this.ClusterName = Guard.IsNotNullOrEmpty(clusterName, nameof(clusterName));
            this.NodeName = Guard.IsNotNullOrEmpty(nodeName, nameof(nodeName));
            this.applicationUri = Guard.IsNotNull(applicationUri, nameof(applicationUri));
            this.ServiceManifestName = Guard.IsNotNullOrEmpty(serviceManifestName, nameof(serviceManifestName));
        }

        public string ClusterName { get; private set; }

        public string NodeName { get; private set; }

        public string ApplicationName
        {
            get { return this.applicationUri.OriginalString; }
        }

        public string ServiceManifestName { get; private set; }

        protected async override Task<DeployedServicePackageHealth> GetHealthAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await this.HealthClient.GetDeployedServicePackageHealthAsync(
                   this.applicationUri,
                   this.ServiceManifestName,
                   this.NodeName,
                   timeout,
                   cancellationToken)
                   .ConfigureAwait(false);
        }

        protected async override Task ProcessHealthAsync()
        {
            await this.Consumer.ProcessDeployedServicePackageHealthAsync(this).ConfigureAwait(false);
        }
    }
}
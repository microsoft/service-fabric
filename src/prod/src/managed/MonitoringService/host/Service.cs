// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMonSvc
{
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;
    using MdsHealthDataConsumer;
    using Microsoft.ServiceFabric.Monitoring.Filters;
    using Microsoft.ServiceFabric.Monitoring.Internal.EventSource;

    /// <summary>
    /// Represents the MonitoringService which co-exists with CM via aligned affinity and reports health data.
    /// </summary>
    internal class Service : PrimaryElectionService
    {
        private static readonly TraceWriterWrapper TraceWriter = new TraceWriterWrapper();
        private HealthDataService healthDataService; 
        private StatefulServiceContext serviceContext;

        internal Service(StatefulServiceContext context)
            : base(context, TraceWriter)
        {
            this.serviceContext = context;
        }

        protected override async Task RunAsync(CancellationToken cancellationToken)
        {
            if (this.healthDataService == null)
            {
                // compose the HealthDataService instance with all the dependencies.
                var config = new ServiceConfiguration(this.serviceContext.CodePackageActivationContext, TraceWriter);
                var filterRepository = new EntityFilterRepository(config);

                var healthClient = new FabricHealthClientWrapper(
                    TraceWriter,
                    this.serviceContext.ServiceTypeName,
                    this.serviceContext.PartitionId,
                    this.serviceContext.ReplicaId);

                var eventWriter = new MonitoringEventWriter();
                var metricsEmitter = new MetricsEmitter(config, TraceWriter);
                var consumer = new IfxHealthDataConsumer(eventWriter, metricsEmitter);
                var producer = new HealthDataProducer(healthClient, consumer, TraceWriter, config, filterRepository);
                this.healthDataService = new HealthDataService(producer, TraceWriter, config, filterRepository);

                this.TraceInfo("Service.RunAsync: Composed new HealthDataService instance");
            }

            this.TraceInfo("Service.RunAsync: Invoking HealthDataService.RunAsync");
            await this.healthDataService.RunAsync(cancellationToken).ConfigureAwait(false);
        }

        private void TraceInfo(string message)
        {
            TraceWriter.WriteInfo(
                            "{0}. Partition ID: {1}, replica ID: {2}.",
                            message,
                            this.serviceContext.PartitionId,
                            this.serviceContext.ReplicaId);
        }
    }
}
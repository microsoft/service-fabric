// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMonSvc
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Health;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Monitoring.Filters;
    using Microsoft.ServiceFabric.Monitoring.Interfaces;

    /// <summary>
    /// This class runs a depth-first traversal on the entity tree starting at ClusterEntity. 
    /// It invokes each entity to read and process health data.
    /// </summary>
    internal class HealthDataProducer
    {
        private readonly Stack<IEntity> stack;
        private readonly FabricHealthClientWrapper healthClient;
        private readonly HealthDataConsumer consumer;
        private readonly TraceWriterWrapper traceWriter;
        private readonly IServiceConfiguration config;
        private readonly TimeSpan cancellationTimeout = TimeSpan.FromMilliseconds(100);
        private readonly EntityFilterRepository filterRepository;
        private readonly Stopwatch stopwatch;

        internal HealthDataProducer(
            FabricHealthClientWrapper healthClient, 
            HealthDataConsumer consumer, 
            TraceWriterWrapper traceWriter, 
            IServiceConfiguration config,
            EntityFilterRepository filterRepository)
        {
            this.healthClient = Guard.IsNotNull(healthClient, nameof(healthClient));
            this.consumer = Guard.IsNotNull(consumer, nameof(consumer));
            this.traceWriter = Guard.IsNotNull(traceWriter, nameof(traceWriter));
            this.config = Guard.IsNotNull(config, nameof(config));
            this.filterRepository = Guard.IsNotNull(filterRepository, nameof(filterRepository));
            this.stopwatch = new Stopwatch();

            this.stack = new Stack<IEntity>();
        }

        public async Task ReadHealthDataAsync(CancellationToken cancellationToken)
        {
            this.traceWriter.WriteInfo("HealthDataProducer.ReadHealthData: Health data read pass has started.");

            IEntity clusterEntity = new ClusterEntity(this.healthClient, this.consumer, this.traceWriter, this.config, this.filterRepository);

            // We will be performing a depth-first traversal of entities. Push the first entity to stack.
            this.stack.Push(clusterEntity);
            var entityCount = 0;
            
            this.stopwatch.Restart();

            while (this.stack.Count > 0)
            {
                if (cancellationToken.IsCancellationRequested)
                {
                    this.traceWriter.WriteInfo("HealthDataProducer.ReadHealthDataAsync: Entity processing canceled.");
                    this.stopwatch.Reset();
                    return;
                }

                if (this.stopwatch.Elapsed >= this.config.HealthDataReadTimeout)
                {
                    this.traceWriter.WriteError("HealthDataProducer.ReadHealthDataAsync: Entity processing timed out.");
                    this.stopwatch.Reset();

                    this.healthClient.ReportHealth(
                        HealthState.Error, 
                        "ReadHealthData", 
                        "Read health data loop timed out. The Monitoring service took more time to process health data than the HealthDataReadTimeoutInSeconds value.");

                    return;
                }

                IEntity entity = this.stack.Pop();
                if (entity == null)
                {
                    continue;
                }

                var resultEntityList = await entity.ProcessAsync(cancellationToken).ConfigureAwait(false);
                entityCount++;

                // If the previous entity had any children we need to process, push them on to stack.
                if (resultEntityList != null && !cancellationToken.IsCancellationRequested)
                {
                    foreach (var entityToProcess in resultEntityList)
                    {
                        this.stack.Push(entityToProcess);
                    }
                }
            }

            this.traceWriter.WriteInfo(
                "HealthDataProducer.ReadHealthData: Read pass completed. Processed {0} entities in {1} millisecond.", 
                entityCount, 
                this.stopwatch.Elapsed);

            this.stopwatch.Reset();

            this.healthClient.ReportHealth(
                        HealthState.Ok,
                        "ReadHealthData",
                        "Read health data loop completed successfully.");
        }
    }
}
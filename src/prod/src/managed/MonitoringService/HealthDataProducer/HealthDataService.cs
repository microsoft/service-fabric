// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMonSvc
{
    using System;
    using System.Diagnostics;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Monitoring.Filters;
    using Microsoft.ServiceFabric.Monitoring.Interfaces;

    /// <summary>
    /// Implements the service which manages the long running loop for reading and processing health data.
    /// </summary>
    internal class HealthDataService
    {
        internal HealthDataService(
            HealthDataProducer producer, 
            TraceWriterWrapper traceWriter, 
            IServiceConfiguration config,
            EntityFilterRepository filterRepository)
        {
            this.Producer = Guard.IsNotNull(producer, nameof(producer));
            this.Trace = Guard.IsNotNull(traceWriter, nameof(traceWriter));
            this.Config = Guard.IsNotNull(config, nameof(config));
            this.FilterRepository = Guard.IsNotNull(filterRepository, nameof(filterRepository));
        }

        protected HealthDataProducer Producer { get; private set; }

        protected TraceWriterWrapper Trace { get; private set; }

        protected IServiceConfiguration Config { get; private set; }

        protected EntityFilterRepository FilterRepository { get; private set; }

        internal async Task RunAsync(CancellationToken cancellationToken)
        {
            if (cancellationToken.IsCancellationRequested)
            {
                this.Trace.WriteInfo("HealthDataService.RunAsync returning due to cancellation.");
                return;
            }

            this.Trace.WriteInfo("HealthDataService.RunAsync starting.");

            await this.ExecuteRunAsync(cancellationToken).ConfigureAwait(false);

            this.Trace.WriteInfo("HealthDataService.RunAsync terminated.");
        }

        internal virtual async Task DelayNextReadPass(CancellationToken cancellationToken)
        {
            this.Trace.WriteInfo("HealthDataService.DelayNextReadPass: Scheduling next pass with Task.Delay.");
            await Task.Delay(this.Config.HealthDataReadInterval, cancellationToken).ConfigureAwait(false);
        }

        private async Task ExecuteRunAsync(CancellationToken cancellationToken)
        {
            try
            {
                this.Trace.WriteInfo("HealthDataService.ExecuteRunAsync: Invoking Producer.ReadHealthDataAsync.");

                while (!cancellationToken.IsCancellationRequested)
                {
                    // Rehydrate the config properties for every iteration 
                    // to make sure that config has not changed due to a config update
                    this.Config.Refresh();
                    this.FilterRepository.RefreshFilterRepository();

                    await this.Producer.ReadHealthDataAsync(cancellationToken).ConfigureAwait(false);
                    await this.DelayNextReadPass(cancellationToken).ConfigureAwait(false);
                }
            }
            catch (OperationCanceledException ex)
            {
                if (cancellationToken.IsCancellationRequested)
                {
                    this.Trace.WriteInfo("HealthDataService.ExecuteRunAsync: Handled TaskCanceledException after cancellation was requested.");
                }
                else
                {
                    this.Trace.Exception(ex);
                    throw;
                }
            }
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service
{
    using System.Collections.Generic;
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;
    using EventStore.Service.ApiEndpoints.Server;
    using EventStore.Service.DataReader;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Services.Communication.Runtime;
    using Microsoft.ServiceFabric.Services.Runtime;
    using EventStore.Service.LogProvider;
    using System;

    /// <summary>
    /// Main Class for Event Store service
    /// </summary>
    internal sealed class EventStoreService : StatefulService
    {
        private static TimeSpan MaxWaitForTaskRelease = TimeSpan.FromSeconds(6);

        private readonly ClusterEndpointSecuritySettingsChangeNotifier clusterEndpointSecuritySettingsChangeNotifier;

        private EventStoreRuntime runtime;

        #region Public_Constructor

        /// <summary>
        /// </summary>
        /// <param name="context"></param>
        public EventStoreService(StatefulServiceContext context, string endpoint)
            : base(
                  context,
                  new ReliableStateManager(
                    context,
                    new ReliableStateManagerConfiguration(
                        GetReplicatorSettings(endpoint))))
        {
            this.clusterEndpointSecuritySettingsChangeNotifier =
                new ClusterEndpointSecuritySettingsChangeNotifier(
                    endpoint,
                    this.UpdateReplicatorSettings);
        }

        #endregion Public_Constructor

        /// <summary>
        /// Create communication listeners for the service
        /// </summary>
        /// <returns>A collection of listeners.</returns>
        protected override IEnumerable<ServiceReplicaListener> CreateServiceReplicaListeners()
        {
            // Create runtime with Caching Enabled.
            // TODO - Move the caching enabled Param to Config
            this.runtime = new EventStoreRuntime(new EventStoreReader(DataReaderMode.DirectReadMode));

            return new[]
            {
              new ServiceReplicaListener(serviceContext => new OwinCommunicationListener(runtime, serviceContext) ,"RestEndpoint")
            };
        }

        protected override async Task RunAsync(CancellationToken cancellationToken)
        {
            EventStoreLogger.Logger.LogMessage("RunAsync: Switching Mode to Caching Mode");

            await this.runtime.EventStoreReader.SwitchModeAsync(DataReaderMode.CacheMode, cancellationToken).ConfigureAwait(false);

            try
            {
                await Task.Delay(Timeout.Infinite, cancellationToken).ConfigureAwait(false);
            }
            catch (OperationCanceledException)
            {
                if (cancellationToken.IsCancellationRequested)
                {
                    using (var cancellationSource = new CancellationTokenSource(MaxWaitForTaskRelease))
                    {
                        EventStoreLogger.Logger.LogMessage("RunAsync: Cancellation Requested. Switching Data Reader mode");

                        await this.runtime.EventStoreReader.SwitchModeAsync(DataReaderMode.DirectReadMode, cancellationSource.Token).ConfigureAwait(false);

                        EventStoreLogger.Logger.LogMessage("RunAsync: Cancellation Requested. Mode Switched");
                    }
                }
            }
        }

        private static ReliableStateManagerReplicatorSettings GetReplicatorSettings(string endpoint)
        {
            var replicatorSettings = new ReliableStateManagerReplicatorSettings()
            {
                ReplicatorAddress = endpoint,
                SecurityCredentials = SecurityCredentials.LoadClusterSettings()
            };

            return replicatorSettings;
        }

        private void UpdateReplicatorSettings(string endpoint)
        {
            var replicatorSettings = GetReplicatorSettings(endpoint);
            var stateProviderReplica = ((ReliableStateManager)this.StateManager).Replica;
            var impl = (ReliableStateManagerImpl)stateProviderReplica;
            impl.Replicator.LoggingReplicator.UpdateReplicatorSettings(replicatorSettings);
        }
    }
}
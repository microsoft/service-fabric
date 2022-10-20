// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.GatewayResourceManager.Service
{
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Services.Communication.Runtime;
    using Microsoft.ServiceFabric.Services.Runtime;

    public class FabricGatewayResourceManager : StatefulService
    {
        public const string GatewayResourceManagerTraceType = "GatewayResourceManager.Service";

        private GatewayResourceManagerImpl gatewayResourceManagerImpl;

        internal FabricGatewayResourceManager(StatefulServiceContext serviceContext, string endpoint)
            : base(
            serviceContext,
            new ReliableStateManager(
                serviceContext,
                new ReliableStateManagerConfiguration(
                    new ReliableStateManagerReplicatorSettings()
                    {
                        ReplicatorAddress = endpoint
                    })))
        {
        }

        protected override IEnumerable<ServiceReplicaListener> CreateServiceReplicaListeners()
        {
            GatewayResourceManagerTrace.TraceSource.WriteInfo(GatewayResourceManagerTraceType, "Enter CreateServiceReplicaListeners");
            return new[] { new ServiceReplicaListener(this.CreateCommunicationListener) };
        }

        protected override async Task RunAsync(CancellationToken cancellationToken)
        {
            GatewayResourceManagerTrace.TraceSource.WriteInfo(GatewayResourceManagerTraceType, "Enter RunAsync");
            try
            {
                await this.InitializeAsync(cancellationToken).ConfigureAwait(false);

                GatewayResourceManagerTrace.TraceSource.WriteInfo(GatewayResourceManagerTraceType, "After InitializeAsync");
                await Task.Delay(Timeout.Infinite, cancellationToken).ConfigureAwait(false);
            }
            catch (FabricNotPrimaryException)
            {
                //// do nothing, just exit
            }
            catch (FabricObjectClosedException)
            {
                //// do nothing, just exit
            }

            GatewayResourceManagerTrace.TraceSource.WriteInfo(GatewayResourceManagerTraceType, "Exit RunAsync");
        }

        private ICommunicationListener CreateCommunicationListener(ServiceContext serviceContext)
        {
            GatewayResourceManagerTrace.TraceSource.WriteInfo(GatewayResourceManagerTraceType, "Enter CreateCommunicationListener, serviceInitializationParameters is not null {0}", serviceContext == null ? false : true);
            if (serviceContext != null)
            {
                GatewayResourceManagerTrace.TraceSource.WriteInfo(GatewayResourceManagerTraceType, "Enter CreateCommunicationListener, partition id is {0}", serviceContext.PartitionId);
                GatewayResourceManagerTrace.TraceSource.WriteInfo(GatewayResourceManagerTraceType, "Enter CreateCommunicationListener, service name is {0}", serviceContext.ServiceName);
            }

            this.gatewayResourceManagerImpl = new GatewayResourceManagerImpl(StateManager, serviceContext);
            var listener = new CommunicationListener(serviceContext) { Impl = this.gatewayResourceManagerImpl };
            return listener;
        }

        private async Task InitializeAsync(CancellationToken cancellationToken)
        {
            await Task.Delay(1);
        }
    }
}
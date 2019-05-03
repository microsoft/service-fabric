// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Server
{
    using System;
    using System.Fabric;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using EventStore.Service.LogProvider;
    using Microsoft.ServiceFabric.Services.Communication.Runtime;

    internal class OwinCommunicationListener : ICommunicationListener
    {
        private readonly EventStoreRuntime currentRuntime;
        private readonly ServiceContext serviceContext;
        private OwinHttpServer owinHttpServer;

        public OwinCommunicationListener(EventStoreRuntime currentRuntime, ServiceContext serviceContext)
        {
            Assert.IsNotNull(currentRuntime, "currentRuntime != null");
            this.currentRuntime = currentRuntime;
            this.serviceContext = serviceContext;
        }

        public Task<string> OpenAsync(CancellationToken cancellationToken)
        {
            var endpoint = this.serviceContext.CodePackageActivationContext.GetEndpoint("RestEndpoint");
            var fabricSecuritySetting = SecuritySetting.GetClusterSecurityDetails();

            var listeningAddress = String.Format(CultureInfo.InvariantCulture, "{0}://+:{1}/", fabricSecuritySetting.EndpointProtocol, endpoint.Port);
            var publishUri = listeningAddress.Replace("+", FabricRuntime.GetNodeContext().IPAddressOrFQDN);

            this.owinHttpServer = new OwinHttpServer(this.currentRuntime, fabricSecuritySetting, cancellationToken);
            this.owinHttpServer.StartOwinHttpServer(listeningAddress);

            EventStoreLogger.Logger.LogMessage("Uri: {0}", publishUri);

            return Task.FromResult(publishUri);
        }

        public Task CloseAsync(CancellationToken cancellationToken)
        {
            this.owinHttpServer.StopOwinHttpServer();
            return Task.FromResult(true);
        }

        public void Abort()
        {
            this.owinHttpServer.StopOwinHttpServer();
        }
    }
}
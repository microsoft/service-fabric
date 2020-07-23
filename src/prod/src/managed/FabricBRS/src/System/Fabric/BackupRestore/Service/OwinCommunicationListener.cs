// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Service
{
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.BackupRestore.BackupRestoreEndPoints;
    using Microsoft.ServiceFabric.Services.Communication.Runtime;
    using Microsoft.ServiceFabric.Services.Runtime;

    public class OwinCommunicationListener : ICommunicationListener
    {
        private readonly StatefulService  statefulService;
        private readonly ServiceContext serviceContext;
        private OwinHttpServer owinHttpServer;

        public OwinCommunicationListener(StatefulService statefulService, ServiceContext serviceContext)
        {
            this.statefulService = statefulService;
            this.serviceContext = serviceContext;

        }
        public Task<string> OpenAsync(CancellationToken cancellationToken)
        {
            var endpoint = this.serviceContext.CodePackageActivationContext.GetEndpoint("RestEndpoint");
            var fabricBrsSecuritySetting =  SecuritySetting.GetClusterSecurityDetails();

            var listeningAddress = String.Format(CultureInfo.InvariantCulture, "{0}://+:{1}/", fabricBrsSecuritySetting.EndpointProtocol, endpoint.Port);
            var publishUri = listeningAddress.Replace("+", FabricRuntime.GetNodeContext().IPAddressOrFQDN);

            this.owinHttpServer = new OwinHttpServer(this.statefulService, fabricBrsSecuritySetting, cancellationToken);
            this.owinHttpServer.StartOwinHttpServer(listeningAddress);
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
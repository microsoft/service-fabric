// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.GatewayResourceManager.Service
{
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Communication.Runtime;

    public class CommunicationListener : ICommunicationListener
    {
        private StatefulServiceContext serviceContext;

        public CommunicationListener(ServiceContext serviceContext)
        {
            this.serviceContext = serviceContext as StatefulServiceContext;
        }

        internal GatewayResourceManagerImpl Impl { get; set; }

        public Task<string> OpenAsync(CancellationToken cancellationToken)
        {
            var serviceEndPoint = Program.ServiceAgent.RegisterGatewayResourceManager(
                this.serviceContext.PartitionId,
                this.serviceContext.ReplicaId,
                this.Impl);
            return Task.FromResult(serviceEndPoint);
        }

        public Task CloseAsync(CancellationToken cancellationToken)
        {
            Program.ServiceAgent.UnregisterGatewayResourceManager(
                this.serviceContext.PartitionId,
                this.serviceContext.ReplicaId);

            return Task.FromResult(0);
        }

        public void Abort()
        {
            this.CloseAsync(CancellationToken.None).Wait();
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service
{
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Communication.Runtime;

    public class FASCommunicationListener : ICommunicationListener
    {
        private StatefulServiceContext serviceContext;

        public FASCommunicationListener(ServiceContext serviceContext)
        {
            this.serviceContext = serviceContext as StatefulServiceContext;
        }

        internal FaultAnalysisServiceImpl Impl { get; set; }

        public Task<string> OpenAsync(CancellationToken cancellationToken)
        {
            var serviceEndPoint = Program.ServiceAgent.RegisterFaultAnalysisService(
                this.serviceContext.PartitionId,
                this.serviceContext.ReplicaId,
                this.Impl);
            return Task.FromResult(serviceEndPoint);
        }

        public Task CloseAsync(CancellationToken cancellationToken)
        {
            Program.ServiceAgent.UnregisterFaultAnalysisService(
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
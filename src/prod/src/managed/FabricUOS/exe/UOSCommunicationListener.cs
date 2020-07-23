// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeOrchestration.Service
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Communication.Runtime;

    internal class UOSCommunicationListener : ICommunicationListener
    {
        private readonly StatefulServiceContext serviceContext;
        private readonly IUpgradeOrchestrationService service;

        public UOSCommunicationListener(StatefulServiceContext serviceContext, IUpgradeOrchestrationService service)
        {
            this.serviceContext = serviceContext;
            this.service = service;
        }

        public void Abort()
        {
            this.CloseAsync(CancellationToken.None).Wait();
        }

        public Task CloseAsync(Threading.CancellationToken cancellationToken)
        {
            Program.ServiceAgent.UnregisterService(
                this.serviceContext.PartitionId,
                this.serviceContext.ReplicaId);

            return Task.FromResult(0);
        }

        public Task<string> OpenAsync(Threading.CancellationToken cancellationToken)
        {
            var serviceEndPoint = Program.ServiceAgent.RegisterService(
                                      this.serviceContext.PartitionId,
                                      this.serviceContext.ReplicaId,
                                      this.service);
            return Task.FromResult(serviceEndPoint);
        }
    }
}
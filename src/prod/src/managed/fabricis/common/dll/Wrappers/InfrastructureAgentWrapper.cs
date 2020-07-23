// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Fabric.Description;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Collections.ObjectModel;
    using System.Fabric.Query;

    internal class InfrastructureAgentWrapper : IInfrastructureAgentWrapper
    {
        private readonly FabricInfrastructureServiceAgent agent = new FabricInfrastructureServiceAgent();

        public void RegisterInfrastructureServiceFactory(IStatefulServiceFactory factory)
        {
            agent.RegisterInfrastructureServiceFactory(factory);
        }

        public string RegisterInfrastructureService(Guid partitionId, long replicaId, IInfrastructureService service)
        {
            return agent.RegisterInfrastructureService(partitionId, replicaId, service);
        }

        public void UnregisterInfrastructureService(Guid partitionId, long replicaId)
        {
            agent.UnregisterInfrastructureService(partitionId, replicaId);
        }

        public Task StartInfrastructureTaskAsync(InfrastructureTaskDescription description, TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return agent.StartInfrastructureTaskAsync(description, timeout, cancellationToken);            
        }

        public Task FinishInfrastructureTaskAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return agent.FinishInfrastructureTaskAsync(taskId, instanceId, timeout, cancellationToken);            
        }

        public Task<InfrastructureTaskQueryResult> QueryInfrastructureTaskAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return agent.QueryInfrastructureTaskAsync(timeout, cancellationToken);
        }
    }
}
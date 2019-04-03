// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System;
    using System.Fabric;
    using Description;
    using Query;
    using Threading;
    using Threading.Tasks;

    internal sealed class NullInfrastructureAgent : IInfrastructureAgentWrapper
    {
        public void RegisterInfrastructureServiceFactory(IStatefulServiceFactory factory)
        {
            // Nothing to do; not used when InfrastructureService is run as a user application
        }

        public string RegisterInfrastructureService(Guid partitionId, long replicaId, IInfrastructureService service)
        {
            // This endpoint, returned from ChangeRole, is not used when InfrastructureService is run as a user application
            return "{0}:{1}".ToString(partitionId, replicaId);
        }

        public void UnregisterInfrastructureService(Guid partitionId, long replicaId)
        {
            // Nothing to do
        }

        public Task StartInfrastructureTaskAsync(InfrastructureTaskDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task FinishInfrastructureTaskAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<InfrastructureTaskQueryResult> QueryInfrastructureTaskAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }
    }
}
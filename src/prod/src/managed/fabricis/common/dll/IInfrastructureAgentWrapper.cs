// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Fabric.Description;
    using System.Fabric.Query;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Wrapper interface primarily for <see cref="FabricInfrastructureServiceAgent"/> for mock/test purposes.
    /// </summary>
    internal interface IInfrastructureAgentWrapper
    {
        void RegisterInfrastructureServiceFactory(IStatefulServiceFactory factory);

        string RegisterInfrastructureService(Guid partitionId, long replicaId, IInfrastructureService service);

        void UnregisterInfrastructureService(Guid partitionId, long replicaId);

        Task StartInfrastructureTaskAsync(InfrastructureTaskDescription description, TimeSpan timeout,
            CancellationToken cancellationToken);

        Task FinishInfrastructureTaskAsync(string taskId, long instanceId, TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<InfrastructureTaskQueryResult> QueryInfrastructureTaskAsync(TimeSpan timeout,
            CancellationToken cancellationToken);
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Collections.Generic;
    using System.Fabric.Repair;
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;

    internal interface IRestartPolicy
    {
        IHealthCheck HealthCheck { get; }

        IPolicyStore PolicyStore { get; }

        Task InitializeAsync(CancellationToken token);

        Task<IList<RepairTask>> GetRepairTaskToApproveAsync(CancellationToken token);
    }

    internal interface IRestartPolicyFactory
    {
        IRestartPolicy Create();
    }
}
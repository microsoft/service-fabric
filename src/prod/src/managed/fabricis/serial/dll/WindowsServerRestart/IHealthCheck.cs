// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Threading;
    using System.Threading.Tasks;

    internal interface IHealthCheck
    {
        // TRUE: If healthy to proceed
        // FALSE: If not healthy
        Task<bool> PerformCheckAsync(CancellationToken token);
    }

    internal interface IHealthCheckFactory
    {
        IHealthCheck Create();
    }
}
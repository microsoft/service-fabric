// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Threading.Tasks;

    /// <summary>
    /// A policy that could be applied to change the <see cref="IAction"/> acting on tenant jobs, repair tasks.
    /// </summary>
    internal interface IActionPolicy
    {
        Task ApplyAsync(Guid activityId, CoordinatorContext coordinatorContext);

        void Reset();
    }
}
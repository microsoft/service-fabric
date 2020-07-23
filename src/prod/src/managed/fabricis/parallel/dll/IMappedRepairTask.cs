// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    internal interface IMappedRepairTask : IActionableWorkItem
    {
        string Id { get; }

        IRepairTask RepairTask { get; set; }

        /// <summary>
        /// The tenant job that this object wraps.
        /// </summary>
        ITenantJob TenantJob { get; set; }
    }
}
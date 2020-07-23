// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    internal interface IAllowActionRecord
    {
        Guid JobId { get; }

        uint? UD { get; }

        DateTimeOffset CreationTime { get; }

        ActionType Action { get; }
    }
}
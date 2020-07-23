// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using System.Collections.Generic;

    internal interface IAllowActionMap
    {
        void Set(IAllowActionRecord record);

        void Remove(Guid jobId);

        IAllowActionRecord Get(Guid jobId, uint? ud);

        IReadOnlyList<IAllowActionRecord> Get();

        void Clear();
    }
}
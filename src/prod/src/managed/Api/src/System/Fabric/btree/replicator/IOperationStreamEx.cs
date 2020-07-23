// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Replicator
{
    using System.Threading;
    using System.Threading.Tasks;

    public interface IOperationStreamEx
    {
        Task<IOperationEx> GetOperationAsync(CancellationToken cancellationToken);
    }
}
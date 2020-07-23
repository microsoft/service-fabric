// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Replicator
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;

    public interface IOperationDataStreamEx
    {
        /// <summary>
        /// Get the next piece of data from the stream
        /// </summary>
        /// <param name="cancellationToken">Cancellation token for the operation</param>
        /// <returns>A Task.</returns>
        [SuppressMessage(FxCop.Category.MSInternal, FxCop.Rule.UseApprovedGenericsForPrecompiledAssemblies, Justification = "Not precompiled assembly.")]
        Task<IOperationData> GetNextAsync(CancellationToken cancellationToken);
    }
}
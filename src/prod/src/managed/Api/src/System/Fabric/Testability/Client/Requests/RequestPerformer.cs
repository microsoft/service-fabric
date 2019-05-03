// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System.Fabric.Testability.Common;
    using System.Threading;
    using System.Threading.Tasks;

    internal class RequestPerformer : IStrategy
    {
        public RequestPerformer()
        {
        }

        public async Task PerformAsync(IRetryContext context, CancellationToken cancellationToken)
        {
            ThrowIf.Null(context, "context");
            await context.Request.PerformAsync(cancellationToken);
            context.Request.ProcessResult(context);
        }
    }
}


#pragma warning restore 1591
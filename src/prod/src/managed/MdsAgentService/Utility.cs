// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMdsAgentSvc
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;

    internal static class Utility
    {
        internal static Task<TResult> CreateCompletedTask<TResult>(TResult result)
        {
            var tcs = new TaskCompletionSource<TResult>();
            tcs.SetResult(result);
            return tcs.Task;
        }
    }
}
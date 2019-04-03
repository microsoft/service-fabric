// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Common
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;

    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;

    internal static class TaskUtility
    {
        private static readonly Task CompletedVoidTask = CreateCompletedTask<bool>(true);

        public static Task CreateCompletedTask()
        {
            return CompletedVoidTask;
        }

        public static Task<TResult> CreateCompletedTask<TResult>(TResult result)
        {
            return Task.FromResult<TResult>(result);
        }

        public static Task CreateFaultedTask(Exception exception)
        {
            return CreateFaultedTask<bool>(exception);
        }

        public static Task<TResult> CreateFaultedTask<TResult>(Exception exception)
        {
            TaskCompletionSource<TResult> tcs = new TaskCompletionSource<TResult>();
            tcs.SetException(exception);
            return tcs.Task;
        }
    }
}


#pragma warning restore 1591
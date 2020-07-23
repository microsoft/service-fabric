// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Threading;
using System.Threading.Tasks;

namespace System.Fabric.InfrastructureService
{
    internal static class TaskUtility
    {
        /// <summary>
        /// Creates a continuation of the antecedent task.  If the antecedent completes successfully,
        /// the continuation function is called synchronously.  If the antecedent faults, the continuation task
        /// will fault too, preserving the antecedent's exceptions.  If the antecedent is cancelled, the
        /// continuation task will be cancelled too.
        /// </summary>
        /// <remarks>
        /// Avoids losing the original exception stack trace, and avoids creating nested chains of AggregateExceptions,
        /// which can bury the original HRESULT and make COM interop return a less specific error.
        /// </remarks>
        public static Task<TResult> Then<TResult>(this Task antecedent, Func<Task, TResult> continuationFunction)
        {
            TaskCompletionSource<TResult> tcs = new TaskCompletionSource<TResult>();

            antecedent.ContinueWith(t =>
            {
                if (t.IsFaulted)
                {
                    tcs.SetException(t.Exception.InnerExceptions);
                }
                else if (t.IsCanceled)
                {
                    tcs.SetCanceled();
                }
                else
                {
                    try
                    {
                        TResult result = continuationFunction(t);
                        tcs.SetResult(result);
                    }
                    catch (Exception e)
                    {
                        tcs.SetException(e);
                    }
                }
            },
                TaskContinuationOptions.ExecuteSynchronously);

            return tcs.Task;
        }

        /// <summary>
        /// Creates a task that completes when the given WaitHandle is signalled.
        /// </summary>
        /// <param name="handle">The WaitHandle to register.</param>
        /// <returns></returns>
        public static Task WaitAsync(WaitHandle handle)
        {
            TaskCompletionSource<object> tcs = new TaskCompletionSource<object>();

            // Discarding the returned RegisteredWaitHandle here, because it is tricky to pass
            // it to the callback safely, and executeOnlyOnce == true anyway, so manual unregistration
            // is not strictly required.
            ThreadPool.RegisterWaitForSingleObject(
                handle,
                (state, timedOut) => tcs.SetResult(null),
                null,
                Timeout.Infinite,
                true); // executeOnlyOnce

            return tcs.Task;
        }

        /// <summary>
        /// Creates a task that completes after the given time delay.
        /// </summary>
        public static Task SleepAsync(TimeSpan delay)
        {
            return Task.Delay(delay);
        }

        /// <summary>
        /// Creates a task which throws the given exception.
        /// </summary>
        public static Task<string> ThrowAsync(Exception e)
        {
            return Task.Factory.StartNew<string>(() => { throw e; });
        }
    }
}
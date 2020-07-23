// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Threading.Tasks;
    using Common;
    using Threading;
    using Linq;

    internal class LinearRetryPolicy : IRetryPolicy
    {
        private readonly TraceType traceType;

        public LinearRetryPolicy(
            TraceType traceType,
            TimeSpan deltaBackoff,
            int maxAttempts)
        {
            traceType.ThrowIfNull("traceType");

            this.traceType = traceType;
            this.DeltaBackoff = deltaBackoff;
            this.MaxAttempts = maxAttempts;
        }

        public TimeSpan DeltaBackoff { get; private set; }

        public int MaxAttempts { get; private set; }

        public Func<Exception, bool> ShouldRetry { get; set; }

        public T Execute<T>(Func<T> func, string funcName)
        {
            for (int i = 0; i < MaxAttempts; i++)
            {
                bool retry = false;

                try
                {
                    T result = func();
                    return result;
                }
                catch (AggregateException ae)
                {
                    AggregateException flatEx = ae.Flatten();

                    string messages = string.Join(", ", flatEx.InnerExceptions.Select(e => e.Message));

                    Trace.WriteWarning(
                        traceType, 
                        "RetryPolicy: {0} catching aggregating exception to determine if retry is allowed. Consolidated error: {1}", 
                        funcName, 
                        messages);

                    int attemptCount = i;

                    flatEx.Handle(e =>
                    {
                        retry = IsRetryAllowed(attemptCount, e, funcName);
                        return retry;
                    });
                }
                catch (Exception e)
                {
                    Trace.WriteWarning(
                        traceType,
                        "RetryPolicy: {0} catching exception to determine if retry is allowed. Error: {1}",
                        funcName,
                        e);

                    retry = IsRetryAllowed(i, e, funcName);

                    if (!retry)
                    {
                        throw;
                    }
                }

                if (retry)
                {
                    Thread.Sleep(DeltaBackoff);
                }
            }

            return default(T);
        }

        public async Task<T> ExecuteAsync<T>(Func<CancellationToken, Task<T>> func, string funcName, CancellationToken token)
        {
            T result = default(T);
            for (int i = 0; i < this.MaxAttempts; i++)
            {
                bool retry = false;
                result = await func(token).ContinueWith(
                    task =>
                    {
                        token.ThrowIfCancellationRequested();
                        if (!task.IsFaulted && !task.IsCanceled)
                        {
                            return task.Result;
                        }

                        if (task.Exception != null)
                        {
                            retry = this.HandleException(i, task.Exception, funcName);
                            if (!retry)
                            {
                                throw task.Exception;
                            }
                        }
                        else
                        {
                            retry = true;
                        }

                        return default(T);
                    },
                    token);

                if (retry)
                {
                    await Task.Delay(this.DeltaBackoff, token);
                }
                else
                {
                    break;
                }
            }

            return result;
        }
       
        // Check if exception is retriable exception or not. In case exception is AggregateException flatten it
        // before checking it.
        private bool HandleException(int attemptCount, Exception exception, string methodName)
        {
            AggregateException ae = exception as AggregateException;
            bool handled = false;
            if (ae != null)
            {
                var flatEx = ae.Flatten();
                string messages = string.Join(", ", flatEx.InnerExceptions.Select(e => e.Message));

                Trace.WriteWarning(
                    this.traceType,
                    "RetryPolicy: {0} catching aggregating exception to determine if retry is allowed. Consolidated error: {1}",
                    methodName,
                    messages);

                flatEx.Handle(
                    e =>
                    {
                        handled = this.IsRetryAllowed(attemptCount, e, methodName);
                        return handled;
                    });
            }
            else
            {
                handled = this.IsRetryAllowed(attemptCount, exception, methodName);
            }

            return handled;
        }

        /// <summary>
        /// Determines whether retry allowed is allowed by checking if the exception is retry-able. And if it is,
        /// whether the max. attempts have reached.
        /// </summary>
        /// <param name="attemptCount">The attempt count.</param>
        /// <param name="exception">The exception.</param>
        /// <param name="methodName">Name of the method.</param>
        /// <returns><c>true</c> if retry is allowed. <c>false</c> otherwise.</returns>
        private bool IsRetryAllowed(int attemptCount, Exception exception, string methodName)
        {
            if (!ShouldRetry(exception))
            {
                Trace.WriteWarning(
                    traceType,
                    "RetryPolicy: {0} unknown exception in attempt {1} of {2}. Not retrying further. Error: {3}",
                    methodName,
                    attemptCount + 1,
                    MaxAttempts,
                    exception);

                return false;
            }

            if (attemptCount == MaxAttempts - 1)
            {
                Trace.WriteWarning(
                    traceType,
                    "RetryPolicy: All {0} retry attempts exhausted in {1}. Error: {2}",
                    MaxAttempts,
                    methodName,
                    exception);
                
                return false;
            }

            Trace.WriteInfo(
                traceType,
                "RetryPolicy: Failed in {0} in attempt {1} of {2}. Retrying in {3}s. Error: {4}",
                methodName,
                attemptCount + 1,
                MaxAttempts,
                DeltaBackoff.TotalSeconds,
                exception);
                
            return true;
        }
    }
}
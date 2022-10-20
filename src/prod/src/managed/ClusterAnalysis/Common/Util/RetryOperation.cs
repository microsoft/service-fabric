// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Util
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using ClusterAnalysis.Common.Log;

    public class RetryOperation
    {
        private ILogger logger;

        public RetryOperation(ILogger logger, TimeSpan deltaBackoff, int maxAttempts, Func<Exception, bool> retryPolicy)
        {
            Assert.IsNotNull(retryPolicy);
            Assert.IsNotNull(logger);
            this.logger = logger;
            this.DeltaBackoff = deltaBackoff;
            this.MaxAttempts = maxAttempts;
            this.ShouldRetry = retryPolicy;
        }

        public TimeSpan DeltaBackoff { get; }

        public int MaxAttempts { get; }

        public Func<Exception, bool> ShouldRetry { get; }

        public async Task<T> ExecuteAsync<T>(Func<CancellationToken, Task<T>> func, string funcName, CancellationToken token)
        {
            Assert.IsNotNull(func, "func can't be null");

            T result = default(T);
            for (int i = 0; i < this.MaxAttempts; i++)
            {
                bool retry = false;
                var localCopy = i;
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
                            retry = this.HandleException(localCopy, task.Exception, funcName);
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
                    await Task.Delay(this.DeltaBackoff, token).ConfigureAwait(false);
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
            if (!this.ShouldRetry(exception))
            {
                this.logger.LogMessage("RetryOperation: Exception: {0} is not Retryable", exception);
                return false;
            }

            if (attemptCount != this.MaxAttempts - 1)
            {
                this.logger.LogMessage("RetryOperation: Retrying after Exception: {0}", exception);
                return true;
            }

            this.logger.LogMessage("RetryOperation: All {0} retry attempts exhausted in {1}. Error: {2}", this.MaxAttempts, methodName, exception);
            return false;
        }
    }
}
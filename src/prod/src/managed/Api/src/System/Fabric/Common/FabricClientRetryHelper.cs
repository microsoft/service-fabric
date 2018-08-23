// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Diagnostics;
    using System.Fabric;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;

    internal static class FabricClientRetryHelper
    {
        private const string TraceType = "FabricClientRetryHelper";
        private static readonly TimeSpan DefaultOperationTimeout = TimeSpan.FromMinutes(5.0);

        public static async Task ExecuteFabricActionWithRetryAsync(Func<Task> action)
        {
            await ExecuteFabricActionWithRetryAsync(action, new FabricClientRetryErrors(), CancellationToken.None).ConfigureAwait(false);
        }

        public static async Task ExecuteFabricActionWithRetryAsync(Func<Task> action, TimeSpan operationTimeout)
        {
            await ExecuteFabricActionWithRetryAsync(action, new FabricClientRetryErrors(), operationTimeout, CancellationToken.None).ConfigureAwait(false);
        }

        public static async Task ExecuteFabricActionWithRetryAsync(Func<Task> action, FabricClientRetryErrors errors)
        {
            await ExecuteFabricActionWithRetryAsync(action, errors, DefaultOperationTimeout, CancellationToken.None).ConfigureAwait(false);
        }

        public static async Task ExecuteFabricActionWithRetryAsync(Func<Task> action, FabricClientRetryErrors errors, TimeSpan operationTimeout)
        {
            await ExecuteFabricActionWithRetryAsync(action, errors, DefaultOperationTimeout, CancellationToken.None).ConfigureAwait(false);
        }

        public static async Task ExecuteFabricActionWithRetryAsync(Func<Task> action, CancellationToken cancellationToken)
        {
            await ExecuteFabricActionWithRetryAsync(action, new FabricClientRetryErrors(), DefaultOperationTimeout, cancellationToken).ConfigureAwait(false);
        }

        public static async Task ExecuteFabricActionWithRetryAsync(Func<Task> action, TimeSpan operationTimeout, CancellationToken cancellationToken)
        {
            await ExecuteFabricActionWithRetryAsync(action, new FabricClientRetryErrors(), operationTimeout, cancellationToken).ConfigureAwait(false);
        }

        public static async Task ExecuteFabricActionWithRetryAsync(Func<Task> action, FabricClientRetryErrors errors, CancellationToken cancellationToken)
        {
            await ExecuteFabricActionWithRetryAsync(action, errors, DefaultOperationTimeout, cancellationToken).ConfigureAwait(false);
        }

        public static async Task ExecuteFabricActionWithRetryAsync(Func<Task> action, FabricClientRetryErrors errors, TimeSpan operationTimeout, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter ExecuteFabricActionWithRetryAsync() with action");
            Stopwatch watch = new Stopwatch();
            watch.Start();
            while (true)
            {
                cancellationToken.ThrowIfCancellationRequested();

                try
                {
                    await action().ConfigureAwait(false);
                    return;
                }
                catch (Exception e)
                {
                    bool retryElseSuccess;
                    if (HandleException(e, errors, out retryElseSuccess))
                    {
                        if (retryElseSuccess)
                        {
                            if (watch.Elapsed > operationTimeout)
                            {
                                throw;
                            }
                        }
                        else
                        {
                            // Exception indicates success for API hence return
                            return;
                        }
                    }
                    else
                    {
                        // Could not handle exception hence throwing
                        throw;
                    }
                }

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "ExecuteFabricActionWithRetryAsync() with action: Going to wait for 5 seconds before retry");

                // If we get here it means that we need to retry. Sleep for 5 seconds
                // before next iteration
                await Task.Delay(TimeSpan.FromSeconds(5), cancellationToken).ConfigureAwait(false);
            }
        }

        public static async Task<T> ExecuteFabricActionWithRetryAsync<T>(Func<Task<T>> function)
        {
            return await ExecuteFabricActionWithRetryAsync<T>(function, new FabricClientRetryErrors()).ConfigureAwait(false);
        }

        public static async Task<T> ExecuteFabricActionWithRetryAsync<T>(Func<Task<T>> function, TimeSpan operationTimeout)
        {
            return await ExecuteFabricActionWithRetryAsync<T>(function, new FabricClientRetryErrors(), operationTimeout).ConfigureAwait(false);
        }

        public static async Task<T> ExecuteFabricActionWithRetryAsync<T>(Func<Task<T>> function, FabricClientRetryErrors errors)
        {
            return await ExecuteFabricActionWithRetryAsync<T>(function, errors, DefaultOperationTimeout).ConfigureAwait(false);
        }

        public static async Task<T> ExecuteFabricActionWithRetryAsync<T>(Func<Task<T>> function, FabricClientRetryErrors errors, TimeSpan operationTimeout)
        {
            return await ExecuteFabricActionWithRetryAsync<T>(function, errors, operationTimeout, CancellationToken.None).ConfigureAwait(false);
        }

        public static async Task<T> ExecuteFabricActionWithRetryAsync<T>(Func<Task<T>> function, CancellationToken cancellationToken)
        {
            return await ExecuteFabricActionWithRetryAsync<T>(function, new FabricClientRetryErrors(), CancellationToken.None).ConfigureAwait(false);
        }

        public static async Task<T> ExecuteFabricActionWithRetryAsync<T>(Func<Task<T>> function, TimeSpan operationTimeout, CancellationToken cancellationToken)
        {
            return await ExecuteFabricActionWithRetryAsync<T>(function, new FabricClientRetryErrors(), operationTimeout, CancellationToken.None).ConfigureAwait(false);
        }

        public static async Task<T> ExecuteFabricActionWithRetryAsync<T>(Func<Task<T>> function, FabricClientRetryErrors errors, CancellationToken cancellationToken)
        {
            return await ExecuteFabricActionWithRetryAsync<T>(function, errors, DefaultOperationTimeout).ConfigureAwait(false);
        }

        public static async Task<T> ExecuteFabricActionWithRetryAsync<T>(Func<Task<T>> function, FabricClientRetryErrors errors, TimeSpan operationTimeout, CancellationToken cancellationToken)
        {
          TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter ExecuteFabricActionWithRetryAsync() with function");

            Stopwatch watch = new Stopwatch();
            watch.Start();
            while (true)
            {
                cancellationToken.ThrowIfCancellationRequested();

                try
                {
                    return await function().ConfigureAwait(false);
                }
                catch (Exception e)
                {
                    bool retryElseSuccess;
                    if (HandleException(e, errors, out retryElseSuccess))
                    {
                        if (retryElseSuccess)
                        {
                            if (watch.Elapsed > operationTimeout)
                            {
                                throw;
                            }
                        }
                        else
                        {
                            // Exception indicates success for API hence return
                            return default(T);
                        }
                    }
                    else
                    {
                        // Could not handle exception hence throwing
                        throw;
                    }
                }

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "ExecuteFabricActionWithRetryAsync() with function: Going to wait for 5 seconds before retry");
                // If we get here it means that we need to retry. Sleep for 5 seconds
                // before next iteration
                await Task.Delay(TimeSpan.FromSeconds(5), cancellationToken).ConfigureAwait(false);
            }
        }

        private static bool HandleException(Exception e, FabricClientRetryErrors errors, out bool retryElseSuccess)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "HandleException(): e='{0}', HResult='{0}'", e.ToString(), e.HResult);

            FabricException fabricException = e as FabricException;

            if (fabricException != null)
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "HandleException(): errorCode='{0}'", fabricException.ErrorCode);
            }

            Exception currentEx = e;
            for (int i = 0; i < 5 && currentEx != null; i++)
            {
                if (errors.RetryableExceptions.Contains(currentEx.GetType()))
                {
                    retryElseSuccess = true /*retry*/;
                    return true;
                }

                currentEx = currentEx.InnerException;
            }

            if (fabricException != null && errors.RetryableFabricErrorCodes.Contains(fabricException.ErrorCode))
            {
                retryElseSuccess = true /*retry*/;
                return true;
            }

            if (errors.RetrySuccessExceptions.Contains(e.GetType()))
            {
                retryElseSuccess = false /*success*/;
                return true;
            }

            if (fabricException != null && errors.RetrySuccessFabricErrorCodes.Contains(fabricException.ErrorCode))
            {
                retryElseSuccess = false /*success*/;
                return true;
            }

            if (e.GetType() == typeof(FabricTransientException))
            {
                retryElseSuccess = true /*retry*/;
                return true;
            }

            if(fabricException != null && fabricException.InnerException != null)
            {
                var ex = fabricException.InnerException;

                if (errors.InternalRetrySuccessFabricErrorCodes.Contains((uint)ex.HResult))
                {
                    retryElseSuccess = false /*success*/;
                    return true;
                }
            }

            retryElseSuccess = false;
            return false;
        }
    }
}
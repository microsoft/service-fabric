// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Helpers
{
    using System;
    using System.Diagnostics;
    using System.Fabric;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;
    using ClusterAnalysis.Common.Log;

    /// <summary>
    /// Helper class to execute fabric client operations with retry
    /// </summary>
    public static class FabricClientRetryHelper
    {
        private static readonly TimeSpan DefaultOperationTimeout = TimeSpan.FromMinutes(2);
        
        private static readonly ILogger Logger = LocalDiskLogger.LogProvider.CreateLoggerInstance("FabricClientRetryHelper.log");

        /// <summary>
        /// Helper method to execute given function with defaultFabricClientRetryErrors and default Operation Timeout
        /// </summary>
        /// <param name="function">Action to be performed</param>
        /// <param name="cancellationToken">Cancellation token for Async operation</param>
        /// <returns>Task object</returns>
        public static async Task<T> ExecuteFabricActionWithRetryAsync<T>(Func<Task<T>> function, CancellationToken cancellationToken)
        {
            return await ExecuteFabricActionWithRetryAsync<T>(function, new FabricClientRetryErrors(), DefaultOperationTimeout, cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Helper method to execute given function with given user FabricClientRetryErrors and given Operation Timeout
        /// </summary>
        /// <param name="function">Action to be performed</param>
        /// <param name="errors">Fabric Client Errors that can be retired</param>
        /// <param name="operationTimeout">Timeout for the operation</param>
        /// <param name="cancellationToken">Cancellation token for Async operation</param>
        /// <returns>Task object</returns>
        public static async Task<T> ExecuteFabricActionWithRetryAsync<T>(
            Func<Task<T>> function,
            FabricClientRetryErrors errors,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken)
        {
            bool needToWait = false;
            Stopwatch watch = new Stopwatch();
            watch.Start();
            while (true)
            {
                cancellationToken.ThrowIfCancellationRequested();

                if (needToWait)
                {
                    await Task.Delay(TimeSpan.FromSeconds(5), cancellationToken).ConfigureAwait(false);
                    needToWait = false;
                }

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
                            Logger.LogMessage("ExecuteFabricActionWithRetryAsync:; Retrying due to Exp: {0}", e);
                            if (watch.Elapsed > operationTimeout)
                            {
                                Logger.LogMessage(
                                    "ExecuteFabricActionWithRetryAsync:; Done Retrying. Time Elapsed: {0}, Timeout: {1}. Throwing Exception: {2}",
                                    watch.Elapsed.TotalSeconds,
                                    operationTimeout.TotalSeconds,
                                    e);
                                throw;
                            }

                            needToWait = true;
                            continue;
                        }

                        Logger.LogMessage("ExecuteFabricActionWithRetryAsync:; Exception {0} Handled but No Retry", e);
                        return default(T);
                    }

                    throw;
                }
            }
        }

        private static bool HandleException(Exception e, FabricClientRetryErrors errors, out bool retryElseSuccess)
        {
            FabricException fabricException = e as FabricException;

            if (errors.RetryableExceptions.Contains(e.GetType()))
            {
                retryElseSuccess = true /*retry*/;
                return true;
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

            if (fabricException != null && fabricException.InnerException != null)
            {
                var ex = fabricException.InnerException as COMException;
                if (ex != null && errors.InternalRetrySuccessFabricErrorCodes.Contains((uint)ex.ErrorCode))
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
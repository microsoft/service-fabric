// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Hosting.ContainerActivatorService
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Threading.Tasks;

    internal static class Utility
    {
        internal static string FabricCodePath { get; private set; }
        internal static string FabricLogRoot { get; private set; }

        internal static void InitializeTracing()
        {
            TraceConfig.InitializeFromConfigStore();

            FabricCodePath = FabricEnvironment.GetCodePath();
            FabricLogRoot = FabricEnvironment.GetLogRoot();
        }

        internal static string GetDecryptedValue(string encryptedValue)
        {
            var decryptedValue = NativeConfigStore.DecryptText(encryptedValue);
            return AccountHelper.SecureStringToString(decryptedValue);
        }

        internal static Task ExecuteWithRetriesAsync(
            Func<TimeSpan, Task> func,
            string operationTag,
            string traceType,
            TimeSpan innerTimeout,
            TimeSpan totalTimeout)
        {
            return ExecuteWithRetriesAsync(
                new Func <TimeSpan, Task <object>>(
                    async (timeout) =>
                    {
                        await func.Invoke(timeout);
                        return (object)null;
                    }),
                operationTag,
                traceType,
                innerTimeout,
                totalTimeout);
        }

        internal static async Task<TResult> ExecuteWithRetriesAsync<TResult>(
            Func<TimeSpan, Task<TResult>> func,
            string operationTag,
            string traceType,
            TimeSpan innerTimeout,
            TimeSpan totalTimeout)
        {
            var retryCount = 0;
            var operationId = Guid.NewGuid();
            var timeoutHelper = new TimeoutHelper(totalTimeout);
            var effectiveTimeout = innerTimeout;

            while (true)
            {
                try
                {
                    var res = await func.Invoke(effectiveTimeout);

                    if (retryCount > 0)
                    {
                        HostingTrace.Source.WriteInfo(
                            traceType,
                            "ExecuteWithRetriesAsync: OperationTag={0}, OperationId={1} completed with RetryCount={2}.",
                            operationTag,
                            operationId,
                            retryCount);
                    }

                    return res;
                }
                catch (FabricException ex)
                {
                    if(timeoutHelper.HasTimedOut || 
                       ex.ErrorCode != FabricErrorCode.OperationTimedOut)
                    {
                        throw;
                    }
                }

                retryCount++;

                HostingTrace.Source.WriteWarning(
                    traceType,
                    "ExecuteWithRetriesAsync: OperationTag={0}, OperationId={1}, RetryCount={2}.",
                    operationTag,
                    operationId,
                    retryCount);

                await Task.Delay(TimeSpan.FromSeconds(1));

                effectiveTimeout = TimeSpan.FromTicks(
                    Math.Min(timeoutHelper.RemainingTime.Ticks, innerTimeout.Ticks));
            }
        }
    }
}
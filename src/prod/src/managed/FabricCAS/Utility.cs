// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Hosting.ContainerActivatorService
{
    using Microsoft.ServiceFabric.ContainerServiceClient;
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Text;
    using System.Threading.Tasks;

    internal static class Utility
    {
        internal static string FabricCodePath { get; private set; }
        internal static string FabricLogRoot { get; private set; }
        internal static string FabricDataRoot { get; private set; }

        internal static void InitializeTracing()
        {
            TraceConfig.InitializeFromConfigStore();

            FabricCodePath = FabricEnvironment.GetCodePath();
            FabricLogRoot = FabricEnvironment.GetLogRoot();
            FabricDataRoot = FabricEnvironment.GetLogRoot();
        }

        internal static string GetDecryptedValue(string encryptedValue)
        {
            var decryptedValue = NativeConfigStore.DecryptText(encryptedValue);
            return AccountHelper.SecureStringToString(decryptedValue);
        }

        internal static Task ExecuteWithRetryOnTimeoutAsync(
            Func<TimeSpan, Task> func,
            string operationTag,
            string traceType,
            TimeSpan innerTimeout,
            TimeSpan totalTimeout)
        {
            return ExecuteWithRetryOnTimeoutAsync(
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

        internal static async Task<TResult> ExecuteWithRetryOnTimeoutAsync<TResult>(
            Func<TimeSpan, Task<TResult>> func,
            string operationTag,
            string traceType,
            TimeSpan innerTimeout,
            TimeSpan totalTimeout)
        {
            var retryCount = 0;
            var operationId = Guid.NewGuid();
            var timeoutHelper = new TimeoutHelper(totalTimeout);
            var effectiveTimeout = HostingConfig.Config.DisableDockerRequestRetry ? totalTimeout : innerTimeout;

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
                    if(HostingConfig.Config.DisableDockerRequestRetry || 
                       timeoutHelper.HasTimedOut || 
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

                await Task.Delay(TimeSpan.FromSeconds(5));

                effectiveTimeout = TimeSpan.FromTicks(
                    Math.Min(timeoutHelper.RemainingTime.Ticks, innerTimeout.Ticks));
            }
        }

        internal static async Task CleanupOnErrorAsync(
            string traceType,
            bool autoRemove,
            bool isContainerRoot,
            string cgroupName,
            string containerName,
            string applicationId,
            IContainerActivatorService activatorService,
            TimeoutHelper timeoutHelper,
            FabricException originalEx)
        {
            HostingTrace.Source.WriteWarning(
                traceType,
                "CleanupOnErrorAsync(): Deactivating Container='{0}', AppId={1}.",
                containerName,
                applicationId);

            try
            {
                var deactivationArg = new ContainerDeactivationArgs()
                {
                    ContainerName = containerName,
                    ConfiguredForAutoRemove = autoRemove,
                    IsContainerRoot = isContainerRoot,
                    CgroupName = cgroupName,
                    IsGracefulDeactivation = false
                };

                await Utility.ExecuteWithRetryOnTimeoutAsync(
                    (operationTimeout) =>
                    {
                        return activatorService.DeactivateContainerAsync(
                            deactivationArg,
                            operationTimeout);
                    },
                    $"CleanupOnErrorAsync_DeactivateContainerAsync_{containerName}",
                    traceType,
                    HostingConfig.Config.DockerRequestTimeout,
                    timeoutHelper.RemainingTime);
            }
            catch (FabricException)
            {
                // Ignore.
            }

            throw originalEx;
        }

        internal static string BuildErrorMessage(
            string containerName,
            string applicationId,
            string applicationName,
            Exception ex)
        {
            var errBuilder = new StringBuilder();

            errBuilder.AppendFormat(
                "ContainerName={0}, ApplicationId={1}, ApplicationName={2}.",
                containerName,
                applicationId,
                applicationName);

            if (ex is ContainerApiException containerApiEx)
            {
                errBuilder.AppendFormat(
                     " DockerRequest returned StatusCode={0} with ResponseBody={1}.",
                    containerApiEx.StatusCode,
                    containerApiEx.ResponseBody);
            }
            else
            {
                errBuilder.Append(ex.ToString());
            }

            return errBuilder.ToString();
        }
    }
}
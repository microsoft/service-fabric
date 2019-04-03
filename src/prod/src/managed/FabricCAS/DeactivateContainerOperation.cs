// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Hosting.ContainerActivatorService
{
    using System;
    using System.Fabric;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.ContainerServiceClient;

    internal class DeactivateContainerOperation
    {
        #region Private Memebers

        private const string TraceType = "DeactivateContainerOperation";

        private readonly ContainerActivatorService activator;
        private readonly ContainerDeactivationArgs deactivationArgs;
        private readonly TimeSpan timeout;
        private readonly TimeoutHelper timeoutHelper;
        private readonly TimeSpan retryDelay;
        private readonly TimeSpan deactivationTimeout;

        private int retryCount;

        #endregion

        internal DeactivateContainerOperation(
            ContainerActivatorService activator,
            ContainerDeactivationArgs deactivationArgs,
            TimeSpan timeout)
        {
            this.activator = activator;
            this.deactivationArgs = deactivationArgs;

            //
            // NonGracefulDeactivation (i.e. abort request) should never timeout.
            //
            this.timeout = deactivationArgs.IsGracefulDeactivation ? timeout : TimeSpan.MaxValue;

            this.timeoutHelper = new TimeoutHelper(timeout);
            this.retryDelay = TimeSpan.FromSeconds(HostingConfig.Config.ContainerDeactivationRetryDelayInSec);
            this.deactivationTimeout = HostingConfig.Config.ContainerDeactivationTimeout;

            this.retryCount = 0;
        }

        internal async Task ExecuteAsync()
        {
            do
            {
                var shouldStop = 
                    (retryCount == 0) && 
                    (deactivationTimeout < timeoutHelper.RemainingTime) && 
                    deactivationArgs.IsGracefulDeactivation;

                Exception lastEx = null;

                try
                {
                    await this.StopOrKillAndRemoveContainerAsync(shouldStop);

                    HostingTrace.Source.WriteInfo(
                        TraceType,
                        "Deactivating Container='{0}' Succeeded. IsGracefulDeactivation='{1}'.",
                        this.deactivationArgs.ContainerName,
                        this.deactivationArgs.IsGracefulDeactivation);

                    return;
                }
                catch (ContainerApiException ex)
                {
                    retryCount++;
                    lastEx = ex;

                    var errorMessage = string.Format(
                        "ContainerName='{0}', IsGracefulDeactivation='{1}', IsStopRequest='{2}', RetryCount='{3}', StatucCode={4}, ResponseBody={5}.",
                        this.deactivationArgs.ContainerName,
                        this.deactivationArgs.IsGracefulDeactivation,
                        shouldStop,
                        retryCount,
                        ex.StatusCode,
                        ex.ResponseBody);

                    HostingTrace.Source.WriteWarning(TraceType, errorMessage);
                }
                catch(Exception ex)
                {
                    retryCount++;
                    lastEx = ex;

                    var errorMessage = string.Format(
                        "ContainerName='{0}', IsGracefulDeactivation='{1}', IsStopRequest='{2}', RetryCount='{3}'.",
                        this.deactivationArgs.ContainerName,
                        this.deactivationArgs.IsGracefulDeactivation,
                        shouldStop,
                        retryCount);

                    HostingTrace.Source.WriteWarning(TraceType, errorMessage);
                }

                //
                // In case of nonGracefulDeactivation (i.e. abort request), we should never
                // give up and keep retrying until container is cleaned to avoid any leak
                // of containers.
                //
                if ((!this.deactivationArgs.IsGracefulDeactivation) ||
                    (this.deactivationArgs.IsGracefulDeactivation && timeoutHelper.RemainingTime.Ticks > 0))
                {
                    await Task.Delay(retryDelay);
                }
                else
                {
                    var errMsg = string.Format(
                        "Deactivation failed for ContainerName='{0}'.",
                        this.deactivationArgs.ContainerName);

                    HostingTrace.Source.WriteError(TraceType, errMsg);
                    throw new FabricException(errMsg, lastEx);
                }

            } while (true);
        }

        #region Private Helper Methods

        private async Task StopOrKillAndRemoveContainerAsync(bool shouldStop)
        {
            var containerName = this.deactivationArgs.ContainerName;

            if (shouldStop)
            {
                await Utility.ExecuteWithRetryOnTimeoutAsync(
                    (operationTimeout) =>
                    {
                        return this.activator.Client.ContainerOperation.StopContainerAsync(
                            this.deactivationArgs.ContainerName,
                            (int)this.deactivationTimeout.TotalSeconds,
                            operationTimeout);
                    },
                    $"StopContainerAsync_{containerName}",
                    TraceType,
                    HostingConfig.Config.DockerRequestTimeout,
                    this.timeoutHelper.RemainingTime);

                // Autoremove = true means docker will clean up the containers
                // Autoremove = false means Service Fabric will manually cleanup
                // the container since docker will not.
                if(this.deactivationArgs.ConfiguredForAutoRemove == false)
                {
                    await this.RemoveContainerAsync();
                }
            }
            else
            {
                await this.RemoveContainerAsync();
            }
        }

        private Task RemoveContainerAsync()
        {
            return Utility.ExecuteWithRetryOnTimeoutAsync(
                (operationTimeout) =>
                {
                    return this.activator.Client.ContainerOperation.RemoveContainerAsync(
                        this.deactivationArgs.ContainerName,
                        true,
                        operationTimeout);
                },
                $"RemoveContainerAsync_{this.deactivationArgs.ContainerName}",
                TraceType,
                HostingConfig.Config.DockerRequestTimeout,
                this.timeoutHelper.RemainingTime);
        }

        #endregion
    }
}
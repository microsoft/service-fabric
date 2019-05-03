// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Hosting.ContainerActivatorService
{
    using Microsoft.ServiceFabric.ContainerServiceClient.Config;
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Text;
    using System.Threading.Tasks;

    internal class UpdateRoutesContainerOperation
    {
        #region Private Members

        private const string UpdateRoutesCommand = "route delete 0.0.0.0 {0}&route add 0.0.0.0 MASK 0.0.0.0 {1} METRIC 6000";

        private readonly string traceType;

        private readonly ContainerActivatorService activator;
        private readonly TimeoutHelper timeoutHelper;

        private string containerId;
        private string containerName;
        private string applicationId;
        private string applicationName;
        private string cgroupName;
        private List<string> gatewayIpAddresses;
        private bool autoRemove;
        private bool isContainerRoot;
        private ContainerNetworkType networkType;

        #endregion

        public UpdateRoutesContainerOperation(
            string traceType,
            string containerId,
            string containerName,
            string applicationId,
            string applicationName,
            string cgroupName,
            List<string> gatewayIpAddresses,
            bool autoRemove,
            bool isContainerRoot,
            ContainerNetworkType networkType,
            ContainerActivatorService activator,
            TimeoutHelper timeoutHelper)
        {
            this.traceType = traceType;
            this.containerId = containerId;
            this.containerName = containerName;
            this.applicationId = applicationId;
            this.applicationName = applicationName;
            this.cgroupName = cgroupName;
            this.gatewayIpAddresses = gatewayIpAddresses;
            this.autoRemove = autoRemove;
            this.isContainerRoot = isContainerRoot;
            this.networkType = networkType;
            this.activator = activator;
            this.timeoutHelper = timeoutHelper;
        }

        #region Protected Helper Methods

#if !DotNetCoreClrLinux
        internal async Task ExecuteUpdateRouteCommandAsync()
        {
            var strb = new StringBuilder();
            foreach (var gatewayIpAddress in this.gatewayIpAddresses)
            {
                var command = string.Format(UpdateRoutesCommand, gatewayIpAddress, gatewayIpAddress);
                if (strb.Length > 0)
                {
                    strb.AppendFormat("&{0}", command);
                }
                else
                {
                    strb.Append(command);
                }
            }

            // Route changes on RS3 builds need to be executed with higher privilege.
            ContainerExecConfig execConfig = new ContainerExecConfig()
            {
                AttachStdin = true,
                AttachStdout = true,
                AttachStderr = true,
                Privileged = false,
                Tty = true,
                Cmd = new List<string>() { "cmd.exe", "/c", strb.ToString() },
                User = "ContainerAdministrator"
            };

            FabricException opEx = null;
            opEx = await ExecuteUpdateRouteCommandAsync(execConfig);

            if (opEx != null)
            {
                // reset opEx so that we can deactivate container if second try fails.
                opEx = null;

                // Execute without higher privilege.
                execConfig = new ContainerExecConfig()
                {
                    AttachStdin = true,
                    AttachStdout = true,
                    AttachStderr = true,
                    Privileged = false,
                    Tty = true,
                    Cmd = new List<string>() { "cmd.exe", "/c", strb.ToString() }
                };

                opEx = await ExecuteUpdateRouteCommandAsync(execConfig);

                if (opEx != null)
                {
                    await Utility.CleanupOnErrorAsync(
                        traceType,
                        autoRemove,
                        isContainerRoot,
                        cgroupName,
                        containerName,
                        applicationId,
                        activator,
                        timeoutHelper,
                        opEx);
                }
            }
        }

        private async Task<FabricException> ExecuteUpdateRouteCommandAsync(ContainerExecConfig execConfig)
        {
            FabricException opEx = null;

            try
            {
                await ExecuteUpdateRouteCommandWithRetriesAsync(execConfig);
            }
            catch (FabricException ex)
            {
                opEx = ex;
            }

            return opEx;
        }

        private async Task ExecuteUpdateRouteCommandWithRetriesAsync(ContainerExecConfig execConfig)
        {
            try
            {
                var response = await Utility.ExecuteWithRetryOnTimeoutAsync(
                    (operationTimeout) =>
                    {
                        return this.activator.Client.ExecOperation.CreateContainerExecAsync(
                            this.containerId,
                            execConfig,
                            operationTimeout);
                    },
                    $"ExecuteUpdateRouteCommandAsync_CreateContainerExecAsync_{this.containerName}",
                    traceType,
                    HostingConfig.Config.DockerRequestTimeout,
                    this.timeoutHelper.RemainingTime);

                HostingTrace.Source.WriteNoise(
                    traceType,
                    "Setup exec command to update route for Container={0}, AppId={1} succeeded.",
                    this.containerName,
                    this.applicationId);

                await Utility.ExecuteWithRetryOnTimeoutAsync(
                    (operationTimeout) =>
                    {
                        return this.activator.Client.ExecOperation.StartContainerExecAsync(
                            response.ID,
                            operationTimeout);
                    },
                    $"ExecuteUpdateRouteCommandAsync_StartContainerExecAsync_{this.containerName}",
                    traceType,
                    HostingConfig.Config.DockerRequestTimeout,
                    this.timeoutHelper.RemainingTime);

                HostingTrace.Source.WriteNoise(
                    traceType,
                    "Command execution to update route for Container={0}, AppId={1} succeeded.",
                    this.containerName,
                    this.applicationId);
            }
            catch (Exception ex)
            {
                var errorMessage = string.Format(
                    "Set up exec command to update route failed for {0}.",
                    Utility.BuildErrorMessage(
                        containerName,
                        applicationId,
                        applicationName,
                        ex));

                HostingTrace.Source.WriteWarning(traceType, errorMessage);
                throw new FabricException(errorMessage, FabricErrorCode.InvalidOperation);
            }
        }
#endif
        #endregion
    }
}
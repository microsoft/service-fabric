// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Hosting.ContainerActivatorService
{
    using System;
    using System.Fabric;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using System.Text;
    using System.Net;
    using Microsoft.ServiceFabric.ContainerServiceClient;
    using Microsoft.ServiceFabric.ContainerServiceClient.Config;

    internal class ActivateContainerOperation
    {
        #region Private Memebers

        private const string TraceType = "ActivateContainerOperation";
        private const string RS3Version = "10.0.16299";

        private readonly ContainerActivatorService activator;
        private readonly ContainerActivationArgs activationArgs;
        private readonly TimeoutHelper timeoutHelper;

        private string dockerVersion;
        private string containerId;

        #endregion

        public ActivateContainerOperation(
            ContainerActivatorService activator,
            ContainerActivationArgs activationArgs,
            TimeSpan timeout)
        {
            this.activator = activator;
            this.activationArgs = activationArgs;

            this.timeoutHelper = new TimeoutHelper(timeout);
            this.dockerVersion = string.Empty;
            this.containerId = string.Empty;
        }

        public async Task<string> ExecuteAsync()
        {
#if !DotNetCoreClrLinux
            if (this.activationArgs.ProcessDescription.ResourceGovernancePolicy.NanoCpus > 0)
            {
                this.dockerVersion = await this.activator.GetDockerVersionAsync(this.timeoutHelper.RemainingTime);
            }
#endif

            await this.ConfigureVolumesAsync();
            await this.CreateContainerAsync();

#if !DotNetCoreClrLinux
            // If container is using open network on windows, we need additional set up before and after
            // starting the container. This is a mitigation for no support for outbound connectivity from 
            // open network based containers until RS3.
            if (!string.IsNullOrEmpty(this.activationArgs.ContainerDescription.AssignedIp))
            {
                await this.AddOutboundConnectivityToContainerAsync();
            }
#endif

            await this.StartContainerAsync();

#if !DotNetCoreClrLinux
            // If container is using open network on windows, we need additional set up before and after
            // starting the container. This is a mitigation for no support for outbound connectivity from 
            // open network based containers until RS3.
            if (!string.IsNullOrEmpty(this.activationArgs.ContainerDescription.AssignedIp))
            {
                await this.ExecuteUpdateRouteCommandAsync();
            }
#endif

            return this.containerId;
        }

        #region Private Helper Methods

        private string ContainerName
        {
            get
            {
                return this.activationArgs.ContainerDescription.ContainerName;
            }
        }

        private async Task ConfigureVolumesAsync()
        {
            var createParameters = 
                ConfigHelper.GetVolumeConfig(activationArgs.ContainerDescription);

            if (createParameters.Count > 0)
            {
                IList<Task> taskList = new List<Task>();

                foreach (var volumeParam in createParameters)
                {
                    taskList.Add(Task.Run(() => CreateVolumeAsync(volumeParam)));
                }

                await Task.WhenAll(taskList);
            }

            HostingTrace.Source.WriteNoise(
                TraceType,
                "ConfigureVolumesAsync() succeeded. {0}, VolumesCount={1}.",
                BuildContainerInfoMessage(),
                createParameters.Count);
        }

        private async Task CreateVolumeAsync(VolumeConfig volumeConfig)
        {
            try
            {
                await Utility.ExecuteWithRetriesAsync(
                    (operationTimeout) =>
                    {
                        return this.activator.Client.VolumeOperation.CreateVolumeAsync(
                            volumeConfig,
                            operationTimeout);
                    },
                    $"CreateVolumeAsync_{this.ContainerName}_{volumeConfig.Name}",
                    TraceType,
                    HostingConfig.Config.DockerRequestTimeout,
                    this.timeoutHelper.RemainingTime);
            }
            catch (Exception ex)
            {
                var errorMessage = string.Format(
                    "Creating Volume '{0}' failed for {1}.",
                    volumeConfig.Name,
                    this.BuildErrorMessage(ex));

                HostingTrace.Source.WriteError(TraceType, errorMessage);
                throw new FabricException(errorMessage, FabricErrorCode.InvalidOperation);
            }
        }

        private async Task AddOutboundConnectivityToContainerAsync()
        {
            var netwrokParams = new NetworkConnectConfig()
            {
                Container = this.containerId
            };

            FabricException opEx = null;

            try
            {
                await Utility.ExecuteWithRetriesAsync(
                    (operationTimeout) =>
                    {
                        return this.activator.Client.NetworkOperation.ConnectNetworkAsync(
                            "nat",
                            netwrokParams,
                            operationTimeout);
                    },
                    $"AddOutboundConnectivityToContainerAsync_{this.ContainerName}",
                    TraceType,
                    HostingConfig.Config.DockerRequestTimeout,
                    this.timeoutHelper.RemainingTime);

                HostingTrace.Source.WriteNoise(
                    TraceType,
                    "Adding outbound connectivity to {0} succeeded.",
                    BuildContainerInfoMessage());
            }
            catch(Exception ex)
            {
                var errorMessage = string.Format(
                    "Adding outbound connectivity failed for {0}",
                    this.BuildErrorMessage(ex));

                HostingTrace.Source.WriteError(TraceType, errorMessage);
                opEx = new FabricException(errorMessage, FabricErrorCode.InvalidOperation);
            }

            if (opEx != null)
            {
                await this.CleanupOnErrorAsync(opEx);
            }
        }

#if !DotNetCoreClrLinux
        private async Task ExecuteUpdateRouteCommandAsync()
        {
            var cmd = string.Format(
                   "route delete 0.0.0.0 {0}&route add 0.0.0.0 MASK 0.0.0.0 {1} METRIC 6000",
                   this.activationArgs.GatewayIpAddress,
                   this.activationArgs.GatewayIpAddress);

            // Route changes on RS3 builds need to be executed with higher privilege.
            ContainerExecConfig execConfig = new ContainerExecConfig()
            {
                AttachStdin = true,
                AttachStdout = true,
                AttachStderr = true,
                Privileged = false,
                Tty = true,
                Cmd = new List<string>() { "cmd.exe", "/c", cmd },
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
                    Cmd = new List<string>() { "cmd.exe", "/c", cmd }
                };

                opEx = await ExecuteUpdateRouteCommandAsync(execConfig);

                if (opEx != null)
                {
                    await this.CleanupOnErrorAsync(opEx);
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
                var response = await Utility.ExecuteWithRetriesAsync(
                    (operationTimeout) =>
                    {
                        return this.activator.Client.ExecOperation.CreateContainerExecAsync(
                            this.containerId,
                            execConfig,
                            operationTimeout);
                    },
                    $"ExecuteUpdateRouteCommandAsync_CreateContainerExecAsync_{this.ContainerName}",
                    TraceType,
                    HostingConfig.Config.DockerRequestTimeout,
                    this.timeoutHelper.RemainingTime);

                HostingTrace.Source.WriteNoise(
                    TraceType,
                    "Setup exec command to update route for {0} succeeded.",
                    BuildContainerInfoMessage());

                await Utility.ExecuteWithRetriesAsync(
                    (operationTimeout) =>
                    {
                        return this.activator.Client.ExecOperation.StartContainerExecAsync(
                            response.ID,
                            operationTimeout);
                    },
                    $"ExecuteUpdateRouteCommandAsync_StartContainerExecAsync_{this.ContainerName}",
                    TraceType,
                    HostingConfig.Config.DockerRequestTimeout,
                    this.timeoutHelper.RemainingTime);

                HostingTrace.Source.WriteNoise(
                    TraceType,
                    "Command execution to update route for {0}",
                    BuildContainerInfoMessage());
            }
            catch (Exception ex)
            {
                var errorMessage = string.Format(
                    "Set up exec command to update route failed for {0}.",
                    this.BuildErrorMessage(ex));

                HostingTrace.Source.WriteWarning(TraceType, errorMessage);
                throw new FabricException(errorMessage, FabricErrorCode.InvalidOperation);
            }
        }
#endif

        private async Task CreateContainerAsync()
        {
            var containerConfig = ConfigHelper.GetContainerConfig(
                activationArgs.ProcessDescription,
                activationArgs.ContainerDescription,
                this.dockerVersion);

            this.TracePortBindings();

            await this.CreateContainerInnerAsync(containerConfig, true);
        }

        private async Task CreateContainerInnerAsync(ContainerConfig containerConfig, bool retryDownload)
        {
            var containerName = this.activationArgs.ContainerDescription.ContainerName;
            var containerExist = false;

            try
            {
                var response = await Utility.ExecuteWithRetriesAsync(
                    (operationTimeout) =>
                    {
                        return this.activator.Client.ContainerOperation.CreateContainerAsync(
                            containerName,
                            containerConfig,
                            operationTimeout);
                    },
                    $"CreateContainerInnerAsync_{containerName}",
                    TraceType,
                    HostingConfig.Config.DockerRequestTimeout,
                    this.timeoutHelper.RemainingTime);

                this.containerId = response.ID;

                HostingTrace.Source.WriteInfo(
                    TraceType,
                    "Creating Container={0} from Image={1} for AppId={2} succeeded.",
                    containerName,
                    containerConfig.Image,
                    this.activationArgs.ContainerDescription.ApplicationId);

                return;
            }
            catch(ContainerApiException ex)
            {
                if (ex.StatusCode == HttpStatusCode.NotFound && retryDownload == true)
                {
                    HostingTrace.Source.WriteInfo(
                        TraceType,
                        "Image not found while creating Container={0} from Image={1} for AppId={2}. StatusCode={3}, ResponseBody={4}.",
                        containerName,
                        containerConfig.Image,
                        activationArgs.ContainerDescription.ApplicationId,
                        ex.StatusCode,
                        ex.ResponseBody);
                }
                else if (ex.StatusCode == HttpStatusCode.Conflict)
                {
                    //
                    // This can happen in case of retries when docker request timeout.
                    // The client connection is close but docker might have just created
                    // the container.
                    //
                    containerExist = true;

                    HostingTrace.Source.WriteInfo(
                        TraceType,
                        "Container already exists while Creating Container={0} from Image={1} for AppId={2}. StatusCode={3}, ResponseBody={4}.",
                        containerName,
                        containerConfig.Image,
                        activationArgs.ContainerDescription.ApplicationId,
                        ex.StatusCode,
                        ex.ResponseBody);
                }
                else
                {
                    var errorMessage = string.Format(
                        "Creating Container from Image={0} failed for {1}.",
                        containerConfig.Image,
                        this.BuildErrorMessage(ex));

                    this.TraceAndThrow(errorMessage);
                }
            }
            catch (Exception ex)
            {
                var errorMessage = string.Format(
                    "Creating Container from Image={0} failed for {1}.",
                    containerConfig.Image,
                    this.BuildErrorMessage(ex));

                this.TraceAndThrow(errorMessage);
            }

            if(containerExist)
            {
                var success = await this.GetContainerIdAsync(containerName, containerConfig);
                if(!success)
                {
                    await this.CreateContainerInnerAsync(containerConfig, true);
                }

                return;
            }

            //
            // skipCacheCheck=true to handle the case where an image is
            // deleted outside knowledge of FabricCAS
            //

            var imageDescription = new ContainerImageDescription();
            imageDescription.ImageName = this.activationArgs.ProcessDescription.ExePath;
            imageDescription.UseDefaultRepositoryCredentials = this.activationArgs.ContainerDescription.UseDefaultRepositoryCredentials;
            imageDescription.UseTokenAuthenticationCredentials = this.activationArgs.ContainerDescription.UseTokenAuthenticationCredentials;
            imageDescription.RepositoryCredential = this.activationArgs.ContainerDescription.RepositoryCredential;

            await this.activator.ImageDownloader.DownloadImageAsync(imageDescription, HostingConfig.Config.ContainerImageDownloadTimeout, true /*skipCacheCheck*/);
            await this.CreateContainerInnerAsync(containerConfig, false);
        }

        private async Task<bool> GetContainerIdAsync(
            string containerName,
            ContainerConfig containerConfig)
        {
            try
            {
                var response = await Utility.ExecuteWithRetriesAsync(
                    (operationTimeout) =>
                    {
                        return this.activator.Client.ContainerOperation.InspectContainersAsync(
                            containerName,
                            operationTimeout);
                    },
                    $"InspectContainersAsync_{this.ContainerName}",
                    TraceType,
                    HostingConfig.Config.DockerRequestTimeout,
                    this.timeoutHelper.RemainingTime);

                this.containerId = response.ContainerId;

                HostingTrace.Source.WriteInfo(
                    TraceType,
                    "Getting ID for Container={0} from Image={1} for AppId={2} succeeded. ContainerId={3}.",
                    containerName,
                    containerConfig.Image,
                    this.activationArgs.ContainerDescription.ApplicationId,
                    this.containerId);
            }
            catch (Exception ex)
            {
                var errorMessage = string.Format(
                    "Getting container ID failed after container was found existing. {0}.",
                    this.BuildErrorMessage(ex));

                if (ex is ContainerApiException contApiEx &&
                    contApiEx.StatusCode == HttpStatusCode.NotFound)
                {
                    HostingTrace.Source.WriteInfo(TraceType, errorMessage);
                    return false;
                }

                this.TraceAndThrow(errorMessage);
            }

            return true;
        }

        private void TraceAndThrow(string errorMessage)
        {
            HostingTrace.Source.WriteError(TraceType, errorMessage);
            throw new FabricException(errorMessage, FabricErrorCode.InvalidOperation);
        }

        private async Task StartContainerAsync()
        {
            FabricException opEx = null;

            try
            {
                await Utility.ExecuteWithRetriesAsync(
                    (operationTimeout) =>
                    {
                        return this.activator.Client.ContainerOperation.StartContainerAsync(
                            this.activationArgs.ContainerDescription.ContainerName,
                            operationTimeout);
                    },
                    $"StartContainerAsync_{this.ContainerName}",
                    TraceType,
                    HostingConfig.Config.DockerRequestTimeout,
                    this.timeoutHelper.RemainingTime);

                HostingTrace.Source.WriteInfo(
                    TraceType,
                    "Starting {0} succeeded.",
                    BuildContainerInfoMessage());
            }
            catch (Exception ex)
            {
                var errorMessage = string.Format(
                    "Failed to start Container. {0}",
                    this.BuildErrorMessage(ex));

                HostingTrace.Source.WriteError(TraceType, errorMessage);
                opEx = new FabricException(errorMessage, FabricErrorCode.InvalidOperation);
            }

            if (opEx != null)
            {
                await this.CleanupOnErrorAsync(opEx);
            }
        }

        private async Task CleanupOnErrorAsync(FabricException originalEx)
        {
            HostingTrace.Source.WriteWarning(
                TraceType,
                "CleanupOnErrorAsync(): Deactivating {0}",
                BuildContainerInfoMessage());

            try
            {
                var deactivationArg = new ContainerDeactivationArgs()
                {
                    ContainerName = this.activationArgs.ContainerDescription.ContainerName,
                    ConfiguredForAutoRemove = this.activationArgs.ContainerDescription.AutoRemove,
                    IsContainerRoot = this.activationArgs.ContainerDescription.IsContainerRoot,
                    CgroupName = this.activationArgs.ProcessDescription.CgroupName,
                    IsGracefulDeactivation = false
                };

                await Utility.ExecuteWithRetriesAsync(
                    (operationTimeout) =>
                    {
                        return this.activator.DeactivateContainerAsync(
                            deactivationArg,
                            operationTimeout);
                    },
                    $"CleanupOnErrorAsync_DeactivateContainerAsync_{this.ContainerName}",
                    TraceType,
                    HostingConfig.Config.DockerRequestTimeout,
                    this.timeoutHelper.RemainingTime);
            }
            catch(FabricException)
            {
                // Ignore.
            }

            throw originalEx;
        }

        private string BuildErrorMessage(Exception ex)
        {
            var errBuilder = new StringBuilder(BuildContainerInfoMessage());

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

        private string BuildContainerInfoMessage()
        {
            return string.Format("ContainerName={0}, ApplicationId={1}, ApplicationName={2}",
                activationArgs.ContainerDescription.ContainerName,
                activationArgs.ContainerDescription.ApplicationId,
                activationArgs.ContainerDescription.ApplicationName);
        }

        private void TracePortBindings()
        {
            if(activationArgs.ContainerDescription.PortBindings.Count == 0)
            {
                return;
            }

            var traceStr = new StringBuilder();

            foreach(var portBinding in activationArgs.ContainerDescription.PortBindings)
            {
                traceStr.AppendFormat("| ContainerPort={0}, HostPort={1} |", portBinding.Key, portBinding.Value);
            }

            HostingTrace.Source.WriteInfo(
                TraceType,
                "Creating container={0} for AppId={1} with PortBindings={2}.",
                activationArgs.ContainerDescription.ContainerName,
                activationArgs.ContainerDescription.ApplicationId,
                traceStr);
        }

        #endregion
    }
}
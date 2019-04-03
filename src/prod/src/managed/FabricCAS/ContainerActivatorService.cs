// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Hosting.ContainerActivatorService
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Linq;
    using System.Net.Http;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.ContainerServiceClient;
    using Microsoft.ServiceFabric.ContainerServiceClient.Config;
    using Microsoft.ServiceFabric.ContainerServiceClient.Response;
#if !DotNetCoreClrLinux
    using System.Management;
#endif

    internal class ContainerActivatorService : IContainerActivatorService
    {
        #region Private Member
        private const string TraceType = "ContainerActivatorService";

        private readonly FabricContainerActivatorServiceAgent activatorServiceAgent;
        private readonly ContainerServiceClient client;
        private readonly ContainerImageDownloader imageDownloader;
        private readonly ContainerEventManager eventManager;
        private readonly SemaphoreSlim versionLock;

        private bool isVersionInitialized;
        private string dockerVersion;
#if !DotNetCoreClrLinux
        private string osVersion;
#endif
        #endregion

        internal ContainerActivatorService()
        {
            this.activatorServiceAgent = new FabricContainerActivatorServiceAgent();
            this.client = new ContainerServiceClient(HostingConfig.Config.ContainerServiceNamedPipeOrUnixSocketAddress);
            this.imageDownloader = new ContainerImageDownloader(this);
            this.eventManager = new ContainerEventManager(this);
            this.versionLock = new SemaphoreSlim(1);
            this.isVersionInitialized = false;
            this.dockerVersion = string.Empty;
#if !DotNetCoreClrLinux
            FindOSVersion();
#endif
        }

        internal void Start()
        {
            this.activatorServiceAgent.RegisterContainerActivatorService(this);
        }

        internal ContainerServiceClient Client
        {
            get { return this.client; }
        }

#if !DotNetCoreClrLinux
        internal string OSVersion
        {
            get { return this.osVersion; }
        }
#endif

        internal ContainerImageDownloader ImageDownloader
        {
            get { return this.imageDownloader; }
        }

        internal async Task<string> GetDockerVersionAsync(TimeSpan timeout)
        {
            if(this.isVersionInitialized)
            {
                return this.dockerVersion;
            }

            await this.versionLock.WaitAsync();

            try
            {
                if (this.isVersionInitialized)
                {
                    return this.dockerVersion;
                }

                var response = await Utility.ExecuteWithRetryOnTimeoutAsync(
                    (operationTimeout) =>
                    {
                        return this.client.SystemOperation.GetVersionAsync(
                            operationTimeout);
                    },
                    "GetDockerVersionAsync",
                    TraceType,
                    HostingConfig.Config.DockerRequestTimeout,
                    timeout);

                this.dockerVersion = response.Version;
                this.isVersionInitialized = true;

                HostingTrace.Source.WriteInfo(
                    TraceType, 
                    "DockerVersion query return Version={0}",
                    this.dockerVersion);

                return this.dockerVersion;
            }
            catch (ContainerApiException dex)
            {
                var errorMessage = string.Format(
                    "Getting Docker version failed. StatusCode={0}, ResponseBody={1}.",
                    dex.StatusCode,
                    dex.ResponseBody);

                HostingTrace.Source.WriteError(TraceType, errorMessage);
                throw new FabricException(errorMessage, FabricErrorCode.InvalidOperation);
            }
            catch(Exception ex)
            {
                var errorMessage = $"Getting Docker version failed. {ex}.";
                throw new FabricException(errorMessage, FabricErrorCode.InvalidOperation);
            }
            finally
            {
                this.versionLock.Release();
            }
        }

        internal void OnContainerEvent(ContainerEventNotification eventNotification)
        {
            this.activatorServiceAgent.ProcessContainerEvents(eventNotification);
        }

        internal async Task PruneContainers()
        {
            if (HostingConfig.Config.ContainerCleanupScanIntervalInMinutes <= 0)
            {
                HostingTrace.Source.WriteInfo(
                    TraceType,
                    "Container pruning is disabled as ContainerCleanupScanIntervalInMinutes is set to {0}.",
                    HostingConfig.Config.ContainerCleanupScanIntervalInMinutes);

                return;
            }

            var pruneInterval = TimeSpan.FromMinutes(HostingConfig.Config.ContainerCleanupScanIntervalInMinutes);
            var untilFilterVal = $"{HostingConfig.Config.DeadContainerCleanupUntilInMinutes}m";
            var labelFilterVal = $"{ConfigHelper.PlatformLabelKeyName}={ConfigHelper.PlatformLabelKeyValue}";

            var pruneConfig = new ContainersPruneConfig()
            {
                Filters = new Dictionary<string, IList<string>>()
                {
                    { "until", new List<string> () { untilFilterVal } },
                    { "label", new List<string> () { labelFilterVal } }
                }
            };

            while(true)
            {
                try
                {
                    await Task.Delay(pruneInterval);

                    var pruneResp = await this.client.ContainerOperation.PruneContainersAsync(
                        pruneConfig,
                        HostingConfig.Config.DockerRequestTimeout);

                    HostingTrace.Source.WriteInfo(
                        TraceType,
                        "PruneContainers finished successfully. ContainersDeletedCount={0}, SpaceReclaimed={1} bytes.",
                        pruneResp.ContainersDeleted == null ? 0 : pruneResp.ContainersDeleted.Count,
                        pruneResp.SpaceReclaimed);
                }
                catch(ContainerApiException ex)
                {
                    HostingTrace.Source.WriteWarning(
                        TraceType,
                        "PruneContainers failed with StatusCode={0} with ResponseBody={1}.",
                        ex.StatusCode,
                        ex.ResponseBody);
                }
                catch(Exception ex)
                {
                    HostingTrace.Source.WriteWarning(
                        TraceType,
                        "PruneContainers failed with Exception={0}.",
                        ex);
                }
            }
        }

#region IContainerActivator Methods

        public void StartEventMonitoring(
            bool isContainerServiceManaged,
            UInt64 sinceTime)
        {
            this.client.IsContainerServiceManaged = isContainerServiceManaged;
            this.eventManager.StartMonitoringEvents(sinceTime);

            Task.Run(() => this.PruneContainers());
        }

        public Task<string> ActivateContainerAsync(ContainerActivationArgs activationArgs, TimeSpan timeout)
        {
            var operation = new ActivateContainerOperation(this, activationArgs, timeout);
            return operation.ExecuteAsync();
        }

        public Task ExecuteUpdateRoutesAsync(ContainerUpdateRouteArgs updateRouteArgs, TimeSpan timeout)
        {
            var operation = new ExecuteContainerUpdateRoutesOperation(this, updateRouteArgs, timeout);
            return operation.ExecuteAsync();
        }

        public Task DeactivateContainerAsync(ContainerDeactivationArgs deactivationArgs, TimeSpan timeout)
        {
            var operation = new DeactivateContainerOperation(this, deactivationArgs, timeout);
            return operation.ExecuteAsync();
        }

        public Task DeleteImagesAsync(List<string> imagesNames, TimeSpan timeout)
        {
            return imageDownloader.DeleteImagesAsync(imagesNames, timeout);
        }

        public Task DownloadImagesAsync(List<ContainerImageDescription> imageDescriptions, TimeSpan timeout)
        {
            return imageDownloader.DownloadImagesAsync(imageDescriptions, timeout);
        }

        public async Task<ContainerApiExecutionResponse> InvokeContainerApiAsync(
            ContainerApiExecutionArgs apiExecArgs, TimeSpan timeout)
        {
            HttpContent content = null;

            HostingTrace.Source.WriteInfo(
                TraceType,
                "InvokeContainerApiAsync called with ContainerName={0}, HttpVerb={1}, UriPath={2}, ContentType={3}, RequestBody={4}.",
                apiExecArgs.ContainerName,
                apiExecArgs.HttpVerb,
                apiExecArgs.UriPath,
                apiExecArgs.ContentType,
                apiExecArgs.RequestBody);

            if (!string.IsNullOrEmpty(apiExecArgs.RequestBody))
            {
                content = new StringContent(apiExecArgs.RequestBody, Encoding.UTF8, apiExecArgs.ContentType);
            }

            var response = await this.client.MakeRequestForHttpResponseAsync(
                new HttpMethod(apiExecArgs.HttpVerb),
                apiExecArgs.UriPath,
                null,
                content,
                timeout).ConfigureAwait(false);

            try
            {
                var statusCode = (UInt16)response.StatusCode;

                var contentType = string.Empty;
                if(response.Content.Headers.ContentType != null)
                {
                    contentType = response.Content.Headers.ContentType.ToString();
                }

                var contentEncoding = string.Empty;
                if (response.Content.Headers.ContentEncoding != null &&
                    response.Content.Headers.ContentEncoding.Count > 0)
                {
                    contentEncoding = response.Content.Headers.ContentEncoding.First();
                }

                var responseBody = await response.Content.ReadAsByteArrayAsync();

                return new ContainerApiExecutionResponse()
                {
                    StatusCode = statusCode,
                    ContentType = contentType,
                    ContentEncoding = contentEncoding,
                    ResponseBody = responseBody
                };
            }
            catch(Exception ex)
            {
                HostingTrace.Source.WriteWarning(
                    TraceType,
                    "InvokeContainerApiAsync: ContainerName={0}, HttpVerb={1}, UriPath={2}, ContentType={3}, RequestBody={4} failed. Exception={5}.",
                    apiExecArgs.ContainerName,
                    apiExecArgs.HttpVerb,
                    apiExecArgs.UriPath,
                    apiExecArgs.ContentType,
                    apiExecArgs.RequestBody,
                    ex.ToString());

                response.Dispose();
                throw;
            }
        }
        #endregion

#if !DotNetCoreClrLinux
        private void FindOSVersion()
        {
            try
            {
                var searcher = new ManagementObjectSearcher("SELECT Caption, Version FROM Win32_OperatingSystem");
                var osCollection = searcher.Get();
                if (osCollection != null)
                {
                    var osArray = new ManagementBaseObject[osCollection.Count];
                    osCollection.CopyTo(osArray, 0);
                    if (osArray.Length == 1)
                    {
                        this.osVersion = osArray[0]["Version"].ToString();
                    }
                }
            }
            catch (Exception ex)
            {
                var errorMessage = string.Format("Getting OS version failed. Error={0}.", ex);
                HostingTrace.Source.WriteError(TraceType, errorMessage);
            }
        }
#endif
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Fabric.WRP.Model;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Xml;
    using System.Xml.Linq;

    public class FabricClientWrapper : IFabricClientWrapper
    {
        private static readonly TraceType TraceType = new TraceType("UpgradeSystemService");
        private const int DelayTimeoutInSeconds = 5;
        private readonly FabricClient fabricClient;

        private string imageStoreConnectionString;
        private IEnumerable<Uri> systemServicesName;

        public FabricClientWrapper(FabricClient fabricClient)
        {
            this.fabricClient = fabricClient;
        }

        public FabricClientWrapper()
        {
            this.fabricClient = new FabricClient();
        }

        public FabricClient FabricClient
        {
            get { return this.fabricClient; }
        }

        public Task<ClusterHealth> GetClusterHealthAsync(TimeSpan timeout, CancellationToken token)
        {
            var timeoutHelper = new TimeoutHelper(timeout, Constants.MaxOperationTimeout);

            return FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                () => this.fabricClient.HealthManager.GetClusterHealthAsync(timeoutHelper.GetOperationTimeout(), token),
                timeoutHelper.GetOperationTimeout(),
                token);
        }

        public async Task<ApplicationHealth> GetApplicationHealthAsync(Uri applicationName, TimeSpan timeout, CancellationToken token)
        {
            var timeoutHelper = new TimeoutHelper(timeout, Constants.MaxOperationTimeout);

            return await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                () => this.fabricClient.HealthManager.GetApplicationHealthAsync(applicationName, timeoutHelper.GetOperationTimeout(), token),
                timeoutHelper.GetOperationTimeout(),
                token);
        }

        public async Task<ServiceList> GetServicesAsync(Uri applicationName, TimeSpan timeout, CancellationToken token)
        {
            var timeoutHelper = new TimeoutHelper(timeout, Constants.MaxOperationTimeout);

            return await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                () => this.fabricClient.QueryManager.GetServiceListAsync(applicationName, null, timeout, token),
                timeoutHelper.GetOperationTimeout(),
                token);
        }
        
        public async Task<NodeList> GetNodeListAsync(TimeSpan timeout, CancellationToken token)
        {
            Trace.WriteInfo(TraceType, "Calling GetNodeList. Timeout: {0}", timeout);

            NodeList aggregatedNodeList = new NodeList();
            string continuationToken = null;
            do
            {
                var nodeList = await this.fabricClient.QueryManager.GetNodeListAsync(null, NodeStatusFilter.All, continuationToken, timeout, token);
                aggregatedNodeList.AddRangeNullSafe(nodeList);

                continuationToken = nodeList.ContinuationToken;
            }
            while(!string.IsNullOrEmpty(continuationToken));

            return aggregatedNodeList;
        }

        public async Task<Dictionary<string, ServiceRuntimeDescription>> GetSystemServiceRuntimeDescriptionsAsync(TimeSpan timeout, CancellationToken token)
        {            
            if (this.systemServicesName == null)
            {
                Trace.WriteInfo(TraceType, "Calling GetServiceList for system application. Timeout: {0}", timeout);

                var serviceList = await this.FabricClient.QueryManager.GetServiceListAsync(this.FabricClient.FabricSystemApplication, null, timeout, token);
                systemServicesName = serviceList.Select(service => service.ServiceName);
            }

            List<Task<ServiceDescription>> taskList = new List<Task<ServiceDescription>>();
            foreach(var serviceName in this.systemServicesName)
            {
                Trace.WriteInfo(TraceType, "Calling GetServiceDescription for {0}. Timeout: {1}", serviceName.ToString(), timeout);

                var task = this.FabricClient.ServiceManager.GetServiceDescriptionAsync(serviceName, timeout, token);
                taskList.Add(task);
            }

            await Task.WhenAll(taskList);

            Dictionary<string, ServiceRuntimeDescription> systemServiceReplicaSetSize = new Dictionary<string, ServiceRuntimeDescription>();
            foreach(var task in taskList)
            {
                var serviceDesc = task.Result as StatefulServiceDescription;
                if(serviceDesc == null)
                {
                    Trace.WriteInfo(
                        TraceType, 
                        "The ServiceDescription of {0} is being skipped. UpgradeService only handles stateful service now.",
                        task.Result.ServiceName.ToString());

                    // All system service are stateful
                    // Modify logic to support stateless services if required
                    continue;
                }

                systemServiceReplicaSetSize.Add(
                    serviceDesc.ServiceName.ToString(),
                    new ServiceRuntimeDescription()
                    {
                        MinReplicaSetSize = serviceDesc.MinReplicaSetSize,
                        TargetReplicaSetSize = serviceDesc.TargetReplicaSetSize,
                        PlacementConstraints = serviceDesc.PlacementConstraints
                    });
            }

            return systemServiceReplicaSetSize;
        }

        /// <summary>
        /// It queries cluster manager to get FabricUpgradeProgress
        /// </summary>
        /// <param name="timeout"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public Task<FabricUpgradeProgress> GetFabricUpgradeProgressAsync(
            TimeSpan timeout, 
            CancellationToken cancellationToken )
        {
            var timeoutHelper = new TimeoutHelper(timeout, Constants.MaxOperationTimeout);

            return FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => this.fabricClient.ClusterManager.GetFabricUpgradeProgressAsync(timeoutHelper.GetOperationTimeout(), cancellationToken),
                    timeoutHelper.GetOperationTimeout(),
                    cancellationToken);
        }

        /// <summary>
        /// Returns a task which will complete when underlying upgrade completes.
        /// </summary>
        /// <param name="timeout"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public async Task<FabricUpgradeProgress> GetCurrentRunningUpgradeTaskAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            FabricUpgradeProgress fabricUpgradeProgress = null;
            var timeoutHelper = new TimeoutHelper(timeout, Constants.MaxOperationTimeout);
            while (true)
            {
                fabricUpgradeProgress = await this.fabricClient.ClusterManager.GetFabricUpgradeProgressAsync(
                    timeoutHelper.GetOperationTimeout(),
                    cancellationToken);

                Trace.WriteInfo(TraceType, "Current status: {0}", fabricUpgradeProgress.UpgradeState);
                if (IsUpgradeCompleted(fabricUpgradeProgress.UpgradeState))
                {
                    break;
                }

                if (IsUpgradeInUnSupportedState(fabricUpgradeProgress.UpgradeState))
                {
                    throw new NotSupportedException(string.Format("Cluster Fabric upgrade is in state {0}", fabricUpgradeProgress.UpgradeState));
                }

                var delayTime = timeoutHelper.GetRemainingTime();
                if (delayTime.CompareTo(TimeSpan.FromSeconds(DelayTimeoutInSeconds)) > 0)
                {
                    delayTime = TimeSpan.FromSeconds(DelayTimeoutInSeconds);
                }

                await Task.Delay(delayTime, cancellationToken);
            }

            return fabricUpgradeProgress;
        }

        public async Task<Tuple<string, string>> GetCurrentCodeAndConfigVersionAsync(string targetConfigVersion)
        {
            var nodeList = await this.fabricClient.QueryManager.GetNodeListAsync();
            var node = nodeList.FirstOrDefault(n => !n.ConfigVersion.Equals(targetConfigVersion)) ?? nodeList.First();
            return new Tuple<string, string>(node.CodeVersion, node.ConfigVersion);
        }

        public async Task<string> GetCurrentClusterManifestFile()
        {
            var manifest = await this.fabricClient.ClusterManager.GetClusterManifestAsync();
            var info = Directory.CreateDirectory(Path.Combine(Path.GetTempPath(), Path.GetRandomFileName()));
            var clusterConfigFilePath = Path.Combine(info.FullName, "ClusterManifest.xml");
            File.WriteAllText(clusterConfigFilePath, manifest);
            return clusterConfigFilePath;
        }

        public async Task UpdateSystemServicesDescriptionAsync(
            Dictionary<string, ServiceRuntimeDescription> descriptions, 
            TimeSpan timeout, 
            CancellationToken cancellationToken)
        {
            if(descriptions == null)
            {
                return;
            }

            if (this.systemServicesName == null)
            {
                var serviceList = await this.FabricClient.QueryManager.GetServiceListAsync(this.FabricClient.FabricSystemApplication, null, timeout, cancellationToken);
                this.systemServicesName = serviceList.Select(service => service.ServiceName);
            }

            List<Task> taskList = new List<Task>();
            foreach(var service in descriptions)
            {
                var serviceName = new Uri(service.Key);
                if(!this.systemServicesName.Contains(serviceName))
                {
                    Trace.WriteWarning(TraceType, "Systemervice {0} is not present in this cluster. Skipping updating the service.", serviceName);
                    continue;
                }

                var task = this.UpdateServiceDescriptionAsync(serviceName, service.Value, timeout, cancellationToken);
                taskList.Add(task);
            }

            await Task.WhenAll(taskList);
        }

        public async Task UpdateServiceDescriptionAsync(Uri serviceName, ServiceRuntimeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            if (serviceName == null || 
                description == null)
            {
                return;
            }

            var updateDesc = new StatefulServiceUpdateDescription()
            {
                MinReplicaSetSize = description.MinReplicaSetSize,
                TargetReplicaSetSize = description.TargetReplicaSetSize,
                PlacementConstraints = description.PlacementConstraints
            };

            Trace.WriteInfo(
                TraceType,
                "Calling UpdateServiceDescription for {0}. TargetReplicaSetSize={1}, MinReplicaSetSize={2}. PlacementConstraints={3}", 
                serviceName, 
                updateDesc.TargetReplicaSetSize, 
                updateDesc.MinReplicaSetSize,
                updateDesc.PlacementConstraints ?? "null");

            var timeoutHelper = new TimeoutHelper(timeout, Constants.MaxOperationTimeout);

            await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () =>  this.FabricClient.ServiceManager.UpdateServiceAsync(serviceName, updateDesc, timeoutHelper.GetOperationTimeout(), cancellationToken),
                    timeoutHelper.GetOperationTimeout(),
                    cancellationToken);
        }

        /// <summary>
        /// Disable the given set of nodes
        /// </summary>
        /// <param name="nodesToDisable">Nodes to disable</param>
        /// <param name="timeout"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public async Task DisableNodesAsync(List<PaasNodeStatusInfo> nodesToDisable, TimeSpan timeout, CancellationToken cancellationToken)
        {
            if (nodesToDisable == null)
            {
                Trace.WriteInfo(TraceType, "No nodes to disable.");
                return;
            }

            List<Task> taskList = new List<Task>();
            foreach(var nodeToDisable in nodesToDisable)
            {
                var task = DisableNodeAsync(nodeToDisable, timeout, cancellationToken);
                taskList.Add(task);
            }

            await Task.WhenAll(taskList);
        }


        public async Task DisableNodeAsync(PaasNodeStatusInfo nodeToDisable, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Trace.WriteInfo(TraceType, "Calling DeactivateNode for {0}", nodeToDisable.NodeName);

            var timeoutHelper = new TimeoutHelper(timeout, Constants.MaxOperationTimeout);

            await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => this.FabricClient.ClusterManager.DeactivateNodeAsync(
                        nodeToDisable.NodeName,
                        NodeDeactivationIntent.RemoveData,
                        timeoutHelper.GetOperationTimeout(),
                        cancellationToken),
                    timeoutHelper.GetOperationTimeout(),
                    cancellationToken);
        }

        /// <summary>
        /// Enable the given set of nodes
        /// </summary>
        /// <param name="nodesToEnable">Nodes to enable</param>
        /// <param name="timeout"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public async Task EnableNodesAsync(List<PaasNodeStatusInfo> nodesToEnable, TimeSpan timeout, CancellationToken cancellationToken)
        {
            if(nodesToEnable == null)
            {
                Trace.WriteInfo(TraceType, "No nodes to enable.");
                return;
            }

            List<Task> taskList = new List<Task>();
            foreach (var nodeToEnable in nodesToEnable)
            {
                var task = EnableNodeAsync(nodeToEnable, timeout, cancellationToken);
                taskList.Add(task);
            }

            await Task.WhenAll(taskList);
        }

        public async Task EnableNodeAsync(PaasNodeStatusInfo nodeToEnable, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Trace.WriteInfo(TraceType, "Calling ActivateNode for {0}", nodeToEnable.NodeName);

            var timeoutHelper = new TimeoutHelper(timeout, Constants.MaxOperationTimeout);

            await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => this.FabricClient.ClusterManager.ActivateNodeAsync(
                        nodeToEnable.NodeName,
                        timeoutHelper.GetOperationTimeout(),
                        cancellationToken),
                    timeoutHelper.GetOperationTimeout(),
                    cancellationToken);
        }

        /// <summary>
        /// This function upgrades the cluster. It returns the task which completes when the upgrade completes.
        /// This function does monitored upgrade with failure action as Rollback. 
        /// </summary>
        /// <param name="commandParameter"></param>
        /// <param name="timeout"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public async Task ClusterUpgradeAsync(ClusterUpgradeCommandParameter commandParameter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // No upgrade is required
            if (commandParameter == null)
            {
                Trace.WriteInfo(TraceType, $"No upgrade is required. {nameof(commandParameter)} is Null.");
                return;
            }

            if (string.IsNullOrWhiteSpace(commandParameter.CodeFilePath) &&
                string.IsNullOrWhiteSpace(commandParameter.ConfigFilePath))
            {
                return;
            }

            #region parameter check
            // Validate parameter

            if (!string.IsNullOrWhiteSpace(commandParameter.CodeFilePath) &&
                string.IsNullOrWhiteSpace(commandParameter.CodeVersion))
            {
                throw new ArgumentException("CodeFilePath is not null but its version is empty");
            }

            if (!string.IsNullOrWhiteSpace(commandParameter.CodeFilePath))
            {
                Version version;
                if (!Version.TryParse(commandParameter.CodeVersion, out version))
                {
                    throw new ArgumentException("CodeVersion is not a valid version");
                }
            }

            if (!string.IsNullOrWhiteSpace(commandParameter.CodeFilePath) &&
                string.IsNullOrWhiteSpace(Path.GetFileName(commandParameter.CodeFilePath)))
            {
                throw new ArgumentException("commandParameter.CodeFilePath should end with a file name");
            }

            if (!string.IsNullOrWhiteSpace(commandParameter.ConfigFilePath) &&
                string.IsNullOrWhiteSpace(Path.GetFileName(commandParameter.ConfigFilePath)))
            {
                throw new ArgumentException("commandParameter.ConfigFilePath should end with a file name");
            }
            #endregion

            var timeoutHelper = new TimeoutHelper(timeout, Constants.MaxOperationTimeout);           

            var targetCodeVersion = commandParameter.CodeVersion;
            var targetConfigVersion = commandParameter.ConfigVersion;
            if (!string.IsNullOrWhiteSpace(commandParameter.ConfigFilePath) && 
                string.IsNullOrWhiteSpace(targetConfigVersion))
            {
                // If config version is not mentioned then read it from given manifest file
                targetConfigVersion = GetConfigVersion(commandParameter.ConfigFilePath);
            }

            Trace.WriteInfo(
                TraceType,
                "ClusterUpgradeAsync: CodePath {0}, CodeVersion {1}, ConfigPath {2}, ConfigVersion {3}",
                commandParameter.CodeFilePath,
                targetCodeVersion,
                commandParameter.ConfigFilePath,
                targetConfigVersion);

            await this.CopyAndProvisionUpgradePackageAsync(
                commandParameter, 
                targetCodeVersion, 
                targetConfigVersion, 
                timeoutHelper, 
                cancellationToken);

            Trace.WriteInfo(
                    TraceType,
                    "ClusterUpgradeAsync: Completed CopyAndProvisionUpgradePackageAsync");

            await this.StartUpgradeFabricAsync(commandParameter.UpgradeDescription,
                targetCodeVersion,
                targetConfigVersion,
                timeoutHelper,
                cancellationToken);

            Trace.WriteInfo(
                TraceType,
                "ClusterUpgradeAsync: Completed StartUpgradeFabricAsync");
        }

        /// <summary>
        /// Copy and provision upgrade package.
        /// </summary>
        /// <param name="commandParameter"></param>
        /// <param name="targetCodeVersion"></param>
        /// <param name="targetConfigVersion"></param>
        /// <param name="timeoutHelper"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        internal async Task CopyAndProvisionUpgradePackageAsync(
            ClusterUpgradeCommandParameter commandParameter,
            string targetCodeVersion,
            string targetConfigVersion,
            TimeoutHelper timeoutHelper,
            CancellationToken cancellationToken)
        {
            var imageStorePath = string.IsNullOrWhiteSpace(targetCodeVersion) ? string.Empty : targetCodeVersion;
            imageStorePath += string.IsNullOrWhiteSpace(targetConfigVersion)
                ? string.Empty
                : "_" + targetConfigVersion;
            imageStorePath = imageStorePath.Trim('_');

            Trace.WriteInfo(TraceType, "CopyAndProvision");
            var configFileName = string.Empty;
            var codeFileName = string.Empty;
            var configPathInImageStore = string.Empty;
            var codePathInImageStore = string.Empty;

            if (!string.IsNullOrWhiteSpace(commandParameter.ConfigFilePath))
            {
                configFileName = Path.GetFileName(commandParameter.ConfigFilePath);
                configPathInImageStore = Path.Combine(imageStorePath, configFileName);
            }

            if (!string.IsNullOrWhiteSpace(commandParameter.CodeFilePath))
            {
                codeFileName = Path.GetFileName(commandParameter.CodeFilePath);
                codePathInImageStore = Path.Combine(imageStorePath, codeFileName);
            }

            if (string.IsNullOrWhiteSpace(configFileName) &&
                string.IsNullOrWhiteSpace(codeFileName))
            {
                return;
            }

            var configFilePath = commandParameter.ConfigFilePath;
            var codeFilePath = commandParameter.CodeFilePath;
            if (!string.IsNullOrWhiteSpace(targetCodeVersion))
            {
                var provisionedCodeList = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() =>
                    this.fabricClient.QueryManager.GetProvisionedFabricCodeVersionListAsync(targetCodeVersion),
                        timeoutHelper.GetOperationTimeout(),
                        cancellationToken).ConfigureAwait(false); 
                if (provisionedCodeList.Count != 0)
                {
                    Trace.WriteInfo(TraceType, "Code Already provisioned: {0}", targetCodeVersion);
                    codePathInImageStore = null;
                    codeFilePath = null;
                }
            }

            if (!string.IsNullOrWhiteSpace(targetConfigVersion))
            {
                var provisionedConfigList = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() =>
                    this.fabricClient.QueryManager.GetProvisionedFabricConfigVersionListAsync(targetConfigVersion),
                        timeoutHelper.GetOperationTimeout(),
                        cancellationToken).ConfigureAwait(false); 
                if (provisionedConfigList.Count != 0)
                {
                    Trace.WriteInfo(TraceType, "Config Already provisioned: {0}", targetConfigVersion);
                    configPathInImageStore = null;
                    configFilePath = null;
                }
            }

            // Get current cluster manifest file
            if (string.IsNullOrWhiteSpace(this.imageStoreConnectionString))
            {
                var clusterManifest = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() => 
                    this.fabricClient.ClusterManager.GetClusterManifestAsync(
                        timeoutHelper.GetOperationTimeout(),
                        cancellationToken),
                    timeoutHelper.GetOperationTimeout(),
                    cancellationToken).ConfigureAwait(false); 

                // Get image store connection string if it not yet done.
                this.imageStoreConnectionString = GetImageStoreConnectionString(clusterManifest);
            }

            if (!string.IsNullOrWhiteSpace(configPathInImageStore) ||
                !string.IsNullOrWhiteSpace(codePathInImageStore))
            {
                // CopyClusterPackage is not an async API, so ideally we should create a sync version of ExecuteFabricActionWithRetry and don't spawn a new task thread here.
                //     However, the copy and provision call doesn't happen all the time so there's no perf consideration here to have an extra thread.
                //     Also, I don't see why CopyClusterPackage shouldn't expose an async version in the future.
                // So, don't bother to create a sync version of ExecuteFabricActionWithRetryAsync.
                await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() =>
                    Task.Run(
                        delegate
                        {
                            Trace.WriteInfo(TraceType, "Copy to ImageStorePath: {0}", imageStorePath);

                            this.fabricClient.ClusterManager.CopyClusterPackage(
                                                    this.imageStoreConnectionString,
                                                    configFilePath,
                                                    configPathInImageStore,
                                                    codeFilePath,
                                                    codePathInImageStore,
                                                    timeoutHelper.GetOperationTimeout());
                        }),
                    timeoutHelper.GetRemainingTime(),
                    cancellationToken).ConfigureAwait(false);

                Trace.WriteInfo(TraceType, "Completed Copy to ImageStorePath: {0}", imageStorePath);

                // Provision the code and config
                Trace.WriteInfo(
                    TraceType, 
                    "Provision codePath:{0} configPath:{1}", 
                    codePathInImageStore,
                    configPathInImageStore);

               await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() =>
                    this.fabricClient.ClusterManager.ProvisionFabricAsync(
                        codePathInImageStore,
                        configPathInImageStore,
                        timeoutHelper.GetOperationTimeout(),
                        cancellationToken),
                    FabricClientRetryErrors.ProvisionFabricErrors.Value,
                    timeoutHelper.GetRemainingTime(),                 
                    cancellationToken).ConfigureAwait(false);
                
                Trace.WriteInfo(
                    TraceType,
                    "Completed Provision codePath:{0} configPath:{1}",
                    codePathInImageStore,
                    configPathInImageStore);
            }
        }

        /// <summary>
        /// Gets details for the specific cluster code version provisioned in the system.
        /// </summary>
        /// <param name="codeVersionFilter"></param>
        /// <param name="timeout"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public async Task<ProvisionedFabricCodeVersionList> GetProvisionedFabricCodeVersionListAsync(string codeVersionFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var timeoutHelper = new TimeoutHelper(timeout, Constants.MaxOperationTimeout);

            return await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                () => this.fabricClient.QueryManager.GetProvisionedFabricCodeVersionListAsync(codeVersionFilter),
                timeoutHelper.GetOperationTimeout(),
                cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Gets details for a specific cluster config version provisioned in the system.
        /// </summary>
        /// <param name="configVersionFilter"></param>
        /// <param name="timeout"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public async Task<ProvisionedFabricConfigVersionList> GetProvisionedFabricConfigVersionListAsync(string configVersionFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var timeoutHelper = new TimeoutHelper(timeout, Constants.MaxOperationTimeout);

            return await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() =>
                    this.fabricClient.QueryManager.GetProvisionedFabricConfigVersionListAsync(configVersionFilter),
                        timeoutHelper.GetOperationTimeout(),
                        cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Gets the XML contents of the current running cluster manifest.
        /// </summary>
        /// <param name="timeout"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public async Task<string> GetClusterManifestAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            var timeoutHelper = new TimeoutHelper(timeout, Constants.MaxOperationTimeout);

            return await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() =>
                    this.fabricClient.ClusterManager.GetClusterManifestAsync(
                        timeoutHelper.GetOperationTimeout(),
                        cancellationToken),
                    timeoutHelper.GetOperationTimeout(),
                    cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Copies the cluster manifest file and/or Service Fabric code package to the image store.
        /// </summary>
        /// <param name="imageStoreConnectionString"></param>
        /// <param name="clusterManifestPath"></param>
        /// <param name="clusterManifestPathInImageStore"></param>
        /// <param name="codePackagePath"></param>
        /// <param name="codePackagePathInImageStore"></param>
        /// <param name="timeout"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public async Task CopyClusterPackageAsync(
                string imageStoreConnectionString,
                string clusterManifestPath,
                string clusterManifestPathInImageStore,
                string codePackagePath,
                string codePackagePathInImageStore,
                TimeSpan timeout,
                CancellationToken cancellationToken)
        {
            var timeoutHelper = new TimeoutHelper(timeout, timeout);

            await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() =>
                    Task.Run(() => this.fabricClient.ClusterManager.CopyClusterPackage(
                                                    imageStoreConnectionString,
                                                    clusterManifestPath,
                                                    clusterManifestPathInImageStore,
                                                    codePackagePath,
                                                    codePackagePathInImageStore,
                                                    timeoutHelper.GetOperationTimeout())
                        ),
                    timeoutHelper.GetRemainingTime(),
                    cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Deletes the cluster manifest file and/or Service Fabric code package from the image store.
        /// </summary>
        /// <param name="imageStoreConnectionString"></param>
        /// <param name="clusterManifestPath"></param>
        /// <param name="clusterManifestPathInImageStore"></param>
        /// <param name="codePackagePath"></param>
        /// <param name="codePackagePathInImageStore"></param>
        /// <param name="timeout"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public async Task RemoveClusterPackageAsync(
                string imageStoreConnectionString,
                string clusterManifestPathInImageStore,
                string codePackagePathInImageStore,
                TimeSpan timeout,
                CancellationToken cancellationToken)
        {
            var timeoutHelper = new TimeoutHelper(timeout, timeout);

            await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() =>
                    Task.Run(() => this.fabricClient.ClusterManager.RemoveClusterPackage(
                                                    imageStoreConnectionString,                                                    
                                                    clusterManifestPathInImageStore,
                                                    codePackagePathInImageStore)
                        ),
                    timeoutHelper.GetRemainingTime(),
                    cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Provisions the Service Fabric by using the specified timeout and cancellation token.
        /// </summary>
        /// <param name="codePathInImageStore"></param>
        /// <param name="configPathInImageStore"></param>
        /// <param name="timeout"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public async Task ProvisionFabricAsync(string codePathInImageStore, string configPathInImageStore, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var timeoutHelper = new TimeoutHelper(timeout, timeout);

            await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() =>
                    this.fabricClient.ClusterManager.ProvisionFabricAsync(
                        codePathInImageStore,
                        configPathInImageStore,
                        timeoutHelper.GetOperationTimeout(),
                        cancellationToken),
                    FabricClientRetryErrors.ProvisionFabricErrors.Value,
                    timeoutHelper.GetRemainingTime(),
                    cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Upgrades the Service Fabric by using the specified timeout and cancellation token.
        /// </summary>
        /// <param name="commandDescription"></param>
        /// <param name="targetCodeVersion"></param>
        /// <param name="targetConfigVersion"></param>
        /// <param name="timeout"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public async Task UpgradeFabricAsync(
            CommandProcessorClusterUpgradeDescription commandDescription,
            string targetCodeVersion,
            string targetConfigVersion,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            var timeoutHelper = new TimeoutHelper(timeout, timeout);

            await StartUpgradeFabricAsync(commandDescription, targetCodeVersion, targetConfigVersion, timeoutHelper, cancellationToken);
        }

        /// <summary>
        /// This function starts the upgrade process
        /// </summary>
        /// <param name="commandDescription"></param>
        /// <param name="targetCodeVersion"></param>
        /// <param name="targetConfigVersion"></param>
        /// <param name="timeoutHelper"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        private async Task StartUpgradeFabricAsync(
            CommandProcessorClusterUpgradeDescription commandDescription,
            string targetCodeVersion,
            string targetConfigVersion,
            TimeoutHelper timeoutHelper,
            CancellationToken cancellationToken)
        {
            Trace.WriteInfo(
                TraceType,
                "StartUpgradeFabricAsync - Started");

            var rollingUpgradeMonitoringPolicy = new RollingUpgradeMonitoringPolicy()
            {
                FailureAction = UpgradeFailureAction.Rollback
            };

            var policyDescription = new MonitoredRollingFabricUpgradePolicyDescription()
            {
                UpgradeMode = RollingUpgradeMode.Monitored,
                MonitoringPolicy = rollingUpgradeMonitoringPolicy,
            };

            if (commandDescription != null)
            {
                if (commandDescription.HealthCheckRetryTimeout.HasValue)
                {
                    policyDescription.MonitoringPolicy.HealthCheckRetryTimeout = commandDescription.HealthCheckRetryTimeout.Value;
                }

                if (commandDescription.HealthCheckStableDuration.HasValue)
                {
                    policyDescription.MonitoringPolicy.HealthCheckStableDuration = commandDescription.HealthCheckStableDuration.Value;
                }

                if (commandDescription.HealthCheckWaitDuration.HasValue)
                {
                    policyDescription.MonitoringPolicy.HealthCheckWaitDuration = commandDescription.HealthCheckWaitDuration.Value;
                }

                if (commandDescription.UpgradeDomainTimeout.HasValue)
                {
                    policyDescription.MonitoringPolicy.UpgradeDomainTimeout = commandDescription.UpgradeDomainTimeout.Value;
                }

                if (commandDescription.UpgradeTimeout.HasValue)
                {
                    policyDescription.MonitoringPolicy.UpgradeTimeout = commandDescription.UpgradeTimeout.Value;
                }

                if (commandDescription.ForceRestart.HasValue)
                {
                    policyDescription.ForceRestart = commandDescription.ForceRestart.Value;
                }

                if (commandDescription.UpgradeReplicaSetCheckTimeout.HasValue)
                {
                    policyDescription.UpgradeReplicaSetCheckTimeout = commandDescription.UpgradeReplicaSetCheckTimeout.Value;
                }

                if (commandDescription.HealthPolicy != null)
                {
                    policyDescription.HealthPolicy = new ClusterHealthPolicy
                    {
                        MaxPercentUnhealthyApplications = commandDescription.HealthPolicy.MaxPercentUnhealthyApplications,
                        MaxPercentUnhealthyNodes = commandDescription.HealthPolicy.MaxPercentUnhealthyNodes
                    };

                    if (commandDescription.HealthPolicy.ApplicationHealthPolicies != null)
                    {
                        foreach (var commandProcessorApplicationHealthPolicyKeyValue in commandDescription.HealthPolicy.ApplicationHealthPolicies)
                        {
                            CommandProcessorApplicationHealthPolicy commandProcessorApplicationHealthPolicy =
                                commandProcessorApplicationHealthPolicyKeyValue.Value;

                            if (commandProcessorApplicationHealthPolicy == null)
                            {
                                continue;                                
                            }

                            var applicationHealthPolicy = new ApplicationHealthPolicy();
                            if (commandProcessorApplicationHealthPolicy.DefaultServiceTypeHealthPolicy != null)
                            {
                                applicationHealthPolicy.DefaultServiceTypeHealthPolicy = new ServiceTypeHealthPolicy
                                {
                                    MaxPercentUnhealthyServices = commandProcessorApplicationHealthPolicy.DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyServices
                                };
                            }

                            if (commandProcessorApplicationHealthPolicy.SerivceTypeHealthPolicies != null)
                            {
                                foreach (var commandProcessorServiceTypeHealthPolicyKeyValue in commandProcessorApplicationHealthPolicy.SerivceTypeHealthPolicies)
                                {
                                    if (commandProcessorServiceTypeHealthPolicyKeyValue.Value == null)
                                    {
                                        continue;
                                    }

                                    ServiceTypeHealthPolicy serviceTypeHealthPolicy = new ServiceTypeHealthPolicy
                                    {
                                        MaxPercentUnhealthyServices = commandProcessorServiceTypeHealthPolicyKeyValue.Value.MaxPercentUnhealthyServices
                                    };

                                    applicationHealthPolicy.ServiceTypeHealthPolicyMap.Add(commandProcessorServiceTypeHealthPolicyKeyValue.Key, serviceTypeHealthPolicy);
                                }
                            }

                            policyDescription.ApplicationHealthPolicyMap.Add(new Uri(commandProcessorApplicationHealthPolicyKeyValue.Key), applicationHealthPolicy);
                        }                        
                    }
                }

                if (commandDescription.DeltaHealthPolicy != null)
                {
                    policyDescription.EnableDeltaHealthEvaluation = true;
                    policyDescription.UpgradeHealthPolicy = new ClusterUpgradeHealthPolicy()
                    {
                        MaxPercentDeltaUnhealthyNodes = commandDescription.DeltaHealthPolicy.MaxPercentDeltaUnhealthyNodes,
                        MaxPercentUpgradeDomainDeltaUnhealthyNodes = commandDescription.DeltaHealthPolicy.MaxPercentUpgradeDomainDeltaUnhealthyNodes
                    };
                }
            };

            // Specify the target code and configuration version and upgrade mode.
            var upgradeDescription = new FabricUpgradeDescription()
            {
                TargetCodeVersion = targetCodeVersion,
                TargetConfigVersion = targetConfigVersion,
                UpgradePolicyDescription = policyDescription
            };

            Trace.WriteInfo(TraceType, "Start upgrade");

            await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() =>
                this.fabricClient.ClusterManager.UpgradeFabricAsync(
                    upgradeDescription, 
                    timeoutHelper.GetOperationTimeout(), 
                    cancellationToken),
                FabricClientRetryErrors.UpgradeFabricErrors.Value,
                timeoutHelper.GetOperationTimeout(),                 
                cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Get version from cluster manifest file
        /// </summary>
        /// <param name="configFilePath"></param>
        /// <returns></returns>
        internal static string GetConfigVersion(string configFilePath)
        {
            if (string.IsNullOrWhiteSpace(configFilePath))
            {
                return string.Empty;
            }

            var xml = XDocument.Load(configFilePath);
            return GetConfigVersion(xml);
        }

        private static string GetConfigVersion(XDocument manifestXml)
        {
            if (manifestXml == null || manifestXml.Root == null)
            {
                throw new InvalidOperationException("Root element missing in cluster config");
            }

            var rootElement = manifestXml.Root;
            var verAttr = rootElement.Attribute("Version");
            if (verAttr == null)
            {
                throw new InvalidOperationException("Version value not present in the cluster config file");
            }

            return verAttr.Value;
        }

        /// <summary>
        /// Parses cluster manifest file and return image connection string.
        /// </summary>
        /// <param name="clusterConfig"></param>
        /// <returns></returns>
        private static string GetImageStoreConnectionString(string clusterConfig)
        {
            using (var reader = XmlReader.Create(
                new StringReader(clusterConfig)
#if !DotNetCoreClr
                , new XmlReaderSettings() { XmlResolver = null }
#endif
                ))
            {
                var xml = XDocument.Load(reader);
                if (xml.Root == null)
                {
                    throw new InvalidOperationException("Root element not present");
                }

                var rootElement = xml.Root;
                var defaultNamespace = rootElement.GetDefaultNamespace().NamespaceName;
                var fabricSettingsElements = rootElement.Element(XName.Get("FabricSettings", defaultNamespace));
                if (fabricSettingsElements == null)
                {
                    throw new InvalidOperationException("FabricSettings Element not present");
                }

                var mgmtElement = fabricSettingsElements.Elements(XName.Get("Section", defaultNamespace)).Where(elem =>
                {
                    var attribute = elem.Attribute("Name");
                    return attribute != null && attribute.Value.Equals("Management");
                });

                var storeElement = mgmtElement.Elements().SingleOrDefault(elem =>
                {
                    var attribute = elem.Attribute("Name");
                    return attribute != null && attribute.Value.Equals("ImageStoreConnectionString");

                });

                if (storeElement == null)
                {
                    throw new InvalidOperationException("ImageStoreConnectionString parameter not present");
                }

                var valueAttr = storeElement.Attribute("Value");
                if (valueAttr == null)
                {
                    throw new InvalidOperationException("ImageStoreConnectionString value not present");
                }

                return valueAttr.Value;
            }
        }

        internal static bool IsUpgradeCompleted(FabricUpgradeState upgradeState)
        {
            return upgradeState == FabricUpgradeState.RollingBackCompleted ||
                   upgradeState == FabricUpgradeState.RollingForwardCompleted;
        }

        private static bool IsUpgradeInUnSupportedState(FabricUpgradeState upgradeState)
        {
            return upgradeState == FabricUpgradeState.Failed ||
                   upgradeState == FabricUpgradeState.Invalid ||
                   upgradeState == FabricUpgradeState.RollingForwardPending;
        }
    }
}
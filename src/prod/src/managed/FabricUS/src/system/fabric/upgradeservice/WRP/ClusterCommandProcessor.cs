// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using IO;
    using Linq;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Fabric.WRP.Model;
    using System.Threading.Tasks;
    using Threading;

    internal class ClusterCommandProcessor : ResourceCommandProcessor
    {
        private readonly IConfigStore configStore;
        private readonly string configSectionName;
        private readonly NodeStatusManager nodeStatusManager;
        private readonly IFabricClientWrapper fabricClientWrapper;
        private readonly CommandParameterGenerator commandParameterGenerator;
        private readonly JsonSerializer serializer;                

        public ClusterCommandProcessor(
            IConfigStore configStore,
            string configSectionName,
            IFabricClientWrapper fabricClientWrapper,
            CommandParameterGenerator commandParameterGenerator,            
            NodeStatusManager nodeStatusManager,
            JsonSerializer jsonSerializer)
        {
            configStore.ThrowIfNull(nameof(configStore));
            configSectionName.ThrowIfNullOrWhiteSpace(nameof(configSectionName));
            fabricClientWrapper.ThrowIfNull(nameof(fabricClientWrapper));
            commandParameterGenerator.ThrowIfNull(nameof(commandParameterGenerator));
            nodeStatusManager.ThrowIfNull(nameof(nodeStatusManager));
            jsonSerializer.ThrowIfNull(nameof(jsonSerializer));

            this.configStore = configStore;
            this.configSectionName = configSectionName;
            this.fabricClientWrapper = fabricClientWrapper;
            this.commandParameterGenerator = commandParameterGenerator;            
            this.nodeStatusManager = nodeStatusManager;
            this.serializer = jsonSerializer;          
        }

        public override ResourceType Type { get; } = ResourceType.Cluster;
        public override TraceType TraceType { get; } = new TraceType("ClusterCommandProcessor");

        public override async Task<IOperationStatus> CreateOperationStatusAsync(IOperationDescription operationDescription, IOperationContext context)
        {
            context.ThrowIfNull(nameof(context));

            ClusterOperationDescription description = operationDescription == null ? null : operationDescription as ClusterOperationDescription;             

            Trace.WriteInfo(TraceType, "CreateOperationStatusAsync: begin");
            IOperationStatus status = null;

            try
            {
                Trace.WriteInfo(TraceType, "CreateOperationStatusAsync: performing operation");
                var errorDetails = await DoOperation(description, context);

                Trace.WriteInfo(TraceType, "CreateOperationStatusAsync: building status");
                status = await BuildStatusAsync(description, context, errorDetails);
            }
            catch (Exception ex)
            {
                Trace.WriteError(this.TraceType, $"CreateOperationStatusAsync: Exception encountered: {ex}");
            }

            Trace.WriteInfo(TraceType, "CreateOperationStatusAsync: end");
            return status;
        }

        private async Task<ClusterErrorDetails> DoOperation(ClusterOperationDescription description, IOperationContext context)
        {
            if (description == null)
            {
                Trace.WriteInfo(TraceType, "CreateOperationStatusAsync: operation description is null. No operation to perform.");
                return null;
            }

            var upgradeTask = ClusterTask(
                () => this.ProcessClusterUpgradeAsync(description.UpgradeDescription, context.CancellationToken),
                ClusterOperation.ProcessClusterUpgrade);

            var enableNodesTask = ClusterTask(
                () => this.fabricClientWrapper.EnableNodesAsync(description.NodesToEnabled, Constants.MaxOperationTimeout, context.CancellationToken),
                ClusterOperation.EnableNodes);

            var disableNodesTask = ClusterTask(
                () => this.fabricClientWrapper.DisableNodesAsync(description.NodesToDisabled, Constants.MaxOperationTimeout, context.CancellationToken),
                ClusterOperation.DisableNodes);

            var updateSystemServiceSizeTask = ClusterTask(
                () => this.fabricClientWrapper.UpdateSystemServicesDescriptionAsync(
                    description.SystemServiceDescriptionsToSet,
                    Constants.MaxOperationTimeout,
                    context.CancellationToken),
                ClusterOperation.UpdateSystemServicesReplicaSetSize);
            
            var nodeListForAutoScaleTask = ClusterTask(
                () => this.nodeStatusManager.ProcessWRPResponse(
                    description.ProcessedNodesStatus,
                    Constants.KvsCommitTimeout,
                    context.CancellationToken),
                ClusterOperation.ProcessNodesStatus);

            var errors = await Task.WhenAll(
                upgradeTask, 
                enableNodesTask, 
                disableNodesTask, 
                nodeListForAutoScaleTask,
                updateSystemServiceSizeTask);

            var taskErrors = errors.Where(item => item != null).ToList();
            if (!taskErrors.Any())
            {
                return null;
            }

            return new ClusterErrorDetails()
            {
                Errors = new List<ClusterOperationError>(taskErrors)
            };
        }        

        private async Task<ClusterOperationError> ClusterTask(Func<Task> task, ClusterOperation operation, bool treatErrorsAsTransient = false)
        {
            try
            {
                await task();                
            }
            catch (Exception ex)
            {
                return new ClusterOperationError
                {
                    Operation = operation,
                    Transient = treatErrorsAsTransient || IsTransientException(ex),
                    ErrorDetails = JObject.FromObject(ex)
                };
            }

            return null;
        }

        private async Task<IOperationStatus> BuildStatusAsync(ClusterOperationDescription description, IOperationContext context, ClusterErrorDetails errorDetails)
        {            
            var upgradeProgressTask = this.fabricClientWrapper.GetFabricUpgradeProgressAsync(Constants.MaxOperationTimeout, context.CancellationToken);

            var nodeListQueryTask = this.fabricClientWrapper.GetNodeListAsync(Constants.MaxOperationTimeout, context.CancellationToken);

            // Only query for system services when WRP needs to adjust the replica set size
            Task<Dictionary<string, ServiceRuntimeDescription>> systemServiceSizeQueryTask = null;
            if (description != null &&
                description.SystemServiceDescriptionsToSet != null &&
                description.SystemServiceDescriptionsToSet.Any())
            {
                systemServiceSizeQueryTask =
                    this.fabricClientWrapper.GetSystemServiceRuntimeDescriptionsAsync(Constants.MaxOperationTimeout, context.CancellationToken);
            }

            await Task.WhenAll(
                nodeListQueryTask,
                upgradeProgressTask,                
                systemServiceSizeQueryTask ?? Task.FromResult<Dictionary<string, ServiceRuntimeDescription>>(null));

            FabricUpgradeProgress currentUpgradeProgress = GetResultFromTask(upgradeProgressTask);            
            NodeList nodeList = FilterPrimaryNodeTypesStatus(GetResultFromTask(nodeListQueryTask), description?.PrimaryNodeTypes);
            Dictionary<string, ServiceRuntimeDescription> systemServicesRuntimeDescriptions = GetResultFromTask(systemServiceSizeQueryTask);
            
            List<PaasNodeStatusInfo> nodesDisabled = null;
            List<PaasNodeStatusInfo> nodesEnabled = null;
            if (description != null &&
                nodeList != null)
            {                
                Trace.WriteNoise(TraceType, "BuildStatusAsync: Begin this.nodeStatusManager.ProcessNodeQuery.");
                await this.nodeStatusManager.ProcessNodeQuery(nodeList, Constants.KvsCommitTimeout, context.CancellationToken);
                Trace.WriteNoise(TraceType, "BuildStatusAsync: End this.nodeStatusManager.ProcessNodeQuery.");

                // Send back the list of nodes that are disabled
                if (description.NodesToDisabled != null)
                {
                    nodesDisabled = new List<PaasNodeStatusInfo>();

                    // Send back the Instance# for the requested disabled nodes
                    foreach (var nodeToDisable in description.NodesToDisabled)
                    {
                        var matchingNodeStatus =
                            nodeList.FirstOrDefault(
                                node =>
                                    string.Equals(node.NodeName, nodeToDisable.NodeName,
                                        StringComparison.OrdinalIgnoreCase));
                        if (matchingNodeStatus != null && matchingNodeStatus.NodeStatus == NodeStatus.Disabled)
                        {
                            var nodeDisabled = new PaasNodeStatusInfo(nodeToDisable) { NodeState = NodeState.Disabled };

                            nodesDisabled.Add(nodeDisabled);

                            Trace.WriteInfo(TraceType, "BuildStatusAsync: Node has been successfully disabled. {0}", nodeDisabled);
                        }
                    }
                }

                // Send back the list of nodes that are Enabled
                if (description.NodesToEnabled != null)
                {
                    nodesEnabled = new List<PaasNodeStatusInfo>();

                    // Send back the Instance# for the requested up nodes
                    foreach (var nodeToEnable in description.NodesToEnabled)
                    {
                        var matchingNodeStatus =
                            nodeList.FirstOrDefault(
                                node =>
                                    string.Equals(node.NodeName, nodeToEnable.NodeName,
                                        StringComparison.OrdinalIgnoreCase));

                        // Since a node can be enabled and can still be down, we infer enabled status instead.
                        if (matchingNodeStatus != null
                            && (matchingNodeStatus.NodeStatus != NodeStatus.Disabling)
                            && (matchingNodeStatus.NodeStatus != NodeStatus.Disabled)
                            && (matchingNodeStatus.NodeStatus != NodeStatus.Enabling))
                        {
                            var nodeEnabled = new PaasNodeStatusInfo(nodeToEnable);
                            nodeEnabled.NodeState = NodeState.Enabled;

                            nodesEnabled.Add(nodeEnabled);

                            Trace.WriteInfo(TraceType, "BuildStatusAsync: Node has been successfully enabled. {0}", nodeEnabled);
                        }
                    }
                }
            }

            Trace.WriteNoise(TraceType, "BuildStatusAsync: Begin this.nodeStatusManager.GetNodeStates.");
            var nodesStatus = await this.nodeStatusManager.GetNodeStates(Constants.KvsCommitTimeout, context.CancellationToken);
            Trace.WriteNoise(TraceType, "BuildStatusAsync: End this.nodeStatusManager.GetNodeStates.");

            var status = new ClusterOperationStatus(description)
            {   
                DisabledNodes = nodesDisabled,
                EnabledNodes = nodesEnabled,
                NodesStatus = nodesStatus,
                SystemServiceDescriptions = systemServicesRuntimeDescriptions,             
            };

            if (currentUpgradeProgress != null)
            {
                status.Progress = JObject.FromObject(currentUpgradeProgress, this.serializer);
            }            

            if (errorDetails != null)
            {
                status.ErrorDetails = JObject.FromObject(errorDetails, this.serializer);
            }

            return status;
        }       

        private async Task ProcessClusterUpgradeAsync(
            PaasClusterUpgradeDescription upgradeDesc,
            CancellationToken token)
        {
            if (upgradeDesc == null)
            {
                return;
            }

            var upgradeProgressTask = this.fabricClientWrapper.GetFabricUpgradeProgressAsync(Constants.MaxOperationTimeout, token);
            var clusterHealthQueryTask = this.fabricClientWrapper.GetClusterHealthAsync(Constants.MaxOperationTimeout, token);
            
            await Task.WhenAll(
                upgradeProgressTask,
                clusterHealthQueryTask);

            FabricUpgradeProgress currentUpgradeProgress = GetResultFromTask(upgradeProgressTask);
            ClusterHealth currentClusterHealth = GetResultFromTask(clusterHealthQueryTask);

            ClusterUpgradeCommandParameter upgradeCommandParameter = null;

            try
            {
                upgradeCommandParameter = await this.commandParameterGenerator.GetCommandParameterAsync(
                    currentUpgradeProgress,
                    currentClusterHealth,
                    upgradeDesc,
                    token);

                await UpgradeClusterAsync(upgradeCommandParameter, token);
            }
            finally
            {
                Cleanup(upgradeCommandParameter);
            }
        }

        private async Task UpgradeClusterAsync(ClusterUpgradeCommandParameter upgradeCommandParameter, CancellationToken token)
        {
            if (upgradeCommandParameter == null)
            {
                Trace.WriteInfo(TraceType, "upgradeCommandParameter is null. No op.");
                return;
            }

            if (string.IsNullOrWhiteSpace(upgradeCommandParameter.ConfigFilePath) &&
                string.IsNullOrWhiteSpace(upgradeCommandParameter.CodeFilePath))
            {
                Trace.WriteInfo(TraceType, "upgradeCommandParameter .ConfigFilePath=null and .CodeFilePath=null. No op.");
                return;
            }

            Trace.WriteInfo(TraceType,
                "Upgrade command parameters: CodePath: {0}, CodeVersion: {1}, ConfigPath: {2}, ConfigVersion: {3}",
                upgradeCommandParameter.CodeFilePath, upgradeCommandParameter.CodeVersion,
                upgradeCommandParameter.ConfigFilePath, upgradeCommandParameter.ConfigVersion);

            // Determine targetCodeVersion & targetConfigVersion
            var targetCodeVersion = upgradeCommandParameter.CodeVersion;
            var targetConfigVersion = upgradeCommandParameter.ConfigVersion;

            if (!string.IsNullOrWhiteSpace(upgradeCommandParameter.ConfigFilePath) &&
                string.IsNullOrWhiteSpace(targetConfigVersion))
            {
                // If config version is not mentioned then read it from given manifest file
                targetConfigVersion = FabricClientWrapper.GetConfigVersion(upgradeCommandParameter.ConfigFilePath);
            }

            // Get root image store path
            var imageStorePath = GetImageStorePath(targetCodeVersion, targetConfigVersion);

            // Build cluster package file paths in image store
            var configFileName = string.Empty;
            var codeFileName = string.Empty;
            var configPathInImageStore = string.Empty;
            var codePathInImageStore = string.Empty;

            if (!string.IsNullOrWhiteSpace(upgradeCommandParameter.ConfigFilePath))
            {
                configFileName = Path.GetFileName(upgradeCommandParameter.ConfigFilePath);
                configPathInImageStore = Path.Combine(imageStorePath, configFileName);
            }

            if (!string.IsNullOrWhiteSpace(upgradeCommandParameter.CodeFilePath))
            {
                codeFileName = Path.GetFileName(upgradeCommandParameter.CodeFilePath);
                codePathInImageStore = Path.Combine(imageStorePath, codeFileName);
            }

            // Determine cluster package content that needs to be published.
            var configFilePath = upgradeCommandParameter.ConfigFilePath;
            var codeFilePath = upgradeCommandParameter.CodeFilePath;

            if (!string.IsNullOrWhiteSpace(targetCodeVersion))
            {
                var provisionedCodeList = await this.fabricClientWrapper.GetProvisionedFabricCodeVersionListAsync(targetCodeVersion, Constants.MaxOperationTimeout, token);
                if (provisionedCodeList.Count != 0)
                {
                    Trace.WriteInfo(TraceType, "Code Already provisioned: {0}", targetCodeVersion);
                    codePathInImageStore = null;
                    codeFilePath = null;
                }
            }

            if (!string.IsNullOrWhiteSpace(targetConfigVersion))
            {
                var provisionedConfigList = await this.fabricClientWrapper.GetProvisionedFabricConfigVersionListAsync(targetConfigVersion, Constants.MaxOperationTimeout, token);
                if (provisionedConfigList.Count != 0)
                {
                    Trace.WriteInfo(TraceType, "Config Already provisioned: {0}", targetConfigVersion);
                    configPathInImageStore = null;
                    configFilePath = null;
                }
            }

            if (string.IsNullOrWhiteSpace(configPathInImageStore) &&
                string.IsNullOrWhiteSpace(codePathInImageStore))
            {
                Trace.WriteInfo(TraceType, "Both code={0} and config={1} are already provisioned. No op.", targetCodeVersion, targetConfigVersion);
            }
            else
            {
                Trace.WriteInfo(
                TraceType,
                "UpgradeClusterAsync: CodePath {0}, CodeVersion {1}, ConfigPath {2}, ConfigVersion {3}",
                codeFilePath ?? "null",
                targetCodeVersion ?? "null",
                configFilePath ?? "null",
                targetConfigVersion ?? "null");

                var imageStoreConnectionString = this.configStore.ReadUnencryptedString(Constants.ManagementSection.ManagementSectionName, Constants.ManagementSection.ImageStoreConnectionString);

                var copyTimeout = this.configStore.ReadTimeSpan(this.configSectionName, Constants.ConfigurationSection.CopyClusterPackageTimeoutInSeconds, TimeSpan.FromSeconds(180));

                Trace.WriteInfo(
                    TraceType,
                    "UpgradeClusterAsync: CopyClusterPackageAsync: Config:{0}->{1} Code:{2}->{3} Timeout:{4}",
                    configFilePath ?? "null",
                    configPathInImageStore ?? "null",
                    codeFilePath ?? "null",
                    codePathInImageStore ?? "null",
                    copyTimeout);

                await this.fabricClientWrapper.CopyClusterPackageAsync(
                    imageStoreConnectionString,
                    configFilePath,
                    configPathInImageStore,
                    codeFilePath,
                    codePathInImageStore,
                    copyTimeout,
                    token);

                Trace.WriteInfo(
                    TraceType,
                    "UpgradeClusterAsync: CopyClusterPackageAsync - Completed");

                var provisionTimeout = this.configStore.ReadTimeSpan(this.configSectionName, Constants.ConfigurationSection.ProvisionClusterPackageTimeoutInSeconds, TimeSpan.FromSeconds(300));

                Trace.WriteInfo(
                    TraceType,
                    "UpgradeClusterAsync: ProvisionFabricAsync: Config:{0} Code:{1} Timeout:{2}",
                    configPathInImageStore ?? "null",
                    codePathInImageStore ?? "null",
                    provisionTimeout);

                try
                {
                    await this.fabricClientWrapper.ProvisionFabricAsync(
                        codePathInImageStore,
                        configPathInImageStore,
                        provisionTimeout,
                        token);

                    Trace.WriteInfo(
                        TraceType,
                        "UpgradeClusterAsync: ProvisionFabricAsync - Completed");
                }
                finally
                {
                    try
                    {
                        Trace.WriteInfo(
                            TraceType,
                            "UpgradeClusterAsync: RemoveClusterPackageAsync: Config:{0} Code:{1} Timeout:{2}",
                            configPathInImageStore ?? "null",
                            codePathInImageStore ?? "null",
                            copyTimeout);

                        await this.fabricClientWrapper.RemoveClusterPackageAsync(
                            imageStoreConnectionString,
                            codePathInImageStore,
                            configPathInImageStore,
                            copyTimeout,
                            token);

                        Trace.WriteInfo(
                            TraceType,
                            "UpgradeClusterAsync: RemoveClusterPackageAsync: Config:{0} Code:{1} - Completed",
                            configPathInImageStore ?? "null",
                            codePathInImageStore ?? "null");
                    }
                    catch (Exception ex)
                    {
                        Trace.WriteWarning(
                            TraceType,
                            "UpgradeClusterAsync: RemoveClusterPackageAsync: Config:{0} Code:{1}. Failed with exception: {2}",
                            configPathInImageStore ?? "null",
                            codePathInImageStore ?? "null",
                            ex);
                    }
                }
            }            

            var startUpgradeTimeout = this.configStore.ReadTimeSpan(this.configSectionName, Constants.ConfigurationSection.StartClusterUpgradeTimeoutInSeconds, TimeSpan.FromSeconds(180));

            Trace.WriteInfo(
                TraceType,
                "UpgradeClusterAsync: CodeVersion:{0} ConfigVersion:{1} Timeout:{2}",
                targetCodeVersion ?? "null",
                targetConfigVersion ?? "null",
                startUpgradeTimeout);

            await this.fabricClientWrapper.UpgradeFabricAsync(upgradeCommandParameter.UpgradeDescription, targetCodeVersion, targetConfigVersion, startUpgradeTimeout, token);

            Trace.WriteInfo(
                TraceType,
                "UpgradeClusterAsync: CodeVersion:{0} ConfigVersion:{1} - Started",
                targetCodeVersion ?? "null",
                targetConfigVersion ?? "null");
        }

        private static string GetImageStorePath(
            string targetCodeVersion,
            string targetConfigVersion)
        {
            var imageStorePath = string.IsNullOrWhiteSpace(targetCodeVersion) ? string.Empty : targetCodeVersion;
            imageStorePath += string.IsNullOrWhiteSpace(targetConfigVersion) ? string.Empty : "_" + targetConfigVersion;

            return imageStorePath.Trim('_');
        }

        /// <summary>
        /// Tries to delete temp folder.
        /// </summary>
        /// <param name="parameter"></param>
        private void Cleanup(ClusterUpgradeCommandParameter parameter)
        {
            if (parameter == null)
            {
                return;
            }

            DeleteFileDirectory(parameter.CodeFilePath);
            DeleteFileDirectory(parameter.ConfigFilePath);
        }

        private void DeleteFileDirectory(string fileName)
        {
            try
            {
                if (!string.IsNullOrWhiteSpace(fileName))
                {
                    var codeFileDir = Path.GetDirectoryName(fileName);
                    if (Directory.Exists(codeFileDir))
                    {
                        Directory.Delete(codeFileDir, true);
                    }
                }
            }
            catch
            {
                Trace.WriteWarning(TraceType, "Cleanup failed: Directory {0} couldn't be deleted", fileName);
            }
        }

        private static TResult GetResultFromTask<TResult>(Task<TResult> task)
        {
            return (task != null && task.Status == TaskStatus.RanToCompletion) ? task.Result : default(TResult);
        }

        private NodeList FilterPrimaryNodeTypesStatus(NodeList nodes, List<string> primaryNodeTypes)
        {
            if (nodes == null || !nodes.Any())
            {
                return nodes;
            }

            if (primaryNodeTypes == null || !primaryNodeTypes.Any())
            {
                return nodes;
            }

            var nodeList = new NodeList();
            foreach (var node in nodes)
            {
                if (primaryNodeTypes.Contains(node.NodeType))
                {
                    nodeList.Add(node);
                }
            }

            return nodeList;
        }        
    }
}

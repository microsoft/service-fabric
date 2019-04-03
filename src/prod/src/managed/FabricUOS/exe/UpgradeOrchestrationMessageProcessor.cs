// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeOrchestration.Service
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Query;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager;
    using Microsoft.ServiceFabric.DeploymentManager.BPA;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Microsoft.ServiceFabric.DeploymentManager.Model;
    using Newtonsoft.Json;

    using DMConstants = Microsoft.ServiceFabric.DeploymentManager.Constants;

    using FabricNativeConfigStore = System.Fabric.Common.NativeConfigStore;
    using FABRIC_ERROR_CODE = Interop.NativeTypes.FABRIC_ERROR_CODE;

    internal class UpgradeOrchestrationMessageProcessor
    {
        private const string TraceType = "UOSMessageProcessor";
        private readonly StoreManager storeManager;
        private CancellationToken cancellationToken;
        private FabricClient fabricClient;
        private UpgradeOrchestrator orchestrator;

        public UpgradeOrchestrationMessageProcessor(
            StoreManager storeManager,
            FabricClient fc,
            UpgradeOrchestrator orchestrator,
            CancellationToken cancellationToken)
        {
            this.storeManager = storeManager;
            this.fabricClient = fc;
            this.cancellationToken = cancellationToken;
            this.orchestrator = orchestrator;
        }

        public CancellationToken CancellationToken
        {
            set
            {
                this.cancellationToken = value;
            }
        }

        /// <summary>
        /// process config upgrade message and kick off the config upgrade
        /// </summary>
        /// <param name="jsonConfig">serialized json config</param>
        /// <param name="timeout">timeout config</param>
        /// <param name="cancellationToken">cancellation token</param>
        /// <returns></returns>
        [SuppressMessage("StyleCop.CSharp.DocumentationRules", "SA1650:ElementDocumentationMustBeSpelledCorrectly", Justification = "Words like json are valid to use.")]
        public async Task ProcessStartClusterConfigurationUpgradeAsync(ConfigurationUpgradeDescription configUpgradeDesc, TimeSpan timeout, CancellationToken cancellationToken)
        {
            /* The cancellation token passed in this API call is not used (by design). This token corresponds to the client side call. 
            The global this.cancellationToken is initialised in RunAsync() and is honored in every API call. */

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Entering ProcessStartUpgradeAsync.");
            try
            {
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Deserializing input json config string.");
                StandAloneInstallerJsonModelBase targetJsonConfig = StandAloneInstallerJsonModelBase.GetJsonConfigFromString(configUpgradeDesc.ClusterConfiguration);
                if (targetJsonConfig == null)
                {
                    throw new ArgumentException("The input cluster configuration is not in a valid json format or supported apiVersion.");
                }

                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Retrieve current cluster resource from StoreManager.");
                StandAloneCluster cluster = await this.storeManager.GetClusterResourceAsync(
                                                  Constants.ClusterReliableDictionaryKey, this.cancellationToken).ConfigureAwait(false);

                bool isInDataLossState = await DatalossHelper.IsInDatalossStateAsync(this.cancellationToken).ConfigureAwait(false);
                if (!isInDataLossState)
                {
                    if (cluster == null || cluster.Current == null)
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "Persisted cluster resource is not ready: {0}", cluster == null ? "null" : "current = null");
                        throw new FabricException("UpgradeOrchestrationService is not ready.");
                    }

                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Setting target config and topology based on new input json config.");

                    if (cluster.Pending != null && !this.IsInterruptibleAsync(cluster.Pending).Result)
                    {
                        throw new FabricException(string.Format("Cluster configuration upgrade of type {0} is already in progress and cannot be interrupted.", cluster.Pending.GetType().Name));
                    }

                    StandaloneSettingsValidator validator = new StandaloneSettingsValidator(targetJsonConfig);

                    await UpgradeOrchestrationMessageProcessor.ValidateModel(targetJsonConfig, validator, cluster, true).ConfigureAwait(false);

                    var removedNodes = validator.GetRemovedNodes(cluster.Topology);
                    var addedNodes = validator.GetAddedNodes(cluster.Topology);

                    if (addedNodes.Any() && StandaloneUtility.CheckFabricRunningAsGMSA(cluster.Current.CSMConfig))
                    {
                        /* Need to resolve assembly so that FabricDeployer can load the right binaries from FabricCodePath since not all binaries required by FabricDeployer are present in UOS.Current folder.*/
                        AppDomain.CurrentDomain.AssemblyResolve += new ResolveEventHandler(this.LoadFromFabricCodePath);
                        try
                        {
                            await this.PerformAddNodeOperationGMSAAsync(addedNodes, this.fabricClient, validator.ClusterProperties.NodeTypes).ConfigureAwait(false);
                        }
                        catch (AggregateException ex)
                        {
                            UpgradeOrchestrationTrace.TraceSource.WriteError(TraceType, "Adding nodes for GMSA scenario failed with exception: {0}", ex);
                            throw;
                        }
                        finally
                        {
                            AppDomain.CurrentDomain.AssemblyResolve -= new ResolveEventHandler(this.LoadFromFabricCodePath);
                        }
                    }

                    if (addedNodes.Any())
                    {
                        cluster.TargetNodeConfig = GetTargetNodeConfigAddNode(validator.Topology, cluster.Current.NodeConfig.Version);
                    }

                    if (removedNodes.Any())
                    {
                        cluster.TargetNodeConfig = GetTargetNodeConfigRemoveNode(cluster.Topology, removedNodes, cluster.Current.NodeConfig.Version);
                    }
                    else
                    {
                        cluster.Topology = validator.Topology;
                    }

                    cluster.TargetCsmConfig = validator.ClusterProperties;

                    // Cluster is updated above so persist it.
                    await this.storeManager.PersistClusterResourceAsync(Constants.ClusterReliableDictionaryKey, cluster, cancellationToken).ConfigureAwait(false);

                    await this.UpdatePersistedCodeUpgradePackage(validator).ConfigureAwait(false);
                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Invoking Orchestrator");

                    await this.orchestrator.StartUpgradeAsync(cluster, this.cancellationToken, configUpgradeDesc).ContinueWith(t =>
                    {
                        if (t.Exception != null)
                        {
                            UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "Orchestrator completed with status: {0} exception: {1}", t.Status, t.Exception);
                        }
                        else
                        {
                            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Orchestrator completed with status: {0}", t.Status);
                        }
                    });
                }
                else
                {
                    StandaloneSettingsValidator validator = new StandaloneSettingsValidator(targetJsonConfig);
                    await UpgradeOrchestrationMessageProcessor.ValidateModel(targetJsonConfig, validator, cluster, false).ConfigureAwait(false);

                    cluster = FabricUpgradeOrchestrationService.ConstructClusterFromJson(targetJsonConfig, FabricNativeConfigStore.FabricGetConfigStore());

                    DatalossHelper.DryRunConfigUpgrade(cluster);
                    await this.storeManager.PersistClusterResourceAsync(Constants.ClusterReliableDictionaryKey, cluster, this.cancellationToken);

                    await DatalossHelper.UpdateHeathStateAsync(isHealthy: true).ConfigureAwait(false);
                }
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "ProcessStartUpgradeAsync exception: {0}", e);
                throw UpgradeOrchestrationMessageProcessor.ConvertToComException(e);
            }

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exiting ProcessStartUpgradeAsync.");
        }

        /// <summary>
        /// Get the progress of the current upgrade
        /// </summary>
        /// <param name="timeout"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public async Task<FabricOrchestrationUpgradeProgress> ProcessGetClusterConfigurationUpgradeProgressAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Entering ProcessGetUpgradeProgressAsync.");
            FabricOrchestrationUpgradeProgress upgradeProgress = null;

            try
            {
                string configVersion = await this.GetCurrentJsonConfigVersionAsync(this.cancellationToken).ConfigureAwait(false);

                FabricUpgradeProgress fabricUpgradeProgress = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () =>
                        this.fabricClient.ClusterManager.GetFabricUpgradeProgressAsync(Constants.UpgradeServiceMaxOperationTimeout, this.cancellationToken),
                        Constants.UpgradeServiceMaxOperationTimeout,
                        this.cancellationToken).ConfigureAwait(false);

                ConfigUpgradeErrorDetail errorDetails = await this.storeManager.GetConfigUpgradeErrorDetailsAsync(Constants.ConfigUpgradeErrorDetails, cancellationToken);

                uint manifestVersion;
                upgradeProgress = new FabricOrchestrationUpgradeProgress()
                {
                    UpgradeState = fabricUpgradeProgress.UpgradeState,
                    ProgressStatus = uint.TryParse(fabricUpgradeProgress.TargetConfigVersion, out manifestVersion) ? manifestVersion : 0,
                    ConfigVersion = configVersion,
                    Details = (errorDetails != null) ? errorDetails.ToString() : null
                };
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "ProcessGetUpgradeProgressAsync exception: {0}", e);
                throw;
            }

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exiting ProcessGetUpgradeProgressAsync.");
            return upgradeProgress;
        }

        public async Task<string> ProcessGetClusterConfigurationAsync(string apiVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Entering ProcessGetClusterConfigurationAsync.");
            try
            {
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Retrieve current cluster resource from StoreManager.");
                StandAloneCluster cluster = await this.storeManager.GetClusterResourceAsync(
                                                  Constants.ClusterReliableDictionaryKey, this.cancellationToken).ConfigureAwait(false);

                if (cluster == null || cluster.Current == null)
                {
                    // read from fabric data root
                    string fabricDataRoot = FabricEnvironment.GetDataRoot();
                    string jsonConfigPath = Path.Combine(fabricDataRoot, System.Fabric.FabricDeployer.Constants.FileNames.BaselineJsonClusterConfig); // TODO: Needs to come from image store
                    return File.ReadAllText(jsonConfigPath);
                }
                else
                {
                    var nodesFromFM = new Dictionary<string, NodeDescription>();
                    NodeList nodes = await StandaloneUtility.GetNodesFromFMAsync(this.fabricClient, this.cancellationToken).ConfigureAwait(false);
                    for (int i = 0; i < nodes.Count; ++i)
                    {
                        if (nodes[i].NodeStatus != System.Fabric.Query.NodeStatus.Invalid && !UpgradeOrchestrationMessageProcessor.IsGhostNode(nodes[i]))
                        {
                            NodeDescription nodeDesc = new NodeDescription()
                            {
                                FaultDomain = nodes[i].FaultDomain.ToString(),
                                UpgradeDomain = nodes[i].UpgradeDomain,
                                IPAddress = nodes[i].IpAddressOrFQDN,
                                NodeTypeRef = nodes[i].NodeType,
                                NodeName = nodes[i].NodeName
                            };
                            nodesFromFM.Add(nodes[i].NodeName, nodeDesc);
                        }
                    }

                    cluster.Topology.Nodes = nodesFromFM;

                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Construct StandAloneJsonModel from current cluster resource.");
                    var jsonModel = StandAloneInstallerJsonModelBase.ConstructByApiVersion(cluster.Current.CSMConfig, cluster.Topology, cluster.Current.CSMConfig.Version.Version, apiVersion);
                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Serializing current json model.");
                    var serializerSettings = new JsonSerializerSettings
                    {
                        Formatting = Formatting.Indented,
                        NullValueHandling = NullValueHandling.Ignore,
                        PreserveReferencesHandling = PreserveReferencesHandling.None
                    };
                    serializerSettings.Converters.Add(new Newtonsoft.Json.Converters.StringEnumConverter());
                    return JsonConvert.SerializeObject(jsonModel, serializerSettings);
                }
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "ProcessStartUpgradeAsync exception: {0}", e);
                throw UpgradeOrchestrationMessageProcessor.ConvertToComException(e);
            }
        }

        public async Task<string> ProcessGetUpgradeOrchestrationServiceStateAsync(string inputBlob, TimeSpan timeout, CancellationToken cancellationToken)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Entering ProcessGetUpgradeOrchestrationServiceStateAsync.");
            try
            {
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Retrieve current cluster resource object from StoreManager.");
                return await this.storeManager.GetStorageObjectAsync(
                                                  Constants.ClusterReliableDictionaryKey, this.cancellationToken).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "ProcessGetUpgradeOrchestrationServiceStateAsync exception: {0}", e);
                throw UpgradeOrchestrationMessageProcessor.ConvertToComException(e);
            }
        }

        public async Task<string> ProcessSetUpgradeOrchestrationServiceStateAsync(string inputBlob, TimeSpan timeout, CancellationToken cancellationToken)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Entering ProcessSetUpgradeOrchestrationServiceStateAsync.");
            try
            {
                // data validation
                StandAloneCluster cluster = null;
                if (!string.IsNullOrWhiteSpace(inputBlob))
                {
                    JsonSerializerSettings settings = StandaloneUtility.GetStandAloneClusterDeserializerSettings();

                    cluster = JsonConvert.DeserializeObject<StandAloneCluster>(
                        inputBlob,
                        settings);
                }

                await this.storeManager.SetStorageObjectAsync(Constants.ClusterReliableDictionaryKey, inputBlob, this.cancellationToken).ConfigureAwait(false);

                FabricUpgradeOrchestrationServiceState result = UpgradeOrchestrationMessageProcessor.ConstructServiceStateFromCluster(cluster);

                return JsonConvert.SerializeObject(
                    result,
                    new JsonSerializerSettings
                    {
                        ReferenceLoopHandling = ReferenceLoopHandling.Serialize,
                        NullValueHandling = NullValueHandling.Ignore,
                    });
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "ProcessSetUpgradeOrchestrationServiceStateAsync exception: {0}", e);
                throw UpgradeOrchestrationMessageProcessor.ConvertToComException(e);
            }
        }

        internal static FabricUpgradeOrchestrationServiceState ConstructServiceStateFromCluster(StandAloneCluster cluster)
        {
            FabricUpgradeOrchestrationServiceState result = new FabricUpgradeOrchestrationServiceState();
            if (cluster != null)
            {
                if (cluster.Current != null && cluster.Current.ExternalState != null)
                {
                    result.CurrentCodeVersion = cluster.Current.ExternalState.MsiVersion;

                    if (cluster.Current.ExternalState.ClusterManifest != null)
                    {
                        result.CurrentManifestVersion = cluster.Current.ExternalState.ClusterManifest.Version;
                    }
                }

                if (cluster.Pending != null)
                {
                    result.PendingUpgradeType = cluster.Pending.GetType().Name;

                    if (cluster.Pending.ExternalState != null)
                    {
                        result.TargetCodeVersion = cluster.Pending.ExternalState.MsiVersion;

                        if (cluster.Pending.ExternalState.ClusterManifest != null)
                        {
                            result.TargetManifestVersion = cluster.Pending.ExternalState.ClusterManifest.Version;
                        }
                    }
                }
            }

            return result;
        }

        internal static COMException ConvertToComException(Exception ex)
        {
            string message = string.Format("{0}: {1}", ex.GetType().Name, ex.Message);
            unchecked
            {
                return new COMException(message, (int)FABRIC_ERROR_CODE.FABRIC_E_CONFIG_UPGRADE_FAILED);
            }
        }

        internal static async Task ValidateModel(StandAloneInstallerJsonModelBase targetJsonConfig, StandaloneSettingsValidator validator, StandAloneCluster cluster, bool validateUpgrade)
        {
            BestPracticesAnalyzer.IsJsonConfigModelValid(targetJsonConfig, oldConfig: null, validateDownNodes: false, throwIfError: true);

            if (validateUpgrade)
            {
                await validator.ValidateUpdateFrom(cluster.Current.CSMConfig, cluster.Topology, connectedToCluster: true).ConfigureAwait(false);
            }
        }

        internal static ClusterNodeConfig GetTargetNodeConfigAddNode(StandAloneClusterTopology topology, long version)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Creating TargetNodeConfig");
            List<Microsoft.ServiceFabric.ClusterManagementCommon.NodeStatus> nodeStatus = new List<Microsoft.ServiceFabric.ClusterManagementCommon.NodeStatus>();

            foreach (var nodeName in topology.Nodes.Keys)
            {                
                nodeStatus.Add(
                    new Microsoft.ServiceFabric.ClusterManagementCommon.NodeStatus
                    {
                        NodeName = nodeName,
                        NodeType = topology.Nodes[nodeName].NodeTypeRef,
                        NodeState = NodeState.Enabled,
                        NodeDeactivationIntent = WrpNodeDeactivationIntent.Invalid,
                        InstanceId = 0
                    });
            }

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Setting NodeStatus for nodes {0}", string.Join(", ", nodeStatus.Select(node => node.NodeName).ToArray()));
            return new ClusterNodeConfig
            {
                Version = version + 1,
                NodesStatus = nodeStatus
            };
        }

        internal static ClusterNodeConfig GetTargetNodeConfigRemoveNode(IClusterTopology topology, List<NodeDescription> removedNodes, long version)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Creating TargetNodeConfig");
            List<Microsoft.ServiceFabric.ClusterManagementCommon.NodeStatus> nodeStatus = new List<Microsoft.ServiceFabric.ClusterManagementCommon.NodeStatus>();

            foreach (var nodeName in topology.Nodes.Keys)
            {
                if (removedNodes.Any(removedNode => removedNode.NodeName == nodeName))
                {
                    nodeStatus.Add(
                    new Microsoft.ServiceFabric.ClusterManagementCommon.NodeStatus
                    {
                        NodeName = nodeName,
                        NodeType = topology.Nodes[nodeName].NodeTypeRef,
                        NodeState = NodeState.Disabling,
                        NodeDeactivationIntent = WrpNodeDeactivationIntent.RemoveNode,
                        InstanceId = 0
                    });
                }
                else
                {
                    nodeStatus.Add(
                    new Microsoft.ServiceFabric.ClusterManagementCommon.NodeStatus
                    {
                        NodeName = nodeName,
                        NodeType = topology.Nodes[nodeName].NodeTypeRef,
                        NodeState = NodeState.Enabled,
                        NodeDeactivationIntent = WrpNodeDeactivationIntent.Invalid,
                        InstanceId = 0
                    });
                }
            }

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Setting NodeStatus for nodes {0}", string.Join(", ", nodeStatus.Select(node => node.NodeName).ToArray()));
            return new ClusterNodeConfig
            {
                Version = version + 1,
                NodesStatus = nodeStatus,
                IsUserSet = true
            };
        }

        internal async Task<bool> IsInterruptibleAsync(ClusterUpgradeStateBase clusterUpgradeState)
        {
            if (!clusterUpgradeState.CanInterruptUpgrade())
            {
                return false;
            }

            FabricUpgradeProgress fabricUpgradeProgress = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                () =>
                    this.fabricClient.ClusterManager.GetFabricUpgradeProgressAsync(Constants.UpgradeServiceMaxOperationTimeout, this.cancellationToken),
                    Constants.UpgradeServiceMaxOperationTimeout,
                    this.cancellationToken).ConfigureAwait(false);

            return fabricUpgradeProgress.UpgradeState != FabricUpgradeState.RollingBackInProgress;
        }

        internal async Task PerformAddNodeOperationGMSAAsync(List<NodeDescription> addedNodes, FabricClient fabricClient, List<NodeTypeDescription> allNodeTypes)
        {
            string destinationPackagePath = await this.DownloadProvisionedPackage().ConfigureAwait(false);           
            var nodesFromFM = await StandaloneUtility.GetNodesFromFMAsync(fabricClient, this.cancellationToken).ConfigureAwait(false);
            string clusterManifestPath = await this.GenerateClusterManifestWithAddedNodes(addedNodes, nodesFromFM, allNodeTypes);

            List<string> machines = new List<string>();
            foreach (var addednode in addedNodes)
            {
                if (!nodesFromFM.Any(nodeFromFM => nodeFromFM.NodeName == addednode.NodeName))
                {
                    machines.Add(addednode.IPAddress);
                }
            }

            bool successfulRun = true;
            List<Exception> exceptions = new List<Exception>(); 
            try
            {
                await this.AddNodesAsync(clusterManifestPath, destinationPackagePath, machines, addedNodes);               
            }
            catch (Exception ex)
            {
                successfulRun = false;
                exceptions.Add(ex);
                UpgradeOrchestrationTrace.TraceSource.WriteError("Adding nodes failed with exception {0}.", ex.ToString());
            }

            if (!successfulRun)
            {
                // Best-Effort Rollback             
                try
                {
                    await DeploymentManagerInternal.RemoveNodeConfigurationAsync(machines, false, FabricPackageType.XCopyPackage, true);
                }
                catch (Exception ex)
                {
                    UpgradeOrchestrationTrace.TraceSource.WriteError("Rollback failed with {0}.", ex.ToString());
                    exceptions.Add(ex);
                }

                AggregateException ae = new AggregateException(exceptions);
                throw ae;
            }

            this.CleanUp(clusterManifestPath, destinationPackagePath);
            UpgradeOrchestrationTrace.TraceSource.WriteInfo("Nodes were succesfully added to the GMSA secured cluster");
        }

        private static bool IsGhostNode(Node node)
        {
            /* Federation has a bug due to which if a node comes up and immediately goes down its state is not fully recorded by FM. If this node does not
            come back up and FM does not see it, then nodeid->nodeName translation is not performed. Such nodes should not be reported by UOS */
            return node.NodeName != null && node.NodeName.StartsWith("nodeid:", StringComparison.OrdinalIgnoreCase);
        }

        private async Task<string> GenerateClusterManifestWithAddedNodes(List<NodeDescription> addedNodes, NodeList nodesFromFM, List<NodeTypeDescription> allNodeTypes)
        {
            string clusterManifestContent = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                                        () =>
                                                        this.fabricClient.ClusterManager.GetClusterManifestAsync(Constants.FabricQueryTimeoutInMinutes, this.cancellationToken),
                                                        Constants.FabricQueryRetryTimeoutInMinutes,
                                                        this.cancellationToken).ConfigureAwait(false);
            var clusterManifest = XMLHelper.ReadClusterManifestFromContent(clusterManifestContent);
            var infrastructureType = clusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureWindowsServer;
            infrastructureType.ThrowIfNull("Node Infrastructure section is not available in the cluster manifest.");
            var nodesList = infrastructureType.NodeList.ToList();
            var nodeTypes = clusterManifest.NodeTypes.ToList();

            foreach (var node in addedNodes)
            {
                if (!nodesFromFM.Any(nodeFromFM => nodeFromFM.NodeName == node.NodeName))
                {
                    nodesList.Add(new FabricNodeType
                    {
                        NodeName = node.NodeName,
                        UpgradeDomain = node.UpgradeDomain,
                        FaultDomain = node.FaultDomain,
                        IPAddressOrFQDN = node.IPAddress,
                        NodeTypeRef = node.NodeTypeRef,
                        IsSeedNode = false
                    });

                    if (!nodeTypes.Any(nt => nt.Name == node.NodeTypeRef))
                    {
                        nodeTypes.Add(this.GetCMNodeTypeFromNodeDescription(allNodeTypes.Single(nt => nt.Name == node.NodeTypeRef)));
                    }
                }
            }

            infrastructureType.NodeList = nodesList.ToArray();
            clusterManifest.NodeTypes = nodeTypes.ToArray();
            string clusterManifestPath = Path.Combine(Path.GetTempPath(), "clusterManifest.xml");
            XMLHelper.WriteClusterManifest(clusterManifestPath, clusterManifest);
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Cluster manifest with newly added nodes written to {0}", clusterManifestPath);
            return clusterManifestPath;
        }

        private async Task<string> DownloadProvisionedPackage()
        {
            FabricUpgradeProgress upgradeProgress;
            upgradeProgress = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync<FabricUpgradeProgress>(
                 () =>
                 this.fabricClient.ClusterManager.GetFabricUpgradeProgressAsync(Constants.FabricQueryTimeoutInMinutes, this.cancellationToken),
                 Constants.FabricQueryRetryTimeoutInMinutes,
                 this.cancellationToken).ConfigureAwait(false);

            string packageName = string.Format("WindowsFabric.{0}.cab", upgradeProgress.TargetCodeVersion);
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Searching for package {0} in the native image store", packageName);
            NativeImageStoreClient imageStore = new NativeImageStoreClient(null, false, System.IO.Path.GetTempPath(), null);
            var content = imageStore.ListContentWithDetails(Constants.WindowsFabricStoreName, false, Constants.FabricQueryTimeoutInMinutes);

            var sourcePackage = content.Files.SingleOrDefault(file => file.StoreRelativePath.Contains(packageName));
            
            if (sourcePackage == null)
            {
                throw new FabricException(string.Format("Source package {0} is not found in the image store. Nodes cannot be added to the cluster. Aborting upgrade.", packageName));
            }
            
            var sourcePackagePath = sourcePackage.StoreRelativePath;
            if (string.IsNullOrEmpty(sourcePackagePath))
            {
                throw new FabricException(string.Format("Source package relative path for {0} is not found in the image store. Nodes cannot be added to the cluster. Aborting upgrade.", packageName));
            }

            string destinationPackagePath = Path.Combine(Path.GetTempPath(), packageName);
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Downloading cab file package from {0} to {1}", sourcePackagePath, destinationPackagePath);
            await imageStore.DownloadContentAsync(sourcePackagePath, destinationPackagePath, Constants.UpgradeServiceMaxOperationTimeout, CopyFlag.AtomicCopy).ConfigureAwait(false);
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Package was succesfully downloaded to {0}", destinationPackagePath);

            if (!File.Exists(destinationPackagePath))
            {
                throw new FabricException(string.Format("Source package {0} could not be downloaded from the image store to {1}. Nodes cannot be added to the cluster. Aborting upgrade.", packageName, destinationPackagePath));
            }

            return destinationPackagePath;
        }

        private async Task AddNodesAsync(string clusterManifestPath, string destinationPackagePath, List<string> machines, List<NodeDescription> addedNodes)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Configuring nodes started.");
            await DeploymentManagerInternal.ConfigureNodesAsync(
                    new MachineHealthContainer(machines),
                    clusterManifestPath,
                    null,
                    null,
                    null,
                    null,
                    FabricPackageType.XCopyPackage,
                    destinationPackagePath,
                    null).ConfigureAwait(false);

            Parallel.ForEach(
                    addedNodes,
                    async (NodeDescription node) =>
                    {
                        if (machines.Any(machine => machine == node.IPAddress))
                        {
                            DeploymentManagerInternal.StartAndWaitForInstallerService(node.IPAddress);
                            UpgradeOrchestrationTrace.TraceSource.WriteInfo("Re-enabling the node {0}.", node.NodeName);
                            await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                   () =>
                                   this.fabricClient.ClusterManager.ActivateNodeAsync(node.NodeName, Constants.FabricQueryTimeoutInMinutes, this.cancellationToken),
                                   Constants.FabricQueryRetryTimeoutInMinutes,
                                   this.cancellationToken).ConfigureAwait(false);
                        }
                    });

            await this.PollAddedNodesActivatedAsync(addedNodes).ConfigureAwait(false);
        }

        private async Task PollAddedNodesActivatedAsync(List<NodeDescription> addedNodes)
        {
            System.Fabric.Common.TimeoutHelper timeoutHelper = new System.Fabric.Common.TimeoutHelper(Constants.FabricPollActivationNodesTimeoutInMinutes);
            bool isActivationComplete = true;
            while (!System.Fabric.Common.TimeoutHelper.HasExpired(timeoutHelper) && !this.cancellationToken.IsCancellationRequested)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Checking activation status of added nodes.");
                try
                {
                    System.Fabric.Query.NodeList nodes = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                      () =>
                                      this.fabricClient.QueryManager.GetNodeListAsync(
                                          null,
                                          Constants.FabricQueryTimeoutInMinutes,
                                          this.cancellationToken),
                                      Constants.FabricQueryRetryTimeoutInMinutes).ConfigureAwait(false);
                    isActivationComplete = true;
                    for (int i = 0; i < nodes.Count; ++i)
                    {
                        if (addedNodes.Any(addedNode => addedNode.NodeName == nodes[i].NodeName)
                            && nodes[i].NodeStatus != System.Fabric.Query.NodeStatus.Up)
                        {
                            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Node {0} has not been activated", nodes[i].IpAddressOrFQDN);
                            isActivationComplete = false;
                            break;
                        }
                    }

                    if (isActivationComplete)
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "All nodes to be added are up. Continuing with upgrade");
                        break;
                    }
                    else
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Waiting for five seconds before polling activation status again.");                        
                    }

                    await Task.Delay(TimeSpan.FromSeconds(5), this.cancellationToken).ConfigureAwait(false);                    
                }
                catch (FabricTransientException fte)
                {
                    UpgradeOrchestrationTrace.TraceSource.WriteError(TraceType, "Retrying Polling Activated Node because of retryable exception {0}", fte);
                }
            }

            timeoutHelper.ThrowIfExpired();
        }

        private void CleanUp(string clusterManifestPath, string destinationPackagePath)
        {
            if (File.Exists(destinationPackagePath))
            {
                try
                {
                    File.Delete(destinationPackagePath);
                }
                catch (Exception ex)
                {
                    UpgradeOrchestrationTrace.TraceSource.WriteError("Failed to delete {0}. Exception thrown: {1}", destinationPackagePath, ex.ToString());
                }
            }

            if (File.Exists(clusterManifestPath))
            {
                try
                {
                    File.Delete(clusterManifestPath);
                }
                catch (Exception ex)
                {
                    UpgradeOrchestrationTrace.TraceSource.WriteError("Failed to delete {0}. Exception thrown: {1}", clusterManifestPath, ex.ToString());
                }
            }
        }

        private async Task<string> GetCurrentJsonConfigVersionAsync(CancellationToken cancellationToken)
        {
            StandAloneCluster cluster = await this.storeManager.GetClusterResourceAsync(
                                              Constants.ClusterReliableDictionaryKey, this.cancellationToken).ConfigureAwait(false);

            if (cluster == null || cluster.Current == null)
            {
                // read from fabric data root
                string fabricDataRoot = FabricEnvironment.GetDataRoot();
                string jsonConfigPath = Path.Combine(fabricDataRoot, System.Fabric.FabricDeployer.Constants.FileNames.BaselineJsonClusterConfig); // TODO: Needs to come from image store
                string jsonConfigString = File.ReadAllText(jsonConfigPath);
                StandAloneInstallerJsonModelBase targetJsonConfig = StandAloneInstallerJsonModelBase.GetJsonConfigFromString(jsonConfigString);
                return targetJsonConfig.ClusterConfigurationVersion;
            }
            else
            {
                return cluster.Current.CSMConfig.Version.Version;
            }
        }

        private ClusterManifestTypeNodeType GetCMNodeTypeFromNodeDescription(NodeTypeDescription nodeTypeDescription)
        {
            var nodeType = nodeTypeDescription.ToClusterManifestTypeNodeType();

            var defaultPlacementProperty = new KeyValuePairType()
            {
                Name = StringConstants.DefaultPlacementConstraintsKey,
                Value = nodeType.Name
            };
            if (nodeType.PlacementProperties == null)
            {
                nodeType.PlacementProperties = new[] { defaultPlacementProperty };
            }
            else
            {
                var existingPlacementProperties = nodeType.PlacementProperties.ToList();
                existingPlacementProperties.Add(defaultPlacementProperty);
                nodeType.PlacementProperties = existingPlacementProperties.ToArray();
            }

            nodeType.Endpoints.ClusterConnectionEndpoint = new InternalEndpointType()
            {
                Port = nodeTypeDescription.ClusterConnectionEndpointPort.ToString(),
                Protocol = InternalEndpointTypeProtocol.tcp
            };

            nodeType.Endpoints.LeaseDriverEndpoint = new InternalEndpointType()
            {
                Port = nodeTypeDescription.LeaseDriverEndpointPort.ToString(),
                Protocol = InternalEndpointTypeProtocol.tcp
            };

            nodeType.Endpoints.ServiceConnectionEndpoint = new InternalEndpointType()
            {
                Port = nodeTypeDescription.ServiceConnectionEndpointPort.ToString(),
                Protocol = InternalEndpointTypeProtocol.tcp
            };

            nodeType.Endpoints.ApplicationEndpoints = new FabricEndpointsTypeApplicationEndpoints()
            {
                StartPort = nodeTypeDescription.ApplicationPorts.StartPort,
                EndPort = nodeTypeDescription.ApplicationPorts.EndPort
            };

            if (nodeTypeDescription.EphemeralPorts != null)
            {
                nodeType.Endpoints.EphemeralEndpoints = new FabricEndpointsTypeEphemeralEndpoints()
                {
                    StartPort = nodeTypeDescription.EphemeralPorts.StartPort,
                    EndPort = nodeTypeDescription.EphemeralPorts.EndPort
                };
            }

            if (nodeTypeDescription.KtlLogger != null)
            {
                nodeType.KtlLoggerSettings = nodeTypeDescription.KtlLogger.ToFabricKtlLoggerSettingsType();
            }

            return nodeType;
        }

        private Assembly LoadFromFabricCodePath(object sender, ResolveEventArgs args)
        {
            try
            {
                string folderPath = FabricEnvironment.GetCodePath();
                string assemblyPath = Path.Combine(folderPath, new AssemblyName(args.Name).Name + ".dll");
                if (File.Exists(assemblyPath))
                {
                    return Assembly.LoadFrom(assemblyPath);
                }
            }
            catch (Exception)
            {
                // Supress any Exception so that we can continue to
                // load the assembly through other means
            }

            return null;
        }

        private async Task UpdatePersistedCodeUpgradePackage(StandaloneSettingsValidator validator)
        {
            bool codeVersionChanged = await validator.IsCodeVersionChangedAsync(validator.ClusterProperties.CodeVersion).ConfigureAwait(false);
            if (!codeVersionChanged || validator.ClusterProperties.CodeVersion == DMConstants.AutoupgradeCodeVersion)
            {
                return;
            }

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Found a different version, updating persistence state with version: {0}", validator.ClusterProperties.CodeVersion);

            CodeUpgradeDetail packageDetails = new CodeUpgradeDetail()
            {
                CodeVersion = validator.ClusterProperties.CodeVersion,
                IsUserInitiated = true
            };

            await this.storeManager.PersistCodeUpgradePackageDetailsAsync(Constants.CodeUpgradePackageDetails, packageDetails, this.cancellationToken);
        }
    }
}
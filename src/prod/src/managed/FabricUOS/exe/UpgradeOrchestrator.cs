// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeOrchestration.Service
{
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.FabricDeployer;
    using System.Fabric.Health;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.Fabric.UpgradeService;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager.Common;

    internal sealed class UpgradeOrchestrator
    {
        internal const string TraceType = "UpgradeOrchestration.Service";

        private static readonly FabricErrorCode[] AllowedStartFabricUpgradeErrorCodes =
        {
            FabricErrorCode.FabricUpgradeInProgress,
            FabricErrorCode.FabricAlreadyInTargetVersion,
        };

        private StoreManager storeManager;
        private FabricUpgradeOrchestrationService uosSvc;
        private FabricClient fabricClient;

        public UpgradeOrchestrator(StoreManager storeManager, FabricUpgradeOrchestrationService uosSvc, FabricClient fabricClient)
        {
            this.storeManager = storeManager;
            this.uosSvc = uosSvc;
            this.fabricClient = fabricClient;
        }

        public Task StartUpgradeAsync(
            StandAloneCluster cluster,
            CancellationToken cancellationToken,
            ConfigurationUpgradeDescription configUpgradeDesc)
        {
            try
            {
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "StartUpgradeAsync: Entering.");

                if (cluster == null)
                {
                    throw new ArgumentNullException("cluster");
                }

                Task result = Task.Run(
                    async delegate
                    {
                        await this.OrchestrateUpgradeAsync(cancellationToken, configUpgradeDesc);
                    });

                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "StartUpgradeAsync: Exiting.");
                return result;
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "StartUpgradeAsync exception: {0}", e);
                throw;
            }
        }

        internal async Task OrchestrateUpgradeAsync(
            CancellationToken cancellationToken,
            ConfigurationUpgradeDescription configUpgradeDesc)
        {
            try
            {
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "OrchestrateUpgradeAsync: Entering.");

                while (!cancellationToken.IsCancellationRequested)
                {
                    StandAloneCluster cluster = await this.storeManager.GetClusterResourceAsync(Constants.ClusterReliableDictionaryKey, cancellationToken)
                                                                    .ConfigureAwait(false);

                    if (cluster == null)
                    {
                        throw new InvalidOperationException("OrchestrateUpgradeAsync cannot find persisted cluster resource from StoreManager.");
                    }

                    string currentClusterState = "<null>";
                    if (cluster.Current != null && cluster.Current.ExternalState != null)
                    {
                        currentClusterState = string.Format(
                            CultureInfo.InvariantCulture,
                            "Code version: {0} Config version: {1}",
                            cluster.Current.ExternalState.MsiVersion,
                            cluster.Current.ExternalState.ClusterManifest == null ? "<null>" : cluster.Current.ExternalState.ClusterManifest.Version);
                    }

                    CodeUpgradeDetail packageDetails = await this.storeManager.GetCodeUpgradePackageDetailsAsync(Constants.CodeUpgradePackageDetails, cancellationToken);
                    if (packageDetails != null && !string.IsNullOrEmpty(packageDetails.CodeVersion) && ((cluster.TargetWrpConfig == null) || (cluster.TargetWrpConfig.Version.MsiVersion != packageDetails.CodeVersion)))
                    {
                        string packageLocalDownloadPath = null;
                        string configurationsFilePath = null;

                        try
                        {
                            packageLocalDownloadPath = await this.DownloadAndProvisionPackageAsync(packageDetails.CodeVersion, cancellationToken).ConfigureAwait(false);
                            configurationsFilePath = UpgradeOrchestrator.ExtractConfigurations(packageLocalDownloadPath);

                            this.RemoveLocalCodePackage(packageLocalDownloadPath);
                        }
                        catch (Exception ex)
                        {
                            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Failed to download or extract CAB, Exception: {0}", ex.ToString());
                            await this.OnCodePackageUpgradeFailed(ex.HResult, StringResources.Error_SFCodePackagesUnreachable, cancellationToken);

                            break;
                        }

                        var targetWrpConfig = new StandaloneAdminConfig(configurationsFilePath, packageDetails.IsUserInitiated);
                        targetWrpConfig.Version.MsiVersion = packageDetails.CodeVersion;
                        cluster.TargetWrpConfig = targetWrpConfig;

                        await this.storeManager.PersistClusterResourceAsync(Constants.ClusterReliableDictionaryKey, cluster, cancellationToken).ConfigureAwait(false);
                    }

                    cluster = await this.storeManager.GetClusterResourceAsync(Constants.ClusterReliableDictionaryKey, cancellationToken).ConfigureAwait(false);
                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Running state machine for given cluster resource. Current cluster state: {0}", currentClusterState);

                    bool updated = false;
                    try
                    {
                        updated = cluster.RunStateMachine();
                    }
                    catch (Exception ex)
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Failed to run the state machine, Exception: {0}", ex.ToString());
                        await this.OnCodePackageUpgradeFailed(ex.HResult, string.Format(StringResources.Error_SFConfigUpgradeFailWithException, ex.ToString()), cancellationToken);

                        break;
                    }

                    if (updated == true)
                    {
                        // cluster resource state changed (maybe due to a pending upgrade or maybe something else)
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "State machine returns true (a state change). Persisting the cluster resource.");

                        await this.storeManager.PersistClusterResourceAsync(Constants.ClusterReliableDictionaryKey, cluster, cancellationToken)
                                          .ConfigureAwait(false);

                        cluster = await this.storeManager.GetClusterResourceAsync(Constants.ClusterReliableDictionaryKey, cancellationToken)
                                                      .ConfigureAwait(false);
                    }

                    IEnumerable<NodeStatus> disablingNodes = UpgradeOrchestrator.GetDisablingNodes(cluster);
                    if (cluster.Pending == null && !disablingNodes.Any())
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "State machine reports no pending upgrade.");
                        break;
                    }

                    bool isDeactivationSuccessful = false;
                    try
                    {
                        isDeactivationSuccessful = await this.PerformDisablingNodeOperationsAsync(cluster, cancellationToken).ConfigureAwait(false);
                    }
                    catch (OperationCanceledException ex)
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "Cancellation token was requested. Exception thrown : {0}", ex.ToString());
                        break;
                    }

                    if (!isDeactivationSuccessful)
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "Remove node operations were not performed succesfully. Aborting upgrade.");

                        // Since deactivation fails, reset cluster so that a new upgrade can be initiated by the user again.
                        cluster.Pending = null;
                        cluster.Reset();
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Persist cluster resource after failed deactivation of nodes.");
                        await this.storeManager.PersistClusterResourceAsync(Constants.ClusterReliableDictionaryKey, cluster, cancellationToken)
                                                  .ConfigureAwait(false);
                        break;
                    }

                    if (cluster.Pending == null && disablingNodes.Any())
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "State machine reports no pending upgrade but there are disabling nodes. Running State machine for Remove operation.");
                        bool result = await this.SetTargetNodeConfigForRemovedNodesAsync(
                            disablingNodes, 
                            cluster.Current.NodeConfig.Version, 
                            cluster,
                            configUpgradeDesc,
                            cancellationToken).ConfigureAwait(false);
                        if (result)
                        {
                            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Running state machine again");
                            ReleaseAssert.AssertIfNot(cluster.RunStateMachine(), "RunStateMachine returns false for disabling->remove upgrade. Investigate why RunStateMachine fails to find a new upgrade.");
                            await this.storeManager.PersistClusterResourceAsync(Constants.ClusterReliableDictionaryKey, cluster, cancellationToken);
                        }
                    }

                    bool unhealthyReported = this.ReportUnhealthyIfNecessary(cluster);

                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "State machine reports a pending upgrade. Pending code version: {0}, config version: {1}", cluster.Pending.ExternalState.MsiVersion, cluster.Pending.ExternalState.ClusterManifest.Version);

                    FabricUpgradeProgress upgradeProgress = await UpgradeOrchestrator.DoUpgradeAsync(cluster, cancellationToken, this.fabricClient, this.storeManager, configUpgradeDesc);

                    if (unhealthyReported)
                    {
                        this.ReportHealth(isHealthy: true);
                        unhealthyReported = false;
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Ok health is reported after processing test injection error.");
                    }
                   
                    if (upgradeProgress.UpgradeState == FabricUpgradeState.RollingForwardCompleted)
                    {
                        long nodeConfigVersion = 0;
                        if (cluster.TargetNodeConfig != null)
                        {
                            disablingNodes = UpgradeOrchestrator.GetDisablingNodes(cluster);
                            nodeConfigVersion = cluster.TargetNodeConfig.Version;
                        }

                        cluster.ClusterUpgradeCompleted();
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "OrchestrateUpgradeAsync reports upgrade complete.");

                        if (cluster.Pending != null && cluster.Pending.GetType() == typeof(StandAloneAutoScaleClusterUpgradeState))
                        {
                            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Autoscale upgrade detected. Starting to update system service");

                            StandAloneAutoScaleClusterUpgradeState autoScaleUpgradeState = (StandAloneAutoScaleClusterUpgradeState)cluster.Pending;
                            Dictionary<string, ReplicaSetSize> targetReplicaSetSizes = autoScaleUpgradeState.TargetSystemServicesSize;
                            if (targetReplicaSetSizes != null && targetReplicaSetSizes.Any())
                            {
                                await this.UpdateSystemServicesAsync(targetReplicaSetSizes, Constants.UpgradeServiceMaxOperationTimeout, cancellationToken).ConfigureAwait(false);
                                autoScaleUpgradeState.TargetSystemServicesSize = new Dictionary<string, ReplicaSetSize>();
                                cluster.ClusterUpgradeCompleted();
                            }
                        }

                        if (disablingNodes.Any() && cluster.Pending == null)
                        {
                            bool result = await this.SetTargetNodeConfigForRemovedNodesAsync(
                                disablingNodes,
                                nodeConfigVersion,
                                cluster,
                                configUpgradeDesc,
                                cancellationToken).ConfigureAwait(false);
                            if (result)
                            {
                                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Running state machine again");
                                cluster.RunStateMachine();
                            }
                        }

                        IEnumerable<NodeStatus> removedNodes = GetRemovedNodes(cluster);
                        if (removedNodes.Any())
                        {
                            await this.RemoveNodeStateAsync(removedNodes, cancellationToken).ConfigureAwait(false);
                        }
                    }
                    else
                    {
                        var failureReason = upgradeProgress.FailureReason == null
                            ? ClusterUpgradeFailureReason.Unknown
                            : (ClusterUpgradeFailureReason)upgradeProgress.FailureReason;

                        cluster.ClusterUpgradeRolledBackOrFailed(upgradeProgress.FailureTimestampUtc.ToString(), failureReason);
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "OrchestrateUpgradeAsync reports upgrade failure or rollback.");
                    }

                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Persist cluster resource after upgrade.");

                    if (cluster.Pending == null)
                    {
                        cluster.Reset();
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "State machine reports no pending upgrade. Status is reset.");

                        await this.storeManager.PersistConfigUpgradeErrorDetailsAsync(Constants.ConfigUpgradeErrorDetails, null, cancellationToken).ConfigureAwait(false);

                        packageDetails = await this.storeManager.GetCodeUpgradePackageDetailsAsync(Constants.CodeUpgradePackageDetails, cancellationToken);
                        if (packageDetails != null && !string.IsNullOrEmpty(packageDetails.CodeVersion))
                        {
                            await this.PerformCodeUpgradeCleanupTasksAsync(packageDetails, cancellationToken).ConfigureAwait(false);
                        }
                    }

                    await this.storeManager.PersistClusterResourceAsync(Constants.ClusterReliableDictionaryKey, cluster, cancellationToken);

                    if (cluster.Pending == null)
                    {
                        break;
                    }
                }

                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exit OrchestrateUpgradeAsync");
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "OrchestrateUpgradeAsync exception: {0}", e);
                throw;
            }
        }

        internal bool ReportUnhealthyIfNecessary(StandAloneCluster cluster, Action<bool> reportHealthDelegate = null)
        {
            if (cluster.TargetCsmConfig == null)
            {
                return false;
            }

            FaultInjectionConfig faultInjectionConfig = FaultInjectionHelper.TryGetFaultInjectionConfig(cluster.TargetCsmConfig.FabricSettings);
            if (faultInjectionConfig != null)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Test injection is configured: {0}", faultInjectionConfig);

                UpgradeFlow currentUpgradeFlow = FaultInjectionHelper.GetUpgradeFlow(cluster.Pending);
                if (FaultInjectionHelper.ShouldReportUnhealthy(cluster.Pending, currentUpgradeFlow, faultInjectionConfig))
                {
                    this.ReportHealth(isHealthy: false, reportHealthDelegate: reportHealthDelegate);
                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Test injection error health is reported. Config: {0}", faultInjectionConfig);
                    return true;
                }
            }

            return false;
        }

        internal void ReportHealth(bool isHealthy, Action<bool> reportHealthDelegate = null)
        {
            if (reportHealthDelegate != null)
            {
                reportHealthDelegate(isHealthy);
            }
            else
            {
                this.uosSvc.GetPartition().ReportPartitionHealth(
                    new HealthInformation(
                        "TestInjection",
                        "DummyProperty",
                        isHealthy ? HealthState.Ok : HealthState.Error));
            }
        }

        internal async Task DeactivateNodesToBeRemovedAsync(List<NodeStatus> removedNodes, CancellationToken cancellationToken)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Nodes have been removed from json config. Deactivating nodes begins..");
            foreach (var removedNode in removedNodes)
            {                
                var nodedeactivate = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                      () =>
                                      this.fabricClient.QueryManager.GetNodeListAsync(
                                          removedNode.NodeName,
                                          Constants.FabricQueryTimeoutInMinutes,
                                          cancellationToken),
                                      Constants.FabricQueryRetryTimeoutInMinutes).ConfigureAwait(false);
                var node = nodedeactivate.FirstOrDefault();
                if (IsDisablingNodeValid(node))
                {
                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Disabling node {0}", removedNode.NodeName);
                    await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => this.fabricClient.ClusterManager.DeactivateNodeAsync(
                            node.NodeName,
                            NodeDeactivationIntent.RemoveNode,
                            Constants.FabricQueryTimeoutInMinutes,
                            cancellationToken),
                        Constants.FabricQueryRetryTimeoutInMinutes).ConfigureAwait(false);
                }
            }
        }

        internal async Task PollDeactivatedNodesAsync(IEnumerable<NodeStatus> removedNodes, CancellationToken cancellationToken)
        {
            System.Fabric.Common.TimeoutHelper timeoutHelper = new System.Fabric.Common.TimeoutHelper(Constants.FabricPollDeactivatedNodesTimeoutInMinutes);
            bool isDeactivationComplete = true;
            while (!System.Fabric.Common.TimeoutHelper.HasExpired(timeoutHelper) && !cancellationToken.IsCancellationRequested)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Checking deactivation status of nodes to be removed.");
                try
                {
                    System.Fabric.Query.NodeList nodes = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                      () =>
                                      this.fabricClient.QueryManager.GetNodeListAsync(
                                          null,
                                          Constants.FabricQueryTimeoutInMinutes,
                                          cancellationToken),
                                      Constants.FabricQueryRetryTimeoutInMinutes).ConfigureAwait(false);
                    isDeactivationComplete = true;
                    for (int i = 0; i < nodes.Count; ++i)
                    {
                        if (removedNodes.Any(removedNode => removedNode.NodeName == nodes[i].NodeName)
                            && (nodes[i].NodeStatus != System.Fabric.Query.NodeStatus.Disabled
                                 && nodes[i].NodeStatus != System.Fabric.Query.NodeStatus.Invalid))
                        {
                            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Node {0} has not been disabled", nodes[i].IpAddressOrFQDN);
                            isDeactivationComplete = false;
                            break;
                        }
                    }

                    if (isDeactivationComplete)
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "All nodes to be removed have been disabled. Continuing with upgrade");
                        break;
                    }
                    else
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Waiting for five seconds before polling deactivation status again.");
                    }

                    await Task.Delay(TimeSpan.FromSeconds(5), cancellationToken).ConfigureAwait(false);
                }
                catch (FabricTransientException fte)
                {
                    UpgradeOrchestrationTrace.TraceSource.WriteError(TraceType, "Retrying Polling Deactivated Node because of retryable exception {0}", fte);
                }
            }

            timeoutHelper.ThrowIfExpired();
        }

        private static string ExtractConfigurations(string packageLocalDownloadPath)
        {
            string tempPath = Helpers.GetNewTempPath();
            string fileToExtract = Path.Combine(Constants.PathToFabricCode, Constants.Configurations);
            CabFileOperations.ExtractFiltered(packageLocalDownloadPath, tempPath, new string[] { fileToExtract }, true);
            string tempFile = Path.Combine(tempPath, fileToExtract);
            return Path.Combine(tempPath, fileToExtract);
        }

        private static async Task<T> ExecuteFabricActionWithRetryAsyncHelper<T>(Func<Task<T>> function, CancellationToken cancellationToken)
        {
            try
            {
                return await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                        function,
                                        Constants.UpgradeServiceMaxOperationTimeout,
                                        cancellationToken).ConfigureAwait(false);
            }
            catch (Exception ex)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "ExecuteFabricActionWithRetryAsyncHelper error: {0}", ex);
                return default(T);
            }
        }

        private static async Task<FabricUpgradeProgress> DoUpgradeAsync(
            ICluster cluster,
            CancellationToken cancellationToken,
            FabricClient fabricClient,
            StoreManager storeManager,
            ConfigurationUpgradeDescription configUpgradeDesc)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Enter DoUpgradeAsync.");
            string targetCodeVersion = null;
            string cabPath = null;

            // First check if this is a base line upgrade, if so use the targetcodeversion from the data root cab file.
            FabricUpgradeProgress currentClusterStatus = await UpgradeOrchestrator.GetCurrentClusterStatusAsync(cancellationToken).ConfigureAwait(false);
            string currentCodeVersion = currentClusterStatus.TargetCodeVersion;
            if (string.Equals(currentCodeVersion, Constants.ClusterInitializationCodeVersion, StringComparison.OrdinalIgnoreCase))
            {
                // for baseline upgrade, we get cab from fabric data root
                cabPath = Path.Combine(FabricEnvironment.GetDataRoot(), System.Fabric.FabricDeployer.Constants.FileNames.BaselineCab); // TODO: Needs to come from image store
                targetCodeVersion = CabFileOperations.GetCabVersion(cabPath);
            }

            // If we don't have target code version yet (not a baseline upgrade), see if the cluster specified the target code version and use that if its valid.
            Version version;
            if (string.IsNullOrEmpty(targetCodeVersion) &&
                !string.IsNullOrEmpty(cluster.Pending.ExternalState.MsiVersion) &&
                Version.TryParse(cluster.Pending.ExternalState.MsiVersion, out version))
            {
                targetCodeVersion = cluster.Pending.ExternalState.MsiVersion;
            }

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(
                TraceType,
                "DoUpgradeAsync: Current cluster status: code version {0}, config version {1}, upgrade state {2}. Target code version: {3}",
                currentCodeVersion,
                currentClusterStatus.TargetConfigVersion,
                currentClusterStatus.UpgradeState,
                targetCodeVersion);

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Generating cluster manifest in temp directory.");
            var info = Directory.CreateDirectory(Path.Combine(Path.GetTempPath(), Path.GetRandomFileName()));
            var tempPath = info.FullName;
            string clusterManifestPath = Path.Combine(tempPath, Constants.ClusterManifestTempFileName);
            XMLHelper.WriteXmlExclusive<ClusterManifestType>(clusterManifestPath, cluster.Pending.ExternalState.ClusterManifest);
            
            var upgradeDescription = UpgradeOrchestrator.ConstructUpgradeDescription(configUpgradeDesc, cluster.Pending);
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "ForceRestart: {0}. configUpgradeDesc: {1}", upgradeDescription.ForceRestart, configUpgradeDesc.ToStringDescription());
            var commandParameter = new ClusterUpgradeCommandParameter
            {
                ConfigFilePath = clusterManifestPath,
                CodeFilePath = cabPath,
                ConfigVersion = cluster.Pending.ExternalState.ClusterManifest.Version,
                CodeVersion = targetCodeVersion,
                UpgradeDescription = upgradeDescription
            };

            FabricClientWrapper commandProcessor = new FabricClientWrapper(fabricClient);

            bool upgradeInProgress = false;
            FabricUpgradeProgress upgradeProgress = null;
            string expectedTargetVersion = string.Empty;

            while (true)
            {
                if (!upgradeInProgress)
                {
                    try
                    {
                        upgradeProgress = await ExecuteFabricActionWithRetryAsyncHelper(
                            () =>
                                commandProcessor.GetFabricUpgradeProgressAsync(
                                Constants.FabricQueryTimeoutInMinutes,
                                cancellationToken),
                            cancellationToken).ConfigureAwait(false);
                        if (upgradeProgress == null)
                        {
                            continue;
                        }

                        if (UpgradeOrchestrator.NoUpgradeInProgress(upgradeProgress.UpgradeState))
                        {
                            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Invoking FabricUS ClusterUpgradeAsync API to start the upgrade.");
                            await commandProcessor.ClusterUpgradeAsync(commandParameter, TimeSpan.MaxValue, cancellationToken).ConfigureAwait(false);
                        }
                        else
                        {
                            // This can happen if reconfiguration occurs while an upgrade is in process. When new replica comes up, we should not trigger another upgrade from UOS.
                            if (cluster.Pending.ExternalState.ClusterManifest.Version.Equals(upgradeProgress.TargetConfigVersion))
                            {
                                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Upgrade called with same version {0}. Not initiating another CM upgrade.", cluster.Pending.ExternalState.ClusterManifest.Version);
                            }
                            else if (upgradeProgress.UpgradeState != FabricUpgradeState.RollingBackInProgress)
                            {
                                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Interrupting on-going upgrade with new version {0}, current Upgrade state: {1}", cluster.Pending.ExternalState.ClusterManifest.Version, upgradeProgress.UpgradeState);
                                await commandProcessor.ClusterUpgradeAsync(commandParameter, TimeSpan.MaxValue, cancellationToken).ConfigureAwait(false);
                            }
                        }
                    }
                    catch (FabricException fe)
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "ClusterUpgradeAsync fabric exception: {0} {1}", fe.ErrorCode, fe);

                        if (!AllowedStartFabricUpgradeErrorCodes.Contains(fe.ErrorCode))
                        {
                            throw;
                        }
                    }

                    upgradeInProgress = true;
                }

                upgradeProgress = await ExecuteFabricActionWithRetryAsyncHelper(
                    () =>
                        commandProcessor.GetFabricUpgradeProgressAsync(
                        Constants.FabricQueryTimeoutInMinutes,
                        cancellationToken),
                        cancellationToken).ConfigureAwait(false);
                if (upgradeProgress == null)
                {
                    continue;
                }

                var upgradeState = upgradeProgress.UpgradeState;

                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Upgrade State: {0}, target code version: {1}, target config version: {2}", upgradeState, upgradeProgress.TargetCodeVersion, upgradeProgress.TargetConfigVersion);
                if (NoUpgradeInProgress(upgradeState))
                {
                    // Persisted cluster resource should be retrieved in each iteration because if an upgrade is interrupted, this value may change.
                    cluster = await storeManager.GetClusterResourceAsync(
                                                       Constants.ClusterReliableDictionaryKey, cancellationToken).ConfigureAwait(false);

                    /* Check if an upgrade is pending before retrieving version. When an upgrade is interrupted and the new upgrade finishes, cluster resource is reset.
                     * We must keep using the last expectedTargetVersion value to gracefully exit out of this thread. */
                    if (cluster.Pending != null)
                    {
                        expectedTargetVersion = cluster.Pending.ExternalState.ClusterManifest.Version;
                    }

                    if (upgradeState == FabricUpgradeState.RollingForwardCompleted && upgradeProgress.TargetConfigVersion != expectedTargetVersion)
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(
                            TraceType,
                            "Upgrade finishes with state {0}, but needs to retry. Target config version: {1}. Current config version: {2}. Pending config version: {3}. Retrying.",
                            upgradeState,
                            upgradeProgress.TargetConfigVersion,
                            cluster.Current.ExternalState.ClusterManifest.Version,
                            expectedTargetVersion);
                        await Task.Delay(TimeSpan.FromSeconds(10), cancellationToken).ConfigureAwait(false);
                    }
                    else
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Upgrade finishes with state {0}", upgradeState);
                        break;
                    }
                }
                else if (upgradeState == FabricUpgradeState.RollingBackInProgress ||
                         upgradeState == FabricUpgradeState.RollingForwardPending ||
                         upgradeState == FabricUpgradeState.RollingForwardInProgress)
                {
                    // still in the process of upgrading, sleep and retry
                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Upgrade is still in {0} state. Keep checking state.", upgradeState);
                    await Task.Delay(TimeSpan.FromSeconds(10), cancellationToken).ConfigureAwait(false);
                }
                else
                {
                    // failed for some reason, caller need to retry
                    break;
                }
            }

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exit DoUpgradeAsync");

            return upgradeProgress;
        }

        private static CommandProcessorClusterUpgradeDescription ConstructUpgradeDescription(ConfigurationUpgradeDescription configUpgradeDesc, ClusterUpgradeStateBase pendingUpgrade)
        {
            CommandProcessorClusterUpgradeDescription result = new CommandProcessorClusterUpgradeDescription();
            result.ForceRestart = pendingUpgrade.GetUpgradePolicy().ForceRestart;
            
            result.HealthCheckRetryTimeout = configUpgradeDesc.HealthCheckRetryTimeout;
            result.UpgradeDomainTimeout = configUpgradeDesc.UpgradeDomainTimeout;
            result.UpgradeTimeout = configUpgradeDesc.UpgradeTimeout;

            result.DeltaHealthPolicy = new CommandProcessorClusterUpgradeDeltaHealthPolicy()
            {
                MaxPercentDeltaUnhealthyNodes = configUpgradeDesc.MaxPercentDeltaUnhealthyNodes,
                MaxPercentUpgradeDomainDeltaUnhealthyNodes = configUpgradeDesc.MaxPercentUpgradeDomainDeltaUnhealthyNodes
            };

            var applicationHealthPolicies = new Dictionary<string, CommandProcessorApplicationHealthPolicy>();
            if (configUpgradeDesc.ApplicationHealthPolicies != null)
            {
                foreach (var applicationHealthPolicy in configUpgradeDesc.ApplicationHealthPolicies)
                {
                    var healthPolicy = applicationHealthPolicy.Value;
                    var commandProcessorApplicationHealthPolicy = UpgradeOrchestrator.ToCommandProcessorServiceTypeHealthPolicy(healthPolicy);

                    applicationHealthPolicies.Add(applicationHealthPolicy.Key.ToString(), commandProcessorApplicationHealthPolicy);
                }
            }

            result.HealthPolicy = new CommandProcessorClusterUpgradeHealthPolicy()
            {
                MaxPercentUnhealthyApplications = configUpgradeDesc.MaxPercentUnhealthyApplications,
                MaxPercentUnhealthyNodes = configUpgradeDesc.MaxPercentUnhealthyNodes,
                ApplicationHealthPolicies = applicationHealthPolicies
            };
            
            // todo: only set zero if seed node upgrade is involved.
            result.HealthCheckStableDuration = TimeSpan.Zero;
            result.HealthCheckWaitDuration = TimeSpan.Zero;

            return result;
        }

        private static CommandProcessorApplicationHealthPolicy ToCommandProcessorServiceTypeHealthPolicy(ApplicationHealthPolicy applicationHealthPolicy)
        {
            if (applicationHealthPolicy == null)
            {
                return null;
            }

            var result = new CommandProcessorApplicationHealthPolicy();
            if (applicationHealthPolicy.DefaultServiceTypeHealthPolicy != null)
            {
                result.DefaultServiceTypeHealthPolicy = new CommandProcessorServiceTypeHealthPolicy()
                {
                    MaxPercentUnhealthyServices = applicationHealthPolicy.DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyServices
                };
            }

            if (applicationHealthPolicy.ServiceTypeHealthPolicyMap != null)
            {
                result.SerivceTypeHealthPolicies = applicationHealthPolicy.ServiceTypeHealthPolicyMap.ToDictionary(
                    keyValuePair => keyValuePair.Key,
                    keyValuePair => new CommandProcessorServiceTypeHealthPolicy()
                    {
                        MaxPercentUnhealthyServices = keyValuePair.Value.MaxPercentUnhealthyServices
                    });
            }

            return result;
        }

        private static async Task<FabricUpgradeProgress> GetCurrentClusterStatusAsync(CancellationToken cancellationToken)
        {
            return await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                () => new FabricClientWrapper().GetFabricUpgradeProgressAsync(
                Constants.UpgradeServiceMaxOperationTimeout,
                cancellationToken),
                Constants.UpgradeServiceMaxOperationTimeout,
                cancellationToken).ConfigureAwait(false);
        }

        private static bool NoUpgradeInProgress(FabricUpgradeState upgradeState)
        {
            return upgradeState == FabricUpgradeState.RollingBackCompleted ||
                   upgradeState == FabricUpgradeState.RollingForwardCompleted ||
                   upgradeState == FabricUpgradeState.Failed;
        }

        private static bool IsDisablingNodeValid(System.Fabric.Query.Node node)
        {
            return node.NodeStatus != System.Fabric.Query.NodeStatus.Disabled
                    && node.NodeStatus != System.Fabric.Query.NodeStatus.Disabling
                    && node.NodeStatus != System.Fabric.Query.NodeStatus.Invalid;
        }

        private static byte GetMaxPercentUpgradeDomainDeltaUnhealthyNodes(IClusterTopology topology, IEnumerable<NodeStatus> disablingNodes)
        {
            Dictionary<string, int> upgradeDomainToNodeCountMap = new Dictionary<string, int>();
            Dictionary<string, int> upgradeDomainToDiasablingNodeCountMap = new Dictionary<string, int>();
            foreach (var node in topology.Nodes)
            {
                IncrementEntryInMap(upgradeDomainToNodeCountMap, node.Value.UpgradeDomain);
                if (disablingNodes.Any(disablingNode => disablingNode.NodeName == node.Key))
                {
                    IncrementEntryInMap(upgradeDomainToDiasablingNodeCountMap, node.Value.UpgradeDomain);
                }
            }

            double maxPercentUDUnhealthyNodes = 0;
            foreach (var upgradeDomain in upgradeDomainToDiasablingNodeCountMap.Keys)
            {
                maxPercentUDUnhealthyNodes = Math.Max(maxPercentUDUnhealthyNodes, (double)(upgradeDomainToDiasablingNodeCountMap[upgradeDomain] * 100) / upgradeDomainToNodeCountMap[upgradeDomain]);
            }

            return Convert.ToByte(maxPercentUDUnhealthyNodes);
        }

        private static IEnumerable<NodeStatus> GetRemovedNodes(StandAloneCluster cluster)
        {
            IEnumerable<NodeStatus> removedNodes = new List<NodeStatus>();
            removedNodes = cluster.Current.NodeConfig.NodesStatus.Where(node => node.NodeState == NodeState.Removed);
            return removedNodes;
        }

        private static IEnumerable<NodeStatus> GetDisablingNodes(StandAloneCluster cluster)
        {
            IEnumerable<NodeStatus> disablingNodes = new List<NodeStatus>();
            if (cluster.TargetNodeConfig != null)
            {
                disablingNodes = cluster.TargetNodeConfig.NodesStatus.Where(node => node.NodeState == NodeState.Disabling);
            }
            else
            {
                disablingNodes = cluster.Current.NodeConfig.NodesStatus.Where(node => node.NodeState == NodeState.Disabling);
            }

            return disablingNodes;
        }

        private static void IncrementEntryInMap(Dictionary<string, int> map, string key)
        {
            if (map.ContainsKey(key))
            {
                map[key]++;
            }
            else
            {
                map.Add(key, 1);
            }
        }

        private async Task OnCodePackageUpgradeFailed(int errorCode, string errorMessage, CancellationToken cancellationToken)
        {
            var cluster = await this.storeManager.GetClusterResourceAsync(Constants.ClusterReliableDictionaryKey, cancellationToken).ConfigureAwait(false);
            cluster.Pending = null;
            cluster.Reset();
            await this.storeManager.PersistClusterResourceAsync(Constants.ClusterReliableDictionaryKey, cluster, cancellationToken).ConfigureAwait(false);

            var packageDetails = await this.storeManager.GetCodeUpgradePackageDetailsAsync(Constants.CodeUpgradePackageDetails, cancellationToken);
            packageDetails.CodeVersion = null;
            await this.storeManager.PersistCodeUpgradePackageDetailsAsync(Constants.CodeUpgradePackageDetails, packageDetails, cancellationToken).ConfigureAwait(false);

            ConfigUpgradeErrorDetail errorDetail = new ConfigUpgradeErrorDetail()
            {
                ErrorCode = errorCode,
                ErrorMessage = errorMessage,
            };
            await this.storeManager.PersistConfigUpgradeErrorDetailsAsync(Constants.ConfigUpgradeErrorDetails, errorDetail, cancellationToken).ConfigureAwait(false);
        }

        private async Task PerformCodeUpgradeCleanupTasksAsync(CodeUpgradeDetail packageDetails, CancellationToken cancellationToken)
        {
            await this.UnprovisionOldPackagesAsync(packageDetails.CodeVersion, cancellationToken).ConfigureAwait(false);

            packageDetails.CodeVersion = null;
            await this.storeManager.PersistCodeUpgradePackageDetailsAsync(Constants.CodeUpgradePackageDetails, packageDetails, cancellationToken).ConfigureAwait(false);
        }

        private async Task<bool> IsPackageProvisionedAsync(string codeVersion, CancellationToken cancellationToken)
        {
            var result = false;
            var provisionedCodeList = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => new FabricClient().QueryManager.GetProvisionedFabricCodeVersionListAsync(codeVersion),
                        Constants.FabricQueryTimeoutInMinutes,
                        cancellationToken).ConfigureAwait(false);

            if (provisionedCodeList.Count != 0)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Code Version {0} is already provisioned.", codeVersion);
                result = true;
            }

            return result;
        }

        private async Task<string> DownloadAndProvisionPackageAsync(string codeVersion, CancellationToken cancellationToken)
        {
            var isPackageProvisioned = await this.IsPackageProvisionedAsync(codeVersion, cancellationToken).ConfigureAwait(false);
            string packageLocalDownloadPath = null;
            if (!isPackageProvisioned)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Starting download");
                packageLocalDownloadPath = await StandaloneUtility.DownloadCodeVersionAsync(codeVersion).ConfigureAwait(false);
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Package was succesfully downloaded to {0}", packageLocalDownloadPath);

                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Copying and provisioning version {0} from {1}.", codeVersion, packageLocalDownloadPath);
                await this.ProvisionPackageAsync(codeVersion, packageLocalDownloadPath, cancellationToken).ConfigureAwait(false);
            }
            else
            {
                string packageName = string.Format("WindowsFabric.{0}.cab", codeVersion.Trim());
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Searching for package {0} in the native image store", packageName);

                NativeImageStoreClient imageStore = new NativeImageStoreClient(null, false, System.IO.Path.GetTempPath(), null);
                var content = imageStore.ListContentWithDetails(Constants.WindowsFabricStoreName, false, Constants.FabricQueryTimeoutInMinutes);
                var sourcePackagePath = content.Files.SingleOrDefault(file => file.StoreRelativePath.Contains(packageName)).StoreRelativePath;

                packageLocalDownloadPath = Path.Combine(Path.GetTempPath(), packageName);
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Downloading cab file package from {0} to {1}", sourcePackagePath, packageLocalDownloadPath);
                await imageStore.DownloadContentAsync(sourcePackagePath, packageLocalDownloadPath, Constants.UpgradeServiceMaxOperationTimeout, CopyFlag.AtomicCopy).ConfigureAwait(false);
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Package was succesfully downloaded to {0}", packageLocalDownloadPath);
            }

            return packageLocalDownloadPath;
        }

        private async Task UnprovisionOldPackagesAsync(string codeVersion, CancellationToken cancellationToken)
        {
            try
            {
                var provisionedCodeList = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                        () => new FabricClient().QueryManager.GetProvisionedFabricCodeVersionListAsync(),
                                        Constants.FabricQueryTimeoutInMinutes,
                                        cancellationToken).ConfigureAwait(false);
                foreach (var provisionedVersion in provisionedCodeList)
                {
                    if (string.Compare(provisionedVersion.CodeVersion.Trim(), codeVersion.Trim()) != 0)
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Unprovisioning package {0}", provisionedVersion.CodeVersion);

                        await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                            () => new FabricClient().ClusterManager.UnprovisionFabricAsync(provisionedVersion.CodeVersion, null),
                            Constants.FabricQueryTimeoutInMinutes,
                            cancellationToken).ConfigureAwait(false);
                    }
                }
            }
            catch (Exception ex)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "Failed to unprovision old code packages, Exception: {0}", ex.ToString());
            }
        }

        private void RemoveLocalCodePackage(string packageLocalDownloadPath)
        {
            if (!string.IsNullOrEmpty(packageLocalDownloadPath) && File.Exists(packageLocalDownloadPath))
            {
                try
                {
                    // Best-effort
                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Deleting {0}", packageLocalDownloadPath);
                    File.Delete(packageLocalDownloadPath);
                }
                catch (Exception ex)
                {
                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(
                        TraceType,
                        "Failed to delete {0}, Exception: {1}",
                        packageLocalDownloadPath,
                        ex.ToString());
                }
            }
        }

        private async Task<bool> PerformDisablingNodeOperationsAsync(StandAloneCluster cluster, CancellationToken cancellationToken)
        {
            var disablingNodes = UpgradeOrchestrator.GetDisablingNodes(cluster).ToList();
            bool operationsPerformedSuccesfully = true;
            if (disablingNodes.Any())
            {
                try
                {
                    await this.DeactivateNodesToBeRemovedAsync(disablingNodes, cancellationToken).ConfigureAwait(false);
                }
                catch (OperationCanceledException ex)
                {
                    /* This needs to be checked for scenarios where UOS primary lies on the node to be removed. When the node is disabled,
                       reconfiguration is initiated and the cancellationToken is cancelled. The new primary should then continue the upgrade. */
                    UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "Cancellation token was requested. Exception thrown : {0}", ex.ToString());
                    throw;
                }
                catch (Exception ex)
                {
                    UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "Failed to deactivate nodes. Exception thrown : {0}", ex.ToString());
                    operationsPerformedSuccesfully = false;
                }
            }

            return operationsPerformedSuccesfully;
        }

        private async Task RemoveNodeStateAsync(IEnumerable<NodeStatus> removedNodes, CancellationToken cancellationToken)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Invoking RemoveNodeState for all nodes to be removed");
            List<Task> taskList = new List<Task>();
            try
            {
                foreach (var removedNode in removedNodes)
                {
                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Adding RemoveNodeStateAsync task for {0}", removedNode.NodeName);
                    var task = FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                          () =>
                          this.fabricClient.ClusterManager.RemoveNodeStateAsync(
                              removedNode.NodeName,
                              Constants.FabricQueryTimeoutInMinutes,
                              cancellationToken),
                          Constants.FabricQueryRetryTimeoutInMinutes);
                    taskList.Add(task);
                }

                await Task.WhenAll(taskList);
                UpgradeOrchestrationTrace.TraceSource.WriteInfo("RemoveNodeState succeeded for all nodes to be removed");
            }
            catch (Exception ex)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, string.Format("Invoking RemoveNodeStateAsync failed with exception {0}", ex.ToString()));
            }
        }

        private async Task UpdateSystemServicesAsync(Dictionary<string, ReplicaSetSize> targetReplicaSetSizes, TimeSpan timeout, CancellationToken cancellationToken)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Setting Target System Services size");
            var currentSystemServices = await this.fabricClient.QueryManager.GetServiceListAsync(this.fabricClient.FabricSystemApplication, null, timeout, cancellationToken);
            var currentSystemServicesNames = currentSystemServices.Select(service => service.ServiceName);

            foreach (var service in targetReplicaSetSizes)
            {
                try
                {
                    var serviceName = new Uri(service.Key);
                    if (!currentSystemServicesNames.Contains(serviceName))
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "System service {0} is not present in this cluster.. Skipping updating the service.", serviceName);
                        continue;
                    }

                    var serviceUpdateDescription = new StatefulServiceUpdateDescription()
                    {
                        TargetReplicaSetSize = service.Value.TargetReplicaSetSize,
                        MinReplicaSetSize = service.Value.MinReplicaSetSize
                    };
                    await this.fabricClient.ServiceManager.UpdateServiceAsync(serviceName, serviceUpdateDescription, timeout, cancellationToken);
                }
                catch (Exception ex)
                {
                    UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, string.Format("Invoking UpdateServiceAsync failed with exception {0}", ex.ToString()));
                }           
            }
        }

        private async Task<bool> SetTargetNodeConfigForRemovedNodesAsync(
            IEnumerable<NodeStatus> disablingNodes,
            long nodeConfigVersion,
            StandAloneCluster cluster,
            ConfigurationUpgradeDescription configUpgradeDesc,
            CancellationToken cancellationToken)
        {
            bool isTimedOut = false;           
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Checking nodes have been disabled.");
            try
            {
                await this.PollDeactivatedNodesAsync(disablingNodes, cancellationToken).ConfigureAwait(false);
            }
            catch (TimeoutException ex)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "Timed out waiting for nodes to be deactivated. Exception thrown : {0}", ex.ToString());
                isTimedOut = true;
            }

            if (isTimedOut)
            {
                return false;
            }

            var unhealthyNodesPercentage = Convert.ToByte((double)(disablingNodes.Count() * 100) / cluster.Current.NodeConfig.NodesStatus.Count());
            configUpgradeDesc.MaxPercentUnhealthyNodes = configUpgradeDesc.MaxPercentUnhealthyNodes != 0 ? configUpgradeDesc.MaxPercentUnhealthyNodes : unhealthyNodesPercentage;
            configUpgradeDesc.MaxPercentDeltaUnhealthyNodes = configUpgradeDesc.MaxPercentDeltaUnhealthyNodes != 0 ? configUpgradeDesc.MaxPercentDeltaUnhealthyNodes : unhealthyNodesPercentage;
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Setting MaxPercentUnhealthyNodes to {0} and MaxPercentDeltaUnhealthyNodes to {1}", configUpgradeDesc.MaxPercentUnhealthyNodes, configUpgradeDesc.MaxPercentDeltaUnhealthyNodes);

            configUpgradeDesc.MaxPercentUpgradeDomainDeltaUnhealthyNodes = configUpgradeDesc.MaxPercentUpgradeDomainDeltaUnhealthyNodes != 0 ? configUpgradeDesc.MaxPercentUpgradeDomainDeltaUnhealthyNodes : GetMaxPercentUpgradeDomainDeltaUnhealthyNodes(cluster.Topology, disablingNodes);
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Setting MaxPercentUpgradeDomainDeltaUnhealthyNodes to {0}", configUpgradeDesc.MaxPercentUpgradeDomainDeltaUnhealthyNodes);

            List<Microsoft.ServiceFabric.ClusterManagementCommon.NodeStatus> newNodeStatus = new List<Microsoft.ServiceFabric.ClusterManagementCommon.NodeStatus>();
            bool setStateToRemoved = false;
            foreach (var nodeStatus in cluster.Current.NodeConfig.NodesStatus)
            {                
                if (disablingNodes.Any(node => node.NodeName == nodeStatus.NodeName))
                {
                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Node {0} was removed. Re-setting NodeStatus to Removed", nodeStatus.NodeName);
                    setStateToRemoved = true;
                    newNodeStatus.Add(
                    new Microsoft.ServiceFabric.ClusterManagementCommon.NodeStatus
                    {
                        NodeName = nodeStatus.NodeName,
                        NodeType = nodeStatus.NodeType,
                        NodeState = NodeState.Removed,
                        NodeDeactivationIntent = WrpNodeDeactivationIntent.RemoveNode,
                        InstanceId = 0
                    });
                    cluster.Topology.Nodes.Remove(nodeStatus.NodeName);
                }
                else
                {
                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Node {0} was not removed. Re-using existing NodeStatus.", nodeStatus.NodeName);
                    newNodeStatus.Add(nodeStatus);
                }
            }

            cluster.TargetNodeConfig = new ClusterNodeConfig
            {
                Version = nodeConfigVersion + 1,
                NodesStatus = newNodeStatus
            };

            cluster.TargetCsmConfig = ((StandAloneUserConfig)cluster.Current.CSMConfig).Clone();
            return setStateToRemoved;
        }

        private async Task ProvisionPackageAsync(string targetCodeVersion, string localPackagePath, CancellationToken cancellationToken)
        {
            var commandParameter = new ClusterUpgradeCommandParameter
            {
                ConfigFilePath = null,
                ConfigVersion = null,
                CodeFilePath = localPackagePath,
                CodeVersion = targetCodeVersion
            };

            var timeoutHelper = new System.Fabric.UpgradeService.TimeoutHelper(TimeSpan.MaxValue, System.Fabric.UpgradeService.Constants.MaxOperationTimeout);
            var commandProcessor = new FabricClientWrapper();
            await commandProcessor.CopyAndProvisionUpgradePackageAsync(
                commandParameter,
                commandParameter.CodeVersion,
                null,
                timeoutHelper,
                cancellationToken).ConfigureAwait(false);
        }
    }
}
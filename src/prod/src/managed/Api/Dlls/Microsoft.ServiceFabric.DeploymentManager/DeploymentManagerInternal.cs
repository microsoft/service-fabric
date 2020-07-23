// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.ServiceProcess;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.DeploymentManager.BPA;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Microsoft.ServiceFabric.DeploymentManager.Model;
    using Newtonsoft.Json;

    using DMConstants = Microsoft.ServiceFabric.DeploymentManager.Constants;

    internal static class DeploymentManagerInternal
    {
        /// <summary>
        /// Creates a Service Fabric cluster based on deprecated XML Cluster Manifest file.
        /// </summary>
        /// <param name="clusterManifestPath"></param>
        /// <param name="fabricDataRoot"></param>
        /// <param name="fabricLogRoot"></param>
        /// <param name="fabricPackageType"></param>
        /// <param name="fabricPackageSourcePath"></param>
        /// <param name="fabricPackageDestinationPath"></param>
        /// <param name="rollbackOnFailure"></param>
        /// <returns></returns>
        internal static async Task CreateClusterAsyncInternal(
            string clusterManifestPath,
            string fabricDataRoot,
            string fabricLogRoot,
            FabricPackageType fabricPackageType,
            string fabricPackageSourcePath, // only valid if fabricPackageType is XCopyPackage
            string fabricPackageDestinationPath, // only valid if fabricPackageType is XCopyPackage
            ClusterCreationOptions deploymentOption)
        {
            await CreateClusterAsyncInternal(
                clusterManifestPath,
                fabricDataRoot,
                fabricLogRoot,
                false,
                fabricPackageType,
                fabricPackageSourcePath,
                fabricPackageDestinationPath,
                deploymentOption).ConfigureAwait(false);
        }

        internal static async Task CreateClusterAsyncInternal(
            string clusterConfigPath,
            string fabricDataRoot,
            string fabricLogRoot,
            bool isJsonClusterConfig,
            FabricPackageType fabricPackageType,
            string fabricPackageSourcePath, // only valid if fabricPackageType is XCopyPackage
            string fabricPackageDestinationPath, // only valid if fabricPackageType is XCopyPackage
            ClusterCreationOptions deploymentOption,
            int maxPercentFailedNodes = 0,
            TimeSpan? timeout = null)
        {
            if (string.IsNullOrEmpty(clusterConfigPath) || !File.Exists(clusterConfigPath))
            {
                throw new FileNotFoundException(StringResources.Error_SFPreconditionsConfigPathInvalid);
            }

            if (fabricPackageType == FabricPackageType.XCopyPackage)
            {
                if (!File.Exists(fabricPackageSourcePath))
                {
                    throw new FileNotFoundException(StringResources.Error_SFPreconditionsPackageNotExist);
                }

                if (!fabricPackageSourcePath.EndsWith(".cab", true, null))
                {
                    throw new InvalidOperationException(StringResources.Error_SFPreconditionsPackageSrcMustBeCab);
                }
            }
            else
            {
                if (!string.IsNullOrEmpty(fabricPackageSourcePath) || !string.IsNullOrEmpty(fabricPackageDestinationPath))
                {
                    throw new InvalidOperationException(StringResources.Error_SFPreconditionsMsiPackageUnexpected);
                }
            }

            if (maxPercentFailedNodes < 0 || maxPercentFailedNodes > 100)
            {
                throw new InvalidOperationException(StringResources.Error_MaxPercentFailedNodesInvalid);
            }

            Initialize();
            try
            {
                TimeoutHelper timer = new TimeoutHelper(timeout ?? TimeSpan.MaxValue);
                MachineHealthContainer machineHealthContainer = new MachineHealthContainer(maxPercentFailedNodes);
                if (isJsonClusterConfig)
                {
                    AnalysisSummary summary = await StandaloneUtility.ExecuteActionWithTimeoutAsync<AnalysisSummary>(
                        () => BestPracticesAnalyzer.AnalyzeClusterSetupAsync(
                            clusterConfigPath, 
                            null, 
                            fabricPackageSourcePath, 
                            deploymentOption.HasFlag(ClusterCreationOptions.Force),
                            maxPercentFailedNodes),
                        timer.GetRemainingTime()).ConfigureAwait(false);

                    if (!summary.Passed)
                    {
                        SFDeployerTrace.WriteError(StringResources.Error_BPAClusterSetupCancelled);
                        throw new FabricValidationException(StringResources.Error_BPAClusterSetupCancelled, FabricErrorCode.OperationCanceled);
                    }

                    machineHealthContainer = summary.MachineHealthContainer;
                }
                else if (!BestPracticesAnalyzer.ValidateClusterSetupXmlManifest(clusterConfigPath, fabricPackageType))
                {
                    SFDeployerTrace.WriteError(StringResources.Error_BPAClusterSetupCancelled);
                    throw new FabricValidationException(StringResources.Error_BPAClusterSetupCancelled, FabricErrorCode.OperationCanceled);
                }

                SFDeployerTrace.WriteInfo(StringResources.Info_CreatingServiceFabricCluster);
                string clusterManifestPath;
                if (isJsonClusterConfig)
                {
                    var config = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(clusterConfigPath);
                    var userConfig = config.GetUserConfig();
                    string codeVersion = userConfig.CodeVersion;
                    bool isCodeVersionSpecified = !string.IsNullOrEmpty(codeVersion) && (codeVersion != DMConstants.AutoupgradeCodeVersion);

                    if (fabricPackageType == FabricPackageType.XCopyPackage &&
                        isCodeVersionSpecified)
                    {
                        try
                        {
                            var version = CabFileOperations.GetCabVersion(fabricPackageSourcePath);
                            if (version != codeVersion)
                            {
                                throw new FabricValidationException(StringResources.Error_SFPreconditionsVersionMismatch);
                            }
                        }
                        catch (FabricValidationException ex)
                        {
                            SFDeployerTrace.WriteError(
                                 "Failed to get validate code version for package {0} and codeversion {1}, Exception: {2}",
                                 fabricPackageSourcePath,
                                 codeVersion,
                                ((object)ex).GetType().ToString());

                            throw;
                        }
                    }

                    var clusterResource = GetClusterResource(clusterConfigPath, System.Guid.NewGuid().ToString());
                    if (!(config is DevJsonModel))
                    {
                        // Copy jsonConfig metadata to FabricDataRoot. UOS reads this file to perform baseline upgrade.
                        string dataRootPath = StandaloneUtility.GetFabricDataRootFromClusterManifest(clusterResource.Current.ExternalState.ClusterManifest);
                        CopyClusterResourceToAllMachines(clusterResource, dataRootPath, machineHealthContainer);
                    }

                    clusterManifestPath = StandaloneUtility.ClusterManifestToFile(clusterResource.Current.ExternalState.ClusterManifest, config.Name, config.ClusterConfigurationVersion).FullName;

                    if (deploymentOption.HasFlag(ClusterCreationOptions.Force))
                    {
                        var importantSettings = config.GetFabricSystemSettings();
                        string dataRoot = importantSettings.ContainsKey(Constants.FabricDataRootString) ? importantSettings[Constants.FabricDataRootString] : null;
                        foreach (var machine in machineHealthContainer.GetHealthyMachineNames())
                        {
                            DeleteFabricDataRoot(config, machine, dataRoot);
                        }
                    }
                }
                else
                {
                    // Legacy manifest deployment mode
                    clusterManifestPath = clusterConfigPath;
                    machineHealthContainer.SetHealthyMachines(StandaloneUtility.GetMachineNamesFromClusterManifest(clusterConfigPath));
                }

                bool successfulRun = true;
                List<Exception> exceptions = new List<Exception>();
                List<Task> tasks = new List<Task>();
                try
                {
                    SFDeployerTrace.WriteInfo(StringResources.Info_SFConfiguringNodes);

                    await StandaloneUtility.ExecuteActionWithTimeoutAsync(
                        () => ConfigureNodesAsync(
                                machineHealthContainer,
                                clusterManifestPath,
                                null,
                                isJsonClusterConfig ? clusterConfigPath : null,
                                fabricDataRoot,
                                fabricLogRoot,
                                fabricPackageType,
                                fabricPackageSourcePath,
                                fabricPackageDestinationPath),
                        timer.GetRemainingTime()).ConfigureAwait(false);

                    if (!IsMinQuorumSeedNodeHealthy(machineHealthContainer, clusterManifestPath))
                    {
                        throw new FabricException(StringResources.Error_CreateClusterCancelledNotEnoughHealthy);
                    }

                    // Start fabric installer service or fabric host service on all machines and wait for all services to complete.
                    RunFabricServices(machineHealthContainer, fabricPackageType, timer.GetRemainingTime());

                    // Remove the generated clusterManifest.xml file from temp folder for deploy anywhere deployment.
                    if (isJsonClusterConfig && File.Exists(clusterManifestPath))
                    {
                        try
                        {
                            // Best-effort
                            File.Delete(clusterManifestPath);
                        }
                        catch (Exception ex)
                        {
                            SFDeployerTrace.WriteNoise(
                                StringResources.Error_SFDeletingTempManifest,
                                ((object)ex).GetType().ToString(),
                                clusterManifestPath);
                        }
                    }
                }
                catch (Exception ex)
                {
                    successfulRun = false;
                    SFDeployerTrace.WriteError(StringResources.Error_SFCreateClusterAsyncGeneric, ex);
                    if (tasks.Count > 0)
                    {
                        exceptions.AddRange(tasks.Where(t => t.IsFaulted).Select(u => u.Exception));
                        SFDeployerTrace.WriteError(StringResources.Error_SFCreateClusterErrorsPrompt);
                        for (int i = 0; i < exceptions.Count; i++)
                        {
                            SFDeployerTrace.WriteError(StringResources.Error_SFCreateClusterExList, i, exceptions[i]);
                        }
                    }
                }

                if (!successfulRun)
                {
                    if (!deploymentOption.HasFlag(ClusterCreationOptions.OptOutCleanupOnFailure)
                        && fabricPackageType == FabricPackageType.XCopyPackage)
                    {
                        try
                        {
                            await BestEffortRollback(machineHealthContainer.GetAllMachineNames().ToList()).ConfigureAwait(true);
                        }
                        catch (Exception ex)
                        {
                            SFDeployerTrace.WriteError(StringResources.Error_SFRollbackFailed, ex);
                            exceptions.Add(ex);
                        }
                    }

                    AggregateException ae = new AggregateException(exceptions);
                    throw ae;
                }

                SFDeployerTrace.WriteNoise(StringResources.Info_SFCreateClusterCompleted);
            }
            finally
            {
                Destroy();
            }
        }

        internal static async Task<GoalStateModel> GetGoalStateModelAsync()
        {
            Uri goalStateUri;
            string goalStateUriStr = StandaloneUtility.GetGoalStateUri();
            if (!Uri.TryCreate(goalStateUriStr, UriKind.Absolute, out goalStateUri))
            {
                SFDeployerTrace.WriteError(StringResources.Error_SFGoalStateUriNotParsed, goalStateUriStr);
                throw new FabricValidationException(
                    StringResources.Error_SFGoalStateNotFound,
                    FabricErrorCode.OperationCanceled);
            }

            if (!await StandaloneUtility.IsUriReachableAsync(goalStateUri).ConfigureAwait(false))
            {
                SFDeployerTrace.WriteError(StringResources.Error_SFGoalStateUriNotReachable, goalStateUri.AbsolutePath);
                throw new FabricValidationException(
                    StringResources.Error_SFGoalStateNotFound,
                    FabricErrorCode.OperationCanceled);
            }

            string goalStateJson = await GoalStateModel.GetGoalStateFileJsonAsync(goalStateUri).ConfigureAwait(false);
            if (string.IsNullOrEmpty(goalStateJson))
            {
                SFDeployerTrace.WriteError(StringResources.Error_SFGoalStateFileEmpty);
                throw new FabricValidationException(
                    StringResources.Error_SFGoalStateFileEmpty,
                    FabricErrorCode.OperationCanceled);
            }

            GoalStateModel model = GoalStateModel.GetGoalStateModelFromJson(goalStateJson);
            if (model == null || model.Packages == null)
            {
                SFDeployerTrace.WriteError(StringResources.Error_SFGoalStateFileNotDeserialize);
                throw new FabricValidationException(
                    StringResources.Error_SFGoalStateFileNotDeserialize,
                    FabricErrorCode.OperationCanceled);
            }

            return model;
        }

        internal static List<RuntimePackageDetails> GetDownloadableRuntimeVersions(
            DownloadableRuntimeVersionReturnSet returnSet)
        {
            Initialize();
            try
            {
                var model = GetGoalStateModelAsync().Result;
                return GetRuntimeSupportedPackages(model, returnSet);
            }
            finally
            {
                Destroy();
            }
        }

        internal static List<RuntimePackageDetails> GetUpgradeableRuntimeVersions(
            string baseVersion)
        {
            Version baseVer;
            if (!Version.TryParse(baseVersion, out baseVer))
            {
                throw new FabricValidationException(
                    StringResources.Error_SFInvalidBaseVersion,
                    FabricErrorCode.OperationCanceled);
            }

            Initialize();
            try
            {
                var model = GetGoalStateModelAsync().Result;
                return GetRuntimeUpgradeablePackages(model, baseVersion);
            }
            finally
            {
                Destroy();
            }
        }

        internal static List<RuntimePackageDetails> GetRuntimeUpgradeablePackages(GoalStateModel model, string baseVersion)
        {
            var allPackages = model.Packages;
            var result = new List<RuntimePackageDetails>();
            foreach (PackageDetails package in allPackages)
            {
                if (IsPackageUpgradeCompatible(package, baseVersion))
                {
                    result.Add(RuntimePackageDetails.CreateFromPackageDetails(package));
                }
            }

            return result;
        }

        internal static List<RuntimePackageDetails> GetRuntimeSupportedPackages(GoalStateModel model, DownloadableRuntimeVersionReturnSet returnSet = DownloadableRuntimeVersionReturnSet.Supported)
        {
            var allPackages = model.Packages;
            var result = new List<RuntimePackageDetails>();
            switch (returnSet)
            {
                case DownloadableRuntimeVersionReturnSet.All:
                    foreach (PackageDetails package in allPackages)
                    {
                        result.Add(RuntimePackageDetails.CreateFromPackageDetails(package));
                    }

                    break;

                case DownloadableRuntimeVersionReturnSet.Supported:
                    foreach (PackageDetails package in allPackages)
                    {
                        if (!IsPackageExpired(package))
                        {
                            result.Add(RuntimePackageDetails.CreateFromPackageDetails(package));
                        }
                    }

                    break;

                case DownloadableRuntimeVersionReturnSet.Latest:
                    PackageDetails goalPackageDetails = model.Packages.SingleOrDefault(package => package.IsGoalPackage);
                    if (goalPackageDetails == null)
                    {
                        SFDeployerTrace.WriteError(StringResources.Error_SFGoalStateFileHasNoIsGoalTrue);
                        throw new FabricValidationException(
                            StringResources.Error_SFGoalStateFileHasNoIsGoalTrue,
                            FabricErrorCode.OperationCanceled);
                    }

                    result.Add(RuntimePackageDetails.CreateFromPackageDetails(goalPackageDetails));
                    break;
            }

            return result;
        }

        internal static bool IsPackageExpired(PackageDetails package)
        {
            DateTime currentDate = DateTime.UtcNow.Date;
            if (!package.SupportExpiryDate.HasValue)
            {
                return false;
            }
            else
            {
                DateTime expiryDate = package.SupportExpiryDate.Value;
                return expiryDate < currentDate;
            }
        }

        internal static bool IsPackageUpgradeCompatible(PackageDetails package, string baseVersion)
        {
            if (string.IsNullOrEmpty(baseVersion) || string.IsNullOrEmpty(package.MinVersion))
            {
                return true;
            }

            var baseVer = new Version(baseVersion);
            var currVer = new Version(package.Version);
            var currMinVer = new Version(package.MinVersion);
            return currMinVer.CompareTo(baseVer) <= 0 && currVer.CompareTo(baseVer) >= 0;
        }

        internal static async Task RemoveClusterAsyncInternal(
            string clusterConfigPath,
            bool deleteLog,
            bool isJsonClusterConfig,
            FabricPackageType fabricPackageType)
        {
            if (string.IsNullOrEmpty(clusterConfigPath) || !File.Exists(clusterConfigPath))
            {
                throw new FileNotFoundException(StringResources.Error_SFPreconditionsConfigPathInvalid);
            }

            Initialize();
            try
            {
                if ( /* JSON config */
                    (isJsonClusterConfig
                     && !BestPracticesAnalyzer.ValidateClusterRemoval(clusterConfigPath))
                    /* XML manifest */
                    || (!isJsonClusterConfig
                        && !BestPracticesAnalyzer.ValidateClusterRemovalXmlManifest(clusterConfigPath)))
                {
                    SFDeployerTrace.WriteNoise(StringResources.Error_BPAClusterSetupCancelled);
                    throw new FabricValidationException(StringResources.Error_BPAClusterSetupCancelled, FabricErrorCode.OperationCanceled);
                }

                var jsonConfig = isJsonClusterConfig ? StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(clusterConfigPath) : null;
                SFDeployerTrace.WriteNoise(StringResources.Info_RemovingServiceFabricCluster);
                List<string> machines = isJsonClusterConfig
                                        ? jsonConfig.GetClusterTopology().Machines
                                        : StandaloneUtility.GetMachineNamesFromClusterManifest(clusterConfigPath);

                await RemoveNodeConfigurationAsync(machines, deleteLog, fabricPackageType, false).ConfigureAwait(false);
                SFDeployerTrace.WriteNoise(StringResources.Info_SFRemoveClusterCompleted);
            }
            catch (Exception ex)
            {
                SFDeployerTrace.WriteError(StringResources.Error_SFCreateClusterEx, ex);
                throw;
            }
            finally
            {
                Destroy();
            }
        }

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1305:FieldNamesMustNotUseHungarianNotation", Justification = "Reviewed. Suppression is OK here for ipAddressOrFQDN.")]
        internal static async Task AddNodeAsyncInternal(
            string nodeName,
            string nodeType,
            string ipAddressOrFQDN,
            string upgradeDomain,
            string faultDomain,
            string fabricPackageSourcePath,
            FabricClient fabricClient)
        {
            if (string.IsNullOrEmpty(nodeName))
            {
                throw new InvalidOperationException(StringResources.Error_SFPreconditionsNodeNameInvalid);
            }

            if (string.IsNullOrEmpty(nodeType))
            {
                throw new InvalidOperationException(StringResources.Error_SFPreconditionsNodeTypeInvalid);
            }

            if (string.IsNullOrEmpty(faultDomain))
            {
                throw new InvalidOperationException(StringResources.Error_SFPreconditionsFaultDomainInvalid);
            }

            if (string.IsNullOrEmpty(upgradeDomain))
            {
                throw new InvalidOperationException(StringResources.Error_SFPreconditionsUpgradeDomainInvalid);
            }

            if (string.IsNullOrEmpty(ipAddressOrFQDN))
            {
                throw new InvalidOperationException(StringResources.Error_SFPreconditionsNodeIPAddressInvalid);
            }

            ReleaseAssert.AssertIf(fabricClient == null, "fabricClient should not be null");

            Initialize();
            try
            {
                FabricUpgradeProgress upgradeProgress;
                upgradeProgress = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync<FabricUpgradeProgress>(
                     () =>
                     fabricClient.ClusterManager.GetFabricUpgradeProgressAsync(TimeSpan.FromMinutes(Constants.FabricOperationTimeoutInMinutes), CancellationToken.None),
                     TimeSpan.FromMinutes(Constants.FabricOperationTimeoutInMinutes),
                     CancellationToken.None).ConfigureAwait(false);

                if (!StandaloneUtility.IsUpgradeCompleted(upgradeProgress.UpgradeState))
                {
                    SFDeployerTrace.WriteError(StringResources.Error_SFAddNodeWhileUpgradeInProgress);
                    throw new FabricValidationException(
                        StringResources.Error_SFAddNodeWhileUpgradeInProgress,
                        FabricErrorCode.OperationCanceled);
                }

                /* Need to check if version is not 0.0.0.0. Sometimes CM returns RollingForwardCompleted even when baseline upgrade is still in progress.
                * This happens because AddNode is called immediately after CreateCluster and CM is not able to update progress in that timespan.
                * CM takes a few seconds to get to RollingForwardInProgress state when baseline upgrade starts.
                */
                if (string.Compare(upgradeProgress.TargetCodeVersion, Constants.PreBaselineVersion) == 0)
                {
                    SFDeployerTrace.WriteError(StringResources.Error_SFAddNodeWhileUpgradeInProgress);
                    throw new FabricValidationException(
                         StringResources.Error_SFAddNodeWhileUpgradeInProgress,
                         FabricErrorCode.OperationCanceled);
                }

                // Get cluster manifest
                string clusterManifestContent = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                                    () => fabricClient.ClusterManager.GetClusterManifestAsync()).ConfigureAwait(false);
                var clusterManifest = XMLHelper.ReadClusterManifestFromContent(clusterManifestContent);

                // Get data root from manifest to run validations
                string fabricDataRoot = StandaloneUtility.GetFabricDataRootFromClusterManifest(clusterManifest);
                string fabricLogRoot = StandaloneUtility.GetFabricLogRootFromClusterManifest(clusterManifest);
                if (!BestPracticesAnalyzer.ValidateAddNode(ipAddressOrFQDN, nodeName, fabricPackageSourcePath, fabricDataRoot, fabricLogRoot))
                {
                    SFDeployerTrace.WriteError(StringResources.Error_BPAAddNodeCancelled);
                    throw new FabricValidationException(StringResources.Error_BPAAddNodeCancelled, FabricErrorCode.OperationCanceled);
                }

                SFDeployerTrace.WriteInfo(StringResources.Info_AddServiceFabricNode);

                List<string> machines = new List<string>();
                machines.Add(ipAddressOrFQDN);

                // NodeType
                var targetNodeType = clusterManifest.NodeTypes.FirstOrDefault(nodeNodeType => nodeNodeType.Name == nodeType);
                if (targetNodeType == null)
                {
                    throw new InvalidOperationException(StringResources.Error_SFPreconditionsNodeTypeInvalid);
                }

                // Add the Node Infrastructure
                var infrastructureType = clusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureWindowsServer;
                infrastructureType.ThrowIfNull("Node Infrastructure section is not available in the cluster manifest.");

                if (infrastructureType.IsScaleMin)
                {
                    SFDeployerTrace.WriteError(StringResources.Error_SFAddNodeInvalidScaleMin);
                    throw new FabricValidationException(StringResources.Error_SFAddNodeInvalidScaleMin, FabricErrorCode.OperationCanceled);
                }

                var nodesList = infrastructureType.NodeList.ToList();
                var existingNode = infrastructureType.NodeList.FirstOrDefault(n => n.NodeName == nodeName);
                if (existingNode == null)
                {
                    nodesList.Add(new FabricNodeType
                    {
                        NodeName = nodeName,
                        UpgradeDomain = upgradeDomain,
                        FaultDomain = faultDomain,
                        IPAddressOrFQDN = ipAddressOrFQDN,
                        NodeTypeRef = nodeType,
                        IsSeedNode = false
                    });
                }
                else
                {
                    existingNode.FaultDomain = faultDomain;
                    existingNode.IPAddressOrFQDN = ipAddressOrFQDN;
                    existingNode.NodeTypeRef = nodeType;
                    existingNode.UpgradeDomain = upgradeDomain;
                }

                infrastructureType.NodeList = nodesList.ToArray();

                // Write cluster manifest
                string clusterManifestPath = Path.Combine(Path.GetTempPath(), "clusterManifest.xml");
                XMLHelper.WriteClusterManifest(clusterManifestPath, clusterManifest);

                bool successfulRun = true;
                List<Exception> exceptions = new List<Exception>();
                List<Task> tasks = new List<Task>();
                try
                {
                    SFDeployerTrace.WriteInfo(StringResources.Info_SFConfiguringNodes);

                    await ConfigureNodesAsync(
                        new MachineHealthContainer(machines),
                        clusterManifestPath,
                        null /*InfrastructureManifestPath*/,
                        null /*JsonClusterConfigPath*/,
                        null /*fabricDataRoot*/,
                        null /*fabricLogRoot*/,
                        FabricPackageType.XCopyPackage,
                        fabricPackageSourcePath /*fabricPackageSourcePath*/,
                        null /*fabricPackageDestinationPath*/).ConfigureAwait(false);

                    // Start fabric installer service or fabric host service on the added machine and wait for all services to complete.
                    StartAndWaitForInstallerService(ipAddressOrFQDN);
                    SFDeployerTrace.WriteInfo("Re-enabling the node {0}.", nodeName);
                    await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => fabricClient.ClusterManager.ActivateNodeAsync(nodeName)).ConfigureAwait(false);
                }
                catch (Exception ex)
                {
                    successfulRun = false;
                    SFDeployerTrace.WriteError(StringResources.Error_SFAddNodeExList, -1, ex);
                    if (tasks.Count > 0)
                    {
                        exceptions.AddRange(tasks.Where(t => t.IsFaulted).Select(u => u.Exception));
                        SFDeployerTrace.WriteError(StringResources.Error_SFAddNodeErrorsPrompt);
                        for (int i = 0; i < exceptions.Count; i++)
                        {
                            SFDeployerTrace.WriteError(StringResources.Error_SFAddNodeExList, i, exceptions[i]);
                        }
                    }
                }

                if (successfulRun)
                {
                    // Remove the generated clusterManifest.xml file from temp folder for deploy anywhere deployment.
                    if (File.Exists(clusterManifestPath))
                    {
                        try
                        {
                            // Best-effort
                            File.Delete(clusterManifestPath);
                        }
                        catch (Exception ex)
                        {
                            SFDeployerTrace.WriteWarning(
                                StringResources.Error_SFDeletingTempManifest,
                                ex.GetType().ToString(),
                                clusterManifestPath);
                        }
                    }
                }
                else
                {
                    try
                    {
                        await BestEffortRollback(machines).ConfigureAwait(true);
                    }
                    catch (Exception ex)
                    {
                        SFDeployerTrace.WriteError(StringResources.Error_SFRollbackFailed, ex);
                        exceptions.Add(ex);
                    }

                    AggregateException ae = new AggregateException(exceptions);
                    throw ae;
                }

                SFDeployerTrace.WriteInfo(StringResources.Info_SFAddNodeCompleted);
            }
            finally
            {
                Destroy();
            }
        }

        internal static async Task RemoveNodeAsyncInternal(FabricClient fabricClient)
        {
            ReleaseAssert.AssertIf(fabricClient == null, "fabricClient should not be null");

            Initialize();
            SFDeployerTrace.WriteWarning(StringResources.Warning_SFDeprecatedRemoveNodeAPI);
            try
            {
                if (!BestPracticesAnalyzer.ValidateRemoveNode(Environment.GetEnvironmentVariable("COMPUTERNAME")))
                {
                    SFDeployerTrace.WriteError(StringResources.Error_BPARemoveNodeCancelled);
                    throw new FabricValidationException(StringResources.Error_BPARemoveNodeCancelled, FabricErrorCode.OperationCanceled);
                }

                // Get cluster manifest
                string clusterManifestContent = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                                    () => fabricClient.ClusterManager.GetClusterManifestAsync()).ConfigureAwait(false);
                var clusterManifest = XMLHelper.ReadClusterManifestFromContent(clusterManifestContent);
                var infrastructureType = clusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureWindowsServer;
                infrastructureType.ThrowIfNull("Node Infrastructure section is not available in the cluster manifest.");

                if (infrastructureType.IsScaleMin)
                {
                    SFDeployerTrace.WriteError(StringResources.Error_SFRemoveNodeInvalidScaleMin);
                    throw new FabricValidationException(StringResources.Error_SFRemoveNodeInvalidScaleMin, FabricErrorCode.OperationCanceled);
                }

                SFDeployerTrace.WriteInfo(StringResources.Info_RemoveServiceFabricNode);
                var node = await FindFabricNodeForCurrentMachineAsync(fabricClient);
                List<string> machines = new List<string>();
                machines.Add(node.IpAddressOrFQDN);

                SFDeployerTrace.WriteInfo(StringResources.Info_SFRemovingNode_Formatted, node.NodeName);
                bool deactiveNodeCalled = false;
                var timeoutHelper = new TimeoutHelper(TimeSpan.FromMinutes(Constants.FabricOperationTimeoutInMinutes));
                while (!TimeoutHelper.HasExpired(timeoutHelper))
                {
                    try
                    {
                        var nodedeactivate = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                                 () => fabricClient.QueryManager.GetNodeListAsync(node.NodeName, Constants.FabricQueryNodeListTimeout, CancellationToken.None),
                                                 Constants.FabricQueryNodeListRetryTimeout).ConfigureAwait(false);
                        node = nodedeactivate.FirstOrDefault();
                        node.ThrowIfNull("Node is not found in the cluster.");

                        if (node.IsSeedNode)
                        {
                            SFDeployerTrace.WriteError(StringResources.Error_SFRemoveNodeCannotRemoveSeed_Formatted, node.NodeName);
                            return;
                        }

                        if (node.NodeStatus != NodeStatus.Disabled)
                        {
                            if (!deactiveNodeCalled)
                            {
                                await fabricClient.ClusterManager.DeactivateNodeAsync(node.NodeName, NodeDeactivationIntent.RemoveNode);
                                deactiveNodeCalled = true;
                            }

                            await Task.Delay(5000);
                            continue;
                        }

                        await RemoveNodeConfigurationAsync(machines, false, FabricPackageType.XCopyPackage, false).ConfigureAwait(false);
                        await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() => fabricClient.ClusterManager.RemoveNodeStateAsync(node.NodeName)).ConfigureAwait(false);
                        SFDeployerTrace.WriteInfo(StringResources.Info_SFRemoveNodeCompleted);
                        return;
                    }
                    catch (FabricTransientException fte)
                    {
                        SFDeployerTrace.WriteError(StringResources.Error_SFRemoveNodeRetrying_Formatted, fte);
                    }
                }

                timeoutHelper.ThrowIfExpired();
            }
            catch (Exception ex)
            {
                SFDeployerTrace.WriteError(StringResources.Error_SFRemoveNodeExList, ex);
                throw;
            }
            finally
            {
                Destroy();
            }
        }

        internal static AnalysisSummary BpaAnalyzeClusterSetup(string clusterConfigPath, string oldClusterConfigPath, string fabricPackagePath, int maxPercentFailedNodes)
        {
            Initialize();
            try
            {
                return BestPracticesAnalyzer.AnalyzeClusterSetupAsync(clusterConfigPath, oldClusterConfigPath, fabricPackagePath, false, maxPercentFailedNodes).Result;
            }
            finally
            {
                Destroy();
            }
        }

        internal static Task ConfigureNodesAsync(
            IEnumerable<string> machineNames,
            string clusterManifestPath,
            string fabricDataRoot,
            string fabricLogRoot)
        {
            return ConfigureNodesAsync(
                       new MachineHealthContainer(machineNames),
                       clusterManifestPath,
                       null /*InfrastructureManifestPath*/,
                       null/*JsonClusterConfigPath*/,
                       fabricDataRoot,
                       fabricLogRoot,
                       FabricPackageType.MSI,
                       null,
                       null);
        }

        internal static Task ConfigureNodesAsync(
            MachineHealthContainer machineHealthContainer,
            string clusterManifestPath,
            string infrastructureManifestPath,
            string jsonClusterConfigPath,
            string fabricDataRoot,
            string fabricLogRoot,
            FabricPackageType fabricPackageType,
            string fabricPackageSourcePath,
            string fabricPackageDestinationPath)
        {
            return Task.Run(() =>
            {
                // Copy package over to target machine if using XCopy package type
                if (fabricPackageType == FabricPackageType.XCopyPackage)
                {
                    if (string.IsNullOrEmpty(fabricPackageSourcePath) || !File.Exists(fabricPackageSourcePath))
                    {
                        string message = string.Format(StringResources.Error_SFSourcePackageInvalidPath, fabricPackageSourcePath ?? string.Empty);
                        SFDeployerTrace.WriteError(message);
                        throw new FileNotFoundException(message);
                    }

                    if (string.IsNullOrEmpty(fabricPackageDestinationPath))
                    {
                        string candidateMachine = machineHealthContainer.GetHealthyMachineNames().ElementAt(0);
                        fabricPackageDestinationPath = Utility.GetDefaultPackageDestination(candidateMachine);
                        SFDeployerTrace.WriteInfo(StringResources.Info_SFDefaultFabricRoot, candidateMachine);
                    }

                    SFDeployerTrace.WriteNoise(StringResources.Info_SFInstallationDirectory, fabricPackageDestinationPath);

                    // Extract FabricInstallerService files locally
                    string tempDirectory = Helpers.GetNewTempPath();
                    SFDeployerTrace.WriteNoise(StringResources.Info_SFExtractingServiceFiles, tempDirectory);
                    ExtractInstallerService(fabricPackageSourcePath, tempDirectory);

                    // Installer Service to destination machines
                    string tempInstallerSvcDirectory = Path.Combine(tempDirectory, Constants.FabricInstallerServiceFolderName);
                    SFDeployerTrace.WriteInfo(StringResources.Info_SFCopyingInstallerAndPackage, tempDirectory);
                    CopyInstallerToAllMachines(machineHealthContainer, fabricPackageSourcePath, fabricPackageDestinationPath, tempInstallerSvcDirectory);

                    try
                    {
                        FabricDirectory.Delete(tempDirectory, true, true);
                    }
                    catch
                    {
                        SFDeployerTrace.WriteNoise(StringResources.Info_CleanTempDirectoryFailed, tempDirectory);
                    }
                }

                Parallel.ForEach<string>(
                    machineHealthContainer.GetHealthyMachineNames(),
                    (string machineName) =>
                    {
                        try
                        {
                            SFDeployerTrace.WriteInfo(StringResources.Info_SFConfiguringMachine, machineName);
                            ConfigurationDeployer.NewNodeConfiguration(
                                clusterManifestPath,
                                infrastructureManifestPath,
                                jsonClusterConfigPath,
                                fabricDataRoot,
                                fabricLogRoot,
                                null,
                                null,
                                false,
                                false,
                                fabricPackageType,
                                fabricPackageDestinationPath,
                                machineName,
                                fabricPackageSourcePath);

                            SFDeployerTrace.WriteInfo(StringResources.Info_SFMachineConfigured, machineName);
                        }
                        catch (Exception ex)
                        {
                            if (machineHealthContainer.MaxPercentFailedNodes == 0)
                            {
                                throw;
                            }

                            machineHealthContainer.MarkMachineAsUnhealthy(machineName);
                            SFDeployerTrace.WriteWarning(StringResources.Error_NodeConfigurationFailed, machineName, ex);
                        }
                    });

                if (!machineHealthContainer.EnoughHealthyMachines())
                {
                    throw new FabricException(StringResources.Error_CreateClusterCancelledNotEnoughHealthy);
                }
            });
        }

        internal static Task RemoveNodeConfigurationAsync(IEnumerable<string> machineNames, bool deleteLog, FabricPackageType fabricPackageType, bool isBestEffortRollBack)
        {
            string controlMachineEventLogsDir = Path.Combine(Utility.GetTempPath("localhost"), Constants.ServiceFabricEventLogsDirectoryName);

            if (Directory.Exists(controlMachineEventLogsDir))
            {
                FabricDirectory.Delete(controlMachineEventLogsDir, true, false);
            }

            return Task.Run(() =>
            {
                Parallel.ForEach(
                    machineNames,
                    (string machineName) =>
                    {
                        if (StandaloneUtility.IsFabricInstalled(machineName))
                        {
                            string fabricPackageRoot = null;
                            if (fabricPackageType == FabricPackageType.XCopyPackage)
                            {
                                // Try to cache Fabric Root directory from registry since registry keys will be deleted after RemoveNodeConfiguration
                                try
                                {
                                    fabricPackageRoot = FabricEnvironment.GetRoot(machineName);
                                }
                                catch (FabricException)
                                {
                                    SFDeployerTrace.WriteWarning(StringResources.Warning_SFFabricRootNotFoundReg, machineName);
                                }
                            }

                            try
                            {
                                SFDeployerTrace.WriteInfo(StringResources.Info_SFRemovingConfiguration, machineName);
                                ConfigurationDeployer.RemoveNodeConfiguration(deleteLog, fabricPackageType, machineName);
                                SFDeployerTrace.WriteInfo(StringResources.Info_SFConfigurationRemoved, machineName);
                            }
                            catch (Exception ex)
                            {
                                SFDeployerTrace.WriteWarning(StringResources.Error_SFRemoveConfigurationFailed, machineName, ex);
                                throw;
                            }
                            finally
                            {
                                // Delete package at package root path
                                if (fabricPackageType == FabricPackageType.XCopyPackage)
                                {
                                    if (!StandaloneUtility.IsFabricInstalled(machineName))
                                    {
                                        DeleteFabricPackage(machineName, fabricPackageRoot);
                                    }
                                }
                            }
                        }

                        if (isBestEffortRollBack)
                        {
                            try
                            {
                                // Read the event logs file in temp directory in each node and display it line by line in Powershell window 
                                // also save them to temp in control machine.
                                string eventLogsLocationOnEachNode = Helpers.GetRemotePath(Path.Combine(Utility.GetTempPath(machineName), Constants.EventLogsFileName), machineName);
                                if (File.Exists(eventLogsLocationOnEachNode))
                                {
                                    string[] lines = System.IO.File.ReadAllLines(eventLogsLocationOnEachNode);
                                    foreach (string line in lines)
                                    {
                                        SFDeployerTrace.WriteError(line);
                                    }
                                }

                                FabricDirectory.CreateDirectory(controlMachineEventLogsDir);

                                string controlMachineEventLogPath = Path.Combine(controlMachineEventLogsDir, machineName + ".txt");
                                File.Copy(eventLogsLocationOnEachNode, controlMachineEventLogPath, true);
                                SFDeployerTrace.WriteInfo(StringResources.Info_ViewEventLogs, controlMachineEventLogsDir, machineName);
                                File.Delete(eventLogsLocationOnEachNode);
                            }
                            catch (Exception e)
                            {
                                SFDeployerTrace.WriteWarning(StringResources.Error_CollectingEventLogsFailed, e, machineName);
                            }
                        }
                    });
            });
        }

        internal static void StartAndWaitForInstallerService(string machineName, TimeSpan? timeout = null)
        {
            TimeoutHelper timer = new TimeoutHelper(timeout ?? TimeSpan.MaxValue);
            ServiceController installerSvc = FabricDeployerServiceController.GetServiceInStoppedState(machineName, Constants.FabricInstallerServiceName, timer.GetRemainingTime());
            StartAndValidateInstallerServiceCompletion(machineName, installerSvc, timer.GetRemainingTime());
        }

        internal static StandAloneCluster GetClusterResource(string jsonConfigPath, string clusterId)
        {
            SFDeployerTrace.WriteInfo(StringResources.Info_SFProcessingClusterConfig);
            StandAloneInstallerJsonModelBase jsonConfig = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(jsonConfigPath);
            if (jsonConfig == null)
            {
                throw new FabricValidationException(StringResources.Error_SFJsonConfigInvalid, FabricErrorCode.OperationCanceled);
            }

            var userConfig = jsonConfig.GetUserConfig();
            var clusterTopology = jsonConfig.GetClusterTopology();
            var adminConfig = new StandaloneAdminConfig();
            clusterId = string.IsNullOrEmpty(jsonConfig.GetClusterRegistrationId()) ? clusterId : jsonConfig.GetClusterRegistrationId();
            var logger = new StandAloneTraceLogger("StandAloneDeploymentManager");
            var clusterResource = new StandAloneCluster(adminConfig, userConfig, clusterTopology, clusterId, logger);
            bool generated = clusterResource.RunStateMachine();

            if (generated && jsonConfig is DevJsonModel)
            {
                DevJsonModel.TuneGeneratedManifest(clusterResource.Current.ExternalState.ClusterManifest, userConfig.FabricSettings);
            }

            if (!generated)
            {
                SFDeployerTrace.WriteError(StringResources.Error_CannotConvertJsonConfig);
                throw new InvalidOperationException(StringResources.Error_CannotConvertJsonConfig);
            }
            
            if (StandaloneUtility.IOTDevicesCount(clusterTopology.Machines) > 0)
            {
                var targetFabricSettings = StandaloneUtility.RemoveFabricSettingsSectionForIOTCore(clusterResource.Current.ExternalState.ClusterManifest);
                DevJsonModel.ApplyUserSettings(targetFabricSettings, userConfig.FabricSettings);
                clusterResource.Current.ExternalState.ClusterManifest.FabricSettings = targetFabricSettings.ToArray();
            }

            return clusterResource;
        }

        private static async Task BestEffortRollback(List<string> machines)
        {
            SFDeployerTrace.UpdateFileLocation(SFDeployerTrace.LastTraceDirectory); // Close handle on previous trace file
            SFDeployerTrace.WriteInfo(StringResources.Info_SFRollbackCleanup);
            await RemoveNodeConfigurationAsync(machines, false, FabricPackageType.XCopyPackage, true);
        }

        private static async Task<Node> FindFabricNodeForCurrentMachineAsync(FabricClient fabricClient)
        {
            var nodes = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                () => fabricClient.QueryManager.GetNodeListAsync()).ConfigureAwait(false);
            var node = nodes.FirstOrDefault(n => NetworkApiHelper.IsAddressForThisMachine(n.IpAddressOrFQDN));
            if (node == null)
            {
                throw new InvalidOperationException("Node is not found in the cluster");
            }

            return node;
        }

        private static void RunFabricServices(MachineHealthContainer machineHealthContainer, FabricPackageType fabricPackageType, TimeSpan timeout)
        {
            SFDeployerTrace.WriteInfo(StringResources.Info_SFServiceInstallation);
            if (fabricPackageType == FabricPackageType.XCopyPackage)
            {
                Parallel.ForEach(
                    machineHealthContainer.GetHealthyMachineNames(),
                    (string m) =>
                    {
                        try
                        {
                            StartAndWaitForInstallerService(m, timeout);
                        }
                        catch (Exception ex)
                        {
                            if (ex is InvalidOperationException || ex is System.ServiceProcess.TimeoutException || ex is FabricException)
                            {
                                if (machineHealthContainer.MaxPercentFailedNodes == 0)
                                {
                                    throw;
                                }

                                machineHealthContainer.MarkMachineAsUnhealthy(m);
                                SFDeployerTrace.WriteWarning(StringResources.Error_StartFabricInstallerFailed, m, ex);
                            }
                            else
                            {
                                throw;
                            }
                        }
                    });

                if (!machineHealthContainer.EnoughHealthyMachines())
                {
                    throw new FabricException(StringResources.Error_CreateClusterCancelledNotEnoughHealthy);
                }
            }
            else
            {
                /*MSI*/
                Parallel.ForEach(
                    machineHealthContainer.GetHealthyMachineNames(),
                    (string m) =>
                    {
                        StartFabricHostService(m, timeout);
                    });
            }
        }

        private static void StartFabricHostService(string machineName, TimeSpan timeout)
        {
            TimeoutHelper timer = new TimeoutHelper(timeout);
            ServiceController hostSvc = FabricDeployerServiceController.GetServiceInStoppedState(machineName, Constants.FabricHostServiceName, timer.GetRemainingTime());
            StartFabricHostService(machineName, hostSvc, timer.GetRemainingTime());
        }

        private static void StartAndValidateInstallerServiceCompletion(string machineName, ServiceController installerSvc, TimeSpan timeout)
        {
            TimeoutHelper timer = new TimeoutHelper(timeout);
            var installerSvcStartedTimeout = timeout == TimeSpan.MaxValue ? TimeSpan.FromMinutes(Constants.FabricInstallerServiceStartTimeoutInMinutes) : timer.GetRemainingTime();

            Task startInstallerTask = Task.Run(() =>
            {
                try
                {
                    installerSvc.WaitForStatus(ServiceControllerStatus.Running, installerSvcStartedTimeout);
                }
                catch (System.ServiceProcess.TimeoutException)
                {
                    string message = string.Format(StringResources.Error_SFInstallerServiceStartTimeout, machineName);
                    SFDeployerTrace.WriteError(message);
                    throw new System.ServiceProcess.TimeoutException(message);
                }

                SFDeployerTrace.WriteInfo(StringResources.Info_SFInstallerServiceStarted, machineName);
            });

            SFDeployerTrace.WriteNoise(StringResources.Info_SFInstallerServiceStarting, machineName);
            installerSvc.Start();
            startInstallerTask.Wait(installerSvcStartedTimeout); // Locks until service is started or reaches timeout

            var hostExistsTimeout = timeout == TimeSpan.MaxValue ? TimeSpan.FromMinutes(Constants.FabricInstallerServiceHostCreationTimeoutInMinutes) : timer.GetRemainingTime();
            try
            {
                installerSvc.WaitForStatus(ServiceControllerStatus.Stopped, hostExistsTimeout); // Waits for FabricInstallerSvc to complete
                SFDeployerTrace.WriteNoise(StringResources.Info_SFInstallerServiceCompleted, machineName);
            }
            catch (System.ServiceProcess.TimeoutException)
            {
                string errorMessage = string.Format(StringResources.Error_SFInstallerServiceTimeoutDetails, machineName);
                SFDeployerTrace.WriteError(errorMessage);
                throw new System.ServiceProcess.TimeoutException(errorMessage);
            }

            ServiceController hostSvc = FabricDeployerServiceController.GetService(Constants.FabricHostServiceName, machineName);
            if (hostSvc == null)
            {
                string message = string.Format(StringResources.Error_SFFabricHostNotInstalled, machineName);
                SFDeployerTrace.WriteError(message);
                throw new FabricServiceNotFoundException(message);
            }

            if (hostSvc.Status == ServiceControllerStatus.Stopped)
            {
                string message = string.Format(StringResources.Error_SFFabricHostNotRunning, machineName);
                SFDeployerTrace.WriteError(message);
                throw new FabricException(message);
            }

            var hostSvcTimeout = timeout == TimeSpan.MaxValue ? TimeSpan.FromMinutes(Constants.FabricHostAwaitRunningTimeoutInMinutes) : timer.GetRemainingTime();
            SFDeployerTrace.WriteNoise(StringResources.Info_SFWaitingFabricHostRunning, machineName);
            try
            {
                hostSvc.WaitForStatus(ServiceControllerStatus.Running, hostSvcTimeout);
            }
            catch (System.ServiceProcess.TimeoutException)
            {
                string message = string.Format(
                                     StringResources.Error_SFTimedOut,
                                     string.Format(StringResources.Info_SFWaitingFabricHostRunning, machineName));
                SFDeployerTrace.WriteError(message);
                throw new System.ServiceProcess.TimeoutException(message);
            }

            SFDeployerTrace.WriteInfo(StringResources.Info_SFFabricHostStarted, machineName);
        }

        private static void StartFabricHostService(string machineName, ServiceController hostSvc, TimeSpan timeout)
        {
            var hostRunningTimeout = timeout == TimeSpan.MaxValue ? TimeSpan.FromMinutes(Constants.FabricHostAwaitRunningTimeoutInMinutes) : timeout;

            Task startHostTask = Task.Run(() =>
            {
                hostSvc.WaitForStatus(ServiceControllerStatus.Running, hostRunningTimeout);
            });

            SFDeployerTrace.WriteInfo(StringResources.Info_SFFabricHostStarting, machineName);
            hostSvc.Start();
            startHostTask.Wait(hostRunningTimeout); // Locks until service is started or reaches timeout
            SFDeployerTrace.WriteInfo(StringResources.Info_SFFabricHostStarted, machineName);
        }

        private static void CopyInstallerToAllMachines(
            MachineHealthContainer machineHealthContainer,
            string sourcePackagePath,
            string packageDestinationPath,
            string installerServiceSourcePath)
        {
            IEnumerable<string> files = Directory.EnumerateFiles(installerServiceSourcePath, "*", SearchOption.TopDirectoryOnly);
            string installerServicePath = Path.Combine(packageDestinationPath, Constants.FabricInstallerServiceFolderName);

            Parallel.ForEach<string>(
                machineHealthContainer.GetHealthyMachineNames(),
                (string machineName) =>
                {
                    try
                    {
                        string destinationInstallerServicePath = Helpers.GetRemotePathIfNotLocalMachine(installerServicePath, machineName);
                        Directory.CreateDirectory(destinationInstallerServicePath);

                        if (!Directory.Exists(destinationInstallerServicePath))
                        {
                            string message = string.Format(StringResources.Error_SFCreateInstallerDirectoryFailure, machineName);
                            SFDeployerTrace.WriteError(message);
                            throw new DirectoryNotFoundException(message);
                        }

                        // Copy Installer service files
                        Parallel.ForEach<string>(
                            files,
                            (string file) =>
                            {
                                string destinationFilePath = Path.Combine(destinationInstallerServicePath, Path.GetFileName(file));
                                SFDeployerTrace.WriteNoise(StringResources.Info_SFCopyingInstallerFile, destinationFilePath);
                                using (FileStream sourceStream = File.Open(file, FileMode.Open, FileAccess.Read, FileShare.Read))
                                {
                                    using (FileStream destStream = File.Create(destinationFilePath))
                                    {
                                        sourceStream.CopyTo(destStream);
                                    }
                                }

                                if (!File.Exists(destinationFilePath))
                                {
                                    string message = string.Format(StringResources.Error_SFErrorCreatingFile, destinationFilePath, machineName);
                                    SFDeployerTrace.WriteError(message);
                                    throw new FileNotFoundException(message);
                                }
                            });
                    }
                    catch (Exception ex)
                    {
                        if (ex is DirectoryNotFoundException || ex is FileNotFoundException)
                        {
                            if (machineHealthContainer.MaxPercentFailedNodes == 0)
                            {
                                throw;
                            }

                            machineHealthContainer.MarkMachineAsUnhealthy(machineName);
                            SFDeployerTrace.WriteWarning(StringResources.Error_CopyFabricInstallerFailed, machineName, ex);
                        }
                        else
                        {
                            throw;
                        }
                    }
                });

            if (!machineHealthContainer.EnoughHealthyMachines())
            {
                throw new FabricException(StringResources.Error_CreateClusterCancelledNotEnoughHealthy);
            }
        }

        private static void ExtractInstallerService(string cabPath, string pathToExtract)
        {
            CabFileOperations.ExtractFiltered(cabPath, pathToExtract, new string[] { Constants.FabricInstallerServiceFolderName }, true);

            string installerSvcDirectory = Path.Combine(pathToExtract, Constants.FabricInstallerServiceFolderName);
            var expectedFiles = new string[] { "FabricCommon.dll", "FabricInstallerService.exe", "FabricResources.dll" };
            if (!Directory.Exists(installerSvcDirectory))
            {
                string message = string.Format(StringResources.Error_ExtractInstallerService, installerSvcDirectory);
                SFDeployerTrace.WriteError(message);
                throw new DirectoryNotFoundException(message);
            }

            foreach (string file in expectedFiles)
            {
                string filePath = Path.Combine(installerSvcDirectory, file);
                if (!File.Exists(filePath))
                {
                    string message = string.Format(StringResources.Error_ExtractInstallerService, filePath);
                    SFDeployerTrace.WriteError(message);
                    throw new FileNotFoundException(message);
                }
            }
        }

        private static void DeleteFabricDataRoot(StandAloneInstallerJsonModelBase config, string machineName, string dataRoot)
        {
            IEnumerable<string> machineNodes = config.Nodes.Where(n => n.IPAddress == machineName).Select(n => n.NodeName);
            foreach (string node in machineNodes)
            {
                string nodeDirectory;
                if (StandaloneUtility.DataRootNodeExists(machineName, node, dataRoot, out nodeDirectory))
                {
                    try
                    {
                        RetryActionHelper<object>(
                                  () =>
                                  {
                                      FabricDirectory.Delete(nodeDirectory, true, true);
                                      return null;
                                  },
                                  Constants.DefaultRetryCount);
                    }
                    catch (Exception ex)
                    {
                        SFDeployerTrace.WriteNoise(StringResources.Info_SFDeleteFabricDataRootFailed, ex.ToString(), machineName);
                        throw ex;
                    }
                }
            }
        }

        private static T RetryActionHelper<T>(Func<T> action, int retryCount)
        {
            var exceptions = new List<Exception>();
            for (int retry = 0; retry < retryCount; retry++)
            {
                try
                {
                    return action();
                }
                catch (Exception ex)
                {
                    exceptions.Add(ex);
                }
            }

            throw new AggregateException(exceptions);
        }

        private static void DeleteFabricPackage(string machineName, string machinePackageRoot)
        {
            if (!string.IsNullOrEmpty(machineName) && !string.IsNullOrEmpty(machinePackageRoot))
            {
                string packageRoot = Helpers.GetRemotePathIfNotLocalMachine(machinePackageRoot, machineName);
                SFDeployerTrace.WriteNoise(StringResources.Info_SFDeletingFabricPackage, packageRoot, machineName);
                try
                {
                    FabricDirectory.Delete(packageRoot, true, true);
                }
                catch (Exception ex)
                {
                    SFDeployerTrace.WriteNoise(StringResources.Info_SFDeleteFabricPackageFailed, ex.ToString(), machineName);
                }

                SFDeployerTrace.WriteNoise(StringResources.Info_SFDeleteFabricPackageCompleted, packageRoot, machineName);
            }
        }

        private static void Initialize()
        {
            AppDomain.CurrentDomain.AssemblyResolve += new ResolveEventHandler(LoadFromFabricCodePath);

            string tracingPath = Path.Combine(Directory.GetCurrentDirectory(), Constants.FabricDeploymentTracesDirectory);
            SFDeployerTrace.UpdateFileLocation(tracingPath);
        }

        private static void Destroy()
        {
            SFDeployerTrace.CloseHandle();

            AppDomain.CurrentDomain.AssemblyResolve -= new ResolveEventHandler(LoadFromFabricCodePath);
        }

        private static Assembly LoadFromFabricCodePath(object sender, ResolveEventArgs args)
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

        private static void CopyClusterResourceToAllMachines(StandAloneCluster clusterResource, string fabricDataRoot, MachineHealthContainer machineHealthContainer)
        {
            string clusterResourceString = JsonConvert.SerializeObject(
                   clusterResource,
                   StandaloneUtility.GetStandAloneClusterSerializerSettings());

            string destinationFilePath = Path.Combine(fabricDataRoot, Constants.BaselineJsonMetadataFileName);

            Parallel.ForEach<string>(
                    machineHealthContainer.GetHealthyMachineNames(),
                    (string machineName) =>
                    {
                        try
                        {
                            string remoteFabricDataRoot = Helpers.GetRemotePathIfNotLocalMachine(fabricDataRoot, machineName);
                            if (!Directory.Exists(remoteFabricDataRoot))
                            {
                                Directory.CreateDirectory(remoteFabricDataRoot);
                            }

                            string remoteDestinationFilePath = Helpers.GetRemotePathIfNotLocalMachine(destinationFilePath, machineName);

                            using (MemoryStream sourceStream = new MemoryStream(Encoding.UTF8.GetBytes(clusterResourceString)))
                            {
                                using (FileStream destStream = File.Create(remoteDestinationFilePath))
                                {
                                    sourceStream.CopyTo(destStream);
                                }
                            }

                            if (!File.Exists(remoteDestinationFilePath))
                            {
                                string message = string.Format(StringResources.Error_SFErrorCreatingFile, remoteDestinationFilePath, machineName);
                                SFDeployerTrace.WriteError(message);
                                throw new FileNotFoundException(message);
                            }
                        }
                        catch (Exception ex)
                        {
                            if (machineHealthContainer.MaxPercentFailedNodes == 0)
                            {
                                throw;
                            }

                            machineHealthContainer.MarkMachineAsUnhealthy(machineName);
                            SFDeployerTrace.WriteWarning(StringResources.Error_CopyClusterResourceFailed, machineName, ex);
                        }
                    });

            if (!machineHealthContainer.EnoughHealthyMachines())
            {
                throw new FabricException(StringResources.Error_CreateClusterCancelledNotEnoughHealthy);
            }
        }

        private static bool IsMinQuorumSeedNodeHealthy(MachineHealthContainer machineHealthContainer, string clusterManifestPath)
        {
            var clusterInfrastructure = StandaloneUtility.GetInfrastructureWindowsServerFromClusterManifest(clusterManifestPath);
            int seedNodeCount = 0;
            int healthySeedNodeCount = 0;
            foreach (FabricNodeType node in clusterInfrastructure.NodeList)
            {
                if (node.IsSeedNode)
                {
                    seedNodeCount++;
                    if (machineHealthContainer.IsMachineHealthy(node.IPAddressOrFQDN))
                    {
                        healthySeedNodeCount++;
                    }
                }
            }

            if (healthySeedNodeCount <= (seedNodeCount / 2))
            {
                SFDeployerTrace.WriteError(StringResources.Error_MinQuorumSeedNodeWasNotEstablished, healthySeedNodeCount, seedNodeCount);
                return false;
            }

            return true;
        }
    }
}
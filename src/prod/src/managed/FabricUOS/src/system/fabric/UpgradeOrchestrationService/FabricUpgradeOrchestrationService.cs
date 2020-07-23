// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeOrchestration.Service
{
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Strings;
    using System.Fabric.UpgradeService;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Microsoft.ServiceFabric.DeploymentManager.Model;
    using Microsoft.ServiceFabric.Services.Communication.Runtime;
    using Microsoft.ServiceFabric.Services.Runtime;
    using Newtonsoft.Json;

    using FabricNativeConfigStore = System.Fabric.Common.NativeConfigStore;

    public class FabricUpgradeOrchestrationService : StatefulService
    {
        internal const string TraceType = "UpgradeOrchestration.Service";

        private static readonly FabricErrorCode[] AllowedStartFabricUpgradeErrorCodes =
        {
            FabricErrorCode.FabricUpgradeInProgress,
            FabricErrorCode.FabricAlreadyInTargetVersion,
        };

        private readonly string endpoint;
        private readonly ClusterEndpointSecuritySettingsChangeNotifier clusterEndpointSecuritySettingsChangeNotifier;
        private readonly bool enableEndpointV2;

        private StoreManager storeManager;
        private UpgradeOrchestrationServiceImpl uosImpl;
        private UpgradeOrchestrationMessageProcessor messageProcessor;
        private FabricClient fabricClient;

        internal FabricUpgradeOrchestrationService(StatefulServiceContext serviceContext, string endpoint, bool enableEndpointV2)
            : base(
                serviceContext,
                new ReliableStateManager(
                    serviceContext,
                    new ReliableStateManagerConfiguration(
                        GetReplicatorSettings(endpoint, enableEndpointV2))))
        {
            this.endpoint = endpoint;
            this.enableEndpointV2 = enableEndpointV2;
            this.fabricClient = new FabricClient();
            this.storeManager = new StoreManager(this.StateManager);
            this.Orchestrator = new UpgradeOrchestrator(this.storeManager, this, this.fabricClient);

            this.messageProcessor = new UpgradeOrchestrationMessageProcessor(
                this.storeManager,
                this.fabricClient,
                this.Orchestrator,
                new CancellationToken(canceled: false));

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "FabricUpgradeOrchestrationService constructor: enableEndpointV2 is {0}", this.enableEndpointV2);

            if (this.enableEndpointV2)
            {
                this.clusterEndpointSecuritySettingsChangeNotifier = new ClusterEndpointSecuritySettingsChangeNotifier(this.endpoint, this.UpdateReplicatorSettings);
            }

            AppDomain.CurrentDomain.AssemblyResolve += new ResolveEventHandler(OnResolveAssemblyFailed);
        }

        internal UpgradeOrchestrator Orchestrator
        {
            get;
            private set;
        }

        internal static Assembly OnResolveAssemblyFailed(object sender, ResolveEventArgs args)
        {
            AssemblyName name = new AssemblyName(args.Name);
            Assembly result = null;
            Constants.AssemblyResolveTable.TryGetValue(name.Name, out result);
            return result;
        }

        internal static StandAloneCluster ConstructClusterFromJson(StandAloneInstallerJsonModelBase jsonModel, FabricNativeConfigStore configStore)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Creating userconfig, cluster topology, adminconfig.");

            var userConfig = jsonModel.GetUserConfig();
            var clusterTopology = jsonModel.GetClusterTopology();
            var adminConfig = new StandaloneAdminConfig();
            var logger = new StandAloneTraceLogger("StandAloneDeploymentManager");

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Creating new StandAlone cluster resource.");

            var clusterId = configStore.ReadUnencryptedString(Constants.SectionName, Constants.ClusterIdParameterName);
            var clusterResource = new StandAloneCluster(adminConfig, userConfig, clusterTopology, clusterId, logger);
            
            return clusterResource;
        }

        internal IStatefulServicePartition GetPartition()
        {
            return this.Partition;
        }

        protected override async Task<bool> OnDataLossAsync(RestoreContext restoreContext, CancellationToken cancellationToken)
        {
            await DatalossHelper.UpdateHeathStateAsync(isHealthy: false).ConfigureAwait(false);
            UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "Data loss happens. Manual cluster config upgrade is required to fix.");
                       
            return false;
        }

        protected override IEnumerable<ServiceReplicaListener> CreateServiceReplicaListeners()
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Enter CreateServiceReplicaListeners");
            return new[] { new ServiceReplicaListener(i => this.CreateCommunicationListener(i)) };
        }

        protected override async Task RunAsync(CancellationToken cancellationToken)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Enter RunAsync");

            try
            {
                var configStore = FabricNativeConfigStore.FabricGetConfigStore();

                StandAloneFabricSettingsActivator.InitializeConfigStore(configStore);

                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "After InitializeAsync");

                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Getting cluster resource from store manager.");
                StandAloneCluster cluster = await this.storeManager.GetClusterResourceAsync(Constants.ClusterReliableDictionaryKey, cancellationToken).ConfigureAwait(false);

                bool isInDatalossState = await DatalossHelper.IsInDatalossStateAsync(cancellationToken).ConfigureAwait(false);
                if (!isInDatalossState)
                {
                    bool isBaselineUpgrade = false;
                    if (cluster == null)
                    {
                        // Cluster resource does not exist, e.g. first time baseline upgrade.
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Cluster resource does not exist in store manager. Initiating new clsuter resource.");

                        cluster = FabricUpgradeOrchestrationService.InitClusterResource(configStore);
                        ReleaseAssert.AssertIf(cluster == null, "Cluster Resource cannot be initialized.");
                        isBaselineUpgrade = true;

                        await this.CompleteAndPersistBaselineStateAsync(cluster, cancellationToken).ConfigureAwait(false);
                    }

                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Setting a valid cancellation token for UpgradeOrchestrationMessageProcessor");
                    this.messageProcessor.CancellationToken = cancellationToken;

                    /* In version 5.7, we added Iron as a new entry to ReliabilityLevel enum. This entry was added as ReliabilityLevel = 1 and hence, it moved all the existing levels one level down.
                     * For clusters created in <=5.6, when upgraded to 5.7+ the ReliabilityLevel field would be interpreted differently. The below code fixes this issue by reading the
                     * UOS state and comparing it against the actual cluster state. If there is a mismatch it sets the UOS to the correct ReliabilityLevel.
                    */
                    if (!isBaselineUpgrade &&
                        cluster.Current != null &&
                        cluster.Current.CSMConfig != null)
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Current ReliabilityLevel set in UOS state {0}", cluster.Current.CSMConfig.ReliabilityLevel);
                        var actualReliabilityLevelForCluster = await this.GetActualReliabilityLevelForCluster(cancellationToken).ConfigureAwait(false);
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Actual ReliabilityLevel set for the cluster {0}", actualReliabilityLevelForCluster);
                        if (actualReliabilityLevelForCluster != cluster.Current.CSMConfig.ReliabilityLevel)
                        {
                            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "UOS ReliabilityLevel is inconsistent with actual reliability level for the cluster.. Setting UOS state to {0}", actualReliabilityLevelForCluster);
                            cluster.Current.CSMConfig.ReliabilityLevel = actualReliabilityLevelForCluster;
                            await this.storeManager.PersistClusterResourceAsync(Constants.ClusterReliableDictionaryKey, cluster, cancellationToken).ConfigureAwait(false);
                            cluster = await this.storeManager.GetClusterResourceAsync(Constants.ClusterReliableDictionaryKey, cancellationToken).ConfigureAwait(false);
                            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "UOS ReliabilityLevel set to {0}", cluster.Current.CSMConfig.ReliabilityLevel);
                        }
                    }
                    
                    /* This is a workaround till actual admin config upgrades are implemented. In 5.7 we changed some properties in ClusterSettings
                       but those will not take effect for clusters created with version < 5.7 since no upgrade reads those settings. 
                       This workaround initiates a WRP config upgrade in the form of SimpleClusterUpgradeState if the settings found are old (after the code upgrade completes to this version). */
                    string isAdminConfigUpgradeAttempted = await this.storeManager.GetStorageObjectAsync(Constants.AdminConfigUpgradeAttemptedDictionaryKey, cancellationToken).ConfigureAwait(false);
                    if (!isBaselineUpgrade && string.IsNullOrEmpty(isAdminConfigUpgradeAttempted) && this.IsFirewallRuleDisabled())
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Old admin configuration settings detected.. Will initiate admin config upgrade after code upgrade completes..");
                        bool isCurrentCodeUpgradeCompleted = false;
                        while (!isCurrentCodeUpgradeCompleted)
                        {
                            isCurrentCodeUpgradeCompleted = await this.CheckCodeUpgradeCompletedAsync(cancellationToken).ConfigureAwait(false);
                            await Task.Delay(TimeSpan.FromSeconds(10), cancellationToken).ConfigureAwait(false);
                        }

                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Setting targetWRPConfig to initiate admin config upgrade.");
                        var adminConfig = new StandaloneAdminConfig();
                        adminConfig.Version.ClusterSettingsVersion = "2.1";
                        cluster.TargetWrpConfig = adminConfig;
                        await this.storeManager.SetStorageObjectAsync(Constants.AdminConfigUpgradeAttemptedDictionaryKey, "true", cancellationToken).ConfigureAwait(false);
                    }

                    if (!isBaselineUpgrade)
                    {
                        // If the cluster manifest versions don't match, send a health warning for users to know.
                        Task manifestCheck = this.SetupClusterManifestVersionCheck(cancellationToken, cluster.Current.ExternalState.ClusterManifest.Version);

                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Invoking Orchestrator.StartUpgradeAsync");
                        Task t = this.Orchestrator.StartUpgradeAsync(cluster, cancellationToken, new ConfigurationUpgradeDescription());
                    }

                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Done with Orchestrator.StartUpgradeAsync");
                }

                var goalStateProvisioner = new StandaloneGoalStateProvisioner(this.storeManager, this.Orchestrator, cancellationToken);
                Task goalStatePollTask = (goalStateProvisioner.IsAutoupgradeEnabled() || goalStateProvisioner.IsAutoupgradeInstallEnabled())
                    ? goalStateProvisioner.SetUpGoalStatePoll(cancellationToken, this.IsSkipInitialGoalStateCheck())
                    : Task.Run(
                    () =>
                    {
                        goalStateProvisioner.EmitGoalStateReachableHealth(this.fabricClient, true /*success*/);
                        goalStateProvisioner.EmitClusterVersionSupportedHealth(this.fabricClient, true /*success*/);
                    },
                    cancellationToken);

                await Task.Delay(Timeout.Infinite, cancellationToken).ConfigureAwait(false);
            }
            catch (FabricNotPrimaryException ex)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteError(TraceType, ex.ToString());
            }
            catch (FabricObjectClosedException ocex)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteError(TraceType, ocex.ToString());
            }

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exit RunAsync");
        }

        private static StandAloneCluster InitClusterResource(FabricNativeConfigStore configStore)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Enter InitClusterResource");

            string fabricDataRoot = FabricEnvironment.GetDataRoot();
            string jsonConfigPath = Path.Combine(fabricDataRoot, Microsoft.ServiceFabric.DeploymentManager.Constants.BaselineJsonMetadataFileName);
            ReleaseAssert.AssertIf(!File.Exists(jsonConfigPath), "Baseline upgrade JsonClusterConfigMetadata not found at {0}.", jsonConfigPath);

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Deserializing Json config.");
            string jsonConfig = File.ReadAllText(jsonConfigPath);

            if (string.IsNullOrEmpty(jsonConfig))
            {
                throw new FabricValidationException(StringResources.Error_SFJsonConfigInvalid, FabricErrorCode.OperationCanceled);
            }

            JsonSerializerSettings settings = StandaloneUtility.GetStandAloneClusterDeserializerSettings();

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exit InitClusterResource");
            return JsonConvert.DeserializeObject<StandAloneCluster>(
                jsonConfig,
                settings);
        }

        private static ReliableStateManagerReplicatorSettings GetReplicatorSettings(string endpoint, bool enableEndpointV2)
        {
            SecurityCredentials securityCredentials = null;
            if (enableEndpointV2)
            {
                securityCredentials = SecurityCredentials.LoadClusterSettings();
            }

            ReliableStateManagerReplicatorSettings replicatorSettings = new ReliableStateManagerReplicatorSettings()
            {
                ReplicatorAddress = endpoint,
                SecurityCredentials = securityCredentials
            };

            return replicatorSettings;
        }

        private async Task SetupClusterManifestVersionCheck(CancellationToken cancellationToken, string clusterManifestVersion)
        {
            try
            {
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Starting SetupClusterManifestVersionCheck");
                while (!cancellationToken.IsCancellationRequested)
                {
                    try
                    {
                        await this.CheckClusterManifestVersion(clusterManifestVersion, cancellationToken).ConfigureAwait(false);
                    }
                    catch (FabricException ex)
                    {
                        // don't exit the loop, the next trial with reattempt it.
                        UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "CheckClusterManifestVersion failed with exception: {0}", ex.ToString());
                    }
                    
                    await Task.Delay(new TimeSpan(24, 0, 0), cancellationToken).ConfigureAwait(false);
                }
            }
            catch (OperationCanceledException ex)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "Cancellation token was requested. Exception thrown : {0}", ex.ToString());
                throw;
            }
            catch (Exception ex)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "SetupClusterManifestVersionCheck failed with {0}", ex.ToString());
                Debug.Assert(false, string.Format("SetupClusterManifestVersionCheck failed with {0}", ex.ToString()));
            }
        }

        private async Task CheckClusterManifestVersion(string clusterManifestVersion, CancellationToken cancellationToken)
        {
            // If an upgrade is still in progress, don't do anything
            FabricUpgradeProgress upgradeProgress = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync<FabricUpgradeProgress>(
                    () =>
                    this.fabricClient.ClusterManager.GetFabricUpgradeProgressAsync(FabricClient.DefaultTimeout, cancellationToken)).ConfigureAwait(false);

            if (upgradeProgress == null || !FabricClientWrapper.IsUpgradeCompleted(upgradeProgress.UpgradeState))
            {
                return;
            }

            string clusterManifestContent = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                    () => this.fabricClient.ClusterManager.GetClusterManifestAsync(FabricClient.DefaultTimeout, cancellationToken)).ConfigureAwait(false);
            var clusterManifest = XMLHelper.ReadClusterManifestFromContent(clusterManifestContent);
            var actualManifestVersion = clusterManifest.Version;

            if (!actualManifestVersion.Equals(clusterManifestVersion, StringComparison.OrdinalIgnoreCase))
            {
                this.fabricClient.HealthManager.ReportHealth(new ClusterHealthReport(new HealthInformation(
                    Constants.UpgradeOrchestrationHealthSourceId,
                    Constants.ManifestVersionMismatchHealthProperty,
                    HealthState.Warning)
                {
                    Description = string.Format(
                    StringResources.WarningHealth_ClusterManifestVersionMismatch, actualManifestVersion, clusterManifestVersion),
                    TimeToLive = TimeSpan.FromHours(48),
                    RemoveWhenExpired = true
                }));
            }
            else
            {
                this.fabricClient.HealthManager.ReportHealth(new ClusterHealthReport(new HealthInformation(
                    Constants.UpgradeOrchestrationHealthSourceId,
                    Constants.ManifestVersionMismatchHealthProperty,
                    HealthState.Ok)
                {
                    TimeToLive = TimeSpan.FromMilliseconds(1),
                    RemoveWhenExpired = true
                }));
            }
        }

        private bool IsSkipInitialGoalStateCheck()
        {
            NativeConfigStore configStore = NativeConfigStore.FabricGetConfigStore();
            return configStore.ReadUnencryptedBool(Constants.SectionName, Constants.SkipInitialGoalStateCheckProperty, UpgradeOrchestrationTrace.TraceSource, TraceType, false /*throwIfInvalid*/);
        }

        private ICommunicationListener CreateCommunicationListener(StatefulServiceContext serviceContext)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Enter CreateCommunicationListener, serviceInitializationParameters is not null {0}", serviceContext == null ? false : true);
            if (serviceContext != null)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(
                    TraceType,
                    "Enter CreateCommunicationListener, partition id: {0}, service name: {1}",
                    serviceContext.PartitionId,
                    serviceContext.ServiceName);
            }

            this.uosImpl = new UpgradeOrchestrationServiceImpl(this.messageProcessor);
            var listener = new UOSCommunicationListener(serviceContext, this.uosImpl);
            return listener;
        }

        private void UpdateReplicatorSettings(string endpoint)
        {
            ReliableStateManagerReplicatorSettings replicatorSettings = GetReplicatorSettings(endpoint, true);
            IStateProviderReplica stateProviderReplica = ((ReliableStateManager)this.StateManager).Replica;
            Microsoft.ServiceFabric.Data.ReliableStateManagerImpl impl = (Microsoft.ServiceFabric.Data.ReliableStateManagerImpl)stateProviderReplica;
            impl.Replicator.LoggingReplicator.UpdateReplicatorSettings(replicatorSettings);

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Updated replicator settings");
        }

        private async Task<ReliabilityLevel> GetActualReliabilityLevelForCluster(CancellationToken cancellationToken)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Entering GetCurrentReliabilityLevelForCluster");
            System.Fabric.Query.NodeList nodes = await StandaloneUtility.GetNodesFromFMAsync(this.fabricClient, cancellationToken).ConfigureAwait(false);
            int seedNodeCount = nodes.Count(p => p.IsSeedNode);

            ReliabilityLevel actualReliabilityLevelInCluster = ReliabilityLevelExtension.GetReliabilityLevel(seedNodeCount);
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "SeedNodeCount from cluster {0}, actualReliabilityLevelInCluster {1}", seedNodeCount, actualReliabilityLevelInCluster);

            return actualReliabilityLevelInCluster;
        }

        private bool IsFirewallRuleDisabled()
        {
            NativeConfigStore configStore = NativeConfigStore.FabricGetConfigStore();
            return configStore.ReadUnencryptedBool(Constants.SecuritySectionName, Constants.DisableFirewallRuleForPublicProfilePropertyName, UpgradeOrchestrationTrace.TraceSource, TraceType, false /*throwIfInvalid*/);
        }

        private async Task<bool> CheckCodeUpgradeCompletedAsync(CancellationToken cancellationToken)
        {
            var commandProcessor = new FabricClientWrapper();
            var upgradeProgress = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () =>
                        commandProcessor.GetFabricUpgradeProgressAsync(
                        Constants.UpgradeServiceMaxOperationTimeout,
                        cancellationToken),
                        Constants.UpgradeServiceMaxOperationTimeout,
                        cancellationToken).ConfigureAwait(false);

            var upgradeState = upgradeProgress.UpgradeState;
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Monitoring code upgrade status..Upgrade State: {0}, target code version: {1}, target config version: {2}", upgradeState, upgradeProgress.TargetCodeVersion, upgradeProgress.TargetConfigVersion);

            return upgradeState == FabricUpgradeState.RollingForwardCompleted;
        }

        private async Task CompleteAndPersistBaselineStateAsync(StandAloneCluster cluster, CancellationToken cancellationToken)
        {
            /* For standalone scenarios, baseline upgrade is performed automatically by CM when cluster bootstraps.
                         * No need to perform an upgrade again. Persist state and return. */
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Baseline upgrade detected. Skipping actual upgrade and simply persisting state.");

            cluster.ClusterUpgradeCompleted();
            ReleaseAssert.AssertIfNot(cluster.Pending == null, "cluster.Pending is not null after baseline upgrade completed. Investigate why cluster.Pending is not null.");
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "State machine reports no pending upgrade. Status is reset.");
            cluster.Reset();

            await this.storeManager.PersistClusterResourceAsync(Constants.ClusterReliableDictionaryKey, cluster, cancellationToken)
                              .ConfigureAwait(false);
        }
    }
}
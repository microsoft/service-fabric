// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.ClusterAnalysis
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.FaultAnalysis.Service.ClusterAnalysis.Configuration;
    using System.Fabric.FaultAnalysis.Service.ClusterAnalysis.Services;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Xml;
    using System.Xml.Serialization;
    using Cluster.ClusterQuery;
    using global::ClusterAnalysis.AnalysisCore.Insights;
    using global::ClusterAnalysis.AnalysisCore.Insights.Agents;
    using global::ClusterAnalysis.Common;
    using global::ClusterAnalysis.Common.Config;
    using global::ClusterAnalysis.Common.Log;
    using global::ClusterAnalysis.Common.Runtime;
    using global::ClusterAnalysis.Common.Store;
    using global::ClusterAnalysis.TraceAccessLayer.StoreConnection;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Services.Client;
    using Newtonsoft.Json;

    internal class ClusterAnalysisApiHandler
    {
        // DO NOT CHANGE THESE CONSTANTS AS THEY ARE USED FOR RD. THINGS WILL BREAK IF YOU DO IT.
        private const string ClusterAnalysisApiStateStoreIdentifier = "ClusterAnalysisApiStateStore";
        private const string ClusterAnalsisApiStateStoreCurrentStateKey = "ClusterAnalsisApiStateStoreCurrentStateKey";
        private const string ClusterAnalysisApiStateStoreLaunchConfigKey = "ClusterAnalysisApiStateStoreLaunchConfigKey";

        private const string ClusterAnalysisTraceIdentifier = "ClusterAnalysisApiHandler";

        private static readonly TimeSpan DelayBetweenRelaunch = TimeSpan.FromMinutes(1);

        private readonly JsonSerializerSettings serializationSettings;

        private readonly FabricClient fabricClient;

        private readonly IReliableStateManager stateManager;

        private readonly SemaphoreSlim startStopApiSingleAccessLock;

        private readonly IPersistentStore<string, string> clusterAnalyzerApiStateStore;

        private readonly ILogger logger;

        private ClusterAnalysisRunner runner;

        private CancellationToken originalCancellationToken;

        private CancellationTokenSource stopCancellationTokenSource;

        public ClusterAnalysisApiHandler(IReliableStateManager reliableStateManager, FabricClient client, CancellationToken token)
        {
            this.stateManager = reliableStateManager;
            this.fabricClient = client;
            this.serializationSettings = new JsonSerializerSettings { TypeNameHandling = TypeNameHandling.All };
            this.originalCancellationToken = token;
            this.startStopApiSingleAccessLock = new SemaphoreSlim(1);
            this.clusterAnalyzerApiStateStore = PersistentStoreProvider.GetStateProvider(this.stateManager)
                .CreatePersistentStoreForStringsAsync(ClusterAnalysisApiStateStoreIdentifier, AgeBasedRetentionPolicy.Forever, token).GetAwaiter().GetResult();

            this.logger = ClusterAnalysisLogProvider.LogProvider.CreateLoggerInstance(ClusterAnalysisTraceIdentifier);
        }

        /// <summary>
        /// Start Cluster Analysis
        /// </summary>
        /// <remarks>
        /// This abstraction get Invoked when User calls the rest API to Start Analysis.
        /// </remarks>
        /// <param name="configuration">The start configuration</param>
        /// <param name="token"></param>
        /// <returns></returns>
        public Task StartClusterAnalysisAsync(ClusterAnalysisConfiguration configuration, CancellationToken token)
        {
            ReleaseAssert.AssertIfNull(configuration, "configuration");

            // Please note - since this is a user call, the end user call parameter value is true.
            return this.StartClusterAnalysisInternalAsync(endUserCall: true, configuration: configuration, token: token);
        }

        /// <summary>
        /// Stop Cluster Analysis
        /// </summary>
        /// <remarks>
        /// This abstraction get Invoked when User calls the rest API to Stop Analysis.
        /// </remarks>
        /// <param name="stopOptions"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        public Task StopClusterInsightAsync(StopOptions stopOptions, CancellationToken token)
        {
            // This is a user facing API, so we call stop with the mention that its an end user call with an intent to stop.
            return this.StopClusterInsightAsync(endUserCall: true, stopOptions: stopOptions, token: token);
        }

        /// <summary>
        /// This is the internal launch point which FAS will call into. Depending on the situation,
        /// this function will decide if CI needs to be enabled, and if yes, will start it. 
        /// </summary>
        /// <param name="token"></param>
        /// <returns></returns>
        internal async Task InternalLaunchAsync(CancellationToken token)
        {
            if (!(await this.IsCurrentDeploymentOneboxAsync(token).ConfigureAwait(false)))
            {
                this.logger.LogMessage("Cluster Analysis can only be Enabled in One box Environment. Skipping Cluster Analysis Launch");
                return;
            }

            // Get current status.
            var persistedStatus = await this.GetCurrentStatusAsync(token).ConfigureAwait(false);
            if (persistedStatus.UserDesiredState == FeatureState.Stopped)
            {
                this.logger.LogMessage("User Desired State is Stopped. Skipping Cluster Analysis Launch");
                return;
            }

            // Get the config from our store.
            var persistedConfig = await this.GetCurrentConfigurationAsync(token).ConfigureAwait(false);
            ReleaseAssert.AssertIf(persistedStatus.UserDesiredState == FeatureState.Running && persistedConfig == null, "Invalid State. When Desired State == Running, Config can't be null");

            if (persistedConfig == null)
            {
                this.logger.LogMessage("No Stored Config. Using Auto start default configs to Launch CI");
                persistedConfig = this.GetAutoStartLaunchConfig();
            }

            this.logger.LogMessage("Launching Cluster Analysis with Config: {0}", persistedConfig);
            await this.StartClusterAnalysisInternalAsync(endUserCall: false, configuration: persistedConfig, token: token).ConfigureAwait(false);
        }

        private async Task StartClusterAnalysisInternalAsync(bool endUserCall, ClusterAnalysisConfiguration configuration, CancellationToken token)
        {
            await this.startStopApiSingleAccessLock.WaitAsync(token).ConfigureAwait(false);

            try
            {
                var currentState = await this.GetCurrentStatusAsync(token).ConfigureAwait(false);

                // We skip the current state check if it is "not" an end user call. This is to cover scenario like process crash
                // where internal data structure may not have been updated to reflect stopped status and the new primary calls
                // this start again. TODO: Think of better of supporting this scenario.
                if (endUserCall && currentState.CurrentState == FeatureState.Running)
                {
                    this.logger.LogMessage("Analysis is already Running. Please Call Stop and then start.");
                    throw new Exception("TODO ::: Analysis is already Running. Please Call Stop and then start");
                }

                // Create a linked token source.
                this.stopCancellationTokenSource = CancellationTokenSource.CreateLinkedTokenSource(token);

                var runtime = await this.CreateRunTimeAsync(configuration, this.stopCancellationTokenSource.Token).ConfigureAwait(false);
                this.runner = new ClusterAnalysisRunner(runtime.GetLogProvider());
                await this.runner.StartClusterAnalysisAsync(
                    runtime,
                    new List<IAnalysisConsumer> { new TraceAnalysisConsumer() },
                    this.stopCancellationTokenSource.Token);

                // Persist the current Configuration.
                await this.PersistCurrentConfigurationAsync(configuration, token);

                // If this was called by end user, we update the user intention.
                if (endUserCall)
                {
                    currentState.UserDesiredState = FeatureState.Running;
                }

                currentState.CurrentState = FeatureState.Running;
                await this.PersisteCurrentStatusAsync(currentState, token);
            }
            catch (Exception exp)
            {
                // Currently, for Preview, We swallow the Exception. This ensures we don't crash FAS.
                this.logger.LogError("Cluster Analysis Launch Failed. Exception Encountered : {0}", exp.ToString());
            }
            finally
            {
                this.startStopApiSingleAccessLock.Release();
            }
        }

        private async Task StopClusterInsightAsync(bool endUserCall, StopOptions stopOptions, CancellationToken token)
        {
            await this.startStopApiSingleAccessLock.WaitAsync(token).ConfigureAwait(false);

            try
            {
                var currentStatus = await this.GetCurrentStatusAsync(token).ConfigureAwait(false);
                if (currentStatus == AnalysisFeatureStatus.Default)
                {
                    this.logger.LogMessage("CI Was never Running.");

                    // TODO: What to do in this case, is throwing an exception the right way to respond to user API?
                    // Maybe throw strongly typed exception which upper layer can catch and return appropriate response to user?
                    throw new Exception("CI Service was never enabled");
                }

                if (currentStatus.CurrentState == FeatureState.Stopped)
                {
                    this.logger.LogMessage("Cluster Insight is currently Stopped");
                    return;
                }

                await this.runner.StopClusterAnalysisAsync(stopOptions, token).ConfigureAwait(false);

                // If this was called by end user, we update the user intention.
                if (endUserCall)
                {
                    currentStatus.UserDesiredState = FeatureState.Stopped;
                }

                // Update the current Status of the CI Service.
                currentStatus.CurrentState = FeatureState.Stopped;
                await this.PersisteCurrentStatusAsync(currentStatus, token).ConfigureAwait(false);
            }
            catch (OperationCanceledException)
            {
            }
            catch (Exception exp)
            {
                // Currently, for Preview, We swallow the Exception. This ensures we don't crash FAS.
                this.logger.LogError("Failed to Stop Cluster Analysis. Exception Encountered: {0}", exp);
            }
            finally
            {
                this.logger.LogMessage("Running Finalizer");

                this.stopCancellationTokenSource.Cancel();

                this.stopCancellationTokenSource = null;

                this.startStopApiSingleAccessLock.Release();
            }
        }

        private ClusterAnalysisConfiguration GetAutoStartLaunchConfig()
        {
            return new ClusterAnalysisConfiguration
            {
                AgentConfiguration = new List<AnalysisAgentConfiguration>()
            };
        }

        private async Task<IInsightRuntime> CreateRunTimeAsync(ClusterAnalysisConfiguration configuration, CancellationToken token)
        {
            this.logger.LogMessage("CreateRunTimeAsync:: Entering");

            IInsightRuntime runtime;
            var isOneBox = await this.IsCurrentDeploymentOneboxAsync(token).ConfigureAwait(false);
            if (isOneBox)
            {
                var config = this.CreateOneBoxConfig(configuration);
                this.logger.LogMessage("OneBoxConfig: {0}", config);

                runtime = DefaultInsightRuntime.GetInstance(
                    ClusterAnalysisLogProvider.LogProvider,
                    PersistentStoreProvider.GetStateProvider(this.stateManager),
                    config,
                    new PerformanceSessionManager(ClusterAnalysisLogProvider.LogProvider, token),
                    new TaskRunner(ClusterAnalysisLogProvider.LogProvider, this.OnUnhandledExceptionInAnalysisAsync),
                    token);

                this.AddServiceToRuntime(runtime, typeof(FabricClient), this.fabricClient);
                this.AddServiceToRuntime(runtime, typeof(IClusterQuery), ClusterQuery.CreateClusterQueryInstance(runtime));
                this.AddServiceToRuntime(runtime, typeof(IResolveServiceEndpoint), new ResolveServiceEndpoint(ServicePartitionResolver.GetDefault()));

                var localStoreConnection = new LocalTraceStoreConnectionInformation(
                    runtime.GetCurrentConfig().RuntimeContext.FabricDataRoot,
                    runtime.GetCurrentConfig().RuntimeContext.FabricLogRoot,
                    runtime.GetCurrentConfig().RuntimeContext.FabricCodePath);

                this.AddServiceToRuntime(
                    runtime,
                    typeof(TraceStoreConnectionInformation),
                    localStoreConnection);
            }
            else
            {
                this.logger.LogMessage("CreateRunTimeAsync:: Only One-Box deployment supported currently");
                throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "Cluster Analysis is only supported on One-Box deployment for Preview."));
            }

            return runtime;
        }

        private Task OnUnhandledExceptionInAnalysisAsync(string taskName, Func<Task> failedWorkItem, Exception exp)
        {
            this.logger.LogMessage(
                "OnUnhandledExceptionInClusterAnalysis:: TaskName: {0}, Work Item: {1} Encountered Exception: {2}",
                taskName,
                failedWorkItem,
                exp);

            // Stop Cluster Insight
            this.StopClusterInsightAsync(false, StopOptions.StopAnalysis, this.originalCancellationToken).GetAwaiter().GetResult();

            Task.Delay(DelayBetweenRelaunch, this.originalCancellationToken).GetAwaiter().GetResult();

            // Re-Launch CI. TODO Have some policy to restart only X Times and then crash the process.
            return this.InternalLaunchAsync(this.originalCancellationToken);
        }

        private void AddServiceToRuntime(IInsightRuntime runtime, Type type, object service)
        {
            if (runtime.GetService(type) == null)
            {
                runtime.AddService(type, service);
            }
        }

        private async Task<bool> IsCurrentDeploymentOneboxAsync(CancellationToken token)
        {
            ClusterManifestType clusterManifest =
                this.GetClusterManifestFromString(await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => this.fabricClient.ClusterManager.GetClusterManifestAsync(TimeSpan.FromSeconds(30), token),
                    TimeSpan.FromMinutes(2),
                    token).ConfigureAwait(false));

            var windowsInfra = clusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureWindowsServer;
            if (windowsInfra != null)
            {
                this.logger.LogMessage("Windows Infra");

                return windowsInfra.IsScaleMin && windowsInfra.NodeList.Length > 0 &&
                       windowsInfra.NodeList.All(node => node.IPAddressOrFQDN == windowsInfra.NodeList[0].IPAddressOrFQDN);
            }

            var linuxInfra = clusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureLinux;
            if (linuxInfra != null)
            {
                this.logger.LogMessage("Linux Infra");

                return linuxInfra.IsScaleMin && linuxInfra.NodeList.Length > 0 &&
                       linuxInfra.NodeList.All(node => node.IPAddressOrFQDN == linuxInfra.NodeList[0].IPAddressOrFQDN);
            }

            this.logger.LogMessage("Unrecognized OS.");

            return false;
        }

        private ClusterManifestType GetClusterManifestFromString(string content)
        {
            var xmlReaderSettings = new XmlReaderSettings { XmlResolver = null };
            using (var stringReader = new StringReader(content))
            {
                using (XmlReader reader = XmlReader.Create(stringReader, xmlReaderSettings))
                {
                    XmlSerializer xs = new XmlSerializer(typeof(ClusterManifestType));
                    ClusterManifestType data = (ClusterManifestType)xs.Deserialize(reader);
                    return data;
                }
            }
        }

        private Config CreateOneBoxConfig(ClusterAnalysisConfiguration configuration)
        {
            return new Config
            {
                RunMode = RunMode.OneBoxCluster,
                RuntimeContext =
                    new RuntimeContext
                    {
                        FabricCodePath = FabricEnvironment.GetCodePath(),
                        FabricDataRoot = FabricEnvironment.GetDataRoot(),
                        FabricLogRoot = FabricEnvironment.GetLogRoot()
                    },
                IsCrashDumpAnalysisEnabled = false,
                CrashDumpAnalyzerServiceName = string.Empty,
                NamedConfigManager = this.PopulateConfigurationManager(configuration)
            };
        }

        private NamedConfigManager PopulateConfigurationManager(ClusterAnalysisConfiguration configuration)
        {
            var configManagerData = configuration.AgentConfiguration.ToDictionary(oneConfig => oneConfig.AgentName, oneConfig => oneConfig.AgentContext);
            return new NamedConfigManager(configManagerData);
        }

        private Task PersisteCurrentStatusAsync(AnalysisFeatureStatus currentStatus, CancellationToken token)
        {
            return this.clusterAnalyzerApiStateStore.SetEntityAsync(
                ClusterAnalsisApiStateStoreCurrentStateKey,
                JsonConvert.SerializeObject(currentStatus, this.serializationSettings),
                token);
        }

        private async Task<AnalysisFeatureStatus> GetCurrentStatusAsync(CancellationToken token)
        {
            if (!await this.clusterAnalyzerApiStateStore.IsKeyPresentAsync(ClusterAnalsisApiStateStoreCurrentStateKey, token).ConfigureAwait(false))
            {
                return AnalysisFeatureStatus.Default;
            }

            var state = await this.clusterAnalyzerApiStateStore.GetEntityAsync(ClusterAnalsisApiStateStoreCurrentStateKey, token).ConfigureAwait(false);

            return JsonConvert.DeserializeObject<AnalysisFeatureStatus>(state, this.serializationSettings);
        }

        private Task PersistCurrentConfigurationAsync(ClusterAnalysisConfiguration configuration, CancellationToken token)
        {
            return this.clusterAnalyzerApiStateStore.SetEntityAsync(
                ClusterAnalysisApiStateStoreLaunchConfigKey,
                JsonConvert.SerializeObject(configuration, this.serializationSettings),
                token);
        }

        private async Task<ClusterAnalysisConfiguration> GetCurrentConfigurationAsync(CancellationToken token)
        {
            if (!await this.clusterAnalyzerApiStateStore.IsKeyPresentAsync(ClusterAnalysisApiStateStoreLaunchConfigKey, token).ConfigureAwait(false))
            {
                return null;
            }

            var configString = await this.clusterAnalyzerApiStateStore.GetEntityAsync(ClusterAnalysisApiStateStoreLaunchConfigKey, token).ConfigureAwait(false);

            return JsonConvert.DeserializeObject<ClusterAnalysisConfiguration>(configString, this.serializationSettings);
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service
{
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.FaultAnalysis.Service.Chaos;
    using System.Fabric.FaultAnalysis.Service.Common;
    using System.Fabric.FaultAnalysis.Service.Engine;
    using System.Fabric.FaultAnalysis.Service.Test;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Collections;
    using Microsoft.ServiceFabric.Services.Communication.Runtime;
    using Microsoft.ServiceFabric.Services.Runtime;

    public class FaultAnalysisService : StatefulService
    {
        private const string FaultAnalysisServiceTraceType = "FaultAnalysis.Service";
        private const string ClusterAnalysisEnabledConfig = "ClusterAnalysisEnabled";
        private readonly ClusterEndpointSecuritySettingsChangeNotifier clusterEndpointSecuritySettingsChangeNotifier;
        private readonly string endpoint;
        private readonly bool enableEndpointV2;

        private readonly ChaosScheduler scheduler;

        private ReliableFaultsEngine engine;
        private FaultAnalysisServiceMessageProcessor messageProcessor;
        private ActionStore actionStore;
        private ChaosMessageProcessor chaosMessageProcessor;
        private FaultAnalysisServiceImpl faultAnalysisServiceImpl;

#if !DotNetCoreClr
        private ClusterAnalysis.ClusterAnalysisApiHandler clusterAnalysisApiHandler;
#endif

        internal FaultAnalysisService(StatefulServiceContext serviceContext, string endpoint, bool enableEndpointV2)
            : base(
            serviceContext,
            new ReliableStateManager(
                serviceContext,
                new ReliableStateManagerConfiguration(
                    GetReplicatorSettings(endpoint, enableEndpointV2))))
        {
            this.endpoint = endpoint;
            this.enableEndpointV2 = enableEndpointV2;

            if (this.enableEndpointV2)
            {
                this.clusterEndpointSecuritySettingsChangeNotifier = new ClusterEndpointSecuritySettingsChangeNotifier(endpoint, this.UpdateReplicatorSettings);
            }

            this.scheduler = new ChaosScheduler(this.StateManager, this.Partition);
        }

        public bool IsTestMode
        {
            get;
            private set;
        }

        protected override IEnumerable<ServiceReplicaListener> CreateServiceReplicaListeners()
        {
            TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Enter CreateServiceReplicaListeners");

            return new[] { new ServiceReplicaListener(this.CreateCommunicationListener) };
        }

        protected override async Task RunAsync(CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Enter RunAsync");
            this.IsTestMode = false;

            try
            {
                await this.InitializeAsync(cancellationToken).ConfigureAwait(false);
                TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "After InitializeAsync");

                if (this.IsTestMode)
                {
                    this.InternalTest();
                }

                await Task.Delay(Timeout.Infinite, cancellationToken).ConfigureAwait(false);
            }
            catch (FabricNotPrimaryException)
            {
                // do nothing, just exit
            }
            catch (FabricObjectClosedException)
            {
                // do nothing, just exit
            }

            TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Exit RunAsync");
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

        private ICommunicationListener CreateCommunicationListener(ServiceContext serviceContext)
        {
            TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Enter CreateCommunicationListener, serviceInitializationParameters is not null {0}", serviceContext == null ? false : true);
            if (serviceContext != null)
            {
                TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Enter CreateCommunicationListener, partition id is {0}", serviceContext.PartitionId);
                TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Enter CreateCommunicationListener, service name is {0}", serviceContext.ServiceName);
            }

            this.faultAnalysisServiceImpl = new FaultAnalysisServiceImpl();
            var listener = new FASCommunicationListener(serviceContext) { Impl = this.faultAnalysisServiceImpl };
            return listener;
        }

        private async Task InitializeAsync(CancellationToken cancellationToken)
        {
            System.Fabric.Common.NativeConfigStore configStore = System.Fabric.Common.NativeConfigStore.FabricGetConfigStore();
            string isTestModeEnabled = configStore.ReadUnencryptedString(FASConstants.FaultAnalysisServiceConfigSectionName, "TestModeEnabled");
            if (isTestModeEnabled.ToLowerInvariant() == "true")
            {
                TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Enabling internal test mode");
                this.IsTestMode = true;
            }

            string apiTestModeString = configStore.ReadUnencryptedString(FASConstants.FaultAnalysisServiceConfigSectionName, "ApiTestMode");
            int apiTestMode;
            if (!int.TryParse(apiTestModeString, out apiTestMode))
            {
                apiTestMode = 0;
            }

            string requestTimeout = configStore.ReadUnencryptedString(FASConstants.FaultAnalysisServiceConfigSectionName, FASConstants.RequestTimeoutInSecondsName);
            int requestTimeoutInSecondsAsInt = string.IsNullOrEmpty(requestTimeout) ? FASConstants.DefaultRequestTimeoutInSeconds : int.Parse(requestTimeout);

            string operationTimeout = configStore.ReadUnencryptedString(FASConstants.FaultAnalysisServiceConfigSectionName, FASConstants.OperationTimeoutInSecondsName);
            int operationTimeoutInSecondsAsInt = string.IsNullOrEmpty(operationTimeout) ? FASConstants.DefaultOperationTimeoutInSeconds : int.Parse(operationTimeout);

            TestabilityTrace.TraceSource.WriteInfo(
                FaultAnalysisServiceTraceType,
                "Setting requestTimeout='{0}' and operationTimeout='{1}'",
                TimeSpan.FromSeconds(requestTimeoutInSecondsAsInt),
                TimeSpan.FromSeconds(operationTimeoutInSecondsAsInt));

            string maxStoredActionCount = configStore.ReadUnencryptedString(FASConstants.FaultAnalysisServiceConfigSectionName, FASConstants.MaxStoredActionCountName);
            long maxStoredActionCountAsLong = string.IsNullOrEmpty(maxStoredActionCount) ? FASConstants.DefaultMaxStoredActionCount : long.Parse(maxStoredActionCount);

            string storedActionCleanupIntervalInSeconds = configStore.ReadUnencryptedString(FASConstants.FaultAnalysisServiceConfigSectionName, FASConstants.StoredActionCleanupIntervalInSecondsName);
            int storedActionCleanupIntervalInSecondsAsInt = string.IsNullOrEmpty(storedActionCleanupIntervalInSeconds) ? FASConstants.DefaultStoredActionCleanupIntervalInSeconds : int.Parse(storedActionCleanupIntervalInSeconds);

            string completedActionKeepDurationInSeconds = configStore.ReadUnencryptedString(FASConstants.FaultAnalysisServiceConfigSectionName, FASConstants.CompletedActionKeepDurationInSecondsName);
            int completedActionKeepDurationInSecondsAsInt = string.IsNullOrEmpty(completedActionKeepDurationInSeconds) ? FASConstants.DefaultCompletedActionKeepDurationInSeconds : int.Parse(completedActionKeepDurationInSeconds);
            TestabilityTrace.TraceSource.WriteInfo(
                FaultAnalysisServiceTraceType,
                "MaxStoredActionCount={0}, StoredActionCleanupIntervalInSeconds={1}, CompletedActionKeepDurationInSecondsName={2}",
                maxStoredActionCountAsLong,
                storedActionCleanupIntervalInSecondsAsInt,
                completedActionKeepDurationInSecondsAsInt);

            string commandStepRetryBackoffInSeconds = configStore.ReadUnencryptedString(FASConstants.FaultAnalysisServiceConfigSectionName, FASConstants.CommandStepRetryBackoffInSecondsName);
            int commandStepRetryBackoffInSecondsAsInt = string.IsNullOrEmpty(commandStepRetryBackoffInSeconds) ? FASConstants.DefaultCommandStepRetryBackoffInSeconds : int.Parse(commandStepRetryBackoffInSeconds);
            TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Read from config commandStepRetryBackoffInSeconds='{0}'", commandStepRetryBackoffInSecondsAsInt);

            string concurrentRequestsAsString = configStore.ReadUnencryptedString(FASConstants.FaultAnalysisServiceConfigSectionName, FASConstants.ConcurrentRequestsName);
            int concurrentRequests = string.IsNullOrEmpty(concurrentRequestsAsString) ? FASConstants.DefaultConcurrentRequests : int.Parse(concurrentRequestsAsString);
            TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Read from config concurrentRequests='{0}'", concurrentRequests);

            string dataLossCheckWaitDurationInSecondsAsString = configStore.ReadUnencryptedString(FASConstants.FaultAnalysisServiceConfigSectionName, FASConstants.DataLossCheckWaitDurationInSecondsName);
            int dataLossCheckWaitDurationInSeconds = string.IsNullOrEmpty(dataLossCheckWaitDurationInSecondsAsString) ? FASConstants.DefaultDataLossCheckWaitDurationInSeconds : int.Parse(dataLossCheckWaitDurationInSecondsAsString);
            TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Read from config dataLossCheckWaitDurationInSeconds='{0}'", dataLossCheckWaitDurationInSeconds);

            string dataLossCheckPollIntervalInSecondsAsString = configStore.ReadUnencryptedString(FASConstants.FaultAnalysisServiceConfigSectionName, FASConstants.DataLossCheckPollIntervalInSecondsName);
            int dataLossCheckPollIntervalInSeconds = string.IsNullOrEmpty(dataLossCheckPollIntervalInSecondsAsString) ? FASConstants.DefaultDataLossCheckPollIntervalInSeconds : int.Parse(dataLossCheckPollIntervalInSecondsAsString);
            TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Read from config dataLossCheckPollIntervalInSeconds='{0}'", dataLossCheckPollIntervalInSeconds);

            string replicaDropWaitDurationInSecondsAsString = configStore.ReadUnencryptedString(FASConstants.FaultAnalysisServiceConfigSectionName, FASConstants.ReplicaDropWaitDurationInSecondsName);
            int replicaDropWaitDurationInSeconds = string.IsNullOrEmpty(replicaDropWaitDurationInSecondsAsString) ? FASConstants.DefaultReplicaDropWaitDurationInSeconds : int.Parse(replicaDropWaitDurationInSecondsAsString);
            TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Read from config replicaDropWaitDurationInSeconds='{0}'", replicaDropWaitDurationInSeconds);

            string chaosTelemetrySamplingProbabilityAsString = configStore.ReadUnencryptedString(FASConstants.FaultAnalysisServiceConfigSectionName, FASConstants.ChaosTelemetrySamplingProbabilityConfigKeyName);
            double chaosTelemetrySamplingProbability = string.IsNullOrEmpty(chaosTelemetrySamplingProbabilityAsString) ? FASConstants.DefaultChaosTelemetrySamplingProbability : double.Parse(chaosTelemetrySamplingProbabilityAsString);
            TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Read from config chaosTelemetrySamplingProbability='{0}'", chaosTelemetrySamplingProbability);

            string chaosTelemetryReportPeriodInSecondsAsString = configStore.ReadUnencryptedString(FASConstants.FaultAnalysisServiceConfigSectionName, FASConstants.ChaostelemetryReportPeriodInSecondsConfigKeyName);
            int chaostelemetryReportPeriodInSeconds = string.IsNullOrEmpty(chaosTelemetryReportPeriodInSecondsAsString) ? FASConstants.DefaultTelemetryReportPeriodInSeconds : int.Parse(chaosTelemetryReportPeriodInSecondsAsString);
            TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Read from config chaostelemetryReportPeriodInSeconds='{0}'", chaostelemetryReportPeriodInSeconds);

            TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Creating action tables");
            var actionTable = await this.StateManager.GetOrAddAsync<IReliableDictionary<Guid, byte[]>>(FASConstants.ActionReliableDictionaryName).ConfigureAwait(false);

            var historyTable = await this.StateManager.GetOrAddAsync<IReliableDictionary<Guid, byte[]>>(FASConstants.HistoryReliableDictionaryName).ConfigureAwait(false);

            var stoppedNodeTable = await this.StateManager.GetOrAddAsync<IReliableDictionary<string, bool>>(FASConstants.StoppedNodeTable);

            TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Creating ActionStore");
            this.actionStore = new ActionStore(
                this.StateManager,
                this.Partition,
                actionTable,
                historyTable,
                stoppedNodeTable,
                maxStoredActionCountAsLong,
                storedActionCleanupIntervalInSecondsAsInt,
                completedActionKeepDurationInSecondsAsInt,
                this.IsTestMode,
                cancellationToken);

            FabricClient fabricClient = new FabricClient();

            this.engine = new ReliableFaultsEngine(this.actionStore, this.IsTestMode, this.Partition, commandStepRetryBackoffInSecondsAsInt);
            this.messageProcessor = new FaultAnalysisServiceMessageProcessor(
                this.Partition,
                fabricClient,
                this.engine,
                this.actionStore,
                this.StateManager,
                stoppedNodeTable,
                TimeSpan.FromSeconds(requestTimeoutInSecondsAsInt),
                TimeSpan.FromSeconds(operationTimeoutInSecondsAsInt),
                concurrentRequests,
                dataLossCheckWaitDurationInSeconds,
                dataLossCheckPollIntervalInSeconds,
                replicaDropWaitDurationInSeconds,
                cancellationToken);

            string maxStoredChaosEventCleanupIntervalInSeconds = configStore.ReadUnencryptedString(FASConstants.FaultAnalysisServiceConfigSectionName, FASConstants.StoredChaosEventCleanupIntervalInSecondsConfigName);
            int maxStoredChaosEventCleanupIntervalInSecondsAsInt = string.IsNullOrEmpty(maxStoredChaosEventCleanupIntervalInSeconds) ? FASConstants.StoredChaosEventCleanupIntervalInSecondsDefault : int.Parse(maxStoredChaosEventCleanupIntervalInSeconds);

            string maxStoredChaosEventCount = configStore.ReadUnencryptedString(FASConstants.FaultAnalysisServiceConfigSectionName, FASConstants.MaxStoredChaosEventCountConfigName);
            int maxStoredChaosEventCountAsInt = string.IsNullOrEmpty(maxStoredChaosEventCount) ? FASConstants.MaxStoredChaosEventCountDefault : int.Parse(maxStoredChaosEventCount);

            this.chaosMessageProcessor = new ChaosMessageProcessor(
                this.StateManager,
                apiTestMode,
                maxStoredChaosEventCleanupIntervalInSecondsAsInt,
                maxStoredChaosEventCountAsInt,
                chaosTelemetrySamplingProbability,
                chaostelemetryReportPeriodInSeconds,
                this.Partition,
                fabricClient,
                this.scheduler,
                cancellationToken);

            await this.messageProcessor.ResumePendingActionsAsync(fabricClient, cancellationToken).ConfigureAwait(false);
            this.faultAnalysisServiceImpl.MessageProcessor = this.messageProcessor;

            this.faultAnalysisServiceImpl.ChaosMessageProcessor = this.chaosMessageProcessor;
            this.faultAnalysisServiceImpl.LoadActionMapping();

            await this.scheduler.InitializeAsync(fabricClient, this.chaosMessageProcessor, cancellationToken).ConfigureAwait(false);

            TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Done initializing engine");

            await this.InitClusterAnalysisEngineAsync(fabricClient, configStore, cancellationToken).ConfigureAwait(false);
        }

        private Task InitClusterAnalysisEngineAsync(FabricClient fabricClient, NativeConfigStore configStore, CancellationToken cancellationToken)
        {
#if DotNetCoreClr
            TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Skipping Cluster Analysis in .Net Core environment");
            return Task.FromResult(true);
#else
            string clusterAnalysisEnabledAsString = configStore.ReadUnencryptedString(FASConstants.FaultAnalysisServiceConfigSectionName, ClusterAnalysisEnabledConfig);
            if (string.IsNullOrWhiteSpace(clusterAnalysisEnabledAsString))
            {
                TestabilityTrace.TraceSource.WriteWarning(FaultAnalysisServiceTraceType, "ClusterAnalysisEnabled feature Flag is Null/Empty. Skipping Cluster Analysis.");
                return Task.FromResult(true);
            }

            bool clusterAnalysisEnabled = false;
            if (!bool.TryParse(clusterAnalysisEnabledAsString, out clusterAnalysisEnabled))
            {
                TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "ClusterAnalysisConfig : {0} Couldn't be parsed. Skipping Cluster Analysis.", clusterAnalysisEnabledAsString);
                clusterAnalysisEnabled = false;
            }

            if (!clusterAnalysisEnabled)
            {
                TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Cluster Anaysis Feature Status is Disabled");
                return Task.FromResult(true);
            }

            TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Launching Cluster Analysis");
            this.clusterAnalysisApiHandler = new System.Fabric.FaultAnalysis.Service.ClusterAnalysis.ClusterAnalysisApiHandler(this.StateManager, fabricClient, cancellationToken);

            try
            {
                return this.clusterAnalysisApiHandler.InternalLaunchAsync(cancellationToken);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteError(FaultAnalysisServiceTraceType, "Exception Launching Cluster Analysis " + e);
                throw;
            }
#endif
        }

        private void InternalTest()
        {
            TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Running internal tests");
            ActionTest actionTest = ActionTest.CreateAsync(this.messageProcessor, this.actionStore, this.StateManager).GetAwaiter().GetResult();
            actionTest.Test();
        }

        private void UpdateReplicatorSettings(string endpoint)
        {
            ReliableStateManagerReplicatorSettings replicatorSettings = GetReplicatorSettings(endpoint, true);

            IStateProviderReplica stateProviderReplica = ((ReliableStateManager)this.StateManager).Replica;
            Microsoft.ServiceFabric.Data.ReliableStateManagerImpl impl = (Microsoft.ServiceFabric.Data.ReliableStateManagerImpl)stateProviderReplica;
            impl.Replicator.LoggingReplicator.UpdateReplicatorSettings(replicatorSettings);

            TestabilityTrace.TraceSource.WriteInfo(FaultAnalysisServiceTraceType, "Updated replicator settings");
        }
    }
}
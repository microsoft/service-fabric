// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Cluster.ClusterQuery;
    using ClusterAnalysis.AnalysisCore.Callback;
    using ClusterAnalysis.AnalysisCore.ClusterEntityDescription;
    using ClusterAnalysis.AnalysisCore.Helpers;
    using ClusterAnalysis.AnalysisCore.Insights.Agents;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.Common.Runtime;
    using ClusterAnalysis.Common.Store;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using TraceAccessLayer.StoreConnection;
    using Assert = ClusterAnalysis.Common.Util.Assert;

    /// <summary>
    /// Base class for all Insight Generators
    /// </summary>
    internal abstract class BaseInsightGenerator : IInsightGenerator
    {
        private static readonly TimeSpan MaxAnalysisTime = TimeSpan.FromMinutes(30);

        private AnalysisScheduler analysisScheduler;

        /// <summary>
        /// TODO: Persist time the Object was first created. That will be insight start time.
        /// </summary>
        /// <param name="insightRuntime"></param>
        /// <param name="token"></param>
        protected BaseInsightGenerator(IInsightRuntime insightRuntime, ICallbackStore callbackStore, CancellationToken token)
        {
            Assert.IsNotNull(insightRuntime, "Runtime can't be null");
            Assert.IsNotNull(callbackStore, "Callback store can't be null");
            this.InsightRuntime = insightRuntime;
            this.Logger = this.InsightRuntime.GetLogProvider().CreateLoggerInstance(this.GetType().ToString().Split('.').Last());
            this.CallbackStore = callbackStore;

            // Since we may launch a task that we need to cancel, we go ahead create a linked token.
            // Any launched task will respect this linked entity. So effectively, we can chose to cancel it,
            // as well as the creator of this class.
            this.InternalTokenSource = CancellationTokenSource.CreateLinkedTokenSource(token);
            this.InsightLocalToken = this.InternalTokenSource.Token;

            this.ClusterQueryInstance = this.InsightRuntime.GetService(typeof(IClusterQuery)) as IClusterQuery;
            Assert.IsNotNull(this.ClusterQueryInstance, "Runtime doesn't have a valid ClusterQuery object");

            this.EntityStore = this.InsightRuntime.GetStoreProvider()
                .CreatePersistentStoreForStringsAsync(this.GetType() + "_EntityStore", AgeBasedRetentionPolicy.OneMonth, this.InsightLocalToken).GetAwaiter().GetResult();

            this.analysisScheduler = (AnalysisScheduler)this.InsightRuntime.GetService(typeof(AnalysisScheduler));
            Assert.IsNotNull(this.analysisScheduler, "AnalysisScheduler can't be null");
        }

        protected IInsightRuntime InsightRuntime { get; }

        protected ILogger Logger { get; }

        protected IPersistentStore<string, string> EntityStore { get; }

        protected ICallbackStore CallbackStore { get; }

        protected IClusterQuery ClusterQueryInstance { get; }

        protected CancellationToken InsightLocalToken { get; }

        protected CancellationTokenSource InternalTokenSource { get; }

        #region Public_Abstractions

        public Task StartGeneratingAsync()
        {
            this.Logger.LogMessage("Starting Generation");
            this.InitTimeTriggeredAgentScheduling();
            return this.InitializeSignalTriggeredAgentsAsync();
        }

        /// <inheritdoc />
        public async Task StopGeneratingAsync(StopOptions stopOptions)
        {
            this.Logger.LogMessage("StopGeneratingAsync:: Stopping");

            // Stop signal triggered Agents
            await this.StopSignalTriggeredAgentsAsync(stopOptions).ConfigureAwait(false);

            this.Logger.LogMessage("StopGeneratingAsync:: Signalling token which will indicate to Agent that they should now stop");

            // Clear any state that we may have
            if (stopOptions == StopOptions.RemoveAllState)
            {
                await this.EntityStore.ClearAsync(this.InternalTokenSource.Token).ConfigureAwait(false);
            }

            // Give opportunity to kids to do any cleaning required
            await this.StopAsyncInternalAsync(stopOptions);

            // Signal the token
            this.InternalTokenSource.Cancel();
        }

        /// <inheritdoc />
        /// <remarks>
        /// To simplify, we return the max delay that can be there before we get a signal and finish analyzing it.
        /// </remarks>
        public DateTime InsightAvailableTillTime()
        {
            return DateTime.UtcNow - this.GetMaxDelayForInsightGeneration() - MaxAnalysisTime;
        }

        /// <summary>
        /// Add a Consumer
        /// </summary>
        /// <param name="analysisConsumer">Consumer</param>
        /// <param name="option">Consuming options.</param>
        public void AddConsumer(IAnalysisConsumer analysisConsumer, ConsumeOptions option)
        {
            Assert.IsNotNull(analysisConsumer, "analysisConsumer");
            var signalTriggeredAgents = this.GetSignalTriggeredAgents();
            if (signalTriggeredAgents == null)
            {
                return;
            }

            foreach (var oneAgent in signalTriggeredAgents)
            {
                this.Logger.LogMessage("AddConsumer:: Agent: {0}, Adding Consumer: {1} with Option: {2}", oneAgent, analysisConsumer.GetType(), option);
                this.analysisScheduler.AddConsumer(oneAgent, analysisConsumer, option);
            }
        }

        /// <inheritdoc />
        public abstract InsightType GetInsightType();

        #endregion Public_Abstractions

        #region Protected_Abstractions

        /// <summary>
        /// Handle incoming notification. This implementation asks it's kids to do detailed analysis and then
        /// call the routine to Save the state after each analysis.
        /// </summary>
        /// <param name="scenarioData"></param>
        /// <returns></returns>
        internal async Task HandleIncomingEventNotificationAsync(ScenarioData scenarioData)
        {
            var entity = this.ExtractClusterEntityFromSignal(scenarioData.TraceRecord);

            // Couldn't figure out the entity, just bail out.
            // Today there are many cases where this can happen and we don't want to throw for this (maybe someday we will)
            if (entity == null)
            {
                return;
            }

            if (this.IsExcludedFromObservation(entity))
            {
                this.Logger.LogMessage("Entity {0} is Excluded from Insight Generation", entity);
                return;
            }

            await this.EntityStore.SerializeAndSaveObjectInStoreAsync(entity.GetUniqueIdentity().ToString(), entity, this.InsightLocalToken).ConfigureAwait(false);

            await this.InternalHandleIncomingEventNotificationAsync(scenarioData, entity).ConfigureAwait(false);

            await this.DispatchSignalToAgentsAsync(entity, scenarioData).ConfigureAwait(false);
        }

        /// <summary>
        /// In case child class need to have some custom logic executed when Stop is called, they can override this function.
        /// </summary>
        /// <returns></returns>
        protected virtual Task StopAsyncInternalAsync(StopOptions options)
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// This will be called if there is a notification for a signal which we have expressed interest in.
        /// </summary>
        /// <param name="scenarioData"></param>
        /// <param name="baseEntity"></param>
        /// <returns></returns>
        protected abstract Task InternalHandleIncomingEventNotificationAsync(ScenarioData scenarioData, BaseEntity baseEntity);

        /// <summary>
        /// Check if an entity is excluded from observation. If yes we don't
        /// generate insight for this entity.
        /// </summary>
        /// <param name="entity"></param>
        /// <returns></returns>
        protected virtual bool IsExcludedFromObservation(BaseEntity entity)
        {
            return false;
        }

        protected async Task DispatchSignalToAgentsAsync(BaseEntity targetEntity, ScenarioData scenarioData)
        {
            var agentIdentifers = this.GetSignalTriggeredAgents();
            if (agentIdentifers == null)
            {
                return;
            }

            bool signalDispatched = false;
            foreach (var oneAgentifier in agentIdentifers)
            {
                Assert.IsNotNull(AgentDirectory.SingleInstance, "AgentDirectory.SingleInstance != null");
                if (AgentDirectory.SingleInstance.GetOrCreateAgentInstance(oneAgentifier).NotifyFilters.Any(item => item.TargetScenario == scenarioData.Scenario))
                {
                    await this.analysisScheduler.StartAsync(oneAgentifier, targetEntity, scenarioData).ConfigureAwait(false);
                    signalDispatched = true;
                }
            }

            if (!signalDispatched)
            {
                throw new Exception(string.Format("Error Condition: No Target found for Scenario: {0}", scenarioData.Scenario));
            }
        }

        /// <summary>
        /// Gets a collection of agents that are triggered when a specific signal is seen.
        /// </summary>
        /// <returns></returns>
        protected virtual IList<AgentIdentifier> GetSignalTriggeredAgents()
        {
            return null;
        }

        /// <summary>
        /// Gets a collection of agents that are recurring in nature.
        /// </summary>
        /// <returns></returns>
        protected virtual IList<AgentIdentifier> GetTimeTriggeredAgents()
        {
            return null;
        }

        /// <summary>
        /// Get the max delays that can be there before insight is available.
        /// </summary>
        /// <remarks>
        /// We are delayed but the slowest store we have. Some Kid, who is not using let's say slowest store may be delayed
        /// a little, but in overall schema of things, that's ok.
        /// </remarks>
        /// <returns></returns>
        protected TimeSpan GetMaxDelayForInsightGeneration()
        {
            // TODO
            return TimeSpan.FromMinutes(10);
        }

        #endregion Protected_Abstractions

        #region Private_Abstractions

        /// <summary>
        /// Extract the cluster Entity from
        /// </summary>
        /// <param name="fabricEvent"></param>
        /// <returns></returns>
        private BaseEntity ExtractClusterEntityFromSignal(TraceRecord fabricEvent)
        {
            Assert.IsNotNull(fabricEvent);

            BaseEntity baseEntity;
            if (!ClusterEntityExtractor.TryExtractClusterEntity(this.Logger, this.ClusterQueryInstance, fabricEvent, this.InsightLocalToken, out baseEntity))
            {
                this.Logger.LogWarning("ExtractClusterEntityFromSignal:: Couldn't Find Matching Cluster Entity for Event: '{0}'.", fabricEvent);

                return null;
            }

            return baseEntity;
        }

        private void InitTimeTriggeredAgentScheduling()
        {
            var timeTriggeredAgents = this.GetTimeTriggeredAgents();
            if (timeTriggeredAgents == null)
            {
                return;
            }

            this.Logger.LogMessage(
                "InitSignalTriggeredAgentScheduling:: Time Triggered Agents List: '{0}'",
                string.Join(";", timeTriggeredAgents.Select(agent => agent.GetType())));

            foreach (var oneAgentIdentifier in timeTriggeredAgents)
            {
                var timeTriggeredAgent = AgentDirectory.SingleInstance.GetOrCreateAgentInstance(oneAgentIdentifier) as TimeTriggeredAgent;
                Assert.IsNotNull(timeTriggeredAgent, string.Format("Agent Identifier : {0} Not a Time Triggered Agent", oneAgentIdentifier));
                Task.Run(() => this.RunRegularlyAtIntervalAsync(timeTriggeredAgent), this.InsightLocalToken).ContinueWith(
                    task =>
                    {
                        this.Logger.LogError("BaseInsightGenerator:: Task Encountered unexpected Exception '{0}'", task.Exception);
                        Environment.FailFast(
                            string.Format(
                                CultureInfo.InvariantCulture,
                                "BaseStore:: Exception Encountered '{0}'",
                                task.Exception != null ? task.Exception.ToString() : "None"),
                            task.Exception);
                    },
                    TaskContinuationOptions.OnlyOnFaulted);
            }
        }

        private async Task InitializeSignalTriggeredAgentsAsync()
        {
            var signalTriggeredAgents = this.GetSignalTriggeredAgents();
            if (signalTriggeredAgents == null)
            {
                return;
            }

            this.Logger.LogMessage(
                "InitializeSignalTriggeredAgentsAsync:: Signal Triggered Agents List: '{0}'",
                string.Join(";", signalTriggeredAgents));

            // Ok, so some complicated linq magic going on. Here is what we are trying to do. So different agents may be interested in the same scenario,
            // but their notify duration (i.e. signal age) may be different. In such cases, we first group filters by scenario and then for each scenario register with the "minimum"
            // of Notify duration among the group. When we will ultimately receive the signals, while dispatching them to the agents, we would check again
            // if along with their scenario, does their notify duration condition is satisfied or not.
            Assert.IsNotNull(AgentDirectory.SingleInstance, "AgentDirectory.SingleInstance != null");
            var allFilters = signalTriggeredAgents.SelectMany(agentIdentifier => AgentDirectory.SingleInstance.GetOrCreateAgentInstance(agentIdentifier).NotifyFilters);

            var uniqueFilters = allFilters.GroupBy(item => item.TargetScenario).ToDictionary(
                gp => gp.Key,
                gp => gp.ToList().OrderBy(item => item.SignalAvailableForAtLeastDuration).First());

            foreach (var singleFilter in uniqueFilters.Values)
            {
                this.Logger.LogMessage("InitializeSignalTriggeredAgentsAsync:: Registering callback for scenario : '{0}'", singleFilter);
                await this.CallbackStore.RegisterCallbackAsync(singleFilter, this.HandleIncomingEventNotificationAsync, this.InsightLocalToken).ConfigureAwait(false);
            }
        }

        private async Task StopSignalTriggeredAgentsAsync(StopOptions stopOptions)
        {
            var signalTriggeredAgents = this.GetSignalTriggeredAgents();
            if (signalTriggeredAgents == null || !signalTriggeredAgents.Any())
            {
                return;
            }

            // Unregister the callbacks we have registered.
            Assert.IsNotNull(AgentDirectory.SingleInstance, "AgentDirectory.SingleInstance != null");
            foreach (var singleFilter in signalTriggeredAgents.SelectMany(agentIdentifier => AgentDirectory.SingleInstance.GetOrCreateAgentInstance(agentIdentifier).NotifyFilters))
            {
                this.Logger.LogMessage("StopSignalTriggeredAgentsAsync:: UnRegistering callback for scenario : '{0}'", singleFilter);
                await this.CallbackStore.UnRegisterCallbackAsync(singleFilter, this.HandleIncomingEventNotificationAsync, this.InsightLocalToken).ConfigureAwait(false);
            }

            // Convey stop intention to each agent.
            foreach (var oneAgentIdentifier in signalTriggeredAgents)
            {
                this.Logger.LogMessage("StopSignalTriggeredAgentsAsync:: Stopping Agent: '{0}'", oneAgentIdentifier);
                await AgentDirectory.SingleInstance.GetOrCreateAgentInstance(oneAgentIdentifier).StopAsync(stopOptions).ConfigureAwait(false);
            }
        }

        private async Task RunRegularlyAtIntervalAsync(TimeTriggeredAgent agent)
        {
            while (!this.InsightLocalToken.IsCancellationRequested)
            {
                try
                {
                    await agent.StartAsync().ConfigureAwait(false);
                    await Task.Delay(agent.SchedulingFrequencyDuration, this.InsightLocalToken).ConfigureAwait(false);
                }
                catch (OperationCanceledException oce)
                {
                    this.Logger.LogMessage("RunRegularlyAtIntervalAsync:: Operation Cancellation. Exp '{0}'. Shutting Down", oce);
                    this.Logger.Flush();
                    break;
                }
                catch (Exception exp)
                {
                    this.Logger.LogWarning("RunRegularlyAtIntervalAsync:: Encountered Exception '{0}'", exp);
                    this.Logger.Flush();
                    throw;
                }
            }
        }

        #endregion Private_Abstractions
    }
}
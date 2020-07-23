// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.Agents
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Runtime.ExceptionServices;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Threading.Tasks.Dataflow;
    using ClusterAnalysis.AnalysisCore.Callback;
    using ClusterAnalysis.AnalysisCore.ClusterEntityDescription;
    using ClusterAnalysis.AnalysisCore.Helpers;
    using ClusterAnalysis.Common;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.Common.Store;
    using ClusterAnalysis.TraceAccessLayer.StoreConnection;
    using Common.Util;
    using Exceptions;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Assert = ClusterAnalysis.Common.Util.Assert;

    /// <summary>
    /// This is the scheduler for scheduling analysis.
    /// </summary>
    internal class AnalysisScheduler
    {
        private const string TracePrefix = "AnalysisScheduler";

        private const int MaxSignalsBufferedCount = 10;

        // This can be updated in future if required.
        private const int MaxSignalsProcessedInParallel = 3;

        private static readonly TimeSpan DurationBetweenResumeCheck = TimeSpan.FromSeconds(1);

        // Persistent store for signals waiting to be processed.
        private IPersistentStore<Guid, string> signalWaitingToBeProcessedStoreInstance;

        private ActionBlock<AnalysisContainer> signalConsumeDataFlow;

        // In memory map of analysis and the signals available for that analysis.
        private ConcurrentDictionary<Guid, ConcurrentQueue<ScenarioData>> signalWaitingToBeProcessedMap;

        private IDictionary<AgentIdentifier, IDictionary<IAnalysisConsumer, ConsumeOptions>> consumerMap;

        private TypedObjectStore<AnalysisMetadata> analysisMetadataObjectStore;

        private TypedObjectStore<AnalysisContainer> analysisContainerObjectStore;

        public AnalysisScheduler(
            ILogProvider logProvider,
            IStoreProvider storeProvider,
            ITaskRunner taskRunner,
            TraceStoreConnection traceStoreConnection,
            CancellationToken token)
        {
            Assert.IsNotNull(logProvider, "Log Provider can't be null");
            Assert.IsNotNull(storeProvider, "Store Provider can't be null");
            Assert.IsNotNull(taskRunner, "Task Runner can't be null");
            Assert.IsNotNull(traceStoreConnection, "Trace store connection can't be null");

            this.Logger = logProvider.CreateLoggerInstance(TracePrefix);
            this.TaskRunner = taskRunner;
            this.consumerMap = new Dictionary<AgentIdentifier, IDictionary<IAnalysisConsumer, ConsumeOptions>>();
            this.CancelToken = token;

            // The in memory Caches for typed objects
            this.signalWaitingToBeProcessedMap = new ConcurrentDictionary<Guid, ConcurrentQueue<ScenarioData>>();

            this.analysisMetadataObjectStore = new TypedObjectStore<AnalysisMetadata>(
                this.Logger,
                storeProvider,
                AgentConstants.AnalysisMetadataStoreId,
                AgentConstants.AnalysisMetadataStoreRetentionPolicy,
                token);

            this.analysisContainerObjectStore = new TypedObjectStore<AnalysisContainer>(
                this.Logger,
                storeProvider,
                AgentConstants.AnalysisContainerStoreId,
                AgentConstants.AnalysisContainerStoreRetentionPolicy,
                token);

            this.signalWaitingToBeProcessedStoreInstance = storeProvider.CreatePersistentStoreKeyGuidValueStringAsync(
                AgentConstants.AnalysisUnprocessedSignalStoreId,
                AgentConstants.AnalysisUnprocessedSignalStoreRetentionPolicy,
                this.CancelToken).GetAwaiter().GetResult();

            Assert.IsNotNull(this.signalWaitingToBeProcessedStoreInstance, "signalWaitingToBeProcessedStoreInstance");

            // Restore state from any of the previous Runs.
            this.RestoreStateFromPreviousRunsAsync().GetAwaiter().GetResult();

            this.Logger.LogMessage("kicking off Activation Task");
            var activationTask = this.TaskRunner.Run("AgentActivationTask", this.ActivateAnalysisAsync, this.CancelToken);

            this.InitializeDataFlowPipeline();
        }

        /// <summary>
        /// Task Runner.
        /// </summary>
        protected ITaskRunner TaskRunner { get; }

        protected ILogger Logger { get; }

        protected CancellationToken CancelToken { get; }

        #region Public_Abstractions

        public async Task StartAsync(AgentIdentifier agentIdentifier, BaseEntity targetEntity, ScenarioData scenarioData)
        {
            Assert.IsNotNull(targetEntity, "targetEntity");
            Assert.IsNotNull(scenarioData, "scenarioData");

            // Create a new, or retreive an existing container for analyzing this interesting event
            var container = await this.GetAnalysisContainerInterestedInEventAsync(scenarioData.TraceRecord).ConfigureAwait(false);

            // This implies that there is an existing analysis which is waiting for this signal.
            if (container != null)
            {
                Assert.IsTrue(
                    targetEntity.Equals(container.AnalysisEvent.TargetEntity),
                    string.Format(CultureInfo.InvariantCulture, "Entity Mismatch: New: {0}, Existing: {1}", targetEntity, container.AnalysisEvent.TargetEntity));

                // Update the scenario data that is the reason this analysis container got resumed.
                container.CurrentInvokationData = scenarioData;

                // TODO: Do Atomically.
                if (await this.IsAnalysisContainerInPositionToAcceptEventsAsync(container).ConfigureAwait(false))
                {
                    // Push the analysis into analysis compute queue
                    await this.ScheduleAnalysisAsync(container).ConfigureAwait(false);
                }
                else
                {
                    await this.SaveToQueueAsync(container, scenarioData).ConfigureAwait(false);
                }
            }
            else
            {
                Assert.IsNotNull(AgentDirectory.SingleInstance, "AgentDirectory.SingleInstance != null");
                var agent = AgentDirectory.SingleInstance.GetOrCreateAgentInstance(agentIdentifier);

                // Check if this signal is eligible for further analysis.
                if (!await agent.IsEligibleForAnalysisAsync(targetEntity, scenarioData).ConfigureAwait(false))
                {
                    return;
                }

                container = agent.CreateNewAnalysisContainer(scenarioData.TraceRecord);

                // Update the analysis with information on the target entity.
                container.AnalysisEvent.TargetEntity = targetEntity;

                // Update the scenario data that is the reason this analysis container got created/resumed.
                container.CurrentInvokationData = scenarioData;

                // Push the analysis into analysis compute queue
                await this.ScheduleAnalysisAsync(container).ConfigureAwait(false);
            }
        }

        // TODO : Improve this Logic.
        public async Task StopAnalysisAsync(StopOptions options)
        {
            this.Logger.LogMessage("StopAnalysisAsync - Entering");

            // if token has already been signalled, well nothing that we can do now. we simply return.
            if (this.CancelToken.IsCancellationRequested)
            {
                this.Logger.LogWarning("StopAnalysisAsync - InsightLocalToken is already signalled. Indicate a potential Issue.");
                return;
            }

            if (options == StopOptions.StopAnalysis)
            {
                // There is really nothing that we have to do here.
                this.Logger.LogMessage("StopAnalysisAsync - No Op");
            }

            if (options == StopOptions.RemoveAllState)
            {
                this.Logger.LogMessage("StopAnalysisAsync - Deleting All State");
                await this.analysisContainerObjectStore.ClearAllAsync().ConfigureAwait(false);
                await this.analysisMetadataObjectStore.ClearAllAsync().ConfigureAwait(false);
                await this.signalWaitingToBeProcessedStoreInstance.ClearAsync(this.CancelToken).ConfigureAwait(false);
            }
        }

        #endregion Public_Abstractions

        #region Internal_Abstractions

        internal void AddConsumer(AgentIdentifier agentIdentifier, IAnalysisConsumer analysisConsumer, ConsumeOptions option)
        {
            Assert.IsNotNull(analysisConsumer, "analysisConsumer");
            if (this.consumerMap.ContainsKey(agentIdentifier))
            {
                var consumersForThisAgent = this.consumerMap[agentIdentifier];
                if (consumersForThisAgent.Contains(new KeyValuePair<IAnalysisConsumer, ConsumeOptions>(analysisConsumer, option)))
                {
                    throw new Exception(string.Format(CultureInfo.InvariantCulture, "Consumer: {0} with Option: {1} Already Present", analysisConsumer, option));
                }

                consumersForThisAgent[analysisConsumer] = option;
            }
            else
            {
                this.consumerMap[agentIdentifier] = new Dictionary<IAnalysisConsumer, ConsumeOptions> { { analysisConsumer, option } };
            }
        }

        #endregion Internal_Abstractions

        #region Private_Abstractions

        /// <summary>
        /// Entry point for a an Analysis container to Begin. This is true for the main analysis as well
        /// as the update to an existing analysis container
        /// </summary>
        /// <param name="analysis"></param>
        /// <returns></returns>
        /// <exception cref="Exception"></exception>
        private async Task ScheduleAnalysisAsync(AnalysisContainer analysis)
        {
            Assert.IsNotNull(analysis, "analysis");

            var key = analysis.GetUniqueIdentity();

            this.Logger.LogMessage("ScheduleAnalysisAsync : Key : {0}", key);

            // Get the Associated Analysis Metadata.
            var analysisMetadata = await this.analysisMetadataObjectStore.GetOrAddTypedObjectAsync(key, new AnalysisMetadata(key)).ConfigureAwait(false);

            if (analysis.InterestFilter != null)
            {
                analysisMetadata.HasRegisteredInterest = true;
                await this.analysisMetadataObjectStore.PersistTypedObjectAsync(analysisMetadata).ConfigureAwait(false);
            }

            this.AssertIfComputePipelineInFaultedState();

            // Push the signal into our dataflow pipeline.
            var result = await this.signalConsumeDataFlow.SendAsync(analysis, this.CancelToken).ConfigureAwait(false);
            if (!result)
            {
                this.Logger.LogWarning(string.Format(CultureInfo.InvariantCulture, "Failed to Process Analysis: '{0}'", analysis));
                throw new Exception(string.Format(CultureInfo.InvariantCulture, "Failed to Process Analysis: '{0}'", analysis));
            }
        }

        /// <summary>
        /// Check and return if we have an analysis container that is interested in this Event.
        /// </summary>
        /// <param name="traceRecord"></param>
        /// <returns>If container exists, the container is returned. Otherwise null</returns>
        private async Task<AnalysisContainer> GetAnalysisContainerInterestedInEventAsync(TraceRecord traceRecord)
        {
            foreach (var oneSchedulingKvp in await this.analysisMetadataObjectStore.GetCurrentSnapshotOfAllTypedObjectsAsync().ConfigureAwait(false))
            {
                this.CancelToken.ThrowIfCancellationRequested();
                if (!oneSchedulingKvp.Value.HasRegisteredInterest)
                {
                    continue;
                }

                var analysisToBeResumed = await this.analysisContainerObjectStore.GetTypedObjectAsync(oneSchedulingKvp.Key).ConfigureAwait(false);

                if (analysisToBeResumed.InterestFilter.IsMatch(traceRecord))
                {
                    return analysisToBeResumed;
                }
            }

            return null;
        }

        /// <summary>
        /// Check if a container is in position to accept updates
        /// </summary>
        /// <remarks>
        /// Updates can only be accepted once main analysis is finished. This routines checks that.
        /// </remarks>
        /// <param name="analysis"></param>
        /// <returns></returns>
        private async Task<bool> IsAnalysisContainerInPositionToAcceptEventsAsync(AnalysisContainer analysis)
        {
            Assert.IsNotNull(analysis, "analysis");
            var schedulingInfo = (await this.analysisMetadataObjectStore.GetTypedObjectAsync(analysis.GetUniqueIdentity()).ConfigureAwait(false)).SchedulingInfo;
            return schedulingInfo.CurrentStatus == AnalysisStatus.Suspended && schedulingInfo.GetContinuationType() == ContinuationType.WaitForInterest;
        }

        /// <summary>
        /// Save signals for a specific container in a queue. This is done in scenario where the container
        /// is not yet ready to accept signals.
        /// </summary>
        /// <param name="analysis"></param>
        /// <param name="data"></param>
        /// <returns></returns>
        private Task SaveToQueueAsync(AnalysisContainer analysis, ScenarioData data)
        {
            Assert.IsNotNull(data, "data");
            var key = analysis.GetUniqueIdentity();

            ConcurrentQueue<ScenarioData> existingQueue;
            if (this.signalWaitingToBeProcessedMap.TryGetValue(key, out existingQueue))
            {
                if (existingQueue.Contains(data))
                {
                    throw new SignalAlreadyPresentException(string.Format(CultureInfo.InvariantCulture, "Save Queue. Analysis: {0}, Data: {1}", analysis, data));
                }

                existingQueue.Enqueue(data);
            }
            else
            {
                // A tiny window exists for race condition. I thought about using AddOrUpdate but that won't help either since
                // the update lambda is called outside lock. So keep it the way it is for now. In future, consider using a lock
                // for entire operation.
                existingQueue = this.signalWaitingToBeProcessedMap.GetOrAdd(key, new ConcurrentQueue<ScenarioData>(new[] { data }));
            }

            // Persist it.
            return this.signalWaitingToBeProcessedStoreInstance.SetEntityAsync(
                data.GetUniqueIdentity(),
                HandyUtil.Serialize(existingQueue),
                this.CancelToken);
        }

        private async Task KickOffAnalysisAsync(AnalysisContainer analysis)
        {
            await this.PreAnalysisActionsAsync(analysis).ConfigureAwait(false);

            this.CancelToken.ThrowIfCancellationRequested();

            var key = analysis.GetUniqueIdentity();

            var schedulingInfo = (await this.analysisMetadataObjectStore.GetTypedObjectAsync(key)).SchedulingInfo;

            Continuation continuation = null;
            ExceptionDispatchInfo dispatchInfo = null;
            try
            {
                Assert.IsNotNull(AgentDirectory.SingleInstance, "AgentDirectory.SingleInstance != null");
                var agent = AgentDirectory.SingleInstance.GetOrCreateAgentInstance(analysis.Agent);

                if (schedulingInfo.GetContinuationType() == ContinuationType.WaitForInterest)
                {
                    this.Logger.LogMessage("KickOffAnalysisAsync:: Launching Update. Key : {0}", key);
                    continuation = await agent.UpdateAnalysisAsync(analysis).ConfigureAwait(false);
                }
                else
                {
                    this.Logger.LogMessage("KickOffAnalysisAsync:: Calling Main Analysis. Key : {0}", key);
                    continuation = await agent.DoAnalysisAsync(analysis).ConfigureAwait(false);
                }

                this.Logger.LogMessage("KickOffAnalysisAsync:: Continuation : {0} Analysis {1}, key: {2}", continuation, analysis, key);
            }
            catch (Exception exp)
            {
                this.Logger.LogMessage("KickOffAnalysisAsync:: Exception {0} Encountered while Analysing Container: {1}", exp, analysis);
                dispatchInfo = ExceptionDispatchInfo.Capture(exp);
            }

            await this.PostAnalysisActionsAsync(analysis, continuation, dispatchInfo).ConfigureAwait(false);
        }

        private async Task PreAnalysisActionsAsync(AnalysisContainer analysis)
        {
            var key = analysis.GetUniqueIdentity();

            var analysisMetadata = await this.analysisMetadataObjectStore.GetTypedObjectAsync(key);

            analysisMetadata.SchedulingInfo.MarkStarted();

            //// We don't increment analysis attempted count if this is simply an update.
            ////if (schedulingInfo.GetContinuationType() != ContinuationType.WaitForInterest)
            ////{
            ////    schedulingInfo.IncrementAnalysisAttemptedCount();
            ////}

            analysisMetadata.LastInvokedTime = DateTimeOffset.UtcNow;

            await this.analysisMetadataObjectStore.PersistTypedObjectAsync(analysisMetadata).ConfigureAwait(false);
        }

        private async Task PostAnalysisActionsAsync(AnalysisContainer analysis, Continuation continuation, ExceptionDispatchInfo expDispatchInfo)
        {
            var key = analysis.GetUniqueIdentity();

            var analysisMetadata = await this.analysisMetadataObjectStore.GetTypedObjectAsync(key).ConfigureAwait(false);

            analysisMetadata.SchedulingInfo.StartUse();

            // if we have encountered an exception while processing this container, mark as failed.
            if (expDispatchInfo != null)
            {
                // expDispatchInfo.SourceException.Data.Add("FaultingAnalysisSchedulingInfo", schedulingInfo);
                // expDispatchInfo.SourceException.Data.Add("FaultingAnalysis", analysis);
                analysis.AddExceptionSeen(expDispatchInfo.SourceException);
                analysisMetadata.SchedulingInfo.MarkFailed();
            }
            else
            {
                analysisMetadata.SchedulingInfo.SetContinuation(continuation);
            }

            analysis.SetAnalysisStatus(analysisMetadata.SchedulingInfo.CurrentStatus);

            await this.analysisContainerObjectStore.PersistTypedObjectAsync(analysis).ConfigureAwait(false);

            analysisMetadata.SchedulingInfo.StopUse();

            // We delete the object from presisted store once the analysis is finished (Finished, or Failed)
            if (analysisMetadata.SchedulingInfo.CurrentStatus == AnalysisStatus.Completed ||
                analysisMetadata.SchedulingInfo.CurrentStatus == AnalysisStatus.Failed)
            {
                await this.analysisMetadataObjectStore.DeletedTypedObjectAsync(analysisMetadata).ConfigureAwait(false);
            }
            else
            {
                // Persist it.
                await this.analysisMetadataObjectStore.PersistTypedObjectAsync(analysisMetadata);
            }

            if (this.consumerMap.ContainsKey(analysis.Agent))
            {
                // TODO: Today we expect analysisConsumer to be very light weight. In future, we can potentially
                // post them in a new Task.
                foreach (var oneConsumer in this.consumerMap[analysis.Agent])
                {
                    if (analysisMetadata.SchedulingInfo.CurrentStatus == AnalysisStatus.Completed &&
                        (oneConsumer.Value == ConsumeOptions.Finished || oneConsumer.Value == ConsumeOptions.OnlySuccessfullyFinished))
                    {
                        await oneConsumer.Key.ConsumeAsync(analysis.AnalysisEvent, this.CancelToken).ConfigureAwait(false);
                        continue;
                    }

                    if (analysisMetadata.SchedulingInfo.CurrentStatus == AnalysisStatus.Failed &&
                        (oneConsumer.Value == ConsumeOptions.Finished || oneConsumer.Value == ConsumeOptions.OnlyFailed))
                    {
                        await oneConsumer.Key.ConsumeAsync(analysis.AnalysisEvent, this.CancelToken).ConfigureAwait(false);
                    }
                }
            }

            // Re-throw the exception.
            if (expDispatchInfo != null)
            {
                expDispatchInfo.Throw();
            }
        }

        private void AssertIfComputePipelineInFaultedState()
        {
            if (this.signalConsumeDataFlow.Completion.IsCanceled || this.signalConsumeDataFlow.Completion.IsFaulted)
            {
                this.Logger.LogWarning("AssertIfComputePipelineInFaultedState:: In Faulted State");
                throw new Exception("Internal Error. Subsystem in faulted State");
            }
        }

        private async Task RestoreStateFromPreviousRunsAsync()
        {
            if (this.GetType().IsSubclassOf(typeof(TimeTriggeredAgent)))
            {
                this.Logger.LogMessage("RestoreStateFromPreviousRunsAsync:: Skipping Restore for TimeTriggered Agents");
                return;
            }

            // Restore the signals that haven't been processed yet. Read them from persistent storage and put them into our in memory queue.
            var analysisDetails = await this.signalWaitingToBeProcessedStoreInstance.GetAllValuesAsync(this.CancelToken).ConfigureAwait(false);
            this.Logger.LogMessage("RestoreStateFromPreviousRunsAsync:: Restoring unprocessed Signal Queue. Queue Depth: {0}", analysisDetails.Count);
            foreach (var entry in analysisDetails)
            {
                var queue = this.signalWaitingToBeProcessedMap.GetOrAdd(entry.Key, InsightsUtil.DeSerialize<ConcurrentQueue<ScenarioData>>(entry.Value));
            }

            // Restores the Analysis metadata map from persistent storage.
            var metadataInMemoryMap = await this.analysisMetadataObjectStore.GetCurrentSnapshotOfAllTypedObjectsAsync().ConfigureAwait(false);
            foreach (var oneMetatada in metadataInMemoryMap.Values)
            {
                // Little bit of hacky way. Basically any analysis that was in progress or not started from some earlier run,
                // we mark them as suspended with immediate resume so that activator thread can pick it up.
                if (oneMetatada.SchedulingInfo.CurrentStatus == AnalysisStatus.InProgress ||
                    oneMetatada.SchedulingInfo.CurrentStatus == AnalysisStatus.NotStarted ||
                    oneMetatada.SchedulingInfo.CurrentStatus == AnalysisStatus.Queued)
                {
                    oneMetatada.SchedulingInfo.SetContinuation(Continuation.ResumeImmediately);
                    await this.analysisMetadataObjectStore.PersistTypedObjectAsync(oneMetatada).ConfigureAwait(false);
                }
            }
        }

        private async Task ActivateAnalysisAsync()
        {
            while (!this.CancelToken.IsCancellationRequested)
            {
                var allMetadata = await this.analysisMetadataObjectStore.GetCurrentSnapshot().ConfigureAwait(false);

                while (!allMetadata.IsEmpty())
                {
                    this.CancelToken.ThrowIfCancellationRequested();

                    var oneMetadata = allMetadata.ExtractMax();

                    if (oneMetadata.SchedulingInfo.InUse())
                    {
                        continue;
                    }

                    if (oneMetadata.SchedulingInfo.CurrentStatus == AnalysisStatus.Completed)
                    {
                        continue;
                    }

                    if (oneMetadata.SchedulingInfo.IsOutOfAttempts())
                    {
                        continue;
                    }

                    if (oneMetadata.SchedulingInfo.CurrentStatus == AnalysisStatus.Suspended &&
                        oneMetadata.SchedulingInfo.GetContinuationType() == ContinuationType.WaitForTime)
                    {
                        var nextActivationTime = oneMetadata.SchedulingInfo.GetNextActivationTimeUtc();
                        if (DateTime.UtcNow >= nextActivationTime)
                        {
                            var analysisToBeResumed = await this.analysisContainerObjectStore.GetTypedObjectAsync(oneMetadata.Key).ConfigureAwait(false);

                            await this.ResumeAnalysisAsync(oneMetadata.Key, analysisToBeResumed).ConfigureAwait(false);
                        }
                    }

                    if (oneMetadata.SchedulingInfo.CurrentStatus == AnalysisStatus.Suspended &&
                        oneMetadata.SchedulingInfo.GetContinuationType() == ContinuationType.WaitForInterest)
                    {
                        var interestedData = await this.GetQueuedUpDataForAnalysisAsync(oneMetadata.Key).ConfigureAwait(false);

                        // If null, it implies there is no data waiting for this analysis container. Continue.
                        if (interestedData == null)
                        {
                            continue;
                        }

                        var analysisToBeResumed = await this.analysisContainerObjectStore.GetTypedObjectAsync(oneMetadata.Key).ConfigureAwait(false);

                        Assert.IsNotNull(analysisToBeResumed.InterestFilter != null, "analysisToBeResumed.InterestFilter != null");

                        // This is just a fail safe against any race condition that may exist that causes us to process same data twice.
                        if (analysisToBeResumed.AnalysisEvent.CorrelatedTraceRecords.Contains(interestedData.TraceRecord))
                        {
                            this.Logger.LogMessage("Trace Record data : {0} Already Processed inside Container: {1}", interestedData, analysisToBeResumed);
                            continue;
                        }

                        // Basically any signal under the analysis key should have been added to this queue only if
                        // Analysis has expressed an interest in this signal. We validate that here to catch any bugs.
                        Assert.IsTrue(analysisToBeResumed.InterestFilter.IsMatch(interestedData.TraceRecord));

                        analysisToBeResumed.CurrentInvokationData = interestedData;

                        // Resume Analysis.
                        await this.ResumeAnalysisAsync(oneMetadata.Key, analysisToBeResumed).ConfigureAwait(false);
                    }
                }

                await Task.Delay(DurationBetweenResumeCheck, this.CancelToken);
            }
        }

        private async Task ResumeAnalysisAsync(Guid key, AnalysisContainer analysis)
        {
            this.Logger.LogMessage("ResumeAnalysisAsync:: Resuming Analysis Key:{0}, Value: {1}, ProgressMarkder: {2}", key, analysis, analysis.GetProgressedTill());

            var metadata = await this.analysisMetadataObjectStore.GetTypedObjectAsync(key).ConfigureAwait(false);

            metadata.SchedulingInfo.MarkQueued();

            // Persist the updated metadata
            await this.analysisMetadataObjectStore.PersistTypedObjectAsync(metadata);

            // Schedule the analysis.
            await this.ScheduleAnalysisAsync(analysis);
        }

        private void InitializeDataFlowPipeline()
        {
            // Initialize the data flow pipeline
            this.signalConsumeDataFlow = new ActionBlock<AnalysisContainer>(
                async analysis => await this.KickOffAnalysisAsync(analysis).ConfigureAwait(false),
                new ExecutionDataflowBlockOptions
                {
                    BoundedCapacity = MaxSignalsBufferedCount,
                    MaxDegreeOfParallelism = MaxSignalsProcessedInParallel,
                    CancellationToken = this.CancelToken
                });

            this.signalConsumeDataFlow.Completion.ContinueWith(this.TaskRunner.GetDefaultContinuation("AgentActionBlock", this.CancelToken), this.CancelToken);
        }

        // Sometime when a signal is received, the analysis container may not be in position to accept the data and
        // the data is put into this queue. When the analysis is in position to accept more data, this routine
        // is called to fetch data (if present).
        private async Task<ScenarioData> GetQueuedUpDataForAnalysisAsync(Guid analysisContainerIdentifier)
        {
            ConcurrentQueue<ScenarioData> queueOfDataInDispatchWaiting;
            if (!this.signalWaitingToBeProcessedMap.TryGetValue(analysisContainerIdentifier, out queueOfDataInDispatchWaiting))
            {
                return null;
            }

            ScenarioData firstInQueue;
            if (!queueOfDataInDispatchWaiting.TryDequeue(out firstInQueue))
            {
                return null;
            }

            // Persist the change in queue.
            await this.signalWaitingToBeProcessedStoreInstance.SetEntityAsync(
                analysisContainerIdentifier,
                InsightsUtil.Serialize(queueOfDataInDispatchWaiting),
                this.CancelToken).ConfigureAwait(false);

            return firstInQueue;
        }

        #endregion Private_Abstractions
    }
}
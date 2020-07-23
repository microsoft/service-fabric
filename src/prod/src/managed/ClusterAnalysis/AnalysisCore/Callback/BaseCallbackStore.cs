// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Callback
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Threading.Tasks.Dataflow;
    using ClusterAnalysis.AnalysisCore.Callback.Exceptions;
    using ClusterAnalysis.AnalysisCore.Helpers;
    using ClusterAnalysis.Common;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.Common.Store;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Assert = ClusterAnalysis.Common.Util.Assert;

    /// <summary>
    /// Base implementation for different types of stores.
    /// </summary>
    internal abstract class BaseCallbackStore : ICallbackStore
    {
        // DO NOT CHANGE THESE STRINGS AS THEY ARE USED IN RELIABLE COLLECTION DATA STRUCTURE. IF YOU CHANGE THINGS MAY BREAK
        private const string CallbackTimeBookmarkStore = "CallbackTimeBookmarkStore";

        private const string CurrentTimeBookmarkKey = "CurrentTimeBookmarkKey";

        private const string ReportedSignalUniqueIdsStoreName = "ReportedSignalUniqueIdsStore";

        private const string SignalsInDispatchQueueStoreName = "SignalsInDispatchQueueStoreName";

        /// <summary>
        /// We only look back for signals for this maximum duration.
        /// </summary>
        private static readonly TimeSpan MaxTimeBackToLookForNewSignals = TimeSpan.FromMinutes(15);

        private IDictionary<NotifyFilter, IList<Func<ScenarioData, Task>>> callbackMap;

        private SemaphoreSlim singleAccessLock;

        private Task newEventSearcherTask;

        private Task eventDispatcherTask;

        private IPersistentStore<Guid, string> reportedSignalsUniqueIdsStore;

        private IPersistentStore<string, DateTime> callBackCheckPointTimeStore;

        private IPersistentStore<Guid, string> signalsInDispatchQueuePersistentStore;

        private BufferBlock<ScenarioNotificationData> producerConsumerBufferBlock;

        private ITaskRunner taskRunner;

        protected BaseCallbackStore(IStoreProvider storeProvider, ITaskRunner taskRunner, ILogProvider logProvider, CancellationToken token)
        {
            Assert.IsNotNull(storeProvider, "Store Provider can't be null");
            Assert.IsNotNull(taskRunner, "Task Runner can't be null");
            Assert.IsNotNull(logProvider, "Log Provider can't be null");

            this.singleAccessLock = new SemaphoreSlim(1);
            this.callbackMap = new Dictionary<NotifyFilter, IList<Func<ScenarioData, Task>>>();
            this.Logger = logProvider.CreateLoggerInstance(this.GetType().ToString().Split('.').Last());
            this.taskRunner = taskRunner;

            this.InternalTokenSource = CancellationTokenSource.CreateLinkedTokenSource(token);
            token = this.InternalTokenSource.Token;

            // Some signals may not show up immedietly so we don't want their last seen time bailing on us.
            this.callBackCheckPointTimeStore = storeProvider.CreatePersistentStoreForTimeAsync(CallbackTimeBookmarkStore, AgeBasedRetentionPolicy.OneDay, token)
                .GetAwaiter().GetResult();

            // This store keeps track of unique Ids of signals seen to avoid duplicate reporting. The entries in this
            // collection needs to be long lived. Consider a signal that is living in the system for a long time
            // If we don't track that this has been reported, we may try to report it again.
            this.reportedSignalsUniqueIdsStore = storeProvider
                .CreatePersistentStoreKeyGuidValueStringAsync(ReportedSignalUniqueIdsStoreName, AgeBasedRetentionPolicy.OneWeek, token).GetAwaiter()
                .GetResult();

            this.signalsInDispatchQueuePersistentStore = storeProvider
                .CreatePersistentStoreKeyGuidValueStringAsync(SignalsInDispatchQueueStoreName, AgeBasedRetentionPolicy.OneHour, token).GetAwaiter().GetResult();

            this.producerConsumerBufferBlock = new BufferBlock<ScenarioNotificationData>();
            this.InitSignalSeenBookmarkStoreAsync(token).Wait(token);
        }

        #region Protected_Props

        protected CancellationTokenSource InternalTokenSource { get; }

        /// <summary>
        /// Logger Object
        /// </summary>
        protected ILogger Logger { get; }

        #endregion Protected_Props

        #region Public_Abstractions

        /// <inheritdoc />
        public abstract IEnumerable<Scenario> GetCallbackSupportedScenario();

        /// <inheritdoc />
        /// <remarks>
        /// You can add the same routine again and we will call you once for each registered callback. Caller should handle that logic
        /// </remarks>
        public async Task RegisterCallbackAsync(NotifyFilter filter, Func<ScenarioData, Task> callbackRoutine, CancellationToken token)
        {
            Assert.IsNotNull(filter, "filter");
            Assert.IsNotNull(callbackRoutine, "callbackRoutine");

            this.Logger.LogMessage("RegisterCallbackAsync:: Registering callback with Filter '{0}'", filter);

            if (!this.GetCallbackSupportedScenario().Contains(filter.TargetScenario))
            {
                throw new NotSupportedException(
                    string.Format(CultureInfo.InvariantCulture, "Scenario : '{0}' not supported for callback", filter.TargetScenario));
            }

            await this.singleAccessLock.WaitAsync(token).ConfigureAwait(false);

            try
            {
                if (!this.callbackMap.ContainsKey(filter))
                {
                    this.callbackMap[filter] = new List<Func<ScenarioData, Task>> { callbackRoutine };
                }
                else
                {
                    this.callbackMap[filter].Add(callbackRoutine);
                }

                if (this.newEventSearcherTask == null)
                {
                    this.Logger.LogMessage("RegisterCallbackAsync:: Kicking off Searcher Task");

                    // kick off tasks that continuously monitors for new signals.
                    this.newEventSearcherTask = this.taskRunner.Run("NewEventSearcherTask", () => this.SearchForNewSignalsInClusterAsync(token), this.InternalTokenSource.Token);
                }

                if (this.eventDispatcherTask == null)
                {
                    this.Logger.LogMessage("RegisterCallbackAsync:: Kicking off Dispatcher Task");

                    // kick off a task that continuously looks for new signals seen and invoke callbacks associated with them.
                    this.eventDispatcherTask = this.taskRunner.Run("CallbackDispatcherTask", () => this.DispatchSignalsAsync(token), this.InternalTokenSource.Token);
                }
            }
            finally
            {
                this.singleAccessLock.Release();
            }
        }

        /// <inheritdoc />
        public async Task UnRegisterCallbackAsync(NotifyFilter filter, Func<ScenarioData, Task> callbackRoutine, CancellationToken token)
        {
            Assert.IsNotNull(filter, "filter");
            Assert.IsNotNull(callbackRoutine, "callbackRoutine");
            this.Logger.LogMessage("UnRegisterCallbackAsync:: UnRegistering callback with Filter '{0}'", filter);

            if (!this.GetCallbackSupportedScenario().Contains(filter.TargetScenario))
            {
                throw new NotSupportedException(
                    string.Format(CultureInfo.InvariantCulture, "Scenario : '{0}' not supported for callback", filter.TargetScenario));
            }

            await this.singleAccessLock.WaitAsync(token).ConfigureAwait(false);

            try
            {
                if (!this.callbackMap.ContainsKey(filter))
                {
                    throw new CallbackNotRegisteredException(string.Format(CultureInfo.InvariantCulture, "Callback for Filter: {0} not registered", filter));
                }

                if (!this.callbackMap[filter].Contains(callbackRoutine))
                {
                    throw new CallbackNotRegisteredException(
                        string.Format(CultureInfo.InvariantCulture, "Callback for Filter: {0} with Delegage: {1} not registered", filter, callbackRoutine));
                }

                this.callbackMap[filter].Remove(callbackRoutine);

                // if there are no callbacks for this filter, remove the entry from map.
                if (!this.callbackMap[filter].Any())
                {
                    this.callbackMap[filter] = null;
                    this.callbackMap.Remove(filter);
                }
            }
            finally
            {
                this.singleAccessLock.Release();
            }
        }

        /// <inheritdoc />
        public async Task ResetCheckpointAsync(CancellationToken token)
        {
            await this.singleAccessLock.WaitAsync(token).ConfigureAwait(false);

            try
            {
                this.callbackMap.Clear();

                this.InternalTokenSource.Cancel();

                await this.eventDispatcherTask.ConfigureAwait(false);

                this.eventDispatcherTask = null;

                await this.newEventSearcherTask.ConfigureAwait(false);

                this.newEventSearcherTask = null;

                await this.reportedSignalsUniqueIdsStore.ClearAsync(token).ConfigureAwait(false);
                await this.callBackCheckPointTimeStore.ClearAsync(token).ConfigureAwait(false);
                await this.signalsInDispatchQueuePersistentStore.ClearAsync(token).ConfigureAwait(false);

                await this.ResetInternalAsync(token);
            }
            finally
            {
                this.singleAccessLock.Release();
            }
        }

        #endregion Public_Abstractions

        #region Protected_Abstractions

        /// <summary>
        /// Child classes can do reset in this method.
        /// </summary>
        /// <returns></returns>
        protected virtual Task ResetInternalAsync(CancellationToken token)
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// Get signals matching the specific scenario
        /// </summary>
        /// <param name="scenarios"></param>
        /// <param name="duration"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        protected abstract Task<IDictionary<Scenario, IList<TraceRecord>>> GetSignalsForScenarioAsync(
            IList<Scenario> scenarios,
            Duration duration,
            CancellationToken token);

        /// <summary>
        /// Time between checks for availability of new signals
        /// </summary>
        /// <returns></returns>
        protected abstract TimeSpan GetDelayBetweenCheckForNewSignalAvailability();

        #endregion Protected_Abstractions

        #region Private_Abstraction

        /// <summary>
        /// Routine that actually does the monitoring for availability of new signals that satisfies the notify filters
        /// </summary>
        /// <param name="token"></param>
        /// <returns></returns>
        private async Task SearchForNewSignalsInClusterAsync(CancellationToken token)
        {
            // Check if there are any undispatched signals from last run. If yes, first dispatch them.
            var signalsFromLastRunNotYetDispatched = await this.GetUndispatchedSignalsFromLastRunAsync(token).ConfigureAwait(false);
            if (signalsFromLastRunNotYetDispatched.Any())
            {
                this.Logger.LogMessage(
                    "SearchForNewSignalsInClusterAsync {0} signals from last run. Posting them",
                    signalsFromLastRunNotYetDispatched.Count);

                // We sleep for some time for caller to register callback again. 
                // Not ideal but required since we can't persist func using DataContractSerializer
                // so have to rely on agents to re-register.
                await Task.Delay(TimeSpan.FromSeconds(10), token).ConfigureAwait(false);
                await this.PostNewSignalToDispatherQueueAsync(signalsFromLastRunNotYetDispatched, token).ConfigureAwait(false);
            }

            // Now that we are done paying our debt, continuously look for new signals.
            while (!token.IsCancellationRequested)
            {
                var newSignals = await this.GetNewSignalsSeenAsync(token).ConfigureAwait(false);

                // Sort them by Time.
                newSignals = newSignals.OrderBy(item => item.ScenarioData.EventSeenTime).ToList();

                // Persist these signals. This comes in handy if before dispatching, process crashes.
                // The dispatch queue will clear the signals once they are dispatched.
                foreach (var oneData in newSignals)
                {
                    await this.signalsInDispatchQueuePersistentStore
                        .SerializeAndSaveObjectInStoreAsync(oneData.ScenarioData.TraceRecord.GetUniqueId(), oneData, token).ConfigureAwait(false);
                }

                await this.PostNewSignalToDispatherQueueAsync(newSignals, token).ConfigureAwait(false);

                await Task.Delay(this.GetDelayBetweenCheckForNewSignalAvailability(), token).ConfigureAwait(false);
            }
        }

        // This routine reads from the undispatched persistent store and return all the values present there.
        private async Task<IList<ScenarioNotificationData>> GetUndispatchedSignalsFromLastRunAsync(CancellationToken token)
        {
            var undispatchDataList = new List<ScenarioNotificationData>();
            var allDataSerialized = await this.signalsInDispatchQueuePersistentStore.GetAllValuesAsync(token).ConfigureAwait(false);

            foreach (var oneVal in allDataSerialized.Values)
            {
                var data = InsightsUtil.DeSerialize<ScenarioNotificationData>(oneVal);
                undispatchDataList.Add(data);
            }

            return undispatchDataList;
        }

        private async Task<IList<ScenarioNotificationData>> GetNewSignalsSeenAsync(CancellationToken token)
        {
            IList<ScenarioNotificationData> dataResult = new List<ScenarioNotificationData>();

            var allFilters = await this.GetAllNotifyFiltersAsync(token).ConfigureAwait(false);

            var allScenarios = allFilters.Select(item => item.TargetScenario);

            var duration = await this.GetDurationToLookForNewSignalsAsync(token).ConfigureAwait(false);

            var allSignalsGroupedByScenario = await this.GetSignalsForScenarioAsync(allScenarios.ToList(), duration, token).ConfigureAwait(false);

            // No new signals at all to report.
            if (!allSignalsGroupedByScenario.Any())
            {
                return dataResult;
            }

            await this.UpdateCurrentTimeBookmarkAsync(duration.EndTime, token).ConfigureAwait(false);

            foreach (var filter in allFilters)
            {
                token.ThrowIfCancellationRequested();

                // No new signals seen for this scenario. Move to next one.
                if (!allSignalsGroupedByScenario.ContainsKey(filter.TargetScenario))
                {
                    continue;
                }

                // Found some for this scenario. Do filtering (for e.g. already reported etc.) and wrap them up in nicer abstraction.
                var newScenarioDataToShare = await this.FilterNewSignalsAsync(allSignalsGroupedByScenario[filter.TargetScenario], filter, token)
                    .ConfigureAwait(false);

                foreach (var oneData in newScenarioDataToShare)
                {
                    dataResult.Add(new ScenarioNotificationData(oneData, filter));
                }
            }

            return dataResult;
        }

        private async Task PostNewSignalToDispatherQueueAsync(IEnumerable<ScenarioNotificationData> callbackData, CancellationToken token)
        {
            foreach (var oneData in callbackData)
            {
                token.ThrowIfCancellationRequested();

                // Post this new signal along with callback information to the buffer (between producer and consumer)
                var result = await this.producerConsumerBufferBlock.SendAsync(oneData, token).ConfigureAwait(false);
                if (result)
                {
                    continue;
                }

                var message = string.Format(CultureInfo.InvariantCulture, "Failed to Send Data: {0}", oneData);
                this.Logger.LogWarning(message);
                throw new SignalDispatchException(message);
            }
        }

        /// <summary>
        /// Dispatches the new Events seen to their registered handlers.
        /// </summary>
        /// <param name="token"></param>
        /// <returns></returns>
        private async Task DispatchSignalsAsync(CancellationToken token)
        {
            while (await this.producerConsumerBufferBlock.OutputAvailableAsync(token).ConfigureAwait(false))
            {
                var dispatchData = await this.producerConsumerBufferBlock.ReceiveAsync(token).ConfigureAwait(false);

                // Check again to make sure that this signal hasn't been dispatched already.
                if (await this.IsSignalAlreadyReportedAsync(dispatchData.ScenarioData, token))
                {
                    continue;
                }

                // Mark the signal as Reported. This help us protect from duplicate reporting.
                await this.RecordThatSignalReportedAsync(dispatchData.ScenarioData, token).ConfigureAwait(false);

                // Remove the reported data from the persistent queue. There is still a tiny window where a signal could be lost
                // i.e. between this removal and the before the signal actually get dispatched. But that's ok.
                // This is to protect against the scenario where handler keeps on failing repeatedly and if that happens
                // and if we don't record signal as reported, we will never make forward progress.
                await this.signalsInDispatchQueuePersistentStore.RemoveEntityIfPresentAsync(dispatchData.ScenarioData.TraceRecord.GetUniqueId(), token)
                    .ConfigureAwait(false);

                var callbacks = await this.GetCallbacksForFilterAsync(dispatchData.Filter, token).ConfigureAwait(false);

                var taskList = callbacks.Select(callback => callback(dispatchData.ScenarioData)).ToList();

                await Task.WhenAll(taskList).ConfigureAwait(false);
            }
        }

        private async Task<IEnumerable<ScenarioData>> FilterNewSignalsAsync(
            IEnumerable<TraceRecord> traceRecords,
            NotifyFilter filter,
            CancellationToken token)
        {
            var newSignalsMatchingFilter = new List<ScenarioData>();
            foreach (var oneTraceRecord in traceRecords.OrderBy(record => record.TimeStamp))
            {
                // Encapsulate the new information into a shareable abstraction. We mark the event seen time as current time
                // which may not be completely accurate but it's close enough for all purposes.
                var scenarioData = new ScenarioData(filter.TargetScenario, oneTraceRecord, DateTime.UtcNow);

                if (DateTime.UtcNow - oneTraceRecord.TimeStamp > filter.SignalAvailableForAtLeastDuration)
                {
                    if (!await this.IsSignalAlreadyReportedAsync(scenarioData, token).ConfigureAwait(false))
                    {
                        newSignalsMatchingFilter.Add(scenarioData);
                    }
                }
            }

            return newSignalsMatchingFilter;
        }

        private async Task<Duration> GetDurationToLookForNewSignalsAsync(CancellationToken token)
        {
            // Always query from last signal seen - max signal delay. This is to account for the cases where some nodes have already pushed latest data
            // but others are lagging behind. In such a case if we just start looking from time of last signal seen, we may miss data from slow nodes.
            // Our logic to keep track of reported signal will ensure that we don't end up analyzing seen incident twice
            var lastCheckpointTime = await this.GetCurrentTimeBookmarkAsync(token).ConfigureAwait(false) - this.GetStoreIsBehindByDuration();
            var endTime = DateTime.UtcNow;

            // We don't want to ever look back for signals for duration > predefined value for optimization reason.
            if (endTime - lastCheckpointTime <= MaxTimeBackToLookForNewSignals)
            {
                return new Duration(lastCheckpointTime, endTime);
            }

            lastCheckpointTime = endTime - MaxTimeBackToLookForNewSignals;
            return new Duration(lastCheckpointTime, endTime);
        }

        private TimeSpan GetStoreIsBehindByDuration()
        {
            return TimeSpan.FromMinutes(1);
        }

        private Task<bool> IsSignalAlreadyReportedAsync(ScenarioData signal, CancellationToken token)
        {
            return this.reportedSignalsUniqueIdsStore.IsKeyPresentAsync(signal.TraceRecord.GetUniqueId(), token);
        }

        /// <summary>
        /// Mark a signal as reported
        /// </summary>
        /// <param name="signal"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        /// <remarks>
        /// Temporary change to convert store into a string store and storing the signal to catch a tricky hash collision issue.
        /// </remarks>
        private async Task RecordThatSignalReportedAsync(ScenarioData signal, CancellationToken token)
        {
            var identifier = signal.GetUniqueIdentity();
            if (await this.reportedSignalsUniqueIdsStore.IsKeyPresentAsync(signal.TraceRecord.GetUniqueId(), token).ConfigureAwait(false))
            {
                throw new DuplicateSignalRecordException(
                    string.Format(CultureInfo.InvariantCulture, "Already Reported: Id: {0}, Signal: {1}", identifier, signal));
            }

            // We need a list which we don't have, so using same (unique) key as value.
            await this.reportedSignalsUniqueIdsStore.SetEntityAsync(signal.TraceRecord.GetUniqueId(), signal.TraceRecord.GetUniqueId().ToString(), token)
                .ConfigureAwait(false);
        }

        /// <summary>
        /// Initialize last time signal was seen
        /// </summary>
        /// <remarks>
        /// Initial value is obtained by subtracting the period store is running behind from current time.
        /// </remarks>
        private async Task InitSignalSeenBookmarkStoreAsync(CancellationToken token)
        {
            // If not present in store, go ahead add the entry in store.
            if (!await this.callBackCheckPointTimeStore.IsKeyPresentAsync(CurrentTimeBookmarkKey, token).ConfigureAwait(false))
            {
                this.Logger.LogMessage("InitSignalSeenBookmarkStore:: No cached Start. Setting it to: '{0}'", DateTime.UtcNow);
                this.UpdateCurrentTimeBookmarkAsync(DateTime.UtcNow, token).Wait(token);
            }

            var startTime = await this.callBackCheckPointTimeStore.GetEntityAsync(CurrentTimeBookmarkKey, token).ConfigureAwait(false);
            this.Logger.LogMessage("InitSignalSeenBookmarkStore:: We will look for new Signals Starting Time: '{0}'", startTime);
        }

        private Task UpdateCurrentTimeBookmarkAsync(DateTime latestSeenTime, CancellationToken token)
        {
            return this.callBackCheckPointTimeStore.SetEntityAsync(CurrentTimeBookmarkKey, latestSeenTime, token);
        }

        private async Task<DateTime> GetCurrentTimeBookmarkAsync(CancellationToken token)
        {
            if (!await this.callBackCheckPointTimeStore.IsKeyPresentAsync(CurrentTimeBookmarkKey, token).ConfigureAwait(false))
            {
                throw new CallbackStoreBookmarkMissingException("Checkpoint Missing");
            }

            return await this.callBackCheckPointTimeStore.GetEntityAsync(CurrentTimeBookmarkKey, token).ConfigureAwait(false);
        }

        private async Task<IList<NotifyFilter>> GetAllNotifyFiltersAsync(CancellationToken token)
        {
            IList<NotifyFilter> allNotifyFilters;
            await this.singleAccessLock.WaitAsync(token).ConfigureAwait(false);
            try
            {
                allNotifyFilters = this.callbackMap.Keys.ToList();
            }
            finally
            {
                this.singleAccessLock.Release();
            }

            return allNotifyFilters;
        }

        private async Task<IList<Func<ScenarioData, Task>>> GetCallbacksForFilterAsync(NotifyFilter filter, CancellationToken token)
        {
            await this.singleAccessLock.WaitAsync(token).ConfigureAwait(false);
            try
            {
                return this.callbackMap[filter];
            }
            catch (KeyNotFoundException)
            {
                this.Logger.LogWarning("GetCallbacksForFilterAsync:: Key {0} not found", filter);
                throw;
            }
            finally
            {
                this.singleAccessLock.Release();
            }
        }

        [DataContract]
        private class ScenarioNotificationData
        {
            public ScenarioNotificationData(ScenarioData scenarioData, NotifyFilter filter)
            {
                Assert.IsNotNull(scenarioData, "scenarioData");
                Assert.IsNotNull(filter, "filter");
                this.ScenarioData = scenarioData;
                this.Filter = filter;
            }

            [DataMember]
            public ScenarioData ScenarioData { get; private set; }

            [DataMember]
            public NotifyFilter Filter { get; private set; }

            /// <inheritdoc />
            public override string ToString()
            {
                return string.Format(CultureInfo.InvariantCulture, "NotifyFilter: {0}, Data: {1}", this.Filter, this.ScenarioData);
            }
        }

        #endregion Private_Abstraction
    }
}
// ----------------------------------------------------------------------
//  <copyright file="BaseCallbackStore.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace ClusterInsight.InsightCore.DataSetLayer.CallbackStore
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Globalization;
    using System.Linq;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Threading.Tasks.Dataflow;
    using Cluster.ClusterQuery;
    using ClusterInsight.Common;
    using ClusterInsight.Common.Log;
    using ClusterInsight.Common.Runtime;
    using ClusterInsight.Common.Store;
    using ClusterInsight.Common.Util;
    using ClusterInsight.InsightCore.DataSetLayer.DataModel;
    using ClusterInsight.InsightCore.DataSetLayer.Exceptions;
    using ClusterInsight.InsightCore.Helpers;

    /// <summary>
    /// Base implementation for different types of stores.
    /// </summary>
    internal abstract class BaseCallbackStore : DelayedStore, ICallbackStore
    {
        private const string SignalSeenTillTimeStoreName = "SignalSeenTillTimeStore";

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

        private IPersistentStore<int> reportedSignalsUniqueIdsStore;

        private IPersistentStore<DateTime> signalSeenTillThisTimeStore;

        private IPersistentStore<string> signalsInDispatchQueuePersistentStore;

        private BufferBlock<ScenarioNotificationData> producerConsumerBufferBlock;

        private ITaskRunner taskRunner;

        /// <summary>
        /// </summary>
        /// <param name="runtime"></param>
        /// <param name="token"></param>
        protected BaseCallbackStore(IInsightRuntime runtime, CancellationToken token)
        {
            this.singleAccessLock = new SemaphoreSlim(1);
            this.callbackMap = new Dictionary<NotifyFilter, IList<Func<ScenarioData, Task>>>();
            this.Runtime = runtime;
            string typeName = this.GetType().ToString();
            this.Logger = this.Runtime.GetLogProvider().CreateLoggerInstance(typeName.Split('.').Last());
            this.taskRunner = runtime.TaskRunner;

            this.InternalTokenSource = CancellationTokenSource.CreateLinkedTokenSource(token);
            token = this.InternalTokenSource.Token;

            this.Query = this.Runtime.GetService(typeof(IClusterQuery)) as IClusterQuery;
            Assert.IsNotNull(this.Query, "Runtime doesn't contain valid Cluster Query object");

            // Some signals may not show up immedietly so we don't want their last seen time bailing on us.
            this.signalSeenTillThisTimeStore = this.Runtime.GetStoreProvider()
                .CreatePersistentStoreForTimeAsync(SignalSeenTillTimeStoreName, AgeBasedRetentionPolicy.TwoWeek, token).GetAwaiter().GetResult();

            // This store keeps track of unique Ids of signals seen to avoid duplicate reporting. The entries in this
            // collection needs to be long lived. Consider a signal that is living in the system for a long time
            // If we don't track that this has been reported, we may try to report it again.
            this.reportedSignalsUniqueIdsStore = this.Runtime.GetStoreProvider()
                .CreatePersistentStoreForIntAsync(ReportedSignalUniqueIdsStoreName, AgeBasedRetentionPolicy.OneWeek, token).GetAwaiter().GetResult();

            this.signalsInDispatchQueuePersistentStore = this.Runtime.GetStoreProvider()
                .CreatePersistentStoreForStringsAsync(SignalsInDispatchQueueStoreName, AgeBasedRetentionPolicy.OneHour, token).GetAwaiter().GetResult();

            this.producerConsumerBufferBlock = new BufferBlock<ScenarioNotificationData>();
            this.InitSignalSeenBookmarkStoreAsync(token).Wait(token);
        }

        #region Protected_Props

        protected CancellationTokenSource InternalTokenSource { get; }

        /// <summary>
        /// Runtime
        /// </summary>
        protected IInsightRuntime Runtime { get; }

        /// <summary>
        /// Logger Object
        /// </summary>
        protected ILogger Logger { get; }

        /// <summary>
        /// Query object to query cluster
        /// </summary>
        protected IClusterQuery Query { get; }

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
                    this.newEventSearcherTask = this.taskRunner.Run(() => this.SearchForNewSignalsInClusterAsync(token), this.InternalTokenSource.Token);
                }

                if (this.eventDispatcherTask == null)
                {
                    this.Logger.LogMessage("RegisterCallbackAsync:: Kicking off Dispatcher Task");

                    // kick off a task that continuously looks for new signals seen and invoke callbacks associated with them.
                    this.eventDispatcherTask = this.taskRunner.Run(() => this.DispatchSignalsAsync(token), this.InternalTokenSource.Token);
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

            this.Logger.LogMessage("Remove : Got Lock");

            try
            {
                if (!this.callbackMap.ContainsKey(filter))
                {
                    throw new CallbackNotRegisteredException(string.Format(CultureInfo.InvariantCulture, "Callback for Filter: {0} not registered", filter));
                }

                if (!this.callbackMap[filter].Contains(callbackRoutine))
                {
                    throw new CallbackNotRegisteredException(string.Format(CultureInfo.InvariantCulture, "Callback for Filter: {0} with Delegage: {1} not registered", filter, callbackRoutine));
                }

                this.Logger.LogMessage("Remove : Removing Now");
                this.callbackMap[filter].Remove(callbackRoutine);

                // if there are no callbacks for this filter, remove the entry from map.
                if (!this.callbackMap[filter].Any())
                {
                    this.callbackMap[filter] = null;
                    this.callbackMap.Remove(filter);

                    this.Logger.LogMessage("Remove : Emptying Nulling");
                }
            }
            finally
            {
                this.Logger.LogMessage("Remove : Releasing Lock");
                this.singleAccessLock.Release();
            }
        }

        /// <inheritdoc />
        public async Task ResetAsync(CancellationToken token)
        {
            await this.singleAccessLock.WaitAsync(token).ConfigureAwait(false);

            this.Logger.LogMessage("Remove : Got Lock");

            try
            {
                this.Logger.LogMessage("Remove : Clearing the callback Collection");

                this.callbackMap.Clear();

                this.Logger.LogMessage("Remove : Cancelling Internal Token");

                this.InternalTokenSource.Cancel();

                await this.eventDispatcherTask.ConfigureAwait(false);

                this.Logger.LogMessage("Remove : Finish awaiting for event dispatcher");

                this.eventDispatcherTask = null;

                await this.newEventSearcherTask.ConfigureAwait(false);

                this.Logger.LogMessage("Remove : Finished Awaiting for new Searcher");

                this.newEventSearcherTask = null;

                await this.reportedSignalsUniqueIdsStore.ClearAsync(token).ConfigureAwait(false);
                await this.signalSeenTillThisTimeStore.ClearAsync(token).ConfigureAwait(false);
                await this.signalsInDispatchQueuePersistentStore.ClearAsync(token).ConfigureAwait(false);

                await this.ResetInternalAsync(token);
            }
            finally
            {
                this.Logger.LogMessage("Remove : Releasing Lock");
                this.singleAccessLock.Release();
            }
        }

        #endregion Public_Abstractions

        #region Protected_Abstractions

        /// <summary>
        /// Child classes can do reset in this method.
        /// </summary>
        /// <returns></returns>
        protected abstract Task ResetInternalAsync(CancellationToken token);

        /// <summary>
        /// Get signals matching the specific scenario
        /// </summary>
        /// <param name="scenario"></param>
        /// <param name="duration"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        protected abstract Task<IEnumerable<FabricEvent>> GetSignalsForScenarioAsync(Scenario scenario, Duration duration, CancellationToken token);

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
            var signalsFromLastRunNotYetDispatched = await this.GetUndispatchedSignalsFromLastRun(token).ConfigureAwait(false);
            if (signalsFromLastRunNotYetDispatched.Any())
            {
                this.Logger.LogMessage(
                    "Remove:: SearchForNewSignalsInClusterAsync {0} signals from last run. Posting them",
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

                // Persist these signals. This comes in handy if before dispatching, process crashes.
                // The dispatch queue will clear the signals once they are dispatched.
                // Before dispatching, we sort the signals in order they were seen.
                foreach (var oneData in newSignals.OrderBy(item => item.ScenarioData.EventSeenTime))
                {
                    this.Logger.LogMessage("Remove:: Signal Seen : {0}", oneData);
                    await StoreHelper.SaveObjectInStoreAsync(
                        this.signalsInDispatchQueuePersistentStore,
                        oneData.ScenarioData.GetUniqueIdentity().ToString(),
                        oneData,
                        token);
                }

                await this.PostNewSignalToDispatherQueueAsync(newSignals, token).ConfigureAwait(false);

                await Task.Delay(this.GetDelayBetweenCheckForNewSignalAvailability(), token).ConfigureAwait(false);
            }
        }

        // This routine reads from the undispatched persistent store and return all the values present there.
        private async Task<IList<ScenarioNotificationData>> GetUndispatchedSignalsFromLastRun(CancellationToken token)
        {
            var undispatchDataList = new List<ScenarioNotificationData>();
            var allDataSerialized = await this.signalsInDispatchQueuePersistentStore.GetAllValuesAsync(token).ConfigureAwait(false);

            foreach (var oneVal in allDataSerialized.Values)
            {
                var data = InsightsUtil.DeSerialize<ScenarioNotificationData>(oneVal);
                this.Logger.LogMessage("Remove:: GetUndispatchedSignalsFromLastRun -> Undispatched Signal {0}", data);
                undispatchDataList.Add(data);
            }

            return undispatchDataList;
        }

        private async Task<IList<ScenarioNotificationData>> GetNewSignalsSeenAsync(CancellationToken token)
        {
            IList<ScenarioNotificationData> dataResult = new List<ScenarioNotificationData>();
            foreach (var filter in await this.GetAllNotifyFiltersAsync(token).ConfigureAwait(false))
            {
                this.Logger.LogMessage("Remove:: Searching for filter : {0}", filter);
                token.ThrowIfCancellationRequested();
                var newScenarioDataToShare = await this.GetNewSignalsMatchingGivenFilterAsync(filter, token).ConfigureAwait(false);

                foreach (var oneData in newScenarioDataToShare)
                {
                    dataResult.Add(new ScenarioNotificationData(oneData, filter));
                }

                if (newScenarioDataToShare.Any())
                {
                    // TODO: This may not be most accurate when the scenario represent expiring of an event. For now
                    // Its ok as this doesn't have much of a side effect but some thinking required here.
                    await this.SetTimeWhenSignalLastSeenForScenarioAsync(filter.TargetScenario, newScenarioDataToShare.Last().FabricEvent.OriginTime, token);
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

                this.Logger.LogMessage("Remove:: DispatchSignalsAsync Dispatching : {0}", dispatchData.ScenarioData);

                if (await this.IsSignalAlreadyReportedAsync(dispatchData.ScenarioData, token))
                {
                    this.Logger.LogMessage("Remove:: DispatchSignalsAsync Already Reported {0}", dispatchData.ScenarioData);
                    continue;
                }

                // Mark the signal as Reported. This help us protect from duplicate reporting.
                await this.RecordThatSignalReportedAsync(dispatchData.ScenarioData, token).ConfigureAwait(false);

                // Remove the reported data from the persistent queue. There is still a tiny window where a signal could be lost
                // i.e. between this removal and the before the signal actually get dispatched. But that's ok.
                // This is to protect against the scenario where handler keeps on failing repeatedly and if that happens
                // and if we don't record signal as reported, we will never make forward progress.
                await this.signalsInDispatchQueuePersistentStore.RemoveEntityIfPresentAsync(dispatchData.ScenarioData.GetUniqueIdentity().ToString(), token)
                    .ConfigureAwait(false);

                var callbacks = await this.GetCallbacksForFilterAsync(dispatchData.Filter, token).ConfigureAwait(false);

                var taskList = callbacks.Select(callback => callback(dispatchData.ScenarioData)).ToList();

                await Task.WhenAll(taskList).ConfigureAwait(false);

                this.Logger.LogMessage("Remove:: DispatchSignalsAsync Done Dispatching");
            }
        }

        /// <summary>
        /// Get new signals from store that matches the given filter
        /// </summary>
        /// <param name="filter"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        private async Task<IEnumerable<ScenarioData>> GetNewSignalsMatchingGivenFilterAsync(NotifyFilter filter, CancellationToken token)
        {
            var allSignalsSeen = await this.GetSignalsForScenarioAsync(
                filter.TargetScenario,
                await this.GetDurationToLookForNewSignalsForScenarioAsync(filter.TargetScenario, token).ConfigureAwait(false),
                token).ConfigureAwait(false);

            var newSignalsMatchingFilter = new List<ScenarioData>();
            foreach (var oneSignal in allSignalsSeen.OrderBy(signal => signal.OriginTime))
            {
                // Encapsulate the new information into a shareable abstraction. We mark the event seen time as current time
                // which may not be completely accurate but it's close enough for all purposes.
                var scenarioData = new ScenarioData(filter.TargetScenario, oneSignal, DateTime.UtcNow);

                if (DateTime.UtcNow - oneSignal.OriginTime > filter.SignalAvailableForAtLeastDuration)
                {
                    if (!filter.NotifyOnlyForUnique || !await this.IsSignalAlreadyReportedAsync(scenarioData, token).ConfigureAwait(false))
                    {
                        newSignalsMatchingFilter.Add(scenarioData);
                    }
                }
            }

            return newSignalsMatchingFilter;
        }

        private async Task<Duration> GetDurationToLookForNewSignalsForScenarioAsync(Scenario targetScenario, CancellationToken token)
        {
            // Always query from last signal seen - max signal delay. This is to account for the cases where some nodes have already pushed latest data
            // but others are lagging behind. In such a case if we just start looking from time of last signal seen, we may miss data from slow nodes.
            // Our logic to keep track of reported signal will ensure that we don't end up analyzing seen incident twice
            var startTime = await this.GetTimeWhenSignalLastSeenForScenarioAsync(targetScenario, token).ConfigureAwait(false) - this.GetStoreIsBehindByDuration();
            var endTime = DateTime.UtcNow;

            // We don't want to ever look back for signals for duration > predefined value for optimization reason.
            if (endTime - startTime <= MaxTimeBackToLookForNewSignals)
            {
                return new Duration(startTime, endTime);
            }

            startTime = endTime - MaxTimeBackToLookForNewSignals;
            return new Duration(startTime, endTime);
        }

        private Task<bool> IsSignalAlreadyReportedAsync(ScenarioData signal, CancellationToken token)
        {
            return this.reportedSignalsUniqueIdsStore.IsKeyPresentAsync(signal.GetUniqueIdentity().ToString(), token);
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
            if (await this.reportedSignalsUniqueIdsStore.IsKeyPresentAsync(identifier.ToString(), token).ConfigureAwait(false))
            {
                throw new DuplicateSignalRecordException(string.Format(CultureInfo.InvariantCulture, "Already Reported: Id: {0}, Signal: {1}", identifier, signal));
            }

            // We need a list which we don't have, so using same (unique) key as value.
            await this.reportedSignalsUniqueIdsStore.SetEntityAsync(identifier.ToString(), identifier, token).ConfigureAwait(false);
        }

        /// <summary>
        /// Initialize last time signal was seen
        /// </summary>
        /// <remarks>
        /// Initial value is obtained by subtracting the period store is running behind from current time.
        /// </remarks>
        private async Task InitSignalSeenBookmarkStoreAsync(CancellationToken token)
        {
            foreach (var scenario in this.GetCallbackSupportedScenario())
            {
                // If not present in store, go ahead add the entry in store.
                if (!await this.signalSeenTillThisTimeStore.IsKeyPresentAsync(scenario.ToString(), token).ConfigureAwait(false))
                {
                    this.Logger.LogMessage(
                        "InitSignalSeenBookmarkStore:: For Scenario '{0}', No cached Start. Setting it to: '{1}'",
                        scenario,
                        DateTime.UtcNow);
                    this.SetTimeWhenSignalLastSeenForScenarioAsync(scenario, DateTime.UtcNow, token).Wait(token);
                }

                var startTime = await this.signalSeenTillThisTimeStore.GetEntityAsync(scenario.ToString(), token).ConfigureAwait(false);
                this.Logger.LogMessage(
                    "InitSignalSeenBookmarkStore:: For Scenario '{0}', we will look for new Signals Starting Time: '{1}'",
                    scenario,
                    startTime);
            }
        }

        private async Task SetTimeWhenSignalLastSeenForScenarioAsync(Scenario scenario, DateTime latestSeenTime, CancellationToken token)
        {
            await this.signalSeenTillThisTimeStore.SetEntityAsync(scenario.ToString(), latestSeenTime, token).ConfigureAwait(false);
        }

        private async Task<DateTime> GetTimeWhenSignalLastSeenForScenarioAsync(Scenario scenario, CancellationToken token)
        {
            if (!await this.signalSeenTillThisTimeStore.IsKeyPresentAsync(scenario.ToString(), token).ConfigureAwait(false))
            {
                throw new SignalTimeBookmarkMissingException(string.Format(CultureInfo.InvariantCulture, "Last Signal Time for scenario '{0}' Not present", scenario));
            }

            return await this.signalSeenTillThisTimeStore.GetEntityAsync(scenario.ToString(), token).ConfigureAwait(false);
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
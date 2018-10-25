// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Chaos
{
    using System.Collections.Generic;
    using System.Fabric.Chaos.Common;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.FaultAnalysis.Service.Chaos;
    using System.Fabric.FaultAnalysis.Service.Common;
    using System.Fabric.JsonSerializerImpl;
    using System.Fabric.Strings;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Collections;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using Newtonsoft.Json.Linq;

    internal class ChaosMessageProcessor
    {
        private const string TraceType = "Chaos.ChaosMessageProcessor";
        private const double AutoStopTimerCallbackPeriodInSeconds = 1.0;

        private readonly FabricClient fabricClient;
        private readonly CancellationToken cancellationToken;
        private readonly int maxEventCount;
        private readonly IStatefulServicePartition partition;

        /// <summary>
        /// With default configuration, there are 2 iterations per minute, during an iteration at most 5 events
        /// are generated, an event is at most 4 KB in size; so allowing Chaos to run for 2 days, we would produce
        /// less than 30000 events (less than 115 MB)
        /// </summary>
        private readonly int truncatePeriodInSeconds;
        private readonly object timerSyncBlock = new object();
        private readonly object autoStopTimerSyncBlock = new object();
        private readonly int telemetryReportPeriodInSeconds;
        private readonly object telemetryReportTimerSyncBlock = new object();
        private readonly SemaphoreSlim stopChaosSemaphore; // every call to register a stop chaos event must be guarded to prevent race between user, chaos and schedule

        private volatile Timer autoStopTimer;
        private Fabric.Common.TimeoutHelper autoStopTimeoutHelper;
        private Timer truncateTimer;
        private CancellationTokenSource aggregateCancellationTokenSource;
        private CancellationTokenSource stopCancellationTokenSource;
        private Task startChaosTask;
        private Timer telemetryReportTimer;
        private double chaosSnapshotTelemetrySamplingProbability;

        private ChaosScheduler chaosScheduler;

        public ChaosMessageProcessor(
            IReliableStateManager stateManager,
            int testMode,
            int truncatePeriodInSeconds,
            int maxEventsInDictionary,
            double chaosSnapshotTelemetrySamplingProbability,
            int chaostelemetryReportPeriodInSeconds,
            IStatefulServicePartition partition,
            FabricClient fabricClient,
            ChaosScheduler chaosScheduler,
            CancellationToken cancellationToken)
        {
            this.StateManager = stateManager;
            this.fabricClient = fabricClient;
            this.IsTestMode = ((TestMode)testMode).HasFlag(TestMode.ChaosTestMode);
            this.cancellationToken = cancellationToken;
            this.maxEventCount = this.IsTestMode
                                            ? FASConstants.TestMaxNumberOfEventsInDictionary
                                            : maxEventsInDictionary;
            this.partition = partition;
            this.truncatePeriodInSeconds = this.IsTestMode
                                            ? FASConstants.TestTuncationPeriodInSeconds
                                            : truncatePeriodInSeconds;

            this.telemetryReportPeriodInSeconds = this.IsTestMode
                ? FASConstants.TestTelemetryReportPeriodInSeconds
                : chaostelemetryReportPeriodInSeconds;

            this.chaosSnapshotTelemetrySamplingProbability = chaosSnapshotTelemetrySamplingProbability;

            this.StartTelemetryReportTimer(cancellationToken);

            this.stopChaosSemaphore = new SemaphoreSlim(1, 1);
            this.chaosScheduler = chaosScheduler;
        }

        /// <summary>
        /// Holds the parameters that the user passed in as input
        /// </summary>
        public IReliableDictionary<string, byte[]> ParametersDict { get; private set; }

        /// <summary>
        /// This holds the timestamp of the started event, to find this information
        /// fast so that on resume, time to run can be updated accordingly
        /// </summary>
        public IReliableDictionary<string, byte[]> StartTimeDict { get; private set; }

        /// <summary>
        /// This holds the latest event that went into the EventsDictioary (mostly to facilitate fast retrieval of this special event)
        /// </summary>
        public IReliableDictionary<string, byte[]> LastEventKeyDict { get; private set; }

        /// <summary>
        /// This holds all events keyed by their timestamp tick count
        /// </summary>
        public IReliableDictionary<long, byte[]> EventsDict { get; private set; }

        /// <summary>
        /// Holds the current task that Chaos test is performing, keyed by Constants.ChaosLastKeyDictionaryName
        /// </summary>
        public IReliableDictionary<string, byte[]> StatusDictionary { get; private set; }

        public IReliableDictionary<string, byte[]> ReportFilterDictionary { get; private set; }

        public IReliableDictionary<string, byte[]> ChaosSchedulerStatusDictionary { get; private set; }

        public IReadOnlyDictionary<string, byte[]> ChaosFoundEventPayloadForTelemetryDictionary { get; private set; }

        public ChaosParameters ChaosParameters { get; private set; }

        public IReliableStateManager StateManager { get; private set; }

        public bool IsTestMode { get; private set; }

        public async Task ProcessStartChaosOldAsync(ChaosParameters chaosParameters, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo("StartTrek", "Enter ProcessStartChaosOldAsync, datetimeutc={0} with chaosParameters={1}", DateTime.UtcNow, chaosParameters.ToString());

            await ChaosUtil.ValidateChaosTargetFilterAsync(
                this.fabricClient,
                chaosParameters,
                ChaosConstants.RequestTimeoutDefault,
                ChaosConstants.OperationTimeoutDefault,
                cancellationToken).ConfigureAwait(false);

            this.ProcessReservedKeysInContext(chaosParameters);

            bool shouldStart = await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                Guid.Empty,
                () => this.ShouldStartAsync(),
                this.partition,
                "ShouldStartAsync",
                FASConstants.MaxRetriesForReliableDictionary,
                this.cancellationToken);

            if (!shouldStart)
            {
                return;
            }

            // Save parameters in the dict, create a started event and save it in the event dict, save the start time and last event key
            this.ParametersDict = this.ParametersDict ??
                await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosParametersTableName).ConfigureAwait(false);

            // Save the passed in parameters
            byte[] chaosParametersInBytes = chaosParameters.ToBytes();

            await this.ParametersDict.AddOrUpdateAsync<string>(
                this.StateManager,
                FASConstants.ChaosParametersKey,
                chaosParametersInBytes,
                this.partition,
                this.cancellationToken).ConfigureAwait(false);

            // Now hold it in an instance variable, so that in StartChaosAsync we can use this.ChaosParameters
            this.ChaosParameters = chaosParameters;

            StartedEvent startedEvent = new StartedEvent(
                DateTime.UtcNow,
                chaosParameters);

            await this.StateManager.RegisterChaosEventAndUpdateChaosStatusAsync(
                startedEvent,
                ChaosStatus.Running,
                this.partition,
                this.cancellationToken,
                async () =>
                {
                    // We need to clear the next iteration rd because that
                    // retains state from past iterations
                    await this.StateManager.RemoveKeyFromReliableDictionaryAsync(
                        FASConstants.NextItearationTimeStampRDName,
                        FASConstants.NextItearationTimeStampKey,
                        this.partition,
                        this.cancellationToken);
                    FabricEvents.Events.ChaosStart(
                        TraceType,
                        startedEvent.TimeStampUtc.Ticks,
                        chaosParameters.MaxConcurrentFaults,
                        chaosParameters.TimeToRun.TotalSeconds,
                        chaosParameters.MaxClusterStabilizationTimeout.TotalSeconds,
                        chaosParameters.WaitTimeBetweenIterations.TotalSeconds,
                        chaosParameters.WaitTimeBetweenFaults.TotalSeconds);

                    ChaosTraceEventWriter.WriteChaosStartedTraceEvent(chaosParameters);
                }).ConfigureAwait(false);

            // Save current time as "StartTime"
            this.StartTimeDict = await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.StartTimeTableName).ConfigureAwait(false);
            byte[] currentTimeInBytes = BitConverter.GetBytes(startedEvent.TimeStampUtc.Ticks);
            await this.StartTimeDict.AddOrUpdateAsync<string>(this.StateManager, FASConstants.StartTimeKey, currentTimeInBytes, this.partition, this.cancellationToken).ConfigureAwait(false);

            await this.StartChaosAsync(this.ChaosParameters.TimeToRun, this.cancellationToken).ConfigureAwait(false);
        }

        public async Task ProcessStartChaosAsync(ChaosParameters chaosParameters, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo("StartTrek", "Enter ProcessStartChaosAsync, datetimeutc={0}, with ChaosParameters={1}", DateTime.UtcNow, chaosParameters.ToString());

            var startTime = DateTime.UtcNow;
            var endTime = startTime.Add(chaosParameters.TimeToRun);

            var chaosParametersDictionary = new Dictionary<string, ChaosParameters>();
            chaosParametersDictionary.Add(ChaosConstants.ChaosScheduler_DefaultParameterKey, chaosParameters);

            List<ChaosScheduleTimeRangeUtc> times = new List<ChaosScheduleTimeRangeUtc>() { ChaosScheduleTimeRangeUtc.WholeDay };
            var everyDay = new HashSet<DayOfWeek>() { DayOfWeek.Sunday, DayOfWeek.Monday, DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Thursday, DayOfWeek.Friday, DayOfWeek.Saturday };
            ChaosScheduleJobActiveDays activeEveryday = new ChaosScheduleJobActiveDays(everyDay);
            ChaosScheduleJob job = new ChaosScheduleJob(ChaosConstants.ChaosScheduler_DefaultParameterKey, activeEveryday, times);
            var chaosScheduleJobs = new List<ChaosScheduleJob>() { job };

            ChaosSchedule schedule = new ChaosSchedule(startTime, endTime, chaosParametersDictionary, chaosScheduleJobs);

            ChaosScheduleDescription chaosScheduleDescription = new ChaosScheduleDescription(0, schedule);

            var combinedCancellationTokenSource = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, this.cancellationToken);

            await this.chaosScheduler.SetScheduleAsync(chaosScheduleDescription, true, combinedCancellationTokenSource.Token).ConfigureAwait(false);
        }

        public async Task ProcessStopChaosOldAsync(bool initiatedBySystem = false)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter ProcessStopChaosOldAsync");

            this.stopChaosSemaphore.Wait(this.cancellationToken);
            try
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "StopChaosAPI: stopChaosSemaphore acquired, current count = {0}", this.stopChaosSemaphore.CurrentCount);

                bool shouldStop = await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                                    Guid.Empty,
                                    () => this.StateManager.ChaosWasRunningAsync(this.partition, this.cancellationToken),
                                    this.partition,
                                    "ShouldStopAsync",
                                    FASConstants.MaxRetriesForReliableDictionary,
                                    this.cancellationToken).ConfigureAwait(false);

                if (!shouldStop)
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "Should not stop, so just return");

                    return;
                }

                if (this.stopCancellationTokenSource.IsCancellationRequested)
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "ProcessStopChaosOldAsync: stopCancellationTokenSource already cancellation requested");
                    return;
                }

                // Signal cancellation token, save StoppedEvent in EventDict, and save key in LastEventKey dict
                this.EventsDict = this.EventsDict ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<long, byte[]>>(FASConstants.ChaosEventsDictionaryName).ConfigureAwait(false);
                this.StatusDictionary = this.StatusDictionary ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosStatusDictionaryName).ConfigureAwait(false);

                try
                {
                    string reason;
                    if (initiatedBySystem)
                    {
                        reason = StringResources.ChaosInfo_TimeToRunIsUp;
                    }
                    else
                    {
                        reason = StringResources.ChaosInfo_UserInitiatedStop;
                    }

                    StoppedEvent stoppedEvent = new StoppedEvent(DateTime.UtcNow, reason);
                    await this.StateManager.RegisterChaosEventAndUpdateChaosStatusAsync(
                        stoppedEvent,
                        ChaosStatus.Stopped,
                        this.partition,
                        this.cancellationToken,
                        () =>
                        {
                            // We need to clear the next iteration rd because that
                            // retains state from past iterations
                            this.StateManager.RemoveKeyFromReliableDictionaryAsync(
                                FASConstants.NextItearationTimeStampRDName,
                                FASConstants.NextItearationTimeStampKey,
                                this.partition,
                                this.cancellationToken).GetAwaiter().GetResult();
                            FabricEvents.Events.ChaosStop(ChaosUtil.ChaosTelemetryStopEventId, stoppedEvent.TimeStampUtc.Ticks, initiatedBySystem ? 0 : 1);
                            ChaosTraceEventWriter.WriteChaosStoppedTraceEvent(initiatedBySystem ? ChaosStopReason.TimeToRunOver : ChaosStopReason.ClientApi);
                            this.stopCancellationTokenSource.Cancel();
                            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Cancellation signaled due to StopChaos API.", DateTime.UtcNow);
                        }).ConfigureAwait(false);
                }
                catch (Exception e)
                {
                    // OperationCanceledException below - It may be nothing was committed, so this should throw a retryable error.  This ends up as a FabricTransientException on client side.
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "Task got cancelled in StopChaos. Exception e = {0}", e.ToString());
                    FaultAnalysisServiceUtility.ThrowTransientExceptionIfRetryable(e);

                    throw;
                }
                finally
                {
                    this.startChaosTask = null;
                }
            }
            finally
            {
                this.stopChaosSemaphore.Release();
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "StopChaosAPI: stopChaosSemaphore released");

                if (this.stopCancellationTokenSource == null)
                {
                   TestabilityTrace.TraceSource.WriteInfo(TraceType, "StopChaosAPI: the stopCancellationTokenSource is null");
                }
                else
                {
                   TestabilityTrace.TraceSource.WriteInfo(TraceType, "StopChaosAPI: the stopCancellationTokenSource.IsCancellationRequested = {0}", this.stopCancellationTokenSource.IsCancellationRequested);
                }
            }
        }

        public async Task ProcessStopChaosAsync(CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter ProcessStopChaosAsync");

            var aggregateCancellationTokenSource = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, this.cancellationToken);

            await this.chaosScheduler.StopChaosAsync(aggregateCancellationTokenSource.Token);
        }

        public async Task<string> ProcessGetChaosAsync(
            string inputBlob,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter ProcessGetChaosAsync");

            var combinedCancellationTokenSource = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, this.cancellationToken);

            Guid activityId = Guid.NewGuid();
            var schedulerJson = await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure<string>(
                activityId,
                () => this.ProcessGetChaosAsyncPrivate(combinedCancellationTokenSource.Token),
                this.partition,
                "ProcessGetChaosAsyncPrivate",
                FASConstants.MaxRetriesForReliableDictionary,
                combinedCancellationTokenSource.Token).ConfigureAwait(false);

            return schedulerJson;
        }

        public async Task<ChaosReport> ProcessGetChaosReportAsync(
            Guid activityId,
            ChaosReportFilter reportFilter,
            string continuationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter ProcessGetChaosReportAsync, using ChaosReportFilter={0}", reportFilter.ToString());

            var report = await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure<ChaosReport>(
                activityId,
                () => this.ProcessGetChaosReportAsyncPrivate(reportFilter, continuationToken),
                this.partition,
                "ProcessGetChaosReportAsyncPrivate",
                FASConstants.MaxRetriesForReliableDictionary,
                this.cancellationToken).ConfigureAwait(false);

            return report;
        }

        public async Task<string> ProcessGetChaosEventsAsync(
            string inputblob,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter ProcessGetEventsAsync, using ChaosReportFilter={0}", inputblob);

            Guid activityId = Guid.NewGuid();
            var combinedCancellationTokenSource = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, this.cancellationToken);
            string eventsJson = string.Empty;

            var eventsDescription = ChaosUtil.DeserializeChaosEventsDescriptionFromNative(inputblob);

            TestabilityTrace.TraceSource.WriteInfo("StartTrek", "Enter ProcessGetChaosEventsAsync, input events description = {0} output to = {1}", inputblob, eventsDescription.ToString());

            try
            {
                // get the filter by searching through the filter dictionary or creating a new one based on the inputblob
                ChaosReportFilter reportFilter = null;
                var continuationToken = eventsDescription.ContinuationToken;

                if (!string.IsNullOrEmpty(continuationToken))
                {
                    reportFilter = await this.GetReportFilterAsync(activityId, continuationToken).ConfigureAwait(false);
                }
                else
                {
                    // report filter null in this case so getChaosReportDescription.Filter will be taken later
                    var continuationTokenGuid = Guid.NewGuid().ToString();
                    var fileTimeUtcTicks = DateTime.UtcNow.ToFileTimeUtc();

                    continuationToken = string.Format(FASConstants.ChaosReportContinuationTokenFormat, fileTimeUtcTicks, continuationTokenGuid);
                }

                reportFilter = reportFilter ?? new ChaosReportFilter(eventsDescription.Filter.StartTimeUtc, eventsDescription.Filter.EndTimeUtc);

                // if MaxResults is null or 0 then we will treat it as returning "everything" which will be long.MaxValue
                // a limit with how big the response can be will be reached long before this limit is reached.
                var resultLimit = eventsDescription.MaxResults.HasValue ? eventsDescription.MaxResults.Value : 0;
                if (resultLimit == 0)
                {
                    resultLimit = long.MaxValue;
                }

                if (!string.IsNullOrEmpty(eventsDescription.ClientType)
                    && (eventsDescription.ClientType.Equals(ChaosConstants.RestClientTypeName, StringComparison.OrdinalIgnoreCase)
                        || eventsDescription.ClientType.Equals(ChaosConstants.NativeClientTypeName, StringComparison.OrdinalIgnoreCase)))
                {
                    ChaosUtility.DisableOptimizationForValidationFailedEvent = true;
                }
                else
                {
                    ChaosUtility.DisableOptimizationForValidationFailedEvent = false;
                }

                // get the events and serialize to a json string
                var events = await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure<ChaosEventsSegment>(
                    activityId,
                    () => this.ProcessGetChaosEventsAsyncPrivate(reportFilter, continuationToken, resultLimit),
                    this.partition,
                    "ProcessGetChaosEventsAsyncPrivate",
                    FASConstants.MaxRetriesForReliableDictionary,
                    combinedCancellationTokenSource.Token).ConfigureAwait(false);

                eventsJson = ChaosUtil.SerializeChaosEvents(events);

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0}:ProcessGetChaosEventsAsync - serialized ChaosEvents as: {1}", activityId, eventsJson);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0}:ProcessGetChaosEventsAsync - Exception occurred: {1}", activityId, e.Message);
                FaultAnalysisServiceUtility.ThrowTransientExceptionIfRetryable(e);

                throw;
            }

            TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0}:ProcessGetChaosEventsAsync - exiting with eventsJson as: {1}", activityId, eventsJson);
            return eventsJson;
        }

        public async Task<string> ProcessPostChaosScheduleAsync(
            string inputBlob,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter ProcessPostChaosScheduleAsync with input {0}", inputBlob);

            var combinedCancellationTokenSource = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, this.cancellationToken);

            Guid activityId = Guid.NewGuid();
            string result = await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure<string>(
                activityId,
                () => this.ProcessPostChaosScheduleAsyncPrivate(inputBlob, timeout, combinedCancellationTokenSource.Token),
                this.partition,
                "ProcessPostChaosScheduleAsyncPrivate",
                FASConstants.MaxRetriesForReliableDictionary,
                combinedCancellationTokenSource.Token).ConfigureAwait(false);

            return string.Empty;
        }

        public async Task<string> ProcessGetChaosScheduleAsync(
            string inputBlob,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter ProcessGetChaosScheduleAsync");

            Guid activityId = Guid.NewGuid();
            var combinedCancellationTokenSource = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, this.cancellationToken);

            var schedulerJson = await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure<string>(
                activityId,
                () => this.ProcessGetChaosScheduleAsyncPrivate(),
                this.partition,
                "ProcessGetChaosScheduleAsyncPrivate",
                FASConstants.MaxRetriesForReliableDictionary,
                combinedCancellationTokenSource.Token).ConfigureAwait(false);

            return schedulerJson;
        }

        public async Task<bool> ResumeChaosAsync(CancellationToken cancellationToken)
        {
            var shouldResume = await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                                   ChaosUtil.ChaosOperationID,
                                   () => this.StateManager.ChaosWasRunningAsync(this.partition, cancellationToken),
                                   this.partition,
                                   "ShouldResumeChaosAsync",
                                   FASConstants.MaxRetriesForReliableDictionary,
                                   cancellationToken).ConfigureAwait(false);

            if (!shouldResume)
            {
                return false;
            }

            bool didResume = await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                ChaosUtil.ChaosOperationID,
                () => this.DoResumeChaosAsyncPrivate(cancellationToken),
                this.partition,
                "DoResumeChaosAsyncPrivate",
                FASConstants.MaxRetriesForReliableDictionary,
                cancellationToken).ConfigureAwait(false);

            return didResume;
        }

        internal async Task<ChaosReportFilter> GetReportFilterAsync(Guid activityId, string continuationToken)
        {
            var filter = await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure<ChaosReportFilter>(
                            activityId,
                            () => this.GetReportFilterAsyncPrivate(continuationToken),
                            this.partition,
                            "GetReportFilterAsync",
                            FASConstants.MaxRetriesForReliableDictionary,
                            this.cancellationToken).ConfigureAwait(false);

            return filter;
        }

        /// <summary>
        /// Register a stopped event and failure event explaining why Chaos is not resuming if Chaos should have resumed.
        /// </summary>
        internal async Task RegisterStoppedEventForRestartFailureAsync()
        {
            var shouldStop = await this.ShouldStopAsync().ConfigureAwait(false);
            if (shouldStop)
            {
                // No stopped event found as the most recent event. Should register it.
                // Testerror event has to come before stopped event otherwise another failure causing a
                // call to this method would result in shouldStop = true the Testerror will be the last event
                // and we will end up placing another stopped event resulting in
                // StoppedEvent, TestErrorEvent, StoppedEvent, TestErrorevent
                await this.StateManager.RegisterChaosEventAndUpdateChaosStatusAsync(
                    new TestErrorEvent(DateTime.UtcNow, StringResources.ChaosScheduler_ResetStateDueToError),
                    ChaosStatus.Stopped,
                    this.partition,
                    this.cancellationToken,
                    () =>
                    {
                        FabricEvents.Events.ChaosEngineError(
                            ChaosConstants.ChaosScheduler_StateResetOnRestart_TelemetryId,
                            StringResources.ChaosScheduler_ResetStateDueToError);
                    }).ConfigureAwait(false);

                StoppedEvent stoppedEvent = new StoppedEvent(DateTime.UtcNow, StringResources.ChaosInfo_SystemCannotRestartChaos);
                await this.StateManager.RegisterChaosEventAndUpdateChaosStatusAsync(
                    stoppedEvent,
                    ChaosStatus.Stopped,
                    this.partition,
                    this.cancellationToken,
                    () =>
                    {
                        // We need to clear the next iteration rd because that
                        // retains state from past iterations
                        this.StateManager.RemoveKeyFromReliableDictionaryAsync(
                            FASConstants.NextItearationTimeStampRDName,
                            FASConstants.NextItearationTimeStampKey,
                            this.partition,
                            this.cancellationToken).GetAwaiter().GetResult();
                        FabricEvents.Events.ChaosStop(ChaosUtil.ChaosTelemetryStopEventId, stoppedEvent.TimeStampUtc.Ticks, 0);
                        if (this.stopCancellationTokenSource != null)
                        {
                            // Really this shouldn't be happening since when FAS comes up and the resume is occuring there shouldn't be chaos already started
                            TestabilityTrace.TraceSource.WriteWarning(TraceType, "Do resume Chaos is cancelling a chaos token on resume.");
                            this.stopCancellationTokenSource.Cancel();
                            TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0}:Cancellation signalled from autostop on RegisterChaosEventAndUpdateChaosStatusAsync.", DateTime.UtcNow);
                        }
                    }).ConfigureAwait(false);
            }
        }

        internal async Task<ChaosParameters> GetChaosParametersAsync(Guid activityId)
        {
            var chaosParams = await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure<ChaosParameters>(
                activityId,
                () => this.GetChaosParametersAsyncPrivate(this.cancellationToken),
                this.partition,
                "GetChaosParametersAsync",
                FASConstants.MaxRetriesForReliableDictionary,
                this.cancellationToken).ConfigureAwait(false);

            return chaosParams;
        }

        /// <summary>
        /// Get the time that has elapsed since last start Chaos event.
        /// </summary>
        /// <param name="activityId">A guid.</param>
        /// <returns>Timespan of duration between now and previous start chaosevent. Value will be 0 if no start time is found.</returns>
        internal async Task<DateTime> GetChaosStartTimeAsync(Guid activityId)
        {
            var startTime = await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure<DateTime>(
                activityId,
                () => this.GetChaosStartTimeAsyncPrivate(this.cancellationToken),
                this.partition,
                "GetChaosStartTimeAsync",
                FASConstants.MaxRetriesForReliableDictionary,
                this.cancellationToken).ConfigureAwait(false);

            return startTime;
        }

        private async Task<DateTime> GetChaosStartTimeAsyncPrivate(CancellationToken cancellationToken)
        {
            DateTime startTime = new DateTime();

            this.StartTimeDict = this.StartTimeDict ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.StartTimeTableName).ConfigureAwait(false);

            using (var tx = this.StateManager.CreateTransaction())
            {
                var conditionalStartTime = await this.StartTimeDict.TryGetValueAsync(
                    tx,
                    FASConstants.StartTimeKey,
                    FASConstants.ReliableDictionaryTimeout,
                    cancellationToken).ConfigureAwait(false);

                if (conditionalStartTime.HasValue)
                {
                    startTime = new DateTime(BitConverter.ToInt64(conditionalStartTime.Value, 0));
                }

                await tx.CommitAsync().ConfigureAwait(false);
            }

            return startTime;
        }

        private async Task<ChaosParameters> GetChaosParametersAsyncPrivate(CancellationToken cancellationToken)
        {
            ChaosParameters chaosParams = new ChaosParameters();

            this.ParametersDict = await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosParametersTableName).ConfigureAwait(false);

            using (var tx = this.StateManager.CreateTransaction())
            {
                var parametersResult =
                    await this.ParametersDict.TryGetValueAsync(
                        tx,
                        FASConstants.ChaosParametersKey,
                        FASConstants.ReliableDictionaryTimeout,
                        cancellationToken).ConfigureAwait(false);
                if (parametersResult.HasValue)
                {
                    chaosParams = new ChaosParameters();
                    chaosParams.FromBytes(parametersResult.Value);
                }

                await tx.CommitAsync().ConfigureAwait(false);
            }

            return chaosParams;
        }

        private async Task<string> ProcessPostChaosScheduleAsyncPrivate(
            string inputBlob,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo("StartTrek", "Enter ProcessPostChaosScheduleAsyncPrivate, input schedule = {0}", inputBlob);

            ChaosScheduleDescription schedule = ChaosSchedulerUtil.DeserializeChaosScheduleDescription(inputBlob);

            await this.chaosScheduler.SetScheduleAsync(schedule, false, cancellationToken).ConfigureAwait(false);

            return string.Empty;
        }

        private async Task<string> ProcessGetChaosAsyncPrivate(CancellationToken cancellationToken)
        {
            this.ChaosSchedulerStatusDictionary = await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosSchedulerStateName).ConfigureAwait(false);
            this.ParametersDict = await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosParametersTableName).ConfigureAwait(false);
            this.StatusDictionary = await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosStatusDictionaryName).ConfigureAwait(false);

            ChaosStatus currentStatus = ChaosStatus.Running;
            ChaosParameters chaosParameters = new ChaosParameters();
            SchedulerState schedulerState = new SchedulerState(ChaosScheduleStatus.Invalid, ChaosStatus.None);

            using (ITransaction tx = this.StateManager.CreateTransaction())
            {
                currentStatus = await this.StatusDictionary.GetCurrentStatusAsync(tx, this.partition, cancellationToken).ConfigureAwait(false);

                var parametersResult = await this.ParametersDict.TryGetValueAsync(tx, FASConstants.ChaosParametersKey).ConfigureAwait(false);
                if (parametersResult.HasValue)
                {
                    chaosParameters.FromBytes(parametersResult.Value);
                }

                var schedulerStateResult = await this.ChaosSchedulerStatusDictionary.TryGetValueAsync(tx, FASConstants.ChaosSchedulerStatusDictionaryStateKey).ConfigureAwait(false);
                if (schedulerStateResult.HasValue)
                {
                    schedulerState.FromBytes(schedulerStateResult.Value);
                }

                await tx.CommitAsync().ConfigureAwait(false);
            }

            ChaosDescription chaosDescription = new ChaosDescription(currentStatus, chaosParameters, schedulerState.ScheduleStatus);

            return ChaosSchedulerUtil.SerializeChaosDescription(chaosDescription);
        }

        private async Task<ChaosReport> ProcessGetChaosReportAsyncPrivate(
            ChaosReportFilter reportFilter,
            string continuationToken)
        {
            this.ParametersDict = await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosParametersTableName).ConfigureAwait(false);

            List<ChaosEvent> eventHistory = new List<ChaosEvent>();

            this.EventsDict = this.EventsDict ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<long, byte[]>>(FASConstants.ChaosEventsDictionaryName).ConfigureAwait(false);

            ChaosParameters chaosParameters = new ChaosParameters();
            string nextContinuationToken = string.Empty;
            using (ITransaction tx = this.StateManager.CreateTransaction())
            {
                var parametersResult = await this.ParametersDict.TryGetValueAsync(tx, FASConstants.ChaosParametersKey).ConfigureAwait(false);

                if (parametersResult.HasValue)
                {
                    chaosParameters.FromBytes(parametersResult.Value);
                }
                else
                {
                    ChaosUtil.ThrowNotReady(StringResources.ChaosError_ChaosEngineNotReady);
                }

                Func<long, bool> filter = (key) => { return reportFilter.Match(key); };

                IAsyncEnumerable<KeyValuePair<long, byte[]>> enumerable =
                    await this.EventsDict.CreateEnumerableAsync(
                        tx,
                        filter,
                        EnumerationMode.Ordered).ConfigureAwait(false);

                int count = 0;
                int numberOfEventPerBatch = this.IsTestMode
                    ? FASConstants.ChaosEventBatchLengthTest
                    : FASConstants.ChaosEventBatchLength;

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "numberOfEventPerBatch={0}", numberOfEventPerBatch);

                ChaosEvent firstOfNextBatch = null;
                using (IAsyncEnumerator<KeyValuePair<long, byte[]>> enumerator = enumerable.GetAsyncEnumerator())
                {
                    while (count < numberOfEventPerBatch && await enumerator.MoveNextAsync(CancellationToken.None).ConfigureAwait(false))
                    {
                        KeyValuePair<long, byte[]> chaosEventKvp = enumerator.Current;

                        var eventToAdd = ByteSerializationHelper.GetEventFromBytes(chaosEventKvp.Value);

                        TestabilityTrace.TraceSource.WriteInfo(TraceType, string.Format("ProcessGetChaosReportAsyncPrivate: Going to Add event: {0} : {1}", chaosEventKvp.Key, eventToAdd));

                        eventHistory.Add(eventToAdd);
                        ++count;
                    }

                    if (count == numberOfEventPerBatch
                        && await enumerator.MoveNextAsync(CancellationToken.None).ConfigureAwait(false))
                    {
                        TestabilityTrace.TraceSource.WriteInfo(TraceType, "One batch is not sufficient!!!");

                        KeyValuePair<long, byte[]> chaosEventKvp = enumerator.Current;
                        firstOfNextBatch = ByteSerializationHelper.GetEventFromBytes(chaosEventKvp.Value);
                        reportFilter.StartTimeUtc = firstOfNextBatch.TimeStampUtc;

                        this.ReportFilterDictionary = this.ReportFilterDictionary
                            ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosReportFilterDictionaryName).ConfigureAwait(false);

                        if (await this.ReportFilterDictionary.ContainsKeyAsync(tx, continuationToken).ConfigureAwait(false))
                        {
                            await this.ReportFilterDictionary.SetAsync(tx, continuationToken, reportFilter.ToBytes()).ConfigureAwait(false);
                        }
                        else
                        {
                            await this.ReportFilterDictionary.AddAsync(tx, continuationToken, reportFilter.ToBytes()).ConfigureAwait(false);
                        }

                        nextContinuationToken = continuationToken;
                    }
                }

                await tx.CommitAsync().ConfigureAwait(false);
            }

            this.StatusDictionary = this.StatusDictionary ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosStatusDictionaryName).ConfigureAwait(false);

            ChaosStatus currentStatus = ChaosStatus.Running;
            using (var tx = this.StateManager.CreateTransaction())
            {
                currentStatus = await this.StatusDictionary.GetCurrentStatusAsync(tx, this.partition, this.cancellationToken).ConfigureAwait(false);
                await tx.CommitAsync().ConfigureAwait(false);
            }

            var report = new ChaosReport(
                chaosParameters,
                currentStatus,
                eventHistory,
                nextContinuationToken);

            return report;
        }

        private async Task<ChaosEventsSegment> ProcessGetChaosEventsAsyncPrivate(
            ChaosReportFilter reportFilter,
            string continuationToken,
            long resultLimit)
        {
            List<ChaosEvent> eventHistory = new List<ChaosEvent>();

            this.EventsDict = this.EventsDict ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<long, byte[]>>(FASConstants.ChaosEventsDictionaryName).ConfigureAwait(false);

            string nextContinuationToken = string.Empty;
            using (ITransaction tx = this.StateManager.CreateTransaction())
            {
                Func<long, bool> filter = (key) => { return reportFilter.Match(key); };

                IAsyncEnumerable<KeyValuePair<long, byte[]>> enumerable =
                    await this.EventsDict.CreateEnumerableAsync(
                        tx,
                        filter,
                        EnumerationMode.Ordered).ConfigureAwait(false);

                long count = 0;

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "ProcessGetChaosEventsAsyncPrivate: numberOfEventPerBatch={0}", resultLimit);

                ChaosEvent firstOfNextBatch = null;
                using (IAsyncEnumerator<KeyValuePair<long, byte[]>> enumerator = enumerable.GetAsyncEnumerator())
                {
                    while (count < resultLimit && await enumerator.MoveNextAsync(CancellationToken.None).ConfigureAwait(false))
                    {
                        KeyValuePair<long, byte[]> chaosEventKvp = enumerator.Current;

                        var eventToAdd = ByteSerializationHelper.GetEventFromBytes(chaosEventKvp.Value);

                        TestabilityTrace.TraceSource.WriteInfo(TraceType, string.Format("ProcessGetChaosEventsAsyncPrivate: Going to Add event: {0} : {1} with kind {2}", chaosEventKvp.Key, eventToAdd, eventToAdd.Kind));

                        eventHistory.Add(eventToAdd);
                        ++count;
                    }

                    if (count == resultLimit
                        && await enumerator.MoveNextAsync(CancellationToken.None).ConfigureAwait(false))
                    {
                        TestabilityTrace.TraceSource.WriteInfo(TraceType, "One batch is not sufficient!!!");

                        KeyValuePair<long, byte[]> chaosEventKvp = enumerator.Current;
                        firstOfNextBatch = ByteSerializationHelper.GetEventFromBytes(chaosEventKvp.Value);
                        reportFilter.StartTimeUtc = firstOfNextBatch.TimeStampUtc;

                        this.ReportFilterDictionary = this.ReportFilterDictionary
                            ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosReportFilterDictionaryName).ConfigureAwait(false);

                        if (await this.ReportFilterDictionary.ContainsKeyAsync(tx, continuationToken).ConfigureAwait(false))
                        {
                            await this.ReportFilterDictionary.SetAsync(tx, continuationToken, reportFilter.ToBytes()).ConfigureAwait(false);
                        }
                        else
                        {
                            await this.ReportFilterDictionary.AddAsync(tx, continuationToken, reportFilter.ToBytes()).ConfigureAwait(false);
                        }

                        nextContinuationToken = continuationToken;
                    }
                }

                await tx.CommitAsync().ConfigureAwait(false);
            }

            var eventsSegment = new ChaosEventsSegment(
                eventHistory,
                nextContinuationToken);

            return eventsSegment;
        }

        private async Task<string> ProcessGetChaosScheduleAsyncPrivate()
        {
            this.ChaosSchedulerStatusDictionary = await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosSchedulerStateName).ConfigureAwait(false);

            var scheduleDescription = new ChaosScheduleDescription();

            using (ITransaction tx = this.StateManager.CreateTransaction())
            {
                var scheduleDescriptionResult = await this.ChaosSchedulerStatusDictionary.TryGetValueAsync(tx, FASConstants.ChaosSchedulerStatusDictionaryScheduleKey).ConfigureAwait(false);
                if (scheduleDescriptionResult.HasValue)
                {
                    scheduleDescription.FromBytes(scheduleDescriptionResult.Value);
                }

                await tx.CommitAsync().ConfigureAwait(false);
            }

            return ChaosSchedulerUtil.SerializeChaosScheduleDescription(scheduleDescription);
        }

        private async Task<ChaosReportFilter> GetReportFilterAsyncPrivate(string continuationToken)
        {
            ChaosReportFilter filter = null;
            this.ReportFilterDictionary = this.ReportFilterDictionary
                ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosReportFilterDictionaryName).ConfigureAwait(false);

            using (var tx = this.StateManager.CreateTransaction())
            {
                if (await this.ReportFilterDictionary.ContainsKeyAsync(tx, continuationToken).ConfigureAwait(false))
                {
                    var conditionalFilter = await this.ReportFilterDictionary.TryGetValueAsync(tx, continuationToken).ConfigureAwait(false);

                    if (conditionalFilter.HasValue)
                    {
                        filter = ByteSerializationHelper.GetReportFilterFromBytes(conditionalFilter.Value);
                    }
                }

                await tx.CommitAsync().ConfigureAwait(false);
            }

            return filter;
        }

        // This method should only run one at a time (controlled by timer)
        private async Task TruncateAsync()
        {
            if (this.StateManager == null || this.EventsDict == null)
            {
                return;
            }

            using (var tx = this.StateManager.CreateTransaction())
            {
                // Get the count of events
                long totalEventCount = await this.EventsDict.GetCountAsync(tx).ConfigureAwait(false);

                long totalFilterCount =
                    (this.ReportFilterDictionary == null)
                    ? 0
                    : await this.ReportFilterDictionary.GetCountAsync(tx).ConfigureAwait(false);

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter TruncateAsync, count is {0}", totalEventCount);

                if (totalEventCount <= this.maxEventCount && totalFilterCount <= this.maxEventCount)
                {
                    if (this.truncateTimer != null)
                    {
                        lock (this.timerSyncBlock)
                        {
                            if (this.truncateTimer != null)
                            {
                                this.truncateTimer.Change(TimeSpan.FromSeconds(this.truncatePeriodInSeconds), Timeout.InfiniteTimeSpan);
                            }
                        }
                    }

                    return;
                }

                if (totalEventCount > this.maxEventCount)
                {
                    await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                        Guid.Empty,
                        () => this.PruneDictionaryAsync<long>(this.EventsDict, tx, totalEventCount),
                        this.partition,
                        "PruneDictionaryAsync(EventsDictionary)",
                        FASConstants.MaxRetriesForReliableDictionary,
                        this.cancellationToken).ConfigureAwait(false);
                }

                if (this.ReportFilterDictionary != null && totalFilterCount > this.maxEventCount)
                {
                    await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                        Guid.Empty,
                        () => this.PruneDictionaryAsync<string>(this.ReportFilterDictionary, tx, totalFilterCount),
                        this.partition,
                        "PruneDictionaryAsync(FiltersDictionary)",
                        FASConstants.MaxRetriesForReliableDictionary,
                        this.cancellationToken).ConfigureAwait(false);
                }

                await tx.CommitAsync().ConfigureAwait(false);
            }
        }

        private void OnTruncate(object state)
        {
            Task.Factory.StartNew(() => this.OnTruncateInnerAsync(), this.cancellationToken);
        }

        // This method should only run one at a time (controlled by timer)
        private async Task OnTruncateInnerAsync()
        {
            string localtype = TraceType + "OnTruncateInnerAsync";

            bool exceptionOccurred = false;

            try
            {
                await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                    Guid.Empty,
                    () => this.TruncateAsync(),
                    this.partition,
                    "OnTruncateInnerAsync",
                    FASConstants.MaxRetriesForReliableDictionary,
                    this.cancellationToken).ConfigureAwait(false);
            }
            catch (FabricNotPrimaryException fnp)
            {
                FaultAnalysisServiceUtility.TraceFabricNotPrimary(Guid.Empty, fnp);
                exceptionOccurred = true;
            }
            catch (FabricObjectClosedException foc)
            {
                FaultAnalysisServiceUtility.TraceFabricObjectClosed(Guid.Empty, foc);
                exceptionOccurred = true;
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteError(localtype, "Exception in OnTruncateInnerAsync: {0}", e);
                throw;
            }

            if (!exceptionOccurred)
            {
                if (this.truncateTimer != null)
                {
                    lock (this.timerSyncBlock)
                    {
                        if (this.truncateTimer != null)
                        {
                            this.truncateTimer.Change(TimeSpan.FromSeconds(this.truncatePeriodInSeconds), Timeout.InfiniteTimeSpan);
                        }
                    }
                }
            }
        }

        private async Task PruneDictionaryAsync<TKey>(
            IReliableDictionary<TKey, byte[]> dictionary,
            ITransaction tx,
            long entryCountTotal) where TKey : IComparable<TKey>, IEquatable<TKey>
        {
            var enumerable = await dictionary.CreateEnumerableAsync(tx, EnumerationMode.Ordered).ConfigureAwait(false);
            long count = entryCountTotal - this.maxEventCount;

            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Extra events that need to be removed = {0}", count);

            List<TKey> eventKeysToRemove = new List<TKey>();

            using (IAsyncEnumerator<KeyValuePair<TKey, byte[]>> enumerator = enumerable.GetAsyncEnumerator())
            {
                while (count > 0 && await enumerator.MoveNextAsync(this.cancellationToken).ConfigureAwait(false))
                {
                    eventKeysToRemove.Add(enumerator.Current.Key);
                    --count;
                }
            }

            foreach (var key in eventKeysToRemove)
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Going to remove chaos event key={0}", key);
                await dictionary.TryRemoveAsync(tx, key, FASConstants.DefaultDictionaryTimeout, this.cancellationToken).ConfigureAwait(false);
            }
        }

        /// <summary>
        /// This method processes the reserved 'magic' keys which are used for testing Chaos
        /// </summary>
        /// <param name="chaosParameters">ChaosParameters object that holds the context dictionary</param>
        private void ProcessReservedKeysInContext(ChaosParameters chaosParameters)
        {
            if (chaosParameters.Context.ContainsKey(ChaosConstants.EnableForcedAssertKey))
            {
                ChaosUtility.ForcedEngineAssertEnabled = bool.Parse(chaosParameters.Context[ChaosConstants.EnableForcedAssertKey]);
            }
            else
            {
                ChaosUtility.ForcedEngineAssertEnabled = false;
            }

            ChaosUtility.ForcedMoveOfReplicaEnabled = chaosParameters.Context.ContainsKey(ChaosConstants.EnableForceMoveOfReplicaKey);
        }

        private async Task<bool> ShouldStartAsync()
        {
            TestabilityTrace.TraceSource.WriteInfo("StartTrek", "Enter ShouldStartAsync, datetimeutc={0}", DateTime.UtcNow);

            // If there is no last event or if the last event was a stopped event, then start chaos; otherwise just return
            this.LastEventKeyDict = this.LastEventKeyDict ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosLastEventKeyDictionaryName).ConfigureAwait(false);

            ConditionalValue<byte[]> lastEventKeyResult;
            using (var tx = this.StateManager.CreateTransaction())
            {
                if (await this.LastEventKeyDict.ContainsKeyAsync(tx, FASConstants.ChaosLastEventKeyName, FASConstants.ReliableDictionaryTimeout, this.cancellationToken).ConfigureAwait(false))
                {
                    lastEventKeyResult = await this.LastEventKeyDict.TryGetValueAsync(tx, FASConstants.ChaosLastEventKeyName, LockMode.Update, FASConstants.ReliableDictionaryTimeout, this.cancellationToken).ConfigureAwait(false);
                    if (lastEventKeyResult.HasValue)
                    {
                        var lastKey = BitConverter.ToInt64(lastEventKeyResult.Value, 0);

                        this.EventsDict = this.EventsDict ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<long, byte[]>>(FASConstants.ChaosEventsDictionaryName).ConfigureAwait(false);

                        var lastEventResult = await this.EventsDict.TryGetValueAsync(tx, lastKey, FASConstants.ReliableDictionaryTimeout, this.cancellationToken).ConfigureAwait(false);
                        if (lastEventResult.HasValue)
                        {
                            var lastEvent = ByteSerializationHelper.GetEventFromBytes(lastEventResult.Value);

                            // Starts only if the lastevent was a StoppedEvent, because if it was not stopped, it should already be
                            // running; if the user need to change the params, he need to stop it first, then start with the
                            // new parameter
                            bool wasStopped = lastEvent is StoppedEvent;

                            TestabilityTrace.TraceSource.WriteInfo("StartTrek", "Inside of ShouldStartAsync, lastEvent: {0}, wasStopped: {1}", lastEvent, wasStopped);

                            if (!wasStopped)
                            {
                                TestabilityTrace.TraceSource.WriteInfo(string.Format("{0}:LastEvent:{1}", lastEvent.TimeStampUtc.Ticks, lastEvent));
                                ChaosUtil.ThrowAlreadyRunning();
                            }
                        }
                    }
                    else
                    {
                        // assume not started, so start again
                    }
                }
                else
                {
                    // assume not started, so start again
                }

                await tx.CommitAsync().ConfigureAwait(false);
            }

            return true;
        }

        private async Task<bool> ShouldStopAsync()
        {
            // If Chaos was previously stopped, no need to do anything, return immediately; otherwise move on...
            ChaosEvent lastEvent = null;
            ConditionalValue<byte[]> lastEventKey;
            bool shouldReturn = false;

            this.LastEventKeyDict = this.LastEventKeyDict ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosLastEventKeyDictionaryName).ConfigureAwait(false);

            using (var tx = this.StateManager.CreateTransaction())
            {
                if (await this.LastEventKeyDict.ContainsKeyAsync(tx, FASConstants.ChaosLastEventKeyName, FASConstants.ReliableDictionaryTimeout, this.cancellationToken).ConfigureAwait(false))
                {
                    lastEventKey = await this.LastEventKeyDict.TryGetValueAsync(tx, FASConstants.ChaosLastEventKeyName, LockMode.Update, FASConstants.ReliableDictionaryTimeout, this.cancellationToken).ConfigureAwait(false);

                    if (lastEventKey.HasValue)
                    {
                        var lastKey = BitConverter.ToInt64(lastEventKey.Value, 0);

                        this.EventsDict = this.EventsDict ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<long, byte[]>>(FASConstants.ChaosEventsDictionaryName).ConfigureAwait(false);

                        var lastEventResult = await this.EventsDict.TryGetValueAsync(tx, lastKey, FASConstants.ReliableDictionaryTimeout, this.cancellationToken).ConfigureAwait(false);

                        if (lastEventResult.HasValue)
                        {
                            lastEvent = ByteSerializationHelper.GetEventFromBytes(lastEventResult.Value);

                            // Starts only if the lastevent was a StoppedEvent, because if it was not stopped, it should already be
                            // running; if the user need to change the params, he need to stop it first, then start with the
                            // new parameter
                            bool wasStopped = lastEvent is StoppedEvent;
                            if (wasStopped)
                            {
                                shouldReturn = true;
                            }
                        }
                    }
                    else
                    {
                        shouldReturn = true;
                    }
                }
                else
                {
                    shouldReturn = true;
                }

                await tx.CommitAsync().ConfigureAwait(false);

                if (shouldReturn)
                {
                    return false;
                }
            }

            return true;
        }

        private async Task<bool> DoResumeChaosAsyncPrivate(CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteNoise("StartChaos", "Enter DoResumeChaosAsyncPrivate");

            // Resume chaos with the saved parameters but subtracting from the time to run whatever amount of time chaos has already run
            // Get hold of the parameters dictionary, so that we can update the time-to-run before re-starting chaos
            this.ParametersDict = this.ParametersDict ??
                await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosParametersTableName).ConfigureAwait(false);

            this.LastEventKeyDict = this.LastEventKeyDict ??
                await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosLastEventKeyDictionaryName).ConfigureAwait(false);

            TimeSpan alreadyRan = TimeSpan.Zero;
            ChaosParameters chaosParameters = new ChaosParameters();
            using (ITransaction tx = this.StateManager.CreateTransaction())
            {
                // Read in parameters (that was passed into at the beginning)
                var parametersResult =
                    await this.ParametersDict.TryGetValueAsync(
                        tx,
                        FASConstants.ChaosParametersKey,
                        FASConstants.ReliableDictionaryTimeout,
                        cancellationToken).ConfigureAwait(false);

                if (parametersResult.HasValue)
                {
                    byte[] parametersInBytes = parametersResult.Value;
                    chaosParameters.FromBytes(parametersInBytes);
                    this.ChaosParameters = new ChaosParameters(
                        chaosParameters.MaxClusterStabilizationTimeout,
                        chaosParameters.MaxConcurrentFaults,
                        chaosParameters.EnableMoveReplicaFaults,
                        chaosParameters.TimeToRun,
                        chaosParameters.Context,
                        chaosParameters.WaitTimeBetweenIterations,
                        chaosParameters.WaitTimeBetweenFaults,
                        chaosParameters.ClusterHealthPolicy);

                    this.ChaosParameters.RehydratePropertiesNotInCtorFrom(chaosParameters);
                }

                this.StartTimeDict = this.StartTimeDict ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.StartTimeTableName).ConfigureAwait(false);
                var conditionalStartTime = await this.StartTimeDict.TryGetValueAsync(
                    tx,
                    FASConstants.StartTimeKey,
                    FASConstants.ReliableDictionaryTimeout,
                    cancellationToken).ConfigureAwait(false);

                if (conditionalStartTime.HasValue)
                {
                    // if starttime is found then update the time to run
                    byte[] startTimeInBytes = conditionalStartTime.Value;
                    DateTime startTime = new DateTime(BitConverter.ToInt64(startTimeInBytes, 0));

                    alreadyRan = DateTime.UtcNow.Subtract(startTime);
                    TestabilityTrace.TraceSource.WriteInfo("StartChaos", "alreadyran={0}", alreadyRan);
                }
                else
                {
                    // do nothing
                }

                await tx.CommitAsync().ConfigureAwait(false);
            }

            this.ProcessReservedKeysInContext(this.ChaosParameters);

            if (this.ChaosParameters.TimeToRun.Subtract(alreadyRan) > TimeSpan.Zero)
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "DoResumeChaosAsyncPrivate: Is starting Chaos");

                await this.StartChaosAsync(this.ChaosParameters.TimeToRun.Subtract(alreadyRan), cancellationToken).ConfigureAwait(false);
                return true;
            }
            else
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "DoResumeChaosAsyncPrivate: Is not starting Chaos");

                // 0 time left to run, Chaos should be stopped in the event dictionary and status if not already.
                var shouldStop = await this.ShouldStopAsync().ConfigureAwait(false);
                if (shouldStop)
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "DoResumeChaosAsyncPrivate: Is starting registering Stopped Event");

                    // No stopped event found as the most recent event. Should register it.
                    StoppedEvent stoppedEvent = new StoppedEvent(DateTime.UtcNow, StringResources.ChaosInfo_TimeToRunIsUp);
                    await this.StateManager.RegisterChaosEventAndUpdateChaosStatusAsync(
                        stoppedEvent,
                        ChaosStatus.Stopped,
                        this.partition,
                        this.cancellationToken,
                        async () =>
                        {
                            // We need to clear the next iteration rd because that
                            // retains state from past iterations
                            await this.StateManager.RemoveKeyFromReliableDictionaryAsync(
                                FASConstants.NextItearationTimeStampRDName,
                                FASConstants.NextItearationTimeStampKey,
                                this.partition,
                                this.cancellationToken).ConfigureAwait(false);
                            FabricEvents.Events.ChaosStop(ChaosUtil.ChaosTelemetryStopEventId, stoppedEvent.TimeStampUtc.Ticks, 0);
                            ChaosTraceEventWriter.WriteChaosStoppedTraceEvent(ChaosStopReason.TimeToRunOver);

                            if (this.stopCancellationTokenSource != null)
                            {
                                // Really this shouldn't be happening since when FAS comes up and the resume is occuring there shouldn't be chaos already started
                                // however it may be that someone hits
                                TestabilityTrace.TraceSource.WriteWarning(TraceType, "Do resume Chaos is cancelling a chaos token on resume.", DateTime.UtcNow);
                                this.stopCancellationTokenSource.Cancel();
                                TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0}:Cancellation signalled from autostop on DoResumeChaosAsyncPrivate.", DateTime.UtcNow);
                            }
                         }).ConfigureAwait(false);
                }

                return false;
            }
        }

        // Posts the fault loop on a threadpool thread passing in the cancellation token
        private async Task StartChaosAsync(TimeSpan timeToRun, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo("StartChaos", "Enter StartChaosAsync, datetimeutc={0}, timetorun={1}", DateTime.UtcNow, timeToRun);

            if (this.startChaosTask != null && !ChaosUtil.InTerminalState(this.startChaosTask))
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "In StartChaosAsync, Chaos is already executing on the threadpool.");
                return;
            }

            this.stopCancellationTokenSource = new CancellationTokenSource();

            this.aggregateCancellationTokenSource = CancellationTokenSource.CreateLinkedTokenSource(
                cancellationToken,
                this.stopCancellationTokenSource.Token,
                this.cancellationToken);

            this.StartTruncateTimer(this.aggregateCancellationTokenSource.Token);

            // Pass the events dictionary into the chaos scenario so that the information can be harvested
            this.EventsDict = this.EventsDict ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<long, byte[]>>(FASConstants.ChaosEventsDictionaryName).ConfigureAwait(false);
            var scenario = new ChaosScenario(
                this.fabricClient,
                this.ChaosParameters,
                this.EventsDict,
                this.LastEventKeyDict,
                this.StateManager,
                timeToRun,
                this.chaosSnapshotTelemetrySamplingProbability,
                this.partition,
                this.IsTestMode,
                this.aggregateCancellationTokenSource.Token);

            // Just start the loop on a threadpool thread and return
            this.startChaosTask = await Task.Factory.StartNew(
                () => scenario.ExecuteIterationsWithPauseAsync()).ConfigureAwait(false);
            this.StartCheckAutoStopTimer(timeToRun);
        }

        private void StartTruncateTimer(CancellationToken token)
        {
            try
            {
                this.truncateTimer = this.truncateTimer ?? new Timer(this.OnTruncate, null, Timeout.Infinite, Timeout.Infinite);
                this.truncateTimer.Change(TimeSpan.FromSeconds(this.truncatePeriodInSeconds), Timeout.InfiniteTimeSpan);
                token.Register(this.CancelTimer);
            }
            catch (ObjectDisposedException)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "When tried to register the CancelTimer method to the cancellation token, the associated CancellationTokenSource had already been disposed off.");
            }
        }

        private void StartTelemetryReportTimer(CancellationToken token)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter StartTelemetryReportTimer");

            try
            {
                this.telemetryReportTimer = this.telemetryReportTimer ?? new Timer(this.OnReportTelemetry, null, Timeout.Infinite, Timeout.Infinite);
                this.telemetryReportTimer.Change(TimeSpan.FromSeconds(this.telemetryReportPeriodInSeconds), Timeout.InfiniteTimeSpan);
                token.Register(this.CancelTemetryReportTimer);
            }
            catch (ObjectDisposedException)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "When tried to register the CancelTimer method to the cancellation token, the associated CancellationTokenSource had already been disposed off.");
            }
        }

        private void OnReportTelemetry(object state)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter OnReportTelemetry");
            Task.Factory.StartNew(() => this.OnReportTelemetryInnerAsync(), this.cancellationToken);
        }

        private async Task OnReportTelemetryInnerAsync()
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter OnReportTelemetryInnerAsync");
            string localtype = TraceType + "OnReportTelemetryInnerAsync";

            bool exceptionOccurred = false;

            try
            {
                await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                    Guid.Empty,
                    () => this.ReportTelemetryAsync(),
                    this.partition,
                    "OnReportTelemetryInnerAsync",
                    FASConstants.MaxRetriesForReliableDictionary,
                    this.cancellationToken).ConfigureAwait(false);
            }
            catch (FabricNotPrimaryException fnp)
            {
                FaultAnalysisServiceUtility.TraceFabricNotPrimary(Guid.Empty, fnp);
                exceptionOccurred = true;
            }
            catch (FabricObjectClosedException foc)
            {
                FaultAnalysisServiceUtility.TraceFabricObjectClosed(Guid.Empty, foc);
                exceptionOccurred = true;
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteError(localtype, "Exception in OnReportTelemetryInnerAsync: {0}", e);
                throw;
            }

            if (!exceptionOccurred)
            {
                if (this.telemetryReportTimer != null)
                {
                    lock (this.telemetryReportTimerSyncBlock)
                    {
                        if (this.telemetryReportTimer != null)
                        {
                            this.telemetryReportTimer.Change(TimeSpan.FromSeconds(this.telemetryReportPeriodInSeconds), Timeout.InfiniteTimeSpan);
                        }
                    }
                }
            }
        }

        private async Task ReportTelemetryAsync()
        {
            ChaosReportFilter chaosReportFilter = new ChaosReportFilter(DateTime.MinValue, DateTime.MaxValue);
            var retryableErrors = new FabricClientRetryErrors();

            // NotReady is thrown if Chaos was never started.
            retryableErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.NotReady);

            // The below API (local) call ends up calling a method in this class, yet we use the API version instead of direct method call because we want the convenience of FabricClientRetryHelper
            // Also, this is an infrequent call anyway.
            ChaosReport chaosReport = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync<ChaosReport>(
                () => this.fabricClient.TestManager.GetChaosReportAsync(chaosReportFilter, FASConstants.DefaultChaosInternalRequestTimeout, this.cancellationToken), retryableErrors, this.cancellationToken).ConfigureAwait(false);

            bool found = chaosReport != null
                && chaosReport.History != null
                && chaosReport.History.Count > 0;

            if (found)
            {
                this.StartTimeDict = this.StartTimeDict ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.StartTimeTableName).ConfigureAwait(false);

                long lastStartTimeInTicks = -1;
                using (var tx = this.StateManager.CreateTransaction())
                {
                    lastStartTimeInTicks = await this.StartTimeDict.GetLastStartTimeInTicksAsync(tx, this.partition, this.cancellationToken).ConfigureAwait(false);
                    await tx.CommitAsync().ConfigureAwait(false);
                }

                // get status dictionary and check if status is running
                this.StatusDictionary = this.StatusDictionary ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosStatusDictionaryName).ConfigureAwait(false);

                ChaosStatus currentStatus = ChaosStatus.None;
                using (var tx = this.StateManager.CreateTransaction())
                {
                    currentStatus = await this.StatusDictionary.GetCurrentStatusAsync(tx, this.partition, this.cancellationToken).ConfigureAwait(false);
                    await tx.CommitAsync().ConfigureAwait(false);
                }

                FabricEvents.Events.ChaosFound(
                        TraceType,
                        found,
                        currentStatus == ChaosStatus.Running,
                        lastStartTimeInTicks);
            }
            else
            {
                FabricEvents.Events.ChaosFound(
                        TraceType,
                        false,
                        false,
                        -1);
            }
        }

        private void CancelTimer()
        {
            if (this.truncateTimer != null)
            {
                this.truncateTimer.Change(Timeout.Infinite, Timeout.Infinite);
                this.truncateTimer.Dispose();
                this.truncateTimer = null;
            }

            if (this.autoStopTimer != null)
            {
                this.autoStopTimer.Change(Timeout.Infinite, Timeout.Infinite);
                this.autoStopTimer.Dispose();
                this.autoStopTimer = null;
            }
        }

        private void StartCheckAutoStopTimer(TimeSpan timeToRun)
        {
            try
            {
                this.autoStopTimeoutHelper = new Fabric.Common.TimeoutHelper(timeToRun);

                this.autoStopTimer = this.autoStopTimer ?? new Timer(this.OnCheckAutoStop, this.autoStopTimeoutHelper, Timeout.Infinite, Timeout.Infinite);
                this.autoStopTimer.Change(TimeSpan.FromSeconds(AutoStopTimerCallbackPeriodInSeconds), Timeout.InfiniteTimeSpan);
                this.cancellationToken.Register(this.CancelTimer);
            }
            catch (ObjectDisposedException)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "When tried to register the CancelTimer method to the cancellation token, the associated CancellationTokenSource had already been disposed off.");
            }
        }

        /// <summary>
        /// When FAS primary comes up, it calculates 'alreadyRan' by subtracting 'startTimeUtc' (read from RD) from 'DateTime.UtcNow'
        /// Then initializes the 'autoStopTimeoutHelper' with the (timeToRun - alreadyRan)
        /// </summary>
        /// <param name="autoStopTimeoutHelper">A stopwatch</param>
        private void OnCheckAutoStop(object autoStopTimeoutHelper)
        {
            var timeoutHelper = (Fabric.Common.TimeoutHelper)autoStopTimeoutHelper;

            if (timeoutHelper.GetRemainingTime() > TimeSpan.Zero)
            {
                if (this.autoStopTimer != null)
                {
                    lock (this.autoStopTimerSyncBlock)
                    {
                        if (this.autoStopTimer != null)
                        {
                            this.autoStopTimer.Change(TimeSpan.FromSeconds(AutoStopTimerCallbackPeriodInSeconds), Timeout.InfiniteTimeSpan);
                        }
                    }
                }
            }
            else
            {
                this.stopChaosSemaphore.Wait(this.cancellationToken);
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "OnCheckAutoStop: stopChaosSemaphore acquired current count is {0}", this.stopChaosSemaphore.CurrentCount);
                try
                {
                    if (this.stopCancellationTokenSource.IsCancellationRequested)
                    {
                        TestabilityTrace.TraceSource.WriteInfo(TraceType, "OnCheckAutoStop: stopCancellationTokenSource already cancellation requested");
                        return;
                    }

                    // only create stop event and cancel the token if it hasn't been cancelled yet.
                    try
                    {
                        StoppedEvent stoppedEvent = new StoppedEvent(DateTime.UtcNow, StringResources.ChaosInfo_TimeToRunIsUp);
                        this.StateManager.RegisterChaosEventAndUpdateChaosStatusAsync(
                            stoppedEvent,
                            ChaosStatus.Stopped,
                            this.partition,
                            this.cancellationToken,
                            () =>
                            {
                                // We need to clear the next iteration rd because that
                                // retains state from past iterations
                                this.StateManager.RemoveKeyFromReliableDictionaryAsync(
                                    FASConstants.NextItearationTimeStampRDName,
                                    FASConstants.NextItearationTimeStampKey,
                                    this.partition,
                                    this.cancellationToken).GetAwaiter().GetResult();
                                FabricEvents.Events.ChaosStop(ChaosUtil.ChaosTelemetryStopEventId, stoppedEvent.TimeStampUtc.Ticks, 0);
                                ChaosTraceEventWriter.WriteChaosStoppedTraceEvent(ChaosStopReason.TimeToRunOver);

                                this.stopCancellationTokenSource.Cancel();
                                TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0}:Cancellation signalled from autostop.", DateTime.UtcNow);
                            }).GetAwaiter().GetResult();
                    }
                    catch (OperationCanceledException oce)
                    {
                        TestabilityTrace.TraceSource.WriteWarning(TraceType, "Task got cancelled in OnCheckAutoStop: '{0}'.", oce);
                    }
                    finally
                    {
                        this.startChaosTask = null;
                    }
                }
                finally
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "OnCheckAutoStop: stopChaosSemaphore released and stopCancellationtokenSource is IsCancellationRequested {0}", this.stopCancellationTokenSource.IsCancellationRequested);
                    this.stopChaosSemaphore.Release();
                }
            }
        }

        private void CancelTemetryReportTimer()
        {
            if (this.telemetryReportTimer != null)
            {
                this.telemetryReportTimer.Change(Timeout.Infinite, Timeout.Infinite);
                this.telemetryReportTimer.Dispose();
                this.telemetryReportTimer = null;
            }
        }
    }
}
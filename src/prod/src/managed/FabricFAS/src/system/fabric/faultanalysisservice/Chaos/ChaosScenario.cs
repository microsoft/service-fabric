// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Chaos
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Chaos.Common;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Chaos.RandomActionGenerator;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.FaultAnalysis.Service.Chaos.Common;
    using System.Fabric.FaultAnalysis.Service.Common;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Collections;

    internal sealed class ChaosScenario
    {
        private const string TraceType = "Chaos.ChaosTestScenario";
        private const int MaxConsecutiveFailures = 1;

        private readonly RootActionGenerator rootActionGenerator;
        private readonly ActionTranslator actionTranslator;

        private readonly Random random;

        private readonly IList<FabricTestAction> generatedActions;

        private readonly IStatefulServicePartition partition;
        private readonly CancellationToken cancellationToken;
        private readonly ValidationHelper validationHelper;
        private readonly double chaosSnapshotTelemetrySamplingProbability;
        private bool shouldFaultSystemService;
        private int maximumNumberOfRetriesInGetClusterSnapshotAction = ChaosConstants.DefaultMaxNumberOfRetriesInGetClusterSnapshotAction;

        private Stopwatch stopwatch;
        private int consecutiveFailures;
        private bool testMode;

        internal ChaosScenario(
            FabricClient fabricClient,
            ChaosParameters chaosParameters,
            IReliableDictionary<long, byte[]> eventsDictionary,
            IReliableDictionary<string, byte[]> lastEventKeyDict,
            IReliableStateManager stateManager,
            TimeSpan timeToRun,
            double chaosSnapshotTelemetrySamplingProbability,
            IStatefulServicePartition partition,
            bool testMode,
            CancellationToken cancellationToken)
        {
            this.actionTranslator = new ActionTranslator();
            this.random = new Random((int)DateTime.Now.Ticks);
            this.rootActionGenerator = new RootActionGenerator(chaosParameters.ActionGeneratorParameters, this.random);
            this.generatedActions = new List<FabricTestAction>();
            this.FabricClient = fabricClient;
            this.TestContext = this.FabricClient.FaultManager.TestContext;
            this.ChaosParameters = chaosParameters;
            this.EventsDictionary = eventsDictionary;
            this.LastEventKeyDict = lastEventKeyDict;
            this.StateManager = stateManager;

            this.consecutiveFailures = 0;

            this.chaosSnapshotTelemetrySamplingProbability = chaosSnapshotTelemetrySamplingProbability;

            this.partition = partition;
            this.cancellationToken = cancellationToken;

            if (testMode)
            {
                this.testMode = testMode;
                chaosParameters.OperationTimeout = FASConstants.TestOperationTimeout;
            }

            this.validationHelper = new ValidationHelper(chaosParameters, fabricClient);

            this.ApplyInternalConfigFromContextIfPresent(this.ChaosParameters.Context);
        }

        internal int IterationsCompleted { get; set; }

        internal IList<FabricTestAction> GeneratedActions
        {
            get
            {
                return this.generatedActions;
            }
        }

        internal FabricClient FabricClient { get; set; }

        internal FabricTestContext TestContext { get; set; }

        internal ChaosParameters ChaosParameters { get; set; }

        /// <summary>
        /// This holds all events keyed by their timestamp tick count
        /// </summary>
        internal IReliableDictionary<long, byte[]> EventsDictionary { get; set; }

        /// <summary>
        /// This holds the latest event that went into the EventsDictionary (mostly to facilitate fast retrieval of this special event)
        /// </summary>
        internal IReliableDictionary<string, byte[]> LastEventKeyDict { get; set; }

        /// <summary>
        /// Holds the current task that Chaos test is performing, keyed by Constants.ChaosLastKeyDictionaryName
        /// </summary>
        internal IReliableDictionary<string, byte[]> StatusDictionary { get; set; }

        /// <summary>
        /// This holds the timestamp of the next iteration
        /// </summary>
        internal IReliableDictionary<string, byte[]> NextIterationTimeStampRD { get; set; }

        internal IReliableStateManager StateManager { get; set; }

        internal Task StartChaosTask { get; set; }

        internal async Task ExecuteIterationsWithPauseAsync()
        {
            TestabilityTrace.TraceSource.WriteNoise("StartTrek", "Enter ExecuteIterationsWithPauseAsync, datetimeutc={0}", DateTime.UtcNow);

            this.stopwatch = new Stopwatch();
            this.stopwatch.Start();

            Exception capturedException = null;

            TimeSpan waitTime = this.ChaosParameters.WaitTimeBetweenIterations;

            while (!this.cancellationToken.IsCancellationRequested)
            {
                // If this is not the beginning of a fresh Chaos run, before starting a new iteration,
                // consult the NextIterationTimeStampRD to find out if there is some
                // residual wait time from the previous iteration, if there is then wait that amount
                var nextIterationTimeStampUtc = await this.StateManager.GetUtcTimeStampAsync(
                                                                                        FASConstants.NextItearationTimeStampRDName,
                                                                                        FASConstants.NextItearationTimeStampKey,
                                                                                        this.partition,
                                                                                        this.cancellationToken).ConfigureAwait(false);

                var residualWaitTime = nextIterationTimeStampUtc.Subtract(DateTime.UtcNow);

                if (residualWaitTime > TimeSpan.Zero)
                {
                    await this.StateManager.RegisterChaosEventAndUpdateChaosStatusAsync(
                           new WaitingEvent(DateTime.UtcNow, StringHelper.Format(StringResources.ChaosInfo_ResidualWaitingFromPreviousIteration, residualWaitTime)),
                           ChaosStatus.Running,
                           this.partition,
                           this.cancellationToken,
                           () =>
                           {
                               TestabilityTrace.TraceSource.WriteInfo(TraceType, "Registering WaitingEvent for waiting '{0}' left over from the previous iteration.", residualWaitTime);
                           }).ConfigureAwait(false);

                    await Task.Delay(residualWaitTime, this.cancellationToken).ConfigureAwait(false);
                }

                try
                {
                    if (await this.IsClusterReadyForFaultsAsync(this.cancellationToken).ConfigureAwait(false))
                    {
                        System.Fabric.Common.TimeoutHelper timer = new System.Fabric.Common.TimeoutHelper(this.ChaosParameters.MaxClusterStabilizationTimeout);

                        StringBuilder validationReport = new StringBuilder();

                        var clusterReport =
                            await
                                this.validationHelper.ValidateClusterHealthAsync(
                                    timer.GetRemainingTime(),
                                    this.cancellationToken).ConfigureAwait(false);

                        if (clusterReport.ValidationFailed)
                        {
                            // quadratic with an upper bound of DefaultMaximumBackoffForChaosIterations
                            waitTime += (waitTime >= FASConstants.DefaultMaximumBackoffForChaosIterations) ? TimeSpan.Zero : waitTime;

                            var serviceReport =
                                await
                                    this.validationHelper.ValidateAllServicesAsync(
                                        timer.GetRemainingTime(),
                                        this.cancellationToken).ConfigureAwait(false);

                            if (serviceReport.ValidationFailed)
                            {
                                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Even though some services are unhealthy or unstable, going to induce faults, because the cluster is healthy.");

                                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Failure reason: \n'{0}'", serviceReport.FailureReason);

                                validationReport.Append(serviceReport.FailureReason);
                            }

                            StringBuilder reportBuilder = new StringBuilder();
                            reportBuilder.Append(StringHelper.Format(StringResources.ChaosInfo_WaitingNotice, waitTime));
                            reportBuilder.AppendLine();
                            reportBuilder.AppendLine(clusterReport.FailureReason);

                            validationReport.Insert(0, reportBuilder.ToString());

                            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Cluster validation failed for '{0}'.", clusterReport.FailureReason);

                            var validationFailedEvent = new ValidationFailedEvent(DateTime.UtcNow, validationReport.ToString());

                            // record validation failed event
                            await this.StateManager.RegisterChaosEventAndUpdateChaosStatusAsync(
                                validationFailedEvent,
                                ChaosStatus.Running,
                                this.partition,
                                this.cancellationToken,
                                () =>
                                {
                                    FabricEvents.Events.ChaosValidationFailed(
                                        Guid.NewGuid().ToString(),
                                        validationFailedEvent.TimeStampUtc.Ticks,
                                        validationFailedEvent.Reason);
                                }).ConfigureAwait(false);

                            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Pausing for '{0}' before performing next check.", waitTime);
                        }
                        else
                        {
                            waitTime = this.ChaosParameters.WaitTimeBetweenIterations;

                            await this.StateManager.RegisterCurrentStatusAsync(
                                ChaosStatus.Running,
                                this.partition,
                                this.cancellationToken).ConfigureAwait(false);

                            var timestampOfNextIteration = DateTime.UtcNow.Add(waitTime);

                            await this.StateManager.SetUtcTimeStampAsync(
                                FASConstants.NextItearationTimeStampRDName,
                                FASConstants.NextItearationTimeStampKey,
                                timestampOfNextIteration,
                                this.partition,
                                this.cancellationToken).ConfigureAwait(false);

                            await this.ExecuteFaultIterationAsync(this.cancellationToken).ConfigureAwait(false);

                            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Pausing for '{0}' before executing next iteration.", waitTime);
                        }
                    }
                }
                catch (Exception exception)
                {
                    capturedException = exception;

                    // quadratic with an upper bound of DefaultMaximumBackoffForChaosIterations
                    waitTime += (waitTime >= FASConstants.DefaultMaximumBackoffForChaosIterations) ? TimeSpan.Zero : waitTime;

                    var timestampOfNextIteration = DateTime.UtcNow.Add(waitTime);

                    await this.StateManager.SetUtcTimeStampAsync(
                        FASConstants.NextItearationTimeStampRDName,
                        FASConstants.NextItearationTimeStampKey,
                        timestampOfNextIteration,
                        this.partition,
                        this.cancellationToken).ConfigureAwait(false);

                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "Exception occurred in the ChaosTestScenario loop: {0}, cancellationreq: {1}", capturedException, this.cancellationToken.IsCancellationRequested);
                }

                if (capturedException != null)
                {
                    var ae = capturedException as AggregateException;
                    if (ae != null)
                    {
                        capturedException = ae.Flatten().InnerException;
                    }

                    if (!ChaosUtil.IsExpectedException(capturedException))
                    {
                        string testErrorEventMessage = capturedException.Message;

                        if (capturedException is FabricChaosEngineException)
                        {
                            testErrorEventMessage = StringResources.ChaosError_UnexpectedInChaosEngine;
                        }

                        await this.StateManager.RegisterChaosEventAndUpdateChaosStatusAsync(
                            new TestErrorEvent(DateTime.UtcNow, testErrorEventMessage),
                            ChaosStatus.Running,
                            this.partition,
                            this.cancellationToken).ConfigureAwait(false);

                        TestabilityTrace.TraceSource.WriteInfo(TraceType, "Unexpected exception '{0}' was turned into TestErrorEvent.", capturedException);
                    }

                    // handled the exception, now clear it
                    capturedException = null;
                }

                if (this.testMode && (this.ChaosParameters.Context != null && this.ChaosParameters.Context.ContainsKey(ChaosConstants.FailoverAfterChaosFaultsKey)))
                {
                    this.partition.ReportFault(FaultType.Transient);
                }

                await this.StateManager.RegisterChaosEventAndUpdateChaosStatusAsync(
                           new WaitingEvent(DateTime.UtcNow, StringHelper.Format(StringResources.ChaosInfo_WaitingBetweenIterations, waitTime)),
                           ChaosStatus.Running,
                           this.partition,
                           this.cancellationToken,
                           () =>
                           {
                               TestabilityTrace.TraceSource.WriteInfo(TraceType, "Registering WaitingEvent for waiting '{0}' between iterations.", waitTime);
                           }).ConfigureAwait(false);
                await Task.Delay(waitTime, this.cancellationToken).ConfigureAwait(false);
                this.IterationsCompleted++;
            }

            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Session has completed. \nTotal iterations: {0}. Total elapsed time: {1}", this.IterationsCompleted, this.GetElapsedTime());
        }

        /// <summary>
        /// Get test actions for the iteration.
        /// Execute each action and wait for a random wait duration.
        /// </summary>
        private async Task ExecuteFaultIterationAsync(CancellationToken token)
        {
            TestabilityTrace.TraceSource.WriteInfo("StartTrek", "Enter ExecuteFaultIterationAsync, datetimeutc={0}", DateTime.UtcNow);

            var stateTransitionList = await this.GetGeneratedActionsAsync(token).ConfigureAwait(false);
            var commandsMap = this.actionTranslator.GetCommandsMap(stateTransitionList);
            var testActionList = commandsMap.Keys.ToList();

            TestabilityTrace.TraceSource.WriteInfo("StartTrek", "Got generated actions, datetimeutc={0}", DateTime.UtcNow);

            if (testActionList != null && testActionList.Count > 0)
            {
                List<Task> tasks = new List<Task>();
                Dictionary<Task, FabricTestAction> taskToActionMap = new Dictionary<Task, FabricTestAction>();
                Dictionary<Task, StateTransitionAction> taskToTransitionMap = new Dictionary<Task, StateTransitionAction>();
                await this.StateManager.RegisterCurrentStatusAsync(ChaosStatus.Running, this.partition, this.cancellationToken).ConfigureAwait(false);

                foreach (FabricTestAction testAction in testActionList)
                {
                    var taskForAction = this.ExecuteTestActionAsync(testAction, token);
                    taskToActionMap.Add(taskForAction, testAction);
                    taskToTransitionMap.Add(taskForAction, commandsMap[testAction]);

                    // Run one task
                    tasks.Add(taskForAction);
                }

                ChaosTraceEventWriter.WriteChaosFaultScheduledTraceEvent(stateTransitionList);

                try
                {
                    await Task.WhenAll(tasks).ConfigureAwait(false);
                }
                catch (Exception e)
                {
                    if (!this.HandleException(tasks, taskToActionMap))
                    {
                        throw;
                    }

                    TestabilityTrace.TraceSource.WriteWarning(TraceType, "Expected exception happened while executing faults: '{0}'", e);
                }

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Done one iteration of faults");
            }
        }

        private bool HandleException(List<Task> tasks, Dictionary<Task, FabricTestAction> taskToActionMap)
        {
            var faultedTasks = tasks.Where(t => t.IsFaulted);

            foreach (var task in faultedTasks)
            {
                ThrowIf.IsTrue(task.Exception == null, "Faulted task '{0}' should have an associated exception.", task);

                var fabricException = task.Exception.InnerException as FabricException;

                if (fabricException == null)
                {
                    return false;
                }

                Type actionType = taskToActionMap[task].GetType();

                if (!ChaosConstants.ExpectedExceptionTypeMap.ContainsKey(actionType))
                {
                    return false;
                }

                var expectedErrorCodes = ChaosConstants.ExpectedExceptionTypeMap[actionType];

                if (!expectedErrorCodes.Contains(fabricException.ErrorCode))
                {
                    return false;
                }
            }

            return true;
        }

        private async Task ExecuteTestActionAsync(FabricTestAction testAction, CancellationToken token)
        {
            ReleaseAssert.AssertIf(testAction == null, "testAction should not be null");
            string testActionId = testAction.ToString() + testAction.GetHashCode();
            string actionName = testAction.GetType().Name;

            // wait for random time duration.
            double pauseMilliseconds = this.random.NextDouble() * this.ChaosParameters.WaitTimeBetweenFaults.TotalMilliseconds;
            TestabilityTrace.TraceSource.WriteInfoWithId(TraceType, testActionId, "Pausing for {0} milliseconds before executing next action", pauseMilliseconds);
            await Task.Delay(TimeSpan.FromMilliseconds(pauseMilliseconds), token).ConfigureAwait(false);

            // create an id for action for tracing.
            TestabilityTrace.TraceSource.WriteInfoWithId(TraceType, testActionId, "Executing action: {0}", actionName);

            // Execute this action linked token.
            await this.TestContext.ActionExecutor.RunAsync(testAction, token).ContinueWith(t =>
                                                                                            {
                                                                                                this.HandleTaskComplete(t, testActionId, actionName);
                                                                                            }).ConfigureAwait(false);

            TestabilityTrace.TraceSource.WriteInfoWithId(TraceType, testActionId, "Action {0} completed.", testAction.GetType().Name);
        }

        private async Task<IList<StateTransitionAction>> GetGeneratedActionsAsync(CancellationToken token)
        {
            Guid activityId = Guid.NewGuid();

            TestabilityTrace.TraceSource.WriteInfo("StartTrek", "Just before taking snapshot, datetimeutc={0}", DateTime.UtcNow);
            ClusterStateSnapshot taskSnapshot = await this.GetStateSnapshotAsync(token).ConfigureAwait(false);
            IList<StateTransitionAction> ragActions = new List<StateTransitionAction>();

            if (taskSnapshot != null)
            {
                ragActions = this.rootActionGenerator.GetNextActions(taskSnapshot, activityId);

                List<string> faultsInCurrentIteration = new List<string>(ragActions.Count);

                foreach (var a in ragActions)
                {
                    faultsInCurrentIteration.Add(a.ToString());
                }

                TestabilityTrace.TraceSource.WriteInfo("StartTrek", "Just before creating ExecutingFaultsEvent, datetimeutc={0}", DateTime.UtcNow);

                var execFaultsEvent = new ExecutingFaultsEvent(DateTime.UtcNow, faultsInCurrentIteration);

                await this.StateManager.RegisterChaosEventAndUpdateChaosStatusAsync(
                    new ExecutingFaultsEvent(DateTime.UtcNow, faultsInCurrentIteration),
                    ChaosStatus.Running,
                    this.partition,
                    token,
                    () =>
                    {
                        FabricEvents.Events.ChaosExecutingFaults(TraceType, execFaultsEvent.TimeStampUtc.Ticks, execFaultsEvent.Faults.Count, string.Join(FASConstants.FaultStringDelimeter, execFaultsEvent.Faults));
                    }).ConfigureAwait(false);
            }
            else
            {
                // GetStateSnapshot must have failed hence we need to retry the entire iteration. 
            }

            return ragActions;
        }

        // Gather latest information to create StateSnapshot
        private Task<ClusterStateSnapshot> GetStateSnapshotAsync(CancellationToken token)
        {
            return this.GetClusterStateSnapshotAsync(token).ContinueWith<ClusterStateSnapshot>(
                t =>
                {
                    this.HandleTaskComplete(t, "GetClusterStateSnapshot", "GetClusterStateSnapshot");
                    if (t.Exception == null)
                    {
                        return t.Result;
                    }
                    else
                    {
                        return null;
                    }
                },
                token);
        }

        // Get latest active application info.
        private async Task<ClusterStateSnapshot> GetClusterStateSnapshotAsync(CancellationToken token)
        {
            GetClusterStateSnapshotAction getClusterStateSnapshotAction = new GetClusterStateSnapshotAction(
                                                                                                            this.chaosSnapshotTelemetrySamplingProbability,
                                                                                                            this.shouldFaultSystemService,
                                                                                                            this.maximumNumberOfRetriesInGetClusterSnapshotAction,
                                                                                                            this.ChaosParameters.ChaosTargetFilter);

            await this.TestContext.ActionExecutor.RunAsync(getClusterStateSnapshotAction, token).ConfigureAwait(false);

            return getClusterStateSnapshotAction.Result;
        }

        private async Task<bool> IsClusterReadyForFaultsAsync(CancellationToken token)
        {
            string localType = TraceType + "ClusterReady";
            TestabilityTrace.TraceSource.WriteInfo(localType, "Entered ");

            this.StatusDictionary = this.StatusDictionary ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosStatusDictionaryName).ConfigureAwait(false);

            await this.StateManager.RegisterCurrentStatusAsync(ChaosStatus.Running, this.partition, this.cancellationToken).ConfigureAwait(false);

            IsClusterUpgradingAction isUpgradingAction = new IsClusterUpgradingAction
                                                             {
                                                                 ActionTimeout = this.ChaosParameters.OperationTimeout,
                                                                 RequestTimeout = this.ChaosParameters.RequestTimeout
                                                             };

            var clusterUpgradeCheckTask = this.TestContext.ActionExecutor.RunAsync(isUpgradingAction, token);
            bool checkFailed = false;
            await clusterUpgradeCheckTask.ContinueWith(
                t =>
                {
                    this.HandleTaskComplete(t, "IsClusterUpgrading", "IsClusterUpgrading");
                    if (t.Exception != null)
                    {
                        checkFailed = true;
                    }
                },
                token).ConfigureAwait(false);

            IsInfrastructureUpgradingAction infraUpgradingAction = new IsInfrastructureUpgradingAction
                                                                       {
                                                                           ActionTimeout = this.ChaosParameters.OperationTimeout,
                                                                           RequestTimeout = this.ChaosParameters.RequestTimeout
                                                                       };

            var infraUpgradeCheckTask = this.TestContext.ActionExecutor.RunAsync(infraUpgradingAction, token);
            bool infraCheckFailed = false;
            await infraUpgradeCheckTask.ContinueWith(
                t =>
                {
                    this.HandleTaskComplete(t, "IsInfrastructureUpgrading", "IsInfrastructureUpgrading");
                    if (t.Exception != null)
                    {
                        infraCheckFailed = true;
                    }
                },
                    token).ConfigureAwait(false);

            bool isready = true;

            if (checkFailed || infraCheckFailed)
            {
                // Record validation failed event
                await this.StateManager.RegisterChaosEventAndUpdateChaosStatusAsync(
                    new WaitingEvent(DateTime.UtcNow, StringResources.ChaosInfo_WaitingForQuiescentCluster),
                    ChaosStatus.Running,
                    this.partition,
                    this.cancellationToken,
                    () =>
                    {
                        TestabilityTrace.TraceSource.WriteInfo("[0]:Upgrade going on, so going to wait.", DateTime.UtcNow.ToString());
                    }).ConfigureAwait(false);

                return false;
            }

            if (isUpgradingAction.Result || infraUpgradingAction.Result)
            {
                isready = false;
            }

            // Not ready because cluster or infrastructure is upgrading
            if (!isready)
            {
                // Record validation failed event
                await this.StateManager.RegisterChaosEventAndUpdateChaosStatusAsync(
                    new WaitingEvent(DateTime.UtcNow, StringResources.ChaosInfo_WaitingForQuiescentCluster),
                    ChaosStatus.Running,
                    this.partition,
                    this.cancellationToken,
                    () => { TestabilityTrace.TraceSource.WriteInfo("[0]:Upgrade going on, so going to wait.", DateTime.UtcNow.ToString()); }).ConfigureAwait(false);

                return false;
            }
            else
            {
                return true;
            }
        }

        private void ApplyInternalConfigFromContextIfPresent(Dictionary<string, string> startContext)
        {
            if (startContext == default(Dictionary<string, string>))
            {
                return;
            }

            if (startContext.ContainsKey(ChaosConstants.ChaosShouldFaultSystemKey))
            {
                if (!bool.TryParse(startContext[ChaosConstants.ChaosShouldFaultSystemKey], out this.shouldFaultSystemService))
                {
                    TestabilityTrace.TraceSource.WriteWarning(TraceType, "Could not parse 'ShouldFaultSystem' in the Context dictionary.");
                }
            }

            if (startContext.ContainsKey(ChaosConstants.MaxNumberOfRetriesInGetClusterSnapshotActionKey))
            {
                if (!int.TryParse(startContext[ChaosConstants.MaxNumberOfRetriesInGetClusterSnapshotActionKey], out this.maximumNumberOfRetriesInGetClusterSnapshotAction))
                {
                    TestabilityTrace.TraceSource.WriteWarning(TraceType, "Could not parse 'MaxNumberOfRetriesInGetClusterSnapshotActionKey' in the Context dictionary.");
                }
            }
        }

        private void HandleTaskComplete(Task t, string actionId, string actionName)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Action '{0}' completed with status: '{1}'.", actionName, t.Status.ToString());
            if (t.Exception != null)
            {
                t.Exception.Handle(ex =>
                {
                    var exceptionToTrace = ex is AggregateException ? ex.InnerException : ex;

                    if (exceptionToTrace is FabricValidationException)
                    {
                        string reason = string.Format("Failed while executing {0}, with the exception: {1}", actionName, exceptionToTrace);
                        var validationFailedEvent = new ValidationFailedEvent(DateTime.UtcNow, reason);

                        // record validation failed event
                        this.StateManager.RegisterChaosEventAndUpdateChaosStatusAsync(
                            validationFailedEvent,
                            ChaosStatus.Running,
                            this.partition,
                            this.cancellationToken,
                            () =>
                            {
                                FabricEvents.Events.ChaosValidationFailed(
                                Guid.NewGuid().ToString(),
                                validationFailedEvent.TimeStampUtc.Ticks,
                                validationFailedEvent.Reason);
                            }).GetAwaiter().GetResult();
                    }
                    else
                    {
                        TestabilityTrace.TraceSource.WriteInfo(
                            TraceType,
                            actionId,
                            "Action {0}. Failed with exception {1}.",
                            actionName,
                            exceptionToTrace);
                    }

                    // Exception will be propagated if total consecutive failures greater than MaxConsecutiveFailures
                    return this.RecordError(); // throwing the exception if false
                });
            }
            else
            {
                this.RecordSuccess();
            }
        }

        /// <summary>
        /// This API supports the Service Fabric platform and is not meant to be called from your code
        /// </summary>
        /// <returns>This API supports the Service Fabric platform and is not meant to be called from your code</returns>
        private TimeSpan GetElapsedTime()
        {
            return this.stopwatch.Elapsed;
        }

        private bool RecordError()
        {
            int result = Interlocked.Increment(ref this.consecutiveFailures);
            return result < MaxConsecutiveFailures;
        }

        private void RecordSuccess()
        {
            Interlocked.Exchange(ref this.consecutiveFailures, 0);
        }
    }
}
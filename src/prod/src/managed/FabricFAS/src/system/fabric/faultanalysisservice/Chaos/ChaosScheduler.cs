//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Chaos
{
    using System;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Strings;
    using Collections.Generic;
    using Fabric.Chaos.Common;
    using Fabric.Chaos.DataStructures;
    using Health;
    using IO;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Collections;
    using Service.Common;
    using Threading;
    using Threading.Tasks;

    /// <summary>
    /// <para>Represents a ChaosScheduler</para>
    /// </summary>
    internal sealed class ChaosScheduler
    {
        private const string TraceComponent = "Chaos.ChaosScheduler";
        /*
            EVERY SINGLE STATE NEEDS TO ACCEPT A STOPCHAOS COMMAND THAT RESULTS IN A NoChaosScheduleStopped STATE.
            { new StateTransition(new SchedulerState(YOUR NEW STATE), Command.StopChaos ), new SchedulerState(SchedulerState.NoChaosScheduleStopped) },
        */
        private static readonly Dictionary<StateTransition, SchedulerState> TransitionTable = new Dictionary<StateTransition, SchedulerState>()
        {
            // Chaos not running, Scheduler Stopped
            { new StateTransition(new SchedulerState(SchedulerState.NoChaosScheduleStopped), Command.StopChaos),    new SchedulerState(SchedulerState.NoChaosScheduleStopped) },
            { new StateTransition(new SchedulerState(SchedulerState.NoChaosScheduleStopped), Command.SetSchedule),  new SchedulerState(SchedulerState.NoChaosSchedulePending) },

            // Chaos not running, schedule Pending
            { new StateTransition(new SchedulerState(SchedulerState.NoChaosSchedulePending), Command.StopChaos),       new SchedulerState(SchedulerState.NoChaosScheduleStopped) },
            { new StateTransition(new SchedulerState(SchedulerState.NoChaosSchedulePending), Command.SetSchedule),     new SchedulerState(SchedulerState.NoChaosSchedulePending) },
            { new StateTransition(new SchedulerState(SchedulerState.NoChaosSchedulePending), Command.MatureSchedule),  new SchedulerState(SchedulerState.NoChaosScheduleActive) },

            // Chaos not running, schedule active
            { new StateTransition(new SchedulerState(SchedulerState.NoChaosScheduleActive), Command.StopChaos),      new SchedulerState(SchedulerState.NoChaosScheduleStopped) },
            { new StateTransition(new SchedulerState(SchedulerState.NoChaosScheduleActive), Command.SetSchedule),    new SchedulerState(SchedulerState.NoChaosSchedulePending) },
            { new StateTransition(new SchedulerState(SchedulerState.NoChaosScheduleActive), Command.StartChaos),     new SchedulerState(SchedulerState.ChaosScheduleActive) },
            { new StateTransition(new SchedulerState(SchedulerState.NoChaosScheduleActive), Command.ExpireSchedule), new SchedulerState(SchedulerState.NoChaosScheduleExpired) },

            // Chaos running, schedule active
            { new StateTransition(new SchedulerState(SchedulerState.ChaosScheduleActive), Command.StopChaos),      new SchedulerState(SchedulerState.NoChaosScheduleStopped) },
            { new StateTransition(new SchedulerState(SchedulerState.ChaosScheduleActive), Command.FinishChaos),    new SchedulerState(SchedulerState.NoChaosScheduleActive) },
            { new StateTransition(new SchedulerState(SchedulerState.ChaosScheduleActive), Command.ExpireSchedule), new SchedulerState(SchedulerState.NoChaosScheduleExpired) },

            // Chaos not running, schedule expired
            { new StateTransition(new SchedulerState(SchedulerState.NoChaosScheduleExpired), Command.StopChaos),    new SchedulerState(SchedulerState.NoChaosScheduleStopped) },
            { new StateTransition(new SchedulerState(SchedulerState.NoChaosScheduleExpired), Command.SetSchedule),  new SchedulerState(SchedulerState.NoChaosSchedulePending) }
        };

        private readonly IStatefulServicePartition partition;
        private readonly SemaphoreSlim stateSemaphore;

        private FabricClient fabricClient;
        private ChaosMessageProcessor chaosMessageProcessor;

        // Not threadsafe, only read/write state and schedule when inside stateSemaphore.
        private SchedulerState state;
        private ChaosScheduleDescription scheduleDescription;
        private ChaosScheduleEventInstancesEnumerator eventInstancesEnumerator;

        internal ChaosScheduler(IReliableStateManager stateManager, IStatefulServicePartition partition)
        {
            this.StateManager = stateManager;
            this.partition = partition;

            this.stateSemaphore = new SemaphoreSlim(1, 1);

            this.state = new SchedulerState(SchedulerState.NoChaosScheduleStopped);
            this.scheduleDescription = new ChaosScheduleDescription();
        }

        internal enum Command
        {
            Invalid = 0,
            SetSchedule,
            ExpireSchedule,
            MatureSchedule,
            StartChaos,
            FinishChaos,
            StopChaos
        }

        public IReliableStateManager StateManager { get; private set; }

        public IReliableDictionary<string, byte[]> StatusDictionary { get; private set; }

        internal async Task InitializeAsync(FabricClient fabricClient, ChaosMessageProcessor chaosMessageProcessor, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "InitializeAsync - started");

            this.fabricClient = fabricClient;
            this.chaosMessageProcessor = chaosMessageProcessor;

            await this.RestartRecoveryAsync(cancellationToken).ConfigureAwait(false);

#pragma warning disable 4014
            Task.Run(() => this.StartAndRunAsync(cancellationToken)).ContinueWith(task =>
            {
                if (task.IsCanceled)
                {
                    // This will print after the thread running the scheduler ends.
                    TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "Scheduler task cancelled.");
                }
            });
#pragma warning restore 4014

            TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "InitializeAsync - ended");
        }

        internal async Task StopChaosAsync(CancellationToken cancellationToken)
        {
            if (!this.stateSemaphore.Wait(ChaosConstants.SchedulerLockWaitMilliseconds, cancellationToken))
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceComponent, "StopChaosAsync - did not acquire lock in provisioned time. Not progressing.");

                return;
            }

            try
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "stateSemaphore Acquired by StopChaosAsync");

                await this.chaosMessageProcessor.ProcessStopChaosOldAsync(false).ConfigureAwait(false);
                await this.TryMoveStateAsync(Command.StopChaos, cancellationToken).ConfigureAwait(false);
                this.CheckStateAndThrowOnError(SchedulerState.NoChaosScheduleStopped);
            }
            finally
            {
                this.stateSemaphore.Release();
                TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "stateSemaphore Released by StopChaosAsync");
            }
        }

        internal async Task SetScheduleAsync(ChaosScheduleDescription scheduleDescription, bool useAutoVersion, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "Enter SetScheduleAsync with scheduleDescription={0} and useAutoVersion={1}", scheduleDescription.ToString(), useAutoVersion);

            if (!this.stateSemaphore.Wait(ChaosConstants.SchedulerLockWaitMilliseconds, cancellationToken))
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceComponent, "SetScheduleAsync - did not acquire lock in provisioned time. Not progressing.");

                return;
            }

            try
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "stateSemaphore Acquired by SetScheduleAsync");

                if (useAutoVersion || scheduleDescription.Version == this.scheduleDescription.Version)
                {
                    var newScheduleDescription = new ChaosScheduleDescription(this.scheduleDescription.Version, scheduleDescription.Schedule);
                    newScheduleDescription.IncrementVersion();
                    await this.SetScheduleInternalAsync(newScheduleDescription, cancellationToken).ConfigureAwait(false);
                }
                else
                {
                    throw new System.ArgumentException(
                        string.Format(
                            StringResources.ChaosScheduler_ScheduleVersionMismatch,
                            this.scheduleDescription.Version,
                            scheduleDescription.Version),
                        "ChaosScheduleDescription.Version");
                }
            }
            finally
            {
                this.stateSemaphore.Release();
                TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "stateSemaphore Released by SetScheduleAsync");
            }
        }

        private async Task SetScheduleInternalAsync(ChaosScheduleDescription scheduleDescription, CancellationToken cancellationToken)
        {
            // Must only be called when inside the semaphore
            TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "Enter SetScheduleInternalAsync");

            var peak = this.PeakMoveState(Command.SetSchedule);
            if (peak.Equals(SchedulerState.NoChaosSchedulePending))
            {
                await ChaosSchedulerUtil.VerifyChaosScheduleAsync(scheduleDescription.Schedule, this.fabricClient, cancellationToken);
                TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "schedule verified");

                this.scheduleDescription = scheduleDescription;
                await this.WriteScheduleToReliableStoreAsync(scheduleDescription, cancellationToken).ConfigureAwait(false);

                this.eventInstancesEnumerator = new ChaosScheduleEventInstancesEnumerator(this.scheduleDescription.Schedule, DateTime.UtcNow);
                this.eventInstancesEnumerator.MoveNext();

                await this.TryMoveStateAsync(Command.SetSchedule, cancellationToken).ConfigureAwait(false);
                this.CheckStateAndThrowOnError(SchedulerState.NoChaosSchedulePending);
            }
            else if (peak.Equals(SchedulerState.ChaosScheduleActive))
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceComponent, "Attempting to set schedule when chaos was running");

                ChaosUtil.ThrowAlreadyRunning();
            }
        }

        private async Task SetScheduleAndTryResumeAsync(ChaosScheduleDescription scheduleDescription, CancellationToken cancellationToken)
        {
            // Must only be called when inside the semaphore
            TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "Enter SetScheduleAndTryResumeAsync");

            await ChaosSchedulerUtil.VerifyChaosScheduleAsync(scheduleDescription.Schedule, this.fabricClient, cancellationToken);
            TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "schedule verified");

            this.scheduleDescription = scheduleDescription;
            await this.WriteScheduleToReliableStoreAsync(scheduleDescription, cancellationToken).ConfigureAwait(false);

            this.eventInstancesEnumerator = new ChaosScheduleEventInstancesEnumerator(this.scheduleDescription.Schedule, DateTime.UtcNow);
            this.eventInstancesEnumerator.MoveNext();

            if (this.eventInstancesEnumerator.HasEvents())
            {
                while (this.eventInstancesEnumerator.Current.CompareTo(DateTime.UtcNow) == -1)
                {
                    this.eventInstancesEnumerator.MoveNext();
                }
            }

            await this.TryMoveStateAsync(Command.SetSchedule, cancellationToken).ConfigureAwait(false);
            await this.TryMoveStateAsync(Command.MatureSchedule, cancellationToken).ConfigureAwait(false);

            bool didResume = await this.chaosMessageProcessor.ResumeChaosAsync(cancellationToken);

            if (didResume)
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "Chaos did resume");

                // chaos was resumed and is running. We have lock protecting the state so we can move between these states without worrying about scheduler trying to take action based on states between these transitions.
                await this.TryMoveStateAsync(Command.StartChaos, cancellationToken).ConfigureAwait(false);
            }
            else
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "Chaos did not resume");
            }
        }

        private async Task WriteScheduleToReliableStoreAsync(ChaosScheduleDescription scheduleDescription, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "Enter WriteScheduleToReliableStoreAsync with scheduleDescription={0}", scheduleDescription.ToString());

            this.StatusDictionary = this.StatusDictionary ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosSchedulerStateName).ConfigureAwait(false);

            await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                Guid.Empty,
                () => this.WriteScheduleToReliableStoreHelperAsync(scheduleDescription, cancellationToken),
                this.partition,
                "WriteScheduleToReliableStoreHelperAsync",
                FASConstants.MaxRetriesForReliableDictionary,
                cancellationToken).ConfigureAwait(false);
        }

        private async Task WriteScheduleToReliableStoreHelperAsync(ChaosScheduleDescription scheduleDescription, CancellationToken cancellationToken)
        {
            using (ITransaction tx = this.StateManager.CreateTransaction())
            {
                await this.StatusDictionary.SetAsync(tx, FASConstants.ChaosSchedulerStatusDictionaryScheduleKey, scheduleDescription.ToBytes()).ConfigureAwait(false);
                await tx.CommitAsync();
            }
        }

        private async Task TryMatureScheduleAsync(CancellationToken cancellationToken)
        {
            // Must only be called when inside the semaphore
            TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "Enter TryMatureScheduleAsync");

            var peak = this.PeakMoveState(Command.MatureSchedule);
            if (peak.Equals(SchedulerState.NoChaosScheduleActive))
            {
                if (DateTime.UtcNow >= this.scheduleDescription.Schedule.StartDate)
                {
                    // Schedule should be active
                    await this.TryMoveStateAsync(Command.MatureSchedule, cancellationToken).ConfigureAwait(false);
                    this.CheckStateAndThrowOnError(SchedulerState.NoChaosScheduleActive);
                }
            }
        }

        private async Task TryExpireSchedule(CancellationToken cancellationToken)
        {
            // Must only be called when inside the semaphore
            TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "Enter TryExpireScheduleAsync");

            var peak = this.PeakMoveState(Command.ExpireSchedule);
            if (peak.Equals(SchedulerState.NoChaosScheduleExpired))
            {
                if (DateTime.UtcNow >= this.scheduleDescription.Schedule.ExpiryDate)
                {
                    try
                    {
                        await this.chaosMessageProcessor.ProcessStopChaosOldAsync(true).ConfigureAwait(false);
                    }
                    catch (Exception e)
                    {
                        TestabilityTrace.TraceSource.WriteError(TraceComponent, "Try expire schedule failed on process stop chaos old async with error e {0}", e.ToString());
                    }

                    await this.TryMoveStateAsync(Command.ExpireSchedule, cancellationToken).ConfigureAwait(false);
                    this.CheckStateAndThrowOnError(SchedulerState.NoChaosScheduleExpired);
                }
            }
        }

        private async Task TryStartChaosAsync(CancellationToken cancellationToken)
        {
            // Must only be called when inside the semaphore
            TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "Enter TryStartChaosAsync");

            ChaosParameters parameters = new ChaosParameters();
            var peak = this.PeakMoveState(Command.StartChaos);
            if (peak.Equals(SchedulerState.ChaosScheduleActive))
            {
                if (!this.eventInstancesEnumerator.HasEvents())
                {
                    // schedule is empty, no point trying to check for events.
                    return;
                }

                DateTime datetimeNow = DateTime.UtcNow;

                // if current time is after current event, check next event
                while (this.eventInstancesEnumerator.Current.CompareTo(datetimeNow) == -1)
                {
                    this.eventInstancesEnumerator.MoveNext();
                }

                if (this.eventInstancesEnumerator.Current.Start <= datetimeNow &&
                    datetimeNow <= this.eventInstancesEnumerator.Current.End)
                {
                    // start chaos
                    parameters = new ChaosParameters(this.eventInstancesEnumerator.Current.ChaosParameters);

                    if (datetimeNow.Add(this.eventInstancesEnumerator.Current.End - datetimeNow) >= this.scheduleDescription.Schedule.ExpiryDate)
                    {
                        // Give Chaos time to run that will not go past expiry of schedule.
                        parameters.TimeToRun = this.scheduleDescription.Schedule.ExpiryDate - datetimeNow;
                    }
                    else
                    {
                        parameters.TimeToRun = this.eventInstancesEnumerator.Current.End - datetimeNow;
                    }

                    if (parameters.TimeToRun.TotalSeconds < 1)
                    {
                        TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "TryStartChaosAsync not starting Chaos because time to run Chaos is less than 1 second");
                        return;
                    }

                    try
                    {
                        await this.chaosMessageProcessor.ProcessStartChaosOldAsync(parameters, cancellationToken).ConfigureAwait(false);
                        TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "Chaos started.");
                    }
                    catch (Exception ex)
                    {
                        string exceptionMessage = string.Format("TryStartChaosAsync - Failed to start chaos. Reason {0}", ex.Message);
                        TestabilityTrace.TraceSource.WriteError(TraceComponent, exceptionMessage);

                        await this.chaosMessageProcessor.ProcessStopChaosOldAsync(true).ConfigureAwait(false);
                        await this.TryMoveStateAsync(Command.StopChaos, cancellationToken).ConfigureAwait(false);

                        ChaosUtility.ThrowOrAssertIfTrue("ChaosScheduler::TryStartChaosAsync", true, exceptionMessage);

                        return;
                    }

                    await this.TryMoveStateAsync(Command.StartChaos, cancellationToken).ConfigureAwait(false);
                    this.CheckStateAndThrowOnError(SchedulerState.ChaosScheduleActive);
                }
                //// else we are before the start of the next event, so we just simply wait until then and do nothing.
            }
        }

        private async Task TryFinishChaosAsync(CancellationToken cancellationToken)
        {
            // Must only be called when inside the semaphore
            TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "Enter TryFinishChaosAsync");

            var peak = this.PeakMoveState(Command.FinishChaos);
            if (peak.Equals(SchedulerState.NoChaosScheduleActive))
            {
                if (this.eventInstancesEnumerator.HasEvents() && this.eventInstancesEnumerator.Current.CompareTo(DateTime.UtcNow) == -1)
                {
                    // Time now is pass the current event end time
                    TestabilityTrace.TraceSource.WriteWarning(TraceComponent, "Current event has gone on for too long, going to stop Chaos.");

                    await this.chaosMessageProcessor.ProcessStopChaosOldAsync(true).ConfigureAwait(false);
                    await this.TryMoveStateAsync(Command.FinishChaos, cancellationToken).ConfigureAwait(false);
                    this.CheckStateAndThrowOnError(SchedulerState.NoChaosScheduleActive);
                }
            }
        }

        private async Task StartAndRunAsync(CancellationToken cancellationToken)
        {
            if (!this.stateSemaphore.Wait(ChaosConstants.SchedulerLockWaitMilliseconds, cancellationToken))
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceComponent, "StartAndRunAsync - initial setup - did not acquire lock in provisioned time. Not progressing.");
                return;
            }

            try
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "stateSemaphore Acquired by scheduler in StartAndRunAsync initialization");
                await this.WriteStateToReliableStoreAsync(this.state, cancellationToken).ConfigureAwait(false);
            }
            catch (Exception ex)
            {
                string exceptionMessage = string.Format("StartAndRunAsync - failed to set initial state. Exception:{0}", ex.Message);
                TestabilityTrace.TraceSource.WriteError(TraceComponent, exceptionMessage);
                ChaosUtility.ThrowOrAssertIfTrue("ChaosScheduler::StartAndRunAsync", true, exceptionMessage);

                return;
            }
            finally
            {
                this.stateSemaphore.Release();
                TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "stateSemaphore Released by scheduler in StartAndRunAsync initialization");
            }

            while (!cancellationToken.IsCancellationRequested)
            {
                if (!this.stateSemaphore.Wait(ChaosConstants.SchedulerLockWaitMilliseconds, cancellationToken))
                {
                    TestabilityTrace.TraceSource.WriteWarning(TraceComponent, "StartAndRunAsync - scheduler loop - did not acquire lock in provisioned time.");
                    continue;
                }

                try
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "stateSemaphore Acquired by scheduler in StartAndRunAsync work loop");

                    SchedulerState stateSnapshot = new SchedulerState(this.state);

                    if (this.state.Equals(SchedulerState.NoChaosSchedulePending))
                    {
                        await this.TryMatureScheduleAsync(cancellationToken).ConfigureAwait(false);
                    }
                    else if (this.state.Equals(SchedulerState.NoChaosScheduleActive))
                    {
                        await this.TryExpireSchedule(cancellationToken).ConfigureAwait(false);
                        await this.TryStartChaosAsync(cancellationToken).ConfigureAwait(false);
                    }
                    else if (this.state.Equals(SchedulerState.ChaosScheduleActive))
                    {
                        await this.TryExpireSchedule(cancellationToken).ConfigureAwait(false);
                        await this.TryFinishChaosAsync(cancellationToken).ConfigureAwait(false);
                    }
                    //// Scheduler takes no action on all other states.
                }
                catch (InvalidOperationException ex)
                {
                    TestabilityTrace.TraceSource.WriteError(TraceComponent, "Scheduler encountered a bad state transaction {0}", ex.Message);

                    await this.WriteStateToReliableStoreAsync(SchedulerState.NoChaosScheduleStopped, cancellationToken).ConfigureAwait(false);
                    await this.chaosMessageProcessor.ProcessStopChaosOldAsync(true).ConfigureAwait(false);

                    this.state = new SchedulerState(SchedulerState.NoChaosScheduleStopped);
                }
                finally
                {
                    this.stateSemaphore.Release();
                    TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "stateSemaphore Released by scheduler in StartAndRunAsync work loop");
                }

                await Task.Delay(TimeSpan.FromMilliseconds(ChaosConstants.SchedulerCycleWaitIntervalMilliseconds), cancellationToken).ConfigureAwait(false);
            }
        }

        private SchedulerState PeakMoveState(Command command)
        {
            // Must only be called when inside the semaphore
            SchedulerState tempstate;
            StateTransition inState = new StateTransition(this.state, command);

            if (ChaosScheduler.TransitionTable.TryGetValue(inState, out tempstate))
            {
                return tempstate;
            }

            return this.state;
        }

        private async Task TryMoveStateAsync(Command command, CancellationToken cancellationToken)
        {
            // Must only be called when inside the semaphore
            TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "Enter TryMoveStateAsync");

            SchedulerState newState;
            if (ChaosScheduler.TransitionTable.TryGetValue(new StateTransition(this.state, command), out newState))
            {
                await this.WriteStateToReliableStoreAsync(newState, cancellationToken).ConfigureAwait(false);
                TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "State changed from {0} to {1}. Reason {2}.", this.state, newState, command);
                this.state = newState;
            }
            else
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceComponent, "Transition with command {0} from state {1} is not valid", command, this.state);
            }
        }

        /// <summary>
        /// Initialize the state of the scheduler and the schedule. Will attempt to recover information from the sources in order
        /// 1. RD of schedule and scheduler status
        /// 2. previously used ChaosParameters and ChaosEvents History
        /// 3. recover based on well known default of empty schedule and scheduler stopped.
        /// </summary>
        /// <param name="cancellationToken"></param>
        private async Task RestartRecoveryAsync(CancellationToken cancellationToken)
        {
            if (!this.stateSemaphore.Wait(ChaosConstants.SchedulerLockWaitMilliseconds, cancellationToken))
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceComponent, "RestartRecoveryAsync - did not acquire lock in provisioned time. Not progressing.");
                return;
            }

            try
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "stateSemaphore Acquired by RestartRecoveryAsync");

                if (await this.TryRecoveryFromSchedule(cancellationToken).ConfigureAwait(false))
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "ChaosScheduler recovered from saved schedule");
                }
                else if (await this.TryRecoveryFromHistory(cancellationToken).ConfigureAwait(false))
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "ChaosScheduler recovered from saved ChaosParameters and event history");
                }
                else
                {
                    await this.RecoverFromDefault(cancellationToken).ConfigureAwait(false);
                    TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "ChaosScheduler recovered from default values.");
                }
            }
            finally
            {
                this.stateSemaphore.Release();
                TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "stateSemaphore Released by RestartRecoveryAsync");
            }

            return;
        }

        /// <summary>
        /// Attempt to recover from status of the Chaos schedule and Chaos scheduler status from RD. Chaos will be running if it should be running.
        /// </summary>
        /// <returns>boolean representing if the recovery was successful.</returns>
        private async Task<bool> TryRecoveryFromSchedule(CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "TryRecoveryFromSchedule entered.");

            SchedulerState schedulerState = new SchedulerState(SchedulerState.NoChaosScheduleStopped);
            ChaosScheduleDescription chaosScheduleDescription = new ChaosScheduleDescription();

            this.StatusDictionary = this.StatusDictionary ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosSchedulerStateName).ConfigureAwait(false);
            using (ITransaction tx = this.StateManager.CreateTransaction())
            {
                var schedulerResult = await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure<ConditionalValue<byte[]>>(
                    Guid.Empty,
                    () => this.StatusDictionary.TryGetValueAsync(tx, FASConstants.ChaosSchedulerStatusDictionaryScheduleKey),
                    this.partition,
                    "RestartRecoveryAsync",
                    FASConstants.MaxRetriesForReliableDictionary,
                    cancellationToken).ConfigureAwait(false);

                var schedulerStateResult = await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure<ConditionalValue<byte[]>>(
                    Guid.Empty,
                    () => this.StatusDictionary.TryGetValueAsync(tx, FASConstants.ChaosSchedulerStatusDictionaryStateKey),
                    this.partition,
                    "RestartRecoveryAsync",
                    FASConstants.MaxRetriesForReliableDictionary,
                    cancellationToken).ConfigureAwait(false);

                if (!schedulerResult.HasValue || !schedulerStateResult.HasValue)
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "TryRecoveryFromSchedule failed to recover. Scheduler state or schedule was non existent.");
                    return false;
                }

                chaosScheduleDescription.FromBytes(schedulerResult.Value);
                schedulerState.FromBytes(schedulerStateResult.Value);

                await tx.CommitAsync().ConfigureAwait(false);
            }

            try
            {
                if (schedulerState.ScheduleStatus.Equals(ChaosScheduleStatus.Pending))
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "TryRecoveryFromSchedule scheduler state is pending");
                    await this.SetScheduleInternalAsync(chaosScheduleDescription, cancellationToken).ConfigureAwait(false);
                }
                else if (schedulerState.ScheduleStatus.Equals(ChaosScheduleStatus.Active))
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "TryRecoveryFromSchedule scheduler state is active");
                    await this.SetScheduleAndTryResumeAsync(chaosScheduleDescription, cancellationToken).ConfigureAwait(false);
                }

                // expire and stopped ChaosScheduleStatus will result in no action being taken
                // the schedule and status is still correct in the RD because that is where the values were read from
            }
            catch (System.ArgumentException ex)
            {
                string exceptionMessage = string.Format("RestartRecoveryAsync - failed to recover chaos schedule. Reason {0}", ex.Message);
                TestabilityTrace.TraceSource.WriteError(TraceComponent, exceptionMessage);

                ChaosUtility.ThrowOrAssertIfTrue("ChaosScheduler::RestartRecoveryAsync", true, exceptionMessage);
            }

            return true;
        }

        /// <summary>
        /// Attempt to recover from saved Chaos parameters and history of events. Will only recover if Chaos was running and needs to run.
        /// </summary>
        /// <returns>boolean representing if the recovery was successful.</returns>
        private async Task<bool> TryRecoveryFromHistory(CancellationToken cancellationToken)
        {
            bool chaosWasRunning = await this.chaosMessageProcessor.StateManager.ChaosWasRunningAsync(this.partition, cancellationToken).ConfigureAwait(false);

            if (!chaosWasRunning)
            {
                // If chaos was not running then there is no need to try recovering Chaos.
                TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "TryRecoveryFromHistory failed to recover. Chaos was not running.");
                return false;
            }

            var chaosParams = await this.chaosMessageProcessor.GetChaosParametersAsync(Guid.NewGuid()).ConfigureAwait(false);
            var chaosStartTime = await this.chaosMessageProcessor.GetChaosStartTimeAsync(Guid.NewGuid()).ConfigureAwait(false);

            ChaosSchedule schedule = ChaosSchedulerUtil.Create247Schedule(chaosParams, chaosStartTime, chaosStartTime + chaosParams.TimeToRun);

            await this.SetScheduleAndTryResumeAsync(new ChaosScheduleDescription(0, schedule), cancellationToken).ConfigureAwait(false);

            return true;
        }

        /// <summary>
        /// Recover by setting schedule to empty and status to stopped. No Chaos will be running.
        /// </summary>
        private async Task RecoverFromDefault(CancellationToken cancellationToken)
        {
            // Can't restart Chaos if it was running, make sure a stop event is registered.
            await this.chaosMessageProcessor.RegisterStoppedEventForRestartFailureAsync().ConfigureAwait(false);

            SchedulerState schedulerState = new SchedulerState(SchedulerState.NoChaosScheduleStopped);
            ChaosScheduleDescription chaosScheduleDescription = new ChaosScheduleDescription();

            await ChaosSchedulerUtil.VerifyChaosScheduleAsync(chaosScheduleDescription.Schedule, this.fabricClient, cancellationToken);

            // Normally, the schedule and status should not be set directly but this is an exception as
            // this is a setup step and we are only setting the state to the initial entry state
            this.scheduleDescription = chaosScheduleDescription;
            this.state = schedulerState;

            await this.WriteScheduleToReliableStoreAsync(this.scheduleDescription, cancellationToken).ConfigureAwait(false);
            await this.WriteStateToReliableStoreAsync(this.state, cancellationToken).ConfigureAwait(false);
        }

        private async Task WriteStateToReliableStoreAsync(SchedulerState schedulerState, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceComponent, "Enter WriteStateToReliableStoreAsync");

            this.StatusDictionary = this.StatusDictionary ?? await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosSchedulerStateName).ConfigureAwait(false);

            using (ITransaction tx = this.StateManager.CreateTransaction())
            {
                await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                    Guid.Empty,
                    () => this.StatusDictionary.SetAsync(tx, FASConstants.ChaosSchedulerStatusDictionaryStateKey, schedulerState.ToBytes()),
                    this.partition,
                    "WriteStateToReliableStoreAsync",
                    FASConstants.MaxRetriesForReliableDictionary,
                    cancellationToken).ConfigureAwait(false);

                await tx.CommitAsync().ConfigureAwait(false);
            }
        }

        private void CheckStateAndThrowOnError(SchedulerState expectedState)
        {
            if (!this.state.Equals(expectedState))
            {
                throw new InvalidOperationException(string.Format(StringResources.ChaosScheduler_InconsistentState, this.state, expectedState));
            }
        }

        private sealed class StateTransition
        {
            private SchedulerState state;
            private Command command;

            public StateTransition(SchedulerState state, Command command)
            {
                this.state = state;
                this.command = command;
            }

            public StateTransition(ChaosScheduleStatus scheduleStatus, ChaosStatus chaosStatus, Command command)
            {
                this.state = new SchedulerState(scheduleStatus, chaosStatus);
                this.command = command;
            }

            public override string ToString()
            {
                return string.Format("Scheduler State: {0}, Comand: {1}", this.state, Enum.GetName(typeof(Command), this.command));
            }

            public override int GetHashCode()
            {
                return 17
                       + (31 * this.state.GetHashCode())
                       + (37 * this.command.GetHashCode());
            }

            public override bool Equals(object obj)
            {
                StateTransition other = obj as StateTransition;

                return other != null
                       && this.state.Equals(other.state)
                       && this.command == other.command;
            }
        }
    }
}
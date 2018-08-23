// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Engine
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.FaultAnalysis.Service.Actions;
    using System.Fabric.FaultAnalysis.Service.Actions.Steps;
    using System.Fabric.FaultAnalysis.Service.Common;
    using System.Fabric.Query;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using Description;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Collections;
    using TestabilityTrace = System.Fabric.Common.TestabilityTrace;

    internal class ActionStore
    {
        private const string TraceType = "ActionStore";
        private const int MaxRetries = 3;

        private static readonly TimeSpan DefaultDictionaryTimeout = TimeSpan.FromSeconds(4.0d);

        private readonly IReliableStateManager stateManager;
        private readonly IStatefulServicePartition partition;

        // Actions that are running, or that have not started.  Access pattern: when both this and historyTable are accessed, this should always be accessed before historyTable.
        private readonly IReliableDictionary<Guid, byte[]> actionTable;

        // Stores actions in terminal state that are no longer running.  See comment above about access pattern.
        private readonly IReliableDictionary<Guid, byte[]> historyTable;
        private readonly IReliableDictionary<string, bool> stoppedNodeTable;
        private readonly bool isTestMode;
        private readonly CancellationToken cancellationToken;

        private Timer truncateTimer;

        private long maxStoredActionCount;
        private int storedActionCleanupIntervalInSeconds;
        private int completedActionKeepDurationInSeconds;

        public ActionStore(
            IReliableStateManager stateManager,
            IStatefulServicePartition partition,
            IReliableDictionary<Guid, byte[]> actionTable,
            IReliableDictionary<Guid, byte[]> historyTable,
            IReliableDictionary<string, bool> stoppedNodeTable,
            long maxStoredActionCount,
            int storedActionCleanupIntervalInSeconds,
            int completedActionKeepDurationInSeconds,
            bool isTestMode,
            CancellationToken cancellationToken)
        {
            ThrowIf.Null(stateManager, "stateManager");
            ThrowIf.Null(actionTable, "actionTable");
            ThrowIf.Null(historyTable, "historyTable");
            ThrowIf.Null(stoppedNodeTable, "stoppedNodeTable");

            ThrowIf.OutOfRange(maxStoredActionCount, 0, long.MaxValue, "maxStoredActionCount");
            ThrowIf.OutOfRange(storedActionCleanupIntervalInSeconds, 0, int.MaxValue, "storedActionCleanupIntervalInSeconds");
            ThrowIf.OutOfRange(completedActionKeepDurationInSeconds, 0, int.MaxValue, "completedActionKeepDurationInSeconds");

            this.stateManager = stateManager;
            this.partition = partition;

            this.actionTable = actionTable;
            this.historyTable = historyTable;
            this.stoppedNodeTable = stoppedNodeTable;

            this.maxStoredActionCount = maxStoredActionCount;
            this.storedActionCleanupIntervalInSeconds = storedActionCleanupIntervalInSeconds;
            this.completedActionKeepDurationInSeconds = completedActionKeepDurationInSeconds;
            this.isTestMode = isTestMode;
            this.cancellationToken = cancellationToken;

            this.truncateTimer = new Timer(this.TruncateCallback, null, Timeout.Infinite, Timeout.Infinite);
            this.truncateTimer.Change(TimeSpan.FromSeconds(this.storedActionCleanupIntervalInSeconds), Timeout.InfiniteTimeSpan);
            cancellationToken.Register(this.CancelTimer);
        }

        public Task InitializeNewActionAsync(ActionStateBase actionState, TimeSpan timeout)
        {
            return FaultAnalysisServiceUtility.RetryOnTimeout(
                actionState.OperationId,
                () => this.InitializeNewActionInnerAsync(actionState),
                timeout,
                this.cancellationToken);
        }

        public async Task<string> GetStoppedNodeListAsync()
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter ActionStore.GetStoppedNodeListAsync");

            List<string> stoppedNodes = new List<string>();
            using (var tx = this.stateManager.CreateTransaction())
            {
                var enumerable = await this.stoppedNodeTable.CreateEnumerableAsync(tx).ConfigureAwait(false);

                var enumerator = enumerable.GetAsyncEnumerator();
                while (await enumerator.MoveNextAsync(CancellationToken.None))
                {
                    string s = enumerator.Current.Key;
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "ActionStore.GetStoppedNodeListAsync adding stopped node {0}", s);
                    stoppedNodes.Add(s);
                }
            }

            return string.Join(";", stoppedNodes);
        }

        // Called during execution of an Action to move the Action to the next Step.  The next Step may be backwards in the case of rollback.
        public Task UpdateActionStateAsync(ActionStateBase actionState)
        {
            return this.UpdateActionStateAsync(actionState, false);
        }

        public async Task UpdateActionStateAsync(ActionStateBase actionState, bool wasCalledFromCancelApiPath)
        {
            await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                actionState.OperationId,
                () => this.UpdateActionStateInnerAsync(actionState, wasCalledFromCancelApiPath),
                this.partition,
                "UpdateActionStateAsync",
                ActionStore.MaxRetries,
                this.cancellationToken).ConfigureAwait(false);
        }

        public async Task UpdateActionStateInnerAsync(ActionStateBase actionState, bool wasCalledFromCancelApiPath)
        {
            if (!wasCalledFromCancelApiPath)
            {
                // This is from an engine update.  In this case it needs to check to see if the user called the cancel api before writing.  That call inside overwrite the flag, but does not persist
                RollbackState readRollBackState = await this.CheckUserCancellationAndUpdateIfNeededAsync(actionState, CancellationToken.None, string.Empty).ConfigureAwait(false);
                if (readRollBackState == RollbackState.RollingBackDueToUserCancel
                    || readRollBackState == RollbackState.RollingBackForce)
                {
                    actionState.RollbackState = readRollBackState;
                }
            }

            await this.PersistAsync(actionState).ConfigureAwait(false);
        }

        public Task<IEnumerable<ActionStateBase>> GetRunningActionsAsync()
        {
            TestCommandListDescription description = new TestCommandListDescription(
                TestCommandStateFilter.Running | TestCommandStateFilter.RollingBack,
                TestCommandTypeFilter.All);
            return this.GetSelectedActionsAsync(description);
        }

        public async Task<IEnumerable<ActionStateBase>> GetSelectedActionsAsync(TestCommandListDescription description)
        {
            TestCommandStateFilter stateFilter = description.TestCommandStateFilter;
            TestCommandTypeFilter typeFilter = description.TestCommandTypeFilter;

            TestabilityTrace.TraceSource.WriteInfo(TraceType, "GetSelectedActionsAsync() stateFilter={0}, typeFilter={1}", stateFilter.ToString(), typeFilter.ToString());
            List<ActionStateBase> selectedActions = new List<ActionStateBase>();

            try
            {
                await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                    Guid.Empty,
                    () => this.GetSelectedActionsInnerAsync(selectedActions, stateFilter, typeFilter),
                    this.partition,
                    "Replace",
                    ActionStore.MaxRetries,
                    this.cancellationToken).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteError(TraceType, e.ToString());
                throw;
            }

            return selectedActions;
        }

        public async Task GetSelectedActionsInnerAsync(List<ActionStateBase> selectedActions, TestCommandStateFilter stateFilter, TestCommandTypeFilter typeFilter)
        {
            selectedActions.Clear();

            // non-terminal state
            if (stateFilter.HasFlag(TestCommandStateFilter.Running) ||
                stateFilter.HasFlag(TestCommandStateFilter.RollingBack))
            {
                await this.AddActionsMatchingFilterAsync(this.actionTable, selectedActions, stateFilter, typeFilter).ConfigureAwait(false);
            }

            // terminal state
            if (stateFilter.HasFlag(TestCommandStateFilter.CompletedSuccessfully) ||
                stateFilter.HasFlag(TestCommandStateFilter.Failed) ||
                stateFilter.HasFlag(TestCommandStateFilter.Cancelled) ||
                stateFilter.HasFlag(TestCommandStateFilter.ForceCancelled))
            {
                await this.AddActionsMatchingFilterAsync(this.historyTable, selectedActions, stateFilter, typeFilter).ConfigureAwait(false);
            }
        }

        public async Task<ActionStateBase> FindByOperationIdAsync(Guid operationId, CancellationToken cancellationToken)
        {
            ActionStateBase state = null;
            try
            {
                using (var tx = this.stateManager.CreateTransaction())
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter transaction inside {0} - operationId={1}", "FindByOperationId", operationId);
                    var data = await this.actionTable.TryGetValueAsync(tx, operationId, DefaultDictionaryTimeout, this.cancellationToken).ConfigureAwait(false);
                    if (data.HasValue)
                    {
                        TestabilityTrace.TraceSource.WriteInfo(TraceType, "FindOperationId: {0}, actiontable has value", operationId);
                        state = this.ReadData(data.Value);
                    }
                    else
                    {
                        data = await this.historyTable.TryGetValueAsync(tx, operationId, DefaultDictionaryTimeout, this.cancellationToken).ConfigureAwait(false);
                        if (data.HasValue)
                        {
                            TestabilityTrace.TraceSource.WriteInfo(TraceType, "FindOperationId: {0}, history table has value", operationId);
                            state = this.ReadData(data.Value);
                        }
                        else
                        {
                            string error = string.Format(CultureInfo.InvariantCulture, "OperationId '{0}' was not found", operationId);
                            TestabilityTrace.TraceSource.WriteWarning(TraceType, error);
                            throw FaultAnalysisServiceUtility.CreateException(TraceType, Interop.NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_KEY_NOT_FOUND, error);
                        }
                    }
                }

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Exit transaction inside {0} - operationId={1}", "FindByOperationId", operationId);
            }
            catch (FabricNotPrimaryException fnp)
            {
                FaultAnalysisServiceUtility.TraceFabricNotPrimary(operationId, fnp);
                throw;
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "FindOperationId: {0}", e.ToString());
                throw;
            }

            return state;
        }

        public async Task CancelTestCommandAsync(Guid operationId, bool force)
        {
            ActionStateBase state = null;
            try
            {
                using (var tx = this.stateManager.CreateTransaction())
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter transaction inside {0} - operationId={1}", "CancelTestCommandAsync", operationId);
                    var data = await this.actionTable.TryGetValueAsync(tx, operationId, DefaultDictionaryTimeout, this.cancellationToken).ConfigureAwait(false);
                    if (data.HasValue)
                    {
                        state = this.ReadData(data.Value);
                    }
                    else
                    {
                        string error = string.Format(CultureInfo.InvariantCulture, "OperationId '{0}' was not found", operationId);
                        TestabilityTrace.TraceSource.WriteWarning(TraceType, error);
                        throw FaultAnalysisServiceUtility.CreateException(TraceType, Interop.NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_KEY_NOT_FOUND, error);
                    }

                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "CancelActionAsync: operationId={0}, force={1}", operationId, force);

                    if (force)
                    {
                        if (state.RollbackState != RollbackState.RollingBackDueToUserCancel)
                        {
                            string error = string.Format(CultureInfo.InvariantCulture, "CancelActionAsync: operationId={0}, force is not allowed for something in {1}", operationId, state.RollbackState.ToString());
                            TestabilityTrace.TraceSource.WriteError(TraceType, error);

                            throw FaultAnalysisServiceUtility.CreateException(TraceType, Interop.NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_TEST_COMMAND_STATE, error);
                        }
                        else
                        {
                            state.RollbackState = RollbackState.RollingBackForce;
                        }
                    }
                    else
                    {
                        // if it's in force, don't allow it back down to graceful                        
                        if (state.RollbackState == RollbackState.RollingBackForce)
                        {
                            string error = string.Format(
                                CultureInfo.InvariantCulture,
                                "CancelActionAsync: operationId={0}, CancelTestCommandAsync has already been called with force==true.  Switching to force==false is not valid.",
                                operationId);
                            TestabilityTrace.TraceSource.WriteWarning(TraceType, error);

                            throw FaultAnalysisServiceUtility.CreateException(TraceType, Interop.NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_TEST_COMMAND_STATE, error);
                        }

                        state.RollbackState = RollbackState.RollingBackDueToUserCancel;
                    }

                    byte[] stateToPersist = state.ToBytes();

                    await this.actionTable.AddOrUpdateAsync(tx, state.OperationId, stateToPersist, (k, v) => stateToPersist, DefaultDictionaryTimeout, this.cancellationToken).ConfigureAwait(false);
                    await tx.CommitAsync().ConfigureAwait(false);
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "CancelActionAsync: operationId={0}, force={1} committed", operationId, force);
                }

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter transaction inside {0} - operationId={1}", "CancelTestCommandAsync", operationId);
            }
            catch (FabricNotPrimaryException fnp)
            {
                FaultAnalysisServiceUtility.TraceFabricNotPrimary(operationId, fnp);
                throw;
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteError(TraceType, "CancelActionAsync: {0}", e.ToString());
                throw;
            }
        }

        public void TruncateCallback(object state)
        {
            Task.Factory.StartNew(() => this.TruncateCallbackInnerAsync(), this.cancellationToken);
        }

        // This method should only run one at a time (controlled by timer)        
        public async Task TruncateCallbackInnerAsync()
        {
            bool observedException = false;
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter truncate callback");

            try
            {
                long count = await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                    Guid.Empty,
                    () => this.GetActionCountAsync(true),
                    this.partition,
                    "GetActionCountAsync",
                    ActionStore.MaxRetries,
                    this.cancellationToken).ConfigureAwait(false);

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Action store size is {0}", count);
                if (count > this.maxStoredActionCount)
                {
                    await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                        Guid.Empty,
                        () => this.TruncateAsync(count),
                        this.partition,
                        "TruncateCallbackInnerAsync",
                        ActionStore.MaxRetries,
                        this.cancellationToken).ConfigureAwait(false);
                }
            }
            catch (FabricNotPrimaryException fnp)
            {
                FaultAnalysisServiceUtility.TraceFabricNotPrimary(Guid.Empty, fnp);
                observedException = true;
            }
            catch (FabricObjectClosedException foc)
            {
                FaultAnalysisServiceUtility.TraceFabricObjectClosed(Guid.Empty, foc);
                observedException = true;
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Error: {0}", e.ToString());
                observedException = true;
                throw;
            }

            if (!observedException)
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Rescheduling timer for {0} seconds", this.storedActionCleanupIntervalInSeconds);
                this.truncateTimer.Change(TimeSpan.FromSeconds(this.storedActionCleanupIntervalInSeconds), Timeout.InfiniteTimeSpan);
            }
        }

        public async Task<long> GetActionCountAsync(bool excludeRunningActions)
        {
            long count = 0;
            try
            {
                using (var tx = this.stateManager.CreateTransaction())
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter transaction inside {0} - operationId={1}, operationId.GetHashCode={2}, tx.TranslationId={3}, tx.GetHashCode={4}", "GetActionCountAsync", Guid.Empty, 0, tx.TransactionId, tx.GetHashCode());
                    if (!excludeRunningActions)
                    {
                        count = await this.actionTable.GetCountAsync(tx).ConfigureAwait(false);
                    }

                    count += await this.historyTable.GetCountAsync(tx).ConfigureAwait(false);

                    await tx.CommitAsync().ConfigureAwait(false);
                }

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter transaction inside {0} - operationId={1}, operationId.GetHashCode={2}", "GetActionCountAsync", Guid.Empty, 0);
            }
            catch (FabricNotPrimaryException fnp)
            {
                FaultAnalysisServiceUtility.TraceFabricNotPrimary(Guid.Empty, fnp);
                throw;
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteError(TraceType, "GetActionCountAsync: {0}", e.ToString());
                throw;
            }

            return count;
        }

        internal async Task<RollbackState> CheckUserCancellationAndUpdateIfNeededAsync(ActionStateBase actionState, CancellationToken cancellationToken, string location)
        {
            ActionStateBase readActionState = null;
            readActionState = await this.FindByOperationIdAsync(actionState.OperationId, cancellationToken).ConfigureAwait(false);
            TestabilityTrace.TraceSource.WriteNoise(TraceType, "CheckUserCancellationAndUpdateIfNeededAsync() {0} location='{1}' read rollbackstate={2}", actionState.OperationId, location, readActionState.RollbackState);

            return readActionState.RollbackState;
        }

        private static bool MatchesStateFilter(ActionStateBase actionState, TestCommandStateFilter filter)
        {
            if (filter == TestCommandStateFilter.All)
            {
                return true;
            }

            if (filter == TestCommandStateFilter.Default)
            {
                // future use
                return true;
            }

            StepStateNames stateName = actionState.StateProgress.Peek();
            RollbackState rollbackState = actionState.RollbackState;

            if (filter.HasFlag(TestCommandStateFilter.RollingBack))
            {
                if ((rollbackState == RollbackState.RollingBackAndWillFailAction
                    || rollbackState == RollbackState.RollingBackForce
                    || rollbackState == RollbackState.RollingBackDueToUserCancel)
                    &&
                    (stateName != StepStateNames.Failed))
                {
                    return true;
                }
            }

            if (filter.HasFlag(TestCommandStateFilter.Cancelled))
            {
                // stateName == CompletedSuccessfully is possible if the command was in its last step, and finished the last step successfully
                // before cancellation status was read.  There is no point in "cancelling" since the point is to stop execution.  Since it completed successfully,
                // that has been achieved.  However, the status should be translated here so it is not confusing to user.
                if ((stateName == StepStateNames.Failed || stateName == StepStateNames.CompletedSuccessfully) &&
                    (rollbackState == RollbackState.RollingBackDueToUserCancel))
                {
                    return true;
                }
            }

            if (filter.HasFlag(TestCommandStateFilter.ForceCancelled))
            {
                // See notes above in Cancelled section, which also applies here.
                if ((stateName == StepStateNames.Failed || stateName == StepStateNames.CompletedSuccessfully)
                    && (rollbackState == RollbackState.RollingBackForce))
                {
                    return true;
                }
            }

            switch (stateName)
            {
                case StepStateNames.IntentSaved:
                    return filter.HasFlag(TestCommandStateFilter.Running);
                case StepStateNames.LookingUpState:
                    return filter.HasFlag(TestCommandStateFilter.Running);
                case StepStateNames.PerformingActions:
                    return filter.HasFlag(TestCommandStateFilter.Running);
                case StepStateNames.CompletedSuccessfully:
                    return filter.HasFlag(TestCommandStateFilter.CompletedSuccessfully);
                case StepStateNames.Failed:
                    return filter.HasFlag(TestCommandStateFilter.Failed);
                default:
                    return false;
            }
        }

        private static bool MatchesTypeFilter(ActionType actionType, TestCommandTypeFilter filter)
        {
            if (filter == TestCommandTypeFilter.All)
            {
                return true;
            }

            if (filter == TestCommandTypeFilter.Default)
            {
                // future use
                return true;
            }

            switch (actionType)
            {
                case ActionType.InvokeDataLoss:
                    return filter.HasFlag(TestCommandTypeFilter.PartitionDataLoss);
                case ActionType.InvokeQuorumLoss:
                    return filter.HasFlag(TestCommandTypeFilter.PartitionQuorumLoss);
                case ActionType.RestartPartition:
                    return filter.HasFlag(TestCommandTypeFilter.PartitionRestart);
                case ActionType.StartNode:
                case ActionType.StopNode:
                    return filter.HasFlag(TestCommandTypeFilter.NodeTransition);
                default:
                    return false;
            }
        }

        // Called to initialize state when a new message is received.
        private async Task InitializeNewActionInnerAsync(ActionStateBase actionState)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter InitializeNewActionAsync");

            try
            {
                using (var tx = this.stateManager.CreateTransaction())
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter transaction inside InitializeNewActionAsync - operationId={0}", actionState.OperationId);
                    bool keyAlreadyExists = await this.actionTable.ContainsKeyAsync(tx, actionState.OperationId, DefaultDictionaryTimeout, this.cancellationToken).ConfigureAwait(false);
                    if (!keyAlreadyExists)
                    {
                        keyAlreadyExists = await this.historyTable.ContainsKeyAsync(tx, actionState.OperationId, DefaultDictionaryTimeout, this.cancellationToken).ConfigureAwait(false);
                    }

                    if (keyAlreadyExists)
                    {
                        string error = string.Format(CultureInfo.InvariantCulture, "OperationId '{0}' already exists", actionState.OperationId);
                        TestabilityTrace.TraceSource.WriteWarning(TraceType, error);
                        throw FaultAnalysisServiceUtility.CreateException(TraceType, Interop.NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TEST_COMMAND_OPERATION_ID_ALREADY_EXISTS, error);
                    }
                }

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Exit transaction inside InitializeNewActionAsync - operationId={0}", actionState.OperationId);
            }
            catch (FabricNotPrimaryException fnp)
            {
                FaultAnalysisServiceUtility.TraceFabricNotPrimary(actionState.OperationId, fnp);
                throw;
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "Could not add new action: {0}", e.ToString());
                throw;
            }

            await this.PersistAsync(actionState, true).ConfigureAwait(false);
        }

        private async Task AddActionsMatchingFilterAsync(IReliableDictionary<Guid, byte[]> dictionary, List<ActionStateBase> selectedActions, TestCommandStateFilter stateFilter, TestCommandTypeFilter typeFilter)
        {
            using (var tx = this.stateManager.CreateTransaction())
            {
                var enumerable = await dictionary.CreateEnumerableAsync(tx).ConfigureAwait(false);

                var enumerator = enumerable.GetAsyncEnumerator();
                while (await enumerator.MoveNextAsync(CancellationToken.None))
                {
                    ActionStateBase actionState = this.ReadData(enumerator.Current.Value);

                    StepStateNames stateName = actionState.StateProgress.Peek();
                    if (MatchesStateFilter(actionState, stateFilter) && MatchesTypeFilter(actionState.ActionType, typeFilter))
                    {
                        selectedActions.Add(actionState);
                    }
                    else
                    {
                        TestabilityTrace.TraceSource.WriteNoise(TraceType, "{0} - Current action does not match filter, not adding", actionState.OperationId);
                    }
                }
            }
        }

        // This method should only run one at a time (controlled by timer)
        private async Task TruncateAsync(long totalActionCount)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter TruncateAsync, count is {0}", totalActionCount);
            List<ActionStateBase> completedActionsMatchingFilter = new List<ActionStateBase>();

            long numberToTruncate = totalActionCount - this.maxStoredActionCount;

            using (var tx = this.stateManager.CreateTransaction())
            {
                var enumerable = await this.historyTable.CreateEnumerableAsync(tx).ConfigureAwait(false);
                var enumerator = enumerable.GetAsyncEnumerator();
                while (await enumerator.MoveNextAsync(this.cancellationToken).ConfigureAwait(false))
                {
                    ActionStateBase actionState = this.ReadData(enumerator.Current.Value);
                    StepStateNames stateName = actionState.StateProgress.Peek();

                    if (stateName == StepStateNames.CompletedSuccessfully || stateName == StepStateNames.Failed)
                    {
                        if (DateTime.UtcNow - actionState.TimeStopped > TimeSpan.FromSeconds(this.completedActionKeepDurationInSeconds))
                        {
                            TestabilityTrace.TraceSource.WriteInfo(TraceType, "DEBUG - Adding action {0} which ended at {1}, time now is {2}", actionState.OperationId, actionState.TimeStopped, DateTime.UtcNow);
                            completedActionsMatchingFilter.Add(actionState);
                        }
                    }
                }

                if (completedActionsMatchingFilter.Count > numberToTruncate)
                {
                    // Sort by completed date and cut 'numberToCut' of those.
                    completedActionsMatchingFilter.Sort((a, b) => { return a.TimeStopped.CompareTo(b.TimeStopped); });
                    int numToTruncateAsInt = (int)numberToTruncate;
                    completedActionsMatchingFilter.RemoveRange(numToTruncateAsInt, completedActionsMatchingFilter.Count - numToTruncateAsInt);
                }

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "TruncateAsync Removing '{0}' completed actions", completedActionsMatchingFilter.Count);
                foreach (ActionStateBase a in completedActionsMatchingFilter)
                {
                    await this.historyTable.TryRemoveAsync(tx, a.OperationId, DefaultDictionaryTimeout, this.cancellationToken).ConfigureAwait(false);
                }

                await tx.CommitAsync().ConfigureAwait(false);
            }
        }

        private Task PersistAsync(ActionStateBase actionState)
        {
            return this.PersistAsync(actionState, false);
        }

        // "throwException" - if this call originated from a client call, the exception should be thrown back.
        private async Task PersistAsync(ActionStateBase actionState, bool throwException)
        {
            StepStateNames currentStateName = actionState.StateProgress.Peek();

            byte[] stateToPersist = actionState.ToBytes();

            try
            {
                using (var tx = this.stateManager.CreateTransaction())
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter transaction inside {0} - operationId={1}", "PersistAsync", actionState.OperationId);
                    if (actionState.StateProgress.Peek() == StepStateNames.CompletedSuccessfully ||
                        actionState.StateProgress.Peek() == StepStateNames.Failed)
                    {
                        await this.actionTable.TryRemoveAsync(tx, actionState.OperationId, DefaultDictionaryTimeout, this.cancellationToken).ConfigureAwait(false);

                        TestabilityTrace.TraceSource.WriteInfo(
                            TraceType,
                            "Adding {0} to history table state={1}, rollbackState={2}",
                            actionState.OperationId,
                            actionState.StateProgress.Peek(),
                            actionState.RollbackState.ToString());
                        await this.historyTable.AddOrUpdateAsync(tx, actionState.OperationId, stateToPersist, (k, v) => stateToPersist, DefaultDictionaryTimeout, this.cancellationToken).ConfigureAwait(false);
                    }
                    else
                    {
                        await this.actionTable.AddOrUpdateAsync(tx, actionState.OperationId, stateToPersist, (k, v) => stateToPersist, DefaultDictionaryTimeout, this.cancellationToken).ConfigureAwait(false);
                    }

                    await tx.CommitAsync().ConfigureAwait(false);
                }

                try
                {
                    if (actionState.StateProgress.Peek() == StepStateNames.CompletedSuccessfully)
                    {
                        await actionState.AfterSuccessfulCompletion().ConfigureAwait(false);
                    }
                    else if (actionState.StateProgress.Peek() == StepStateNames.Failed)
                    {
                        await actionState.AfterFatalRollback().ConfigureAwait(false);
                    }
                }
                catch (Exception ex)
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} afterX threw {1}, reporting fault", actionState.OperationId, ex);
                    this.partition.ReportFault(FaultType.Transient);
                }

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter transaction inside {0} - operationId={1}", "PersistAsync", actionState.OperationId);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Error in persisting in state " + currentStateName + "  " + e.ToString());
                throw;
            }

            if (this.isTestMode)
            {
                this.TestModeSerialize(actionState);
            }

            TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} Exiting PersistAsync", actionState.OperationId);
        }

        private void CancelTimer()
        {
            if (this.truncateTimer != null)
            {
                this.truncateTimer.Change(Timeout.Infinite, Timeout.Infinite);
                this.truncateTimer.Dispose();
                this.truncateTimer = null;
            }
        }

        private ActionStateBase ReadData(byte[] bytes)
        {
            ActionStateBase result = null;
            using (BinaryReader br = new BinaryReader(new MemoryStream(bytes)))
            {
                // The first 4 bytes are the command type
                ActionType a = ActionStateBase.ReadCommandType(br);

                if (a == ActionType.InvokeDataLoss)
                {
                    result = InvokeDataLossState.FromBytes(br);
                }
                else if (a == ActionType.InvokeQuorumLoss)
                {
                    result = InvokeQuorumLossState.FromBytes(br);
                }
                else if (a == ActionType.RestartPartition)
                {
                    result = RestartPartitionState.FromBytes(br);
                }
                else if (a == ActionType.TestStuck)
                {
                    result = StuckState.FromBytes(br);
                }
                else if (a == ActionType.TestRetryStep)
                {
                    result = TestRetryStepState.FromBytes(br);
                }
                else if (a == ActionType.StartNode)
                {
                    result = NodeCommandState.FromBytes(br, a);
                }
                else if (a == ActionType.StopNode)
                {
                    result = NodeCommandState.FromBytes(br, a);
                }
            }

            return result;
        }

        private void TestModeSerialize(ActionStateBase state)
        {
            if (this.isTestMode)
            {
                // DEBUG/////////////////////////////////////////////
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Begin test");
                try
                {
                    byte[] bytes = state.ToBytes();
                    ActionStateBase deserializedData = this.ReadData(bytes);
                    if (!state.VerifyEquals(deserializedData))
                    {
                        ReleaseAssert.Failfast("{0} data should match but does not", state.OperationId);
                    }
                }
                catch (Exception e)
                {
                    TestabilityTrace.TraceSource.WriteError(TraceType, "{0} - TestModeSerialize exception {1}", state.OperationId.ToString(), e.ToString());
                    throw;
                }
            }
        }
    }
}
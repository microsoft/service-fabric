// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Engine
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.FaultAnalysis.Service.Actions;
    using System.Fabric.FaultAnalysis.Service.Actions.Steps;
    using System.Fabric.FaultAnalysis.Service.Common;
    using System.Fabric.FaultAnalysis.Service.Test;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using FabricTestAction = System.Fabric.FaultAnalysis.Service.Actions.FabricTestAction;
    using TestabilityTrace = System.Fabric.Common.TestabilityTrace;

    internal class ReliableFaultsEngine
    {
        private const string TraceType = "Engine";
        private readonly Random random;
        private readonly ActionStore actionStore;
        private readonly bool isTestMode;
        private readonly IStatefulServicePartition partition;
        private readonly int commandStepRetryBackoffInSeconds;

        public ReliableFaultsEngine(ActionStore actionStore, bool isTestMode, IStatefulServicePartition partition, int commandStepRetryBackoffInSeconds)
        {
            ThrowIf.Null(actionStore, "actionStore");

            this.random = new Random();

            this.actionStore = actionStore;
            this.isTestMode = isTestMode;

            this.partition = partition;

            this.commandStepRetryBackoffInSeconds = commandStepRetryBackoffInSeconds;
        }

        public async Task RunAsync(FabricClient fc, FabricTestAction action, ActionStateBase actionState, ServiceInternalFaultInfo serviceInternalFaultInfo, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} - Inside RunAsync of Engine, entering state machine", actionState.OperationId);
            try
            {
                do
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    RollbackState readRollbackState = await this.CheckUserCancellationAndUpdateIfNeededAsync(actionState, cancellationToken, FASConstants.OuterLoop).ConfigureAwait(false);

                    // For the non-force case we need to cleanup, so that is why there's no break statement in that case.
                    if (readRollbackState == RollbackState.RollingBackForce)
                    {
                        actionState.StateProgress.Push(StepStateNames.Failed);
                        await this.actionStore.UpdateActionStateAsync(actionState).ConfigureAwait(false);
                        break;
                    }

                    await this.RunStateMachineAsync(fc, action, actionState, serviceInternalFaultInfo, cancellationToken).ConfigureAwait(false);

                    if (actionState.RollbackState == RollbackState.RollingBackAndWillRetryAction)
                    {
                        actionState.ErrorCausingRollback = 0;
                        int pauseTime = this.random.Next(10, 60);
                        TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} - Pausing for {1} seconds before retrying", actionState.OperationId, pauseTime);

                        // Clear the rollback state so it will go forward when it resumes.
                        actionState.RollbackState = RollbackState.NotRollingBack;
                        await this.actionStore.UpdateActionStateAsync(actionState).ConfigureAwait(false);
                        await Task.Delay(TimeSpan.FromSeconds(pauseTime), cancellationToken).ConfigureAwait(false);
                    }
                }
                while (actionState.StateProgress.Peek() != StepStateNames.CompletedSuccessfully &&
                    actionState.StateProgress.Peek() != StepStateNames.Failed);
            }
            catch (FabricNotPrimaryException notPrimary)
            {
                FaultAnalysisServiceUtility.TraceFabricNotPrimary(actionState.OperationId, notPrimary);
            }
            catch (FabricObjectClosedException objectClosed)
            {
                FaultAnalysisServiceUtility.TraceFabricObjectClosed(actionState.OperationId, objectClosed);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} caught exception - {1}", actionState.OperationId, e);
                throw;
            }

            TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} - Exiting state machine", actionState.OperationId);
        }

        private static bool IsTerminalState(ActionStateBase actionState)
        {
            return actionState.StateProgress.Peek() == StepStateNames.IntentSaved ||
                actionState.StateProgress.Peek() == StepStateNames.CompletedSuccessfully ||
                actionState.StateProgress.Peek() == StepStateNames.Failed;
        }

        // Translate certain retry errors to E_ABORT
        private static int TranslateRollbackError(int errorCausingRollback)
        {
            if (errorCausingRollback == unchecked((int)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_READY))
            {
                return unchecked((int)NativeTypes.FABRIC_ERROR_CODE.E_ABORT);
            }

            // Return the original error code
            return errorCausingRollback;
        }

        private async Task RunStateMachineAsync(
            FabricClient fabricClient,
            FabricTestAction action,
            ActionStateBase actionState,
            ServiceInternalFaultInfo serviceInternalFaultInfo,
            CancellationToken cancellationToken)
        {
            if (actionState.StateProgress == null || actionState.StateProgress.Count == 0)
            {
                ReleaseAssert.AssertIf(actionState.StateProgress == null || actionState.StateProgress.Count == 0, "ActionProgress should not be null or empty");
            }

            Exception actionError = null;
            if (actionState.RollbackState == RollbackState.NotRollingBack || 
                (actionState.RollbackState != RollbackState.RollingBackForce && actionState.RetryStepWithoutRollingBackOnFailure))
            {
                // TODO: should also include Error
                while (actionState.StateProgress.Peek() != StepStateNames.CompletedSuccessfully) 
                {
                    cancellationToken.ThrowIfCancellationRequested();

                    RollbackState readRollbackState = await this.CheckUserCancellationAndUpdateIfNeededAsync(actionState, cancellationToken, FASConstants.ForwardLoop).ConfigureAwait(false);
                    if ((readRollbackState == RollbackState.RollingBackForce) ||
                        ((readRollbackState == RollbackState.RollingBackDueToUserCancel) && !actionState.RetryStepWithoutRollingBackOnFailure))
                    {
                        break;
                    }

                    try
                    {
                        await this.RunStepAsync(fabricClient, action, actionState, cancellationToken, serviceInternalFaultInfo).ConfigureAwait(false);

                        ActionTest.PerformInternalServiceFaultIfRequested(actionState.OperationId, serviceInternalFaultInfo, actionState, cancellationToken);
                        if (actionState.StateProgress.Peek() == StepStateNames.CompletedSuccessfully)
                        {
                            TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} - completed successfully, clearing ErrorCausingRollback", actionState.OperationId);
                            actionState.ErrorCausingRollback = 0;
                        }

                        actionState.TimeStopped = DateTime.UtcNow;
                        await this.actionStore.UpdateActionStateAsync(actionState).ConfigureAwait(false);
                    }
                    catch (RetrySameStepException)
                    {
                        // Retry the command in the same step - do not rollback or go forward, and do not call ActionStore.UpdateActionStateAsync().
                        TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - threw RetrySameStepException, retrying state {1} ", actionState.StateProgress.Peek());
                    }
                    catch (FabricNotPrimaryException)
                    {
                        throw;
                    }
                    catch (FabricObjectClosedException)
                    {
                        throw;
                    }
                    catch (Exception e)
                    {
                        TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - RunStateMachineAsync caught: {1}", actionState.OperationId, e.ToString());
                        readRollbackState = this.CheckUserCancellationAndUpdateIfNeededAsync(actionState, cancellationToken, FASConstants.ForwardLoopExceptionBlock).GetAwaiter().GetResult();

                        // 1st line: if this is a force rollback (RollingBackForce), just exit
                        // 2nd line: if !RetryStepWithoutRollingBackOnFailure and there was a graceful cancel then exit this block and proceed to the rollback code block below.
                        // If RetryStepWithoutRollingBackOnFailure is true, which it is only for the node steps today,  then first call HandleRollback to translate the exception.
                        if ((readRollbackState == RollbackState.RollingBackForce)
                            || ((readRollbackState == RollbackState.RollingBackDueToUserCancel) && !actionState.RetryStepWithoutRollingBackOnFailure))
                        {
                            break;
                        }
                        else
                        {
                            bool isRetryable = this.HandleRollback(actionState.OperationId, e);
                            if (isRetryable)
                            {
                                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - observed retryable exception, will retry action.  Exception: {1}", actionState.OperationId, e.ToString());
                                actionState.RollbackState = RollbackState.RollingBackAndWillRetryAction;
                            }
                            else
                            {
                                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - observed non-retryable exception.  Exception: {1}", actionState.OperationId, e.ToString());
                                actionState.RollbackState = RollbackState.RollingBackAndWillFailAction;
                            }
                        }

                        actionError = e;
                        break;
                    }
                }
            }

            if (actionState.RollbackState == RollbackState.RollingBackAndWillRetryAction
                || actionState.RollbackState == RollbackState.RollingBackAndWillFailAction
                || (actionState.RollbackState == RollbackState.RollingBackDueToUserCancel && (actionState.StateProgress.Peek() != StepStateNames.CompletedSuccessfully)))
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Rollingback type={1}", actionState.OperationId, actionState.ActionType);
                if (!this.isTestMode && actionState.StateProgress.Peek() == StepStateNames.CompletedSuccessfully)
                {
                    string error = string.Format(CultureInfo.InvariantCulture, "{0} - state should not be CompletedSuccessfully", actionState.OperationId);
                    TestabilityTrace.TraceSource.WriteError(TraceType, error);
                    ReleaseAssert.Failfast(error);
                }

                // If actionError is not null it means we are currently running a resumed rollback.  In that case the ErrorCausingRollback must have
                // already been set.                
                if (actionError != null)
                {
                    actionState.ErrorCausingRollback = TranslateRollbackError(actionError.HResult);
                    TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Translated ErrorCausingRollback ={1}", actionState.OperationId, actionState.ErrorCausingRollback);
                }

                if (this.isTestMode && actionState.StateProgress.Peek() == StepStateNames.CompletedSuccessfully)
                {
                    // In test mode it's intentionally possible to fault an action after it's completed its work, but before the state name has been updated.
                    actionState.StateProgress.Pop();
                }

                await this.actionStore.UpdateActionStateAsync(actionState).ConfigureAwait(false);

                try
                {
                    while (actionState.StateProgress.Peek() != StepStateNames.IntentSaved &&
                        actionState.StateProgress.Peek() != StepStateNames.Failed)
                    {
                        cancellationToken.ThrowIfCancellationRequested();
                        RollbackState readRollbackState = await this.CheckUserCancellationAndUpdateIfNeededAsync(actionState, cancellationToken, FASConstants.OuterCleanupLoop).ConfigureAwait(false);
                        if (readRollbackState == RollbackState.RollingBackDueToUserCancel)
                        {
                            // Do nothing, already rolling back - debug only
                            TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Read RollingBackDueToUserCancel in outer rollback loop", actionState.OperationId);
                        }
                        else if (readRollbackState == RollbackState.RollingBackForce)
                        {
                            TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Read RollingBackForce in outer rollback loop", actionState.OperationId);
                            break;
                        }

                        StepStateNames currentStateName = actionState.StateProgress.Peek();
                        TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} - DEBUG - Rollback path loop, current state {1}", actionState.OperationId, actionState.StateProgress.Peek());

                        try
                        {
                            await this.CleanupStepAsync(fabricClient, action, actionState, cancellationToken, serviceInternalFaultInfo).ConfigureAwait(false);

                            await this.actionStore.UpdateActionStateAsync(actionState).ConfigureAwait(false);
                        }
                        catch (FabricNotPrimaryException)
                        {
                            throw;
                        }
                        catch (FabricObjectClosedException)
                        {
                            throw;
                        }
                        catch (Exception e)
                        {
                            ReleaseAssert.Failfast("Unexpected exception, RunStateAsync for cleanup should have handled {0}", e);
                        }
                    }

                    // If this is true rollback is finished.  If it is retryable set the state to LookingUpState                    
                    if (actionState.StateProgress.Peek() == StepStateNames.IntentSaved)
                    {
                        if (actionState.RollbackState == RollbackState.RollingBackAndWillRetryAction)
                        {
                            actionState.StateProgress.Push(StepStateNames.LookingUpState);
                            actionState.ClearInfo();
                        }
                        else if (actionState.RollbackState == RollbackState.RollingBackAndWillFailAction)
                        {
                            actionState.StateProgress.Push(StepStateNames.Failed);

                            actionState.RollbackState = RollbackState.NotRollingBack;
                            actionState.TimeStopped = DateTime.UtcNow;
                        }
                        else if (actionState.RollbackState == RollbackState.RollingBackDueToUserCancel)
                        {
                            actionState.StateProgress.Push(StepStateNames.Failed);

                            actionState.TimeStopped = DateTime.UtcNow;
                        }
                        else if (actionState.RollbackState == RollbackState.RollingBackForce)
                        {
                            actionState.StateProgress.Push(StepStateNames.Failed);

                            actionState.TimeStopped = DateTime.UtcNow;
                        }
                        else
                        {
                            string error = string.Format(CultureInfo.InvariantCulture, "{0} - RollbackState == NotRollingBack not expected", actionState.OperationId);
                            ReleaseAssert.Failfast(error);
                        }
                    }
                    else if (actionState.RollbackState == RollbackState.RollingBackForce)
                    {
                        actionState.StateProgress.Push(StepStateNames.Failed);
                        actionState.TimeStopped = DateTime.UtcNow;
                    }
                }
                catch (OperationCanceledException)
                {
                    // This means the cancellation token is set, not that an api call observed an E_ABORT
                    throw;
                }
                catch (FabricNotPrimaryException)
                {
                    throw;
                }
                catch (FabricObjectClosedException)
                {
                    throw;
                }
                catch (Exception e)
                {
                    ReleaseAssert.Failfast("Unexpected exception, RunStateAsync for cleanup should have handled {0}", e);
                }

                TestabilityTrace.TraceSource.WriteInfo(
                    TraceType,
                    "{0} - Action failed, type='{1}', will retry={2}, RollbackState={3}",
                    actionState.OperationId,
                    actionState.ActionType,
                    actionState.RollbackState == RollbackState.RollingBackAndWillRetryAction ? "true" : "false",
                    actionState.RollbackState);

                await this.actionStore.UpdateActionStateAsync(actionState).ConfigureAwait(false);
            }            
            else if (actionState.StateProgress.Peek() == StepStateNames.CompletedSuccessfully)
            {
                // user cancelled, but action/command completed anyways before cancellation was checked.
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "DEBUG {0} - Action type '{1}' completed successfully, not updating again ", actionState.OperationId, actionState.ActionType);
            }
            else if ((actionState.StateProgress.Peek() == StepStateNames.IntentSaved) &&
                (actionState.RollbackState == RollbackState.RollingBackDueToUserCancel))
            {
                actionState.StateProgress.Push(StepStateNames.Failed);
                actionState.TimeStopped = DateTime.UtcNow;

                await this.actionStore.UpdateActionStateAsync(actionState).ConfigureAwait(false);
            }                       
            else if (actionState.RollbackState == RollbackState.RollingBackForce)
            {
                // Note: unlike the case above this does not have a state of IntentSaved as a requirement since a force rollback is an abort and does run the steps in reverse.
                // It is possible for the StateProgress to be CompletedSuccessfully here, since we want to exit as quickly as possible.  In that case, the block 2 blocks above handles it -
                // we do nothing extra, and the command finishes executing.  If the user calls an api for information on this command, we translate the state to ForceCancelled if state is a terminal state
                // and RollbackState is RollingBackForce.  See ActionStore.MatchesStateFilter().
                actionState.TimeStopped = DateTime.UtcNow;
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "Bottom of Engine.RunAsync() - state is={0}, rollbackState={1}", actionState.StateProgress.Peek().ToString(), actionState.RollbackState.ToString());
                actionState.StateProgress.Push(StepStateNames.Failed);
                await this.actionStore.UpdateActionStateAsync(actionState).ConfigureAwait(false);
            }
            else
            {
                string unexpectedError = string.Format(CultureInfo.InvariantCulture, "Unexpected case reached, state is={0}, rollbackState={1}", actionState.StateProgress.Peek().ToString(), actionState.RollbackState.ToString());
                TestabilityTrace.TraceSource.WriteError(TraceType, "{0}", unexpectedError);
                ReleaseAssert.Failfast(unexpectedError);
            }
        }

        private async Task RunStepAsync(
            FabricClient fabricClient,
            FabricTestAction action,
            ActionStateBase actionState,
            CancellationToken cancellationToken,
            ServiceInternalFaultInfo serviceInternalFaultInfo)
        {
            StepStateNames state = actionState.StateProgress.Peek();
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Running state={0}, name={1}, key={2}", state, actionState.ActionType, actionState.OperationId);

            StepBase actionUnit = null;

            actionUnit = action.GetStep(fabricClient, actionState, state, cancellationToken);

            TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} - Running {1}", actionState.OperationId, actionUnit.StepName);
            try
            {
                while (true)
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    RollbackState readRollbackState = await this.CheckUserCancellationAndUpdateIfNeededAsync(actionState, cancellationToken, FASConstants.InnerForwardLoop).ConfigureAwait(false);
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} - readRollbackState={1}", actionState.OperationId, readRollbackState);

                    // If RetryStepWithoutRollingbackOnFailure == true, then don't allow graceful user cancel
                    if (!actionState.RetryStepWithoutRollingBackOnFailure && (readRollbackState == RollbackState.RollingBackDueToUserCancel))
                    {
                        TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} - read RollingBackDueToUserCancel breaking from Run loop inside RunStepAsync()", actionState.OperationId);
                        break;
                    }

                    // RollingBackForce always stops execution
                    if (readRollbackState == RollbackState.RollingBackForce)
                    {
                        break;
                    }

                    Exception runException = null;
                    try
                    {
                        TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0}, {1} - calling Step.Run()", actionState.OperationId, actionState.ActionType);
                        ActionStateBase newContext = await actionUnit.RunAsync(cancellationToken, serviceInternalFaultInfo).ConfigureAwait(false);
                        TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0}, {1} - calling break after run", actionState.OperationId, actionState.ActionType);
                        break;
                    }
                    catch (Exception runExceptionTemp)
                    {
                        runException = runExceptionTemp;
                        TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0}, {1} - runException {2}", actionState.OperationId, actionState.ActionType, runException);
                        if (actionState.RetryStepWithoutRollingBackOnFailure)
                        {
                            // trace and loop.  Should have /backoff/?
                            TestabilityTrace.TraceSource.WriteWarning(
                                TraceType,
                                "{0}, {1} has RetryStepWithoutRollingbackOnFailure set to true, retrying step name='{2}'.  Caught exception: {3}",
                                actionState.OperationId,
                                actionState.ActionType,
                                actionUnit.StepName,
                                runException);

                            this.ProcessRetryStepExceptions(actionState.OperationId, runException);
                        }
                        else
                        {
                            throw;
                        }
                    }

                    if (runException != null)
                    {
                        await Task.Delay(TimeSpan.FromSeconds(this.commandStepRetryBackoffInSeconds), cancellationToken).ConfigureAwait(false);
                    }
                }
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - RunState, error: {1}", actionState.OperationId, e.ToString());
                throw;
            }
        }

        private void ProcessRetryStepExceptions(Guid operationId, Exception e)
        {
            TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - ProcessRetryStepExceptions, error: {1}", operationId, e.ToString());
            if (e is FatalException)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - ProcessRetryStepExceptions fatal exception, error: {1}", operationId, e.ToString());
                throw e;
            }
            else if (e is RollbackAndRetryCommandException)
            {
                // the question here is whether if by throwing this type of exception, it automatically looks transient to engine.handlerollback, or whether we can just throw this e.inner and assume that e.inner is rollback  retryaboe
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - ProcessRetryStepExceptions rolling back and retrying on exception: {1}", operationId, e.ToString());
                throw new FabricException(FabricErrorCode.NotReady);
            }
            else if (e is RetrySameStepException)
            {
                // just trace
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - ProcessRetryStepExceptions retrying same step on exception: {1}", operationId, e.ToString());                
            }
            else
            {
                // Trace for debugability.  This condition means the caller will retry the same step instead of rollingback.  Any action/command that has ActionStateBase.RetryStepWithoutRollingBackOnFailure 
                // set to true and which does not fit the first 2 conditions above will result in retrying the same step.
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - ProcessRetryStepExceptions deafult case - retrying same step on exception: {1}", operationId, e.ToString());
            }
        }

        private async Task CleanupStepAsync(
            FabricClient fabricClient,
            FabricTestAction action,
            ActionStateBase actionState,
            CancellationToken cancellationToken,
            ServiceInternalFaultInfo serviceInternalFaultInfo)
        {
            StepStateNames state = actionState.StateProgress.Peek();
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Cleaning up state={0}, name={1}, key={2}", state, actionState.ActionType, actionState.OperationId);

            StepBase actionUnit = null;

            actionUnit = action.GetStep(fabricClient, actionState, state, cancellationToken);

            TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} - Cleaning up {1}", actionState.OperationId, actionUnit.StepName);

            try
            {
                while (true)
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    RollbackState readRollbackState = await this.CheckUserCancellationAndUpdateIfNeededAsync(actionState, cancellationToken, FASConstants.InnerCleanupLoop).ConfigureAwait(false);
                    if (readRollbackState == RollbackState.RollingBackDueToUserCancel)
                    {
                        // Do nothing, already rolling back
                        TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Inner cleanup loop read RollingBackDueToUserCancel", actionState.OperationId);
                    }
                    else if (readRollbackState == RollbackState.RollingBackForce)
                    {
                        TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Inner cleanup loop read RollingBackForce", actionState.OperationId);
                        break;
                    }

                    try
                    {
                        await actionUnit.CleanupAsync(cancellationToken).ConfigureAwait(false);
                        actionState.StateProgress.Pop();
                        break;
                    }
                    catch (Exception cleanupException)
                    {
                        TestabilityTrace.TraceSource.WriteWarning(
                            TraceType,
                            "{0} - Cleanup of action type={1}, failed with {2}, retrying",
                            actionState.OperationId,
                            actionState.ActionType,
                            cleanupException);
                    }
                    
                    await Task.Delay(TimeSpan.FromSeconds(this.commandStepRetryBackoffInSeconds), cancellationToken).ConfigureAwait(false);
                }
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - CleanupStepAsync, error: {1}", actionState.OperationId, e.ToString());
                throw;
            }
        }

        private bool HandleRollback(Guid operationId, Exception exception)
        {
            return this.HandleRollback(operationId, exception, new List<FabricErrorCode>());
        }

        private bool HandleRollback(Guid operationId, Exception exception, List<FabricErrorCode> expectedErrorCodes)
        {
            TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Enter HandleRollback", operationId);
            if (exception == null)
            {
                string error = string.Format(CultureInfo.InvariantCulture, "{0} - exception is null", operationId);
                ReleaseAssert.Failfast(error);
            }

            if (exception is FatalException)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - HandleRollback - FatalException", operationId);
                return false;
            }

            if (exception is RollbackAndRetryCommandException)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - HandleRollback - RollbackAndRetryCommandException", operationId);
                return true;
            }

            // Use for internal testing
            if (exception is InvalidOperationException)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - HandleRollback - InvalidOperationException", operationId);
                return false;
            }

            if (exception is OperationCanceledException)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Retrying after OperationCanceledException", operationId);
                return true;
            }

            if (exception is FabricInvalidAddressException)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Retrying after {1}", operationId, exception);
                return true;
            }

            // Add to this list 
            if (exception is FabricException)
            {
                FabricException fabricException = exception as FabricException;
                if ((fabricException.ErrorCode == FabricErrorCode.InvalidReplicaStateForReplicaOperation) || // eg restart of down replica
                    (fabricException.ErrorCode == FabricErrorCode.InvalidAddress) ||
                    (fabricException.ErrorCode == FabricErrorCode.NotReady) ||
                    (fabricException.ErrorCode == FabricErrorCode.OperationTimedOut) ||
                    (fabricException.ErrorCode == FabricErrorCode.ReplicaDoesNotExist))
                {
                    TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Retrying after {1}", operationId, exception);
                    return true;
                }
            }

            FabricException innerException = exception as FabricException;
            if (innerException == null)
            {
                // malformed exception
                string error = string.Format(CultureInfo.InvariantCulture, "{0} - Exception type was not expected: '{1}'", operationId, exception);
                TestabilityTrace.TraceSource.WriteError(TraceType, "{0}", error);
                return false;                
            }

            if (innerException is FabricTransientException)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Retrying after inner ex {1}", operationId, exception);
                return true;
            }

            TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Bottom of HandleRollback, printing exception info", operationId);
            TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Exception: {1}", operationId, exception);
            TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Inner ex {1}", operationId, innerException);
            TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Inner ex error code {1}", operationId, innerException.ErrorCode);

            // nonretryable
            return false;
        }
    
        private Task<RollbackState> CheckUserCancellationAndUpdateIfNeededAsync(ActionStateBase actionState, CancellationToken cancellationToken, string location)
        {
            return FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                actionState.OperationId,
                () => this.CheckUserCancellationAndUpdateIfNeededInnerAsync(actionState, cancellationToken, location),
                this.partition,
                "ReliableFaultsEngine.CheckUserCancellationAndUpdateIfNeededAsync",
                3,
                cancellationToken);
        }

        private async Task<RollbackState> CheckUserCancellationAndUpdateIfNeededInnerAsync(ActionStateBase actionState, CancellationToken cancellationToken, string location)
        {
            TestabilityTrace.TraceSource.WriteNoise(TraceType, "CheckUserCancellationAndUpdateIfNeededAsync() {0} ENTER location='{1}'", actionState.OperationId, location);
            RollbackState rollbackState;

            rollbackState = await this.actionStore.CheckUserCancellationAndUpdateIfNeededAsync(actionState, cancellationToken, location).ConfigureAwait(false);
            if (rollbackState == RollbackState.RollingBackDueToUserCancel || rollbackState == RollbackState.RollingBackForce)
            {
                actionState.RollbackState = rollbackState;
            }
            else
            {
                // This is an internal test hook used in ActionTest.cs and TSInternal.test.  This condition will never be true outside of that.
                if (actionState.ServiceInternalFaultInfo != null)
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "CheckUserCancellationAndUpdateIfNeededAsync() {0}/{1} - test cancel case, checking...", actionState.OperationId, location);
                    rollbackState = ActionTest.PerformTestUserCancelIfRequested(actionState.OperationId, location, actionState.ServiceInternalFaultInfo, actionState);
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "CheckUserCancellationAndUpdateIfNeededAsync() {0}/{1} - returned rollbackState is {2}", actionState.OperationId, location, rollbackState, rollbackState);
                    actionState.RollbackState = rollbackState;
                }
            }

            return rollbackState;
        }
    }
}
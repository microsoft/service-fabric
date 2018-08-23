// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions.Steps
{
    using System.Fabric.FaultAnalysis.Service.Actions;
    using System.Fabric.FaultAnalysis.Service.Test;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using Common;
    using Fabric.Common;
    using Query;
    using TestabilityTrace = System.Fabric.Common.TestabilityTrace;

    internal static class StartNodeStepsFactory
    {
        public static StepBase GetStep(
            StepStateNames stateName,
            FabricClient fabricClient,
            ActionStateBase actionState,
            StartNodeFromFASAction action,
            TimeSpan requestTimeout,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken)
        {
            StepBase step = null;
            NodeCommandState startNodeState = Convert(actionState);

            StepStateNames prevContext = actionState.StateProgress.Peek();

            if (stateName == StepStateNames.LookingUpState)
            {
                step = new StartNodeStepsFactory.LookingUpState(fabricClient, startNodeState, action, requestTimeout, operationTimeout);
            }
            else if (stateName == StepStateNames.PerformingActions)
            {
                step = new StartNodeStepsFactory.PerformingActions(fabricClient, startNodeState, action, requestTimeout, operationTimeout);
            }
            else if (stateName == StepStateNames.CompletedSuccessfully)
            {
                // done - but then this method should not have been called               
                ReleaseAssert.Failfast(string.Format(CultureInfo.InvariantCulture, "{0} - GetStep() should not have been called when the state name is CompletedSuccessfully"), actionState.OperationId);
            }
            else
            {
                ReleaseAssert.Failfast(string.Format(CultureInfo.InvariantCulture, "{0} - Unexpected state name={1}", actionState.OperationId, stateName));
            }

            return step;
        }

        public static NodeCommandState Convert(ActionStateBase actionState)
        {
            NodeCommandState startNodeState = actionState as NodeCommandState;
            if (startNodeState == null)
            {
                throw new InvalidCastException("State object could not be converted");
            }

            return startNodeState;
        }

        internal class LookingUpState : StepBase
        {
            private readonly StartNodeFromFASAction action;

            public LookingUpState(FabricClient fabricClient, NodeCommandState state, StartNodeFromFASAction action, TimeSpan requestTimeout, TimeSpan operationTimeout)
                : base(fabricClient, state, requestTimeout, operationTimeout)
            {
                this.action = action;
            }

            public override StepStateNames StepName
            {
                get { return StepStateNames.LookingUpState; }
            }

            public override async Task<ActionStateBase> RunAsync(CancellationToken cancellationToken, ServiceInternalFaultInfo serviceInternalFaultInfo)
            {
                NodeCommandState state = Convert(this.State);

                Node queriedNode = await FaultAnalysisServiceUtility.GetNodeInfoAsync(
                    this.State.OperationId,
                    this.FabricClient,
                    state.Info.NodeName,
                    this.action.Partition,
                    this.action.StateManager,
                    this.action.StoppedNodeTable,
                    this.RequestTimeout,
                    this.OperationTimeout,
                    cancellationToken).ConfigureAwait(false);               

                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - StartNode LookingUpState reading RD", this.State.OperationId);
                bool isStopped = await FaultAnalysisServiceUtility.ReadStoppedNodeStateAsync(
                    this.State.OperationId,
                    this.action.Partition,
                    this.action.StateManager,
                    this.action.StoppedNodeTable,
                    state.Info.NodeName,
                    cancellationToken).ConfigureAwait(false);

                if (FaultAnalysisServiceUtility.IsNodeRunning(queriedNode))
                {
                    if (!isStopped)
                    {
                        // For illustration, if you just called StartNodeUsingNodeNameAsync() in this situation w/o checking first, you'd either get instance mismatch or node has not stopped yet
                        // Note: this is different than the logic in the PerformingActions step (the former does not check instance id, the latter does), which is after the call to StartNodeUsingNodeNameAsync(), because
                        // this is a precondition check.
                        Exception nodeAlreadyUp = FaultAnalysisServiceUtility.CreateException(
                            TraceType,
                            NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NODE_IS_UP,
                            string.Format(CultureInfo.InvariantCulture, "Node {0} already started", state.Info.NodeName),
                            FabricErrorCode.NodeIsUp);

                        throw new FatalException("fatal", nodeAlreadyUp);
                    }         
                    else
                    {
                        // The only way this can happen is OOB start.  FAS should fix it's incorrect state then fail the command with
                        // node already up.
                        TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - StartNode LookingUpState setting RD entry for node {1} to not stopped", this.State.OperationId, state.Info.NodeName);
                        await FaultAnalysisServiceUtility.SetStoppedNodeStateAsync(
                            this.action.State.OperationId,
                            this.action.Partition,
                            this.action.StateManager,
                            this.action.StoppedNodeTable,
                            queriedNode.NodeName,
                            false,
                            cancellationToken).ConfigureAwait(false);
                        Exception nodeIsUp = FaultAnalysisServiceUtility.CreateException(
                            TraceType,
                            Interop.NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NODE_IS_UP,
                            string.Format(CultureInfo.InvariantCulture, "Node {0} is up", state.Info.NodeName));
                        throw new FatalException("fatal", nodeIsUp);
                    }
                }                
                else if (queriedNode.NodeStatus == NodeStatus.Down && !isStopped)
                {
                    // This is a special scenario that can happen if:
                    // 1)  There was an OOB stop using the old api
                    // 2)  A node went down (not stopped, down)
                    // Don't handle this, return node down.                    
                    Exception nodeIsDown = FaultAnalysisServiceUtility.CreateException(
                        TraceType,
                        Interop.NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NODE_IS_DOWN,
                        string.Format(CultureInfo.InvariantCulture, "Node {0} is down", state.Info.NodeName));
                    throw new FatalException("fatal", nodeIsDown);
                }

                state.Info.InitialQueriedNodeStatus = queriedNode.NodeStatus;
                state.Info.NodeWasInitiallyInStoppedState = isStopped;

                state.StateProgress.Push(StepStateNames.PerformingActions);

                return state;
            }

            public override Task CleanupAsync(CancellationToken cancellationToken)
            {
                // debug - remove later
                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - Enter Cleanup for LookingupState", this.State.OperationId);
                return base.CleanupAsync(cancellationToken);
            }
        }

        internal class PerformingActions : StepBase
        {
            private readonly StartNodeFromFASAction action;

            public PerformingActions(FabricClient fabricClient, NodeCommandState state, StartNodeFromFASAction action, TimeSpan requestTimeout, TimeSpan operationTimeout)
                : base(fabricClient, state, requestTimeout, operationTimeout)
            {
                this.action = action;
            }

            public override StepStateNames StepName
            {
                get { return StepStateNames.PerformingActions; }
            }

            public override async Task<ActionStateBase> RunAsync(CancellationToken cancellationToken, ServiceInternalFaultInfo serviceInternalFaultInfo)
            {
                NodeCommandState state = Convert(this.State);

                // The return value is ignored, this is just being used to check if the RemoveNodeState was called.
                Node queriedNode = await FaultAnalysisServiceUtility.GetNodeInfoAsync(
                    this.State.OperationId,
                    this.FabricClient,
                    state.Info.NodeName,
                    this.action.Partition,
                    this.action.StateManager,
                    this.action.StoppedNodeTable,
                    this.RequestTimeout,
                    this.OperationTimeout,
                    cancellationToken).ConfigureAwait(false);

                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - calling StartNodeUsingNodeNameAsync, ApiInputNodeInstanceId={1}", this.State.OperationId, state.Info.InputNodeInstanceId);

                Exception exception = null;
                try
                {
                    await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => this.FabricClient.FaultManager.StartNodeUsingNodeNameAsync(
                            state.Info.NodeName,
                            state.Info.InputNodeInstanceId,
                            null,
                            0,
                            this.RequestTimeout,
                            cancellationToken),
                        FabricClientRetryErrors.StartNodeErrors.Value,
                        this.OperationTimeout,
                        cancellationToken).ConfigureAwait(false);
                }
                catch (Exception e)
                {
                    TestabilityTrace.TraceSource.WriteWarning(StepBase.TraceType, "{0} - StartNodeUsingNodeNameAsync threw {1}", this.State.OperationId, e);
                    exception = e;
                }

                cancellationToken.ThrowIfCancellationRequested();

                SuccessRetryOrFail status = SuccessRetryOrFail.Invalid;

                if (exception != null)
                {
                    FabricException fe = exception as FabricException;
                    if (fe != null)
                    {
                        status = this.HandleFabricException(fe, state);
                    }
                    else 
                    {
                        TestabilityTrace.TraceSource.WriteWarning(StepBase.TraceType, "{0} - StartNodeUsingNodeNameAsync threw non-FabricException with ErrorCode={1}", this.State.OperationId, exception);
                        status = SuccessRetryOrFail.RetryStep;
                    }
                }
                else
                {
                    // success
                    status = SuccessRetryOrFail.Success;

                    await FaultAnalysisServiceUtility.SetStoppedNodeStateAsync(
                        this.action.State.OperationId,
                        this.action.Partition,
                        this.action.StateManager,
                        this.action.StoppedNodeTable,
                        state.Info.NodeName,
                        false,
                        cancellationToken).ConfigureAwait(false);
                }

                ActionTest.PerformInternalServiceFaultIfRequested(this.State.OperationId, serviceInternalFaultInfo, this.State, cancellationToken, true);

                if (status == SuccessRetryOrFail.RetryStep)
                {
                    throw new RetrySameStepException("retrystep", exception);
                }
                else if (status == SuccessRetryOrFail.Fail)
                {
                    throw new FatalException("fatal", exception);
                }
                else if (status == SuccessRetryOrFail.Success)
                {
                    // no-op
                }
                else
                {
                    ReleaseAssert.Failfast(string.Format(CultureInfo.InvariantCulture, "This condition should not have been hit.  OperationId: {0}", this.State.OperationId));
                }

                await this.ValidateAsync(this.FabricClient, state, cancellationToken).ConfigureAwait(false);

                state.StateProgress.Push(StepStateNames.CompletedSuccessfully);
                return state;
            }

            private SuccessRetryOrFail HandleFabricException(FabricException fe, NodeCommandState state)
            {
                SuccessRetryOrFail status = SuccessRetryOrFail.Invalid;

                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - StartNodeUsingNodeNameAsync threw FabricException with ErrorCode={1}", this.State.OperationId, fe.ErrorCode);

                if (fe.ErrorCode == FabricErrorCode.InstanceIdMismatch)
                {
                    status = SuccessRetryOrFail.Fail;
                }
                else if (fe.ErrorCode == FabricErrorCode.NodeIsUp)
                {
                    status = SuccessRetryOrFail.Success;
                }
                else if (fe.ErrorCode == FabricErrorCode.NodeHasNotStoppedYet)
                {
                    status = SuccessRetryOrFail.RetryStep; 
                }
                else if (fe.ErrorCode == FabricErrorCode.InvalidAddress)
                {
                    if (state.Info.InitialQueriedNodeStatus != NodeStatus.Down && state.Info.NodeWasInitiallyInStoppedState == false)
                    {
                        // This is a (probably unlikely) case that may happen if 
                        // 1. The request reaches the target node, which is up and is processed.
                        // 2. The response is dropped
                        // 3. The request is retried with enough delay such that the node has now transitioned from up to stopped
                        // So, we say that if the initial conditions were valid and we get InvalidAddress, then consider it successful.
                        status = SuccessRetryOrFail.Success;
                    }                 
                    else
                    {
                        // The preconditions passed, but the node is down now, so something out of band happened.
                        status = SuccessRetryOrFail.RetryStep;
                    }
                }
                else if (fe.ErrorCode == FabricErrorCode.NodeNotFound)
                {
                    // Always fatal
                    string nodeNotFoundErrorMessage = string.Format(CultureInfo.InvariantCulture, "{0} - node {1} was not found", state.OperationId, state.Info.NodeName);
                    TestabilityTrace.TraceSource.WriteError(StepBase.TraceType, nodeNotFoundErrorMessage);
                    status = SuccessRetryOrFail.Fail;
                }
                else
                {
                    status = SuccessRetryOrFail.RetryStep;
                }

                return status;
            }

            private async Task ValidateAsync(FabricClient fc, NodeCommandState state, CancellationToken cancellationToken)
            {
                // It takes a few seconds for the node to shutdown + a few seconds for FM to find out.
                await Task.Delay(TimeSpan.FromSeconds(15), cancellationToken).ConfigureAwait(false);

                TimeoutHelper timeoutHelper = new TimeoutHelper(this.OperationTimeout);
                Node queriedNode = null;

                do
                {
                    TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - start node validating node '{1}' is not down", this.State.OperationId, state.Info.InputNodeInstanceId);
                    queriedNode = await FaultAnalysisServiceUtility.GetNodeInfoAsync(
                        this.State.OperationId,
                        fc,
                        state.Info.NodeName,
                        this.action.Partition,
                        this.action.StateManager,
                        this.action.StoppedNodeTable,
                        this.RequestTimeout,
                        this.OperationTimeout,
                        cancellationToken).ConfigureAwait(false);

                    if (FaultAnalysisServiceUtility.IsNodeRunning(queriedNode))
                    {
                        break;
                    }

                    await Task.Delay(TimeSpan.FromSeconds(5.0d), cancellationToken).ConfigureAwait(false);
                }
                while (timeoutHelper.GetRemainingTime() > TimeSpan.Zero);

                if (!FaultAnalysisServiceUtility.IsNodeRunning(queriedNode))
                {
                    // something is wrong, retry                    
                    FaultAnalysisServiceUtility.ThrowEngineRetryableException(string.Format(CultureInfo.InvariantCulture, "{0} - start node validation - node is not running yet.  Status={1}", state.OperationId, queriedNode.NodeStatus));
                }
            }
        }
    }
}
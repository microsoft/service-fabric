// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions.Steps
{
    using System.Fabric.FaultAnalysis.Service.Actions;
    using System.Fabric.FaultAnalysis.Service.Test;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using Common;
    using Fabric.Common;
    using Query;
    using TestabilityTrace = System.Fabric.Common.TestabilityTrace;

    internal static class StopNodeStepsFactory
    {
        public static StepBase GetStep(
            StepStateNames stateName,
            FabricClient fabricClient,
            ActionStateBase actionState,
            StopNodeFromFASAction action,
            TimeSpan requestTimeout,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken)
        {
            StepBase step = null;
            NodeCommandState state = Convert(actionState);

            StepStateNames prevContext = actionState.StateProgress.Peek();

            if (stateName == StepStateNames.LookingUpState)
            {
                step = new StopNodeStepsFactory.LookingUpState(fabricClient, state, action, requestTimeout, operationTimeout);
            }
            else if (stateName == StepStateNames.PerformingActions)
            {
                step = new StopNodeStepsFactory.PerformingActions(fabricClient, state, action, requestTimeout, operationTimeout);
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
            NodeCommandState stopNodeState = actionState as NodeCommandState;
            if (stopNodeState == null)
            {
                throw new InvalidCastException("State object could not be converted");
            }

            return stopNodeState;
        }

        internal class LookingUpState : StepBase
        {
            private readonly StopNodeFromFASAction action;

            public LookingUpState(FabricClient fabricClient, NodeCommandState state, StopNodeFromFASAction action, TimeSpan requestTimeout, TimeSpan operationTimeout)
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

                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - StopNode.LookingUpState performing node query", this.State.OperationId);

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
                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - StopNode.LookingUpState node query completed", this.State.OperationId);

                // Check for bad state
                if (queriedNode == null ||
                    queriedNode.NodeStatus == NodeStatus.Invalid ||
                    queriedNode.NodeStatus == NodeStatus.Unknown ||
                    queriedNode.NodeStatus == NodeStatus.Removed)
                {
                    // Fail the command                     
                    Exception nodeNotFoundException = FaultAnalysisServiceUtility.CreateException(
                        TraceType,
                        Interop.NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NODE_NOT_FOUND,
                        string.Format(CultureInfo.InvariantCulture, "{0} - Node {1} does not exist", this.State.OperationId, state.Info.NodeName));
                    TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - throwing fatal exception {1}", this.State.OperationId, nodeNotFoundException);
                    throw new FatalException("fatal", nodeNotFoundException);
                }                

                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - StopNode LookingUpState reading RD", this.State.OperationId);
                bool isStopped = await FaultAnalysisServiceUtility.ReadStoppedNodeStateAsync(
                    this.State.OperationId,
                    this.action.Partition,
                    this.action.StateManager,
                    this.action.StoppedNodeTable,
                    state.Info.NodeName,
                    cancellationToken).ConfigureAwait(false);

                if (queriedNode.NodeStatus == NodeStatus.Down && isStopped)
                {
                    // Node already stopped
                    Exception nodeAlreadyStopped = FaultAnalysisServiceUtility.CreateException(
                        TraceType,
                        Interop.NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_ALREADY_STOPPED,
                        string.Format(CultureInfo.InvariantCulture, "Node {0} is already stopped", state.Info.NodeName));
                    throw new FatalException("fatal", nodeAlreadyStopped);
                }
                else if (queriedNode.NodeStatus != NodeStatus.Down && isStopped)
                {
                    // FM says the node is up, so FAS has incorrect state, perhaps because of an out of band start from the original deprecated api.
                    // Correct the state, then continue to run this command normally.  It is valid.
                    await FaultAnalysisServiceUtility.SetStoppedNodeStateAsync(
                        this.action.State.OperationId,
                        this.action.Partition,
                        this.action.StateManager,
                        this.action.StoppedNodeTable,
                        queriedNode.NodeName,
                        false,
                        cancellationToken).ConfigureAwait(false);
                }
                else if (queriedNode.NodeStatus == NodeStatus.Down && !isStopped)
                {
                    // Node is down (as opposed to stopped)                    
                    Exception nodeIsDown = FaultAnalysisServiceUtility.CreateException(
                        TraceType,
                        Interop.NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NODE_IS_DOWN,
                        string.Format(CultureInfo.InvariantCulture, "Node {0} is down", state.Info.NodeName));
                    throw new FatalException("fatal", nodeIsDown);
                }

                state.Info.InitialQueriedNodeStatus = queriedNode.NodeStatus;
                state.Info.NodeWasInitiallyInStoppedState = isStopped;
                TestabilityTrace.TraceSource.WriteInfo(
                    StepBase.TraceType, 
                    "{0} - StopNode LookingUpState InitialQueriedNodeStatus='{1}', NodeWasInitiallyInStoppedState='{2}'", 
                    this.State.OperationId,
                    state.Info.InitialQueriedNodeStatus,
                    state.Info.NodeWasInitiallyInStoppedState);

                state.StateProgress.Push(StepStateNames.PerformingActions);
                return state;
            }

            public override Task CleanupAsync(CancellationToken cancellationToken)
            {
                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - Enter Cleanup for LookingupState", this.State.OperationId);                
                return Task.FromResult(true);
            }
        }

        internal class PerformingActions : StepBase
        {
            private readonly StopNodeFromFASAction action;

            public PerformingActions(FabricClient fabricClient, NodeCommandState state, StopNodeFromFASAction action, TimeSpan requestTimeout, TimeSpan operationTimeout)
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
                await FaultAnalysisServiceUtility.GetNodeInfoAsync(
                    this.State.OperationId,
                    this.FabricClient,
                    state.Info.NodeName,
                    this.action.Partition,
                    this.action.StateManager,
                    this.action.StoppedNodeTable,
                    this.RequestTimeout,
                    this.OperationTimeout,
                    cancellationToken).ConfigureAwait(false);

                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - calling StopNodeUsingNodeNameAsync, ApiInputNodeInstanceId={1}", this.State.OperationId, state.Info.InputNodeInstanceId);

                Exception exception = null;
                SuccessRetryOrFail status = SuccessRetryOrFail.Invalid;

                try
                {
                    TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} stopping with duration '{1}'", this.State.OperationId, state.Info.StopDurationInSeconds);

                    await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => this.FabricClient.FaultManager.StopNodeInternalAsync(
                            state.Info.NodeName,
                            state.Info.InputNodeInstanceId,
                            state.Info.StopDurationInSeconds,
                            this.RequestTimeout,
                            cancellationToken),
                        this.OperationTimeout,
                        cancellationToken).ConfigureAwait(false);
                }
                catch (Exception e)
                {
                    exception = e;
                }

                cancellationToken.ThrowIfCancellationRequested();

                if (exception != null)
                {
                    TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - StopNodeUsingNodeNameAsync api threw {1}", this.State.OperationId, exception);

                    FabricException fe = exception as FabricException;
                    if (fe != null)
                    {
                        status = this.HandleFabricException(fe, state);
                    }
                    else
                    {
                        // retry the step
                        TestabilityTrace.TraceSource.WriteWarning(StepBase.TraceType, "{0} - unexpected exception {1}, retrying step", this.State.OperationId, exception);
                        status = SuccessRetryOrFail.RetryStep;
                    }
                }
                else
                {
                    TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - StopNodeUsingNodeNameAsync api returned success", this.State.OperationId);

                    status = SuccessRetryOrFail.Success;

                    TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - setting state in stopped node table", this.State.OperationId);
                    await FaultAnalysisServiceUtility.SetStoppedNodeStateAsync(
                        this.action.State.OperationId,
                        this.action.Partition,
                        this.action.StateManager,
                        this.action.StoppedNodeTable,
                        state.Info.NodeName,
                        true, // stopped
                        cancellationToken).ConfigureAwait(false);
                }

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

                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - before calling PerformInternalServiceFaultIfRequested", this.State.OperationId);
                ActionTest.PerformInternalServiceFaultIfRequested(this.State.OperationId, serviceInternalFaultInfo, this.State, cancellationToken, true);

                await this.ValidateAsync(this.FabricClient, state, cancellationToken).ConfigureAwait(false);

                state.StateProgress.Push(StepStateNames.CompletedSuccessfully);
                return state;
            }

            private SuccessRetryOrFail HandleFabricException(FabricException fe, NodeCommandState state)
            {
                SuccessRetryOrFail status = SuccessRetryOrFail.Invalid;

                if (fe.ErrorCode == FabricErrorCode.InstanceIdMismatch)
                {
                    TestabilityTrace.TraceSource.WriteWarning(StepBase.TraceType, "{0} - StopNode api threw InstanceIdMismatch", this.State.OperationId);
                    status = SuccessRetryOrFail.Fail;
                }
                else if (fe.ErrorCode == FabricErrorCode.InvalidAddress)
                {
                    TestabilityTrace.TraceSource.WriteInfo(
                        StepBase.TraceType,
                        "{0} - StopNode HandleFabricException InitialQueriedNodeStatus='{1}', NodeWasInitiallyInStoppedState='{2}'",
                        this.State.OperationId,
                        state.Info.InitialQueriedNodeStatus,
                        state.Info.NodeWasInitiallyInStoppedState);

                    // If the request was valid (node was up), but the response was dropped, and then the request was retried, we might now be
                    // sending a stop to a node that is now stopped.  The request will return InvalidAddress, but this is really success.
                    if (state.Info.InitialQueriedNodeStatus != NodeStatus.Down && state.Info.NodeWasInitiallyInStoppedState == false)
                    {
                        TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - StopNode api threw InvalidAddress, considering stop command successful due to preconditions", this.State.OperationId);
                        status = SuccessRetryOrFail.Success;
                    }
                    else
                    {
                        // Invalid address is not expected here.  There may have been an out of band stop using the deprecated api.  We should retry until we get something else.
                        TestabilityTrace.TraceSource.WriteWarning(StepBase.TraceType, "{0} - StopNode api threw InvalidAddress", this.State.OperationId);
                        status = SuccessRetryOrFail.RetryStep;
                    }
                }
                else if (fe.ErrorCode == FabricErrorCode.NodeNotFound)
                {
                    status = SuccessRetryOrFail.Fail;
                }
                else
                {
                    TestabilityTrace.TraceSource.WriteWarning(StepBase.TraceType, "{0} - StopNode api threw {1}", this.State.OperationId, fe);
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
                    TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - stop node validating node '{1}' is not up", this.State.OperationId, state.Info.InputNodeInstanceId);
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

                    if (!FaultAnalysisServiceUtility.IsNodeRunning(queriedNode))
                    {
                        break;
                    }

                    await Task.Delay(TimeSpan.FromSeconds(5.0d), cancellationToken).ConfigureAwait(false);
                }
                while (timeoutHelper.GetRemainingTime() > TimeSpan.Zero);

                if (FaultAnalysisServiceUtility.IsNodeRunning(queriedNode))
                {
                    // Something is amiss.  The api returned success, but we're not reaching the desired state.  It might be something out of band happened.  This is best effort, so don't fail.
                    TestabilityTrace.TraceSource.WriteWarning(StepBase.TraceType, "{0} - node '{1}' did not reach desired state in {2}", this.State.OperationId, state.Info.InputNodeInstanceId, this.OperationTimeout);
                }
            }
        }
    }
}
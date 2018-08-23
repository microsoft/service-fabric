// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Engine
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.FaultAnalysis.Service.Actions;
    using System.Fabric.FaultAnalysis.Service.Actions.Steps;
    using System.Fabric.FaultAnalysis.Service.Common;
    using System.Fabric.FaultAnalysis.Service.Test;
    using System.Linq;
    using System.Numerics;
    using System.Threading;
    using System.Threading.Tasks;
    using Description;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Collections;
    using Query;
    using FabricTestAction = System.Fabric.FaultAnalysis.Service.Actions.FabricTestAction;
    using InvokeDataLossAction = System.Fabric.FaultAnalysis.Service.Actions.InvokeDataLossAction;
    using InvokeQuorumLossAction = System.Fabric.FaultAnalysis.Service.Actions.InvokeQuorumLossAction;
    using RestartPartitionAction = System.Fabric.FaultAnalysis.Service.Actions.RestartPartitionAction;
    using TestabilityTrace = System.Fabric.Common.TestabilityTrace;

    // The class that handles client commands - hook this up to the real client interface.
    internal class FaultAnalysisServiceMessageProcessor
    {
        private const string TraceType = "MessageProcessor";
        private readonly FabricClient fabricClient;
        private readonly ReliableFaultsEngine engine;
        private readonly ActionStore actionStore;
        private readonly IReliableStateManager stateManager;
        private readonly IReliableDictionary<string, bool> stoppedNodeTable;
        private readonly TimeSpan requestTimeout;
        private readonly TimeSpan operationTimeout;
        private readonly BlockingCollection<ActionStateBase> queue;
        private readonly int concurrentRequests;
        private readonly ConcurrentDictionary<Guid, ActionCompletionInfo> pendingTasks;
        private readonly EntitySynchronizer entitySynch;
        private readonly int dataLossCheckWaitDurationInSeconds;
        private readonly int dataLossCheckPollIntervalInSeconds;
        private readonly int replicaDropWaitDurationInSeconds;

        private Task consumerTask;
        private Task pendingTasksTask;
        private int isShuttingDown;

        // In the end, I'd like the first arg to be a fabricClient producer as opposed to a raw client
        public FaultAnalysisServiceMessageProcessor(
            IStatefulServicePartition partition,
            FabricClient fabricClient,
            ReliableFaultsEngine engine,
            ActionStore actionStore,
            IReliableStateManager stateManager,
            IReliableDictionary<string, bool> stoppedNodeTable,
            TimeSpan requestTimeout,
            TimeSpan operationTimeout,
            int concurrentRequests,
            int dataLossCheckWaitDurationInSeconds,
            int dataLossCheckPollIntervalInSeconds,
            int replicaDropWaitDurationInSeconds,
            CancellationToken cancellationToken)
        {
            ThrowIf.Null(partition, "partition");
            ThrowIf.Null(fabricClient, "fabricClient");
            ThrowIf.Null(engine, "engine");
            ThrowIf.Null(actionStore, "actionStore");
            ThrowIf.Null(stateManager, "stateManager");
            ThrowIf.Null(stoppedNodeTable, "stoppedNodeTable");

            this.Partition = partition;
            this.fabricClient = fabricClient;
            this.engine = engine;
            this.actionStore = actionStore;
            this.stoppedNodeTable = stoppedNodeTable;
            this.stateManager = stateManager;

            this.requestTimeout = requestTimeout;
            this.operationTimeout = operationTimeout;

            this.entitySynch = new EntitySynchronizer();

            this.queue = new BlockingCollection<ActionStateBase>(new ConcurrentQueue<ActionStateBase>());
            this.pendingTasks = new ConcurrentDictionary<Guid, ActionCompletionInfo>();
            
            this.concurrentRequests = concurrentRequests;
            this.dataLossCheckWaitDurationInSeconds = dataLossCheckWaitDurationInSeconds;
            this.dataLossCheckPollIntervalInSeconds = dataLossCheckPollIntervalInSeconds;
            this.replicaDropWaitDurationInSeconds = replicaDropWaitDurationInSeconds;

            this.consumerTask = Task.Run(() => this.ConsumeAndRunActionsAsync(cancellationToken), cancellationToken);
            this.pendingTasksTask = Task.Run(() => this.ProcessCompletedActionsAsync(cancellationToken), cancellationToken);
        }

        public IStatefulServicePartition Partition
        {
            get;
            private set;
        }

        public bool IsReady
        {
            get;
            private set;
        }

        public async Task ResumePendingActionsAsync(FabricClient fc, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Getting running actions");
            IEnumerable<ActionStateBase> incompleteActions = await this.actionStore.GetRunningActionsAsync();
            IEnumerable<ActionStateBase> two = incompleteActions.OrderBy(s => s.TimeReceived);
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Done getting running actions");

            foreach (ActionStateBase actionState in two)
            {
                if (actionState is NodeCommandState)
                {
                    NodeCommandState nodeState = actionState as NodeCommandState;

                    nodeState.NodeSync = this.entitySynch.NodeSynchronizer;
                    nodeState.StoppedNodeTable = this.stoppedNodeTable;
                    this.entitySynch.NodeSynchronizer.Add(nodeState.Info.NodeName);
                }

                FabricTestAction action = await this.ConstructActionAsync(actionState.ActionType, actionState);
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} - Resuming action of type {1}", actionState.OperationId, actionState.ActionType);
                this.Enqueue(actionState);
            }
        }

        // Use this method signature for now until the actual client interface is decided
        public async Task ProcessDataLossCommandAsync(Guid operationId, PartitionSelector partitionSelector, DataLossMode dataLossMode, TimeSpan timeout, ServiceInternalFaultInfo serviceInternalFaultInfo)
        {
            ThrowIfDataLossModeInvalid(dataLossMode);

            ActionStateBase actionState = new InvokeDataLossState(operationId, serviceInternalFaultInfo, partitionSelector, dataLossMode);

            try
            {
                // After this call finishes the intent has been persisted
                await this.actionStore.InitializeNewActionAsync(actionState, timeout);

                this.Enqueue(actionState);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Exception {1}", operationId, e);
                throw;
            }
        }

        // Test only, only accessible from "internal test", not from fabric client
        public async Task ProcessStuckCommandAsync(Guid operationId, ServiceInternalFaultInfo serviceInternalFaultInfo)
        {
            ActionStateBase actionState = new StuckState(operationId, serviceInternalFaultInfo);

            try
            {
                await this.actionStore.InitializeNewActionAsync(actionState, FASConstants.DefaultTestTimeout);

                this.Enqueue(actionState);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Exception {1}", operationId, e);
                throw;
            }
        }

        // Test only, only accessible from "internal test", not from fabric client
        public async Task ProcessRetryStepCommandAsync(Guid operationId, ServiceInternalFaultInfo serviceInternalFaultInfo)
        {
            ActionStateBase actionState = new TestRetryStepState(operationId, serviceInternalFaultInfo);

            try
            {
                await this.actionStore.InitializeNewActionAsync(actionState, FASConstants.DefaultTestTimeout);

                this.Enqueue(actionState);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Exception {1}", operationId, e);
                throw;
            }
        }

        public Task<ActionStateBase> ProcessGetProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return FaultAnalysisServiceUtility.RetryOnTimeout(
                operationId,
                () => this.actionStore.FindByOperationIdAsync(operationId, cancellationToken),
                timeout,
                cancellationToken);
        }

        // Use this method signature for now until the actual client interface is decided
        public async Task ProcessQuorumLossCommandAsync(Guid operationId, PartitionSelector partitionSelector, QuorumLossMode quorumLossMode, TimeSpan quorumLossDuration, TimeSpan timeout, ServiceInternalFaultInfo serviceInternalFaultInfo)
        {
            ThrowIfQuorumLossModeInvalid(quorumLossMode);
                        
            InvokeQuorumLossState actionState = new InvokeQuorumLossState(operationId, serviceInternalFaultInfo, partitionSelector, quorumLossMode, quorumLossDuration);

            try
            {
                // After this call finishes the intent has been persisted
                await this.actionStore.InitializeNewActionAsync(actionState, timeout);

                this.Enqueue(actionState);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Exception {1}", operationId, e);
                throw;
            }
        }

        // Use this method signature for now until the actual client interface is decided
        public async Task ProcessRestartPartitionCommandAsync(Guid operationId, PartitionSelector partitionSelector, RestartPartitionMode restartPartitionMode, TimeSpan timeout, ServiceInternalFaultInfo serviceInternalFaultInfo)
        {
            ThrowIfRestartPartitionModeInvalid(restartPartitionMode);

            RestartPartitionState actionState = new RestartPartitionState(operationId, serviceInternalFaultInfo, partitionSelector, restartPartitionMode);

            try
            {
                // After this call finishes the intent has been persisted
                await this.actionStore.InitializeNewActionAsync(actionState, timeout);

                this.Enqueue(actionState);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Exception {1}", operationId, e);
                throw;
            }
        }

        public async Task<TestCommandQueryResult> ProcessGetTestCommandListAsync(TestCommandListDescription description)
        {
            // 5728192
            const int MaxNumberOfItems = 10000;
            IEnumerable<ActionStateBase> result = await this.actionStore.GetSelectedActionsAsync(description);

            List<TestCommandStatus> list = new List<TestCommandStatus>();

            int end = Math.Min(result.Count(), MaxNumberOfItems);
            for (int i = 0; i < end; i++)
            {
                TestCommandProgressState state = FaultAnalysisServiceUtility.ConvertState(result.ElementAt(i), TraceType);
                TestCommandType type = ConvertType(result.ElementAt(i));

                list.Add(new TestCommandStatus(result.ElementAt(i).OperationId, state, type));
            }

            TestCommandQueryResult testCommandQueryResult = new TestCommandQueryResult(list);
            return testCommandQueryResult;
        }

        public Task<string> ProcessGetStoppedNodeListAsync()
        {
            return this.actionStore.GetStoppedNodeListAsync();
        }

        public async Task ProcessStartNodeCommandAsync(Guid operationId, string nodeName, BigInteger nodeInstanceId, TimeSpan timeout, ServiceInternalFaultInfo serviceInternalFaultInfo)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} - ProcessStartNodeCommandAsync", operationId);
            this.entitySynch.NodeSynchronizer.Add(nodeName);

            NodeCommandState actionState = new NodeCommandState(ActionType.StartNode, operationId, this.entitySynch.NodeSynchronizer, serviceInternalFaultInfo, nodeName, nodeInstanceId, 0);

            try
            {
                // After this call finishes the intent has been persisted
                await this.actionStore.InitializeNewActionAsync(actionState, timeout);
                this.Enqueue(actionState);
            }
            catch (Exception e)
            {
                this.entitySynch.NodeSynchronizer.Remove(nodeName);
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Exception {1}", operationId, e);
                throw;
            }
        }

        public async Task ProcessStopNodeCommandAsync(Guid operationId, string nodeName, BigInteger nodeInstanceId, int durationInSeconds, TimeSpan timeout, ServiceInternalFaultInfo serviceInternalFaultInfo)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} - ProcessStopNodeCommandAsync, duration is {1}", operationId, durationInSeconds);
            this.entitySynch.NodeSynchronizer.Add(nodeName);
            
            NodeCommandState actionState = new NodeCommandState(ActionType.StopNode, operationId, this.entitySynch.NodeSynchronizer, serviceInternalFaultInfo, nodeName, nodeInstanceId, durationInSeconds);

            try
            {
                // After this call finishes the intent has been persisted
                await this.actionStore.InitializeNewActionAsync(actionState, timeout);
                this.Enqueue(actionState);
            }
            catch (Exception e)
            {
                this.entitySynch.NodeSynchronizer.Remove(nodeName);
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Exception {1}", operationId, e);
                throw;
            }
        }

        public Task CancelTestCommandAsync(Guid operationId, bool force)
        {
            return this.actionStore.CancelTestCommandAsync(operationId, force);
        }

        private static void CheckForInvalidStatus(ConcurrentDictionary<Guid, ActionCompletionInfo> pendingTasks)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter CheckForInvalidStatus");

            // This shouldn't happen, but check for it occasionally
            IEnumerable<KeyValuePair<Guid, ActionCompletionInfo>> invalidTasks = pendingTasks.Where(p => p.Value.Task.Status == TaskStatus.Canceled || p.Value.Task.Status == TaskStatus.Faulted);
            foreach (KeyValuePair<Guid, ActionCompletionInfo> it in invalidTasks)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "Key '{0}' has unexpected task status '{1}'", it.Key, it.Value.Task.Status);
            }

            if (invalidTasks.Count() != 0)
            {
                ReleaseAssert.Failfast("Invalid task statuses, see errors above");
            }

            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Exit CheckForInvalidStatus");
        }

        private static TestCommandType ConvertType(ActionStateBase action)
        {
            ActionType type = action.ActionType;
            switch (type)
            {
                case ActionType.InvokeDataLoss:
                    return TestCommandType.PartitionDataLoss;
                case ActionType.InvokeQuorumLoss:
                    return TestCommandType.PartitionQuorumLoss;
                case ActionType.RestartPartition:
                    return TestCommandType.PartitionRestart;
                case ActionType.StartNode:
                    return TestCommandType.NodeTransition;
                case ActionType.StopNode:
                    return TestCommandType.NodeTransition;
                default:
                    return TestCommandType.Invalid;
            }
        }

        private static void ThrowIfDataLossModeInvalid(DataLossMode dataLossMode)
        {
            if (dataLossMode == DataLossMode.Invalid)
            {
                throw FaultAnalysisServiceUtility.CreateException(TraceType, Interop.NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG, Strings.StringResources.Error_UnsupportedDataLossMode);
            }
        }

        private static void ThrowIfQuorumLossModeInvalid(QuorumLossMode quorumLossMode)
        {
            if (quorumLossMode == QuorumLossMode.Invalid)
            {
                throw FaultAnalysisServiceUtility.CreateException(TraceType, Interop.NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG, Strings.StringResources.Error_UnsupportedQuorumLossMode);
            }
        }

        private static void ThrowIfRestartPartitionModeInvalid(RestartPartitionMode restartPartitionMode)
        {
            if (restartPartitionMode == RestartPartitionMode.Invalid)
            {
                throw FaultAnalysisServiceUtility.CreateException(TraceType, Interop.NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG, Strings.StringResources.Error_UnsupportedRestartPartitionMode);
            }
        }

        private void Enqueue(ActionStateBase actionState)
        {
            try
            {
                this.queue.Add(actionState);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "Error while enqueuing request={0}, error={1}", actionState.OperationId, e.ToString());
                throw;
            }
        }

        // Launched as a task
        private async Task ConsumeAndRunActionsAsync(CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter ConsumeAndRunActions");

            using (SemaphoreSlim semaphore = new SemaphoreSlim(this.concurrentRequests, this.concurrentRequests))
            {
                while (!cancellationToken.IsCancellationRequested)
                {
                    try
                    {
                        TestabilityTrace.TraceSource.WriteNoise(TraceType, "DEBUG Waiting for semaphore, max count={0}", this.concurrentRequests);
                        await semaphore.WaitAsync(cancellationToken).ConfigureAwait(false);
                        TestabilityTrace.TraceSource.WriteNoise(TraceType, "DEBUG Done waiting for semaphore");
                    }
                    catch (OperationCanceledException e)
                    {
                        TestabilityTrace.TraceSource.WriteWarning(TraceType, e.ToString());
                        break;
                    }

                    ActionStateBase actionState = null;
                    bool wasSuccessful = false;
                    try
                    {
                        TestabilityTrace.TraceSource.WriteNoise(TraceType, "Trying to Dequeue");

                        wasSuccessful = this.queue.TryTake(out actionState);
                        if (!wasSuccessful)
                        {
                            TestabilityTrace.TraceSource.WriteNoise(TraceType, "Queue was empty");
                            semaphore.Release();
                            await Task.Delay(TimeSpan.FromSeconds(1), cancellationToken);
                            continue;
                        }

                        TestabilityTrace.TraceSource.WriteNoise(TraceType, "Dequeued Name={0} Key={1}", actionState.ActionType, actionState.OperationId);
                    }
                    catch (OperationCanceledException e)
                    {
                        TestabilityTrace.TraceSource.WriteWarning(TraceType, e.ToString());
                        break;
                    }
                    catch (Exception ex)
                    {
                        TestabilityTrace.TraceSource.WriteWarning(TraceType, "TryDequeue failed with={0}", ex.ToString());
                        semaphore.Release();
                        continue;
                    }

                    Task t = null;
                    try
                    {
                        System.Fabric.FaultAnalysis.Service.Actions.FabricTestAction action = await this.ConstructActionAsync(actionState.ActionType, actionState);
                        t = this.engine.RunAsync(this.fabricClient, action, actionState, actionState.ServiceInternalFaultInfo, cancellationToken);
                    }
                    catch (FabricNotPrimaryException notPrimary)
                    {
                        FaultAnalysisServiceUtility.TraceFabricNotPrimary(actionState.OperationId, notPrimary);
                        break;
                    }
                    catch (FabricObjectClosedException objectClosed)
                    {
                        FaultAnalysisServiceUtility.TraceFabricObjectClosed(actionState.OperationId, objectClosed);
                        break;
                    }

                    if (!this.pendingTasks.TryAdd(actionState.OperationId, new ActionCompletionInfo(actionState.OperationId, t, semaphore)))
                    {
                        TestabilityTrace.TraceSource.WriteError(TraceType, "Add of key={0} failed, was it already added?", actionState.OperationId);
                        ReleaseAssert.Failfast("Add of key={0} failed, was it already added?", actionState.OperationId);
                    }
                    else
                    {
                        TestabilityTrace.TraceSource.WriteInfo(TraceType, "Action type={0}, key={1} started", actionState.ActionType, actionState.OperationId);
                    }

                    Interlocked.Increment(ref this.isShuttingDown);
                }

                TestabilityTrace.TraceSource.WriteWarning(TraceType, "Cancellation was requested");

                return;
            }
        }

        private async Task ProcessCompletedActionsAsync(CancellationToken cancellationToken)
        {
            // Look for completed tasks and remove them and move into the historical table.  If the task failed, mark it            
            while (!cancellationToken.IsCancellationRequested)
            {
                try
                {
                    while (this.pendingTasks.Count != 0)
                    {
                        cancellationToken.ThrowIfCancellationRequested();

                        // find the things in terminal finished state, process accordingly, call release on semaphore
                        KeyValuePair<Guid, ActionCompletionInfo> kv = this.pendingTasks.Where(p => p.Value.Task.Status == TaskStatus.RanToCompletion).FirstOrDefault();
                        if (!kv.Equals(default(KeyValuePair<Guid, ActionCompletionInfo>)))
                        {
                            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Fetched a completed action key={0}", kv.Key);
                            ActionCompletionInfo info;
                            if (!this.pendingTasks.TryRemove(kv.Key, out info))
                            {
                                // This should never happen?
                                TestabilityTrace.TraceSource.WriteError(TraceType, "Removal of key={0} failed", kv.Key);
                                ReleaseAssert.Failfast("Removal of key={0} failed", kv.Key);
                            }
                            else
                            {
                                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Key={0} removed successfully", kv.Key);

                                try
                                {
                                    info.Semaphore.Release();
                                }
                                catch (ObjectDisposedException ode)
                                {
                                    if ((ode.Message == "The semaphore has been disposed.") && (this.isShuttingDown > 0))
                                    {
                                        break;
                                    }

                                    throw;
                                }
                            }
                        }
                        else
                        {
                            TestabilityTrace.TraceSource.WriteNoise(TraceType, "Nothing to remove yet");
                        }
                    }

                    await Task.Delay(TimeSpan.FromSeconds(1), cancellationToken).ConfigureAwait(false);
                    FaultAnalysisServiceMessageProcessor.CheckForInvalidStatus(this.pendingTasks);
                }
                catch (OperationCanceledException)
                {
                    throw;
                }
                catch (Exception e)
                {
                    // This shouldn't be reached
                    TestabilityTrace.TraceSource.WriteError(TraceType, "ProcessCompletedActionsAsync caught {0}", e.ToString());
                    ReleaseAssert.Failfast("ProcessCompletedActionsAsync caught {0}", e.ToString());
                }
            }
        }

        private async Task<FabricTestAction> ConstructActionAsync(ActionType actionType, ActionStateBase actionStateBase)
        {
            FabricTestAction action = null;
            if (actionType == ActionType.InvokeDataLoss)
            {
                InvokeDataLossState actionState = actionStateBase as InvokeDataLossState;

                StepStateNames currentState = actionState.StateProgress.Peek();
                if (currentState == StepStateNames.IntentSaved)
                {
                    actionState.StateProgress.Push(StepStateNames.LookingUpState);
                    await this.actionStore.UpdateActionStateAsync(actionState);
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "action state has been updated");
                }

                action = new InvokeDataLossAction(
                    this.stateManager, 
                    this.Partition, 
                    actionState, 
                    actionState.Info.PartitionSelector, 
                    actionState.Info.DataLossMode, 
                    this.dataLossCheckWaitDurationInSeconds,
                    this.dataLossCheckPollIntervalInSeconds,
                    this.replicaDropWaitDurationInSeconds,
                    this.requestTimeout, 
                    this.operationTimeout);
            }
            else if (actionType == ActionType.InvokeQuorumLoss)
            {
                InvokeQuorumLossState actionState = actionStateBase as InvokeQuorumLossState;

                StepStateNames currentState = actionState.StateProgress.Peek();
                if (currentState == StepStateNames.IntentSaved)
                {
                    actionState.StateProgress.Push(StepStateNames.LookingUpState);
                    await this.actionStore.UpdateActionStateAsync(actionState);
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "action state has been updated");
                }

                // This is the case for resuming an action after a failover
                action = new InvokeQuorumLossAction(this.stateManager, this.Partition, actionState, actionState.Info.PartitionSelector, actionState.Info.QuorumLossMode, actionState.Info.QuorumLossDuration, this.requestTimeout, this.operationTimeout);
            }
            else if (actionType == ActionType.RestartPartition)
            {
                RestartPartitionState actionState = actionStateBase as RestartPartitionState;

                StepStateNames currentState = actionState.StateProgress.Peek();
                if (currentState == StepStateNames.IntentSaved)
                {
                    actionState.StateProgress.Push(StepStateNames.LookingUpState);
                    await this.actionStore.UpdateActionStateAsync(actionState);
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "action state has been updated");
                }

                // This is the case for resuming an action after a failover
                action = new RestartPartitionAction(this.stateManager, this.Partition, actionState, actionState.Info.PartitionSelector, actionState.Info.RestartPartitionMode, this.requestTimeout, this.operationTimeout);
            }
            else if (actionType == ActionType.TestStuck)
            {
                StuckState actionState = actionStateBase as StuckState;

                StepStateNames currentState = actionState.StateProgress.Peek();
                if (currentState == StepStateNames.IntentSaved)
                {
                    actionState.StateProgress.Push(StepStateNames.LookingUpState);
                    await this.actionStore.UpdateActionStateAsync(actionState);
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "action state has been updated");
                }

                action = new StuckAction(this.stateManager, this.Partition, actionState, this.requestTimeout, this.operationTimeout);
            }
            else if (actionType == ActionType.TestRetryStep)
            {
                TestRetryStepState actionState = actionStateBase as TestRetryStepState;

                StepStateNames currentState = actionState.StateProgress.Peek();
                if (currentState == StepStateNames.IntentSaved)
                {
                    actionState.StateProgress.Push(StepStateNames.LookingUpState);
                    await this.actionStore.UpdateActionStateAsync(actionState);
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "action state has been updated");
                }

                action = new TestRetryStepAction(this.stateManager, this.Partition, actionState, this.requestTimeout, this.operationTimeout);
            }
            else if (actionType == ActionType.StartNode)
            {
                NodeCommandState actionState = actionStateBase as NodeCommandState;
                actionState.StoppedNodeTable = this.stoppedNodeTable;

                StepStateNames currentState = actionState.StateProgress.Peek();
                if (currentState == StepStateNames.IntentSaved)
                {
                    actionState.StateProgress.Push(StepStateNames.LookingUpState);
                    await this.actionStore.UpdateActionStateAsync(actionState);
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "action state has been updated");
                }

                action = new StartNodeFromFASAction(this.stateManager, this.Partition, actionState, this.stoppedNodeTable, this.requestTimeout, this.operationTimeout);
            }
            else if (actionType == ActionType.StopNode)
            {
                NodeCommandState actionState = actionStateBase as NodeCommandState;
                actionState.StoppedNodeTable = this.stoppedNodeTable;

                StepStateNames currentState = actionState.StateProgress.Peek();
                if (currentState == StepStateNames.IntentSaved)
                {
                    actionState.StateProgress.Push(StepStateNames.LookingUpState);
                    await this.actionStore.UpdateActionStateAsync(actionState);
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "action state has been updated");
                }

                action = new StopNodeFromFASAction(this.stateManager, this.Partition, actionState, this.stoppedNodeTable, this.requestTimeout, this.operationTimeout);
            }
            else
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Unknown actionType");
            }

            return action;
        }
    }
}
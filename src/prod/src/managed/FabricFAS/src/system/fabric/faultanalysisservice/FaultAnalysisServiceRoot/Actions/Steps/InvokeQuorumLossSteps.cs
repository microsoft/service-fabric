// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions.Steps
{
    using System.Collections.Generic;
    using System.Fabric.FaultAnalysis.Service.Actions;
    using System.Fabric.FaultAnalysis.Service.Test;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Common;
    using Description;
    using Fabric.Common;
    using Query;
    using InvokeQuorumLossAction = System.Fabric.FaultAnalysis.Service.Actions.InvokeQuorumLossAction;
    using TestabilityTrace = System.Fabric.Common.TestabilityTrace;    

    internal static class QuorumLossStepsFactory
    {
        public const string BehaviorNamePrefix = "BlockStatefulServiceReopen_";

        public static async Task RemoveUTAsync(FabricClient fabricClient, ActionStateBase state, TimeSpan requestTimeout, TimeSpan operationTimeout, CancellationToken cancellationToken)
        {
            InvokeQuorumLossState invokeQuorumLossState = Convert(state);
            Guid partitionId = invokeQuorumLossState.Info.PartitionId;

            List<Task> tasks = new List<Task>();

            if (invokeQuorumLossState.Info.UnreliableTransportInfo != null)
            {
                foreach (Tuple<string, string> info in invokeQuorumLossState.Info.UnreliableTransportInfo)
                {
                    UnreliableTransportBehavior behavior = new UnreliableTransportBehavior("*", "StatefulServiceReopen");
                    behavior.AddFilterForPartitionId(partitionId);
                    string nodeName = info.Item1;
                    string behaviorName = info.Item2;

                    TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - Cleaning up behavior={1}", state.OperationId, behaviorName);

                    Task task = FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => fabricClient.TestManager.RemoveUnreliableTransportBehaviorAsync(
                            nodeName,
                            behaviorName,
                            requestTimeout,
                            cancellationToken),
                        FabricClientRetryErrors.RemoveUnreliableTransportBehaviorErrors.Value,
                        operationTimeout,
                        cancellationToken);
                    tasks.Add(task);
                }

                await Task.WhenAll(tasks).ConfigureAwait(false);

                // TODO: Wait for some time so that the unreliable transport behavior can be read from the files.
                // Bug#2271465 - Unreliable transport through API should return only once the behavior has been successfully applied
                await Task.Delay(TimeSpan.FromSeconds(5.0), cancellationToken).ConfigureAwait(false);
            }
        }

        public static StepBase GetStep(
            StepStateNames stateName,
            FabricClient fabricClient,
            ActionStateBase actionState,
            InvokeQuorumLossAction action,
            TimeSpan requestTimeout,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken)
        {
            StepBase step = null;
            InvokeQuorumLossState state = Convert(actionState);

            if (stateName == StepStateNames.LookingUpState)
            {
                step = new QuorumLossStepsFactory.LookingUpState(fabricClient, state, requestTimeout, operationTimeout, action.PartitionSelector, action.QuorumLossMode);
            }
            else if (stateName == StepStateNames.PerformingActions)
            {
                step = new QuorumLossStepsFactory.PerformingActions(fabricClient, state, requestTimeout, operationTimeout, action.PartitionSelector);
            }
            else if (stateName == StepStateNames.CompletedSuccessfully)
            {
                // done - but then this method should not have been called    
                TestabilityTrace.TraceSource.WriteError(StepBase.TraceType, "{0} - GetStep() should not have been called when the state nme is CompletedSuccessfully", actionState.OperationId);
                ReleaseAssert.Failfast("GetStep() should not have been called when the state nme is CompletedSuccessfully");
            }
            else
            {
                string error = string.Format(CultureInfo.InvariantCulture, "{0} - Unexpected state name={1}", actionState.OperationId, stateName);
                TestabilityTrace.TraceSource.WriteError(StepBase.TraceType, "{0}", error);
                ReleaseAssert.Failfast(error);
            }

            return step;
        }

        public static InvokeQuorumLossState Convert(ActionStateBase actionState)
        {
            InvokeQuorumLossState invokeQuorumLossState = actionState as InvokeQuorumLossState;
            if (invokeQuorumLossState == null)
            {
                throw new InvalidCastException("State object could not be converted");
            }

            return invokeQuorumLossState;
        }

        internal class LookingUpState : StepBase
        {
            private readonly PartitionSelector partitionSelector;
            private readonly QuorumLossMode quorumLossMode;

            public LookingUpState(FabricClient fabricClient, InvokeQuorumLossState state, TimeSpan requestTimeout, TimeSpan operationTimeout, PartitionSelector partitionSelector, QuorumLossMode quorumLossMode)
                : base(fabricClient, state, requestTimeout, operationTimeout)
            {
                this.partitionSelector = partitionSelector;
                this.quorumLossMode = quorumLossMode;
            }

            public override StepStateNames StepName
            {
                get { return StepStateNames.LookingUpState; }
            }

            public override async Task<ActionStateBase> RunAsync(CancellationToken cancellationToken, ServiceInternalFaultInfo serviceInternalFaultInfo)
            {
                InvokeQuorumLossState state = Convert(this.State);

                // get info about the service so we can check type and trss                                
                ServiceDescription result = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => this.FabricClient.ServiceManager.GetServiceDescriptionAsync(
                        this.partitionSelector.ServiceName,
                        this.RequestTimeout,
                        cancellationToken),
                    this.OperationTimeout,
                    cancellationToken).ConfigureAwait(false);

                if (result.Kind != ServiceDescriptionKind.Stateful)
                {
                    // The message in the first arg is only for debugging, it is not returned to the user.
                    throw new FabricInvalidForStatelessServicesException("FabricInvalidForStatelessServicesException", FabricErrorCode.InvalidForStatelessServices);
                }

                StatefulServiceDescription statefulServiceDescription = result as StatefulServiceDescription;
                ReleaseAssert.AssertIf(statefulServiceDescription == null, string.Format(CultureInfo.InvariantCulture, "{0} - Service is not a stateful service", this.State.OperationId));

                if (!statefulServiceDescription.HasPersistedState)
                {
                    // The message in the first arg is only for debugging, it is not returned to the user.
                    throw new FabricOnlyValidForStatefulPersistentServicesException("This is only valid for stateful persistent services", FabricErrorCode.OnlyValidForStatefulPersistentServices);
                }

                SelectedPartition targetPartition = await FaultAnalysisServiceUtility.GetSelectedPartitionStateAsync(
                    this.FabricClient,
                    this.partitionSelector,
                    this.RequestTimeout,
                    this.OperationTimeout,
                    cancellationToken).ConfigureAwait(false);

                Guid partitionId = targetPartition.PartitionId;

                // get data about replicas in that partition
                ServiceReplicaList replicasResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => this.FabricClient.QueryManager.GetReplicaListAsync(
                        partitionId,
                        0,
                        this.RequestTimeout,
                        cancellationToken),
                    this.OperationTimeout,
                    cancellationToken).ConfigureAwait(false);

                List<StatefulServiceReplica> tempReplicas = new List<StatefulServiceReplica>();
                foreach (var replica in replicasResult)
                {
                    StatefulServiceReplica statefulReplica = replica as StatefulServiceReplica;
                    ReleaseAssert.AssertIf(statefulReplica == null, "Expected stateful replica");
                    tempReplicas.Add(statefulReplica);
                }

                List<StatefulServiceReplica> targetReplicas = null;
                if (this.quorumLossMode == QuorumLossMode.AllReplicas)
                {
                    targetReplicas = tempReplicas.Where(r => r.ReplicaRole == ReplicaRole.Primary || r.ReplicaRole == ReplicaRole.ActiveSecondary).ToList();
                }
                else if (this.quorumLossMode == QuorumLossMode.QuorumReplicas)
                {
                    targetReplicas = FaultAnalysisServiceUtility.GetReplicasForPartialLoss(state.OperationId, tempReplicas);
                }
                else
                {
                    throw FaultAnalysisServiceUtility.CreateException(StepBase.TraceType, Interop.NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG, Strings.StringResources.Error_UnsupportedQuorumLossMode);
                }

                if (targetReplicas == null)
                {
                    // This will cause the command to rollback and retry
                    throw new FabricTransientException("The operation could not be performed, please retry", FabricErrorCode.NotReady);
                }

                List<string> targetNodes = new List<string>();
                foreach (var replica in targetReplicas)
                {
                    targetNodes.Add(replica.NodeName);
                }

                List<Tuple<string, string>> unreliableTransportInfoList = new List<Tuple<string, string>>();
                foreach (string nodeName in targetNodes)
                {
                    UnreliableTransportBehavior behavior = new UnreliableTransportBehavior("*", "StatefulServiceReopen");
                    behavior.AddFilterForPartitionId(partitionId);

                    // ApplyingUnreliableTransport.BehaviorNamePrefix + nodeName;
                    string behaviorName = this.CreateBehaviorName(nodeName); 

                    unreliableTransportInfoList.Add(new Tuple<string, string>(nodeName, behaviorName));
                }

                state.StateProgress.Push(StepStateNames.PerformingActions);

                state.Info.PartitionId = partitionId;
                state.Info.ReplicaIds = targetReplicas.Select(r => r.Id).ToList();
                state.Info.UnreliableTransportInfo = unreliableTransportInfoList;

                return state;
            }

            public override Task CleanupAsync(CancellationToken cancellationToken)
            {
                // debug - remove later           
                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - Cleanup for RemovingUnreliableTransport", this.State.OperationId);
                return base.CleanupAsync(cancellationToken);
            }

            private string CreateBehaviorName(string nodeName)
            {
                return QuorumLossStepsFactory.BehaviorNamePrefix + nodeName + "_" + this.State.OperationId;
            }
        }

        internal class PerformingActions : StepBase
        {
            private readonly PartitionSelector partitionSelector;

            public PerformingActions(FabricClient fabricClient, InvokeQuorumLossState state, TimeSpan requestTimeout, TimeSpan operationTimeout, PartitionSelector partitionSelector)
                : base(fabricClient, state, requestTimeout, operationTimeout)
            {
                this.partitionSelector = partitionSelector;
            }

            public override StepStateNames StepName
            {
                get { return StepStateNames.PerformingActions; }
            }

            public override async Task<ActionStateBase> RunAsync(CancellationToken cancellationToken, ServiceInternalFaultInfo serviceInternalFaultInfo)
            {
                InvokeQuorumLossState state = Convert(this.State);

                Guid partitionId = state.Info.PartitionId;
                List<Tuple<string, string>> unreliableTransportInfo = state.Info.UnreliableTransportInfo;
                List<long> targetReplicas = state.Info.ReplicaIds;

                var unreliableTransportTaskList = new List<Task>();
                List<Tuple<string, string>> unreliableTransportInfoList = new List<Tuple<string, string>>();
                foreach (Tuple<string, string> ut in unreliableTransportInfo)
                {
                    string nodeName = ut.Item1;
                    string behaviorName = ut.Item2;

                    System.Fabric.Common.UnreliableTransportBehavior behavior = new System.Fabric.Common.UnreliableTransportBehavior("*", "StatefulServiceReopen");
                    behavior.AddFilterForPartitionId(partitionId);

                    TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - applying '{1}'", this.State.OperationId, behaviorName);

                    unreliableTransportTaskList.Add(FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => this.FabricClient.TestManager.AddUnreliableTransportBehaviorAsync(
                            nodeName,
                            behaviorName,
                            behavior,
                            this.RequestTimeout,
                            cancellationToken),
                        this.OperationTimeout,
                        cancellationToken));
                }

                await Task.WhenAll(unreliableTransportTaskList).ConfigureAwait(false);

                // Bug#2271465 - Unreliable transport through API should return only once the behavior has been successfully applied
                await Task.Delay(TimeSpan.FromSeconds(5.0), cancellationToken).ConfigureAwait(false);

                List<Task> tasks = new List<Task>();

                foreach (long replicaId in targetReplicas)
                {
                    ReplicaSelector replicaSelector = ReplicaSelector.ReplicaIdOf(PartitionSelector.PartitionIdOf(this.partitionSelector.ServiceName, partitionId), replicaId);

                    TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - faulting replica with id={1}", this.State.OperationId, replicaId);
                    Task task = FaultAnalysisServiceUtility.RestartReplicaAsync(this.FabricClient, replicaSelector, CompletionMode.DoNotVerify, this.RequestTimeout, this.OperationTimeout, cancellationToken);
                    tasks.Add(task);
                }

                await Task.WhenAll(tasks).ConfigureAwait(false);

                ActionTest.PerformInternalServiceFaultIfRequested(this.State.OperationId, serviceInternalFaultInfo, this.State, cancellationToken, true);

                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - keeping partition in quorum loss for '{1}'", this.State.OperationId, state.Info.QuorumLossDuration);
                await Task.Delay(state.Info.QuorumLossDuration, cancellationToken).ConfigureAwait(false);

                TimeoutHelper timeoutHelper = new TimeoutHelper(this.OperationTimeout);

                bool conditionSatisfied = false;

                int quorumLossCheckRetries = FASConstants.QuorumLossCheckRetryCount;
                do
                {
                    TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - checking PartitionStatus", this.State.OperationId);
                    ServicePartitionList partitionsResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => this.FabricClient.QueryManager.GetPartitionListAsync(
                            this.partitionSelector.ServiceName,
                            null,
                            this.RequestTimeout,
                            cancellationToken),
                        this.OperationTimeout,
                        cancellationToken).ConfigureAwait(false);

                    foreach (StatefulServicePartition partition in partitionsResult)
                    {
                        if (partition.PartitionInformation.Id == partitionId)
                        {
                            if (partition.PartitionStatus == ServicePartitionStatus.InQuorumLoss)
                            {
                                conditionSatisfied = true;
                                break;
                            }
                        }
                    }

                    await AsyncWaiter.WaitAsync(TimeSpan.FromSeconds(5), cancellationToken).ConfigureAwait(false);
                }
                while (!conditionSatisfied && quorumLossCheckRetries-- > 0);

                if (!conditionSatisfied)
                {
                    string error = string.Format(CultureInfo.InvariantCulture, "{0} - Service could not induce quorum loss for service '{1}', partition '{2}'. Please retry", this.State.OperationId, this.partitionSelector.ServiceName, partitionId);
                    TestabilityTrace.TraceSource.WriteWarning(StepBase.TraceType, error);
                    
                    throw new FabricTransientException("The operation could not be performed, please retry", FabricErrorCode.NotReady);
                }

                await QuorumLossStepsFactory.RemoveUTAsync(this.FabricClient, this.State, this.RequestTimeout, this.OperationTimeout, cancellationToken);

                state.StateProgress.Push(StepStateNames.CompletedSuccessfully);

                return state;
            }

            public override Task CleanupAsync(CancellationToken cancellationToken)
            {
                // debug - remove later           
                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "Enter Cleanup for RemovingUnreliableTransport");

                return QuorumLossStepsFactory.RemoveUTAsync(this.FabricClient, this.State, this.RequestTimeout, this.OperationTimeout, cancellationToken);
            }
        }
    }
}
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
    using InvokeDataLossAction = System.Fabric.FaultAnalysis.Service.Actions.InvokeDataLossAction;
    using TestabilityTrace = System.Fabric.Common.TestabilityTrace;

    internal static class DataLossStepsFactory
    {
        public static StepBase GetStep(
            StepStateNames stateName,
            FabricClient fabricClient,
            ActionStateBase actionState,
            InvokeDataLossAction action,
            TimeSpan requestTimeout,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken)
        {
            StepBase step = null;
            InvokeDataLossState invokeDataLossState = Convert(actionState);

            StepStateNames prevContext = actionState.StateProgress.Peek();

            if (stateName == StepStateNames.LookingUpState)
            {
                step = new DataLossStepsFactory.LookingUpState(fabricClient, invokeDataLossState, requestTimeout, operationTimeout, action.PartitionSelector);
            }
            else if (stateName == StepStateNames.PerformingActions)
            {
                step = new DataLossStepsFactory.PerformingActions(fabricClient, invokeDataLossState, requestTimeout, operationTimeout, action.PartitionSelector, action.DataLossCheckWaitDurationInSeconds, action.DataLossCheckPollIntervalInSeconds, action.ReplicaDropWaitDurationInSeconds);
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

        public static InvokeDataLossState Convert(ActionStateBase actionState)
        {
            InvokeDataLossState invokeDataLossState = actionState as InvokeDataLossState;
            if (invokeDataLossState == null)
            {
                throw new InvalidCastException("State object could not be converted");
            }

            return invokeDataLossState;
        }

        internal class LookingUpState : StepBase
        {
            private readonly PartitionSelector partitionSelector;

            public LookingUpState(FabricClient fabricClient, InvokeDataLossState state, TimeSpan requestTimeout, TimeSpan operationTimeout, PartitionSelector partitionSelector)
                : base(fabricClient, state, requestTimeout, operationTimeout)
            {
                this.partitionSelector = partitionSelector;
            }

            public override StepStateNames StepName
            {
                get { return StepStateNames.LookingUpState; }
            }

            public override async Task<ActionStateBase> RunAsync(CancellationToken cancellationToken, ServiceInternalFaultInfo serviceInternalFaultInfo)
            {
                InvokeDataLossState state = Convert(this.State);

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

                int targetReplicaSetSize = (result as StatefulServiceDescription).TargetReplicaSetSize;

                SelectedPartition targetPartition = await FaultAnalysisServiceUtility.GetSelectedPartitionStateAsync(
                    this.FabricClient,
                    this.partitionSelector,
                    this.RequestTimeout,
                    this.OperationTimeout,
                    cancellationToken).ConfigureAwait(false);

                Guid partitionId = targetPartition.PartitionId;

                long preDataLossNumber = 0;

                ServicePartitionList partitionsResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                   () => this.FabricClient.QueryManager.GetPartitionListAsync(
                       this.partitionSelector.ServiceName,
                       null,
                       this.RequestTimeout,
                       cancellationToken),
                   this.OperationTimeout,
                   cancellationToken).ConfigureAwait(false);

                bool partitionFound = false;
                foreach (StatefulServicePartition partition in partitionsResult)
                {
                    if (partition.PartitionInformation.Id == partitionId)
                    {
                        preDataLossNumber = partition.PrimaryEpoch.DataLossNumber;
                        partitionFound = true;
                        break;
                    }
                }

                if (!partitionFound)
                {
                    throw new FabricException(StringHelper.Format(StringResources.Error_PartitionNotFound), FabricErrorCode.PartitionNotFound);
                }

                ServiceReplicaList failoverManagerReplicasResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => this.FabricClient.QueryManager.GetReplicaListAsync(
                        FASConstants.FmPartitionId,
                        0,
                        this.RequestTimeout,
                        cancellationToken),
                    this.OperationTimeout,
                    cancellationToken).ConfigureAwait(false);

                string failoverManagerPrimaryNodeName = string.Empty;
                var readyFMReplicas = failoverManagerReplicasResult.Where(r => r.ReplicaStatus == ServiceReplicaStatus.Ready).ToArray();
                foreach (var replica in readyFMReplicas)
                {
                    StatefulServiceReplica statefulReplica = replica as StatefulServiceReplica;
                    ReleaseAssert.AssertIf(statefulReplica == null, "FM Replica is not a stateful replica");
                    if (statefulReplica.ReplicaRole == ReplicaRole.Primary)
                    {
                        failoverManagerPrimaryNodeName = replica.NodeName;
                    }
                }

                if (string.IsNullOrEmpty(failoverManagerPrimaryNodeName))
                {
                    throw new FabricException(StringHelper.Format(StringResources.Error_PartitionPrimaryNotReady, "FailoverManager"), FabricErrorCode.NotReady);
                }

                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - FM primary location={1}", this.State.OperationId, failoverManagerPrimaryNodeName);
                string behaviorName = "BlockDoReconfiguration_" + this.State.OperationId;
                List<Tuple<string, string>> unreliableTransportInfo = new List<Tuple<string, string>>();
                unreliableTransportInfo.Add(new Tuple<string, string>(failoverManagerPrimaryNodeName, behaviorName));

                state.StateProgress.Push(StepStateNames.PerformingActions);
                state.Info.DataLossNumber = preDataLossNumber;
                state.Info.NodeName = failoverManagerPrimaryNodeName;
                state.Info.PartitionId = partitionId;
                state.Info.UnreliableTransportInfo = unreliableTransportInfo;
                state.Info.TargetReplicaSetSize = targetReplicaSetSize;
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
            private readonly PartitionSelector partitionSelector;
            private readonly int dataLossCheckWaitDurationInSeconds;
            private readonly int dataLossCheckPollIntervalInSeconds;
            private readonly int replicaDropWaitDurationInSeconds;

            public PerformingActions(FabricClient fabricClient, InvokeDataLossState state, TimeSpan requestTimeout, TimeSpan operationTimeout, PartitionSelector partitionSelector, int dataLossCheckWaitDurationInSeconds, int dataLossCheckPollIntervalInSeconds, int replicaDropWaitDurationInSeconds)
                : base(fabricClient, state, requestTimeout, operationTimeout)
            {
                this.partitionSelector = partitionSelector;
                this.dataLossCheckWaitDurationInSeconds = dataLossCheckWaitDurationInSeconds;
                this.dataLossCheckPollIntervalInSeconds = dataLossCheckPollIntervalInSeconds;
                this.replicaDropWaitDurationInSeconds = replicaDropWaitDurationInSeconds;
            }

            public override StepStateNames StepName
            {
                get { return StepStateNames.PerformingActions; }
            }

            public static async Task RemoveUnreliableTransportAsync(FabricClient fabricClient, string nodeName, string behaviorName, TimeSpan requestTimeout, TimeSpan operationTimeout, CancellationToken cancellationToken)
            {
                try
                {
                    await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => fabricClient.TestManager.RemoveUnreliableTransportBehaviorAsync(
                            nodeName,
                            behaviorName,
                            requestTimeout,
                            cancellationToken),
                        FabricClientRetryErrors.RemoveUnreliableTransportBehaviorErrors.Value,
                        operationTimeout,
                        cancellationToken).ConfigureAwait(false);
                }
                catch (Exception e)
                {
                    if (e.InnerException != null)
                    {
                        FabricException ee = e.InnerException as FabricException;
                        if (ee != null)
                        {
                            TestabilityTrace.TraceSource.WriteError(StepBase.TraceType, "DEBUG, remove UT threw inner.code {0}", ee.ErrorCode);
                        }
                    }

                    throw;
                }

                // TODO: Wait for some time so that the unreliable transport behavior can be read from the files.
                // Bug#2271465 - Unreliable transport through API should return only once the behavior has been successfully applied
                await Task.Delay(TimeSpan.FromSeconds(5.0), cancellationToken).ConfigureAwait(false);
            }

            public override async Task<ActionStateBase> RunAsync(CancellationToken cancellationToken, ServiceInternalFaultInfo serviceInternalFaultInfo)
            {
                InvokeDataLossState state = Convert(this.State);

                PartitionSelector partitionSelector = state.Info.PartitionSelector;
                DataLossMode dataLossMode = state.Info.DataLossMode;
                long preDataLossNumber = state.Info.DataLossNumber;
                string failoverManagerPrimaryNodeName = state.Info.NodeName;
                Guid partitionId = state.Info.PartitionId;
                string behaviorName = state.Info.UnreliableTransportInfo.First().Item2;
                int targetReplicaSetSize = state.Info.TargetReplicaSetSize;

                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - applying UT, partitionId={1}", this.State.OperationId, partitionId);
                System.Fabric.Common.UnreliableTransportBehavior behavior = new System.Fabric.Common.UnreliableTransportBehavior("*", "DoReconfiguration");
                behavior.AddFilterForPartitionId(partitionId);

                await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => this.FabricClient.TestManager.AddUnreliableTransportBehaviorAsync(
                        failoverManagerPrimaryNodeName,
                        behaviorName,
                        behavior,
                        this.RequestTimeout,
                        cancellationToken),
                    this.OperationTimeout,
                    cancellationToken).ConfigureAwait(false);
                
                // TODO: Wait for some time so that the unreliable transport behavior can be read from the files.
                // Bug#2271465 - Unreliable transport through API should return only once the behavior has been successfully applied
                await Task.Delay(TimeSpan.FromSeconds(5.0), cancellationToken).ConfigureAwait(false);

                ServiceReplicaList replicasResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => this.FabricClient.QueryManager.GetReplicaListAsync(
                        partitionId,
                        0,
                        this.RequestTimeout,
                        cancellationToken),
                    this.OperationTimeout,
                    cancellationToken).ConfigureAwait(false);

                List<StatefulServiceReplica> replicaList = new List<StatefulServiceReplica>();
                foreach (var replica in replicasResult)
                {
                    StatefulServiceReplica statefulReplica = replica as StatefulServiceReplica;
                    ReleaseAssert.AssertIf(statefulReplica == null, "Service Replica is not of stateful type even though service is stateful");
                    replicaList.Add(statefulReplica);
                }

                // Select target replicas based on the DataLosMode
                List<StatefulServiceReplica> targets = null;
                if (dataLossMode == DataLossMode.FullDataLoss)
                {
                    targets = GetReplicasForFullDataLoss(replicaList);
                }
                else if (dataLossMode == DataLossMode.PartialDataLoss)
                {
                    targets = FaultAnalysisServiceUtility.GetReplicasForPartialLoss(state.OperationId, replicaList);
                }
                else
                {
                    throw FaultAnalysisServiceUtility.CreateException(StepBase.TraceType, Interop.NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG, Strings.StringResources.Error_UnsupportedDataLossMode);
                }

                if (targets == null)
                {
                    // This will cause the command to rollback and retry
                    throw new FabricTransientException("The operation could not be performed, please retry", FabricErrorCode.NotReady);
                }

                foreach (var replica in targets)
                {
                    TestabilityTrace.TraceSource.WriteInfo(
                        StepBase.TraceType,
                        "{0} - Removing replica {1} in partition {2} with role {3} and status {4} to induce data loss",
                        this.State.OperationId,
                        replica.Id,
                        partitionId,
                        replica.ReplicaRole,
                        replica.ReplicaStatus);

                    await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                         () => this.FabricClient.ServiceManager.RemoveReplicaAsync(
                             replica.NodeName,
                             partitionId,
                             replica.Id,
                             this.RequestTimeout,
                             cancellationToken),
                         FabricClientRetryErrors.RemoveReplicaErrors.Value,
                         this.OperationTimeout,
                         cancellationToken).ConfigureAwait(false);
                }

                ActionTest.PerformInternalServiceFaultIfRequested(this.State.OperationId, serviceInternalFaultInfo, this.State, cancellationToken, true);
                
                await this.WaitForAllTargetReplicasToGetDroppedAsync(partitionId, targets, cancellationToken).ConfigureAwait(false);

                await RemoveUnreliableTransportAsync(this.FabricClient, failoverManagerPrimaryNodeName, behaviorName, this.RequestTimeout, this.OperationTimeout, cancellationToken).ConfigureAwait(false);

                bool dataLossWasSuccessful = false;
                TimeoutHelper timeoutHelper = new TimeoutHelper(TimeSpan.FromSeconds(30));
                do
                {
                    ServicePartitionList partitionsResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => this.FabricClient.QueryManager.GetPartitionListAsync(
                            this.partitionSelector.ServiceName,
                            null,
                            this.RequestTimeout,
                            cancellationToken),
                        this.OperationTimeout,
                        cancellationToken).ConfigureAwait(false);

                    bool partitionFound = false;
                    long postDataLossNumber = 0;
                    foreach (StatefulServicePartition partition in partitionsResult)
                    {
                        if (partition.PartitionInformation.Id == partitionId)
                        {
                            postDataLossNumber = partition.PrimaryEpoch.DataLossNumber;
                            partitionFound = true;
                            break;
                        }
                    }

                    if (!partitionFound)
                    {
                        throw new FabricException(StringHelper.Format(StringResources.Error_PartitionNotFound), FabricErrorCode.PartitionNotFound);
                    }

                    TestabilityTrace.TraceSource.WriteInfo(
                        StepBase.TraceType,
                        "{0} - Checking data loss numbers for partition {1} with remaining time {2}. Current numbers {3}:{4}",
                        this.State.OperationId,
                        partitionId,
                        timeoutHelper.GetRemainingTime(),
                        preDataLossNumber,
                        postDataLossNumber);

                    if (postDataLossNumber != preDataLossNumber)
                    {
                        dataLossWasSuccessful = true;
                        break;
                    }

                    await System.Fabric.Common.AsyncWaiter.WaitAsync(TimeSpan.FromSeconds(this.dataLossCheckPollIntervalInSeconds), cancellationToken).ConfigureAwait(false);
                }
                while (timeoutHelper.GetRemainingTime() > TimeSpan.Zero);

                if (!dataLossWasSuccessful)
                {
                    // This is only viewable internally for debug.  This will cause a retry of the whole flow.
                    string error = string.Format(
                        CultureInfo.InvariantCulture,
                        "{0} - Service could not induce data loss for service '{1}' partition '{2}' in '{3}' Please retry",
                        this.State.OperationId,
                        partitionSelector.ServiceName,
                        partitionId,
                        this.dataLossCheckWaitDurationInSeconds);
                    TestabilityTrace.TraceSource.WriteWarning(StepBase.TraceType, error);
                    throw new FabricTransientException("The operation could not be performed, please retry", FabricErrorCode.NotReady);
                }

                state.StateProgress.Push(StepStateNames.CompletedSuccessfully);

                return state;
            }

            public override Task CleanupAsync(CancellationToken cancellationToken)
            {
                // debug - remove later
                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - Enter Cleanup for PerformingAction", this.State.OperationId);
                InvokeDataLossState state = Convert(this.State);

                string behaviorName = state.Info.UnreliableTransportInfo.First().Item2;
                return RemoveUnreliableTransportAsync(this.FabricClient, state.Info.NodeName, behaviorName, this.RequestTimeout, this.OperationTimeout, cancellationToken);
            }

            // Select all P, S, I that are not Down and not Dropped
            private static List<StatefulServiceReplica> GetReplicasForFullDataLoss(List<StatefulServiceReplica> replicaList)
            {
                List<StatefulServiceReplica> targetReplicas = new List<StatefulServiceReplica>();
                foreach (StatefulServiceReplica replica in replicaList)
                {
                    if ((FaultAnalysisServiceUtility.IsPrimaryOrSecondary(replica) || (replica.ReplicaRole == ReplicaRole.IdleSecondary))
                        && FaultAnalysisServiceUtility.IsReplicaUp(replica))
                    {
                        targetReplicas.Add(replica);
                    }
                }

                return targetReplicas;
            }

            private async Task WaitForAllTargetReplicasToGetDroppedAsync(Guid partitionId, List<StatefulServiceReplica> targets, CancellationToken cancellationToken)
            {
                bool allTargetReplicasRemoved = false;
                TimeoutHelper timeoutHelper = new TimeoutHelper(TimeSpan.FromSeconds(this.replicaDropWaitDurationInSeconds)); 
                do
                {
                    ServiceReplicaList queriedReplicasAfterFault = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => this.FabricClient.QueryManager.GetReplicaListAsync(
                            partitionId,
                            0,
                            this.RequestTimeout,
                            cancellationToken),
                        this.OperationTimeout,
                        cancellationToken).ConfigureAwait(false);

                    allTargetReplicasRemoved = this.AllTargetReplicasDropped(targets, queriedReplicasAfterFault);
                    if (!allTargetReplicasRemoved)
                    {
                        await Task.Delay(TimeSpan.FromMilliseconds(500), cancellationToken).ConfigureAwait(false);
                    }
                }
                while (!allTargetReplicasRemoved && timeoutHelper.GetRemainingTime() > TimeSpan.Zero);

                if (!allTargetReplicasRemoved)
                {                 
                    string error = string.Format(
                        CultureInfo.InvariantCulture,
                        "{0} - Service could not drop all replicas for '{1}' partition '{2}' in '{3}'.  See traces above for which replicas did not get dropped.  Retrying",
                        this.State.OperationId,
                        this.partitionSelector.ServiceName,
                        partitionId,
                        this.dataLossCheckWaitDurationInSeconds);

                    TestabilityTrace.TraceSource.WriteWarning(StepBase.TraceType, error);

                    // The will cause a retry of the whole operation
                    throw new FabricTransientException("The operation could not be performed, retrying.  Issue: " + error, FabricErrorCode.NotReady);
                }
            }

            private bool AllTargetReplicasDropped(List<StatefulServiceReplica> targetReplicas, ServiceReplicaList queriedReplicasAfterFault)
            {           
                // For each target replica, look for dropped or not present
                foreach (StatefulServiceReplica r in targetReplicas)
                {
                    foreach (StatefulServiceReplica s in queriedReplicasAfterFault)
                    {
                        // If it is either:
                        //   - not present in the query OR
                        //   - present in the query and it has a ReplicaStatus Dropped
                        // that is what we want.  Otherwise, break out and retry the query.
                        if (r.ReplicaId == s.ReplicaId)
                        {
                            if (s.ReplicaStatus != ServiceReplicaStatus.Dropped)
                            {
                                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - Replica with id={1} is not Dropped and has status={2}", this.State.OperationId, s.ReplicaId, s.ReplicaStatus);
                                return false;
                            }
                        }
                    }                    
                }
                
                return true;
            }
        }
    }
}
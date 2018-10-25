// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions.Steps
{    
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.FaultAnalysis.Service.Actions;
    using System.Fabric.FaultAnalysis.Service.Common;
    using System.Fabric.FaultAnalysis.Service.Test;
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Interop;
    using RestartPartitionAction = System.Fabric.FaultAnalysis.Service.Actions.RestartPartitionAction;
    using TestabilityTrace = System.Fabric.Common.TestabilityTrace;

    internal static class RestartPartitionStepsFactory
    {
        public static StepBase GetStep(
            StepStateNames stateName,
            FabricClient fabricClient,
            ActionStateBase actionState,
            RestartPartitionAction action,
            TimeSpan requestTimeout,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken)
        {
            StepBase step = null;
            RestartPartitionState restartPartitionState = Convert(actionState);

            if (stateName == StepStateNames.LookingUpState)
            {
                step = new LookingUpStep(fabricClient, restartPartitionState, requestTimeout, operationTimeout, action.PartitionSelector, action.RestartPartitionMode);
            }
            else if (stateName == StepStateNames.PerformingActions)
            {
                step = new RestartingSelectedReplicas(fabricClient, restartPartitionState, requestTimeout, operationTimeout, action.RestartPartitionMode);
            }
            else if (stateName == StepStateNames.CompletedSuccessfully)  
            {
                // done - but then this method should not have been called               
                ReleaseAssert.Failfast("GetStep() should not have been called when the state nme is CompletedSuccessfully");
            }
            else
            {
                ReleaseAssert.Failfast(string.Format(CultureInfo.InvariantCulture, "Unexpected state name={0}", stateName));
            }

            return step;
        }

        public static RestartPartitionState Convert(ActionStateBase actionState)
        {
            RestartPartitionState restartPartitionState = actionState as RestartPartitionState;
            if (restartPartitionState == null)
            {
                throw new InvalidCastException("State object could not be converted");
            }

            return restartPartitionState;
        }

        internal class LookingUpStep : StepBase
        {
            private readonly PartitionSelector partitionSelector;
            private readonly RestartPartitionMode restartPartitionMode;

            public LookingUpStep(FabricClient fabricClient, RestartPartitionState state, TimeSpan requestTimeout, TimeSpan operationTimeout, PartitionSelector partitionSelector, RestartPartitionMode restartPartitionMode)
                : base(fabricClient, state, requestTimeout, operationTimeout)
            {
                this.partitionSelector = partitionSelector;
                this.restartPartitionMode = restartPartitionMode;
            }

            public override StepStateNames StepName
            {
                get { return StepStateNames.LookingUpState; }
            }

            public override async Task<ActionStateBase> RunAsync(CancellationToken cancellationToken, ServiceInternalFaultInfo serviceInternalFaultInfo)
            {
                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "Inside CollectingState, service={0}", this.partitionSelector.ServiceName);
                RestartPartitionState state = Convert(this.State);

                // Get service info and validate if the parameters are valid                
                ServiceDescription result = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => this.FabricClient.ServiceManager.GetServiceDescriptionAsync(
                        this.partitionSelector.ServiceName,
                        this.RequestTimeout,
                        cancellationToken),
                    this.OperationTimeout,
                    cancellationToken).ConfigureAwait(false);

                if (result.Kind != ServiceDescriptionKind.Stateful && this.restartPartitionMode == RestartPartitionMode.OnlyActiveSecondaries)
                {
                    // The message in the first arg is only for debugging, it is not returned to the user.
                    string debugText = string.Format(CultureInfo.InvariantCulture, "RestartPartition: for stateless services only RestartPartitionMode.AllReplicasOrInstances is valid");
                    TestabilityTrace.TraceSource.WriteWarning(StepBase.TraceType, debugText);
                    throw FaultAnalysisServiceUtility.CreateException(StepBase.TraceType, NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG, debugText);
                }

                bool hasPersistedState = false;
                if (result.Kind == ServiceDescriptionKind.Stateful)
                {
                    StatefulServiceDescription statefulDescription = result as StatefulServiceDescription;
                    ReleaseAssert.AssertIf(statefulDescription == null, "Stateful service description is not WinFabricStatefulServiceDescription");
                    hasPersistedState = statefulDescription.HasPersistedState;
                }

                SelectedPartition targetPartition = await FaultAnalysisServiceUtility.GetSelectedPartitionStateAsync(
                    this.FabricClient,
                    this.partitionSelector,
                    this.RequestTimeout,
                    this.OperationTimeout,
                    cancellationToken).ConfigureAwait(false);

                Guid partitionId = targetPartition.PartitionId;

                // get replicas for target
                ServiceReplicaList replicasResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => this.FabricClient.QueryManager.GetReplicaListAsync(
                        partitionId,
                        0,
                        this.RequestTimeout,
                        cancellationToken),
                    this.OperationTimeout,
                    cancellationToken).ConfigureAwait(false);

                // get replicas for fm in order to get the primary
                ServiceReplicaList failoverManagersReplicasResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => this.FabricClient.QueryManager.GetReplicaListAsync(
                        FASConstants.FmPartitionId,
                        0,
                        this.RequestTimeout,
                        cancellationToken),
                    this.OperationTimeout,
                    cancellationToken).ConfigureAwait(false);

                string failoverManagerPrimaryNodeName = string.Empty;
                var readyFMReplicas = failoverManagersReplicasResult.Where(r => r.ReplicaStatus == ServiceReplicaStatus.Ready).ToArray();
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

                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - FM primary is at node={1}", this.State.OperationId, failoverManagerPrimaryNodeName);
                string behaviorName = RestartingSelectedReplicas.UTBehaviorPrefixName + "_" + this.State.OperationId;
                List<Tuple<string, string>> unreliableTransportInfo = new List<Tuple<string, string>>();
                unreliableTransportInfo.Add(new Tuple<string, string>(failoverManagerPrimaryNodeName, behaviorName));

                state.StateProgress.Push(StepStateNames.PerformingActions);
                state.Info.PartitionId = partitionId;
                state.Info.NodeName = failoverManagerPrimaryNodeName;
                state.Info.HasPersistedState = hasPersistedState;
                state.Info.UnreliableTransportInfo = unreliableTransportInfo;

                return state;
            }

            public override Task CleanupAsync(CancellationToken cancellationToken)
            {
                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "Enter Cleanup for CollectingState");
                return base.CleanupAsync(cancellationToken);
            }
        }

        internal class RestartingSelectedReplicas : StepBase
        {
            public const string UTBehaviorPrefixName = "BlockReconfiguration";

            private readonly RestartPartitionMode restartPartitionMode;

            public RestartingSelectedReplicas(FabricClient fabricClient, RestartPartitionState state, TimeSpan requestTimeout, TimeSpan operationTimeout, RestartPartitionMode restartPartitionMode)
                : base(fabricClient, state, requestTimeout, operationTimeout)
            {
                this.restartPartitionMode = restartPartitionMode;
            }

            public override StepStateNames StepName
            {
                get { return StepStateNames.PerformingActions; }
            }

            public static async Task RemoveUnreliableTransportAsync(ActionStateBase state, FabricClient fabricClient, TimeSpan requestTimeout, TimeSpan operationTimeout, CancellationToken cancellationToken)
            {
                RestartPartitionState restartPartitionState = Convert(state);
                if (restartPartitionState.Info.UnreliableTransportInfo != null)
                {
                    string behaviorName = restartPartitionState.Info.UnreliableTransportInfo.First().Item2;
                    string failoverManagerPrimaryNodeName = restartPartitionState.Info.NodeName;
                    TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - Enter Cleanup for '{1}', fmPrimaryNodeName={2}, behavior name={3}", state.OperationId, behaviorName, failoverManagerPrimaryNodeName, behaviorName);

                    await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => fabricClient.TestManager.RemoveUnreliableTransportBehaviorAsync(
                            failoverManagerPrimaryNodeName,
                            behaviorName,
                            requestTimeout,
                            cancellationToken),
                        FabricClientRetryErrors.RemoveUnreliableTransportBehaviorErrors.Value,
                        operationTimeout,
                        cancellationToken).ConfigureAwait(false);

                    // TODO: Wait for some time so that the unreliable transport behavior can be read from the files.
                    // Bug#2271465 - Unreliable transport through API should return only once the behavior has been successfully applied
                    await Task.Delay(TimeSpan.FromSeconds(5.0), cancellationToken).ConfigureAwait(false);
                }                
            }

            public override async Task<ActionStateBase> RunAsync(CancellationToken cancellationToken, ServiceInternalFaultInfo serviceInternalFaultInfo)
            {
                RestartPartitionState state = Convert(this.State);

                Guid partitionId = state.Info.PartitionId;
                bool hasPersistedState = state.Info.HasPersistedState;
                string failoverManagerPrimaryNodeName = state.Info.NodeName;
                string behaviorName = state.Info.UnreliableTransportInfo.First().Item2;

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

                // Bug#2271465 - Unreliable transport through API should return only once the behavior has been successfully applied
                await Task.Delay(TimeSpan.FromSeconds(5.0), cancellationToken).ConfigureAwait(false);

                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - applied UT on partitionId {1}, node={2}", this.State.OperationId, partitionId, failoverManagerPrimaryNodeName);
                ServiceReplicaList replicasResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => this.FabricClient.QueryManager.GetReplicaListAsync(
                        partitionId,
                        0,
                        this.RequestTimeout,
                        cancellationToken),
                    this.OperationTimeout,
                    cancellationToken).ConfigureAwait(false);

                var stableReplicasToRestart = replicasResult.Where(r => r.ReplicaStatus == ServiceReplicaStatus.Ready).ToArray();

                foreach (var replica in stableReplicasToRestart)
                {
                    if (this.restartPartitionMode == RestartPartitionMode.OnlyActiveSecondaries)
                    {
                        StatefulServiceReplica statefulReplica = replica as StatefulServiceReplica;
                        ReleaseAssert.AssertIf(statefulReplica == null, "Stateful service replica is not StatefulServiceReplica");
                        if (statefulReplica.ReplicaRole == ReplicaRole.Primary)
                        {
                            continue;
                        }
                    }

                    TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - restarting replica partition={1}, node={2}, replica id={3}", this.State.OperationId, partitionId, replica.NodeName, replica.Id);                    
                    if (hasPersistedState)
                    {
                        await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                            () => this.FabricClient.ServiceManager.RestartReplicaAsync(
                                replica.NodeName,
                                partitionId,
                                replica.Id,
                                this.RequestTimeout,
                                cancellationToken),
                            FabricClientRetryErrors.RestartReplicaErrors.Value,
                            this.OperationTimeout,
                            cancellationToken).ConfigureAwait(false);
                    }
                    else
                    {
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
                }

                ActionTest.PerformInternalServiceFaultIfRequested(this.State.OperationId, serviceInternalFaultInfo, this.State, cancellationToken, true);

                await RemoveUnreliableTransportAsync(this.State, this.FabricClient, this.RequestTimeout, this.OperationTimeout, cancellationToken);
                state.StateProgress.Push(StepStateNames.CompletedSuccessfully);

                return state;
            }

            public override Task CleanupAsync(CancellationToken cancellationToken)
            {
                return RemoveUnreliableTransportAsync(this.State, this.FabricClient, this.RequestTimeout, this.OperationTimeout, cancellationToken);
            }
        }
    }
}
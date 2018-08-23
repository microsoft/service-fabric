// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Fabric.Query;
    using System.Fabric.Result;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;

    internal class InvokeQuorumLossAction : FabricTestAction<InvokeQuorumLossResult>
    {
        public InvokeQuorumLossAction(PartitionSelector partitionSelector, QuorumLossMode QuorumLossMode, TimeSpan QuorumLossDuration)
        {
            this.PartitionSelector = partitionSelector;
            this.QuorumLossMode = QuorumLossMode;
            this.QuorumLossDuration = QuorumLossDuration;
        }

        public PartitionSelector PartitionSelector { get; set; }

        public QuorumLossMode QuorumLossMode { get; set; }

        public TimeSpan QuorumLossDuration { get; set; }

        internal override Type ActionHandlerType
        {
            get { return typeof(InvokeQuorumLossActionHandler); }
        }

        private class InvokeQuorumLossActionHandler : FabricTestActionHandler<InvokeQuorumLossAction>
        {
            protected override async Task ExecuteActionAsync(FabricTestContext testContext, InvokeQuorumLossAction action, CancellationToken cancellationToken)
            {
                ThrowIf.Null(action.PartitionSelector, "PartitionSelector");

                var helper = new TimeoutHelper(action.ActionTimeout);

                // get info about the service so we can check type and trss
                ServiceDescription result = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => testContext.FabricClient.ServiceManager.GetServiceDescriptionAsync(
                        action.PartitionSelector.ServiceName, 
                        action.RequestTimeout, 
                        cancellationToken),
                    helper.GetRemainingTime(),
                    cancellationToken).ConfigureAwait(false);

                if (result.Kind != ServiceDescriptionKind.Stateful)
                {
                    throw new InvalidOperationException(StringHelper.Format(StringResources.Error_InvalidServiceTypeTestability, "QuorumLoss", "Stateful", action.PartitionSelector.ServiceName, "Stateless"));
                }

                StatefulServiceDescription statefulServiceDescription = result as StatefulServiceDescription;
                ReleaseAssert.AssertIf(statefulServiceDescription == null, "Service is not a stateful service");

                if (!statefulServiceDescription.HasPersistedState)
                {
                    throw new InvalidOperationException(StringHelper.Format(StringResources.Error_InvalidServiceTypeTestability, "QuorumLoss", "Stateful Persistent", action.PartitionSelector.ServiceName, "Stateful In-Memory Only"));
                }

                // figure out /which/ partition to select
                var getPartitionStateAction = new GetSelectedPartitionStateAction(action.PartitionSelector)
                {
                    RequestTimeout = action.RequestTimeout,
                    ActionTimeout = helper.GetRemainingTime()
                };

                await testContext.ActionExecutor.RunAsync(getPartitionStateAction, cancellationToken);
                Guid partitionId = getPartitionStateAction.Result.PartitionId;

                // get data about replicas in that partition
                ServiceReplicaList replicasResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => testContext.FabricClient.QueryManager.GetReplicaListAsync(
                            partitionId, 
                            0, 
                            action.RequestTimeout, 
                            cancellationToken),
                        helper.GetRemainingTime(),
                        cancellationToken).ConfigureAwait(false);

                var removeUTRequestList = new List<Tuple<string, string>>();
                Dictionary<Tuple<string, string>, Task> removeUTTaskDictionary = new Dictionary<Tuple<string, string>, Task>();

                try
                {
                    var stableReplicas = replicasResult.Where(r => r.ReplicaStatus == ServiceReplicaStatus.Ready).ToArray();
                    var stableReplicasToRemove = new List<StatefulServiceReplica>();
                    long replicasToRestartWithoutPrimary = 
                        action.QuorumLossMode == QuorumLossMode.AllReplicas 
                            ? stableReplicas.Length - 1
                            : FabricCluster.GetWriteQuorumSize(replicasResult.Count);
                    foreach (var replica in stableReplicas)
                    {
                        StatefulServiceReplica statefulReplica = replica as StatefulServiceReplica;
                        ReleaseAssert.AssertIf(statefulReplica == null, "Service Replica is not of stateful type even though service is stateful");
                        if (statefulReplica.ReplicaRole != ReplicaRole.Primary)
                        {
                            replicasToRestartWithoutPrimary--;
                        }

                        if (replicasToRestartWithoutPrimary >= 0 || statefulReplica.ReplicaRole == ReplicaRole.Primary)
                        {
                            stableReplicasToRemove.Add(statefulReplica);
                        }
                    }

                    // for selected replicas, block reopen so that when we restart the replica (NOT remove the replica) it doesn't come up
                    var utTaskList = new List<Task>();
                    foreach (var statefulReplica in stableReplicasToRemove)
                    {
                        string nodeName = statefulReplica.NodeName;
                        UnreliableTransportBehavior behavior = new UnreliableTransportBehavior("*", "StatefulServiceReopen");
                        behavior.AddFilterForPartitionId(partitionId);
                        string behaviorName = "BlockStatefulServiceReopen_" + nodeName;

                        removeUTRequestList.Add(new Tuple<string, string>(nodeName, behaviorName));
                        utTaskList.Add(
                                    FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                        () =>
                                        testContext.FabricClient.TestManager.AddUnreliableTransportBehaviorAsync(
                                                                    nodeName, 
                                                                    behaviorName, 
                                                                    behavior, 
                                                                    action.RequestTimeout,
                                                                    cancellationToken),
                                    helper.GetRemainingTime(),
                                    cancellationToken));
                    }

                    await Task.WhenAll(utTaskList).ConfigureAwait(false);

                    // TODO: Wait for some time so that the unreliable transport behavior can be read from the files. 
                    // Bug#2271465 - Unreliable transport through API should return only once the behavior has been successfully applied
                    await Task.Delay(TimeSpan.FromSeconds(5.0), cancellationToken);

                    var restartReplicaTaskList = new List<Task>();
                    foreach (var statefulReplica in stableReplicasToRemove)
                    {
                        ReplicaSelector replicaSelector = ReplicaSelector.ReplicaIdOf(PartitionSelector.PartitionIdOf(action.PartitionSelector.ServiceName, partitionId), statefulReplica.Id);

                        var restartReplicaAction = new RestartReplicaAction(replicaSelector)
                        {
                            CompletionMode = CompletionMode.DoNotVerify,
                            RequestTimeout = action.RequestTimeout,
                            ActionTimeout = helper.GetRemainingTime()
                        };

                        restartReplicaTaskList.Add(testContext.ActionExecutor.RunAsync(restartReplicaAction, cancellationToken));
                    }

                    await Task.WhenAll(restartReplicaTaskList).ConfigureAwait(false);
                    await AsyncWaiter.WaitAsync(action.QuorumLossDuration, cancellationToken).ConfigureAwait(false);

                    // validate
                    ServicePartitionList partitionsResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                () => testContext.FabricClient.QueryManager.GetPartitionListAsync(
                                    action.PartitionSelector.ServiceName, 
                                    null, 
                                    action.RequestTimeout, 
                                    cancellationToken),
                                FabricClientRetryErrors.GetPartitionListFabricErrors.Value,
                                helper.GetRemainingTime(),
                                cancellationToken).ConfigureAwait(false);

                    foreach (StatefulServicePartition partition in partitionsResult)
                    {
                        if (partition.PartitionInformation.Id == partitionId)
                        {
                            ReleaseAssert.AssertIf(partition.PartitionStatus != ServicePartitionStatus.InQuorumLoss, "Partition failed to be in Quorum Loss.");
                            break;
                        }
                    }

                    foreach (var removeUTParams in removeUTRequestList)
                    {
                        var currentParams = removeUTParams;
                        Task task = FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                             () => testContext.FabricClient.TestManager.RemoveUnreliableTransportBehaviorAsync(
                                 currentParams.Item1, /*nodeName*/
                                 currentParams.Item2, /*behaviorName*/
                                 action.RequestTimeout,
                                 cancellationToken),
                             FabricClientRetryErrors.RemoveUnreliableTransportBehaviorErrors.Value,
                             helper.GetRemainingTime(),
                             cancellationToken);

                        removeUTTaskDictionary[currentParams] = task;
                    }

                    await Task.WhenAll(removeUTTaskDictionary.Values).ConfigureAwait(false);

                    // TODO: Wait for some time so that the removal of this unreliable transport behavior can be read from the files.
                    // Bug#2271465 - Unreliable transport through API should return only once the behavior has been successully applied
                    await Task.Delay(TimeSpan.FromSeconds(5.0), cancellationToken);
                }
                finally
                {
                    var removeUTTaskList = new List<Task>();

                    foreach (var removeUTRequest in removeUTTaskDictionary)
                    {
                        var currentRemoveUTRequest = removeUTRequest;
                        if (currentRemoveUTRequest.Value == null || currentRemoveUTRequest.Value.IsFaulted)
                        {
                            removeUTTaskList.Add(
                                FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                 () => testContext.FabricClient.TestManager.RemoveUnreliableTransportBehaviorAsync(
                                     currentRemoveUTRequest.Key.Item1, /*nodeName*/
                                     currentRemoveUTRequest.Key.Item2, /*behaviorName*/
                                     action.RequestTimeout,
                                     cancellationToken),
                                 FabricClientRetryErrors.RemoveUnreliableTransportBehaviorErrors.Value,
                                 helper.GetRemainingTime(),
                                 cancellationToken));
                        }
                    }

                    Task.WhenAll(removeUTTaskList).Wait(cancellationToken);

                    // TODO: Wait for some time so that the removal of this unreliable transport behavior can be read from the files.
                    // Bug#2271465 - Unreliable transport through API should return only once the behavior has been successully applied
                    Task.Delay(TimeSpan.FromSeconds(5.0), cancellationToken).GetAwaiter().GetResult();
                }

                action.Result = new InvokeQuorumLossResult(getPartitionStateAction.Result);
                this.ResultTraceString = StringHelper.Format("InvokeQuorumLossAction succeeded for {0} with QuorumLossMode = {1}", partitionId, action.QuorumLossMode);
            }
        }
    }
}